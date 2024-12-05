#ifndef __SALAR_FIBER_H__
#define __SALAR_FIBER_H__

#include <functional>
#include <ucontext.h>
#include <stdint.h>
#include <memory>

namespace sylar
{
    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        using ptr = std::shared_ptr<Fiber>;

        enum class State
        {
            // INIT,       // 初始化
            // HOLD,       // 暂停
            EXEC,       // 执行中
            TERM,       // 结束
            READY,      // 可执行
            EXCEPT      // 异常
        };
    private:
        Fiber();
    public:
        Fiber(std::function<void()> cb, size_t stacksize = 0, bool usecaller = false);
        ~Fiber();
        uint64_t getId() const { return m_id; }
        State getState() const { return m_state; }

        void swapIn();
        void swapOut();
        void call();

        static void SetThis(Fiber::ptr f);     // 设置当前线程的运行协程
        static Fiber::ptr GetThis();       // 返回当前所在的协程
        static Fiber::ptr GetMainFiber();
        static void MainFunc();
        static void CallerMainFunc();
        static uint64_t GetFiberId();
        static void YieldToHold();

    private:
        uint64_t m_id;
        uint32_t m_stacksize;
        State m_state;
        ucontext_t m_ctx;
        void* m_stack;
        std::function<void()> m_cb;
    };
}

#endif