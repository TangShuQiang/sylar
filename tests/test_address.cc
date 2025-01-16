#include "address.h"
#include "log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();


void test() {
    std::vector<sylar::Address::ptr> vec;
    SYLAR_LOG_INFO(g_logger) << "begin";

    bool v = sylar::Address::Lookup(vec, "www.baidu.com:http", AF_INET6);
    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "Lookup fail";
        return;
    }
    for (size_t i = 0; i < vec.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << vec[i]->toString();
    }
    SYLAR_LOG_INFO(g_logger) << "end";

    auto addr = sylar::Address::LookupAnyIPAddress("localhost");
    if (addr) {
        SYLAR_LOG_INFO(g_logger) << *addr;
    } else {
        SYLAR_LOG_ERROR(g_logger) << "error";
    }

}

void test_iface() {
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t>> retMap;
    bool v = sylar::Address::GetInterfaceAddress(retMap, 0);
    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
        return ;
    }
    for (auto& it : retMap) {
        SYLAR_LOG_INFO(g_logger) << it.first << " - " << it.second.first->toString() << " - " << it.second.second;
    }
}

void test_ipv4() {
    auto addr = sylar::IPAddress::Create("www.baidu.com");
    if (addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    }
}

int main() {
    test();
    test_iface();
    test_ipv4();
}