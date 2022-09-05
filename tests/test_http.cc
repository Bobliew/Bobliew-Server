#include "../bobliew/http/http.h"
#include "../bobliew/log.h"

void test() {
    bobliew::http::HttpRequest::ptr req(new bobliew::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");

    req->dump(std::cout) << std::endl;
}

int main(int arg, char** argv) {
    test();
    return 0;
}
