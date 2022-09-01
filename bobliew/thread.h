#ifndef __BOBLIEW_THREAD_H__
#define __BOBLIEW_THREAD_H__

#include <list>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include "noncopyable.h"
//c++17可以通过 shared_mutex使用读写锁
//使用读写锁的原因，很多数据读多写少，如果都用同样的锁，可能会造成性能损失。
//另一派观点认为读写锁开销过大，这取决于现实的开发
namespace bobliew {

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

class Thread {
public:
    //框架下尽量不用裸指针，尽量使用智能指针，降低内存泄露的风险。
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id;}
    const std::string& getName() const { return m_name;}

    void join();

    //需要调用当前线程
    static Thread* GetThis();
    //方便写日志的时候写入名称
    static const std::string& GetName();
    static void SetName (const std::string& name);
private:
    //互斥量和互斥信号量不能作拷贝，否则他们存在的意义就没有了
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;


    //静态函数，所以this指针失效了，
    static void* run(void* arg);
private:
    pid_t m_id;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;
    Semaphore m_semaphore;
};

}





#endif
