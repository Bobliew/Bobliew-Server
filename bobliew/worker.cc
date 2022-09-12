#include "worker.h"
#include "config.h"
#include "util.h"

namespace bobliew {

static bobliew::ConfigVar<std::map<std::string, std::map<std::string, std::string>>>::ptr g_worker_config
    = bobliew::Config::Lookup("workers", std::map<std::string, std::map<std::string, std::string>>(), "worker config");

WorkerGroup::WorkerGroup(uint32_t batch_size, bobliew::Scheduler* s)
    :m_batchSize(batch_size)
    ,m_finish(false)
    ,m_scheduler(s)
    ,m_sem(batch_size){
}

WorkerGroup::~WorkerGroup() {
    waitAll();
}

void WorkerGroup::schedule(std::function<void()> cb, int thread) {
    m_sem.wait();
    m_scheduler->schedule(std::bind(&WorkerGroup::doWork, shared_from_this(), cb), thread);
}

void WorkerGroup::waitAll() {
    if(!m_finish) {
        m_finish = true;
        for(uint32_t i = 0; i < m_batchSize; ++i) {
            m_sem.wait();
        }
    }
}
    
void WorkerGroup::doWork(std::function<void()> cb) {
    cb();
    m_sem.notify();
}


WorkerManager::WorkerManager() :m_stop(false) {
}

void WorkerManager::add(Scheduler::ptr s) {
    m_datas[s->getName()].push_back(s);
}
Scheduler::ptr WorkerManager::get(const std::string& name) {
    auto it = m_datas.find(name);
    if(it == m_datas.end()) {
        return nullptr;
    }
    if(it->second.size() == 1) {
        return it->second[0];
    }
    return it->second[rand() % it->second.size()];
}

IOManager::ptr WorkerManager::getAsIOManager(const std::string& name) {
    return std::dynamic_pointer_cast<IOManager>(get(name));
}

bool WorkerManager::init() {
    auto worker = g_worker_config->getValue();
    return init(worker);
}

bool WorkerManager::init(const std::map<std::string, std::map<std::string, std::string>>& v) {
    for(auto& i : v) {
        std::string name = i.first;
        int32_t thread_num = bobliew::GetParamValue(i.second, "thread_num", 1);
        int32_t worker_num = bobliew::GetParamValue(i.second, "worker_num", 1);
        
        for(int32_t x = 0; x < worker_num; ++x) {
            Scheduler::ptr s;
            if(!x) {
                s = std::make_shared<IOManager>(thread_num, false, name);
            } else {
                s = std::make_shared<IOManager>(thread_num, false, name + "-" + std::to_string(x));
            }
            add(s);
        }
    }
    m_stop = m_datas.empty();
    return true;
}
void WorkerManager::stop() {
    if(m_stop) {
        return;
    }
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            //至少传入一个空lamda，确保不会在空的时候进行
            n->schedule([](){});
            n->stop();
        }
    }
    m_datas.clear();
    m_stop = true;
}

std::ostream& WorkerManager::dump(std::ostream& os) {
    for(auto& i : m_datas) {
        for(auto& n : i.second) {
            n->dump(os) << std::endl;
        }
    }
    return os;
}

uint32_t WorkerManager::getCount() {
    return m_datas.size();
}


}
