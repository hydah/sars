#ifndef _RTC_PLAYER_
#define _RTC_PLAYER_

#include "rtc_peer.hpp"
#include "scache.hpp"
class StreamCache_;

class RtcPlayer : public RtcPeer
{
public:
    RtcPlayer();
    virtual ~RtcPlayer();

public:
    virtual void FeedMediaData(std::unique_ptr<webrtccore::MediaData> media_data);
    void SetStreamCache(std::shared_ptr<StreamCache_> scache) { _scache = scache; };
    virtual void OnConnectionChange(webrtccore::PeerConnectionState new_state);
    virtual void OnRequestIFrame(uint32_t ssrc);

public:
    std::shared_ptr<StreamCache_> _scache;
};

#endif // end of _RTC_PLAYER_