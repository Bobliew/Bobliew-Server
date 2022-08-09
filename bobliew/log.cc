#include "log.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>
#include <functional>
#include <sys/types.h>
#include <vector>
#include <string>

namespace bobliew{



// Logger的实现
Logger::Logger(const std::string& name) {

}

void Logger::addAppender(LogAppender::ptr appender) {
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
        for(auto& i : m_appenders) {
            i->log(level, event);
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
}

void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        m_filestream << m_formatter->format(event);
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


void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        std::cout << m_formatter->format(event);
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
std::string LogFormatter::format(LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss, event);
    }
    return ss.str();
}
//m_pattern是log_formatter里面的格式名称 但这些名称一般的形式为:
// %xxx %xxx{xxx} %%, 为了用于日志输出,需要使用init函数对这些进行识别.
void LogFormatter::init() {
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr; //nstr收录在%和下一个%中的全部内容
    for(size_t i = 0; i < m_pattern.size(); ++i){
        //如果不是%,说明是有用信息.直接添加进行下一次循环
        if(m_pattern[i] != '%') {
            nstr.append(1,m_pattern[i]);
            continue;
        }
        if((i+1) < m_pattern.size()) {
            if(m_pattern[i+1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
        size_t n = i + 1;
        int fmt_status = 0;//解析格式-反映是否进入了中括号中,中括号内外的解析方式有差异.
        size_t fmt_begin =0;
        
        std::string str;//str负责处理存在中括号的内容,如果整个数组都不存在中括号,
        std::string fmt;
        //中括号里面的是特定格式的内容内容,这样的操作对于正常的m_pattern来说,已经结束了
        //接下来只需要根据fmt_status和 nstr来确定是否存在不符合命名规则的日志就可以完成识别了.
        while(n < m_pattern.size()) {
            //如果此时不是在中括号中,同时也不位于中括号上,以及当前元素不是字母
            //(默认为遇到%),这说明从i+1到当前块没有中括号的情况,因此可以直接添加元素到str中,然后结束循环.
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
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, std::string(), 1));
            i = n-1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
        }



    }

}



}
