#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "sylar/tcp_server.h"
#include "servlet.h"

namespace sylar
{
    namespace http
    {
        class HttpServer : public TcpServer
        {
        public:
            using ptr = std::shared_ptr<HttpServer>;

            HttpServer(bool keepalive = false
                , sylar::IOManager* worker = sylar::IOManager::GetThisIOManager()
                , sylar::IOManager* ioWorker = sylar::IOManager::GetThisIOManager()
                , sylar::IOManager* acceptWorker = sylar::IOManager::GetThisIOManager());

            void setName(const std::string& name) override;

            ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
            void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

        protected:
            void handleClient(Socket::ptr client) override;

        private:
            bool m_isKeepalive;             //  是否支持长连接
            ServletDispatch::ptr m_dispatch;    // Servlet 分发器
        };
    }
}

#endif