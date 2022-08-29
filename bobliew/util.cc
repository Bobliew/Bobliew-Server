#include "util.h"
#include <execinfo.h>
#include "fiber.h"
#include "log.h"


namespace bobliew {
bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

//采用唯一标识作为id
pid_t GetThreadId() {
    return (pid_t) syscall(SYS_gettid);
}

uint64_t GetFiberId() {
    return bobliew::Fiber::GetFiberId();
}


void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    //在栈上分配各个函数对象的指针，防止array的数据都储存在栈上，
    //导致栈空间不足。现在有多少个（size）就分配多少空间给该数组。
    void ** array = (void**)malloc((sizeof(void*) * size));
    //backtrace（）,返回调用程序的回溯，从相应的栈帧（stack frame）返回
    //类似gdb中的bt调试。
    size_t s= ::backtrace(array,size);
    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        BOBLIEW_LOG_ERROR(g_logger) << "backtrace_symbols error";
        //free(array); 这里不需要free array，因为根据逻辑，如果string为NULL
        //说明数组也是空数组，所以没必要free
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

std::string BackTracetoString(int size, int skip, const std::string prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix<<bt[i] << std::endl;
    }
    return ss.str();
}


uint64_t GetCurrentMS() {
    timeval tv;
    //gettimeofday 取到当前的时间
    gettimeofday(&tv, NULL);
    //tv_sec是秒，tv_usec是
    return tv.tv_sec * 1000ul + tv.tv_usec/1000;
}

uint64_t GetCurrentUS() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}


}
