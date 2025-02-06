#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include <memory>

#include "http.h"
#include "http11/http11_parser.h"
#include "http11/httpclient_parser.h"

namespace sylar
{
    namespace http
    {
        class HttpRequestParser
        {
        public:
            using ptr = std::shared_ptr<HttpRequestParser>;

            HttpRequestParser();

            /*
                解析协议
                data: 协议文本内存
                len: 协议文本内存长度
                返回实际解析的长度，并且将已解析的数据移除
            */
            size_t execute(char* data, size_t len);

            const http_parser& getParser() const { return m_parser;}
            HttpRequest::ptr getData() const { return m_data; }
            int hasError();

            // 是否解析完成
            int isFinished();

            void setError(int error) { m_error = error;}

            // 获取消息体长度
            uint64_t getContentLength();

            // 返回HttpRequest协议解析的缓存大小
            static uint64_t GetHttpRequestBufferSize();

            // 返回HttpRequest协议的最大消息体大小
            static uint64_t GetHttpRequestMaxBodySize();

        private:
            http_parser m_parser;
            HttpRequest::ptr m_data;
            /*
                1000: invalid method
                1001: invalid version
                1002: invalid field
            */
            int m_error;
        };

        // --------------------------------------------------------------------------------------
        class HttpResponseParser {
        public:
            using ptr = std::shared_ptr<HttpResponseParser>;

            HttpResponseParser();

            size_t execute(char* data, size_t len, bool chunck);

            const httpclient_parser& getParser() const { return m_parser; }
            HttpResponse::ptr getData() const { return m_data; }
            int hasError();
            int isFinished();

            void setError(int error) { m_error = error; }

            uint64_t getContentLength();

            static uint64_t GetHttpResponseBufferSize();
            static uint64_t GetHttpResponseMaxBodySize();

        private:
            httpclient_parser m_parser;
            HttpResponse::ptr m_data;
            int m_error;
        };
    }
}

#endif