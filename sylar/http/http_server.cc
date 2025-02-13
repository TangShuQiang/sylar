#include "http_server.h"
#include "log.h"
#include "http_session.h"

namespace sylar
{
    namespace http
    {
        static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

        HttpServer::HttpServer(bool keepalive, sylar::IOManager* worker, sylar::IOManager* ioWorker, sylar::IOManager* acceptWorker)
            : TcpServer(worker, ioWorker, acceptWorker)
            , m_isKeepalive(keepalive)
            , m_dispatch(std::make_shared<ServletDispatch>()) {
            m_type = "http";

        }

        void HttpServer::setName(const std::string& name) {
            TcpServer::setName(name);
            m_dispatch->setDefault(std::make_shared<NotFoundServlet>(name));
        }

        void HttpServer::handleClient(Socket::ptr client) {
            SYLAR_LOG_DEBUG(g_logger) << "handleClient: " << client->toString();
            HttpSession::ptr session = std::make_shared<HttpSession>(client);
            do {
                auto req = session->recvRequest();
                if (!req) {
                    SYLAR_LOG_DEBUG(g_logger) << "recv http request fail, errno=" << errno << " errstr=" << strerror(errno)
                        << " client: " << client->toString() << " keep_alive=" << m_isKeepalive;
                    break;
                }
                SYLAR_LOG_DEBUG(g_logger) << "req:" << req->toString();
                HttpResponse::ptr rsp = std::make_shared<HttpResponse>(req->getVersion(), req->isClose() || !m_isKeepalive);
                rsp->setHeader("Server", getName());
                // rsp->setBody("hello world!");
                m_dispatch->handler(req, rsp, session);
                session->sendResponse(rsp);
                if (!m_isKeepalive || req->isClose()) {
                    break;
                }
            } while (true);
            session->close();
        }

    }
}