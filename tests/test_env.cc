#include<iostream>

#include "env.h"

int main(int argc, char** argv) {
    std::cout << "argc=" << argc << std::endl;

    sylar::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    sylar::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    sylar::EnvMgr::GetInstance()->addHelp("p", "print help");

    if (!sylar::EnvMgr::GetInstance()->init(argc, argv)) {
        sylar::EnvMgr::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << sylar::EnvMgr::GetInstance()->getExe() << std::endl;
    std::cout << "cwd=" << sylar::EnvMgr::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << sylar::EnvMgr::GetInstance()->getEnv("PATH", "xx") << std::endl;
    std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;
    std::cout << "set env " << sylar::EnvMgr::GetInstance()->setEnv("TEST", "yy") << std::endl;
    std::cout << "test=" << sylar::EnvMgr::GetInstance()->getEnv("TEST", "") << std::endl;

    if (sylar::EnvMgr::GetInstance()->has("p")) {
        sylar::EnvMgr::GetInstance()->printHelp();
    }
}