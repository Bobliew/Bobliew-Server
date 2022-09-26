#include "http_server.h"
#include "../log.h"
#include "servlets/config_servlet.h"
//#include "servlets/status_servlet.h"


namespace bobliew {
namespace http {

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

//HttpServer是TcpServer的一个子类
HttpServer::HttpServer(bool keepalive
                       ,bobliew::IOManager* worker
                       ,bobliew::IOManager* io_worker
                       ,bobliew::IOManager* accept_worker)
                       :TcpServer(worker, io_worker, accept_worker)
                       ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    m_type = "http";
    //m_dispatch->addServlet("/_/statue", Servlet::ptr(new StatusServlet));
    m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
    BOBLIEW_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            BOBLIEW_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " client:" << *client << " keep_alive=" << m_isKeepalive;
            BOBLIEW_LOG_DEBUG(g_logger) << "test req:" << req;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(),
                                       req->isClose() || !m_isKeepalive));
        //当handler之前先整合，再一起发，session用来做一些上下文判断
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        
        //rsp->setBody("Hello bobliew");
        if(!m_isKeepalive || req->isClose()) {
            //std::cout<< "keep_alive=" << m_isKeepalive<< "close=" << req->isClose() << std::endl;
            break;
        }
    } while(true);
    session->close();
}

}
}

