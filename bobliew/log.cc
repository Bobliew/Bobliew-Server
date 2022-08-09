#include "log.h"

namespace bobliew{

Logger::Logger(const std::string& name="root");
void log(LogLevel::Level level, const LogEvent::ptr event);

void debug(LogEvent::ptr event);
void info(LogEvent::ptr event);
void warn(LogEvent::ptr event);
void error(LogEvent::ptr event);
void fatal(LogEvent::ptr event);


void addAppender(LogAppender::ptr appender);
void delAppender(LogAppender::ptr appender);

}
