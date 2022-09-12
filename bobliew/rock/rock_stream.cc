#include "rock_stream.h"
#include "../log.h"
#include "../config.h"
#include <unistd.h>
#include "../worker.h"


namespace bobliew {

static bobliew::Logger::ptr g_logger = BOBLIEW_LOG_NAME("system");
static bobliew::ConfigVar<std::unordered_map<std::string, 
std::unordered_map<std::string, std::string>>>::ptr g_rock_services =
    bobliew::Config::Lookup("rock_services", std::unordered_map<std::string, 
                            std::unordered_map<std::string, std::string>>(), 
                            "rock_services");


std::string RockResult::toString() const{
    std::stringstream ss;
    ss  << "[RockResult result=" << result
        << " used=" << used
        << " response=" << (response ? response->toString() : "null")
        << " request=" << (request ? request->toString() : "null")
        << "]";
    return ss.str();
}

RockStream::RockStream(Socket::ptr sock)
    :AsyncSocketStream(sock, true)
    ,m_decoder(new RockMessageDecoder) {
    BOBLIEW_LOG_DEBUG(g_logger) << "RockStream::RockStream " << this << " "
                << (sock ? sock->toString() : "");
}

RockStream::~RockStream() {
    BOBLIEW_LOG_DEBUG(g_logger) << "RockStream"
        << this << " " << (m_socket ? m_socket->toString() : "");
}

int32_t RockStream::sendMessage(Message::ptr msg) {
    if(isConnected()) {
        RockSendCtx::ptr ctx(new RockSendCtx);
        ctx->msg = msg;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}

RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms) {
    if(isConnected()) {
        RockCtx::ptr ctx(new RockCtx);
        ctx->request = req;
        ctx->sn = req->getSn();
        ctx->timeout = timeout_ms;
        ctx->scheduler = bobliew::Scheduler::GetThis();
        ctx->fiber = bobliew::Fiber::Getthis();
        addCtx(ctx);
        uint64_t ts = bobliew::GetCurrentMS();
        ctx->timer = bobliew::IOManager::GetThis()->addTimer(timeout_ms,
            std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        bobliew::Fiber::YieldToHold();
        return std::make_shared<RockResult>(ctx->result, bobliew::GetCurrentMS() - ts,
                                            ctx->response, req);
    } else {
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, 0, nullptr, req);
    }
}

bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder
        ->serializeTo(stream, msg) > 0;
}

bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder
        ->serializeTo(stream, request) > 0;
}

AsyncSocketStream::Ctx::ptr RockStream::doRecv() {
    auto msg = m_decoder->parserFrom(shared_from_this());
    if(!msg) {
        innerClose();
        return nullptr;
    }

    int type = msg->getType();
    if(type == Message::RESPONSE) {
        auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
        if(!rsp) {
            BOBLIEW_LOG_WARN(g_logger) << "RockStream doRecv response not RockResponse: "
                << msg->toString();
            return nullptr;
        }
        RockCtx::ptr ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
        if(!ctx) {
            BOBLIEW_LOG_WARN(g_logger) << "RockStream request timeout response="
                << rsp->toString();
            return nullptr;
        }
        ctx->result = rsp->getResult();
        ctx->response = rsp;
        return ctx;
    } else if(type == Message::REQUEST) {
        auto req = std::dynamic_pointer_cast<RockRequest>(msg);
        if(!req) {
            BOBLIEW_LOG_WARN(g_logger) << "RockStream doRecv request not RockRequest: "
                << msg->toString();
            return nullptr;
        }
        if(m_requestHandler) {
            m_worker->schedule(std::bind(&RockStream::handleRequest,
                                         std::dynamic_pointer_cast<RockStream>(shared_from_this())
                                         ,req));
        } else {
            BOBLIEW_LOG_WARN(g_logger) << "unhandle request " << req->toString();
        }
    } else if (type == Message::NOTIFY) {
        auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
        if(!nty) {
            BOBLIEW_LOG_WARN(g_logger) << "RockStream doRecv notify not RockNotify: "
                << msg->toString();
            return nullptr;
        }

        if(m_notifyHandler) {
            m_worker->schedule(std::bind(&RockStream::handleNotify,
                                         std::dynamic_pointer_cast<RockStream>(shared_from_this()),
                                         nty));
        } else {
            BOBLIEW_LOG_WARN(g_logger) << "unhandle notify " << nty->toString();
        }
    } else {
        BOBLIEW_LOG_WARN(g_logger) << "RockStream recv unknow type=" << type
            << " msg: " << msg->toString();
    }
    return nullptr;
}

void RockStream::handleRequest(bobliew::RockRequest::ptr req) {
    bobliew::RockResponse::ptr rsp = req->createResponse();
    if(!m_requestHandler(req, rsp, std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        sendMessage(rsp);
        close();
    } else {
        sendMessage(rsp);
    }
}

void RockStream::handleNotify(bobliew::RockNotify::ptr nty) {
    if(!m_notifyHandler(nty, std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        close();
    }
}

RockSession::RockSession(Socket::ptr sock)
    :RockStream(sock) {
    m_autoConnect = false;
}

RockConnection::RockConnection()
    :RockStream(nullptr) {
    m_autoConnect = true;
}

bool RockConnection::connect(bobliew::Address::ptr addr) {
    m_socket = bobliew::Socket::CreateTCP(addr);
    return m_socket->connect(addr);
}

//RockSDLoadBalance::RockSDLoadBalance()







}
