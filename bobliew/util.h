#ifndef __BOBLIEW_UTIL_H__
#define __BOBLIEW_UTIL_H__

#include <iostream>
#include <sys/types.h>
#include <cxxabi.h>

namespace bobliew {


template<class T>
const char* TypetoName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


}




#endif
