#include "../bobliew/bobliew.h"

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

void test_fiber(){
    BOBLIEW_LOG_INFO(g_logger) << "test in 1";
}

int main(int argc, char** argv) {
    bobliew::Scheduler sc(2, true, "bobliew");
    sc.start();
    sc.schedule(&test_fiber);
    sc.stop();
    return 0;
}
