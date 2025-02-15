#include "log.h"
#include "iomanager.h"
#include "http/http_connection.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("www.sylar.top:80");
    SYLAR_LOG_DEBUG(g_logger) << "\n" << addr->toString() + "\n";
    if (!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get addr error";
        return;
    }

    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    bool ret = sock->connect(addr);
    if (!ret) {
        SYLAR_LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed";
        return;
    }
    sock->setRecvTimeout(3000);

    sylar::http::HttpConnection::ptr conn = std::make_shared<sylar::http::HttpConnection>(sock);
    sylar::http::HttpRequest::ptr req = std::make_shared<sylar::http::HttpRequest>();
    req->setPath("/blog/");
    req->setHeader("Host", "www.sylar.top");
    SYLAR_LOG_INFO(g_logger) << "req:\n" << req->toString();
    conn->sendRequest(req);

    auto rsp = conn->recvResponse();
    if (!rsp) {
        SYLAR_LOG_ERROR(g_logger) << "recv response error";
        return;
    }
    SYLAR_LOG_INFO(g_logger) << "rsp:\n" << rsp->toString();

    SYLAR_LOG_INFO(g_logger) << "========================================";
    auto httpresult = sylar::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 3000);
    SYLAR_LOG_INFO(g_logger) << "status=" << (int)httpresult->status
        << " desc=" << httpresult->description
        << " rsp=" << (httpresult->response ? httpresult->response->toString() : "nullptr");
}

void test_pool() {
    sylar::http::HttpConnectionPool::ptr pool
        = std::make_shared<sylar::http::HttpConnectionPool>("www.sylar.top", "", 80, false, 10, 1000 * 30, 5);

    sylar::IOManager::GetThisIOManager()->addTimer(1000, [pool]() {
        auto r = pool->doGet("/", 300);
        SYLAR_LOG_INFO(g_logger) << r->toString();}
    , false);
}

int main() {
    sylar::IOManager iom(3);
    // iom.schedule(run);
    iom.schedule(test_pool);
}