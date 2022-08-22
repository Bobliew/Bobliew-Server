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


namespace bobliew {



pid_t GetThreadId();
uint32_t GetFiberId();


template<class T>
const char* TypetoName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


}




#endif
