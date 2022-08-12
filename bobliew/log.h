#ifndef __BOBLIEW_LOG__
#define __BOBLIEW_LOG__


#include <bits/stdint-uintn.h>
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
class Logger;
class LoggerManager;

class LogLevel{
public:
    enum Level{
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5,
    };
    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};


class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char* file, int32_t line, uint32_t elapse, 
             uint32_t thread_id, uint32_t fiber_id, uint64_t time, 
             const std::string& thread_name);


    //成员函数:getFile getElapse(耗时) getThreadId getFiberId getTime
    //getThreadName getLogger getLevel getSS format
    const char* getFile() const { return m_file;}
    int32_t getLine() const { return m_line;}

    uint32_t getElapse() const { return m_elapse;}
    uint32_t getThreadId() const { return m_threadId;}
    uint32_t getFiberId() const { return m_fiberId;}
    uint64_t getTime() const { return m_time;}
    
    const std::string&getThreadName() const { return m_threadName;}
    std::string getContent() const { return m_ss.str();}
    std::shared_ptr<Logger> getLogger() const { return m_logger;}
    LogLevel::Level getLevel() const { return m_level;}
    std::stringstream& getSS() { return m_ss;}

    void format(const char* fmt, va_list al);

private:
    const char* m_file = nullptr;
    int32_t m_line = 0;
    uint32_t m_elapse = 0;
    uint32_t m_threadId = 0;
    uint32_t m_fiberId = 0;
    uint64_t m_time = 0;
    std::string m_threadName;//线程名称
    std::stringstream m_ss;//日志内容流
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};


class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {};
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;
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
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender(){}

    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val) { m_formatter = val;}
    LogFormatter::ptr getFormatter() const { return m_formatter;}
protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger>{
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
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;                                              //主日志器
};



class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    bool reopen();
private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};


}

#endif
