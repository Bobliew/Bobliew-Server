#include "../bobliew/config.h"
#include "../bobliew/log.cc"
#include "yaml-cpp/yaml.h"

#if 1
//bobliew::ConfigVar<int>::ptr g_int_value_config = 
//bobliew::Config::Lookup("system.port",(int) 8080, "system port");

//bobliew::ConfigVar<float>::ptr g_float_value_config = 
//    bobliew::Config::Lookup("system.port",(float) 10.25, "system port");
//
//bobliew::ConfigVar<float>::ptr g_int_valuex_config =
//    bobliew::Config::Lookup("system.port", (float)8080, "system port");

bobliew::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
    bobliew::Config::Lookup("system.bob", std::vector<int>{1,2,3} , "system bob");

bobliew::ConfigVar<std::list<int> >::ptr g_int_list_value_config =
    bobliew::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

bobliew::ConfigVar<std::set<int> >::ptr g_int_set_value_config =
    bobliew::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int set");

bobliew::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config =
    bobliew::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int uset");

bobliew::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config =
    bobliew::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",2}}, "system str int map");

bobliew::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config =
    bobliew::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",2}}, "system str int map");




void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/log.yml");
    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << root;
}

void test_config() {
    //BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before1" << g_int_value_config->getValue();
    //BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before2" << g_float_value_config->to_String();
    
 #define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << #prefix " " #name ": " << i; \
        } \
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->to_String(); \
    }

 #define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for(auto& i : v) { \
            BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << #prefix " " #name ": {" \
                    << i.first << " - " << i.second << "}"; \
        } \
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << #prefix " " #name " yaml: " << g_var->to_String(); \
    }


    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, str_int_umap, before);

    //YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/test.yml");
    //bobliew::Config::LoadFromYaml(root);

    //BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    //BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "after: " << g_float_value_config->to_String();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, str_int_umap, after);
}

#endif
    
class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string to_String() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace bobliew {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
}

bobliew::ConfigVar<Person>::ptr g_person =
    bobliew::Config::Lookup("class.person", Person(), "system person");

bobliew::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    bobliew::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

bobliew::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    bobliew::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
   // BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before: " << g_person->getValue().to_String() << " - " << g_person->to_String();

#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.to_String(); \
        } \
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) <<  prefix << ": size= " << m.size(); \
    }

    g_person->addListener([](const Person& old_value, const Person& new_value){
        BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "old_value=" << old_value.to_String()
                << " new_value=" << new_value.to_String();
    });

    XX_PM(g_person_map, "class.map before");
    //BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "before: " << g_person_vec_map->to_String();

    YAML::Node root = YAML::LoadFile("/home/bobliew/data/bobliew/bin/conf/log.yml");
    bobliew::Config::LoadFromYaml(root);

    BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "after: " << g_person->getValue().to_String() << " - " << g_person->to_String();
    //XX_PM(g_person_map, "class.map after");
   // BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "after: " << g_person_vec_map->to_String();
}


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





int main(int argc, char** argv) {
    test_log();
    return 0;
}
