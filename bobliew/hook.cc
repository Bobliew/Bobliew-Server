#include "hook.h"
#include "iomanager.h"
#include <asm-generic/socket.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include "fd_manager.h"
#include <stdarg.h>

//Hook相当于装饰器，调用read函数的时候，会使用自己的函数

bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

namespace bobliew {

static bobliew::ConfigVar<int>::ptr g_tcp_connect_timeout = 
    bobliew::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep)\
    XX(socket)\
    XX(connect)\
    XX(accept)\
    XX(read)\
    XX(readv)\
    XX(recv)\
    XX(recvfrom)\
    XX(recvmsg)\
    XX(write)\
    XX(writev)\
    XX(send)\
    XX(sendto)\
    XX(sendmsg)\
    XX(close)\
    XX(fcntl)\
    XX(ioctl)\
    XX(getsockopt)\
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
//从系统库里取出对应名称的函数，##为链接成为的意思
//RTLD_NEXT取的是第一个出现的，就是取出库函数里面的定义，根据我们的设置，
//后缀为_f的就是用于库函数的，
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    //利用宏将所有的 XX_f的函数都赋值
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            BOBLIEW_LOG_INFO(g_logger)<<"tcp connect timeout changed from"
            << old_value <<" to "<< new_value;
            s_connect_timeout = new_value;
        });
    }
};

//利用数据初始化的前后顺序，全局变量会在main函数之前生成,确保在正式程序运行前，
//我们就通过hook链接了我们定义好的。
static _HookIniter s_hook_initer;

// 细化到线程需不需要hook
bool is_hook_enable() {
    return t_hook_enable;
}
void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}
}

struct timer_info{
    int cancelled = 0;
};


//传一个我们要hook的函数名，用fd去找Socket
//检查socket状态，再检查不是socket或者是用户设置的nonblock再设置
//socket:先执行hook函数，返回值有效，直接返回
//潜在问题，如果有定时器，但将回调
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                     uint32_t event, int timeout_so, Args&&... args) {
    if(!bobliew::t_hook_enable){
        return fun(fd, std::forward<Args>(args)...);
    }
    //BOBLIEW_LOG_DEBUG(g_logger) <<"do_io<"<< hook_fun_name<<">";

    bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(fd);
    //如果ctx不存在，说明该文件就不是socket，就使用原函数就可以
    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }
    if(ctx->isClosed()){
        errno = EBADF;
        return -1;
    }
    //如果不是socket，或者已经设置过非阻塞，那就直接返回fun()就好了
    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    //如果是eintr说明是中断了函数调用，就不断重复调用。
    while(n==-1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    //如果为-1 且errno为EAGAIN，说明处于阻塞状态，说明要进行异步操作
    //找到当前所在的IOmanager，设置好一个定时任务，条件就是这个任务没有被cancel
    if(n==-1 && errno == EAGAIN) {
        bobliew::IOManager* iom = bobliew::IOManager::GetThis();
        bobliew::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);
        //如果to不为-1，等于已经设置过超时时间，那就重新设一个条件定时的定时器
        //在to后检查状态后，如果没有被处理，就设置错误状态并取消事件。
        //这里的t就是一个指向tinfo的weak指针。
        if(to != (uint64_t)-1) {
            timer = iom->addConditionTimer
                (to,[winfo, fd, iom, event](){
                     auto t = winfo.lock();
                     if(!t || t->cancelled) {
                         return;
                     }
                     t->cancelled = ETIMEDOUT;
                     iom->cancelEvent(fd,(bobliew::IOManager::Event)(event));
                 }, winfo);
        }
        //添加一个事件，要么
        int rt = iom->addEvent(fd, (bobliew::IOManager::Event)(event));
        //事件添加失败，需要取消定时器，因为就直接结束了。
        if(rt) {
            BOBLIEW_LOG_ERROR(g_logger)<< hook_fun_name<<"addEvent("<<fd
                << ", "<<event<<")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            //添加事件成功后，就让出当前协程的执行时间
            //
            bobliew::Fiber::YieldToHold();
            //当有处于同一线程的其他事件（IO或者定时器）通过addEvent加入的时候,
            //就会唤醒这个状态，从而进行后面的处理。
            //为了避免重复设置定时器，需要先取消定时器，然后再继续。
            if(timer){
                timer->cancel();
            }
            //如果已经超时 tinfo->cancelled == ETIMEDOUT
            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }
    return n;
}


//extern "C"代表我们使用C风格，C不允许重载
//可以不用做成cpp风格，因为编译过程中，c++风格会添加很多其他字符来实现函数重载
//的功能，使用C风格可以节约开销。
//在定义我们的实现的同时，也留下原来实现的接口，这样可以满足更加丰富的需求。


extern "C"{
//这段宏就相当于
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

//sleep相关的实现
unsigned int sleep(unsigned int seconds) {
    //如果没有使用hook，那就直接返回我们之前赋值过的xx_f
    if(!bobliew::t_hook_enable) {
        return sleep_f(seconds);
    }
    //Fiber的getthis定义返回了智能指针，IOManager的定义返回指针（因为一般只有一个）
    bobliew::Fiber::ptr fiber = bobliew::Fiber::Getthis();
    bobliew::IOManager* iom = bobliew::IOManager::GetThis();
    //bind接受可调用对象生成新的cb传递给addTimer
    //调用schedule，传递的this是iom，传递的参数是fiber和-1.
    //相当于 void(bobliew::Scheduler::* iom) {iom.schdeule(bobliew::Fiber::ptr fiber,-1);}
    //这里处理的还是秒数
    iom->addTimer(seconds * 1000, std::bind((void(bobliew::IOManager::*)
        (bobliew::Fiber::ptr, int thread))&bobliew::IOManager::schedule,iom, fiber,-1));
    bobliew::Fiber::YieldToHold();
    return 0;
}
int usleep(useconds_t usec) {
    if(!bobliew::t_hook_enable) {
        return usleep_f(usec);
    }
    //Fiber的getthis定义返回了智能指针，IOManager的定义返回指针（因为一般只有一个）
    bobliew::Fiber::ptr fiber = bobliew::Fiber::Getthis();
    bobliew::IOManager* iom = bobliew::IOManager::GetThis();
    //bind接受可调用对象生成新的cb传递给addTimer
    //调用schedule，传递的this是iom，传递的参数是fiber和-1.
    //相当于 void(bobliew::Scheduler::* iom) {iom.schdeule(bobliew::Fiber::ptr fiber,-1);}
    //传递一个千分之一ms的定时器去唤醒
    iom->addTimer(usec/1000, std::bind((void(bobliew::Scheduler::*)
        (bobliew::Fiber::ptr, int thread))&bobliew::IOManager::schedule,iom, fiber,-1));
    bobliew::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!bobliew::is_hook_enable()) {
        return nanosleep_f(req, rem);
    }
    int timeout_ms = req->tv_sec*1000 + req->tv_nsec/1000/1000;
    bobliew::Fiber::ptr fiber = bobliew::Fiber::Getthis();
    bobliew::IOManager* iom = bobliew::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(bobliew::Scheduler::*)
        (bobliew::Fiber::ptr, int thread))&bobliew::IOManager::schedule,
                  iom, fiber, -1)); 
    bobliew::Fiber::YieldToHold();
    return 0;
}

//socket相关的实现
//通过FdMgr来管理sockets
int socket(int domain, int type, int protocol) {
    if(!bobliew::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    bobliew::FdMgr::GetInstance()->get(fd, true);
    return fd;
}


int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", bobliew::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        bobliew::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!bobliew::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClosed()) {
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }
    if(ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }
    int n = connect_f(fd, addr, addrlen);
    if(n==0) {
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }
    bobliew::IOManager* iom = bobliew::IOManager::GetThis();
    bobliew::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer
            (timeout_ms, [winfo, fd, iom](){
                 auto t = winfo.lock();
                 if(!t || t->cancelled) {
                     return;
                 }
                 t->cancelled = ETIMEDOUT;
                 iom->cancelEvent(fd, bobliew::IOManager::WRITE);         
             }, winfo);
    }

    int rt = iom->addEvent(fd, bobliew::IOManager::WRITE);
    if(rt == 0) {
        bobliew::Fiber::YieldToHold();
        if(timer) {
            timer-> cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }else {
        if(timer) {
            timer->cancel();
        }
        BOBLIEW_LOG_ERROR(g_logger) << "connect addEvent (" << fd<<" , WRITE) error";
    }
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET,SO_ERROR, &error, &len)) {
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, bobliew::s_connect_timeout);
}


ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", bobliew::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "read", bobliew::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", bobliew::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvmsg", bobliew::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", bobliew::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", bobliew::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", bobliew::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", bobliew::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen){
    return do_io(s, sendto_f, "sendto", bobliew::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", bobliew::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(bobliew::t_hook_enable) {
        return close_f(fd);
    }

    bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = bobliew::IOManager::GetThis();
        if(iom) {
            iom->cancelAll(fd);
        }
        bobliew::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, .../*arg*/) {
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || ctx->isClosed() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= -O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClosed() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & -O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int  arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd,cmd);
        }
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock* arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
        break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
        break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!(int*)arg;
        bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClosed() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!bobliew::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    //将
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            bobliew::FdCtx::ptr ctx = bobliew::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec*1000 + v->tv_usec/1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}





}
