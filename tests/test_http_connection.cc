#include <iostream>
#include "../bobliew/http/http_connection.h"
#include "../bobliew/log.h"

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

std::string NETWORK = "www.baidu.com";
void test_pool() {
    bobliew::http::HttpConnectionPool::ptr pool(new bobliew::http::HttpConnectionPool(
                                                    NETWORK, "", 80, false, 10, 1000 * 30, 5));

    bobliew::IOManager::GetThis()->addTimer(1000, [pool](){
                                                auto r = pool->doGet("/", 300);
                                                BOBLIEW_LOG_INFO(g_logger) << r->toString();
                                            }, true);


}

void run() {
    bobliew::Address::ptr addr = bobliew::Address::LookupAnyIPAddress(NETWORK + ":80");
    if(!addr) {
        BOBLIEW_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    bobliew::Socket::ptr sock = bobliew::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        BOBLIEW_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    bobliew::http::HttpConnection::ptr conn(new bobliew::http::HttpConnection(sock));
    bobliew::http::HttpRequest::ptr req(new bobliew::http::HttpRequest);
    //req->setPath("/blog/");
    req->setHeader("host", NETWORK);
    BOBLIEW_LOG_INFO(g_logger) << "req:" << std::endl
        << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if(!rsp) {
        BOBLIEW_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    BOBLIEW_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    std::ofstream ofs("rsp.dat");
    ofs << *rsp;

    BOBLIEW_LOG_INFO(g_logger) << "=========================";

    auto r = bobliew::http::HttpConnection::DoGet("http://" + NETWORK, 300);
    BOBLIEW_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    BOBLIEW_LOG_INFO(g_logger) << "=========================";
    test_pool();
}


int main(int argc, char** argv) {
    bobliew::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
