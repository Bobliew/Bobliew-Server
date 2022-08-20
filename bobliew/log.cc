#include "log.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>
#include <functional>
#include <sys/types.h>
#include <vector>
#include <string>
#include <time.h>

//主要逻辑level>m_level.
//DEBUG<INFO<WARN<ERROR<FATAL 
//正式部署后,appder会根据输入的level来决定打印的内容.




namespace bobliew{


//利用宏将枚举量level转化为字符串
const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {
#define XX(name)\
    case LogLevel::name:\
        return #name;\
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
#define XX(level, v)\
    if(str == #v) {\
        return LogLevel::level;\
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :m_event(e) {
}

LogEventWrap::~LogEventWrap() {
    m_event->getLogger()->log(m_event->getLevel(), m_event);
}


std::stringstream& LogEventWrap::getSS() {
    return m_event->getSS();
}

//各种FormatItem的子类
//消息格式器
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};
// level格式器,这里的命名规则是将   (level)转化为   (string),目的是从日志器中取出相应想知道的信息.
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadName();
    } 
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        //将时间格式转化为字符串
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    } 
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};

//LogEvent
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
             const char* file, int32_t line, uint32_t elapse, 
             uint32_t thread_id, uint32_t fiber_id, uint64_t time, 
             const std::string& thread_name)
        :m_file(file)
        ,m_line(line)
        ,m_elapse(elapse)
        ,m_threadId(thread_id)
        ,m_fiberId(fiber_id)
        ,m_time(time)
        ,m_threadName(thread_name)
        ,m_logger(logger)
        ,m_level(level) {
}


// Logger的实现
Logger::Logger(const std::string& name) 
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    if(!appender ->getFormatter()) {
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it){
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}


void Logger::log(LogLevel::Level level, const LogEvent::ptr event) {
    if(level >= m_level) {
        auto self = shared_from_this();
        //后续需要加上锁
        if(!m_appenders.empty()) {
            for(auto& i : m_appenders) {
            //这里的i是LogAppender类型,所以使用的log函数是m_appenders下的.
                i->log(self, level, event);
            }
        } else if(m_root) {
        //是否可以利用shared from this 来替代m_root
            m_root->log(level, event);
        }
    }
}


void Logger::debug(LogEvent::ptr event) {
}
void Logger::info(LogEvent::ptr event) {
}
void Logger::warn(LogEvent::ptr event) {
}
void Logger::error(LogEvent::ptr event) {
}
void Logger::fatal(LogEvent::ptr event) {
}

//FileLogAppender 的实现
FileLogAppender::FileLogAppender(const std::string& filename)
:m_filename(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    //后续在添加线程池后需要上锁
    if(level >= m_level) {
        uint64_t now = event->getTime();
        if(now >= (m_lastTime + 3)) {
            reopen();
            m_lastTime = now;
        }
        //后续上锁
        if(!m_formatter->format(m_filestream, logger, level, event)){
            std::cout << "error" << std::endl;
        }
    }
}

bool FileLogAppender::reopen() {
    if(m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}



//StdoutLogAppender 的实现


void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        //还没加入线程池模块 后续应该在此处上锁 
        m_formatter->format(std::cout, logger, level, event);
    }
}

//这是基于Log4j的日志框架,通过不同的appender,会将同一条日志输出到不同的目的地.
//例如初步写的file和stdout.下面简要记录一下会实现的功能.
//需要注意的是因为这相当利用C++进行模仿,因此可能会与实际的Log4j存在差异.
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
//上面的是传入的参数pattern的默认格式

//LogFormatter
LogFormatter::LogFormatter(const std::string& pattern) 
:m_pattern(pattern) {
    init();
}

//format函数的作用在于根据输入的信息返回格式化日志文本
    /**
     * @brief 返回格式化日志文本
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
//潜在错误 Logformatter里的format函数定义不同,而子类的相同但却在private中
//Debug, formatitem这个子类应该放在public中,不然无法被外部调用.
//返回格式化好的日志事件输出,同时将格式化好的结果传到流,注意,stringsteam的基类就是ostream
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    //m_items是 vector<FormatItem::ptr> m_items 
    for(auto& i : m_items) {
        //传入的是一个引用,因此最后ofs的值是已经被修改的
        i->format(ofs, logger, level, event);
    }
    return ofs;
}

//m_pattern是log_formatter里面的格式名称 但这些名称一般的形式为:
// %xxx %xxx{xxx} %%, 为了用于日志输出,需要使用init函数对这些进行识别.
void LogFormatter::init() {
    //tuple内的内容大概是 string formatter type 
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr; //nstr(nick name of not string?)收录格式
    for(size_t i = 0; i < m_pattern.size(); ++i){
        //收集没有%前缀的内容.直接添加进行下一次循环
        if(m_pattern[i] != '%') {
            nstr.append(1,m_pattern[i]);
            continue;
        }
        //下面的情况是m_pattern[i] == %,如果出现了%说明可能后面存在一种特定的格式.
        if((i+1) < m_pattern.size()) {
            if(m_pattern[i+1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
        size_t n = i + 1;
        int fmt_status = 0;//解析格式的标志,中括号内外的解析方式有差异. 
        size_t fmt_begin =0;//用于后续substr的记录.
        
        std::string str;//str负责处理存在中括号前的内容(名称),如果整个数组都不存在中括号,
        std::string fmt;//fmt用于记录中括号内的内容(解析内容).
        //中括号里面的是特定格式的内容内容,这样的操作对于正常的m_pattern来说,已经结束了
        //接下来只需要根据fmt_status和 nstr来确定是否存在不符合命名规则的日志就可以完成识别了.
        while(n < m_pattern.size()) {
            //如果此时不是在中括号中,同时也不位于中括号上,以及当前元素不是字母
            //(默认为遇到%),这说明从i+1到当前块没有中括号的情况,因此可以直接添加元素到str中,然后结束循环.
            //
            if(!fmt_status && (!isalpha(m_pattern[n]) && 
               m_pattern[n]!='{' && m_pattern[n] != '}')) {
                str = m_pattern.substr(i+1, n-i-1);
                break;
            }
            //当没处于中括号中时
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i+1, n-i-1);
                    fmt_status=1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            //当处于中括号中时
            }else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin+1, n-fmt_begin-1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i+1);
                }
            }
        }
       //根据fmt_status和nstr来确定是否存在未处理的内容.
        if(fmt_status ==0) {
            if(!nstr.empty()) {
                //nstr储存括号外的内容
                //type = 0 的其实都相当于不处理的内容.
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            //str储存括号内的内容
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n-1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }
    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    //通过这个map将不同的字母和命令联系起来
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
    #define XX(str, C)\
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}
        //上面的宏用于
        //后续将各种FormatItem进行完善后,补全宏
        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
        XX(N, ThreadNameFormatItem),
    #undef XX
    };
    
    for(auto& i : vec) {
    //如果 tupls <2> ==0,这部分内容其实不需要进行处理,因为都是nstr(无关信息)和fmt_status==1(一部分错误格式)时候的场景.
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            //如果status==1,说明是,从s_format_items中提取对应的类型进行输出.
            auto it = s_format_items.find(std::get<0>(i));
            //.find()返回的应该是一个迭代器,因此如果迭代到达了终点,说明该格式不存在对应的解析方式,需要对错误进行记录.
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        } 
        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_loggers[m_root->m_name] = m_root;
    init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {

    auto it = m_loggers.find(name);
    if(it != m_loggers.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

void LoggerManager::init() {

}
}
