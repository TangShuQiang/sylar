#include "hook.h"
#include "log.h"
#include "iomanager.h"
#include "fd_manager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void test_sleep() {
    sylar::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        SYLAR_LOG_INFO(g_logger) << "sleep 2";
        });

    iom.schedule([]() {
        sleep(4);
        SYLAR_LOG_INFO(g_logger) << "sleep 4";
        });
    SYLAR_LOG_INFO(g_logger) << "test_sleep";
}

void test_sock() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("182.61.200.6");

    SYLAR_LOG_INFO(g_logger) << "begin connect";
    int ret = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
    SYLAR_LOG_INFO(g_logger) << "connect ret=" << ret << " errno=" << errno;
    if (ret == -1) {
        return;
    }
    
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    ret = send(sockfd, data, sizeof(data), 0);
    SYLAR_LOG_INFO(g_logger) << "send ret=" << ret << " errno=" << errno;
    if (ret == -1) {
        return;
    }

    std::string buff;
    buff.resize(4096);
    ret = recv(sockfd, &buff[0], buff.size(), 0);
    SYLAR_LOG_INFO(g_logger) << "recv ret=" << ret << " errno=" << errno;
    if (ret == -1) {
        return;
    }
    buff.resize(ret);
    SYLAR_LOG_INFO(g_logger) << buff;
}

int main() {
    // test_sleep();
    sylar::IOManager iom;
    iom.schedule(test_sock);

}