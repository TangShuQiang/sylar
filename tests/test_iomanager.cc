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

sylar::Timer::ptr timer = nullptr;

void test_timer() {
    SYLAR_LOG_INFO(g_logger) << "test_timer";
    sylar::IOManager iom(3, false, "iomanager");
    timer = iom.addTimer(1000, []() {
        static int i = 0;
        SYLAR_LOG_INFO(g_logger) << "----------------------------------------------hello timer i = " << i;
        if (++i == 3) {
            timer->reset(2000, true);
            // timer->cancel();
        }
    }, true);
}

int main() {
    // test1();
    test_timer();
}