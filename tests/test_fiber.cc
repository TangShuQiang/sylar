#include "fiber.h"
#include "log.h"
#include "thread.h"
#include "thread.h"
#include <vector>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run in fiber begin";
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "run in fiber end";    
}


void test_fiber() {
    {
        SYLAR_LOG_INFO(g_logger) << "main begin";
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main continue";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main end";
    }
    SYLAR_LOG_INFO(g_logger) << "main end 2";
}

int main() {
    sylar::Thread::SetName("main");
    SYLAR_LOG_INFO(g_logger) << "main";
    // test_fiber();

    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(sylar::Thread::ptr(
                    new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
}