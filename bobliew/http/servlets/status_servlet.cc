#include "status_servlet.h"
#include "../../bobliew.h"
#include <ostream>

namespace bobliew {
namespace http {

StatusServlet::StatusServlet()
:Servlet("StatusServlet") {

}

std::string format_used_time(int64_t ts) {
    std::stringstream ss;
    bool v = false;
    if(ts >= 3600 * 24) {
        ss << (ts / 3600 / 24) << "d ";
        ts = ts % (3600 * 24);
        v = true;
    }
    if(ts >= 3600) {
        ss << (ts / 3600) << "h ";
        ts = ts % 3600;
        v = true;
    } else if(v) {
        ss << "0h ";
    }

    if(ts >= 60) {
        ss << (ts / 60) << "m ";
        ts = ts % 60;
    } else if(v) {
        ss << "0m ";
    }
    ss << ts << "s";
    return ss.str();
}


//int32_t handle(bobliew::http::HttpRequest::ptr request,
//                       bobliew::http::HttpResponse::ptr response,
//                       bobliew::http::HttpSession::ptr session) {
//    response->setHeader("Content-Type", "text/text; charset=utf-8");
//#define XX(key) \
//    ss << std::setw(30) << std::right << key ": "
//    std::stringstream ss;
//    ss << "=====================================================" << std::endl;
//    XX("server_version") << "bobliew/1.0.0" << std::endl;
//  //  std::vector<Module::ptr> ms;
//
//
//};
}

}
