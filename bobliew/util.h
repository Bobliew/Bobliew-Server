#ifndef __BOBLIEW_UTIL_H__
#define __BOBLIEW_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <sys/types.h>
#include <cxxabi.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <vector>
#include <string>

namespace bobliew {



pid_t GetThreadId();
uint64_t GetFiberId();


void Backtrace(std::vector<std::string>& bt, int size, int skip);

std::string BackTracetoString(int size, int skip, const std::string prefix = " ");




template<class T>
const char* TypetoName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


}




#endif
