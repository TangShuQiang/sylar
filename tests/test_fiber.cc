#include "fiber.h"
#include "log.h"
#include "thread.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
}


void test_fiber() {
    {
        SYLAR_LOG_INFO(g_logger) << "main begin";

        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main end";
    }
    SYLAR_LOG_INFO(g_logger) << "main end 2";
}

int main() {
    sylar::Thread::SetName("main");
    SYLAR_LOG_INFO(g_logger) << "main";
    test_fiber();
}