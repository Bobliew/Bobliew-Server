#ifndef __BOBLIEW_BYTEARRAY_H__
#define __BOBLIEW_BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/uio.h>
#include <vector>
//二进制数组 序列化 反序列化
namespace bobliew {
class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    //用链表节点来链接,实现容纳大量数据。
    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };
    ByteArray(size_t base_size = 4096);
    ~ByteArray();
    //write(写入固定长度的int_t) (大端/小端)
    void writeFint8(const int8_t value);
    void writeFuint8(const uint8_t value);
    void writeFint16(const int16_t value);
    void writeFuint16(const uint16_t value);
    void writeFint32(const int32_t value);
    void writeFuint32(const uint32_t value);
    void writeFint64(const int64_t value);
    void writeFuint64(const uint64_t value);


    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);
    void writeStringVint(const std::string& value);
    void writeStringWithoutLength(const std::string& value);

    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    //内部操作
    void clear();
    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;
    size_t getPosition() const { return m_position;}
    void setPosition(size_t v);
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);
    
    size_t getBaseSize() const { return m_baseSize;}
    size_t getReadSize() const { return m_size - m_position;};
    
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);


    std::string toString() const;
    std::string toHexString() const;

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    size_t getSize() const { return m_size;}
private:
void addCapacity(size_t size);
size_t getCapacity() const { return m_capacity - m_position;}

private:
size_t m_position;
size_t m_baseSize;
size_t m_size;
size_t m_capacity;
int8_t m_endian;
Node* m_root;
Node* m_curr;

};
}



#endif
