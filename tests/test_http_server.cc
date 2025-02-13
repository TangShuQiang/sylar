#include "log.h"
#include "http/http_server.h"

void run() {
    sylar::http::HttpServer::ptr server = std::make_shared<sylar::http::HttpServer>(true);
    sylar::Address::ptr addr = sylar::Address::LookupAny("0.0.0.0:443");
    while (!server->bind(addr)) {
        sleep(2);
    }

    auto servletDispatch = server->getServletDispatch();
    servletDispatch->addServlet("/tsq/xx", [](sylar::http::HttpRequest::ptr req
        , sylar::http::HttpResponse::ptr rsp
        , sylar::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
        });

    servletDispatch->addGlobServlet("/tsq/*", [](sylar::http::HttpRequest::ptr req
        , sylar::http::HttpResponse::ptr rsp
        , sylar::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
        });

    server->start();
}

int main() {
    sylar::IOManager iom(2);
    iom.schedule(run);
}