#include "log.h"
#include "socket.h"
#include "iomanager.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com");
    if (addr) {
        SYLAR_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "get address fail";
        return;
    }
    addr->setPort(80);
    SYLAR_LOG_INFO(g_logger) << "addr=" << addr->toString();
    sylar::Socket::ptr sock = sylar::Socket::CreateTCPSocket();
    if (!sock->connect(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "connect " << addr->toString() << " faile";
        return;
    } else {
        SYLAR_LOG_INFO(g_logger) << "connect " << addr->toString() << " success";
    }

    const char buf[] = "GET / HTTP/1.0\r\n\r\n";
    int ret = sock->send(buf, strlen(buf));
    if (ret <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "send fail ret=" << ret;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    ret = sock->recv(&buffs[0], buffs.size());

    if (ret <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "recv fail ret=" << ret;
        return;
    }

    buffs.resize(ret);
    SYLAR_LOG_INFO(g_logger) << buffs;

}

int main() {
    sylar::IOManager iom;
    iom.schedule(test);
}