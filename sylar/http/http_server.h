#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "sylar/tcp_server.h"

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

        protected:
            void handleClient(Socket::ptr client) override;

        private:
            bool m_isKeepalive;             //  是否支持长连接
        };
    }
}

#endif