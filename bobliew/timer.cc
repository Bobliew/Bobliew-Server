#include "timer.h"
#include "thread.h"
#include "util.h"
#include "log.h"

namespace bobliew {

bobliew::Logger::ptr x_logger = BOBLIEW_LOG_ROOT();

bool Timer::Comparator::operator()(const Timer::ptr& lhs, 
                                   const Timer::ptr& rhs) const {
    if(!lhs->m_next && !rhs->m_next) {
        return false;
    }
    if(!lhs->m_next) {
        return true;
    }
    if(!rhs->m_next) {
        return false;
    }
    if(lhs->m_next < rhs->m_next) {
        return true;
    }
    if(rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
    :m_recurring(recurring)
    ,m_ms(ms)
    ,m_cb(cb)
    ,m_manager(manager) {
    //毫秒级加上操作间隔 
    //就等于下一个时间
    m_next = bobliew::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) :m_next(next) {
}

TimerManager::TimerManager() {

}

TimerManager::~TimerManager() {
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, 
                                  bool recurring) {
    // 新建一个timmer，然后将这个timer放进timmermanager
    Timer::ptr timer (new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;

}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    //如果m_tickled已经为true，说明已经调用过ontimer
    //如果m_timers是第一个，那说明之前也没调用过
    //分开addtimer 在hook里面
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if(at_front){
        m_tickled = true;
    }
    lock.unlock();
    if(at_front) {
        //无需在基类定义，因为最后定时器只在IO manager中
        onTimerInsertedAtFront();
    }
}

//weak_ptr不会增加shared_ptr的计数，但是又可以了解所指向的指针是否被释放
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
//返回一下weak_cond的智能指针，如果weak_cond所指向的已经被释放
    if(tmp){
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond, 
                                 bool recurring) {
    //通过weak指针来调用cb方法。
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        return ~0ull;
    }
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = bobliew::GetCurrentMS();
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpireCb(std::vector<std::function<void()>>& cbs) {
    //now_ms获取了当前的微秒
    uint64_t now_ms = bobliew::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if(m_timers.empty()) {
        return;
    }
    bool rollover = detectClockRollover(now_ms);
    if(!rollover &&((*m_timers.begin())->m_next > now_ms)) {
        return;
    }
    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    BOBLIEW_LOG_DEBUG(x_logger)<< "nowtimer:    " << now_timer;
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    BOBLIEW_LOG_DEBUG(x_logger)<< "1:    " << expired[0]->m_ms << "2: ";
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto& timer: expired) {
        cbs.push_back(timer->m_cb);
        if(timer -> m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer-> m_cb = nullptr;
        }
    }

}



//检测服务器时间是否被调后
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previousTime && now_ms < (m_previousTime - 60*60*1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}



bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        //如果存在特定指针的comparator
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}


bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager -> m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = bobliew::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + ms;
    m_manager -> addTimer(shared_from_this(), lock);
    return true;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    //如果在m_manager中找不到当前timer,说明无法刷新
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    // 如果找到了，先从manager中删除it，然后再重新设置m_next，再将这个Timer的
    // 指针传递给manager，如果直接重设m_next加进去，因为我们的比较就是根据
    // m_next进行的，会打乱原有顺序，同时会产生同一事件的两个定时器。
    m_manager -> m_timers.erase(it);
    m_next = bobliew::GetCurrentMS() + m_ms;
    m_manager -> m_timers.insert(shared_from_this());
    return true;
}

bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}


}
