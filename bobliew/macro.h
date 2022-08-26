#ifndef __BOBLIEW_MACRO_H__
#define __BOBLIEW_MACRO_H__


#include <string.h>
#include <assert.h>
#include "util.h"



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
