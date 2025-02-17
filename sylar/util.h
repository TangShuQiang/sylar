#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>
#include <vector>

namespace sylar
{
    pid_t GetThreadId();
    uint32_t GetFiberId();
    uint64_t GetCurrentMS();

    void Backtrack(std::vector<std::string>& bt, int size = 64, int skip = 1);
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

    std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M%S");

    class FSUtil
    {
    public:
        static void ListAllFile(std::vector<std::string>& files
            , const std::string& path
            , const std::string& subfix);
    };
}

#endif