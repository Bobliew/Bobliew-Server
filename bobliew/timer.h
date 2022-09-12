#ifndef __BOBLIEW_TIMER_H__
#define __BOBLIEW_TIMER_H__

#include <bits/stdint-uintn.h>
#include <memory>
#include "thread.h"
#include "mutex.h"
#include <set>
#include <vector>

namespace bobliew{

class TimerManager;

//定时器
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;
public:
//定时器指针
    typedef std::shared_ptr<Timer> ptr;
//取消定时器
    bool cancel();
//刷新设置定时器的执行时间
    bool refresh();
//重置定时器时间 ms定时器执行间隔时间 from_now是否从当前时间开始计算
    bool reset(uint64_t ms, bool from_now);

private:
//构造函数 ms定时器执行间隔时间 cb回调函数 recurring是否循环 manager定时器管理器
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
//构造函数 执行的时间戳
    Timer(uint64_t next);


private:
    //是否循环定时器
    bool m_recurring = false;
    //时间间隔 执行周期
    uint64_t m_ms = 0;
    //精确的执行时间
    uint64_t m_next = 0;
    //回调函数
    std::function<void()> m_cb;
    TimerManager* m_manager;

private:
//定时器比较符函数
struct Comparator {
    bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs)const;
};

};


//定时器管理器
class TimerManager {
friend class Timer;
public:
    //读写锁类型
    typedef RWMutex RWMutexType;
    TimerManager();
    virtual ~TimerManager();

    //添加定时器
    //定时器执行间隔事件
    //定时器回调函数
    //是否循环定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, 
                        bool recurring = false);
    //添加条件定时器
    //和之前一样的参数 添加了weak_cond作为判断条件
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond, 
                                 bool recurring = false);
    //获取最近一个定时器执行的时间间隔
    uint64_t getNextTimer();
    //获取需要执行的定时器的回调函数列表
    void listExpireCb(std::vector<std::function<void()>>& cbs);
    //是否有定时器
    bool hasTimer();

protected:
    //当有新的定时器插入到管理器的首部，执行这个函数
    virtual void onTimerInsertedAtFront() = 0;
    // 将定时器添加到管理器
    //
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

    //检测服务器时间是否被调后
    
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    //set可以传递一个比较器过去，在这里相当于一个函数，
    //因此我们在Timer的Comparator struct中重载了运算符()
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    // 是否触发onTimeerInsertedAtFront
    bool m_tickled = false;
    //上次执行的事件
    uint64_t m_previousTime = 0;
};




}
#endif
