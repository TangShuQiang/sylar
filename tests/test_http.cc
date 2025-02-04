#include "log.h"
#include "http/http.h"


void test_request() {
    sylar::http::HttpRequest::ptr req = std::make_shared<sylar::http::HttpRequest>();
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello world!");
    std::cout << req->toString() << std::endl;
}

void test_response() {
    sylar::http::HttpResponse::ptr rsp = std::make_shared<sylar::http::HttpResponse>();
    rsp->setHeader("X-X", "sylar");
    rsp->setBody("HELLO WORLD!");
    rsp->setStatus((sylar::http::HttpStatus)400);
    rsp->setClose(false);
    std::cout << rsp->toString() << std::endl;
}

int main() {
    test_request();

    std::cout << "------------------------" << std::endl;
    
    test_response();
}