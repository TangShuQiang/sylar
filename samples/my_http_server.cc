#include "log.h"
#include "http/http_server.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    g_logger->setLevel(sylar::LogLevel::INFO);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if (!addr) {
        SYLAR_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    sylar::http::HttpServer::ptr http_server = std::make_shared<sylar::http::HttpServer>(true);
    while (!http_server->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "bind " << addr->toString() << " fail";
        sleep(1);
    }
    http_server->start();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(1);
    SYLAR_LOG_INFO(g_logger) << "===========";
    iom.schedule(run);
}