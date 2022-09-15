#include "../bobliew/http/http_server.h"
#include "../bobliew/log.h"

bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();
bobliew::IOManager::ptr worker;
void run() {
    g_logger->setLevel(bobliew::LogLevel::INFO);
    bobliew::Address::ptr addr = bobliew::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        BOBLIEW_LOG_ERROR(g_logger) << "get address error";
        return;
    }

    bobliew::http::HttpServer::ptr http_server(new bobliew::http::HttpServer(true, worker.get()));
    bool ssl = false;
    while(!http_server->bind(addr, ssl)) {
        BOBLIEW_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }

    if(ssl) {
    }

    http_server->start();
}

int main(int argc, char** argv) {
    bobliew::IOManager iom(3);
    worker.reset(new bobliew::IOManager(4, false));
    iom.schedule(run);
    return 0;
}
