# BOBLiewServer
### LOG/Config
主要实现日志的流式输出,或者格式化输出，但在实际开发过程中，发现流式的使用方法更加方便，可以更为灵活输出想输出的内容。
使用方法为:
```c++
#include "log.h"
//不指定名称
static bobliew::Logger::ptr a_logger = BOBLIEW_LOG_ROOT()
//指定名称
static bobliew::Logger::ptr b_logger = BOBLIEW_LOG_NAME("system")

BOBLIEW_LOG_DEBUG(a_logger) << "hello bobliew";
```

```c++
//返回效果
2022-09-14 12:04:38	74575	 0	4	[DEBUG]	[0x562bce96e950]	tests/test_http_connection.cc:21	hello bobliew
```

可以通过Yaml进行配置修改。 配置文件一般放在 bin/conf/xxx.yml

### Thread
封装pthread,主要锁的类型，Mutex，RWMutex， Spinlock

### fiber
利用调度器(Scheduler)实现线程池,每个线程有一个主协程和若干个子协程，通过主协程和子协程的切换来实现协程调度。

### IOManager
IO调度，基于协程调度器封装epoll，同时基于epoll_wait的时间戳实现一个定时器。
定时器目前有三种，一次性定时器，循环定时器以及条件定时器。

不是所有协程都处于可运行的状态，当需要等待数据抵达的时候，epoll wait会让出执行时间，通过YieldToHold()进入等待状态。
IOManager等待数据抵达后再唤醒对应正在等待的协程。

### HOOK
对sleep和socket相关的函数进行了hook，可以实现以同步的方式写函数，却执行异步的效果。(如果文件不是socket类型或者已经设置过nonblock，则不会进入hook)
原理：对于有设置过超时时间的任务，则会利用在IOManager中设置好的定时器在指定时间重新执行回调函数。
对于没有设置过超时事件的任务，当该线程有新数据抵达时则会被唤醒检查状态，否则将重新进入休眠。

开启/关闭hook的方式是通过 set_hook_enable()这个函数实现的

一个例子：

一般来说linux的sleep会阻塞整个线程，例如sleep(8) sleep(2) 会在10秒时结束。
通过hook之后，则会(几乎)同时开始计时，然后在8s完成。(具体例子在test/test_hook.cc可以发现,同时本文档中各个模块基本上都有对应的test)。

在合适的情况下enable hook可以提升运行时间的利用效率。

### Socket
主要封装了Address和Socket两个类。
Address包含了IPV4，IPV6，UINX Address，提供了基本的域名和IP解析功能
Socket类提供了socket API的功能

### ByteArray
提供对二进制数据的处理，对于各种类型的(int8_t, int16_t, int32_t, int64_t, std::string)的读写支持。
包括字节序列化，序列化到文件，文件反序列化。

### TCPServer
基于Socket类，封装TCPServer，提供简单API，可以绑定一个/多个地址，启动服务器，监听端口，accept链接
处理socket链接等功能。
（未来可以利用这个类作为基类来实现更具体的其他服务器）

### Stream
包括SocketStream HttpConnection（客户端） HttpSession（服务端）来处理流式数据。

### Http模块
利用Mongreal2提供的有限状态机实现对uri/url的解析，利用Stream中的Connection和Session实现客户端和服务端的链接，
基于TCPServer实现HttpServer，提供了Http的请求功能

### Others

##### My nvim config:
<https://github.com/Bobliew/nvim_config_Bobliew>


