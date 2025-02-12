#include "socket_stream.h"

namespace sylar
{
    SocketStream::SocketStream(Socket::ptr socket, bool owner)
        : m_socket(socket)
        , m_owner(owner){}

    SocketStream::~SocketStream() {
        if (m_owner && m_socket) {
            m_socket->close();
        }
    }

    int SocketStream::read(void* buffer, size_t length) {
        if (!isConnect()) {
            return -1;
        }
        return m_socket->recv(buffer, length);
    }

    int SocketStream::write(const void* buffer, size_t length) {
        if (!isConnect()) {
            return -1;
        }
        return m_socket->send(buffer, length);
    }

    void SocketStream::close() {
        if (m_socket) {
            m_socket->close();
        }
    }

    bool SocketStream::isConnect() const {
        return m_socket && m_socket->isConnected();
    }
}