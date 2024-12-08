#include "scheduler.h"
#include "macro.h"
#include <string>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    // static thread_local Scheduler::ptr t_scheduler = nullptr;
    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber::ptr t_scheduler_fiber = nullptr;         // 当前线程的调度协程

    Fiber::ptr Scheduler::GetSchedulerFiber() {
        return t_scheduler_fiber;
    }

    // Scheduler::ptr Scheduler::GetThisScheduler() {
    Scheduler* Scheduler::GetThisScheduler() {
        return t_scheduler;
    }

    // 在非caller线程中，调度协程就是线程的主协程
    // 在caller线程中，调度协程是caller线程的子协程
    Scheduler::Scheduler(size_t threadCount, bool usecaller, const std::string& name)
        : m_usecaller(usecaller), m_name(name), m_threadCount(threadCount) {
        SYLAR_ASSERT(m_threadCount > 0);
        m_rootThreadId = sylar::GetThreadId();
        t_scheduler = this;
        if (usecaller) {
            --m_threadCount;
            sylar::Fiber::GetMainFiber();
            t_scheduler_fiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            m_mainSchedulerFiber = t_scheduler_fiber;
            sylar::Thread::SetName(m_name);
        }
    }

    Scheduler::~Scheduler() {
        SYLAR_ASSERT(m_stopping);
        SYLAR_LOG_DEBUG(g_logger) << "~Scheduler";
        if (t_scheduler == this) {
            t_scheduler = nullptr;
        }
    }

    void Scheduler::start() {
        // t_scheduler = this->shared_from_this();
        MutexType::Lock lock(m_mutex);
        m_stopping = false;
        SYLAR_ASSERT(m_threadpool.empty());
        m_threadpool.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threadpool[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        }
        // lock.unlock();
    }

    void Scheduler::stop() {
        m_stopping = true;
        if (m_usecaller && !stopping()) {
            m_mainSchedulerFiber->call();
        }
        for (auto& it : m_threadpool) {
            it->join();
        }
    }

    void Scheduler::run() {
        SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
        // t_scheduler = this->shared_from_this();
        t_scheduler = this;
        if (sylar::GetThreadId() != m_rootThreadId) {               // 非caller线程，此时创建调度协程（即线程的主协程）
            t_scheduler_fiber = Fiber::GetMainFiber();
        }
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

        while (true) {
            FiberTask fibertask;
            bool tickle_me = false;
            bool isactive = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibertasks.begin();
                while (it != m_fibertasks.end()) {
                    if (it->target_thread_id != -1 && it->target_thread_id != sylar::GetThreadId()) {
                        ++it;
                        tickle_me = true;
                        continue;
                    }
                    SYLAR_ASSERT(it->fiber || it->cb);
                    if (it->fiber && it->fiber->getState() == Fiber::State::EXEC) {
                        ++it;
                        continue;
                    }
                    fibertask = *it;
                    m_fibertasks.erase(it++);
                    ++m_activeThreadCount;
                    isactive = true;
                    break;
                }
                tickle_me |= it != m_fibertasks.end();
            }
            if (tickle_me) {
                tickle();
            }
            if (fibertask.cb) {
                Fiber::ptr cb_fiber(new Fiber(fibertask.cb));
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::State::READY) {
                    schedule(cb_fiber);
                }
            } else if (fibertask.fiber && fibertask.fiber->getState() == Fiber::State::READY) {
                fibertask.fiber->swapIn();
                --m_activeThreadCount;
                if (fibertask.fiber->getState() == Fiber::State::READY) {
                    schedule(fibertask.fiber);
                }
            } else {
                if (isactive) {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::State::TERM) {
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
            }
        }
    }

    void Scheduler::idle() {
        SYLAR_LOG_INFO(g_logger) << "idle";
        while (!stopping()) {
            sylar::Fiber::YieldToHold();
        }
    }

    void Scheduler::tickle() {
        // SYLAR_LOG_INFO(g_logger) << "tickle";
    }

    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_stopping && m_fibertasks.empty() && m_activeThreadCount == 0;
    }

}