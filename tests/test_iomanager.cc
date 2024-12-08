#include "iomanager.h"
#include "log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("root");

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber begin";
    sylar::IOManager::GetThisIOManager();

}

void test1() {
    SYLAR_LOG_INFO(g_logger) << "test_iomanager";
    sylar::IOManager iom(3, false, "iomanager");
    sleep(5);
    // iom.schedule(&test_fiber);
    // sleep(2);
    iom.schedule(&test_fiber);
    // sleep(20);
    sylar::IOManager::GetThisIOManager();
}

int main() {
    test1();
}