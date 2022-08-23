#include "thread.h"
#include "log.h"
#include "util.h"



namespace bobliew {

//线程局部变量，有什么奇技淫巧。
//主线程有可能不是自己创造的。
//指向当前线程的指针
static thread_local Thread* t_thread = nullptr;
static std::string t_thread_name = "UNKNOW";
static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");


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
    //sen_wait将信号值减一
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

Thread* Thread::GetThis(){
    return t_thread;
}
//方便写日志的时候写入名称
const std::string& Thread::GetName(){
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

 
Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb),
    m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    //成功后会返回0
    if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "pthread_create thread fail, rt= " <<
        rt << " name = " << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();
}


Thread::~Thread(){
    if(m_thread) {
        //线程还是会继续运行，当终止时会自动释放，
        //但需要注意线程运行结束的
        pthread_detach(m_thread);
    }
}

//静态函数，所以this指针失效了，
//为了防止shared_ptr在线程函数被调用的时候进行了释放，所以把线程函数换出来，

void* Thread::run(void* arg){
    Thread* thread = (Thread*) arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = bobliew::GetThreadId();
    //设置目标线程名称（但不能超过16个字符数）所以要加以限制， 返回0则成功
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
    std::function<void()> cb;
    cb.swap(thread->m_cb);
    //线程真正地启动
    thread->m_semaphore.notify();
    cb();
    return 0;
}


//
void Thread::join() {
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        //等待m_thread的结束，如果线程成功结束返回0
        if(rt) {
        BOBLIEW_LOG_ERROR(g_logger) << "pthread_join thread fail,rt= "<< rt
                << " name="<< m_name;
        throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}



}
