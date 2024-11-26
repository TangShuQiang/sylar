#include "thread.h"
#include "log.h"
#include <string.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOWN";

    Thread::Thread(std::function<void()> cb, const std::string& name) : m_cb(cb), m_name(name) {
        if (m_name.empty()) {
            m_name = "UNKNOW";
        }
        int ret = pthread_create(&m_pid, nullptr, &Thread::run, this);
        if (ret != 0) {
            SYLAR_LOG_FMT_ERROR(g_logger, "pthread_create: %s", strerror(ret));
            throw std::logic_error("pthread_create error");
        }
        // 确保线程的初始化完成后，主线程才能继续运行
        m_semaphore.wait();
    }

    Thread::~Thread() {
        if (m_pid) {
            pthread_detach(m_pid);
        }
    }

    void Thread::join() {
        if (m_pid) {
            int ret = pthread_join(m_pid, nullptr);
            if (ret != 0) {
                SYLAR_LOG_FMT_ERROR(g_logger, "pthread_join: %s", strerror(ret));
                throw std::logic_error("pthread_join error");
            }
            m_pid = 0;
        }
    }

    Thread* Thread::GetThis() {
        return t_thread;
    }

    const std::string& Thread::GetName() {
        return t_thread_name;
    }

    void Thread::SetName(const std::string& name) {
        if (!name.empty()) {
            if (t_thread) {
                t_thread->m_name = name;
            }
            t_thread_name = name;
        }
    }

    void* Thread::run(void* arg) {
        Thread* thread = (Thread*)arg;
        thread->m_threadId = sylar::GetThreadId();
        t_thread = thread;
        t_thread_name = thread->m_name;
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
        std::function<void()> cb;
        cb.swap(thread->m_cb);
        thread->m_semaphore.notify();
        cb();
        pthread_exit(nullptr);
    }
}