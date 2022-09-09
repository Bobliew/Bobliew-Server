#include "../bobliew/tcp_server.h"
#include "../bobliew/iomanager.h"
#include "../bobliew/log.h"
#include <sys/socket.h>

bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();
void run() {
    auto addr = bobliew::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = bobliew::UnixAddress::ptr(new bobliew::UnixAddress("/tmp/unix_addr"));
    //BOBLIEW_LOG_INFO(g_logger)<<*addr << "-" << *addr2;
    std::vector<bobliew::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    bobliew::TcpServer::ptr tcp_server(new bobliew::TcpServer);
    std::vector<bobliew::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}

int main(int argc, char** argv) {
    bobliew::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

