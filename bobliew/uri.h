#ifndef __BOBLIEW_URI_H__
#define __BOBLIEW_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"

namespace bobliew {
/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/


class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;
    static Uri::ptr Create(const std::string& uri);

    Uri();

    const std::string& getScheme() const { return m_scheme;}

    const std::string& getUserinfo() const { return m_userinfo;}

    const std::string& getHost() const { return m_host;}

    const std::string& getPath() const { return m_path;}

    const std::string& getQuery() const { return m_query;}

    const std::string& getFragment() const { return m_fragment;}

    int32_t getPort() const { return m_port;}

    void setScheme(const std::string& V) { m_scheme = V;}

    void setUserinfo(const std::string& V) { m_userinfo = V;}

    void setHost(const std::string& V) { m_host = V;}

    void setPath(const std::string& V) { m_path = V;}

    void setQuery(const std::string& V) { m_query = V;}

    void setFragment(const std::string& V) { m_fragment = V;}

    void setPort(int32_t V) { m_port = V;}

    std::ostream& dump(std::ostream& os) const;

    std::string toString() const;

    Address::ptr createAddress() const;
private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    int32_t m_port;
};

}
#endif
