#include "signalling.hpp"
#include "nlohmann/json.hpp"
#include "rtc_server.hpp"
#include "scache.hpp"

#include <fstream>
#include <map>
#include <string.h>

using json = nlohmann::json;

const std::string workDir = "../static/";

int serveStatic(HttpResponseWriter *w, HttpMessage *r) {
  std::ifstream fin(workDir + "index.html", std::ios::in | std::ios::binary);
  int ret = COCO_SUCCESS;

  if (!fin) {
    coco_error("Failed to open index.html");
    exit(1);
  }

  fin.seekg(0, std::ios::end);
  long size = fin.tellg();

  w->header()->set_content_length((int)size);
  w->header()->set_content_type("text/html");
  w->WriteHeader(CONSTS_HTTP_OK);

  fin.seekg(0, std::ios::beg);
  char *mline = new char[size];
  fin.read(mline, size);

  ret = w->Write(mline, (int)size);

  fin.close();
  return ret;
}

DefaultHandler::DefaultHandler(ISignallingHandler *sigHandler) {
  _sigHandler = sigHandler;
}

DefaultHandler::~DefaultHandler() {}

int DefaultHandler::serve_http(HttpResponseWriter *w, HttpMessage *r) {
  int ret = COCO_SUCCESS;
  coco_dbg("get http req. path: %s, method: %s", r->path().c_str(),
           r->method_str().c_str());

  if (r->path() == "/") {
    ret = serveStatic(w, r);
  } else if (r->path() == "/hello" && r->method_str() == "GET") {
    std::string res = "hello world";
    w->header()->set_content_length((int)res.length());
    w->header()->set_content_type("text/jsonp");

    ret = w->Write(const_cast<char *>(res.c_str()), (int)res.length());
  }

  return ret;
}

PublishHandler::PublishHandler(ISignallingHandler *sigHandler) {
  _sigHandler = sigHandler;
}

PublishHandler::~PublishHandler() {}

int PublishHandler::serve_http(HttpResponseWriter *w, HttpMessage *r) {
  int ret = COCO_SUCCESS;
  coco_dbg("get http req. path: %s, method: %s", r->path().c_str(),
           r->method_str().c_str());
  if (r->method_str() != "POST") {
    coco_dbg("only support POST method for publish");
    go_http_error(w, 404);
    return ERROR_HTTP_HANDLER_INVALID;
  }
  return _sigHandler->HandlePublish(w, r);
}

PlayHandler::PlayHandler(ISignallingHandler *sigHandler) {
  _sigHandler = sigHandler;
}

PlayHandler::~PlayHandler() {}

int PlayHandler::serve_http(HttpResponseWriter *w, HttpMessage *r) {
  int ret = COCO_SUCCESS;
  coco_dbg("get http req. path: %s, method: %s", r->path().c_str(),
           r->method_str().c_str());
  if (r->method_str() != "POST") {
    coco_dbg("only support POST method for Play");
    go_http_error(w, 404);
    return ERROR_HTTP_HANDLER_INVALID;
  }

  return _sigHandler->HandlePlay(w, r);
}

WebRTCSignalling::WebRTCSignalling(std::string ip, uint32_t port,
                                   std::shared_ptr<RTCServer> rtcServer) {
  httpServer = std::unique_ptr<HttpServer>(new HttpServer(true));
  _ip = ip;
  _port = port;
  _rtcServer = rtcServer;
}

WebRTCSignalling::~WebRTCSignalling() {}

int32_t WebRTCSignalling::init() {
  _mux = std::unique_ptr<HttpServeMux>(new HttpServeMux());
  _mux->handle("/", new DefaultHandler(this));
  _mux->handle("/v1/publish", new PublishHandler(this));
  _mux->handle("/v1/play", new PlayHandler(this));

  return COCO_SUCCESS;
}

int32_t WebRTCSignalling::run() {
  init();
  httpServer->ListenAndServe(_ip, _port, _mux.get());
  httpServer->Start();
}

int32_t WebRTCSignalling::HandlePublish(HttpResponseWriter *w, HttpMessage *r) {
  int32_t ret = 0;

  std::string _body;
  r->body_read_all(_body);

  auto jsonb = json::parse(_body);
  std::string offer = jsonb["sdp"];
  std::string streamUrl = jsonb["streamurl"];

  std::string streamID;
  std::string answer_sdp;
  GetStreamIdFromStreamUrl(streamUrl, streamID);

  auto scache = _rtcServer->GetOrCreateStreamCache(streamID);
  if (scache == nullptr) {
    return -13;
  }
  if (scache->CanPublish == false) {
    return -15;
  }

  auto peer = _rtcServer->CreateRtcPublisher(streamID, offer, answer_sdp);
  if (peer == nullptr) {
    coco_error("Create RtcPublisher err");
    return -14;
  }

  json answer;
  answer["sdp"] = answer_sdp;
  std::string res = answer.dump();

  w->header()->set_content_length((int)res.length());
  w->header()->set_content_type("text/jsonp");
  ret = w->Write(const_cast<char *>(res.c_str()), (int)res.length());

  scache->SetPublisher(peer);
  peer->SetStreamCache(scache);

  return ret;
}

int32_t WebRTCSignalling::HandleUnPublish(HttpResponseWriter *w,
                                          HttpMessage *r) {
  return 0;
}

int32_t WebRTCSignalling::HandlePlay(HttpResponseWriter *w, HttpMessage *r) {
  int32_t ret = 0;

  std::string _body;
  r->body_read_all(_body);

  auto jsonb = json::parse(_body);
  std::string offer = jsonb["sdp"];
  std::string streamUrl = jsonb["streamurl"];

  std::string streamID;
  std::string answer_sdp;
  GetStreamIdFromStreamUrl(streamUrl, streamID);

  auto scache = _rtcServer->GetOrCreateStreamCache(streamID);
  if (scache == nullptr) {
    return -13;
  }
  if (scache->CanPublish) { // no publish yet
    return -15;
  }

  auto peer = _rtcServer->CreateRtcPlayer(streamID, offer, answer_sdp);
  if (peer == nullptr) {
    coco_error("Create RtcPeer err");
    return -14;
  }

  json answer;
  answer["sdp"] = answer_sdp;
  std::string res = answer.dump();

  w->header()->set_content_length((int)res.length());
  w->header()->set_content_type("text/jsonp");
  ret = w->Write(const_cast<char *>(res.c_str()), (int)res.length());

  scache->AddPlayer(peer);
  peer->SetStreamCache(scache);

  return 0;
}

int32_t WebRTCSignalling::HandleUnPlay(HttpResponseWriter *w, HttpMessage *r) {
  return 0;
}