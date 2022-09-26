#ifndef __BOBLIEW_HTTP_SESSION_H__
#define __BOBLIEW_HTTP_SESSION_H__

#include "../streams/socket_stream.h"
#include "http.h"

namespace bobliew {
namespace http {

class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    HttpSession(Socket::ptr sock, bool owner = true);

    HttpRequest::ptr recvRequest();

    int sendResponse(HttpResponse::ptr rsp);

    
};

}
}


#endif
