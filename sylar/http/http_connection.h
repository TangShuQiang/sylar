#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include <memory>
#include <atomic>

#include "http.h"
#include "uri.h"
#include "sylar/streams/socket_stream.h"

namespace sylar
{
    namespace http
    {
        struct HttpResult
        {
            using ptr = std::shared_ptr<HttpResult>;

            enum class Status
            {
                OK = 0,
                INVALID_URL = 1,                    // 非法URL
                INVALID_HOST = 2,                   // 无法解析HOST
                CONNECT_FAIL = 3,                   // 连接失败
                SEND_CLOSE_BY_PEER = 4,             // 连接被对端关闭
                SEND_SOCKET_ERROR = 5,              // 发送请求产生Socket错误
                TIMEOUT = 6,                        // 超时
                CREATE_SOCKET_ERROR = 7,            // 创建Socket失败
                POOL_GET_CONNECTION_FAIL = 8,       // 从连接池中取连接失败
                POOL_INVAALID_CONNECTION = 9,       // 无效的连接
            };

            HttpResult(Status st, HttpResponse::ptr rsp, const std::string& desc)
                : status(st)
                , response(rsp)
                , description(desc) {}

            std::string toString() const;

            Status status;
            HttpResponse::ptr response; // HTTP响应结构体
            std::string description;           // 状态描述
        };

        // ------------------------------------------------------------

        class HttpConnection : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpConnection>;

            HttpConnection(Socket::ptr sock, bool owner = true);
            ~HttpConnection();

            HttpResponse::ptr recvResponse();
            int sendRequest(HttpRequest::ptr req);

            static HttpResult::ptr DoGet(const std::string& url,
                                         uint64_t timeout_ms,
                                         const std::map<std::string, std::string>& headers = {},
                                         const std::string& body = "");

            static HttpResult::ptr DoGet(Uri::ptr uri,
                                         uint64_t timeout_ms,
                                         const std::map<std::string, std::string>& headers = {},
                                         const std::string& body = "");

            static HttpResult::ptr DoPost(const std::string& url,
                                          uint64_t timeout_ms,
                                          const std::map<std::string, std::string>& headers = {},
                                          const std::string& body = "");

            static HttpResult::ptr DoPost(Uri::ptr uri,
                                          uint64_t timeout_ms,
                                          const std::map<std::string, std::string>& headers = {},
                                          const std::string& body = "");

            static HttpResult::ptr DoRequest(HttpMethod method,
                                             const std::string& url,
                                             uint64_t timeout_ms,
                                             const std::map<std::string, std::string>& headers = {},
                                             const std::string& body = "");

            static HttpResult::ptr DoRequest(HttpMethod method,
                                             Uri::ptr uri,
                                             uint64_t timeout_ms,
                                             const std::map<std::string, std::string>& headers = {},
                                             const std::string& body = "");

            static HttpResult::ptr DoRequest(HttpRequest::ptr req,
                                             Uri::ptr uri,
                                             uint64_t timeout_ms);

            uint64_t getCreateTime() const { return m_createTime; }
            uint64_t getRequest() const { return m_request; }
            void addRequest() { ++m_request; }
        private:
            uint64_t m_createTime = 0;
            uint64_t m_request = 0;
        };

        // ------------------------------------------------------------

        class HttpConnectionPool
        {
        public:
            using ptr = std::shared_ptr<HttpConnectionPool>;
            using MutexType = Mutex;

            HttpConnectionPool(const std::string& host
                               , const std::string& vhost
                               , uint32_t port
                               , bool is_https
                               , uint32_t max_size
                               , uint32_t max_alive_time
                               , uint32_t max_request);

            static HttpConnectionPool::ptr Create(const std::string& uri
                                                  , const std::string& vhost
                                                  , uint32_t port
                                                  , uint32_t max_size
                                                  , uint32_t max_alive_time
                                                  , uint32_t max_request);

            HttpConnection::ptr getConnection();

            HttpResult::ptr doGet(const std::string& url,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string>& headers = {},
                                  const std::string& body = "");

            HttpResult::ptr doGet(Uri::ptr uri,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string>& headers = {},
                                  const std::string& body = "");

            HttpResult::ptr doPost(const std::string& url,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

            HttpResult::ptr doPost(Uri::ptr uri,
                                   uint64_t timeout_ms,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

            HttpResult::ptr doRequest(HttpMethod method,
                                      const std::string& url,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string>& headers = {},
                                      const std::string& body = "");

            HttpResult::ptr doRequest(HttpMethod method,
                                      Uri::ptr uri,
                                      uint64_t timeout_ms,
                                      const std::map<std::string, std::string>& headers = {},
                                      const std::string& body = "");

            HttpResult::ptr doRequest(HttpRequest::ptr req,
                                      uint64_t timeout_ms);
        private:
            static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);
        private:
            std::string m_host;
            std::string m_vhost;
            uint32_t m_port;
            uint32_t m_maxSize;
            uint32_t m_maxAliveTime;
            uint32_t m_maxRequest;
            bool m_isHttps;

            MutexType m_mutex;
            std::list<HttpConnection*> m_conns;
            std::atomic<int32_t> m_total = { 0 };
        };
    }
}

#endif