#include "log.h"
#include "http_connection.h"
#include "http_parser.h"
#include "util.h"

namespace sylar
{
    namespace http
    {
        static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

        std::string HttpResult::toString() const {
            std::stringstream ss;
            ss << "[HttpResult status=" << (int)status
                << " description=" << description
                << " response=" << (response ? response->toString() : "nullptr")
                << "]";
            return ss.str();
        }

        // ------------------------------------------------------------

        HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
            : SocketStream(sock, owner)
            , m_createTime(sylar::GetCurrentMS()) {}

        HttpConnection::~HttpConnection() {
            SYLAR_LOG_DEBUG(g_logger) << "HttpConnection::~HttpConnection";
        }

        HttpResponse::ptr HttpConnection::recvResponse() {
            HttpResponseParser::ptr parser = std::make_shared<HttpResponseParser>();
            uint64_t buffSize = HttpResponseParser::GetHttpResponseBufferSize();
            std::shared_ptr<char[]> buffer(new char[buffSize]());
            char* data = buffer.get();
            int offset = 0;
            do {
                int len = read(data + offset, buffSize - offset);
                if (len <= 0) {
                    close();
                    return nullptr;
                }
                len += offset;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, false);
                if (parser->hasError()) {
                    close();
                    return nullptr;
                }
                offset += len - nparse;
                if (offset == (int)buffSize) {
                    close();
                    return nullptr;
                }
                if (parser->isFinished()) {
                    break;
                }
            } while (true);
            auto& client_parser = parser->getParser();
            std::string body;
            if (client_parser.chunked) {
                int len = offset;
                do {
                    bool begin = true;
                    do {
                        if (!begin || len == 0) {
                            int ret = read(data + len, buffSize - len);
                            if (ret <= 0) {
                                close();
                                return nullptr;
                            }
                            len += ret;
                        }
                        data[len] = '\0';
                        size_t nparser = parser->execute(data, len, true);
                        if (parser->hasError()) {
                            close();
                            return nullptr;
                        }
                        len -= nparser;
                        if (len == (int)buffSize) {
                            close();
                            return nullptr;
                        }
                        begin = false;
                        if (parser->isFinished()) {
                            break;;
                        }
                    } while (true);

                    if (client_parser.content_len + 2 <= len) {
                        body.append(data, client_parser.content_len);
                        memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len - 2);
                        len -= client_parser.content_len + 2;
                    } else {
                        body.append(data, len);
                        int left = client_parser.content_len - len + 2;
                        while (left > 0) {
                            int ret = read(data, left > (int)buffSize ? (int)buffSize : left);
                            if (ret <= 0) {
                                close();
                                return nullptr;
                            }
                            body.append(data, ret);
                            left -= ret;
                        }
                        body.resize(body.size() - 2);
                        len = 0;
                    }

                    if (client_parser.chunks_done) {
                        break;
                    }
                } while (true);
            } else {
                int64_t contentLength = parser->getContentLength();
                if (contentLength > 0) {
                    body.resize(contentLength);
                    if (offset >= contentLength) {
                        memcpy(&body[0], data, contentLength);
                    } else {
                        memcpy(&body[0], data, offset);
                        if (readFixSize(&body[offset], contentLength - offset) <= 0) {
                            close();
                            return nullptr;
                        }
                    }
                }
            }
            if (!body.empty()) {
                parser->getData()->setBody(body);
            }
            return parser->getData();
        }

        int HttpConnection::sendRequest(HttpRequest::ptr req) {
            std::string data = req->toString();
            return writeFixSize(data.c_str(), data.size());
        }

        HttpResult::ptr HttpConnection::DoGet(const std::string& url,
                                              uint64_t timeout_ms,
                                              const std::map<std::string, std::string>& headers,
                                              const std::string& body) {
            Uri::ptr uri = Uri::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Status::INVALID_URL, nullptr,
                                                    "invalid url: " + url);
            }
            return DoGet(uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri,
                                              uint64_t timeout_ms,
                                              const std::map<std::string, std::string>& headers,
                                              const std::string& body) {
            return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoPost(const std::string& url,
                                               uint64_t timeout_ms,
                                               const std::map<std::string, std::string>& headers,
                                               const std::string& body) {
            Uri::ptr uri = Uri::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Status::INVALID_URL, nullptr,
                                                    "invalid url: " + url);
            }
            return DoPost(uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri,
                                               uint64_t timeout_ms,
                                               const std::map<std::string, std::string>& headers,
                                               const std::string& body) {
            return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method,
                                                  const std::string& url,
                                                  uint64_t timeout_ms,
                                                  const std::map<std::string, std::string>& headers,
                                                  const std::string& body) {
            Uri::ptr uri = Uri::Create(url);
            if (!uri) {
                return std::make_shared<HttpResult>(HttpResult::Status::INVALID_URL, nullptr,
                                                    "invalid url: " + url);
            }
            return DoRequest(method, uri, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpMethod method,
                                                  Uri::ptr uri,
                                                  uint64_t timeout_ms,
                                                  const std::map<std::string, std::string>& headers,
                                                  const std::string& body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(uri->getPath());
            req->setQuery(uri->getQuery());
            req->setFragment(uri->getFragment());
            req->setMethod(method);
            bool has_host = false;
            for (auto& it : headers) {
                if (strcasecmp(it.first.c_str(), "connection") == 0) {
                    if (strcasecmp(it.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }
                if (!has_host && strcasecmp(it.first.c_str(), "host") == 0) {
                    has_host = !it.second.empty();
                }
                req->setHeader(it.first, it.second);
            }
            if (!has_host) {
                req->setHeader("Host", uri->getHost());
            }
            req->setBody(body);
            return DoRequest(req, uri, timeout_ms);
        }

        HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req,
                                                  Uri::ptr uri,
                                                  uint64_t timeout_ms) {
            Address::ptr addr = uri->createAddress();
            if (!addr) {
                return std::make_shared<HttpResult>(HttpResult::Status::INVALID_HOST, nullptr,
                                                    "invalid host: " + uri->getHost());
            }
            Socket::ptr sock = Socket::CreateTCP(addr);
            if (!sock) {
                return std::make_shared<HttpResult>(HttpResult::Status::CREATE_SOCKET_ERROR, nullptr,
                                                    "create socket fail: " + addr->toString()
                                                    + " errno=" + std::to_string(errno)
                                                    + " errstr=" + strerror(errno));
            }
            if (!sock->connect(addr)) {
                return std::make_shared<HttpResult>(HttpResult::Status::CONNECT_FAIL, nullptr,
                                                    "connect fail: " + addr->toString());
            }
            sock->setRecvTimeout(timeout_ms);
            HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
            int ret = conn->sendRequest(req);
            if (ret == 0) {
                return std::make_shared<HttpResult>(HttpResult::Status::SEND_CLOSE_BY_PEER, nullptr,
                                                    "send request closed by peer: " + addr->toString());
            }
            if (ret < 0) {
                return std::make_shared<HttpResult>(HttpResult::Status::SEND_SOCKET_ERROR, nullptr,
                                                    " send request socket error, errno=" + std::to_string(errno)
                                                    + " errstr=" + strerror(errno));
            }
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_shared<HttpResult>(HttpResult::Status::TIMEOUT, nullptr,
                                                    "recv response timeout: " + addr->toString()
                                                    + " timeout_ms=" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>(HttpResult::Status::OK, rsp, "ok");
        }

        // ------------------------------------------------------------

        HttpConnectionPool::HttpConnectionPool(const std::string& host
                                               , const std::string& vhost
                                               , uint32_t port
                                               , bool is_https
                                               , uint32_t max_size
                                               , uint32_t max_alive_time
                                               , uint32_t max_request)
            : m_host(host)
            , m_vhost(vhost)
            , m_port(port ? port : (is_https ? 443 : 80))
            , m_maxSize(max_size)
            , m_maxAliveTime(max_alive_time)
            , m_maxRequest(max_request)
            , m_isHttps(is_https) {}

        HttpConnectionPool::ptr HttpConnectionPool::Create(const std::string& uri
                                                           , const std::string& vhost
                                                           , uint32_t port
                                                           , uint32_t max_size
                                                           , uint32_t max_alive_time
                                                           , uint32_t max_request) {
            Uri::ptr turi = Uri::Create(uri);
            if (!turi) {
                SYLAR_LOG_ERROR(g_logger) << "invalid uri=" << uri;
            }
            return std::make_shared<HttpConnectionPool>(turi->getHost(), vhost, turi->getPort(),
                                                        turi->getScheme() == "https",
                                                        max_size, max_alive_time, max_request);
        }

        HttpConnection::ptr HttpConnectionPool::getConnection() {
            std::vector<HttpConnection*> invalid_conns;
            HttpConnection* ptr = nullptr;
            {
                MutexType::Lock lock(m_mutex);
                while (!m_conns.empty()) {
                    auto conn = m_conns.front();
                    m_conns.pop_front();
                    if (!conn->isConnected()) {
                        invalid_conns.push_back(conn);
                        continue;
                    }
                    if (conn->getCreateTime() + m_maxAliveTime < sylar::GetCurrentMS()) {
                        invalid_conns.push_back(conn);
                        continue;
                    }
                    ptr = conn;
                    break;
                }
            }
            for (auto i : invalid_conns) {
                delete i;
            }
            m_total -= invalid_conns.size();
            // 创建新连接
            if (!ptr) {
                IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
                if (!addr) {
                    SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
                    return nullptr;
                }
                addr->setPort(m_port);
                Socket::ptr sock = Socket::CreateTCP(addr);
                if (!sock) {
                    SYLAR_LOG_ERROR(g_logger) << "create sock fail: " << addr->toString();
                    return nullptr;
                }
                if (!sock->connect(addr)) {
                    SYLAR_LOG_ERROR(g_logger) << "sock connect fail: " << addr->toString();
                    return nullptr;
                }
                ptr = new HttpConnection(sock);
                ++m_total;
            }
            return HttpConnection::ptr(ptr, std::bind(&ReleasePtr, std::placeholders::_1, this));
        }

        HttpResult::ptr HttpConnectionPool::doGet(const std::string& url,
                                                  uint64_t timeout_ms,
                                                  const std::map<std::string, std::string>& headers,
                                                  const std::string& body) {
            return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri,
                                                  uint64_t timeout_ms,
                                                  const std::map<std::string, std::string>& headers,
                                                  const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return doGet(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doPost(const std::string& url,
                                                   uint64_t timeout_ms,
                                                   const std::map<std::string, std::string>& headers,
                                                   const std::string& body) {
            return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri,
                                                   uint64_t timeout_ms,
                                                   const std::map<std::string, std::string>& headers,
                                                   const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return doPost(ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method,
                                                      const std::string& url,
                                                      uint64_t timeout_ms,
                                                      const std::map<std::string, std::string>& headers,
                                                      const std::string& body) {
            HttpRequest::ptr req = std::make_shared<HttpRequest>();
            req->setPath(url);
            req->setMethod(method);
            bool has_host = false;
            for (auto& it : headers) {
                if (strcasecmp(it.first.c_str(), "connection") == 0) {
                    if (strcasecmp(it.second.c_str(), "keep-alive") == 0) {
                        req->setClose(false);
                    }
                    continue;
                }
                if (!has_host && strcasecmp(it.first.c_str(), "host") == 0) {
                    has_host = !it.second.empty();
                }
                req->setHeader(it.first, it.second);
            }
            if (!has_host) {
                if (m_vhost.empty()) {
                    req->setHeader("Host", m_host);
                } else {
                    req->setHeader("Host", m_vhost);
                }
            }
            req->setBody(body);
            return doRequest(req, timeout_ms);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method,
                                                      Uri::ptr uri,
                                                      uint64_t timeout_ms,
                                                      const std::map<std::string, std::string>& headers,
                                                      const std::string& body) {
            std::stringstream ss;
            ss << uri->getPath()
                << (uri->getQuery().empty() ? "" : "?")
                << uri->getQuery()
                << (uri->getFragment().empty() ? "" : "#")
                << uri->getFragment();
            return doRequest(method, ss.str(), timeout_ms, headers, body);
        }

        HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req,
                                                      uint64_t timeout_ms) {
            auto conn = getConnection();
            if (!conn) {
                return std::make_shared<HttpResult>(HttpResult::Status::POOL_INVAALID_CONNECTION, nullptr,
                                                    "pool host:" + m_host + " port:" + std::to_string(m_port));
            }
            auto sock = conn->getSocket();
            if (!sock) {
                return std::make_shared<HttpResult>(HttpResult::Status::POOL_INVAALID_CONNECTION, nullptr,
                                                    "pool host:" + m_host + " port:" + std::to_string(m_port));
            }
            sock->setRecvTimeout(timeout_ms);
            int ret = conn->sendRequest(req);
            if (ret == 0) {
                return std::make_shared<HttpResult>(HttpResult::Status::SEND_CLOSE_BY_PEER, nullptr,
                                                    "send request closed by peer: " + sock->getRemoteAddress()->toString());
            }
            if (ret < 0) {
                return std::make_shared<HttpResult>(HttpResult::Status::SEND_SOCKET_ERROR, nullptr,
                                                    "send request socket error, errno=" + std::to_string(errno)
                                                    + " errstr=" + strerror(errno));
            }
            auto rsp = conn->recvResponse();
            if (!rsp) {
                return std::make_shared<HttpResult>(HttpResult::Status::TIMEOUT, nullptr,
                                                    "recv response timeout: " + sock->getRemoteAddress()->toString()
                                                    + " timeout_ms:" + std::to_string(timeout_ms));
            }
            return std::make_shared<HttpResult>(HttpResult::Status::OK, rsp, "ok");
        }

        void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
            ptr->addRequest();
            if (!ptr->isConnected()
                || ptr->getCreateTime() + pool->m_maxAliveTime < sylar::GetCurrentMS()
                || ptr->getRequest() >= pool->m_maxRequest
                || pool->m_conns.size() >= pool->m_maxSize) {
                delete ptr;
                --pool->m_total;
                return;
            }
            MutexType::Lock lock(pool->m_mutex);
            pool->m_conns.push_back(ptr);
        }
    }
}