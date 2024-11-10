#include "log.h"
#include <map>
#include <functional>

namespace sylar
{

    // ------------------------------- LogEvent
    LogEvent::LogEvent(const char* file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
        : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time) {}

    // ------------------------------- LogLevel
    const char* LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
        #define XX(name)    \
            case LogLevel::Level::name: \
                return #name;   \
                break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
        #undef XX
        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    // ------------------------------- Logger
    Logger::Logger(const std::string& name) : m_name(name), m_level(LogLevel::Level::DEBUG) {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S} %t %F [%p] [%c] %f:%l %m%n"));
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            for (auto& it : m_appenders) {
                it->log(self, level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::Level::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::Level::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::Level::WARN, event);
    }

    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::Level::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::Level::FATAL, event);
    }

    void Logger::addAppender(LogAppender::ptr appender) {
        if (!appender->getFormatter()) {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    // --------------------------------- LogFormatter
    LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for (auto& it : m_items) {
            it->format(ss, logger, level, event);
        }
        return ss.str();
    }

    // %m %d{%Y-%m-%d %H:%M:%S} %%
    void LogFormatter::init() {
        /*
            str,     format,           type
            [          ""                 0
            ]          ""                 0
          %d{%Y-                          0        // error
            z          ""                 1        // 比如'z'这个格式不存在
            m          ""                 1
            d    "%Y-%m-%d %H:%M:%S"      1
        */
        std::vector<std::tuple<std::string, std::string, int>> vec;

        std::string nstr;       // 存储 [、]、%等非格式字符
        for (size_t i = 0; i < m_pattern.size(); ++i) {
            if (m_pattern[i] != '%') {
                nstr.append(1, m_pattern[i]);
                continue;
            }
            // m_pattern[i] == '%'
            // 解析 '%%'
            if ((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                ++i;
                continue;
            }
            // 解析 % 后面的字符
            size_t idx = i + 1;
            std::string str;
            std::string fmt;
            int status = 0;         // 1: {
            int fmt_begin = 0;
            while (idx < m_pattern.size()) {
                if (status == 0) {
                    // 碰到 空格 或者 %，解析当前格式类型
                    if (isalpha(m_pattern[idx]) == false && m_pattern[idx] != '{' && m_pattern[idx] != '}') {
                        str = m_pattern.substr(i + 1, idx - i - 1);
                        break;
                    }
                    if (m_pattern[idx] == '{') {
                        str = m_pattern.substr(i + 1, idx - i - 1);
                        status = 1;
                        fmt_begin = idx + 1;
                        ++idx;
                        continue;
                    }
                } else {
                    if (m_pattern[idx] == '}') {
                        fmt = m_pattern.substr(fmt_begin, idx - fmt_begin);
                        status = 0;
                        ++idx;
                        break;
                    }
                }
                ++idx;
                if (idx == m_pattern.size()) {
                    str = m_pattern.substr(i + 1);
                }
            }
            if (!nstr.empty()) {
                vec.emplace_back(nstr, "", 0);
                nstr.clear();
            }
            if (status == 0) {
                vec.emplace_back(str, fmt, 1);
                i = idx - 1;
            } else {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.emplace_back("<<pattern_error>>", m_pattern.substr(i), 0);
                break;
            }
        }
        if (!nstr.empty()) {
            vec.emplace_back(nstr, "", 0);
        }
        // for (auto i : vec) {
        //     std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        // }
        static std::map<std::string, std::function<FormatItem::ptr(const std::string&)>> s_format_items = {
            #define XX(str, C)  \
                {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));} }

                XX(m, MessageFormatItem),           // 消息
                XX(p, LevelFormatItem),             // 日志级别
                XX(r, ElapseFormatItem),            // 累计毫秒数
                XX(c, NameFormatItem),              // 日志名称
                XX(t, ThreadIdFormatItem),          // 线程id
                XX(n, NewLineFormatItem),           // 换行 
                XX(d, DateTimeFormatItem),          // 时间
                XX(f, FilenameFormatItem),          // 文件名
                XX(l, LineFormatItem),              // 行号
                XX(F, FiberIdIdFormatItem)          // 协程id
                // XX(N, ThreadNameFormatItem)         // 线程名称
            #undef XX
        };
        for (auto& it : vec) {
            if (std::get<2>(it) == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(it))));
            } else {
                auto map_it = s_format_items.find(std::get<0>(it));
                if (map_it == s_format_items.end()) {     // 使用未定义格式
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format: %" + std::get<0>(it) + ">>")));
                    m_error = true;
                } else {
                    m_items.push_back(map_it->second(std::get<1>(it)));
                }
            }
        }
    }

    // --------------------------------- StdoutLogAppender

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    // --------------------------------- FileLogAppender
    FileLogAppender::FileLogAppender(const std::string& filename) : m_filename(filename) {}

    bool FileLogAppender::reopen() {
        if (m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

} // namespace sylar
