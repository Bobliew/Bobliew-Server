#ifndef __BOBLIEW_HOOK_H__
#define __BOBLIEW_HOOK_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <time.h>


namespace bobliew {

//bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT(); 

// 细化到线程需不需要hook
bool is_hook_enable();

void set_hook_enable(bool flag);


}
//hook是可以举一反三，先实现一下需求最迫切的定时器sleep和socket里面的一些的函数
//实现在用户端同步的操作实现异步的性能效果，减少代码冗杂关键在于hook.cc中的一些
//宏和functional的巧妙运用。
//调用的时候就不需要bobliew::啦~直接引用头文件即可
//extern "C"代表我们使用C风格，C不允许重载
//可以不用做成cpp风格，因为编译过程中，c++风格会添加很多其他字符来实现函数重载
//的功能，使用C风格可以节约开销。
extern "C" {
//在定义我们的实现的同时，也留下原来实现的接口，这样可以满足更加丰富的需求。
//sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

//定义socket
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

//客户端链接服务器
typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

//服务端accept 创建新的链接
typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;


//read (read write针对文件，socket也是文件)
//recv针对socket
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int s, const void *msg, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int s, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

//
typedef int (*fcntl_fun)(int fd, int cmd, .../* arg */);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int d, unsigned long int request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms);
}

#endif
