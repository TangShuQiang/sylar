#include "fiber.h"
#include "log.h"
#include "config.h"
#include <atomic>
#include "util.h"
#include "macro.h"
#include <stdlib.h>
#include "scheduler.h"

namespace sylar
{
    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");
    static std::atomic<uint64_t> s_fiber_count(0);
    static std::atomic<uint64_t> s_fiber_id(0);

    // static thread_local Fiber::ptr t_fiber = nullptr;         // 当前线程正在运行的协程
    static thread_local Fiber* t_fiber = nullptr;             // 当前线程正在运行的协程
    static thread_local Fiber::ptr t_mainFiber = nullptr;     // 主协程

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Add<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

    class StackAllocator
    {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }
        static void Dealloc(void* p) {
            free(p);
        }
    };

    Fiber::Fiber() : m_id(0), m_stacksize(0), m_state(State::EXEC), m_stack(nullptr) {
        if (getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }
        ++s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber()";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool usecaller)
        : m_id(++s_fiber_id), m_state(State::READY), m_cb(cb) {
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        // m_ctx.uc_link = &GetMainFiber()->m_ctx;
        // m_ctx.uc_link = &Scheduler::GetSchedulerFiber()->m_ctx;
        ++s_fiber_count;
        if (usecaller) {
            m_ctx.uc_link = &GetMainFiber()->m_ctx;
            makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        } else {
            m_ctx.uc_link = &Scheduler::GetSchedulerFiber()->m_ctx;
            makecontext(&m_ctx, &Fiber::MainFunc, 0);
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
    }

    Fiber::~Fiber() {
        if (m_stack) {
            SYLAR_ASSERT(m_state == State::TERM
                || m_state == State::EXCEPT
                || m_state == State::READY);
            StackAllocator::Dealloc(m_stack);
        } else {
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == State::EXEC);
            // Fiber::ptr cur = t_fiber;
            // if (cur.get() == this) {
            //     SetThis(nullptr);
            // }
            if (t_fiber == this) {
                t_fiber = nullptr;
            }
        }
        --s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
    }

    void Fiber::swapIn() {
        // SetThis(shared_from_this());
        SetThis(this);
        SYLAR_ASSERT(m_state != State::EXEC);
        m_state = State::EXEC;
        // if (swapcontext(&GetMainFiber()->m_ctx, &m_ctx)) {
        if (swapcontext(&Scheduler::GetSchedulerFiber()->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::swapOut() {
        // SetThis(GetMainFiber()->shared_from_this());
        // SetThis(Scheduler::GetSchedulerFiber()->shared_from_this());
        SetThis(Scheduler::GetSchedulerFiber().get());
        // if (swapcontext(&m_ctx, &GetMainFiber()->m_ctx)) {
        if (swapcontext(&m_ctx, &Scheduler::GetSchedulerFiber()->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }

    // 单独测试Fiber
    // void Fiber::swapOut() {
    //     SetThis(GetMainFiber().get());
    //     if (swapcontext(&m_ctx, &GetMainFiber()->m_ctx)) {
    //         SYLAR_ASSERT2(false, "swapcontext");
    //     }
    // }

    void Fiber::call() {
        // SetThis(shared_from_this());
        SetThis(this);
        SYLAR_ASSERT(m_state != State::EXEC);
        m_state = State::EXEC;
        if (swapcontext(&GetMainFiber()->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }


    /// ------------------------------------------------------

    // 设置当前线程的运行协程
    // void Fiber::SetThis(Fiber::ptr p) {
    void Fiber::SetThis(Fiber* f) {
        t_fiber = f;
    }

    // 返回当前所在的协程
    // Fiber::ptr Fiber::GetThis() {
    Fiber* Fiber::GetThis() {
        if (t_fiber) {
            return t_fiber;
        }
        // t_mainFiber = std::make_shared<Fiber>();
        // SetThis(t_mainFiber);
        // SYLAR_ASSERT(t_fiber == t_mainFiber.get());
        // return t_mainFiber;
        if (!t_mainFiber) {
            t_mainFiber = GetMainFiber();
        }
        t_fiber = t_mainFiber.get();
        return t_fiber;
    }

    Fiber::ptr Fiber::GetMainFiber() {
        if (t_mainFiber) {
            return t_mainFiber;
        }
        t_mainFiber.reset(new Fiber());
        return t_mainFiber;
    }

    void Fiber::MainFunc() {
        // Fiber::ptr cur = GetThis();
        Fiber* cur = GetThis();
        SYLAR_ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        } catch (std::exception& ex) {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        }
        SetThis(Scheduler::GetSchedulerFiber().get());
    }

    void Fiber::CallerMainFunc() {
        // Fiber::ptr cur = GetThis();
        Fiber* cur = GetThis();
        SYLAR_ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = State::TERM;
        } catch (std::exception& ex) {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = State::EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
                << " fiber_id=" << cur->getId()
                << std::endl
                << sylar::BacktraceToString();
        }
        SetThis(GetMainFiber().get());
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }

    void Fiber::YieldToHold() {
        // Fiber::ptr cur = GetThis();
        Fiber* cur = GetThis();
        SYLAR_ASSERT(cur->m_state == State::EXEC);
        cur->m_state = State::READY;
        cur->swapOut();
    }


}