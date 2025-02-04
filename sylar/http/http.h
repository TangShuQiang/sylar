#ifndef __SYLAR_HTTP_H__
#define __SYLAR_HTTP_H__

#include <memory>
#include <map>
#include <string>
#include <stdint.h>
#include <boost/lexical_cast.hpp>

/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

namespace sylar
{
    namespace http
    {
        // HTTP方法枚举
        enum class HttpMethod
        {
        #define XX(num, name, desc)   name = num,
            HTTP_METHOD_MAP(XX)
        #undef XX
            INVALID_METHOD
        };

        // -----------------------------------------------------------------------------

        // HTTP状态枚举
        enum class HttpStatus
        {
        #define XX(code, name, desc) name = code,
            HTTP_STATUS_MAP(XX)
        #undef XX
        };

        // -----------------------------------------------------------------------------

        // 将字符串方法名转成HTTP方法枚举
        HttpMethod StringToHttpMethod(const std::string& m);
        // 将字符串指针转成HTTP方法枚举
        HttpMethod CharsToHttpMethod(const char* m);
        // 将HTTP方法枚举转换成字符串
        const char* HttpMethodToString(const HttpMethod& m);
        // 将HTTP状态枚举转换成字符串
        const char* HttpStatusToString(const HttpStatus& s);

        // 忽略大小写的比较仿函数
        struct CaseInsensitiveLess
        {
            bool operator() (const std::string& lhs, const std::string& rhs) const;
        };

        /*
            检查获取map中的key值，并转成对应的类型，返回是否成功
            true：转换成功，val为对应的值
            false：不存在或者转换失败，val=def
        */
        template<typename MapType, typename T>
        bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T()) {
            auto it = m.find(key);
            if (it == m.end()) {
                val = def;
                return false;
            }
            try {
                val = boost::lexical_cast<T>(it->second);
                return true;
            } catch (...) {
                val = def;
            }
            return false;
        }

        // 获取map中的key值，并转成对应的类型，如果存在且转换成功返回对应的值，否则返回默认值
        template<typename MapType, typename T>
        T getAs(const MapType& m, const std::string& key, const T& def = T()) {
            auto it = m.find(key);
            if (it == m.end()) {
                return def;
            }
            try {
                return boost::lexical_cast<T>(it->second);
            } catch (...) {}
            return def;
        }

        // -----------------------------------------------------------------------------

        class HttpResponse;

        class HttpRequest
        {
        public:
            using ptr = std::shared_ptr<HttpRequest>;
            using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

            HttpRequest(uint8_t version = 0x11, bool close = true);

            // std::shared_ptr<HttpResponse> createResponse();

            HttpMethod getMethod() const { return m_method; }
            uint8_t getVersion() const { return m_version; }
            bool isClose() const { return m_close; }
            bool isWebsocket() const { return m_websocket; }
            const std::string& getPath() const { return m_path; }
            const std::string& getQuery() const { return m_query; }
            const std::string& getFragment() const { return m_fragment; }
            const std::string& getBody() const { return m_body; }
            const MapType& getHeaders() const { return m_headers; }
            const MapType& getParams() const { return m_params; }
            const MapType& getCookies() const { return m_cookies; }

            void setMethod(HttpMethod method) { m_method = method; }
            void setVersion(uint8_t version) { m_version = version; }
            void setClose(bool v) { m_close = v; }
            void setWebsocket(bool v) { m_websocket = v; }
            void setPath(const std::string& path) { m_path = path; }
            void setQuery(const std::string& query) { m_query = query; }
            void setFragment(const std::string& fragment) { m_fragment = fragment; }
            void setBody(const std::string& body) { m_body = body; }
            void setHeaders(const MapType& headers) { m_headers = headers; }
            void setParams(const MapType& params) { m_params = params; }
            void setCookies(const MapType& cookies) { m_cookies = cookies; }

            // 获取HTTP请求的头部参数：如果存在则返回对应值，否则返回默认值
            std::string getHeader(const std::string& key, const std::string& def = "") const;
            std::string getParam(const std::string& key, const std::string& def = "");
            std::string getCookie(const std::string& key, const std::string& def = "");

            void setHeader(const std::string& key, const std::string& val);
            void setParam(const std::string& key, const std::string& val);
            void setCookie(const std::string& key, const std::string& val);

            void delHeader(const std::string& key);
            void delParam(const std::string& key);
            void delCookie(const std::string& key);

            // 判断HTTP请求的请求参数是否存在：如果存在，val非空则赋值
            bool hasHeader(const std::string& key, std::string* val = nullptr);
            bool hasParam(const std::string& key, std::string* val = nullptr);
            bool hasCookie(const std::string& key, std::string* val = nullptr);

            // 检查并获取HTTP请求的头部参数
            template<typename T>
            bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
                return checkGetAs(m_headers, key, val, def);
            }

            // 获取HTTP请求的头部参数
            template<typename T>
            T getHeaderAs(const std::string& key, const T& def = T()) {
                return getAs(m_headers, key, def);
            }

            // 检查并获取HTTP请求的请求参数
            template<typename T>
            bool checkGetParamAs(const std::string& key, T& val, const T& def = T()) {
                return checkGetAs(m_params, key, val, def);
            }

            // 获取HTTP请求的请求参数
            template<typename T>
            T getParamAs(const std::string& key, const T& def) {
                return getAs(m_params, key, def);
            }

            // 检查并获取HTTP请求的Cookie参数
            template<typename T>
            bool checkGetCookieAs(const std::string& key, T& val, const T& def = T()) {
                return checkGetAs(m_cookies, key, val, def);
            }

            // 获取HTTP请求的Cookie参数
            template<typename T>
            T getCookieAs(const std::string& key, const T& def = T()) {
                return getAs(m_cookies, key, def);
            }

            // 序列化输出到流中
            std::ostream& dump(std::ostream& os) const;

            // 转为字符串
            std::string toString() const;

            // void initQueryParam();
            // void initBodyParam();

        private:
            HttpMethod m_method;            // HTTP方法
            uint8_t m_version;              // HTTP版本
            bool m_close;                   // 是否自动关闭
            bool m_websocket;               // 是否为websocket
            uint8_t m_parserParamFlag;
            std::string m_path;             // 请求路径
            std::string m_query;            // 请求参数
            std::string m_fragment;         // 请求fragment
            std::string m_body;             // 请求消息体
            MapType m_headers;              // 请求头部MAP
            MapType m_params;               // 请求参数MAP
            MapType m_cookies;              // 请求Cookie MAP
        };

        // -----------------------------------------------------------------------------

        class HttpResponse
        {
        public:
            using ptr = std::shared_ptr<HttpResponse>;
            using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

            HttpResponse(uint8_t version = 0x11, bool close = true);

            HttpStatus getStatus() const { return m_status; }
            uint8_t getVersion() const { return m_version; }
            bool isClose() const { return m_close; }
            bool isWebsocket() const { return m_websocket; }
            const std::string& getBody() const { return m_body; }
            const std::string& getReason() const { return m_reason; }
            const MapType& getHeaders() const { return m_headers; }
            const std::vector<std::string>& getCookies() const { return m_cookies; }

            void setStatus(HttpStatus status) { m_status = status; }
            void setVersion(uint8_t version) { m_version = version; }
            void setClose(bool v) { m_close = v; }
            void setWebsocket(bool v) { m_websocket = v; }
            void setBody(const std::string& body) { m_body = body; }
            void setReason(const std::string& reason) { m_reason = reason; }
            void setHeaders(const MapType& headers) { m_headers = headers; }

            std::string getHeader(const std::string& key, const std::string& def = "") const;
            void setHeader(const std::string& key, const std::string& val);
            void delHeader(const std::string& key);

            template<typename T>
            bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T()) {
                return checkGetAs(m_headers, key, val, def);
            }

            template<typename T>
            T getHeaderAs(const std::string& key, const T& def = T()) {
                return getAs(m_headers, key, def);
            }

            std::ostream& dump(std::ostream& os) const;

            std::string toString() const;

        private:
            HttpStatus m_status;            // 响应状态
            uint8_t m_version;              // HTTP版本
            bool m_close;                   // 是否自动关闭
            bool m_websocket;               // 是否为websocket
            std::string m_body;             // 请求消息体
            std::string m_reason;           // 响应原因
            MapType m_headers;              // 响应头部MAP
            std::vector<std::string> m_cookies;
        };

    }
}


#endif