#ifndef __BOBLIEW_MODULE_H__
#define __BOBLIEW_MODULE_H__


#include "stream.h"
#include "singleton.h"
#include "mutex.h"
#include "rock/rock_stream.h"
#include <map>
#include <unordered_map>

namespace bobliew {

class Module {
public:
    enum Type {
        MODULE = 0,
        ROCK = 1,
    };
    typedef std::shared_ptr<Module> ptr;
    Module(const std::string& name
           ,const std::string& version
           ,const std::string& filename
           ,uint32_t type = MODULE);
    virtual ~Module() {}

    virtual void onBeforeArgsParse(int argc, char** argv);
    virtual void onAfterArgsParse(int argc, char** argv);

    virtual bool onLoad();
    virtual bool onUnload();

    virtual bool onConnect(bobliew::Stream::ptr stream);
    virtual bool onDisconnect(bobliew::Stream::ptr stream);

    virtual bool onServerReady();
    virtual bool onServerUp();

    virtual bool handleRequest(bobliew::Message::ptr req
                               ,bobliew::Message::ptr rsp
                               ,bobliew::Stream::ptr stream);
    virtual bool handleNotify(bobliew::Message::ptr notify
                              ,bobliew::Stream::ptr stream);
    virtual std::string statusString();
    
    const std::string& getName() const { return m_name;}
    const std::string& getVersion() const { return m_version;}
    const std::string& getFilename() const { return m_filename;}
    const std::string& getId() const { return m_id;}

    void setFilename(const std::string& v) { m_filename = v;}

    uint32_t getType() const { return m_type;}

    void registerService(const std::string& erver_type, const std::string& domain,
                         const std::string& service);

protected:
    std::string m_name;
    std::string m_version;
    std::string m_filename;
    std::string m_id;
    uint32_t m_type;
};




}
#endif
