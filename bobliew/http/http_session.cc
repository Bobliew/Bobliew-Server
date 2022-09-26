
#include "http_session.h"
#include "http_parser.h"
#include "../log.h"



namespace bobliew {
namespace http {

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");
HttpSession::HttpSession(Socket::ptr sock, bool owner) 
:SocketStream(sock, owner){
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();

    //虽然智能指针不能直接指针，但可以通过先设定析构来实现。
    std::shared_ptr<char> buffer (
        new char[buff_size], [](char* ptr) {
            delete[] ptr;
        });
    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if(offset == (int)buff_size) {
            close();
        //    BOBLIEW_LOG_DEBUG(g_logger) << "test3333333333333333333333333";
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    int64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);

        int len = 0;
        if(length >= offset) {
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
               //BOBLIEW_LOG_DEBUG(g_logger) << "test4444444444444444444444444";
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    //parser->getData()->setHeader("Connection", "keep_alive");
    parser->getData()->init();
    //std::string keep_alive =  parser->getData()->getHeader("Connection");
    //if(strcasecmp(keep_alive.c_str(), "Keep-Alive") == 0) {
    //    parser->getData()->setClose(false);
    //}
    //parser->getData()->setClose(false);
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

}
}

