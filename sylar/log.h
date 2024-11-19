#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdarg.h>
#include <map>
#include "util.h"
#include "singleton.h"

#define SYLAR_LOG_LEVEL(logger, level) \
    if (level >= logger->getLevel())    \
        sylar::LogEventWrap(sylar::LogEvent::ptr(std::make_shared<sylar::LogEvent>(logger, level, __FILE__, \
            __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0)))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)    \
    if (level >= logger->getLevel()) \
        sylar::LogEventWrap(std::make_shared<sylar::LogEvent>(logger, level, __FILE__, \
            __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), time(0))).getEvent()->format(fmt, ##__VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt,  ##__VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, ##__VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, ##__VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar
{
    class Logger;
    class LoggerManager;

    //  日志级别
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static const char* ToString(LogLevel::Level level);
        static LogLevel::Level FromString(const std::string& str);
    };

    // 日志事件
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;

        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
            : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_logger(logger), m_level(level) {}

        const char* getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        int32_t getElapse() const { return m_elapse; }
        int32_t getThreadId() const { return m_threadId; }
        int32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string getContent() const { return m_ss.str(); }
        std::stringstream& getSS() { return m_ss; }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }

        void format(const char* fmt, ...);

    private:
        const char* m_file = nullptr;       // 文件名
        int32_t m_line = 0;                 // 行号
        uint32_t m_elapse = 0;              // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;            // 线程id
        uint32_t m_fiberId = 0;             // 协程id
        uint64_t m_time = 0;                // 时间戳
        std::stringstream m_ss;
        std::shared_ptr<Logger> m_logger;
        LogLevel::Level m_level;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        using ptr = std::shared_ptr<LogFormatter>;

        LogFormatter(const std::string& pattern);
        // std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
        void init();
        bool isError() const { return m_error; }

    public:
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;
            virtual ~FormatItem() {}
            virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    // 日志输出地
    class LogAppender
    {
    public:
        using ptr = std::shared_ptr<LogAppender>;

        virtual ~LogAppender() {};
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }

        LogFormatter::ptr getFormatter() const { return m_formatter; }
        void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }
        void setFormatter(const std::string& str) { m_formatter = std::make_shared<LogFormatter>(str); }

    protected:
        LogLevel::Level m_level = LogLevel::Level::UNKNOW;
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        friend class LoggerManager;
    public:
        using ptr = std::shared_ptr<Logger>;

        Logger(const std::string& name = "root", LogLevel::Level level = LogLevel::UNKNOW);
        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void clearAppends();

        const std::string& getName() const { return m_name; }
        void setLevel(LogLevel::Level level) { m_level = level; }
        LogLevel::Level getLevel() const { return m_level; }

        void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }
        void setFormatter(const std::string& str) { m_formatter = std::make_shared<LogFormatter>(str); }

    private:
        std::string m_name;                         // 日志名称
        LogLevel::Level m_level;                    // 日志级别
        std::list<LogAppender::ptr> m_appenders;    // Appender集合
        LogFormatter::ptr m_formatter;              // 默认日志格式（当添加的appender未设置formatter时使用）
        Logger::ptr m_root;                         // 主日志器
    };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<StdoutLogAppender>;

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    };

    // 输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        using ptr = std::shared_ptr<FileLogAppender>;

        FileLogAppender(const std::string& filename);

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    // 日志事件包装器
    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr event) : m_event(event) {}
        ~LogEventWrap() {
            m_event->getLogger()->log(m_event->getLevel(), m_event);
        }
        LogEvent::ptr getEvent() const { return m_event; }
        std::stringstream& getSS() { return m_event->getSS(); }
    private:
        LogEvent::ptr m_event;
    };

    class LoggerManager
    {
    public:
        LoggerManager() {
            m_root.reset(new Logger());
            m_root->addAppender(std::make_shared<StdoutLogAppender>());
            m_loggers[m_root->getName()] = m_root;
        }

        Logger::ptr getLogger(const std::string& name) {
            auto it = m_loggers.find(name);
            if (it != m_loggers.end()) {
                return it->second;
            }
            Logger::ptr logger = std::make_shared<Logger>(name);
            logger->m_root = m_root;
            m_loggers[name] = logger;
            return logger;
            // if (m_loggers.find(name) == m_loggers.end()) {
            //     return m_root;
            // }
            // return m_loggers[name];
        }

        Logger::ptr getRoot() const { return m_root; }

    private:
        Logger::ptr m_root;
        std::map<std::string, Logger::ptr> m_loggers;
    };

    using LoggerMgr = sylar::Singleton<LoggerManager>;

} // namespace sylar

namespace sylar
{
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << logger->getName();
        }
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string& str = "") {}
        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdIdFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S") : m_format(format) {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char timeBuff[64];
            strftime(timeBuff, sizeof(timeBuff), m_format.c_str(), &tm);
            os << timeBuff;
        }
    private:
        std::string m_format;
    };

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string& str = "") {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string& str = "") : m_string(str) {}

        void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        }
    private:
        std::string m_string;
    };
}

#endif