#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include "thread.h"
#include "mutex.h"
#include "fiber.h"
#include <vector>
#include <list>
#include <memory>
#include <functional>
#include <atomic>

namespace sylar
{
    class Scheduler : public std::enable_shared_from_this<Scheduler>
    // class Scheduler
    {
    public:
        using MutexType = Mutex;
        using ptr = std::shared_ptr<Scheduler>;
        Scheduler(size_t threadCount = 1, bool usecaller = true, const std::string& name = "");
        virtual ~Scheduler();

        void start();
        void stop();
        void run();
        virtual void idle();
        virtual void tickle();
        virtual bool stopping();

        static Fiber::ptr GetSchedulerFiber();
        static Scheduler::ptr GetThisScheduler();

        template<typename FiberOrCb>
        void schedule(FiberOrCb fc, int target_thread_id = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, target_thread_id);
            }
            if (need_tickle) {
                tickle();
            }
        }

        template<typename InputIterator>
        void schedule(InputIterator begin, InputIterator end, int target_thread_id = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    need_tickle |= scheduleNoLock(*begin, target_thread_id);
                    ++begin;
                }
            }
            if (need_tickle) {
                tickle();
            }
        }

    private:
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int target_thread_id) {
            bool need_tickle = m_fibertasks.empty();
            if (fc) {
                m_fibertasks.emplace_back(fc, target_thread_id);
            }
            return need_tickle;
        }

        struct FiberTask
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int target_thread_id;

            FiberTask() : fiber(nullptr), cb(nullptr), target_thread_id(-1) {}
            FiberTask(Fiber::ptr f, int thr) : fiber(f), cb(nullptr), target_thread_id(thr) {}
            FiberTask(std::function<void()> c, int thr) : fiber(nullptr), cb(c), target_thread_id(thr) {}
        };

    private:
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threadpool;         // 线程池
        std::list<FiberTask> m_fibertasks;             // 待执行的协程任务
        bool m_usecaller;
        Fiber::ptr m_mainSchedulerFiber;                // usecaller时，main线程的调度协程
        std::string m_name;
    protected:
        // std::vector<int> m_threadIds;
        size_t m_threadCount;                               // 线程数量
        int m_rootThreadId;                                 // 主线程id
        bool m_stopping = true;                            // 是否正在停止
        std::atomic<size_t> m_activeThreadCount{ 0 };        // 工作线程数量
        std::atomic<size_t> m_idleThreadCount{ 0 };         // 空闲线程数量
    };
}

#endif