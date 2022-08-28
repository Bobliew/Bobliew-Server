#ifndef __BOBLIEW_SCHEDULER_H__
#define __BOBLIEW_SCHEDULER_H__

#include <boost/concept_check.hpp>
#include <memory>
#include "fiber.h"
#include "log.h"
#include "mutex.h"
#include <vector>
namespace bobliew{


class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

//use caller,如果设置成true，会被纳入线程调度器。
    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string& name = "");
//基类
    virtual ~Scheduler();




const std::string& getName() const{ return m_name;}


static Scheduler* GetThis();
static Fiber* GetMainFiber();

void start();
void stop();

//调度协程
//fc 协程或函数
//thread 协程执行的线程id,-1为任意线程
template<class FiberOrCb>
void schedule(FiberOrCb fc, int thread = -1) {
    bool need_tickle = false;
    {
    MutexType::Lock lock(m_mutex);
    need_tickle = scheduleNoLock(fc, thread);
    }
    if(need_tickle) {
        tickle();
    }
}

//批量调度，但要小心智能指针的处理。
template<class InputIterator>
void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
        MutexType::Lock lock(m_mutex);
        while(begin != end) {
        //如果传入了智能指针的指针，因为会减一，所以后序要加回来。
            need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
            ++begin;
        }
    }
    if(need_tickle) {
        tickle();
    }
}

protected:
//返回是否可以停止
virtual bool stopping();

//通知协程调度器有任务生成
virtual void tickle();

//协程无任务可调度的时候执行idle线程
virtual void idle();
void setThis();
void run();

bool hasIdleThreads() { return m_idleThreadCount > 0;}
private:
template<class FiberOrCb>
bool scheduleNoLock(FiberOrCb fc, int thread) {
    bool need_tickle = m_fibers.empty();
    //如果存在Fiberandthread，则插入到m_fibers中
    FiberAndThread ft(fc, thread);
    if(ft.fiber || ft.cb){
        m_fibers.push_back(ft);
    }
    return need_tickle;
}

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
        :fiber(f), thread(thr) {
        }

        FiberAndThread(Fiber::ptr* f, int thr)
        :thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr) 
        :cb(f), thread(thr) {
        }

        FiberAndThread(std::function<void()>* f, int thr) 
        : thread(thr){
            cb.swap(*f);
        }
        FiberAndThread()
        :thread(-1) {
        }
        
        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

protected:
//潜在的优化，可以用map来记录
// 类似 map<pid_t, std::list<FiberANdThread>之类的结构来定位对应的协程池
// 如果协程池内部的协程指定了对应线程来运行，那么需要将这个协程移动到对应key的
// 线程池中。
// 实现方式为一个静态的函数,生成一个全局静态变量。
// ，这个函数引用了这个map，类似logger中的GetInstance（）
// 
// typedef std::map<std::pid_t, std::map<FiberAndThread>> Schedulermap
// private:
// static void getinstance(){
//static Schedulermap m_map;
//return m_map;
// }
//
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::string m_name;
    std::list<FiberAndThread> m_fibers;
    Fiber::ptr m_rootFiber;


protected:
//协程下的线程id数组
std::vector<int> m_threadIds;
//线程数量
size_t m_threadCount = 0;
//工作线程数量
std::atomic<size_t>  m_activeThreadCount = {0};
//空闲线程数
std::atomic<size_t>  m_idleThreadCount = {0};
//是否正在停止
bool m_stopping = true;
//是否自动停止
bool m_autoStop = false;
//主线程id（use caller）
int m_rootThread = 0;

};




}








#endif
