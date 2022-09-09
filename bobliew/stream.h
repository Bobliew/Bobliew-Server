#ifndef __BOBLIEW_STREAM_H__
#define __BOBLIEW_STREAM_H__

#include <memory>
#include "bytearray.h"

namespace bobliew {

//stream struct

class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;
    virtual ~Stream() {}
    // 读取数据
    // 接收数据的内存
    // 接收数据的内存大小
    // >0 返回接收到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int read(void* buffer, size_t length) = 0;
    // 读取数据
    // 接收数据的bytearray
    // 接收数据的内存大小
    // >0 返回接收到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int read(ByteArray::ptr ba, size_t length) = 0;
    // 读取固定长度的数据
    // 接收数据的内存
    // 接收数据的内存大小
    // >0 返回接收到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int readFixSize(void* buffer, size_t length);
    // 读取固定长度的数据
    // 接收数据的bytearray
    // 接收数据的内存大小
    // >0 返回接收到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int readFixSize(ByteArray::ptr ba, size_t length);
    // 写数据
    // 写数据的内存
    // 写数据的内存大小
    // >0 返回写入到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int write(const void* buffer, size_t length) = 0;
    // 写数据
    // 写数据的ByteArray
    // 写数据的内存大小
    // >0 返回写入到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int write(ByteArray::ptr ba, size_t length) = 0; 
    // 写入固定长度数据
    // 写数据的内存
    // 写数据的内存大小
    // >0 返回写入到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int writeFixSize(const void* buffer, size_t length) ;
    // 写数据
    // 写数据的ByteArray
    // 写数据的内存大小
    // >0 返回写入到的数据的实际大小
    // =0 被关闭
    // <0 出现流错误
    virtual int writeFixSize(ByteArray::ptr ba, size_t length) ;
    //关闭流
    virtual void close() = 0;

};
}


#endif
