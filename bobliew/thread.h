#ifndef __BOBLIEW_THREAD_H__
#define __BOBLIEW_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
//c++17可以通过 shared_mutex使用读写锁
//使用读写锁的原因，很多数据读多写少，如果都用同样的锁，可能会造成性能损失。
//另一派观点认为读写锁开销过大，这取决于现实的开发
namespace bobliew {



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
};


}





#endif
