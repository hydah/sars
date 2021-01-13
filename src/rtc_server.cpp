// Author: fisheryhe@tencent.com 

#include "rtc_server.hpp"
#include "rtc_publisher.hpp"
#include "rtc_player.hpp"
#include "Stun.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string.h>

int getUfragFromSdp(std::string &sdp, std::string &ufrag)
{
    std::size_t spos = sdp.find("ice-ufrag");
    if (spos == std::string::npos) {
        return ERROR_HTTP_HANDLER_INVALID;
    }
    std::size_t epos = sdp.find("\r\n", spos+10);
    if (epos == std::string::npos) {
        return ERROR_HTTP_HANDLER_INVALID;
    }
    ufrag = sdp.substr(spos+10, epos-spos-10);

    return ERROR_SUCCESS;
}

RTCServer::RTCServer(std::string lip, uint32_t lport, std::string lOuterAddr)
{
    l = nullptr;
    _lip = lip;
    _lport = lport;
    _lOuterAddr = lOuterAddr;
}

RTCServer::~RTCServer()
{
    if(l) {
        delete l;
    }
}

void RTCServer::OnRecvClientPkt(char* buf, int len, struct sockaddr_in* addr)
{
    std::string ipstr = inet_ntoa(addr->sin_addr);
    std::string remote = ipstr + ":" + std::to_string(ntohs(addr->sin_port));

    if (Stun::IsStun((uint8_t*)buf, len))
    {
        auto stun = std::unique_ptr<Stun>(Stun::Parse((uint8_t*)buf, len));

        if (!stun)
        {
            //TODO LOG
            return;
        }

        Stun::Class type = stun->GetClass();
        Stun::Method method = stun->GetMethod();

        if (type == Stun::Class::REQUEST && method == Stun::Method::BINDING)
        {
            std::string username = stun->GetUsername();
            auto connit = SarsPeersUfragMaps.find(username);
            // not found the remote peer
            if (connit == SarsPeersUfragMaps.end())
            {
                coco_error("no remote peer username %s for candidate %s", username.c_str(), remote.c_str());
                return;
            }
            auto peer = connit->second;
            SarsPeersAddrMaps.emplace(remote, peer);
            coco_dbg("insert %s-%s into map",  username.c_str(), remote.c_str());
        }
    }
    // coco_dbg("get pkt from remote %s, len is %d",  remote.c_str(), len);
    auto it = SarsPeersAddrMaps.find(remote);
    if (it == SarsPeersAddrMaps.end())
    {
        // we do not have this remote peer
        coco_error("no remote peer candidate %s", remote.c_str());
        return;
    }

    auto rtcPeer = it->second;
    rtcPeer->remote_addr.addr_type   = webrtccore::kAddrIpv4;
    rtcPeer->remote_addr.ip          = ipstr;
    rtcPeer->remote_addr.port        = addr->sin_port;

    rtcPeer->UdpSocketRecv((uint8_t *)buf, len, it->second->remote_addr);
}

int RTCServer::SendPktToClient(char* buf, int len, struct sockaddr_in* addr)
{
    int ret = 0;
    ssize_t nwrite = 0;
    ret = l->sendto(buf, len, &nwrite, (struct sockaddr *)addr, sizeof(struct sockaddr));
    return ret;
}

int RTCServer::run()
{
    l = listen_udp(_lip, _lport);
    if (l == nullptr) {
        coco_error("create media listen socket failed");
        return -1;
    }
    
    return start();
}

int RTCServer::cycle()
{
    int len = 2048;
    int ret = 0;
    ssize_t nread = 0;
    struct sockaddr_in addr;
  
    while(true) {
        char *buf;
        buf = new char[2048];
        ret = l->recvfrom(buf, 2048, &nread, (struct sockaddr *)&addr, &len);
        if (ret != ERROR_SUCCESS) {
            break;
        }
        OnRecvClientPkt(buf, nread, &addr);
    }

    return ret;
}

int32_t RTCServer::CreateRtcPeer(std::shared_ptr<RtcPeer> peer, const std::string stream_id, std::string &offer, std::string &answer, bool hasLocal)
{
    std::shared_ptr<webrtccore::PeerConnectionFactoryInterface> peer_connection_factory =
        std::make_shared<webrtccore::PeerConnectionFactoryInterface>();

    webrtccore::PeerInitializeParamInterface *pc_init_param =
        peer_connection_factory->CreatePeerInitializeParam();
    pc_init_param->SetLogLevel(webrtccore::kWarning);
    pc_init_param->SetIceTimeOutMs(10000);
    pc_init_param->SetVideoMaxNackQueLen(8192);
    pc_init_param->SetAudioMaxNackQueLen(2048);
    pc_init_param->SetMaxNackWaitTime(2000);
    pc_init_param->SetMaxNackTimes(10);
    pc_init_param->AddAudioExtMap(1, "uurn:ietf:params:rtp-hdrext:ssrc-audio-level");
    pc_init_param->AddVideoExtMap(4, "urn:3gpp:video-orientation");
    std::vector<std::string> audio_feedback_types;
    std::map<std::string, std::string> audio_format_parameters;
    audio_feedback_types.push_back("nack");
    pc_init_param->AddAudioRtpMap("opus", 111, 2, 48000, audio_feedback_types,
                                  audio_format_parameters);
    std::vector<std::string> video_feedback_types;
    std::map<std::string, std::string> video_format_parameters;
    video_feedback_types.push_back("nack");
    video_feedback_types.push_back("nack pli");
    video_feedback_types.push_back("ccm fir");
    video_feedback_types.push_back("goog-remb");
    video_format_parameters["packetization-mode"] = "1";
    video_format_parameters["level-asymmetry-allowed"] = "1";
    video_format_parameters["profile-level-id"] = "42001f";
    uint32_t video_pt     = 102;
    // set true to skip use sdes instead of dtls-srtp, decrease at least 2 rtt to connect
    // need server support, default value is false
    pc_init_param->SetSDESFlag(false);
    pc_init_param->SetAVPFFlag(false);
    pc_init_param->SetDumpMask(0);
    pc_init_param->SetBitrateEtimatorFlag(false);
    pc_init_param->AddVideoRtpMap("H264", video_pt, 2, 90000, video_feedback_types,
                                  video_format_parameters);
#ifdef ___open_rtx_need_server_support__
    // open it reciever can  calc real net worker loss, need server support
    std::vector<std::string> video_rtx_feedback_types;
    std::map<std::string, std::string> video_rtx_format_parameters;
    uint32_t video_rtx_pt = 112;
    video_rtx_format_parameters["apt"] = std::to_string(video_pt);
    pc_init_param->AddVideoRtpMap("rtx", video_rtx_pt, 0, 90000, video_rtx_feedback_types,
                                  video_rtx_format_parameters);
#endif
    video_pt = 125;
    video_format_parameters["profile-level-id"] = "42e01f";
    pc_init_param->AddVideoRtpMap("H264", video_pt, 2, 90000, video_feedback_types,
                                  video_format_parameters);
#ifdef ___open_rtx_need_server_support__
    video_rtx_pt = 129;
    video_rtx_format_parameters["apt"] = std::to_string(video_pt);
    pc_init_param->AddVideoRtpMap("rtx", video_rtx_pt, 0, 90000, video_rtx_feedback_types,
                                  video_rtx_format_parameters);
#endif
    webrtccore::PeerConnectionInterface *pc = peer_connection_factory->CreatePeerConnection(peer.get(), 1,
                                                      stream_id,
                                                      pc_init_param);

    if (pc_init_param) {
        delete pc_init_param;
        pc_init_param = NULL;
    }

    if (NULL == pc) {
        coco_error("pc is null\r\n");
        return -1;
    }
    
    peer->SetPC(pc);

    coco_dbg("my addr %s:%d", _lOuterAddr.c_str(), _lport);
    coco_dbg("SetRemoteDescription: \r\n %s", offer.c_str());

    if (hasLocal) {
        peer->local_video_ssrc_ = rand();
        peer->local_audio_ssrc_ = rand();
        pc->AddLocalVideoTrack(peer->local_video_ssrc_, "MainVideo");
        pc->AddLocalAudioTrack(peer->local_audio_ssrc_, "MainAudio");
    }
   
    pc->SetRemoteDescription(offer);
    webrtccore::RTCOfferAnswerOptions options(1, 1, webrtccore::kAddrIpv4,
                                              _lOuterAddr,
                                              _lport,
                                              true,
                                              webrtccore::kPassive);
    /*get sdp*/
    pc->CreateAnswer(answer, options);

    std::string ufrag;
    if (getUfragFromSdp(offer, ufrag) != ERROR_SUCCESS) {
        coco_dbg("sdp must contain ufrag");
        return -1;
    }
    std::string lufrag;
    if (getUfragFromSdp(answer, lufrag) != ERROR_SUCCESS) {
        coco_dbg("sdp must contain ufrag");
        return -1;
    }

    peer->sender = this;
    peer->ufrag = lufrag + ":" + ufrag;

    // insert peer
    SarsPeersUfragMaps.emplace(peer->ufrag, peer);

    return 0;
}

std::shared_ptr<RtcPublisher> RTCServer::CreateRtcPublisher(const std::string stream_id, std::string &offer, std::string &answer)
{
    auto peer = std::make_shared<RtcPublisher>();
    CreateRtcPeer(peer, stream_id, offer, answer, true);
    return peer;
}

std::shared_ptr<RtcPlayer> RTCServer::CreateRtcPlayer(const std::string stream_id, std::string &offer, std::string &answer)
{
    auto peer = std::make_shared<RtcPlayer>();
    CreateRtcPeer(peer, stream_id, offer, answer, true); // has local
    return peer;
}

std::shared_ptr<StreamCache_> RTCServer::GetOrCreateStreamCache(std::string stream_id)
{
    auto it = SarsPeersStreamIDMaps.find(stream_id);
    if (it != SarsPeersStreamIDMaps.end())
    {
        return it->second;
    }

    auto scache = std::make_shared<StreamCache_>();
    SarsPeersStreamIDMaps.emplace(stream_id, scache);

    return scache;
}