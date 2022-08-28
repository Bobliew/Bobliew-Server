#include "../bobliew/bobliew.h"

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

void test_fiber(){
    static int s_count = 5;
    BOBLIEW_LOG_INFO(g_logger) << "scount= "<< s_count;

    sleep(1);

    if(--s_count >= 0) {
        bobliew::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char** argv) {
    bobliew::Scheduler sc(3, false, "bobliew");
    //sc.schedule(&test_fiber);
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    return 0;
}
