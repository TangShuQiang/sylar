#ifndef __SYLAR_SOCKET_H__
#define __SYALR_SOCKET_H__

#include <memory>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "address.h"
#include "noncopyable.h"

namespace sylar
{
    class Socket : Noncopyable
    {
    public:
        using ptr = std::shared_ptr<Socket>;

        Socket(int family, int type, int protocol);
        
        static Socket::ptr CreateTCP(Address::ptr addr);
        static Socket::ptr CreateUDP(Address::ptr addr);

        static Socket::ptr CreateTCPSocket();
        static Socket::ptr CreateUDPSocket();

        static Socket::ptr CreateTCPSocket6();
        static Socket::ptr CreateUDPSocket6();

        static Socket::ptr CreateUnixTCPSocket();
        static Socket::ptr CreateUnixUDPSocket();


        virtual ~Socket();
        virtual bool bind(const Address::ptr addr);
        virtual bool listen(int backlog = SOMAXCONN);
        virtual Socket::ptr accept();
        virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
        virtual bool reconnect(uint64_t timeout_ms = -1);
        virtual bool close();

        virtual int send(const void* buf, size_t length, int flags = 0);
        virtual int sendTo(const void* buf, size_t length, const Address::ptr addr, int flags = 0);
        virtual int recv(void* buf, size_t length, int flags = 0);
        virtual int recvFrom(void* buf, size_t length, Address::ptr addr, int flags = 0);

        virtual std::ostream& dump(std::ostream& os) const;
        virtual std::string toString() const;

        bool getOption(int level, int optname, void* optval, socklen_t* optlen);
        bool setOption(int level, int optname, const void* optval, socklen_t optlen);

        void setSendTimeout(int64_t t);
        int64_t getSendTimeout();
        void setRecvTimeout(int64_t t);
        int64_t getRecvTimeout();

        int getSocketfd() const { return m_sockfd; }
        int getFamily() const { return m_family; }
        int getType() const { return m_type; }
        int getProtocol() const { return m_protocol; }
        bool isConnected() const { return m_isConnected; }

        Address::ptr getLocalAddress();
        Address::ptr getRemoteAddress();

        bool isValidSock() const;
        int getError();

        bool cancelRead();
        bool cancelWrite();
        bool canncelAccept();
        bool cancelAll();

    private:
        void createSock();
        void initSock();
        bool init(int fd);

    private:
        int m_sockfd;                   // 文件描述符
        int m_family;                     // 协议簇(AF_INET, AF_INET6, AF_UNIX)
        int m_type;                     // 类型(SOCK_STREAM，SOCK_DGRAM)
        int m_protocol;                 // 协议(IPPROTO_TCP、TPTROTO_UDP)
        bool m_isConnected;
        Address::ptr m_localAddress;    // 本地地址
        Address::ptr m_remoteAddress;   // 远端地址
    };
}

#endif