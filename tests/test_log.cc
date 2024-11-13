#include "../sylar/log.h"
#include "../sylar/util.h"
#include <iostream>

int main(int argc, char** argv) {
    sylar::Logger::ptr logger(new sylar::Logger());
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender()));

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 1, sylar::GetThreadId(), sylar::GetFiberId(), time(0)));
    // event->getSS() << "hello log";
    // logger->log(sylar::LogLevel::DEBUG, event); 

    sylar::FileLogAppender::ptr file_appender = std::make_shared<sylar::FileLogAppender>("./log.txt");
    sylar::LogFormatter::ptr fmt = std::make_shared<sylar::LogFormatter>("%d %p %m%n");
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);

    logger->addAppender(file_appender);

    SYLAR_LOG_DEBUG(logger) << "hello";
    SYLAR_LOG_ERROR(logger) << "world";

    SYLAR_LOG_FMT_INFO(logger, "test fmt info %d", 1);

    SYLAR_LOG_FMT_FATAL(logger, "test fmt fatal%s", "nihao");

    sylar::Logger::ptr l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "singlton";


}