#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <assert.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define SYLAR_LIKELY(x)     __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLIKELY(x)   __builtin_expect(!!(x), 0)
#else 
#define SYLAR_LIKELY(x)     (x)
#define SYLAR_UNLIKELY(x)   (x)
#endif

#define SYLAR_ASSERT(x)                                         \
    if (SYLAR_UNLIKELY(!(x))) {                                 \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x   \
            << "\nbacktrack:\n"                                 \
            << sylar::BacktraceToString(100, 2, "     ");       \
        assert(x);                                              \
    }

#define SYLAR_ASSERT2(x, w)                                     \
    if (SYLAR_UNLIKELY(!(x))) {                                 \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x   \
            << "\n" << w                                        \
            << "\nbacktrack:\n"                                 \
            << sylar::BacktraceToString(100, 2, "     ");       \
        assert(x);                                              \
    }

#endif