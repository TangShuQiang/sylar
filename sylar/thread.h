#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>
#include <functional>
#include <string>
#include <memory>
#include "noncopyable.h"
#include "mutex.h"

namespace sylar
{
    class Thread : Noncopyable
    {
    public:
        using ptr = std::shared_ptr<Thread>;
        Thread(std::function<void()> cb, const std::string& name);
        ~Thread();
        pid_t getId() const { return m_threadId; }
        const std::string& getName() const { return m_name; }
        void join();
        static Thread* GetThis();
        static const std::string& GetName();
    private:
        static void* run(void* arg);
    private:
        pid_t m_threadId = -1;            // 在日志中输出的线程id
        pthread_t m_pid = 0;            // pthead_create的pid
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;
    };
}

#endif