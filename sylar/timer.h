#ifndef __SALAR_TIMER_H__
#define __SALAR_TIMER_H__

#include <memory>
#include <set>
#include <vector>
#include <functional>
#include "mutex.h"

namespace sylar
{
    class TimerManager;

    class Timer : public std::enable_shared_from_this<Timer>
    {
        friend class TimerManager;
    public:
        using ptr = std::shared_ptr<Timer>;

        bool cancel();      // 取消定时器
        bool refresh();     // 刷新定时器的执行时间
        // ms: 定时器执行间隔时间       from_now：是否从当前时间开始计算
        bool reset(uint64_t ms, bool from_now);     // 重置定时器时间

    private:
        Timer(uint64_t next);   // 查找时使用
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);

        struct Comparator
        {
            bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
        };

    private:
        bool m_recurring = false;                   // 是否循环定时器
        uint64_t m_ms = 0;                          // 执行周期(隔多少毫秒执行一次)
        uint64_t m_next = 0;                        // 下次执行的具体时间
        std::function<void()> m_cb;                 // 回调函数
        TimerManager* m_manager = nullptr;          // 定时器管理器
    };

    class TimerManager
    {
        friend class Timer;
    public:
        using RWMutexType = RWMutex;

        TimerManager();
        virtual ~TimerManager() {}
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);
        // 添加条件定时器
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

        // 离最近一个定时器执行的时间间隔
        uint64_t getNextTimer();

        void listExpiredCb(std::vector<std::function<void()>>& vec);

    protected:
        virtual void onTimerInsertedAtFront() = 0;
        void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        bool m_tickled = false;                    // 是否触发onTimerInsertedAtFront
        uint64_t m_previousTime = 0;               // 上次执行时间
    };
}

#endif