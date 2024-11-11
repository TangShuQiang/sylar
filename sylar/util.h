#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>

namespace sylar
{
    pid_t GetThreadId();

    uint32_t GetFiberId();
}

#endif