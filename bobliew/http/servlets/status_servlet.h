#ifndef __BOBLIEW_STATUS_SERVLET_H__
#define __BOBLIEW_STATUS_SERVLET_H__

#include "../servlet.h"


namespace bobliew {
namespace http {
class StatusServlet : public Servlet {
public:
    StatusServlet();
    virtual int32_t handle(bobliew::http::HttpRequest::ptr request,
                           bobliew::http::HttpResponse::ptr response,
                           bobliew::http::HttpSession::ptr session) override;
};

}
}

#endif
