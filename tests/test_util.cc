#include "util.h"
#include "log.h"
#include "macro.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert() {
    // SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    SYLAR_ASSERT2(0 == 1, "abcdef xx");
}

int main() {
    test_assert();
}