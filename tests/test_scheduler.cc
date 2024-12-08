#include "scheduler.h"
#include "log.h"
#include <memory>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    SYLAR_LOG_INFO(g_logger) << "test in fiber s_count = " << s_count;

    sleep(1);
    if (--s_count >= 0) {
        // sylar::Scheduler::GetThisScheduler()->schedule(&test_fiber, sylar::GetThreadId());
        sylar::Scheduler::GetThisScheduler()->schedule(sylar::Fiber::ptr(new sylar::Fiber(&test_fiber)), sylar::GetThreadId());
    }
}

int main() {

    // sylar::Thread::SetName("main");

    SYLAR_LOG_INFO(g_logger) << "begin";

    // std::shared_ptr<sylar::Scheduler> sc = std::make_shared<sylar::Scheduler>(5, true, "test");
    // sc->start();
    // sc->schedule(&test_fiber);
    // sc->stop();

    sylar::Scheduler sc(5, true, "test");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();

    SYLAR_LOG_INFO(g_logger) << "over";

}