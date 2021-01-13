#include "coco/base/log.hpp"
#include "rtc_player.hpp"

RtcPlayer::RtcPlayer()
{

}

RtcPlayer::~RtcPlayer()
{

}

void RtcPlayer::FeedMeidaData(std::unique_ptr<webrtccore::MeidaData> media_data)
{
    peer_connection_->FeedMediaData(std::move(media_data));
    return;
}

void RtcPlayer::OnConnectionChange(webrtccore::PeerConnectionState new_state)
{
    peerconnection_state_ = new_state;

    if (new_state == webrtccore::kConnected) {
        _scache->_publisher->RequestIFrame();
    }
}

void RtcPlayer::OnRequestIFrame(uint32_t ssrc)
{
    _scache->_publisher->RequestIFrame();
}