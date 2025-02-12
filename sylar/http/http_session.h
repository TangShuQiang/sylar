#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "streams/socket_stream.h"
#include "http.h"

namespace sylar
{
    namespace http
    {
        class HttpSession : public SocketStream
        {
        public:
            using ptr = std::shared_ptr<HttpSession>;

            HttpSession(Socket::ptr socket, bool owner = true);

            // 接收 HTTP 请求
            HttpRequest::ptr recvRequest();

            // 发送 HTTP 响应
            int sendResponse(HttpResponse::ptr rsp);
        };
    }
}

#endif