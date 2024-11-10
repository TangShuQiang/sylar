#include "../sylar/log.h"
#include <iostream>

int main(int argc, char** argv) {
    sylar::Logger::ptr logger(new sylar::Logger());
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender()));

    sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 1, 2, 3, time(0)));

    event->getSS() << "hello log";

    logger->log(sylar::LogLevel::DEBUG, event); 
}