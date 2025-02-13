#ifndef __SYLAR_SERVLET_H__
#define __SYLAR_SERVLET_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "http.h"
#include "http_session.h"
#include "thread.h"

namespace sylar
{
    namespace http
    {
        class Servlet
        {
        public:
            using ptr = std::shared_ptr<Servlet>;

            Servlet(const std::string& name) : m_name(name) {}

            virtual ~Servlet() {}

            // 处理请求
            virtual int32_t handler(sylar::http::HttpRequest::ptr request
                , sylar::http::HttpResponse::ptr response
                , sylar::http::HttpSession::ptr session) = 0;

            const std::string& getName() const { return m_name; }
        private:
            std::string m_name;
        };

        // 函数式 Servlet
        class FunctionServlet : public Servlet
        {
        public:
            using ptr = std::shared_ptr<FunctionServlet>;
            using callback = std::function<int32_t(sylar::http::HttpRequest::ptr
                , sylar::http::HttpResponse::ptr
                , sylar::http::HttpSession::ptr)>;
            
            FunctionServlet(callback cb);

            int32_t handler(sylar::http::HttpRequest::ptr request
                , sylar::http::HttpResponse::ptr response
                , sylar::http::HttpSession::ptr session) override;
        private:
            callback m_cb;
        };

        class ServletDispatch : public Servlet
        {
        public:
            using ptr = std::shared_ptr<ServletDispatch>;
            using RWMutexType = RWMutex;

            ServletDispatch();

            int32_t handler(sylar::http::HttpRequest::ptr request
                , sylar::http::HttpResponse::ptr response
                , sylar::http::HttpSession::ptr session) override;

            void addServlet(const std::string& uri, Servlet::ptr slt);
            void addServlet(const std::string& uri, FunctionServlet::callback cb);

            void addGlobServlet(const std::string& uri, Servlet::ptr slt);
            void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

            void delServlet(const std::string& uri);
            void delGlobServlet(const std::string& uri);

            Servlet::ptr getDefault() const { return m_default;}
            void setDefault(Servlet::ptr v) { m_default = v;}

            Servlet::ptr getServlt(const std::string& uri);
            Servlet::ptr getGlobServlt(const std::string& uri);

            Servlet::ptr getMatchedServlet(const std::string& uri);

        private:
            RWMutexType m_mutex;
            // 精准匹配: uri(/abc/def) -> servlet
            std::unordered_map<std::string, Servlet::ptr> m_datas;
            // 模糊匹配: uri(/abc/*) -> servlet
           std::vector<std::pair<std::string, Servlet::ptr>> m_globs;
           Servlet::ptr m_default;      // 默认servlet，所有路径都没匹配到时使用
        };

        class NotFoundServlet : public Servlet
        {
        public:
            using ptr = std::shared_ptr<NotFoundServlet>;

            NotFoundServlet(const std::string& name);

            int32_t handler(sylar::http::HttpRequest::ptr request
                , sylar::http::HttpResponse::ptr response
                , sylar::http::HttpSession::ptr session) override;
        private:
            std::string m_name;
            std::string m_content;
        };
    }
}

#endif