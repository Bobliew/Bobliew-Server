#ifndef __BOBLIEW_CONFIG_SERVLET_H__
#define __BOBLIEW_CONFIG_SERVLET_H__

#include "../servlet.h"

namespace bobliew {
namespace http {

class ConfigServlet : public Servlet {
public:
    ConfigServlet();
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
                           bobliew::http::HttpResponse::ptr response,
                           bobliew::http::HttpSession::ptr session) override;

};

}
}


#endif
