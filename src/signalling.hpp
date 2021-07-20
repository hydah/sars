#include "common/error.hpp"
#include "log/log.hpp"
#include "net/layer7/coco_http.hpp"
#include "rtc_server.hpp"

#include <string>
#include <memory>

class ISignallingHandler
{
public:
    ISignallingHandler() {};
    virtual ~ISignallingHandler() {};
public:
    virtual int32_t HandlePublish(HttpResponseWriter* w, HttpMessage* r) = 0;
    virtual int32_t HandleUnPublish(HttpResponseWriter* w, HttpMessage* r) = 0;
    virtual int32_t HandlePlay(HttpResponseWriter* w, HttpMessage* r) = 0;
    virtual int32_t HandleUnPlay(HttpResponseWriter* w, HttpMessage* r) = 0;
};

class DefaultHandler : public IHttpHandler
{
public:
    ISignallingHandler* _sigHandler;
public:
    DefaultHandler(ISignallingHandler* sigHandler);
    virtual ~DefaultHandler();
public:
    virtual int serve_http(HttpResponseWriter* w, HttpMessage* r);
};

class PublishHandler : public IHttpHandler
{
public:
    ISignallingHandler* _sigHandler;
public:
    PublishHandler(ISignallingHandler* sigHandler);
    virtual ~PublishHandler();
public:
    virtual int serve_http(HttpResponseWriter* w, HttpMessage* r);
};

class PlayHandler : public IHttpHandler
{
public:
    ISignallingHandler* _sigHandler;
public:
    PlayHandler(ISignallingHandler* sigHandler);
    virtual ~PlayHandler();
public:
    virtual int serve_http(HttpResponseWriter* w, HttpMessage* r);
};

class WebRTCSignalling : public ISignallingHandler
{
public:
    std::string _ip;
    uint32_t _port;
    std::unique_ptr<HttpServer> httpServer;
    std::unique_ptr<HttpServeMux> _mux;
    std::shared_ptr<RTCServer> _rtcServer;
public:
    WebRTCSignalling(std::string ip, uint32_t port, std::shared_ptr<RTCServer> rtcServer);
    virtual ~WebRTCSignalling();
public:
    int32_t init();
    int32_t run();
    virtual int32_t HandlePublish(HttpResponseWriter* w, HttpMessage* r);
    virtual int32_t HandleUnPublish(HttpResponseWriter* w, HttpMessage* r);
    virtual int32_t HandlePlay(HttpResponseWriter* w, HttpMessage* r);
    virtual int32_t HandleUnPlay(HttpResponseWriter* w, HttpMessage* r);
};