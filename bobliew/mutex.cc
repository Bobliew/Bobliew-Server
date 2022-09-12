#include "mutex.h"
#include "macro.h"
#include "scheduler.h"
namespace bobliew {



Semaphore::Semaphore(uint32_t count){
    //sem_init(指向的信号对象， pshared， 初始整数值value)
    //pshard控制信号量类型，在这里0代表信号量用于多线程同步，
   //如果大于0表示可以共享，用于多个相关进程之间的同步。
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error(" sem_init error ");
    }
}

Semaphore::~Semaphore() {

}

void Semaphore::wait() {
    //如果信号箱大于零，则递减继续，如果信号箱等于零，
    //则调用阻塞直到信号量上升到大于0.
    //sen_wait将信号值减一
    //成功后返回0.
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    //sem_post
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

FiberSemaphore::FiberSemaphore(size_t initial_concurrency)
    :m_concurrency(initial_concurrency) {
}

FiberSemaphore::~FiberSemaphore() {
    BOBLIEW_ASSERT(m_waiters.empty());
}

bool FiberSemaphore::tryWait() {
    BOBLIEW_ASSERT(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return true;
        }
        return false;
    }
}

void FiberSemaphore::wait() {
    BOBLIEW_ASSERT(Scheduler::GetThis());
    {
        MutexType::Lock lock(m_mutex);
        if(m_concurrency > 0u) {
            --m_concurrency;
            return;
        }
        m_waiters.push_back(std::make_pair(Scheduler::GetThis(), Fiber::Getthis()));
    }
    Fiber::YieldToHold();
}

void FiberSemaphore::notify() {
    MutexType::Lock lock(m_mutex);
    if(!m_waiters.empty()) {
        auto next = m_waiters.front();
        m_waiters.pop_front();
        next.first->schedule(next.second);
    } else {
        ++m_concurrency;
    }

}


}
