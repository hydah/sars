#include "scache.hpp"
#include "coco/base/log.hpp"

StreamCache_::StreamCache_()
{
    CanPublish = true;
}

StreamCache_::~StreamCache_()
{
    
}

void StreamCache_::SetPublisher(std::shared_ptr<RtcPublisher> publisher)
{
    CanPublish = false;
    _publisher = publisher;
}

void StreamCache_::AddPlayer(std::shared_ptr<RtcPlayer> player)
{
    players.push_back(player);
}

int StreamCache_::CacheMedia(std::unique_ptr<webrtccore::MeidaData> mediaData)
{
    uint32_t   ssrc         = 0;
    uint32_t   payload_type = 0;
    for (auto it : players) {
        if (it->peerconnection_state_ != webrtccore::kConnected) {
            continue;
        }

        if (webrtccore::kMediaVideo == mediaData->media_type_) {
            ssrc         = it->local_video_ssrc_;
            payload_type = it->local_video_payloadtype_;
        } else if (webrtccore::kMediaAudio == mediaData->media_type_) {
            ssrc         = it->local_audio_ssrc_;
            payload_type = it->local_audio_payloadtype_;
        }

        std::unique_ptr<MediaFrame> raw_data(new MediaFrame(mediaData->media_type_,
                                                            mediaData->data_,
                                                            mediaData->len_,
                                                            mediaData->timestamp_,
                                                            ssrc,
                                                            payload_type,
                                                            mediaData->rotate_angle_,
                                                            mediaData->audio_frame_len_));
        it->FeedMeidaData(std::move(raw_data));
    }
    return 0;
}