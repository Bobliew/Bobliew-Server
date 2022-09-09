#ifndef __BOBLIEW_HTTP_SERVER_H__
#define __BOBLIEW_HTTP_SERVER_H__

#include "../tcp_server.h"
#include "http_session.h"
#include "servlet.h"

namespace bobliew {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false, bobliew::IOManager* worker = bobliew::IOManager::GetThis(),
               bobliew::IOManager* accept_worker = bobliew::IOManager::GetThis());
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}
    virtual void setName(const std::string& v) override;
private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif
