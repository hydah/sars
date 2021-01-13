// Copyright 2020 Tencent Inc.  All rights reserved.
//
// Author: qq@tencent.com (Emperor Penguin)

#ifndef _RTC_SERVER_H_
#define _RTC_SERVER_H_

#include "coco/net/http_stack.hpp"
#include "coco/base/log.hpp"
#include "coco/base/st_socket.hpp"
#include "coco/base/error.hpp"
#include "coco/net/net.hpp"
#include "rtc_peer.hpp"
#include "scache.hpp"

class RTCServer : public ListenRoutine, public ISender
{
public:
    UdpListener *l;
    std::string _lip;
    uint32_t _lport;

    std::map<std::string, std::shared_ptr<StreamCache_>> SarsPeersStreamIDMaps;
    std::map<std::string, std::shared_ptr<RtcPeer>> SarsPeersUfragMaps;
    std::map<std::string, std::shared_ptr<RtcPeer>> SarsPeersAddrMaps;
    std::string _lOuterAddr;

public:
    RTCServer(std::string lip, uint32_t lport, std::string lOuterAddr);
    virtual ~RTCServer();
public:
    virtual int32_t cycle();
    int32_t run();
    void OnRecvClientPkt(char* buf, int len, struct sockaddr_in* addr);
    int32_t SendPktToClient(char* buf, int len, struct sockaddr_in* addr);
    int32_t CreateRtcPeer(std::shared_ptr<RtcPeer> peer, const std::string stream_id, std::string &offer, std::string &answer, bool hasLocal = false);
    std::shared_ptr<RtcPublisher> CreateRtcPublisher(const std::string stream_id, std::string &offer, std::string &answer);
    std::shared_ptr<RtcPlayer> CreateRtcPlayer(const std::string stream_id, std::string &offer, std::string &answer);
    std::shared_ptr<StreamCache_> GetOrCreateStreamCache(std::string stream_id);
};
#endif  // _RTC_SERVER_H_