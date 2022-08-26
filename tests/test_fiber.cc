#include "../bobliew/bobliew.h"
#include "../bobliew/fiber.h"


bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();


void run_in_fiber() {
    BOBLIEW_LOG_INFO(g_logger) << "run_in_fiber_begin"<<std::endl;
    bobliew::Fiber::YieldToHold();
    BOBLIEW_LOG_INFO(g_logger) << "run_in_end"<<std::endl;
    //如果不返回Hold状态，uc_link = nullptr，直接导致线程结束了。
    bobliew::Fiber::YieldToHold();
}

void test_fiber() {
    BOBLIEW_LOG_INFO(g_logger) << "main_begin -1";
    {
        bobliew::Scheduler sc;
        bobliew::Fiber::Getthis();
        BOBLIEW_LOG_INFO(g_logger)<< "main begin"<<std::endl;
        bobliew::Fiber::ptr fiber(new bobliew::Fiber(run_in_fiber));
        fiber->swapIn();
        BOBLIEW_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        BOBLIEW_LOG_INFO(g_logger) << "main after end1"<<std::endl;
        fiber->swapIn();
    }
    BOBLIEW_LOG_INFO(g_logger) << "main after end2"<<std::endl;

}

int main(int argc, char** argv) {
    bobliew::Thread::SetName("bobliew");
    std::vector<bobliew::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
    thrs.push_back(bobliew::Thread::ptr(new bobliew::Thread(&test_fiber, 
    "name_"+std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}

