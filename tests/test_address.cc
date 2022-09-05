#include "../bobliew/address.h"
#include "../bobliew/log.h"


bobliew::Logger::ptr g_logger = BOBLIEW_LOG_ROOT();

void test() {
    std::vector<bobliew::Address::ptr> addrs;
    bool v = bobliew::Address::Lookup(addrs, "www.baidu.com");
    if(!v) {
        BOBLIEW_LOG_ERROR(g_logger) << "lookup fail";
    }

    for(size_t i = 0; i < addrs.size(); ++i) {
        BOBLIEW_LOG_INFO(g_logger) << i << "=" << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<bobliew::Address::ptr, uint32_t> > results;

    bool v = bobliew::Address::GetInterfaceAddresses(results);
    if(!v) {
        BOBLIEW_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        BOBLIEW_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - "
            << i.second.second;
    }
}

void test_ipv4() {
    //auto addr = sylar::IPAddress::Create("www.sylar.top");
    auto addr = bobliew::IPAddress::Create("127.0.0.8");
    if(addr) {
        BOBLIEW_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    test_iface();
    return 0;
}
