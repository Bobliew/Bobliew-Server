#ifndef __BOBLIEW_FD_MANAGER__
#define __BOBLIEW_FD_MANAGER__

#include <memory>
#include "thread.h"
#include <vector>
#include "singleton.h"


namespace bobliew {

//文件句柄的上下文类
//管理文件句柄类型，是否发生阻塞，是否被关闭，读/写是否超时
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInit() const { return m_isInit;}
    bool isSocket() const { return m_isSocket;}
    bool isClosed() const { return m_isClosed;}

    void setUserNonblock(bool v) { m_userNonblock = v;}
    bool getUserNonblock() const { return m_userNonblock;}
    
    void setSysNonblock(bool v) { m_sysNonblock = v;}
    bool getSysNonblock() const { return m_sysNonblock;}

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);
private:
    bool m_isInit: 1;
    bool m_isSocket: 1;
    bool m_sysNonblock: 1;
    bool m_isClosed: 1;
    bool m_userNonblock: 1;
    int m_fd;

    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
};

class FdManager {
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<FdManager> ptr;
    FdManager();
    //get一个特定fd句柄，如果不存在就会创建，类似getthis。
    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}

#endif
