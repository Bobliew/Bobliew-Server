#ifndef __BOBLIEW_MACRO_H__
#define __BOBLIEW_MACRO_H__


#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"


#if defined __GNUC__ || defined __llvm__
#define BOBLIEW_LIKELY(x) __builtin_expect(!!(x), 1)
#define BOBLIEW_UNLIKELY(x) __builtin_expect(!!(x),0)
#else
#define BOBLIEW_LIKELY(x)   (x)
#define BOBLIEW_UNLIKELY(x) (x)
#endif


#define BOBLIEW_ASSERT(x) \
    if(!(x)) {\
        BOBLIEW_LOG_ERROR(BOBLIEW_LOG_ROOT()) << "ASSERTION: " #x\
        << "\nbacktrace: \n" \
        << bobliew::BackTracetoString(100, 2, "    ");\
        assert(x);\
    }

#define BOBLIEW_ASSERT2(x,w) \
    if(!(x)) {\
        BOBLIEW_LOG_ERROR(BOBLIEW_LOG_ROOT()) << "ASSERTION: " #x\
        <<"\n" << w \
        << "\nbacktrace: \n" \
        << bobliew::BackTracetoString(100, 2, "    ");\
        assert(x);\
    }

#endif
