#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <bits/stdint-uintn.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>


namespace bobliew {

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
:Scheduler(threads, use_caller, name) {

    //size是随便大于零的参数，因为在大于linux2.6.8的内核中
    //epoll instance会动态分配，但为了向后兼容，还是需要一个大于零的int.
    // epoll_create::创建了一个新的epoll instance
    //
    m_epfd = epoll_create(5000);
    BOBLIEW_ASSERT(m_epfd > 0);

    //pipe函数创建一个管道，用于进程间通信的单向数据通道。
    //pipefd[0]是读取端， pipefd[1]是写入端
    int rt = pipe(m_tickleFds);
    BOBLIEW_ASSERT(!rt);
    

    epoll_event event;
    //填充指定字节的内容进入event
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET; //avaliable for read and write.
    //m_tickleFds相当于废话机，用来tickle一下epoll interset list. 
    event.data.fd = m_tickleFds[0];
    
    //F_SETFL将文件的flag设置为异步非阻塞
    // 打开文件描述符 m_tickleFds[0]
    // rt仅用于检测异常,成功后都会返回0
    // fcntl改变pipe的属性
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    BOBLIEW_ASSERT(!rt);
    //add/mod/remove entries interset list
    //add this event with m_fickleFds[0] into m_epfd
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    BOBLIEW_ASSERT(!rt);
    //初始化context
    contextResize(32);        
    //调用基类Scheduler的start（）方法，调用好了后就自动启动
    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}


// 1 success, 0 retry, -1 error
int IOManager::addEvent(int fd, IOManager::Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    //使用前需要判断句柄是否超出了范围，需要加锁
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else{
        lock.unlock();
        //潜在问题，在加锁和解锁中间可能存在新的数据修改，如果fd在此时被影响了，
        //可能会导致m_fdCOntexts所占用的空间大小变很大。
        //解决方案是再嵌套一层判断，确保此时的fd是不足的。这样可以在开始避免使用
        //写锁，但在使用写锁后fd的size不会被其他线程同时增大。
        RWMutexType::WriteLock lock2(m_mutex);
        if((int) m_fdContexts.size()>fd) {
            fd_ctx = m_fdContexts[fd];
            lock2.unlock();
        }else {
            //可以确保容纳空间比fd大，所以一定可以放下。
            contextResize(fd*1.5);
            fd_ctx = m_fdContexts[fd];
        }
        lock2.unlock();
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //如果加入一个已经存在的事件，说明可能多个线程可能在处理同一个fd，说明出错。
    if(fd_ctx->events & event) {
        BOBLIEW_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
            << "event = " << event
            << "fd_ctx.event= " << fd_ctx->events;
        BOBLIEW_ASSERT(!(fd_ctx->events & event));
    }

    //查看fd_ctx是否已经拥有事件
    int op = fd_ctx->events ?  EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    //我们采用ET模式，epoll中通过|运算符将不同的事件结合
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    
    //epoll_ctl  句柄，操作，文件描述符，事件
    //意思是
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    //成功操作rt会返回0.
    if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<","
            << op << ", "<< fd << ", " << epevent.events << "):"
            << rt << "("<< errno<<")"<<"("<<strerror(errno)<<") fd_ctx->events="
            << fd_ctx->events;
            return -1;
    }
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    //确保取出来的是空的eventcontext
    BOBLIEW_ASSERT(!event_ctx.scheduler &&
                   !event_ctx.fiber &&
                   ! event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::Getthis();
        BOBLIEW_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, 
                        "state= " << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, IOManager::Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    //非运算符，如果是删除，那么~event的结果就会是0，否则为1
    //Event是之前设定好的环境变量
    Event new_events = (Event)(fd_ctx->events & ~event);
    //判断fd_ctx的Event取反后是否为0，为零说明没有任何操作了
    //如果取反后还有操作，那就是mod，因为之前都是通过|进去的
    // Fucking genius.
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    //epoll在运行中并不会使用data中的内容，但我们需要当事件发生时返回对应的
    //fd context,所以将fd_ctx指针给data.ptr
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<","
            << op << ", "<< fd << ", " << epevent.events << "):"
            << rt << "("<< errno<<")"<<"("<<strerror(errno)<<")";
            return false; 
    }
    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}



bool IOManager::cancelEvent(int fd, IOManager::Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    //非运算符，如果是删除，那么~event的结果就会是0，否则为1
    //Event是之前设定好的环境变量
    Event new_events = (Event)(fd_ctx->events & ~event);
    //判断fd_ctx的Event取反后是否为0，为零说明没有任何操作了
    //如果取反后还有操作，那就是mod，因为之前都是通过|进去的
    // Fucking genius.
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    //epoll在运行中并不会使用data中的内容，但我们需要当事件发生时返回对应的
    //fd context,所以将fd_ctx指针给data.ptr
    epevent.data.ptr = fd_ctx;
    
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<","
            << op << ", "<< fd << ", " << epevent.events << "):"
            << rt << "("<< errno<<")"<<"("<<strerror(errno)<<")";
            return false; 
    }
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size()<fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    //epoll在运行中并不会使用data中的内容，但我们需要当事件发生时返回对应的
    //fd context,所以将fd_ctx指针给data.ptr
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd <<","
            << op << ", "<< fd << ", " << epevent.events << "):"
            << rt << "("<< errno<<")"<<"("<<strerror(errno)<<")";
        return false; 
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    BOBLIEW_ASSERT(fd_ctx->events ==0);
    return true;
}


IOManager* IOManager::GetThis() {
//dyanamic_cast <T> (F::function());
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}



//借用 epoll_wait 方法
//返回是否可以停止
bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
            && m_pendingEventCount ==0
            && Scheduler::stopping();
}

//通知协程调度器有任务生成
void IOManager::tickle() {
//m_tickleFds[0]已经放进了epoll_wait()
//写一个数据进去意思一下
//先判断是否有闲置的线程
    if(!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    BOBLIEW_ASSERT(rt==1);
}

//CORE!!!!!!!!!!!!!!!!!!!!!!!!!!
//协程无任务可调度的时候执行idle线程
void IOManager::idle() {
    BOBLIEW_LOG_DEBUG(g_logger) << "idle";
    //不在协程的栈上分配大数组
    //通过指针调用
    const uint64_t MAX_EVNETS = 256;
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    //shared_ptr不支持直接指向指针，但是可以通过传递一个自定义的deleter来实现
    //析构。
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            BOBLIEW_LOG_INFO(g_logger) << "name = " << m_name << "idle stopping exit";
            break;
        } 

        int rt = 0;
        do {
            //毫秒数
            static const int MAX_TIMEOUT = 3000;
            //存在下一个定时器
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? 
                    MAX_TIMEOUT :next_timeout;
                //如果已经有了设定好的next_timeout,我们还是要取next_timeout和
                //MAX_TIMEOUT之间的较小值。
            } else {
                next_timeout = MAX_TIMEOUT;
                //如果next_TIMEOUT为0，那idle的时间还是设定好的
            }
        

            //epoll wait等待一个描述符为m_epfd的epoll instance，
            //events指向缓冲区，前面已经创建好了的epoll_event数组
            //等到达到了最大数量，就会将储存好的送到缓冲区。
            //MAX_TIMEOUT记录了epoll_wait()函数会阻塞的最大事件
            //我们本来选择的是ET模式，但是如果已经达到空闲的情况，也就没有必要
            //一直要求处于非阻塞的状态。
            //用处：idle是在list空闲的时候运行的，用来收集可能的来自其他协程需要
            //处理的epoll instance，
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);

            // epoll_wait()的返回值，正常情况下会返回收集到的文件描述数量，
            // 0代表没有收集到任何， -1说明出现错误，如果错误为没有收集到任何而
            // 产生的errno会为EINTR，就继续循环。
            // 其他情况：
            // EBADF epfd不是正常的文件描述符，基本不可能发生
            // EFAULT events指向的缓冲区无法正常访问 在此处是不可能发生的
            // EINVAL epfd不是epoll文件的描述符，或者最大等待事件小于零。
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        //将定时器的回调函数schedule进协程
        std::vector<std::function<void()> > cbs;
        listExpireCb(cbs);
        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

//开始处理收集到的epoll instance
//read(int fd, void *buf, size_t count)
//成功时返回读取的字节数，0表示到达文件结尾，出错时返回-1.
        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                //不需要data.fd == m_tickleFds的数据，只需要取出，
                //读干净，否则就会被一直阻塞。
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
            
            //在之前处理数据的时候，已经将一个FdContext*的指针传给了data.ptr
            //使用data.ptr来储存各种我们需要对epoll event进行处理的信息。
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            //如果发生了错误或者挂起，则为其添加读写事件以及fd_ctx本身的事件
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            if((fd_ctx->events & real_events)==NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;
            //将剩余的事件重新插入回到m_epfd
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                BOBLIEW_LOG_ERROR(g_logger) << "epoll_ctl( " << m_epfd << ","
                    << op << ", " << fd_ctx->fd << ", " << event.events << "): "
                    << rt2 << " (" << strerror(errno) << ")";
                continue;
            }
            if(real_events & READ) {
                //处理事件的回调函数或者带有回调函数的协程
                fd_ctx->triggerEvent(READ);
                //-1待办事项
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        //结束时释放协程资源。
        //idle会在run方法里面被调用，其实也可以yiledtoHold，本质也是swapout
        //yieldtohold可能可以减少一些协程的删除的开销。
        Fiber::ptr cur = Fiber::Getthis();
        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();
    }
}


IOManager::FdContext::EventContext& IOManager::FdContext::getContext
(IOManager::Event event) {
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            BOBLIEW_ASSERT2(false, "getContext: NO context");
    }
    throw std::invalid_argument("getContext invalid event");
}

//通过初始化m_fdContexts来避免在addEvent中加入写锁，影响性能
//但代价是初始化IOManager的时候可能需要更大的空间消耗。
void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            //索引就是fd
            m_fdContexts[i]->fd = i;
        }
    }
}


void IOManager::FdContext::resetContext (IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
//在调用triggerEvent的外部函数中都已经加上了锁，所以就不用再在triggerEvent里面
//加锁。
    BOBLIEW_ASSERT(events & event);

    events = (Event)(events & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
    //这里的scheduler已经在add event的时候被加入了Scheduler的指针
    //Scheduler（在这里是IOmanager s的基类）相当于一个总调度器，
    //只需要将我们的call back或者fiber传进去即可。
    //review之前的run方法: 如果是协程，直接swapIn进去进行处理就完事
    //如果是cb，会为这个cb创建一个新协程swapIn。
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}


void IOManager::onTimerInsertedAtFront() {
    tickle();
}

}
