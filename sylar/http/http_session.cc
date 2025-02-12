#include "http_session.h"
#include "http_parser.h"

namespace sylar
{
    namespace http
    {
        HttpSession::HttpSession(Socket::ptr socket, bool owner)
            : SocketStream(socket, owner) {}

        HttpRequest::ptr HttpSession::recvRequest() {
            HttpRequestParser::ptr parser = std::make_shared<HttpRequestParser>();
            uint64_t buffSize = HttpRequestParser::GetHttpRequestBufferSize();
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
                size_t nparse = parser->execute(data, len);
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
            } while(true);
            int64_t contentLength = parser->getContentLength();
            if (contentLength > 0) {
                std::string body;
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
                parser->getData()->setBody(body);
            }
            parser->getData()->init();
            return parser->getData();
        }

        int HttpSession::sendResponse(HttpResponse::ptr rsp) {
            std::string data = rsp->toString();
            return writeFixSize(data.c_str(), data.size());
        }
    }
}