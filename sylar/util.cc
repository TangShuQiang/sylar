#include "util.h"
#include "log.h"
#include "fiber.h"
#include <execinfo.h>
#include <cxxabi.h>
#include <sys/time.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    pid_t GetThreadId() {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId() {
        return Fiber::GetFiberId();
    }

    static std::string demangle(const char* str) {
        size_t size = 0;
        int status = 0;
        std::string rt;
        rt.resize(256);
        if(1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
            char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
            if (v) {
                std::string result(v);
                free(v);
                return result;
            }
        }
        if (1 == sscanf(str, "%255s", &rt[0])) {
            return rt;
        }
        return str;
    }

    uint64_t GetCurrentMS() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t)tv.tv_sec * 1000  + (tv.tv_usec) / 1000;
    }

    void Backtrack(std::vector<std::string>& bt, int size, int skip) {
        void** array = (void**)malloc(sizeof(void*) * size);
        size_t s = ::backtrace(array, size);
        char** strings = backtrace_symbols(array, s);
        if (strings == NULL) {
            SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
            return;
        }
        for (size_t i = skip; i < s; ++i) {
            bt.push_back(demangle(strings[i]));
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string& prefix) {
        std::vector<std::string> bt;
        Backtrack(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); ++i) {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    std::string Time2Str(time_t ts, const std::string& format) {
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }

} 
 