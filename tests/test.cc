#include <iostream>
#include "../bobliew/log.h"


int main(int argc, char** argv){
    bobliew::Logger::ptr logger(new bobliew::Logger);
    logger->addAppender(bobliew::LogAppender::ptr(new bobliew::StdoutLogAppender));
    
    bobliew::FileLogAppender::ptr file_appender(new bobliew::FileLogAppender("./log.txt"));
    std::string str = "TestName";

    
    bobliew::LogFormatter::ptr fmt(new bobliew::LogFormatter("%d%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(bobliew::LogLevel::DEBUG);

    logger -> addAppender(file_appender);
    //bobliew::LogEvent::ptr event(new bobliew::LogEvent(logger, bobliew::LogLevel::DEBUG, __FILE__,
    //                                              __LINE__,  0,  2, 3, time(0), str));
    //event->getSS() << "hellow bobliew log";
    BOBLIEW_LOG_DEBUG(logger) << "bobliew bobliew";
    BOBLIEW_LOG_ERROR(logger) << "ERROR logger";
    //logger->log(bobliew::LogLevel::DEBUG, event);
    //

    auto l = bobliew::LoggerMgr::GetInstance()->getLogger("xx");
    BOBLIEW_LOG_INFO(l) << "xxx";
    return 0;
}
