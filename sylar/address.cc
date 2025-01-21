#include "address.h"
#include "log.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    template<typename T>
    static T GetMask(uint32_t prefix_len) {
        return (1 << (sizeof(T) * 8 - prefix_len)) - 1;
    }

    template<typename T>
    static uint32_t CountBytes(T value) {
        uint32_t ans = 0;
        while (value) {
            ++ans;
            value &= value - 1;
        }
        return ans;
    }

    // ----------------------------------------------------------------------------

    Address::ptr Address::Create(const sockaddr* addr) {
        if (addr == nullptr) {
            return nullptr;
        }
        switch (addr->sa_family) {
            case AF_INET:
                return IPv4Address::ptr(new IPv4Address(*(const sockaddr_in*)addr));
            case AF_INET6:
                return IPv6Address::ptr(new IPv6Address(*(const sockaddr_in6*)addr));
            default:
                return UnknownAddress::ptr(new UnknownAddress(*addr));
        }
    }

    bool Address::Lookup(std::vector<Address::ptr>& vec, const std::string& host, int family, int type, int protocol) {
        addrinfo* results;
        addrinfo hints;
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;
        hints.ai_addrlen = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        std::string node;
        const char* service = NULL;

        // 检查是否为ipv6地址
        if (!host.empty() && host[0] == '[') {
            size_t endipv6 = host.find(']', 1);
            if (endipv6 != std::string::npos) {
                node = host.substr(1, endipv6 - 1);
                if (endipv6 + 1 < host.size() && host[endipv6 + 1] == ':') {
                    service = host.c_str() + endipv6 + 2;       // 端口号/协议地址
                }
            }
        }
        // 检查是否包含端口号（ipv4、域名）
        if (node.empty()) {
            size_t colon_pos = host.find(':');
            if (colon_pos != std::string::npos) {
                node = host.substr(0, colon_pos);
                service = host.c_str() + colon_pos + 1;
            }
        }
        // 没有端口号
        if (node.empty()) {
            node = host;
        }
        int ret = getaddrinfo(node.c_str(), service, &hints, &results);
        if (ret) {
            SYLAR_LOG_DEBUG(g_logger) << "Address::Lookup getaddress(" << host << ", " << family << ", " << type
                << ", " << protocol << ") err=" << errno << " errstr=" << strerror(errno);
            return false;
        }

        for (addrinfo* cur = results; cur; cur = cur->ai_next) {
            vec.push_back(Create(cur->ai_addr));
        }
        freeaddrinfo(results);
        return true;
    }

    Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol) {
        std::vector<Address::ptr> vec;
        if (Lookup(vec, host, family, type, protocol)) {
            return vec[0];
        }
        return nullptr;
    }

    std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol) {
        std::vector<Address::ptr> vec;
        if (Lookup(vec, host, family, type, protocol)) {
            for (auto& i : vec) {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if (v) {
                    return v;
                }
            }
        }
        return nullptr;
    }

    bool Address::GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& retMap, int family) {
        ifaddrs* results;
        if (getifaddrs(&results)) {
            SYLAR_LOG_DEBUG(g_logger) << "Address::GetInterfaceAddress getifaddrs err="
                << errno << " errstr=" << strerror(errno);
            return false;
        }
        try {
            for (ifaddrs* cur = results; cur; cur = cur->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_len = 0;
                if (family != AF_UNSPEC && family != cur->ifa_addr->sa_family) {
                    continue;
                }
                switch (cur->ifa_addr->sa_family) {
                    case AF_INET: {
                        addr = Create(cur->ifa_addr);
                        uint32_t netmask = ((sockaddr_in*)cur->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                        break;
                    }
                    case AF_INET6: {
                        addr = Create(cur->ifa_addr);
                        in6_addr& netmask = ((sockaddr_in6*)cur->ifa_netmask)->sin6_addr;
                        for (int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.s6_addr[i]);
                        }
                        break;
                    }
                    default:
                        break;
                }
                if (addr) {
                    retMap.insert({ cur->ifa_name, {addr, prefix_len} });
                }
            }
        } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
            freeifaddrs(results);
            return false;
        }
        freeifaddrs(results);
        return !retMap.empty();
    }

    bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>>& vec, const std::string& name, int family) {
        if (name.empty() || name == "*") {
            if (family == AF_INET || family == AF_UNSPEC) {
                vec.emplace_back(Address::ptr(new IPv4Address()), 0u);
            }
            if (family == AF_INET6 || family == AF_UNSPEC) {
                vec.emplace_back(Address::ptr(new IPv6Address()), 0u);
            }
            return true;
        }
        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> retMap;
        if (!GetInterfaceAddress(retMap, family)) {
            return false;
        }
        for (auto& it : retMap) {
            if (it.first == name) {
                vec.push_back(it.second);
            }
        }
        return !vec.empty();
    }

    std::string Address::toString() const {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    int Address::getFamily() const {
        return getAddr()->sa_family;
    }

    // ----------------------------------------------------------------------------
    IPAddress::ptr IPAddress::Create(const char* addr, uint16_t port) {
        addrinfo* results;
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        // hints.ai_flags = AI_NUMERICHOST;        // 只解析数字地址（即不解析主机名，只解析 IP 地址）
        hints.ai_family = AF_UNSPEC;            // 不限制地址族，允许返回 IPv4 或 IPv6 地址
        int ret = getaddrinfo(addr, NULL, &hints, &results);
        if (ret) {
            SYLAR_LOG_DEBUG(g_logger) << "IPAddress::Create(" << addr << ", " << port << ") error="
                << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        try {
            IPAddress::ptr retAddr = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr));
            if (retAddr) {
                retAddr->setPort(port);
            }
            freeaddrinfo(results);
            return retAddr;
        } catch (...) {
            SYLAR_LOG_ERROR(g_logger) << "IPAddress::Create exception";
            freeaddrinfo(results);
            return nullptr;
        }
    }

    // ----------------------------------------------------------------------------
    IPv4Address::IPv4Address(const sockaddr_in& addr) : m_addr(addr) {}

    IPv4Address::IPv4Address(uint32_t addr, uint16_t port) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_addr.s_addr = htonl(addr);
        m_addr.sin_port = htons(port);
    }

    IPv4Address::ptr IPv4Address::Create(const char* addr, uint16_t port) {
        IPv4Address::ptr ipv4addr(new IPv4Address());
        ipv4addr->m_addr.sin_port = htons(port);
        int ret = inet_pton(AF_INET, addr, &ipv4addr->m_addr.sin_addr);
        if (ret <= 0) {
            SYLAR_LOG_DEBUG(g_logger) << "IPv4Addredd::Create(" << addr << ", "
                << port << ") ret=" << ret << " errno=" << errno << " errstr=" << strerror(errno);
            return nullptr;
        }
        return ipv4addr;
    }

    const sockaddr* IPv4Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv4Address::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream& IPv4Address::insert(std::ostream& os) const {
        uint32_t addr = ntohl(m_addr.sin_addr.s_addr);
        os << ((addr >> 24) & 0xff) << "."
            << ((addr >> 16) & 0xff) << "."
            << ((addr >> 8) & 0xff) << '.'
            << (addr & 0xff) << ": " << ntohs(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }
        sockaddr_in baddr(m_addr);
        baddr.sin_addr.s_addr |= htonl(GetMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }
        sockaddr_in naddr(m_addr);
        naddr.sin_addr.s_addr &= htonl(~GetMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(naddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
        if (prefix_len > 32) {
            return nullptr;
        }
        sockaddr_in saddr(m_addr);
        saddr.sin_addr.s_addr = htonl(~GetMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(saddr));
    }

    uint32_t IPv4Address::getPort() const {
        return ntohs(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t port) {
        m_addr.sin_port = htons(port);
    }

    // ----------------------------------------------------------------------------
    IPv6Address::IPv6Address() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6& addr) : m_addr(addr) {}

    IPv6Address::ptr IPv6Address::Create(const char* addr, uint16_t port) {
        IPv6Address::ptr ipv6addr(new IPv6Address());
        ipv6addr->m_addr.sin6_port = htons(port);
        int ret = inet_pton(AF_INET6, addr, &ipv6addr->m_addr.sin6_addr);
        if (ret <= 0) {
            SYLAR_LOG_DEBUG(g_logger) << "IPv6Address::Create(" << addr << ", "
                << port << ") ret=" << ret << " errno=" << errno << "errstr=" << strerror(errno);
            return nullptr;
        }
        return ipv6addr;
    }

    const sockaddr* IPv6Address::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv6Address::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream& IPv6Address::insert(std::ostream& os) const {
        os << "[";
        uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
        bool used_zeros = false;
        for (size_t i = 0; i < 8; ++i) {
            if (addr[i] == 0 && used_zeros == false) {
                continue;
            }
            if (i && addr[i - 1] == 0 && used_zeros == false) {
                os << ":";
                used_zeros = true;
            }
            if (i) {
                os << ":";
            }
            os << std::hex << (int)ntohs(addr[i]) << std::dec;
        }
        if (used_zeros == false && addr[7] == 0) {
            os << "::";
        }
        os << "]: " << ntohs(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
        if (prefix_len > 128) {
            return nullptr;
        }
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |= GetMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len) {
        if (prefix_len > 128) {
            return nullptr;
        }
        sockaddr_in6 naddr(m_addr);
        naddr.sin6_addr.s6_addr[prefix_len / 8] &= ~GetMask<uint8_t>(prefix_len % 8);
        for (int i = prefix_len / 8 + 1; i < 16; ++i) {
            naddr.sin6_addr.s6_addr[i] = 0x00;
        }
        return IPv6Address::ptr(new IPv6Address(naddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
        if (prefix_len > 128) {
            return nullptr;
        }
        sockaddr_in6 saddr(m_addr);
        saddr.sin6_addr.s6_addr[prefix_len / 8] = ~GetMask<uint8_t>(prefix_len % 8);
        for (size_t i = 0; i < prefix_len / 8; ++i) {
            saddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(saddr));
    }

    uint32_t IPv6Address::getPort() const {
        return m_addr.sin6_port;
    }

    void IPv6Address::setPort(uint16_t port) {
        m_addr.sin6_port = htons(port);
    }

    // ----------------------------------------------------------------------------
    UnixAddress::UnixAddress() {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + sizeof(m_addr.sun_path) - 1;
    }

    UnixAddress::UnixAddress(const std::string& path) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;
        if (!path.empty() && path[0] == '\0') {
            --m_length;
        }
        if (m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(m_addr.sun_path, path.c_str(), m_length);
        m_length += offsetof(sockaddr_un, sun_path);
    }

    void UnixAddress::setAddrLen(uint32_t len) {
        m_length = len;
    }

    std::string UnixAddress::getPath() const {
        std::stringstream ss;
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
            ss << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
        } else {
            ss << m_addr.sun_path;
        }
        return ss.str();
    }

    const sockaddr* UnixAddress::getAddr() const {
        return (sockaddr*)&m_addr;
    }

    sockaddr* UnixAddress::getAddr() {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const {
        return m_length;
    }

    std::ostream& UnixAddress::insert(std::ostream& os) const {
        // 抽象命名空间地址
        if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
            return os << "\\0" << std::string(m_addr.sun_path + 1,
                m_length - offsetof(sockaddr_un, sun_path) - 1);
        }
        return os << m_addr.sun_path;
    }

    // ----------------------------------------------------------------------------
    UnknownAddress::UnknownAddress(int family) {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    UnknownAddress::UnknownAddress(const sockaddr& addr) : m_addr(addr) {}

    const sockaddr* UnknownAddress::getAddr() const {
        return &m_addr;
    }

    sockaddr* UnknownAddress::getAddr() {
        return &m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const {
        return sizeof(m_addr);
    }

    std::ostream& UnknownAddress::insert(std::ostream& os) const {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

    // ----------------------------------------------------------------------------
    std::ostream& operator<<(std::ostream& os, const Address& addr) {
        return addr.insert(os);
    }


}