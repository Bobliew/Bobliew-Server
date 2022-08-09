#ifndef __BOBLIEW_LOG__
#define __BOBLIEW_LOG__


#include <cctype>
#include <fstream>
#include <string>
#include <memory>
#include <stdint.h>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>


namespace bobliew{

class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent();
private:
    const char* m_file = nullptr;
    int32_t m_line = 0;
    uint32_t m_elapse = 0;
    uint32_t m_threadId = 0;
    uint32_t m_fiberId = 0;
    uint64_t m_time = 0;
    std::string m_content;
};


class LogLevel{
public:
    enum Level{
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };
};

class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    std::string format(LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem();
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
    };
    //初始化,解析日志模板
    void init();
    //返回是否存在错误
    bool isError() const { return m_error;}
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error =false;
}; 


//日志输出地
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender(){}

    virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val) { m_formatter = val;}
    LogFormatter::ptr getFormatter() const { return m_formatter;}
protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger {
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name="root");
    void log(LogLevel::Level level, const LogEvent::ptr event);

    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const{return m_level;}
    void setLevel(LogLevel::Level val) {m_level = val;}
private:
    std::string m_name;                                              //日志名称
    LogLevel::Level m_level;                                         //日志级别
    LogAppender::ptr m_ptr; 
    std::list<LogAppender::ptr> m_appenders;
};

class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(LogLevel::Level level, LogEvent::ptr event) override;
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(LogLevel::Level level, LogEvent::ptr event) override;

    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
};



}

#endif
