#include <netinet/tcp.h>

#include "socket.h"
#include "log.h"
#include "hook.h"
#include "fd_manager.h"
#include "iomanager.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("symtem");

    Socket::Socket(int family, int type, int protocol)
        : m_sockfd(-1)
        , m_family(family)
        , m_type(type)
        , m_protocol(protocol)
        , m_isConnected(false) {
        createSock();
        if (m_type == SOCK_DGRAM) {
            m_isConnected = true;
        }
    }

    Socket::ptr Socket::CreateTCP(Address::ptr addr) {
        return std::make_shared<Socket>(addr->getFamily(), SOCK_STREAM, 0);
    }

    Socket::ptr Socket::CreateUDP(Address::ptr addr) {
        return std::make_shared<Socket>(addr->getFamily(), SOCK_DGRAM, 0);
    }

    Socket::ptr Socket::CreateTCPSocket() {
        return std::make_shared<Socket>(AF_INET, SOCK_STREAM, 0);
    }

    Socket::ptr Socket::CreateUDPSocket() {
        return std::make_shared<Socket>(AF_INET, SOCK_DGRAM, 0);
    }

    Socket::ptr Socket::CreateTCPSocket6() {
        return std::make_shared<Socket>(AF_INET6, SOCK_STREAM, 0);
    }

    Socket::ptr Socket::CreateUDPSocket6() {
        return std::make_shared<Socket>(AF_INET6, SOCK_DGRAM, 0);
    }

    Socket::ptr Socket::CreateUnixTCPSocket() {
        return std::make_shared<Socket>(AF_UNIX, SOCK_STREAM, 0);
    }

    Socket::ptr Socket::CreateUnixUDPSocket() {
        return std::make_shared<Socket>(AF_UNIX, SOCK_DGRAM, 0);
    }

    Socket::~Socket() {
        close();
    }

    bool Socket::bind(const Address::ptr addr) {
        if (addr->getFamily() != m_family) {
            SYLAR_LOG_ERROR(g_logger) << "bind sock.family(" << m_family << ") addr.family("
                << addr->getFamily() << ") not equal, addr=" << addr->toString();
            return false;
        }
        if (!isValidSock()) {
            createSock();
            if (!isValidSock()) {
                return false;
            }
        }
        // Unix处理
        // ....

        if (::bind(m_sockfd, addr->getAddr(), addr->getAddrLen())) {
            SYLAR_LOG_ERROR(g_logger) << "bind error errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        m_localAddress = addr;
        return true;
    }

    bool Socket::listen(int backlog) {
        if (!isValidSock()) {
            SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
            return false;
        }
        if (::listen(m_sockfd, backlog)) {
            SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    Socket::ptr Socket::accept() {
        Socket::ptr newSock = std::make_shared<Socket>(m_family, m_type, m_protocol);
        int newfd = ::accept(m_sockfd, nullptr, nullptr);
        if (newfd == -1) {
            SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sockfd << ") errno=" << errno
                << " errstr=" << strerror(errno);
            return nullptr;
        }
        if (newSock->init(newfd)) {
            return newSock;
        }
        return nullptr;
    }

    bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
        if (addr->getFamily() != m_family) {
            SYLAR_LOG_ERROR(g_logger) << "connect sock.family(" << m_family << ") addr.family("
                << addr->getFamily() << ") not equal, addr=" << addr->toString();
            return false;
        }
        if (!isValidSock()) {
            createSock();
            if (!isValidSock()) {
                return false;
            }
        }
        if (timeout_ms == (uint64_t)-1) {
            if (::connect(m_sockfd, addr->getAddr(), addr->getAddrLen())) {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sockfd << " connect(" << addr->toString()
                    << ") error errno=" << errno << "errstr=" << strerror(errno);
                close();
                return false;
            }
        } else {
            if (::connect_with_timeout(m_sockfd, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
                SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sockfd << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " error errno=" << errno << "errstr=" << strerror(errno);
                close();
                return false;
            }
        }
        m_isConnected = true;
        m_remoteAddress = addr;
        getLocalAddress();
        return true;
    }

    bool Socket::reconnect(uint64_t timeout_ms) {
        if (!m_remoteAddress) {
            SYLAR_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is nullptr";
            return false;
        }
        m_localAddress = nullptr;
        return connect(m_remoteAddress, timeout_ms);
    }

    bool Socket::close() {
        if (m_sockfd != -1) {
            ::close(m_sockfd);
            m_sockfd = -1;
        }
        m_isConnected = false;
        return true;
    }

    int Socket::send(const void* buf, size_t length, int flags) {
        if (isConnected()) {
            return ::send(m_sockfd, buf, length, flags);
        }
        return -1;
    }

    int Socket::sendTo(const void* buf, size_t length, const Address::ptr addr, int flags) {
        return ::sendto(m_sockfd, buf, length, flags, addr->getAddr(), addr->getAddrLen());
    }

    int Socket::recv(void* buf, size_t length, int flags) {
        if (isConnected()) {
            return ::recv(m_sockfd, buf, length, flags);
        }
        return -1;
    }

    int Socket::recvFrom(void* buf, size_t length, Address::ptr addr, int flags) {
        socklen_t len = addr->getAddrLen();
        return ::recvfrom(m_sockfd, buf, length, flags, addr->getAddr(), &len);
    }

    std::ostream& Socket::dump(std::ostream& os) const {
        os << "[Socket sockfd=" << m_sockfd
            << " family=" << m_family
            << " type=" << m_type
            << " protocol=" << m_protocol
            << " isConnected=" << m_isConnected;
        if (m_localAddress) {
            os << " localAddress=" << m_localAddress->toString();
        }
        if (m_remoteAddress) {
            os << " remoteAddress=" << m_remoteAddress->toString();
        }
        os << "]";
        return os;
    }

    std::string Socket::toString() const {
        std::stringstream ss;
        dump(ss);
        return ss.str();
    }

    bool Socket::getOption(int level, int optname, void* optval, socklen_t* optlen) {
        if (::getsockopt(m_sockfd, level, optname, optval, optlen)) {
            SYLAR_LOG_ERROR(g_logger) << "getOption sockfd=" << m_sockfd << " level=" << level
                << " optname=" << optname << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    bool Socket::setOption(int level, int optname, const void* optval, socklen_t optlen) {
        if (::setsockopt(m_sockfd, level, optname, optval, optlen)) {
            SYLAR_LOG_ERROR(g_logger) << "setOption sockfd=" << m_sockfd << " level=" << level
                << " optname=" << optname << " errno=" << errno << " errstr=" << strerror(errno);
            return false;
        }
        return true;
    }

    void Socket::setSendTimeout(int64_t t) {
        struct timeval tv { int(t / 1000), (t % 1000 * 1000) };
        setOption(SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    }

    int64_t Socket::getSendTimeout() {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
        if (ctx) {
            return ctx->getTimeout(SO_SNDTIMEO);
        }
        return -1;
    }

    void Socket::setRecvTimeout(int64_t t) {
        struct timeval tv { int(t / 1000), (t % 1000 * 1000) };
        setOption(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    int64_t Socket::getRecvTimeout() {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sockfd);
        if (ctx) {
            return ctx->getTimeout(SO_RCVTIMEO);
        }
        return -1;
    }

    bool Socket::isValidSock() const {
        return m_sockfd != -1;
    }

    int Socket::getError() {
        int error = 0;
        socklen_t len = sizeof(error);
        if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
            error = errno;
        }
        return error;
    }

    void Socket::createSock() {
        m_sockfd = ::socket(m_family, m_type, m_protocol);
        if (m_sockfd == -1) {
            SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family << ", " << m_type << ", " << m_protocol
                << ") errno=" << errno << " errstr=" << strerror(errno);
            return;
        }
        initSock();
    }

    void Socket::initSock() {
        int val = 1;
        setOption(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        if (m_type == SOCK_STREAM) {
            // 禁用 Nagle 算法，这样就不会将小的数据包合并成一个大的数据包，而是立即发送每一个小的数据包
            setOption(IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
        }
    }

    bool Socket::init(int fd) {
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);
        if (ctx && ctx->isSocket() && !ctx->isClosed()) {
            m_sockfd = fd;
            m_isConnected = true;
            initSock();
            getLocalAddress();
            getRemoteAddress();
            return true;
        }
        return false;
    }

    Address::ptr Socket::getLocalAddress() {
        if (m_remoteAddress) {
            return m_remoteAddress;
        }
        Address::ptr result;
        switch (m_family) {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
            break;
        default:
            result = std::make_shared<UnknownAddress>(m_family);
        }
        socklen_t addrlen = result->getAddrLen();
        if (getsockname(m_sockfd, result->getAddr(), &addrlen)) {
            SYLAR_LOG_ERROR(g_logger) << "getsockname error sockfd=" << m_sockfd
                << " errno=" << errno << " errstr=" << strerror(errno);
            return std::make_shared<UnknownAddress>(m_family);
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    Address::ptr Socket::getRemoteAddress() {
        if (m_remoteAddress) {
            return m_remoteAddress;
        }
        Address::ptr result;
        switch (m_family) {
        case AF_INET:
            result = std::make_shared<IPv4Address>();
            break;
        case AF_INET6:
            result = std::make_shared<IPv6Address>();
            break;
        case AF_UNIX:
            result = std::make_shared<UnixAddress>();
            break;
        default:
            result = std::make_shared<UnknownAddress>(m_family);
        }
        socklen_t addrlen = result->getAddrLen();
        if (getpeername(m_sockfd, result->getAddr(), &addrlen)) {
            SYLAR_LOG_ERROR(g_logger) << "getperrname error sockfd=" << m_sockfd
                << " errno=" << errno << " errstr=" << strerror(errno);
            return std::make_shared<UnknownAddress>(m_family);
        }
        if (m_family == AF_UNIX) {
            UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
            addr->setAddrLen(addrlen);
        }
        m_remoteAddress = result;
        return m_remoteAddress;
    }

    bool Socket::cancelRead() {
        return IOManager::GetThisIOManager()->cancelEvent(m_sockfd, IOManager::Event::READ);
    }

    bool Socket::cancelWrite() {
        return IOManager::GetThisIOManager()->cancelEvent(m_sockfd, IOManager::Event::WRITE);
    }

    bool Socket::canncelAccept() {
        return IOManager::GetThisIOManager()->cancelEvent(m_sockfd, IOManager::Event::READ);
    }

    bool Socket::cancelAll() {
        return IOManager::GetThisIOManager()->cancelAll(m_sockfd);
    }

}