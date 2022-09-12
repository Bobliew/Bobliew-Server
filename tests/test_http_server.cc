#include "../bobliew/http/http_server.h"
#include "../bobliew/log.h"

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");

#define XX(...) #__VA_ARGS__
void run() {
    bobliew::http::HttpServer::ptr server(new bobliew::http::HttpServer(true));
    bobliew::Address::ptr addr = bobliew::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/bobliew/xx", [](bobliew::http::HttpRequest::ptr req
                ,bobliew::http::HttpResponse::ptr rsp
                ,bobliew::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/bobliew/*", [](bobliew::http::HttpRequest::ptr req
                ,bobliew::http::HttpResponse::ptr rsp
                ,bobliew::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/bobliewx/*", [](bobliew::http::HttpRequest::ptr req
                ,bobliew::http::HttpResponse::ptr rsp
                ,bobliew::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    bobliew::IOManager iom(2, true, "main");
    //worker.reset(new bobliew::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
