#ifndef __BOBLIEW_HTTP_PARSER_H__
#define __BOBLIEW_HTTP_PARSER_H__


#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace bobliew{
namespace http{

class HttpRequestParser {
public:
    //智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;
    //构造函数
    HttpRequestParser();
    //解析协议
    //参数 协议文本内存 协议文本长度
    //返回实际解析的长度，并且将已解析的数据移除
    size_t execute(char* data, size_t len);
    //是否解析完成
    int isFinished();
    //是否有错误
    int hasError();
    //返回HttpRequest结构体
    HttpRequest::ptr getData() const { return m_data;}
    //设置错误
    void setError(int v) { m_error = v;}
    //获取消息长度
    uint64_t getContentLength();
    //获取http_parser结构体
    const http_parser& getParser() const { return m_parser;}
public:
    //返回HttpRequest协议解析的缓存大小
    static uint64_t GetHttpRequestBufferSize();
    //返回HttpRequest协议的最大消息体大小
    static uint64_t GetHttpRequestMaxBodySize();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    //错误码
    //1000: invalid method
    //1001: invalid version
    //1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    // interprets a byte stream according to the HTTP specification
    HttpResponseParser();
    size_t execute(char* data, size_t len, bool chunck);
    int isFinished();
    int hasError();
    HttpResponse::ptr getData() const { return m_data;}
    void setError(int v) { m_error = v;}
    uint64_t getContentLength();
    const httpclient_parser& getParser() const { return m_parser;}
public:
    static uint64_t GetHttpResponseBufferSize();
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
};


}
}

#endif
