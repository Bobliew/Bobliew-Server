#ifndef __BOBLIEW_NONCOPYABLE_H__
#define __BOBLIEW_NONCOPYABLE_H__


namespace bobliew {


class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
    
};


}







#endif
