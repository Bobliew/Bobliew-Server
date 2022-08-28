#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "util.h"

namespace bobliew {

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

//当前调度器的指针
static thread_local Scheduler* t_scheduler = nullptr;
//主协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    :m_name(name){
    BOBLIEW_ASSERT(threads > 0);
    if(use_caller) {
        //创建主协程
        bobliew::Fiber::Getthis();
        //该协程会使用当前的线程进行操作，所以应该手动--。
        --threads;
        //防止重复产生过调度器
        BOBLIEW_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, use_caller));
        bobliew::Thread::SetName(m_name);
        //在一个线程里面生成一个调度器，再把当前线程放进调度器里，
        //他的主协程将不再是这个线程的主协程，而是执行run方法的主协程。
        //
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = bobliew::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
    
//协程执行了run方法，但没有实际功能
}
//基类
//
Scheduler::~Scheduler(){
    BOBLIEW_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}





Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}


Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);

    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    BOBLIEW_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    //在这里调度线程,从对应的线程里面getId
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
        m_name+ " " + std::to_string(i)));
        

        //Thread里面的信号量semaphore确保了线程会被初始化，
         //所以一定会有对应的Id(我们将内核态的Id作为我们的线程Id)
         //具体在GetThreadId处体现。
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}


//循环等待 
//调度协程
void Scheduler::stop() {
//当主协程（创建当前调度器的线程中运行run的协程）存在，且状态为初始化或已结束的
//时候，就可以停止（这说明没有其他子协程需要运行）就停下来。
//
//如果用了usecaller，就要在特定的线程进行执行
    m_autoStop = true;
    if(m_rootFiber &&
        m_threadCount == 0 &&
        (m_rootFiber->getState()==Fiber::TERM
        || m_rootFiber->getState()==Fiber::INIT)) {
        BOBLIEW_LOG_INFO(g_logger)<< this << "stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }
    if(m_rootThread != -1) {
        BOBLIEW_ASSERT(GetThis() == this);
    } else {
        BOBLIEW_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
    //tickle唤醒线程，等待他们完成
        tickle();
    }
    if(m_rootFiber) {
        tickle();
    }
    if(!stopping()) {
        if(m_rootThread != -1){
        m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto& i: thrs){
        i->join();
    }
}

bool Scheduler::stopping(){
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() 
    && m_activeThreadCount ==0;
}


void Scheduler::tickle() {
    BOBLIEW_LOG_INFO(g_logger)<<"tickle ";
}

void Scheduler::setThis() {
    t_scheduler = this;
}


void Scheduler::idle(){
    BOBLIEW_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        bobliew::Fiber::YieldToHold();
    }
}

void Scheduler::run() {
    BOBLIEW_LOG_INFO(g_logger) << "run";

    setThis();
    //如果当前线程id和储存的不一致，说明还没有建立scheduler的主协程，通过GetThis
    //来建立。
    if(bobliew::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::Getthis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;
    
    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果已经指定好是那个线程运行，则continue
                //thread为-1的情况时，说明任意线程都可以
                if(it->thread != -1 && it->thread != bobliew::GetThreadId()) {
                    ++it;
                    //释放一个信号，给别的协程可以进入线程。
                    tickle_me = true;
                    continue;
                }
                // 这个启动函数的妙处就在于使用一个自定义的类Fiberandthread进行，
                // 这样可以同样适用于线程或者协程的处理，
                //因为it是Fiberandthread，所以如果找到了可以运行的线程，
                //也要确保至少有 fiber或者回调函数中的一个才能继续运行。
                BOBLIEW_ASSERT(it->fiber || it->cb);
                //如果这段协程已经在运行了，就跳到下一个。
                if(it->fiber && it->fiber->getState() ==Fiber::EXEC){
                    ++it;
                    continue;
                }
                ft = *it; //取出需要处理的FiberAndThread
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }    //上面是假如空闲了，就直接tickle,通知别的线程。
        if(tickle_me) {
            tickle();
        }
        //在这个函数运行的时候，有一段时间是会空出一个thread的，
        //可能也会有别的协程会占用线程。
        if(ft.fiber && (ft.fiber->getState()!= Fiber::TERM &&
            ft.fiber->getState() != Fiber::EXCEPT)) {
            //开始处理协程
            ft.fiber->swapIn();
            --m_activeThreadCount;
            //处理结束后,正常结束后，状态应该为READY
            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM && 
                ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber) {
                //利用cb_fiber将ft.cb换进来
                //因为ft.fiber可以直接作为协程swapin，因此不需要取。
                //调用reset()后就直接reset释放掉当前的智能指针（计数减一）
                //
                cb_fiber->reset(ft.cb);
            } else {
                //因为new Fiber需要bind进去一个this指针，所以用.reset，
                //就可以传入this指针
                cb_fiber.reset(new Fiber(ft.cb));
            }
            //清空ft，因为已经将需要传入的cb传给协程处理了
            ft.reset();
            //将带有cb函数的协程传入
            cb_fiber->swapIn();
            --m_activeThreadCount;
            //处理换出来的函数
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                //意外和结束情况，直接reset成nullptr就行
            } else if(cb_fiber->getState() == Fiber::EXCEPT ||
                cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);//计数减一
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                //.reset()是shraed_ptr计数减一
                //->reset是调用Fiber::reset()
                // 
                cb_fiber.reset(); //释放这个指针
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) {
                BOBLIEW_LOG_INFO(g_logger) << "idle fiber term release ";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM &&
                idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}



}
