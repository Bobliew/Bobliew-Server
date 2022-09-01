#include "fd_manager.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "hook.h"
namespace bobliew {

FdCtx::FdCtx(int fd) 
   :m_isInit(false),
    m_isSocket(false),
    m_sysNonblock(false),
    m_isClosed(false),
    m_userNonblock(false),
    m_fd(fd),
    m_recvTimeout(-1),
    m_sendTimeout(-1) {
    init();
}

FdCtx::~FdCtx() {

}


//句柄判断的作用在于，如果是socket后序可以使用我们准备好的hook函数，
//如果不是socket就还是使用原来的函数就可以了。
//是不是关闭-是不是socket-设置非阻塞
bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    // -1为根据句柄查询失败
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }
    if(m_isSocket) {
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        //如果不是非阻塞的文件类型，则通过fcntl_f添加回去。
        if(!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }
    m_userNonblock = false;
    m_isClosed = false;
    return m_isInit;
}

//设置句柄的超时
void FdCtx::setTimeout(int type,uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}

FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd==-1) {
        return nullptr;
    }
    RWMutexType::ReadLock lock(m_mutex);
    if((int) m_datas.size()<=fd) {
        if(auto_create == false) {
            return nullptr;
        }
    } else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd*1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size()<=fd) {
        return;
    }
    m_datas[fd].reset();
}

}
