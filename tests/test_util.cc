#include "../bobliew/bobliew.h"
#include <assert.h>

bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();



void test_assert() {
    BOBLIEW_LOG_INFO(g_logger) << bobliew::BackTracetoString(10, 1, " ");
    BOBLIEW_ASSERT2(0==1, "abcedf xx");
}


int main(int argc, char** argv) {
    test_assert();
    return 0;
};
