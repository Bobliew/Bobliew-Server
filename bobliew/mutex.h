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

//#include "noncopyable.h"
//#include "fiber.h"


namespace {


class Semaphore {
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
//通过类的构造函数来加锁 通过类的析构函数来解锁
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
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

}



#endif


