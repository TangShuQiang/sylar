#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <memory>
#include <sys/types.h>       
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <map>

namespace sylar
{
    class IPAddress;

    class Address
    {
    public:
        using ptr = std::shared_ptr<Address>;

        // 通过sockaddr指针创建Address
        static Address::ptr Create(const sockaddr* addr);

        /*
        通过host地址返回对应条件的所有Address
        host: 域名、服务器名等，www.baidu.com/www.baidu.com:80/www.baidu.com:http
        family: 协议族(AF_INET, AF_INET6, AF_UNIX)
        type: socket类型(SOCK_STREAM，SOCK_DGRAM)
        protocol: 协议(IPPROTO_TCP、TPTROTO_UDP)
        */
        static bool Lookup(std::vector<Address::ptr>& vec, const std::string& host,
            int family = AF_INET, int type = SOCK_STREAM, int protocol = 0);

        // 通过host地址返回对应条件的任意Address
        static Address::ptr LookupAny(const std::string& host, int family = AF_INET,
            int type = SOCK_STREAM, int protocol = 0);

        // 通过host地址返回对应条件的任意IPAddress
        static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = SOCK_STREAM, int protocol = 0);

        // 获取本机所有网卡的<网卡名，地址，子网掩码位数>
        static bool GetInterfaceAddress(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& retMap, int family = AF_INET);

        // 获取指定网卡的地址和子网掩码位数
        static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>>& vec, const std::string& name, int family = AF_INET);

        virtual const sockaddr* getAddr() const = 0;        // 返回sockaddr指针，只读
        virtual sockaddr* getAddr() = 0;                    // 返回sockaddr指针，只写
        virtual socklen_t getAddrLen() const = 0;           // 返回sockaddr的长度
        virtual std::ostream& insert(std::ostream& os) const = 0;   // 输出可读性地址

        virtual ~Address() {}

        std::string toString() const;   // 返回可读性字符串
        int getFamily() const;
    };

    // ----------------------------------------------------------------------------
    class IPAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<IPAddress>;

        // 通过 域名、IP、服务器名 创建IPAddress
        static IPAddress::ptr Create(const char* addr, uint16_t port = 0);

        // 获取该地址的广播地址， prefix_len: 子网掩码位数
        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
        // 获取该地址的网段
        virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
        // 获取子网掩码地址
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;
        // 返回端口号
        virtual uint32_t getPort() const = 0;
        // 设置端口号
        virtual void setPort(uint16_t port) = 0;
    };

    // ----------------------------------------------------------------------------
    class IPv4Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv4Address>;

        IPv4Address(const sockaddr_in& addr);
        // 通过二进制地址构造IPv4Address
        IPv4Address(uint32_t addr = INADDR_ANY, uint16_t port = 0);
        // 使用点分十进制地址创建IPv4Address
        static IPv4Address::ptr Create(const char* addr, uint16_t port = 0);

        // 返回sockaddr指针，只读
        const sockaddr* getAddr() const override;
        // 返回sockaddr指针，只写
        sockaddr* getAddr() override;
        // 返回sockaddr的长度
        socklen_t getAddrLen() const override;
        // 输出可读性地址
        std::ostream& insert(std::ostream& os) const override;

        // 获取该地址的广播地址， prefix_len: 子网掩码位数
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        // 获取该地址的网段
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        // 获取子网掩码地址
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;
        // 返回端口号
        uint32_t getPort() const override;
        // 设置端口号
        void setPort(uint16_t port) override;

    private:
        sockaddr_in m_addr;
    };

    // ----------------------------------------------------------------------------
    class IPv6Address : public IPAddress
    {
    public:
        using ptr = std::shared_ptr<IPv6Address>;

        IPv6Address();
        IPv6Address(const sockaddr_in6& addr);

        // 使用IPv6地址字符串构造IPv6Address
        static IPv6Address::ptr Create(const char* addr, uint16_t port = 0);

        // 返回sockaddr指针，只读
        const sockaddr* getAddr() const override;
        // 返回sockaddr指针，只写
        sockaddr* getAddr() override;
        // 返回sockaddr的长度
        socklen_t getAddrLen() const override;
        // 输出可读性地址
        std::ostream& insert(std::ostream& os) const override;

        // 获取该地址的广播地址， prefix_len: 子网掩码位数
        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        // 获取该地址的网段
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        // 获取子网掩码地址
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;
        // 返回端口号
        uint32_t getPort() const override;
        // 设置端口号
        void setPort(uint16_t port) override;

    private:
        sockaddr_in6 m_addr;
    };

    // ----------------------------------------------------------------------------
    class UnixAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnixAddress>;

        UnixAddress();
        UnixAddress(const std::string& path);

        void setAddrLen(uint32_t len);
        std::string getPath() const;

        // 返回sockaddr指针，只读
        const sockaddr* getAddr() const override;
        // 返回sockaddr指针，只写
        sockaddr* getAddr() override;
        // 返回sockaddr的长度
        socklen_t getAddrLen() const override;
        // 输出可读性地址
        std::ostream& insert(std::ostream& os) const override;

    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };

    // ----------------------------------------------------------------------------
    class UnknownAddress : public Address
    {
    public:
        using ptr = std::shared_ptr<UnknownAddress>;

        UnknownAddress(int family);
        UnknownAddress(const sockaddr& addr);

        // 返回sockaddr指针，只读
        const sockaddr* getAddr() const override;
        // 返回sockaddr指针，只写
        sockaddr* getAddr() override;
        // 返回sockaddr的长度
        socklen_t getAddrLen() const override;
        // 输出可读性地址
        std::ostream& insert(std::ostream& os) const override;

    private:
        sockaddr m_addr;
    };

    std::ostream& operator<<(std::ostream& os, const Address& addr);
}

#endif