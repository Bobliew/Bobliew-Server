#ifndef __BOBLIEW_FIBER_H__
#define __BOBLIEW_FIBER_H__

//基于ucontext.h 实现协程

#include <memory>
#include<ucontext.h>
#include <functional>



namespace bobliew {

class Scheduler;

// 不可以在栈上创建对象，可以获取当前类的智能指针
// 一定要是智能指针的成员
// 会把当前的智能指针和对象关联
// 如果在栈上用构造函数，但此时shared_pty还没构造完成
// 就无法将智能指针和对象关联。
// esp27 min 7:00
class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT, //初始化状态
        HOLD, //
        EXEC, //
        TERM,
        READY,
        EXCEPT
    };

private:
    //禁用默认构造函数
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller=false);
    ~Fiber();

    //节省线程的创建和析构的开销
    //一旦一个协程结束，用reset可以切换到下一个协程
    //每个线程有main协程，可以开启子协程，
    // mian_fiber <----> sub_fiber
    //       |
    //       |
    //       |
    //       sub_fiber
    //重置协程函数并重置状态（Init或者Term状态）
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台
    void swapOut();
    void call();

public:
    void back();
    //设置当前线程
    static void SetThis(Fiber* f);
    //返回当前执行点的协程
    static Fiber::ptr Getthis();
    //协程切换到后台，并设置为ready状态
    static void YieldToReady();
    //协程切换到后台，并设置为Hold状态
    static void YieldToHold();
    //返回总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static void CallerMainFunc();
    static uint64_t GetFiberId(); 
    State getState() const{ return m_state;}
    uint64_t getId() const{ return m_id;}

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    //ucontext
    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};



}




#endif
