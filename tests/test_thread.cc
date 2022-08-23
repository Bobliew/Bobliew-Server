#include "../bobliew/thread.h"
#include "../bobliew/util.h"
#include "../bobliew/log.h"
#include "../bobliew/config.h"
#include <unistd.h>


bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();
//bobliew::RWMutex s_mutex;
bobliew::Mutex s_mutex;

int count = 0;

void fun1() {
    BOBLIEW_LOG_INFO(g_logger) << " name: " << bobliew::Thread::GetName()
    << " this.name: " << bobliew::Thread::GetThis()->getName()
    << " id: " << bobliew::GetThreadId() << " this.id: " << bobliew::Thread::GetThis()->getId();
    
    for(long long i = 0; i < 100000; ++i) {
        bobliew::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
}

int main (int argc, char** argv) {
    BOBLIEW_LOG_INFO(g_logger)<< " thread test begin ";
    std::vector<bobliew::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        bobliew::Thread::ptr thr(new bobliew::Thread(&fun1, " name_"+std::to_string(i)));
        thrs.push_back(thr);
    }
    for(int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }
    BOBLIEW_LOG_INFO(g_logger) << " thread test end ";
    BOBLIEW_LOG_INFO(g_logger) << " count = " << count;
    return 0;
}

