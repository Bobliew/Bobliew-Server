#include "../bobliew/config.h"
#include "../bobliew/log.cc"
#include "yaml-cpp/yaml.h"



void test_log() {
    //bobliew::FileLogAppender::ptr file_appender(new bobliew::FileLogAppender("./log.txt"));

    static bobliew::Logger::ptr system_log = BOBLIEW_LOG_NAME("system");
    //system_log->addAppender(file_appender);

    //本来就会初始化一个root放在Mgr的GetInstance（）就是s_data中.
    //然后新建立的BOBLIEW_LOG_NAME（“system”）生成的system也被记录到s_data中
    //BOBLIEW_LOG_NAME被宏定义为 Mgr下的getLogger()方法，如果不存在name则生成对应的
    //日志项并返回指针，如果存在则直接返回指针。
    std::cout<<bobliew::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/log.yml");
    bobliew::Config::LoadFromYaml(root);
    std::cout << "=============" << std::endl;
    //我们之前也已经add了listener进入在
    std::cout << bobliew::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << root << std::endl;
    BOBLIEW_LOG_INFO(system_log) << "hello system" << std::endl;
    //system_log->setFormatter("%d - %m%n");
    BOBLIEW_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main() {
test_log();
return 0;
}
