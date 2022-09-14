#include "../bobliew/uri.h"
#include <iostream>

int main(int argc, char** argv) {
    bobliew::Uri::ptr uri = bobliew::Uri::Create("http://admin@www.baidu.com");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
