#include "util.h"


namespace bobliew {


//采用唯一标识作为id
pid_t GetThreadId() {
    return (pid_t) syscall(SYS_gettid);
}

uint32_t GetFiberId() {
    return 0;
}
}
