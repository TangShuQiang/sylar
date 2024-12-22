#include "hook.h"
#include "log.h"
#include "iomanager.h"

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

int main() {
    test_sleep();
}