// Copyright 2020 Tencent Inc.  All rights reserved.
//
// Author: qq@tencent.com (Emperor Penguin)
#include "rtc_peer.hpp"
#include "coco/base/log.hpp"
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h>
#include <sstream>
#include <string.h>


#define  MAX_PEER_NUM     (10)
#define  MAX_RECV_SIZE    (8192)


int32_t CreateDirectory(const std::string &path) {
#define MAX_PATH_LEN 1024
    int32_t dir_path_len = path.length();

    if (dir_path_len > MAX_PATH_LEN) {
        return -1;
    }

    char tmp_dir_path[MAX_PATH_LEN] = { 0 };

    for (int32_t i = 0; i < dir_path_len; ++i) {
        tmp_dir_path[i] = path[i];

        if ((0 == i) && (path[0] == '/')) {
            continue;
        }

        if (tmp_dir_path[i] == '\\' || tmp_dir_path[i] == '/') {
            tmp_dir_path[i] = '\0';

            if (access(tmp_dir_path, 0) != 0) {
                int32_t ret = mkdir(tmp_dir_path, 0755);

                if (ret != 0) {
                    return ret;
                }
            }

            tmp_dir_path[i] = '/';
        }
    }

    return 0;
}

void FilePrintTime (uint64_t time_ms, char *time_fmt, size_t length)
{
    struct tm tm_time;
    time_t time_s = time_ms / 1000;
    localtime_r(&time_s, &tm_time);

    strftime (time_fmt, length, "%Y_%m_%d_%H_%M_%S", &tm_time);
    return;
}

uint64_t GetTimestampMs()
{
    timeval tm;
    gettimeofday (&tm, NULL);
    uint64_t time_ms = tm.tv_sec * 1000 + tm.tv_usec / 1000;
    return time_ms;
}

void PrintData(uint64_t time_ms, char *time_fmt, size_t length) {

    struct tm tm_time;
    time_t time_s = time_ms / 1000;
    localtime_r(&time_s, &tm_time);
    strftime(time_fmt, length, "%Y-%m-%d-%H", &tm_time);
    return;
}

FILE *WriteH264FileStart(std::string stream_id, int32_t file_id) {
    uint64_t time_ms = GetTimestampMs();
    char path[256]      = {'\0'};
    char video_name[256] = {'\0'};

    while (1)   {
        std::string::size_type  pos(0);

        if ((pos = stream_id.find("/")) != std::string::npos) {
            stream_id.replace(pos, 1, "_");
        }

        else {
            break;
        }
    }

    char time_fmt[64] = {'\0'};
    PrintData(time_ms, time_fmt, sizeof(time_fmt));
    snprintf(path, 255, "/data/WebrtcDump/%s/", time_fmt);
    CreateDirectory(path);
    time_fmt[0] = '\0';
    FilePrintTime(time_ms, time_fmt, sizeof(time_fmt));
    snprintf(video_name, 255, "%s%s-%s-worker_%d.264", path, time_fmt, stream_id.c_str(), file_id);
    FILE *out_h264_file = fopen(video_name, "wb");
    return out_h264_file;
}


void WriteH264FileStop(FILE *h264_file) {
    if (h264_file) {
        fclose(h264_file);
        h264_file = NULL;
    }
}

int32_t WriteFile(uint8_t* data_buffer,  uint32_t data_len, FILE *h264_file) {
    int32_t ret = 0;

    if (!data_buffer || !data_len || !h264_file) {
        return -1;
    }

    ret = fwrite(data_buffer, data_len, 1, h264_file);
    return ret;
}

void GetStreamIdFromStreamUrl(const std::string stream_url, std::string& stream_id)
{

    size_t findpos_endpos = stream_url.find("?");

    if (findpos_endpos != stream_url.npos) {
        findpos_endpos -= 1;
    }
    else {
        findpos_endpos = stream_url.length();
    }

    size_t findpos_start = stream_url.find_last_of("/", findpos_endpos);

    if ((findpos_start == stream_url.npos) || (findpos_start + 1 >= stream_url.length())) {
        exit(1);
    }

    findpos_start += 1;

    int32_t length = findpos_endpos - findpos_start + 1;

    stream_id = stream_url.substr(findpos_start, length);
    printf("stream_id:%s\r\n", stream_id.c_str());

    return;
}

MediaFrame::MediaFrame(webrtccore::MediaType media_type,
                       char *data,
                       int32_t len,
                       uint64_t timestamp,
                       uint32_t ssrc,
                       uint32_t payload_type,
                       webrtccore::VideoRotation rotate_angle,
                       uint32_t audio_frame_len) {

    media_type_         = media_type;
    len_                = len;
    timestamp_          = timestamp;
    ssrc_               = ssrc;
    rotate_angle_       = rotate_angle;
    payload_type_       = payload_type;
    audio_frame_len_    = audio_frame_len;

    if (0 < len) {
        data_ = new char[len];
        memcpy(data_, data, len);
    }
}

MediaFrame::~MediaFrame() {
    if (data_) {
        delete []data_;
    }
}


RtcPeer::RtcPeer() {
    is_ice_break_                   = false;
    is_peer_break_                  = false;
    fp_h264_                        = NULL;
    dump_h264_frame_num_            = 0;
    worker_id_                      = 0;
    remote_video_ssrc_              = 0;
    remote_audio_ssrc_              = 0;
    local_video_ssrc_               = 0;
    local_audio_ssrc_               = 0;
    local_video_payloadtype_        = 0;
    local_audio_payloadtype_        = 0;
    peer_connection_                = NULL;
    peerconnection_state_           = webrtccore::kNew;
    udp_fd_                         = -1;
}

RtcPeer::~RtcPeer() {
    if (peer_connection_) {
        delete peer_connection_;
        peer_connection_ = NULL;
    }

    if (fp_h264_) {
        WriteH264FileStop(fp_h264_);
        fp_h264_ = NULL;
    }
}


void RtcPeer::OnConnectionChange(webrtccore::PeerConnectionState new_state) {
    /*send do*/
    printf("worker_id[%d] %s:%d\r\n", worker_id_, __FUNCTION__, new_state);

    peerconnection_state_ = new_state;

    if (webrtccore::PeerConnectionState::kConnected == new_state) {
        StartEncode();
    }

    if (new_state == webrtccore::kDisconnected || new_state == webrtccore::kFailed ||
            new_state == webrtccore::kClosed) {
        is_peer_break_ = true;
    }
}

/*send do*/
void RtcPeer::OnAddLocalVideoTrack(uint32_t ssrc, const std::string &label,
                          const std::map<uint32_t, webrtccore::VideoCodeInfo> &codec_info_map) {

    printf("local worker_id[%d] add Video Track ssrc:%u\r\n", worker_id_, ssrc);
    auto itor = codec_info_map.begin();

    while (itor != codec_info_map.end()) {
        const webrtccore::VideoCodeInfo *video_codec_info = &(itor->second);
        printf("local worker_id[%d] payload_type:%d codec_type:%u profile_level_id:%s\r\n",
               worker_id_,
               video_codec_info->payload_type,
               video_codec_info->codec_type,
               video_codec_info->profile_level_id.c_str());

        if ((0 == local_video_payloadtype_) &&
                (webrtccore::kVideoCodeH264 == video_codec_info->codec_type)) {
            local_video_payloadtype_ = video_codec_info->payload_type;
        }

        itor++;
    }


    local_video_codec_info_map_ = codec_info_map;
    local_video_ssrc_           = ssrc;

}

/*send do*/
void RtcPeer::OnAddLocalAudioTrack(uint32_t ssrc, const std::string &label,
                          const std::map<uint32_t, webrtccore::AudioCodeInfo> &codec_info_map) {

    printf("worker_id[%d] add Audio Track ssrc:%u label:%s\r\n", worker_id_, ssrc, label.c_str());

    auto itor = codec_info_map.begin();

    while (itor != codec_info_map.end()) {
        const webrtccore::AudioCodeInfo *audio_codec_info = &(itor->second);
        printf("local worker_id[%d] payload_type:%d codec_type:%u sample_rate:%d channels:%d\r\n",
               worker_id_,
               audio_codec_info->payload_type,
               audio_codec_info->codec_type,
               audio_codec_info->sample_rate,
               audio_codec_info->channels);

        if ((0 == local_audio_payloadtype_) &&
                (webrtccore::kAudioCodecOpus == audio_codec_info->codec_type)) {
            local_audio_payloadtype_ = audio_codec_info->payload_type;
        }

        itor++;
    }

    local_audio_codec_info_map_ = codec_info_map;
    local_audio_ssrc_           = ssrc;

}



void RtcPeer::OnAddAudioTrack(uint32_t ssrc, const std::string &label,
                                         const std::map<uint32_t,webrtccore::AudioCodeInfo> &codec_info_map) {
    printf("local worker_id[%d] add Audio Track ssrc:%u label:%s\r\n",
           worker_id_, ssrc, label.c_str());
    auto itor = codec_info_map.begin();

    while (itor != codec_info_map.end()) {
        const webrtccore::AudioCodeInfo *audio_codec_info = &(itor->second);
        printf("remote worker_id[%d] payload_type:%d codec_type:%u sample_rate:%d channels:%d\r\n",
               worker_id_,
               audio_codec_info->payload_type,
               audio_codec_info->codec_type,
               audio_codec_info->sample_rate,
               audio_codec_info->channels);
        itor++;
    }

    remote_audio_codec_info_map_ = codec_info_map;
    remote_audio_ssrc_    = ssrc;
    PreToDoPlay();
}


/*recv do*/
void RtcPeer::OnAddVideoTrack(uint32_t ssrc, const std::string &label,
                                         const std::map<uint32_t, webrtccore::VideoCodeInfo> &codec_info_map) {
    printf("DownLoad worker_id[%d] add Video Track ssrc:%u\r\n", worker_id_, ssrc);
    auto itor = codec_info_map.begin();

    while (itor != codec_info_map.end()) {
        const webrtccore::VideoCodeInfo *video_codec_info = &(itor->second);
        printf("remote worker_id[%d] payload_type:%d codec_type:%u profile_level_id:%s\r\n", worker_id_,
               video_codec_info->payload_type,
               video_codec_info->codec_type,
               video_codec_info->profile_level_id.c_str());
        itor++;
    }

    remote_video_codec_info_map_ = codec_info_map;
    remote_video_ssrc_           = ssrc;
    PreToDoPlay();
}


/*recv do by ssrc*/
void RtcPeer::OnRecvMeidaData(std::unique_ptr<webrtccore::MeidaData> media_data) {
    //test file
    if (webrtccore::kMediaVideo == media_data->media_type_) {
        // auto itor = video_codec_info_map_.find(media_data->payload_type_);
        // if (itor  != video_codec_info_map_.end()) {
        //    printf("worker_id_[%d] payload_type_:%d codec_type:%d profile_level_id:%s\r\n",
        //        worker_id_,
        //        media_data->payload_type_,
        //        itor->second.codec_type,
        //        itor->second.profile_level_id.c_str());
        // }
#ifdef ___DELAY_TEST_WITH_SEI__
        DecodeH264RawData(media_data->data_, media_data->len_);
#endif

        if (fp_h264_) {
            WriteFile((uint8_t *)media_data->data_, media_data->len_, fp_h264_);
        }

        if ((fp_h264_) && (1200 <= dump_h264_frame_num_++)) {
            WriteH264FileStop(fp_h264_);
            fp_h264_ = NULL;
        }
    }
    else if (webrtccore::kMediaAudio == media_data->media_type_) {
        // auto itor = audio_codec_info_map_.find(media_data->payload_type_);
        // if (itor  != audio_codec_info_map_.end()) {
        //    printf("worker_id_[%d] payload_type_:%d codec_type:%d sample_rate:%d channels:%d\r\n",
        //        worker_id_,
        //        media_data->payload_type_,
        //        itor->second.codec_type,
        //        itor->second.sample_rate,
        //        itor->second.channels);
        // }
    }
}


void RtcPeer::OnIceConnectionChange(webrtccore::IceConnectionState new_state) {
    coco_info("worker_id_[%d] %d", worker_id_, new_state);

    if (new_state == webrtccore::kIceConnectionFailed ||
            new_state == webrtccore::kIceConnectionClosed) {
        is_ice_break_ = true;
    }
}

void RtcPeer::OnRemoveAudioTrack(uint32_t ssrc) {
}

void RtcPeer::OnRemoveVideoTrack(uint32_t ssrc) {
}

void RtcPeer::OnRecvChannelData(std::unique_ptr<webrtccore::Packet> data) {
}

void RtcPeer::OnRequestIFrame(uint32_t ssrc) {
    coco_dbg("*** will request Iframe for %u", ssrc);
}

void RtcPeer::DoDecode(char *data, int32_t len) {
}
void RtcPeer::DoPlay(char *data, int32_t len) {
}
void RtcPeer::StartEncode() {
}
void RtcPeer::PreToDoPlay() {
}

void RtcPeer::OnSendDataToRemote(std::unique_ptr<webrtccore::Packet> data,
                                          const webrtccore::NetAddr &addr) {
    UdpSocketSend(data->data_, data->len_, addr);
}


/*debug test func, dump decrypt rtp data*/
void RtcPeer::OnSendDumpData(char *dump_data, int32_t len) {
    // struct sockaddr_in remote_addr;
    // remote_addr.sin_family = AF_INET;
    // remote_addr.sin_port   = htons(10868);
    // inet_pton(AF_INET, "127.0.0.1", (void *) & (remote_addr.sin_addr));
    //
    // if (-1 != udp_fd_) {
    //    sendto(udp_fd_, dump_data, len, 0, (struct sockaddr *) &remote_addr, sizeof(remote_addr));
    // }

}

bool RtcPeer::IsPeerTimeOut() {
    if ((is_ice_break_ || is_peer_break_)) {
        return true;
    }

    return false;
}


webrtccore::PeerConnectionInterface *RtcPeer::GetPC() {
    return peer_connection_;
}

void RtcPeer::SetPC(webrtccore::PeerConnectionInterface *pc) {
    peer_connection_ = pc;
}


void RtcPeer::SetUdpFd(int32_t udp_fd) {
    udp_fd_ = udp_fd;
}

void RtcPeer::SetDumpFile(FILE *dump_file) {
    fp_h264_ = dump_file;
}

void RtcPeer::SetWorkerId(int32_t worker_id) {
    worker_id_ = worker_id;
}


void RtcPeer::UdpSocketRecv(uint8_t *data, int32_t len,
                                       const webrtccore::NetAddr &remote_addr) {
    std::unique_ptr<webrtccore::Packet> recv_pkg(new MyPacket((char *)data, len));
    // printf("%s,[%d] recv addr %s:%u\r\n", __FUNCTION__, len,
    //       remote_addr.ip.c_str(), ntohs(remote_addr.port));

    if (peer_connection_) {
        peer_connection_->ProcessRemoteData(std::move(recv_pkg), remote_addr);
    }
}


void RtcPeer::UdpSocketSend(char *data, int32_t len, const webrtccore::NetAddr &addr) {
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = addr.port;
    inet_pton(AF_INET, addr.ip.c_str(), (void *) & (remote_addr.sin_addr));

    // coco_dbg("sendpkt to client [%s-%d], len is %d", addr.ip.c_str(), ntohs(addr.port), len);
    SendPktToClient(data, len, &remote_addr);
}


void RtcPeer::SendPktToClient(char *data, int32_t len, struct sockaddr_in* addr)
{
    struct sockaddr_in _addr;
    _addr.sin_family = AF_INET;
    _addr.sin_port = remote_addr.port;
    inet_pton(AF_INET, remote_addr.ip.c_str(), (void *) & (_addr.sin_addr));
    sender->SendPktToClient(data, len, &_addr);
}


void RtcPeer::GetMediaStats() {
    int32_t out_len = 0;

    if (!peer_connection_) {
        return;
    }

    webrtccore::PeerConnectionStatus pc_status;
    int32_t ret = peer_connection_->GetPeerConnectionStatus(&pc_status);

    if (0 == ret) {
        out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                            " Stm   %s\r\n", pc_status.stream_id->c_str());
        out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                            " Net   rtt:%u bitrate:[send:%" PRIu64 "|recv:%" PRIu64 "]kpbs\r\n",
                            pc_status.rtt,
                            pc_status.send_bitrate_bps >> 10,
                            pc_status.recv_bitrate_bps >> 10);
    }

    if (local_video_ssrc_) {
        webrtccore::LocalVideoTrackStatus video_status;
        ret = peer_connection_->GetLocalVideoTrackStatus(local_video_ssrc_, &video_status);

        if (0 == ret) {
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                "--------Local Video---------\r\n");
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " media codec_type:%u ssrc:%u pt:%u profile_level_id:%s "
                                "lable:%s\r\n",
                                video_status.codec_info.codec_type,
                                video_status.ssrc,
                                video_status.codec_info.payload_type,
                                video_status.codec_info.profile_level_id.c_str(),
                                video_status.lable.c_str());
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " Net   rtt:%u bitrate:%" PRIu64 "->%" PRIu64 "kpbs\r\n",
                                video_status.rtt,
                                video_status.core_bitrate_bps >> 10,
                                video_status.real_bitrate_bps >> 10);
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " flow  recv:%u pop_pkg:%u\r\n",
                                video_status.recv_frame_num,
                                video_status.pop_pkg_num);
        }
    }

    if (local_audio_ssrc_) {
        webrtccore::LocalAudioTrackStatus audio_status;
        ret = peer_connection_->GetLocalAudioTrackStatus(local_audio_ssrc_, &audio_status);

        if (0 == ret) {
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                "--------Local Audio---------\r\n");
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " media codec_type:%u ssrc:%u pt:%u sample_rate:%uHZ"
                                " chn:%u lable:%s\r\n",
                                audio_status.codec_info.codec_type,
                                audio_status.ssrc,
                                audio_status.codec_info.payload_type,
                                audio_status.codec_info.sample_rate,
                                audio_status.codec_info.channels,
                                audio_status.lable.c_str());
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " Net   rtt:%u bitrate:%" PRIu64 "->%" PRIu64 "kpbs\r\n",
                                audio_status.rtt,
                                audio_status.core_bitrate_bps >> 10,
                                audio_status.real_bitrate_bps >> 10);
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " flow  recv:%u pop_pkg:%u\r\n",
                                audio_status.recv_frame_num,
                                audio_status.pop_pkg_num);
        }
    }

    if (remote_video_ssrc_) {
        webrtccore::RemoteVideoTrackStatus video_status;
        ret = peer_connection_->GetRemoteVideoTrackStatus(remote_video_ssrc_, &video_status);

        if (0 == ret) {
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                "--------Remote Video---------\r\n");
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " media codec_type:%u ssrc:%u pt:%u profile_level_id:%s"
                                " lable:%s\r\n",
                                video_status.codec_info.codec_type,
                                video_status.ssrc,
                                video_status.codec_info.payload_type,
                                video_status.codec_info.profile_level_id.c_str(),
                                video_status.lable.c_str());
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " Net   rtt:%u bitrate:%" PRIu64 "->%" PRIu64 "kpbs "
                                "loss:%.2f%%->%.2f%%\r\n",
                                video_status.rtt,
                                video_status.core_bitrate_bps >> 10,
                                video_status.real_bitrate_bps >> 10,
                                (double)video_status.loss_before_recover / 100,
                                (double)video_status.loss_after_recover / 100);
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " flow  recv:%u lost:%u pli:%u idr:%u frozen:%u pop_frame:%u\r\n",
                                video_status.recv_pkg_num,
                                video_status.lost_pkg_num,
                                video_status.pli_cnt,
                                video_status.recv_idr_num,
                                video_status.frozen_time,
                                video_status.pop_frame_num);
        }
    }

    if (remote_audio_ssrc_) {
        webrtccore::RemoteAudioTrackStatus audio_status;
        ret = peer_connection_->GetRemoteAudioTrackStatus(remote_audio_ssrc_, &audio_status);

        if (0 == ret) {
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                "--------Remote Audio---------\r\n");
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " media codec_type:%u ssrc:%u pt:%u sample_rate:%uHZ "
                                "chn:%u lable:%s\r\n",
                                audio_status.codec_info.codec_type,
                                audio_status.ssrc,
                                audio_status.codec_info.payload_type,
                                audio_status.codec_info.sample_rate,
                                audio_status.codec_info.channels,
                                audio_status.lable.c_str());
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " Net   rtt:%u bitrate:%" PRIu64 "->%" PRIu64 "kpbs "
                                "loss:%.2f%%->%.2f%%\r\n",
                                audio_status.rtt,
                                audio_status.core_bitrate_bps >> 10,
                                audio_status.real_bitrate_bps >> 10,
                                (double)audio_status.loss_before_recover / 100,
                                (double)audio_status.loss_after_recover / 100);
            out_len += snprintf((log_out_buffer_ + out_len), (LOG_BUFF_LEN - out_len),
                                " flow  recv:%u  lost:%u pop_frame:%u\r\n",
                                audio_status.recv_pkg_num,
                                audio_status.lost_pkg_num,
                                audio_status.pop_frame_num);
        }
    }

    if (out_len) {
        uint64_t time_ms = GetTimestampMs();
        char time_fmt[64] = {'\0'};
        FilePrintTime(time_ms, time_fmt, sizeof(time_fmt));
        printf("[%s]\r\n%s\r\n", time_fmt,log_out_buffer_);
    }

    return;
}


void RtcPeer::OnTimer() {
    auto now_time_ms_ = GetTimestampMs();
    if (peer_connection_) {
        peer_connection_->OnTime(now_time_ms_);
    }
}