#ifndef __BOBLIEW_IOMANAGER_H__
#define __BOBLIEW_IOMANAGER_H__

#include "bobliew.h"
#include "fiber.h"
#include <sys/epoll.h>

namespace bobliew {

class IOManager : public Scheduler {

public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        //无事件
        NONE = 0x0,
        //EPOLLIN
        READ = 0x1,
        //EPOLLOUT
        WRITE = 0x4,
    };

private:
    //fd的时间
    struct FdContext{
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr; //事件执行所在的scheduler
            Fiber::ptr fiber;     //事件协程
            std::function<void()> cb; //事件回调函数
        };
        //返回当前的event
        EventContext& getContext (Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event); 

        int fd; //事件关联的句柄
        EventContext read; //读事件
        EventContext write; //写事件
        Event events = NONE; //已经注册的事件
        MutexType mutex; 
    };
public:
    IOManager(size_t threads = 1, bool use_caller = true, 
              const std::string& name = "");
    ~IOManager();

    // 1 success, 0 retry, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event); //取消，并触发特定事件

    bool cancelAll(int fd);
    static IOManager* GetThis();


protected:
    //返回是否可以停止
    virtual bool stopping() override;

    //通知协程调度器有任务生成
    virtual void tickle() override;

    //协程无任务可调度的时候执行idle线程
    virtual void idle() override;

    void contextResize(size_t size);

private:
    //epoll文件句柄
    int m_epfd = 0;
    //异步IO

    //相当于 pipefd[2]; pipefd[0] refers read end of the pipe
    //pipefd[1] refers to write end of the pipe
    int m_tickleFds[2];

    //等待执行的事件数量。
    std::atomic<size_t> m_pendingEventCount = {0};
    
    RWMutexType m_mutex;
    //储存上下文的数组
    std::vector<FdContext*> m_fdContexts;


};


}


#endif
