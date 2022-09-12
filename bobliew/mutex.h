#ifndef __BOBLIEW_MUTEX_H_
#define __BOBLIEW_MUTEX_H_

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>

#include "noncopyable.h"
#include "fiber.h"

namespace bobliew{

class Semaphore : Noncopyable{
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();
private:
    //禁用拷贝构造和运算符=重载
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator = (const Semaphore&) = delete;
private:
    sem_t m_semaphore;
};


//局部变量进行锁和解锁，防止漏掉解锁形成死锁
//通过局部变量的构造函数来加锁 通过析构函数来解锁
//利用局部变量的生命周期来管理锁的获取和释放。
//基于raii的原则
//相当于一个模板类，传入一个互斥量，但因为是模板，所以不关心传入的互斥量的类型
template <class T>
struct ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex)
    :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl() {
        unlock();
    }
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock() {
        if(m_locked) {
            //先把m_locked给设置为flase。
            m_locked = false;
            m_mutex.unlock();
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};


class Mutex: Noncopyable{
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() {
        //atrr为指定的初始化，如果为空，就会按照默认的mutex属性进行构造，
        //但在这里我们传入的mutex应该就是符合我们要求的互斥量，所以直接nullptr
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex;

};

template <class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
    :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl() {
        unlock();
    }
    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock() {
        if(m_locked) {
            //先把m_locked给设置为flase。
            m_locked = false;
            m_mutex.unlock();
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

template <class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
    :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl() {
        unlock();
    }
    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock() {
        if(m_locked) {
            //先把m_locked给设置为flase。
            m_locked = false;
            m_mutex.unlock();
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

class RWMutex : Noncopyable{
public:
typedef ReadScopedLockImpl<RWMutex> ReadLock;
typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};


//测试用空锁
class NULLMutex : Noncopyable{
public:
    typedef ScopedLockImpl<NULLMutex> Lock;
    NULLMutex() {}
    ~NULLMutex() {
    }
    void lock() {
    }
    void unlock() {
    }

};

//测试用空锁
class NULLRWMutex : Noncopyable{
public:
    typedef ReadScopedLockImpl<NULLRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NULLRWMutex> WriteLock;
    NULLRWMutex() {}
    ~NULLRWMutex() {
    }
    void rdlock() {
    }
    void wrlock() {
    }
    void unlock() {
    }
};

//尝试几次后会到cpu挂起
class Spinlock : Noncopyable{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }
    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }
    void lock() {
        pthread_spin_lock(&m_mutex);
    }
    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }    
private:
    pthread_spinlock_t m_mutex;
};

class CASLock : Noncopyable{
public:
    CASLock() {
        m_mutex.clear();
    }
    ~CASLock() {
    }
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }        
private:
    volatile std::atomic_flag m_mutex;
};



class FiberSemaphore : Noncopyable {
public:
    typedef Spinlock MutexType;

    FiberSemaphore(size_t initial_concurrency = 0);
    ~FiberSemaphore();
    
    bool tryWait();
    void wait();
    void notify();

    size_t getConcurrency() const { return m_concurrency;}
    void reset() { m_concurrency = 0;}
private:
    MutexType m_mutex;
    std::list<std::pair<Scheduler* , Fiber::ptr>> m_waiters;
    size_t m_concurrency;
};



}




#endif


