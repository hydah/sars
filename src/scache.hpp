#ifndef _STREAM_CACHE_
#define _STREAM_CACHE_

#include "rtc_publisher.hpp"
#include "rtc_player.hpp"
#include "api/webrtc_core_interface.h"
#include <vector>

class RtcPublisher;
class RtcPlayer;

class StreamCache_
{
public:
    StreamCache_();
    virtual ~StreamCache_();

public:
    bool CanPublish;
    std::shared_ptr<RtcPublisher> _publisher;
    std::vector<std::shared_ptr<RtcPlayer>> players;
    // std::shared_ptr<RtcIngester> ingester;
    // std::vector<std::shared_ptr<RtcForwarder>> forwarders;
    
public:
    int CacheMedia(std::unique_ptr<webrtccore::MeidaData> mediaData);
    void SetPublisher(std::shared_ptr<RtcPublisher> publisher);
    void AddPlayer(std::shared_ptr<RtcPlayer> player);
};

#endif // end of _STREAM_CACHE_