#include "../bobliew/config.h"
#include "../bobliew/log.cc"
#include "yaml-cpp/yaml.h"


bobliew::ConfigVar<int>::ptr g_int_value_config = 
    bobliew::Config::Lookup("system.port",(int) 8080, "system port");

bobliew::ConfigVar<float>::ptr g_float_value_config = 
    bobliew::Config::Lookup("system.port",(float) 10.25, "system port");


bobliew::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
    bobliew::Config::Lookup("system.bob", std::vector<int>{1,2,3} , "system bob");

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/log.yml");
    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << root;
}

void test_config() {
    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before1" << g_int_value_config->getValue();
    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before2" << g_float_value_config->to_String();
    
    auto v = g_int_vec_value_config->getValue();
    for(auto& i: v) {
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before int vec"<<i;
    }

    YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/log.yml");
    bobliew::Config::LoadFromYaml(root);

    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "After3" << g_int_value_config->getValue();
    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "After4" << g_float_value_config->to_String();
    
    v = g_int_vec_value_config->getValue();
     for(auto& i: v) {
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "After int vec"<<i;
    }

   



}

int main(int argc, char** argv) {
    test_config();
    return 0;
}
