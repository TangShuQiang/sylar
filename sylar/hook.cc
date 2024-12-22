#include "hook.h"
#include "fiber.h"
#include "iomanager.h"
#include <dlfcn.h>
#include <functional>

#define HOOK_FUN(XX)    \
    XX(sleep)

namespace sylar
{
    static thread_local bool t_hook_enable = false;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

    struct _HookIniter
    {
        _HookIniter() {
            hook_init();
        }

        void hook_init() {
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
            HOOK_FUN(XX);
        #undef XX
        }
    };
    static _HookIniter s_hook_initer;

}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

    unsigned int sleep(unsigned int seconds) {
        if (!sylar::t_hook_enable) {
            return sleep_f(seconds);
        }
        sylar::Fiber::ptr fiber = sylar::Fiber::GetThis()->shared_from_this();
        sylar::IOManager* iom = sylar::IOManager::GetThisIOManager();
        // iom->addTimer(seconds * 1000,
        //     std::bind(
        //         (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int)) & sylar::IOManager::schedule,
        //         iom, fiber, -1),
        //     false);
        iom->addTimer(seconds * 1000, std::bind((void(sylar::Scheduler::*)
            (sylar::Fiber::ptr, int thread))&sylar::IOManager::schedule
            ,iom, fiber, -1));
        sylar::Fiber::YieldToHold();
        return 0;
    }


}