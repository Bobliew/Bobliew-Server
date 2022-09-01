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

class Address {
public:
    typedef std::shared_ptr<Address> ptr;
    //基类，可按照需求换成IPV4，IPV6等不同的格式
    virtual ~Address();

    int getFamily() const;

    virtual const sockaddr* getAddr() const = 0;
    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os)const = 0;
    std::string toString();

    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;


};



class IPAddress : public Address {
public:
    typedef std::shared_ptr<IPAddress> ptr;

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;
    virtual void setPPort(uint32_t v) = 0;
};


class IPV4Address : public IPAddress {
public:
    typedef std::shared_ptr<IPV4Address> ptr;
    IPV4Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
    const sockaddr* getAddr() const override;
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
    IPV6Address(uint32_t address = INADDR_ANY, uint32_t port = 0);
//Address
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
    UnixAddress(const std::string& path);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
private:
    struct sockaddr_un m_addr;
    socklen_t length;
};

class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& insert(std::ostream& os)const override;
private:
    sockaddr m_addr;
};

// stream form for Address
std::ostream operator<<(std::ostream& os, const Address& addr);

}

















#endif
