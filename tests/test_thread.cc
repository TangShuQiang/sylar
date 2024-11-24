#include "thread.h"
#include "log.h"


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
// sylar::Mutex s_mutex;
// sylar::RWMutex s_mutex;
sylar::SpinLock s_mutex;

void func1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
        << " this_name: " << sylar::Thread::GetThis()->getName()
        << " id: " << sylar::GetThreadId()
        << " this_id: " << sylar::Thread::GetThis()->getId();

    for (int i = 0; i < 1000000; ++i) {
        // sylar::Mutex::Lock lock(s_mutex);
        // sylar::RWMutex::WriteLock lock(s_mutex);
        sylar::SpinLock::Lock lock(s_mutex);
        ++count;
        
    }
}

void func2() {
    int k = 1000;
    while(k--) {
        SYLAR_LOG_INFO(g_logger) << "---------------------------";
    }
}
void func3() {
    int k = 1000;
    while(k--) {
        SYLAR_LOG_INFO(g_logger) << "++++++++++++++++++++++++++++";
    }
}

int main() {

    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> vec;
    for (int i = 0; i < 8; ++i) {
        sylar::Thread::ptr th1 = std::make_shared<sylar::Thread>(&func2, "name_" + std::to_string(i * 2));
        sylar::Thread::ptr th2 = std::make_shared<sylar::Thread>(&func3, "name_" + std::to_string(i * 2 + 1));
        vec.push_back(th1);
        vec.push_back(th2);
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count = " << count;
}