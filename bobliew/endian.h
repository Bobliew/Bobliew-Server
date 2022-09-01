#ifndef __BOBLIEW_ENDIAN_H__
#define __BOBLIEW_ENDIAN_H__

//定义几个宏表示是高位地址还是低位地址

#define BOBLIEW_LITTLE_ENDIAN 1
#define BOBLIEW_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace bobliew {


template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t) value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t) value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t) value);
}

#if BYTE_ORDER == BOBLIEW_BIG_ENDIAN
#define BOBLIEW_BYTE_ORDER BOBLIEW_BIG_ENDIAN
#else
#define BOBLIEW_BYTE_ORDER BOBLIEW_LITTLE_ENDIAN
#endif

#if BOBLIEW_BYTE_ORDER == BOBLIEW_BIG_ENDIAN
//如果默认是大端，则只在小端机器上执行byteswap，在大端机器上不进行操作
//确保处理逻辑是一致的，实现跨平台信息传递没有误差
//大端模式：数据的低位保存在高地址
//小端模式：数据的低位保存在低地址
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}
>
#else
//此时BYTE_ORDER = BOBLIEW_LITTLE_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#endif

}


#endif
