#include "tcp_server.h"
#include "log.h"
#include "config.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
        sylar::Config::Add("tcp_server.read_timeout", (uint64_t)(10 * 1000), "tcp server read timeout");

    TcpServer::TcpServer(IOManager* worker, IOManager* ioWorker, IOManager* acceptWorker)
        : m_worker(worker)
        , m_ioWorker(ioWorker)
        , m_acceptWorker(acceptWorker)
        , m_recvTimeout(g_tcp_server_read_timeout->getValue())
        , m_name("sylar/1.0.0")
        , m_isStop(true) {}

    TcpServer::~TcpServer() {}

    bool TcpServer::bind(Address::ptr addr, bool ssl) {
        std::vector<Address::ptr> addrs;
        std::vector<Address::ptr> fails;
        addrs.push_back(addr);
        return bind(addrs, fails, ssl);
    }

    bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails, bool ssl) {
        m_ssl = ssl;
        for (auto& addr : addrs) {
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock->bind(addr)) {
                SYLAR_LOG_ERROR(g_logger) << "bind fail errno=" << errno << " errstr="
                    << strerror(errno) << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            if (!sock->listen()) {
                SYLAR_LOG_ERROR(g_logger) << "listen fail errno=" << errno << " errstr="
                    << strerror(errno) << " addr=[" << addr->toString() << "]";
                fails.push_back(addr);
                continue;
            }
            m_socks.push_back(sock);
        }
        if (!fails.empty()) {
            m_socks.clear();
            return false;
        }
        for (auto& sock : m_socks) {
            SYLAR_LOG_INFO(g_logger) << "type=" << m_type
                << " name=" << m_name
                << " ssl=" << m_ssl
                << " server bind success: " << sock->toString();
        }
        return true;
    }

    void TcpServer::stop() {
        m_isStop = true;
        auto self = shared_from_this();
        m_acceptWorker->schedule((std::function<void()>)[this, self]() {
            for (auto& sock : m_socks) {
                sock->cancelAll();
            }
        });
    }

    bool TcpServer::start() {
        if (!m_isStop) {
            return true;
        }
        m_isStop = false;
        for (auto& sock : m_socks) {
            m_acceptWorker->schedule((std::function<void()>)std::bind(&TcpServer::startAccept,
                shared_from_this(), sock));
        }
        return true;
    }

    void TcpServer::startAccept(Socket::ptr sock) {
        while (!m_isStop) {
            Socket::ptr client = sock->accept();
            if (client) {
                client->setRecvTimeout(m_recvTimeout);
                m_ioWorker->schedule((std::function<void()>)std::bind(&TcpServer::handleClient,
                    shared_from_this(), client));
            } else {
                SYLAR_LOG_ERROR(g_logger) << "accept errno=" << errno
                    << "errstr=" << strerror(errno);
            }
        }
    }

    void TcpServer::handleClient(Socket::ptr client) {
        SYLAR_LOG_INFO(g_logger) << "handleClient: " << client->toString();
    }

    std::string TcpServer::toString(const std::string& prefix) {
        std::stringstream ss;
        ss << prefix << "[type=" << m_type
            << " name=" << m_name
            << " ssl=" << m_ssl
            << " worker=" << (m_worker ? m_worker->getName() : "")
            << " ioWorker=" << (m_ioWorker ? m_ioWorker->getName() : "")
            << " acceptWorker=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
            << " recvTimeout=" << m_recvTimeout << "]" << std::endl;
        std::string pfx = prefix.empty() ? "    " : prefix;
        for (auto& sock : m_socks) {
            ss << pfx << pfx << sock->toString() << std::endl;
        }
        return ss.str();
    }


}