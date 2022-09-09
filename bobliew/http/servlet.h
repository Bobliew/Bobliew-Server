#ifndef __BOBLIEW_SERVLET_H__
#define __BOBLIEW_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "../thread.h"
#include "../util.h"


namespace bobliew {
namespace http {

class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        :m_name(name){}
    virtual ~Servlet() {}
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
    bobliew::http::HttpResponse::ptr response,
    bobliew::http::HttpSession::ptr session) = 0;

    const std::string& getName() const { return m_name;}
    
protected:
    std::string m_name;
};

class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (bobliew::http::HttpRequest::ptr request,
                                   bobliew::http::HttpResponse::ptr response,
                                   bobliew::http::HttpSession::ptr session)> callback;
    FunctionServlet(callback cb);
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
    bobliew::http::HttpResponse::ptr response,
    bobliew::http::HttpSession::ptr session) override;

   
private:
    callback m_cb;
};

class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> ptr;
    virtual ~IServletCreator() {}
    virtual Servlet::ptr get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<HoldServletCreator> ptr;
    HoldServletCreator(Servlet::ptr slt):m_servlet(slt) {
    }
    Servlet::ptr get() const override {
        return m_servlet;
    }
    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    Servlet::ptr m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> ptr;

    ServletCreator() {
    }

    Servlet::ptr get() const override {
        return Servlet::ptr(new T);
    }

    std::string getName() const override {
        return TypeToName<T>();
    }
};

//Servlet分发器
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
                           bobliew::http::HttpResponse::ptr response,
                           bobliew::http::HttpSession::ptr session) override;
    //添加Servlet
    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    //添加模糊匹配Servlet
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void addServletCreator(const std::string& uri, IServletCreator::ptr creator);
    void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);

    template <class T>
    void addServletCreator(const std::string& uri) {
        addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    template <class T>
    void addGlobServletCreator(const std::string& uri) {
        addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    void delServlet(const std::string& uri);

    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}

    void setDefault(Servlet::ptr v) { m_default = v;}

    Servlet::ptr getServlet(const std::string& uri);

    Servlet::ptr getGlobServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);

    void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
private:
    RWMutexType m_mutex;
    std::unordered_map<std::string, IServletCreator::ptr> m_datas;
    std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet(const std::string& name);
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
                           bobliew::http::HttpResponse::ptr response,
                           bobliew::http::HttpSession::ptr session) override;
private:
    std::string m_name;
    std::string m_content;
};



}
}



#endif

