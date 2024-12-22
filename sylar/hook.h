#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <unistd.h>

namespace sylar
{
    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C" {
    using sleep_fun = unsigned int(*)(unsigned int);
    extern sleep_fun sleep_f;

    using usleep_fun = int(*)(useconds_t);
    extern usleep_fun usleep_f;

    using fcntl_fun = int(*)(int, int cmd, ...);
    extern fcntl_fun fcntl_f;

}

#endif