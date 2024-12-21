#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar
{
    class IOManager : public Scheduler, public TimerManager
    {
    public:
        using RWMutexType = RWMutex;
        using ptr = std::shared_ptr<IOManager>;

        IOManager(size_t threadCount = 1, bool usecaller = true, const std::string& name = "");
        ~IOManager();

        bool hasIdleThreads() const { return m_idleThreadCount > 0; }
        static IOManager* GetThisIOManager();

        enum Event
        {
            NONE = 0x0,
            READ = 0x1,     // EPOLLIN
            WRITE = 0x4     // EPOLLOUT
        };

        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        // bool cancelEvent(int fd, Event event);
        // bool cancelAll(int fd);

    protected:
        void idle() override;
        void tickle() override;
        bool stopping() override;
        void onTimerInsertedAtFront() override;

        bool stopping(uint64_t& timeout);

    private:
        // 事件上下文类
        struct EventContext
        {
            Scheduler* scheduler = nullptr;
            Fiber::ptr fiber = nullptr;
            std::function<void()> cb = nullptr;
        };

        // Socket事件上下文类
        struct FdContext
        {
            using MutexType = Mutex;

            EventContext& getContext(Event event);               // 获取事件上下文类
            void resetContext(Event event);                      // 重置事件上下文
            void triggerEvent(Event event);                      // 触发事件

            EventContext readEventContext;                      // 读事件的上下文
            EventContext writeEventContext;                     // 写事件的上下文
            Event events = Event::NONE;                         // 当前的事件
            int fd;                                             // 事件关联的句柄
            MutexType mutex;
        };

        void contextResize(size_t size);

    private:
        int m_epollfd;
        int m_eventfd;
        std::atomic<size_t> m_pendingEventCount{ 0 };      // 当前等待执行的事件数量
        RWMutexType m_mutex;
        std::vector<FdContext*> m_fdContexts;           // socket事件上下文的容器
    };
}

#endif