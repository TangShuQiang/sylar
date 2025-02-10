#include "log.h"
#include "iomanager.h"
#include "tcp_server.h"
#include "address.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public sylar::TcpServer
{
public:
    EchoServer(int type = 0);
    void handleClient(sylar::Socket::ptr client) override;
private:
    int m_type;         // 0: 字符  1:二进制
};

EchoServer::EchoServer(int type) : m_type(type) {}

void EchoServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient" << client->toString();
    while (true) {
        std::string buff;
        buff.resize(1024);
        int ret = client->recv(&buff[0], buff.size());
        if (ret == 0) {
            SYLAR_LOG_INFO(g_logger) << "client close: " << client->toString();
            break;
        } else if (ret < 0) {
            SYLAR_LOG_ERROR(g_logger) << "client error ret=" << ret
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        buff.resize(ret);
        std::cout << buff << std::endl;
    }
}

int type = 0;

void run() {
    SYLAR_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr echoServer = std::make_shared<EchoServer>(type);
    auto addr = sylar::Address::LookupAny("0.0.0.0:8020");
    while (!echoServer->bind(addr)) {
        sleep(2);
    }
    echoServer->start();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        SYLAR_LOG_INFO(g_logger) << "used as [" << argv[0] << " -t] or ["
            << argv[0] << " -b]";
        return 0;
    }
    if (!strcmp(argv[1], "-b")) {
        type = 1;
    }
    sylar::IOManager iom(5);
    iom.schedule(run);
}