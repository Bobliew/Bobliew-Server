#ifndef __BOBLIEW_CONFIG_H__
#define __BOBLIEW_CONFIG_H__

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include <yaml-cpp/yaml.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace bobliew {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "")
        :m_name(name), 
        m_description(description) {
        //transform的性质
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() {}
    
    const std::string& getName() { return m_name;}
    const std::string& getDescription() { return m_description;}

    virtual std::string to_String() = 0;
    virtual bool fromString(const std::string& val) = 0;

protected:
    std::string m_name;
    std::string m_description;

};


//From T to F
template<class F, class T>
class LexicalCast {
public:
    T operator() (const F& v) {
        return boost::lexical_cast<T>(v);
    }
};

//vector <-> string
template <class T>
class LexicalCast <std::string, std::vector<T>> {
public: 
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast <std::vector<T>, std::string> {
public: 
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//list <->string
template <class T>
class LexicalCast <std::string, std::list<T>> {
public: 
    std::vector<T> operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template <class T>
class LexicalCast <std::list<T>, std::string> {
public: 
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//





//Fromstr T operator() (const std::string&)
//toStr  std::string operator() (const T&)
template <class T, 
          class FromStr = LexicalCast<std::string, T>, 
          class ToStr = LexicalCast<T, std::string>>

class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    
    ConfigVar(const std::string& name, 
              const T& default_value, const std::string& description = "")
              : ConfigVarBase(name, description), 
                m_val(default_value) {
    }

    std::string to_String() override {
        try {
            //return boost::lexical_cast<std::string> (m_val);
            return ToStr()(m_val);
        } catch(std::exception& e) {
            BOBLIEW_LOG_ERROR(BOBLIEW_LOG_ROOT()) << "ConfigVar::toString exception"
                <<e.what()<<"convert:"<<typeid(m_val).name() << "to_string";
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
            //m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
        } catch(std::exception& e) {
            BOBLIEW_LOG_ERROR(BOBLIEW_LOG_ROOT()) << "ConfigVar::toString exception"
                <<e.what()<<"convert: string to"<<typeid(m_val).name();
        }
        return false;
    }

    void LoadFromYaml(const YAML::Node& root); 
    const T getValue() const { return m_val;}
    void setValue(const T& v) { m_val = v;}
private:
    T m_val;

};

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, 
                                             const T& default_value,
                                             const std::string& description = "") {
        auto tmp = Lookup<T>(name);
        if(tmp) {
            BOBLIEW_LOG_INFO(BOBLIEW_LOG_ROOT()) << "Look up name = "<<
            name<< "exists";
            return tmp;
        }
        //first_not_of的用法
        //string::npo的用法。
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") !=
            std::string::npos) {
            BOBLIEW_LOG_ERROR(BOBLIEW_LOG_ROOT()) << "Lookup name invalid"<< name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        s_datas[name] = v;
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
        auto it = s_datas.find(name);
        if(it == s_datas.end() ) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> > (it->second);
    }

    static void LoadFromYaml(const YAML::Node& root);
    static ConfigVarBase::ptr LookupBase(const std::string& name);
private:
    static ConfigVarMap s_datas;

};


}


#endif

