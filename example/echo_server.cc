#include "../bobliew/tcp_server.h"
#include "../bobliew/log.h"
#include "../bobliew/iomanager.h"
#include "../bobliew/bytearray.h"
#include "../bobliew/address.h"

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

class EchoServer : public bobliew::TcpServer {
public:
    EchoServer(int type);
    void handleClient(bobliew::Socket::ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type) {
}

void EchoServer::handleClient(bobliew::Socket::ptr client) {
    BOBLIEW_LOG_INFO(g_logger) << "handleClient " << *client;
    bobliew::ByteArray::ptr ba(new bobliew::ByteArray);
    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);

        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            BOBLIEW_LOG_INFO(g_logger) << "client close" << *client;
            break;
        } else if(rt < 0) {
            BOBLIEW_LOG_INFO(g_logger) << "client error rt=" << rt << "errno= "
                << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {
            std::cout << ba->toString();
        } else {
            std::cout << ba->toHexString();
        }
        std::cout.flush();
    }
}

int type = 1;

void run() {
    BOBLIEW_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = bobliew::Address::LookupAny("0.0.0.0:8033");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv){
    if(argc < 2) {
        BOBLIEW_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" 
        << argv[0] << " -b]";
        return 0;
    }
    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }

    bobliew::IOManager iom(2);
    iom.schedule(run);
    return 0;


}

