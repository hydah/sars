// Copyright 2020 Tencent Inc.  All rights reserved.
//
// Author: qq@tencent.com (Emperor Penguin)

#ifndef _RTC_PEER_H_
#define _RTC_PEER_H_

#include "api/webrtc_core_interface.h"

//#define  ___open_rtx_need_server_support__
#define  ___do_not_use_config_file__
//#define USE_XML_CONIFG
//#define  ___DELAY_TEST_WITH_SEI__


#define LOG_BUFF_LEN (4096)

class MyPacket : public webrtccore::Packet {
 public:
    MyPacket(char *data, int32_t len) {
        data_ = data;
        len_ = len;
    }
    ~MyPacket() {
        if (data_) {
            delete []data_;
            data_ = NULL;
        }
    };
};

class MediaFrame: public webrtccore::MediaData {
 public:
    MediaFrame(webrtccore::MediaType media_type, char *data, int32_t len,
               uint64_t timestamp, uint32_t ssrc,
               uint32_t payload_type,
               webrtccore::VideoRotation rotate_angle,
               uint32_t audio_frame_len);
    virtual ~MediaFrame();
};

class ISender
{
public:
    ISender() {};
    ~ISender() {};
    virtual int32_t SendPktToClient(char* buf, int len, struct sockaddr_in* addr) = 0;
};

class RtcPeer : public webrtccore::PeerConnectionObserver {
public:
    RtcPeer();
    ~RtcPeer();
    webrtccore::PeerConnectionInterface *GetPC();
    void SetPC(webrtccore::PeerConnectionInterface *pc);
    void SetUdpFd(int32_t udp_fd);
    void SetDumpFile(FILE *dump_file);
    void SetWorkerId(int32_t worker_id);
    void UdpSocketSend(char *data, int32_t len, const webrtccore::NetAddr &addr);
    virtual void SendPktToClient(char* data, int32_t len, struct sockaddr_in* addr);
    void UdpSocketRecv(uint8_t *data, int32_t len, const webrtccore::NetAddr &remote_addr);
    void GetMediaStats();
    bool IsPeerTimeOut();
    void OnTimer();

public:
    /*call back*/
    virtual void OnIceConnectionChange(webrtccore::IceConnectionState new_state);
    virtual void OnConnectionChange(webrtccore::PeerConnectionState new_state);
    virtual void OnAddAudioTrack(uint32_t ssrc, const std::string &label,
                         const std::map<uint32_t, webrtccore::AudioCodeInfo>
                         &codec_info_map);  // codec_info_map map key is payload_type
    virtual void OnRemoveAudioTrack(uint32_t ssrc);
    virtual void OnAddVideoTrack(uint32_t ssrc, const std::string &label,
                         const std::map<uint32_t, webrtccore::VideoCodeInfo>
                         &codec_info_map);  // codec_info_map map key is payload_type
    virtual void OnAddLocalAudioTrack(uint32_t ssrc, const std::string &label,
                           const std::map<uint32_t, webrtccore::AudioCodeInfo> &codec_info_map);
    virtual void OnAddLocalVideoTrack(uint32_t ssrc, const std::string &label,
                              const std::map<uint32_t, webrtccore::VideoCodeInfo> &codec_info_map);
    virtual void OnRemoveVideoTrack(uint32_t ssrc);
    virtual void OnRecvMediaData(std::unique_ptr<webrtccore::MediaData> media_data);
    virtual void OnRecvChannelData(std::unique_ptr<webrtccore::Packet> data);
    virtual void OnSendDataToRemote(std::unique_ptr<webrtccore::Packet> data,
                            const webrtccore::NetAddr &addr);
    virtual void OnRequestIFrame(uint32_t ssrc);
    virtual void OnSendDumpData(char *dump_data, int32_t len);
    virtual void OnDeliverRtpPacket(webrtccore::MediaType type, std::shared_ptr<webrtccore::RtpPacket> packet);

 private:
    virtual void DoDecode(char *data, int32_t len);
    virtual void DoPlay(char *data, int32_t len);
    virtual void StartEncode();
    virtual void PreToDoPlay();

 public:
    int32_t peer_id_;
    int32_t peer_state;
    int32_t udp_fd_;
    bool is_ice_break_;
    bool is_peer_break_;
    webrtccore::PeerConnectionInterface *peer_connection_;
    FILE *fp_h264_;
    int32_t dump_h264_frame_num_;
    int32_t worker_id_;
    std::map<uint32_t, webrtccore::VideoCodeInfo> local_video_codec_info_map_;
    std::map<uint32_t, webrtccore::AudioCodeInfo> local_audio_codec_info_map_;
    std::map<uint32_t, webrtccore::VideoCodeInfo> remote_video_codec_info_map_;
    std::map<uint32_t, webrtccore::AudioCodeInfo> remote_audio_codec_info_map_;
    uint32_t remote_video_ssrc_;
    uint32_t remote_audio_ssrc_;
    uint32_t local_video_ssrc_;
    uint32_t local_audio_ssrc_;
    uint32_t local_video_payloadtype_;
    uint32_t local_audio_payloadtype_;
    char     log_out_buffer_[LOG_BUFF_LEN];
    webrtccore::PeerConnectionState peerconnection_state_;

public:
    std::vector<std::shared_ptr<MediaFrame>> media_list_;
    ISender* sender;

    int32_t udp_listen_port_base_ ;
    std::string local_outer_addr_;
    std::string ufrag;
    webrtccore::NetAddr remote_addr;

};


extern void GetStreamIdFromStreamUrl(const std::string stream_url, std::string& stream_id);
#endif  // _RTC_PEER_H