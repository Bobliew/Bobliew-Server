#include "../bobliew/bobliew.h"
#include "../bobliew/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>


bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();
int sock = 0;

void test_fiber() {
    BOBLIEW_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);
    
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        BOBLIEW_LOG_INFO(g_logger) << "add event errno= " << errno << " "
        << strerror(errno);
        bobliew::IOManager::GetThis()->addEvent(sock, bobliew::IOManager::READ, [](){
            BOBLIEW_LOG_INFO(g_logger) << "read callback";
        });
        bobliew::IOManager::GetThis()->addEvent(sock,bobliew::IOManager::WRITE, [](){
            BOBLIEW_LOG_INFO(g_logger) << "write callback";
            bobliew::IOManager::GetThis()->cancelEvent(sock,bobliew::IOManager::READ);
            close(sock);
        });
    } else {
        BOBLIEW_LOG_INFO(g_logger) << "else" << "errno" << " "<< strerror(errno);
    }
}


void test_timer() {
    bobliew::IOManager iom(2);
    bobliew::Timer::ptr timer = iom.addTimer(1000, [&timer](){
        BOBLIEW_LOG_INFO(g_logger) << "hello timer";
        static int i = 0;
        if(++i==5){
            timer->cancel();
        }
    }, true);
}


void test_1() {
    std::cout<< "EPOLLIN = "<< EPOLLIN <<" EPOLLOUT= "<<EPOLLOUT<<std::endl;
    bobliew::IOManager iom;
    iom.schedule(&test_fiber);
}

int main(int argc, char** argv) {
    test_timer();
    return 0;
}
