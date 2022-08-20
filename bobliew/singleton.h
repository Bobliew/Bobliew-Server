#ifndef __BOBLIEW_SINGLETON_H_
#define __BOBLIEW_SINGLETON_H_

#include <memory>

namespace bobliew{

namespace {
template < class T, class X, int N>
T& GetInstanceX() {
    static T v;
    return v;
}

template < class T, class X, int N>
std::shared_ptr<T> GetInstancePtr() {
    static std::shared_ptr<T> v(new T);
    return v;
}
}

//单例模式封装类 

template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }
private:
    Singleton() {}
    ~Singleton() {}
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
private:
    SingletonPtr() {}
    ~SingletonPtr() {}
};
}


#endif
