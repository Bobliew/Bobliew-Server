#include "../bobliew/http/http.h"
#include "../bobliew/log.h"

void test() {
    bobliew::http::HttpRequest::ptr req(new bobliew::http::HttpRequest);
    //req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");
    req->setHeader("Connection", "keep-alive");
    req->dump(std::cout) << std::endl;
    std::string conn = req->getHeader("host");
    std::cout<<"conn=" << conn<<std::endl;
}

int main(int arg, char** argv) {
    test();
    return 0;
}
