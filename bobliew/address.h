#ifndef __BOBLIEW_ADDRESS_H__
#define __BOBLIEW_ADDRESS_H__

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <map>
#include <sys/un.h>

namespace bobliew {

class IPAddress;

class Address {
public:
    typedef std::shared_ptr<Address> ptr;
    //基类，可按照需求换成IPV4，IPV6等不同的格式
    //通过sockaddr指针创建address，失败返回nullptr
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
    //通过host返回所有address
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                       int family = AF_INET, int type = 0, int protocol = 0);
    //通过host地址返回对应条件的任意address
    static Address::ptr LookupAny(const std::string& host, int family = AF_INET,
                                  int type = 0, int protocol = 0);
    //通过host地址返回对应条件的任意address，（智能指针）
    static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                         int family = AF_INET, int type = 0, int protocol = 0);

    static bool GetInterfaceAddresses(std::multimap<std::string
                                      ,std::pair<Address::ptr, uint32_t> >& result,
                                      int family = AF_INET);
    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result
                    ,const std::string& iface, int family = AF_INET);
    virtual ~Address() {};
    //返回协议簇
    int getFamily() const;
    //只读 返回sockaddr指针
    virtual const sockaddr* getAddr() const = 0;
    //读写 返回sockaddr指针
    virtual sockaddr* getAddr() = 0;
    //返回sockaddr长度
    virtual socklen_t getAddrLen() const = 0;
    //可读性输出地址
    virtual std::ostream& insert(std::ostream& os)const = 0;
    //返回可读性字符串
    std::string toString() const;
    //运算符重载
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;


};



class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;
    static IPAddress::ptr Create(const char* address, uint16_t port = 0);
    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPPort(uint32_t v) = 0;
};


class IPV4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPV4Address> ptr;
    //使用点分十进制地址创建IPV4Address(192.168.1.1)
    //port 端口号
    static IPV4Address::ptr Create(const char* address, uint16_t port = 0);
    //通过sockaddr构造IPV4结构体
    IPV4Address(const sockaddr_in& address);
    //通过二进制地址构造IPV4结构体
    IPV4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& os)const override;
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPPort(uint32_t v) override; 

private:
    sockaddr_in m_addr;
};


class IPV6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPV6Address> ptr;
    static IPV6Address::ptr Create(const char* address, uint16_t port = 0);
    IPV6Address();
    IPV6Address(const sockaddr_in6& address);
    IPV6Address(const uint8_t address[16], uint16_t port = 0);
//Address
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
//IPAddress
    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networkAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;

    uint32_t getPort() const override;
    void setPPort(uint32_t v) override; 

private:
    sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& insert(std::ostream& os)const override;
private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
private:
    sockaddr m_addr;
};

// stream form for Address
std::ostream& operator<<(std::ostream& os, const Address& addr);

}

















#endif
