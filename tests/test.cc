#include <iostream>
#include "../bobliew/log.h"


int main(int argc, char** argv){
    bobliew::Logger::ptr logger(new bobliew::Logger);
    logger->addAppender(bobliew::LogAppender::ptr(new bobliew::StdoutLogAppender));
    std::string str = "TestName";


    bobliew::LogEvent::ptr event(new bobliew::LogEvent(logger, bobliew::LogLevel::DEBUG, __FILE__,
                                                       __LINE__,  0,  2, 3, time(0), str));
    event->getSS() << "hellow bobliew log";
    logger->log(bobliew::LogLevel::DEBUG, event);
    return 0;
}
