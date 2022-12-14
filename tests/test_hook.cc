#include "../bobliew/hook.h"
#include "../bobliew/iomanager.h"
#include "../bobliew/log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <unistd.h>
#include <sys/epoll.h>




bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

void test_sleep() {
    bobliew::IOManager iom(1);
    iom.schedule([](){
        sleep(3);
        BOBLIEW_LOG_INFO(g_logger) << "sleep 3";
    });

    iom.schedule([](){
        sleep(10);
        BOBLIEW_LOG_INFO(g_logger) << "sleep 2";
    });

    BOBLIEW_LOG_INFO(g_logger) << "test_sleep";
 
}


void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "183.232.231.172", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    BOBLIEW_LOG_INFO(g_logger) << "rt value is "<<rt << "errno is "<< errno;
    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    BOBLIEW_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    BOBLIEW_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    BOBLIEW_LOG_INFO(g_logger) << buff;

}


int main(int argc, char** argv) {
    //bobliew::IOManager iom(1,true,"bobliew");
    test_sleep();
    return 0;
}
