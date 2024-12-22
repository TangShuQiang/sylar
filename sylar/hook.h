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

}

#endif