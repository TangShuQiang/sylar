#include "thread.h"
#include "log.h"


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
// sylar::Mutex s_mutex;
sylar::RWMutex s_mutex;

void func1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
        << " this_name: " << sylar::Thread::GetThis()->getName()
        << " id: " << sylar::GetThreadId()
        << " this_id: " << sylar::Thread::GetThis()->getId();

    for (int i = 0; i < 10000000; ++i) {
        // sylar::Mutex::Lock lock(s_mutex);
        sylar::RWMutex::WriteLock lock(s_mutex);
        ++count;
        
    }
}

int main() {

    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> vec;
    for (int i = 0; i < 16; ++i) {
        sylar::Thread::ptr th = std::make_shared<sylar::Thread>(&func1, "name_" + std::to_string(i));
        vec.push_back(th);
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count = " << count;
}