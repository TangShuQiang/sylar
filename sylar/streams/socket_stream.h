#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "sylar/stream.h"
#include "sylar/socket.h"

namespace sylar
{
    class SocketStream : public Stream
    {
    public:
        using ptr = std::shared_ptr<SocketStream>;

        SocketStream(Socket::ptr socket, bool owner = true);

        ~SocketStream();

        int read(void* buffer, size_t length) override;
        int write(const void* buffer, size_t length) override;
        void close() override;
        Socket::ptr getSocket() const { return m_socket; }
        bool isConnected() const;

    private:
        Socket::ptr m_socket;
        bool m_owner;               // 是否主控
    };
}

#endif