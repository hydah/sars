#include "rtc_publisher.hpp"
#include "log/log.hpp"

RtcPublisher::RtcPublisher()
{

};

RtcPublisher::~RtcPublisher()
{

}

void RtcPublisher::OnRecvMediaData(std::unique_ptr<webrtccore::MediaData> media_data)
{
    // coco_dbg("get data, len is %d", media_data->len_);
    _scache->CacheMedia(std::move(media_data));
}

void RtcPublisher::RequestIFrame()
{
    uint32_t ssrc = remote_video_ssrc_;
    coco_info("request I Frame for ssrc: %u", ssrc);
    peer_connection_->RequestIFrame(ssrc);
}