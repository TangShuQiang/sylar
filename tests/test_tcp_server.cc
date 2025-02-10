#include "tcp_server.h"
#include "log.h"
#include "iomanager.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
    auto addr = sylar::Address::LookupAny("0.0.0.0:8033");
    sylar::TcpServer::ptr tcpServer = std::make_shared<sylar::TcpServer>();
    while (!tcpServer->bind(addr)) {
        sleep(2);
    }
    tcpServer->start();
}

int main() {
    sylar::IOManager iom(2);
    iom.schedule(run);
}