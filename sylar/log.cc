#include "log.h"
#include "config.h"
#include <map>
#include <functional>

namespace sylar
{
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

    LogLevel::Level LogLevel::FromString(const std::string& str) {
    #define XX(s, level)    \
        if (str == s) {     \
            return level;    \
        }
        XX("DEBUG", DEBUG);
        XX("INFO", INFO);
        XX("WARN", WARN);
        XX("ERROR", ERROR);
        XX("FATAL", FATAL);

        XX("debug", DEBUG);
        XX("info", INFO);
        XX("warn", WARN);
        XX("error", ERROR);
        XX("fatal", FATAL);
        return UNKNOW;
    #undef XX
    }


    // ------------------------------- Logger
    Logger::Logger(const std::string& name, LogLevel::Level level) : m_name(name), m_level(level) {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S} %t %F [%p] [%c] %f:%l %m%n"));
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            if (!m_appenders.empty()) {
                for (auto& appender : m_appenders) {
                    appender->log(self, level, event);
                }
            } else if (m_root) {
                m_root->log(level, event);
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
        MutexType::Lock lock(m_mutex);
        if (!appender->getFormatter()) {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppends() {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }


    // -------------------------------- LogEvent
    void LogEvent::format(const char* fmt, ...) {
        va_list al;
        va_start(al, fmt);
        char* buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1) {
            m_ss << std::string(buf, len);
            free(buf);
        }
        va_end(al);
    }

    // --------------------------------- LogFormatter
    LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
        init();
    }

    // std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    //     std::stringstream ss;
    //     for (auto& it : m_items) {
    //         it->format(ss, logger, level, event);
    //     }
    //     return ss.str();
    // }

    std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
        for (auto& it : m_items) {
            it->format(ofs, logger, level, event);
        }
        return ofs;
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

    // void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
    //     if (level >= m_level) {
    //         std::cout << m_formatter->format(logger, level, event);
    //     }
    // }

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            MutexType::Lock lock(m_mutex);
            m_formatter->format(std::cout, logger, level, event);
        }
    }

    // --------------------------------- FileLogAppender
    FileLogAppender::FileLogAppender(const std::string& filename) : m_filename(filename) {
        reopen();
    }

    bool FileLogAppender::reopen() {
        MutexType::Lock lock(m_mutex);
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios::app);
        return !!m_filestream;
    }

    // void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
    //     if (level >= m_level) {
    //         m_filestream << m_formatter->format(logger, level, event);
    //     }
    // }

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            MutexType::Lock lock(m_mutex);
            m_formatter->format(m_filestream, logger, level, event);
        }
    }

    // --------------------------------------

    struct LogAppenderDefine
    {
        int type = 0;  // 1: File, 0: Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine& rhs) const {
            return type == rhs.type
                && level == rhs.level
                && formatter == rhs.formatter
                && file == rhs.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine& rhs) const {
            return name == rhs.name
                && level == rhs.level
                && formatter == rhs.formatter
                && appenders == rhs.appenders;
        }

        bool operator<(const LogDefine& rhs) const {
            return name < rhs.name;
        }
    };

    template<>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string& str) {
            YAML::Node node = YAML::Load(str);
            LogDefine logdefine;
            if (!node["name"].IsDefined()) {
                std::cout << "log config yml err: name is null! " << node << std::endl;
                throw std::logic_error("log config yml name is null");
            }
            logdefine.name = node["name"].as<std::string>();
            if (node["level"].IsDefined()) {
                logdefine.level = LogLevel::FromString(node["level"].as<std::string>());
            }
            if (node["formatter"].IsDefined()) {
                logdefine.formatter = node["formatter"].as<std::string>();
            }
            if (node["appenders"].IsDefined()) {
                for (auto appenderNode : node["appenders"]) {
                    if (!appenderNode["type"].IsDefined()) {
                        std::cout << "log config yml err: appender type is null! " << appenderNode << std::endl;
                        continue;
                    }
                    LogAppenderDefine logappenderdefine;
                    std::string type = appenderNode["type"].as<std::string>();
                    if (type == "StdoutLogAppender") {
                        logappenderdefine.type = 0;
                    } else if (type == "FileLogAppender") {
                        logappenderdefine.type = 1;
                        if (!appenderNode["file"].IsDefined()) {
                            std::cout << "log config yml err: fileappender's file is null! " << appenderNode << std::endl;
                            continue;
                        }
                        logappenderdefine.file = appenderNode["file"].as<std::string>();
                    } else {
                        std::cout << "log config yml err: appender type is invalid! " << appenderNode << std::endl;
                        continue;
                    }
                    if (appenderNode["formatter"].IsDefined()) {
                        logappenderdefine.formatter = appenderNode["formatter"].as<std::string>();
                    }
                    if (appenderNode["level"].IsDefined()) {
                        logappenderdefine.level = LogLevel::FromString(appenderNode["level"].as<std::string>());
                    }
                    logdefine.appenders.push_back(logappenderdefine);
                }
            }
            return logdefine;
        }
    };

    template<>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine& obj) {
            YAML::Node node(YAML::NodeType::Map);
            node["name"] = obj.name;
            if (obj.level != LogLevel::UNKNOW) {
                node["level"] = LogLevel::ToString(obj.level);
            }
            if (!obj.formatter.empty()) {
                node["formatter"] = obj.formatter;
            }
            for (auto& appender : obj.appenders) {
                YAML::Node appenderNode(YAML::NodeType::Sequence);
                if (appender.level != LogLevel::UNKNOW) {
                    appenderNode["level"] = LogLevel::ToString(appender.level);
                }
                if (!appender.formatter.empty()) {
                    appenderNode["formatter"] = appender.formatter;
                }
                if (appender.type == 1) {
                    appenderNode["type"] = "FileLogAppender";
                    appenderNode["file"] = appender.file;
                } else if (appender.type == 0) {
                    appenderNode["type"] = "StdoutLogAppender";
                }
                node["appenders"].push_back(appenderNode);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
        sylar::Config::Add("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter
    {
        LogIniter() {
            g_log_defines->addListener([&](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
                std::cout << "on_logger_conf_changed" << std::endl;
                for (auto& val : new_value) {
                    auto it = old_value.find(val);
                    sylar::Logger::ptr logger;
                    if (it == old_value.end()) {                // 自定义Logger配置在初始配置中不存在
                        logger = SYLAR_LOG_NAME(val.name);
                    } else if (*it == val) {                     // 自定义Logger配置与初始配置一样    
                        continue;
                    } else {                                    // 自定义Logger配置与初始配置不一样    
                        logger = SYLAR_LOG_NAME(val.name);
                    }
                    logger->setLevel(val.level);
                    if (!val.formatter.empty()) {
                        logger->setFormatter(val.formatter);
                    }
                    logger->clearAppends();
                    for (auto& ap : val.appenders) {
                        sylar::LogAppender::ptr logAppender;
                        if (ap.type == 1) {
                            logAppender = std::make_shared<FileLogAppender>(ap.file);
                        } else if (ap.type == 0) {
                            logAppender = std::make_shared<StdoutLogAppender>();
                        }
                        logAppender->setLevel(ap.level);
                        if (!ap.formatter.empty()) {
                            logAppender->setFormatter(ap.formatter);
                        }
                        logger->addAppender(logAppender);
                    }
                }
                for (auto& val : old_value) {           // 如果初始配置中的Logger未在自定义配置中出现，则删除
                    auto it = new_value.find(val);
                    if (it == new_value.end()) {
                        auto logger = SYLAR_LOG_NAME(val.name);
                        logger->setLevel(LogLevel::Level::UNKNOW);
                        logger->clearAppends();
                    }
                }
                });
        }
    };

    static LogIniter __log_init;


} // namespace sylar
