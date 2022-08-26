#include "fiber.h"

#include "config.h"
#include "macro.h"
#include <ctime>
#include "scheduler.h"
#include <atomic>
#include <exception>
#include <ucontext.h>

namespace bobliew {

static Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");
//协程只有两层， 主协程-子协程
static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

//子协程-记录当前协程
static thread_local Fiber* t_fiber = nullptr;
//main协程 智能指针
static thread_local Fiber::ptr t_threadFiber = nullptr;

class MallocStackAllocator {
public:
    static void* ALloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

//把当前的线程变为我们协程的类
//主协程会作为全局变量历经整个函数的声明周期
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)) {
        BOBLIEW_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;
    BOBLIEW_LOG_DEBUG(g_logger) << "Fiber::main ";
}

//真正的创建(子协程)(需要分配栈空间和回调函数)
//每个协程都有对应的栈
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) 
:m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::ALloc(m_stacksize);
    //如果取失败了会返回0
    if(getcontext(&m_ctx)) {
        BOBLIEW_ASSERT2(false, "getcontext");
    }

    //设置好协程的信息
    
    m_ctx.uc_link =nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    BOBLIEW_LOG_DEBUG(g_logger) << "Fiber::Fiber id:"<<m_id;
    
}

//后序可以用一个最小堆来维护m_id
//
Fiber::~Fiber(){
    --s_fiber_count;
    //如果是子协程，这需要释放之前分配的内存空间
    if(m_stack) {
        BOBLIEW_ASSERT(m_state == TERM || m_state==EXCEPT || m_state ==INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        BOBLIEW_ASSERT(!m_cb); //析构的是子线程是没有m_cb的
        BOBLIEW_ASSERT(m_state == EXEC);

        //如果是当前线程就是自己，那么就将t_fiber释放回去。
        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
    BOBLIEW_LOG_DEBUG(g_logger) << "Fiber::~Fiber id: " <<m_id;
}

//重置协程的回调函数，并重置状态
//协程相当于用户态的线程，虽然只有一个主协程，但是能确保当t_fiber是子线程的时候
//t_thread_fiber一定是主线程，所以可以在reset和构造函数中将uc_link指向&t_threadFiber
//会避免上下文都是nullptr导致的线程直接结束。
//另一种方式就是在Mainfunc函数里面手动调回来，这样就不需要在每次reset和构造的时候进行交换了
void Fiber::reset(std::function<void()> cb){
    BOBLIEW_ASSERT(m_stack);
    BOBLIEW_ASSERT(m_state==TERM ||
                   m_state == EXCEPT ||
                   m_state==INIT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        BOBLIEW_ASSERT2(false,"getcontext");
    }
    //m_ctx.uc_link = &t_threadFiber->m_ctx;
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::back() {

    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        BOBLIEW_ASSERT2(false, "swapcontext back()");
    }
}
//切换到当前（main fiber）协程下的子协程运行
//相当于m_ctx和当前正在运行的ctx发生了交换。
//谁调用就换谁进去
void Fiber::swapIn() { 
    SetThis(this);
    BOBLIEW_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)){
        BOBLIEW_ASSERT2(false,"swapcontextin");
    }
}

void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        BOBLIEW_ASSERT2(false, "swapcontext");
    }

}

//相当于两个状态 t_fiber记录确实在运行的 Scheduler::GetMainFiber()->m_ctx
//t_threadFiber记录主协程，交换的时候正在运行的变为主协程，
//然后子协程的地址就会发生交换。
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx,&Scheduler::GetMainFiber()->m_ctx)) {
        BOBLIEW_ASSERT2(false, "swapcontext out")
    }
}

//设置t_fiber 一般由其他函数调用。
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

//返回当前运行的协程
Fiber::ptr Fiber::Getthis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    //Fiber::ptr就是智能指针的类。
    Fiber::ptr main_fiber(new Fiber);
    BOBLIEW_ASSERT(t_fiber == main_fiber.get());

    t_threadFiber = main_fiber;
    return t_threadFiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = Getthis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = Getthis();
    cur->m_state = HOLD;
    cur->swapOut();

}

uint64_t TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = Getthis();
    BOBLIEW_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;//防止一些参数的引用加一。
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        BOBLIEW_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what();
    } catch (...) {
        cur->m_state = EXCEPT;
        BOBLIEW_LOG_ERROR(g_logger)<<"Fiber Except";
    }
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
    BOBLIEW_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}


void Fiber::CallerMainFunc() {
    Fiber::ptr cur = Getthis();
    BOBLIEW_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;//防止一些参数的引用加一。
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        BOBLIEW_LOG_ERROR(g_logger)<<"Fiber Except:"<<ex.what();
    } catch (...) {
        cur->m_state = EXCEPT;
        BOBLIEW_LOG_ERROR(g_logger)<<"Fiber Except";
    }
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    BOBLIEW_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}
}
