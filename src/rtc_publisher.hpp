#ifndef _RTC_PUBLISHER_
#define _RTC_PUBLISHER_

#include "rtc_peer.hpp"
#include "scache.hpp"
class StreamCache_;

class RtcPublisher : public RtcPeer
{
public:
    RtcPublisher();
    virtual ~RtcPublisher();

public:
    virtual void OnRecvMediaData(std::unique_ptr<webrtccore::MediaData> media_data);
    void SetStreamCache(std::shared_ptr<StreamCache_> scache) { _scache = scache; };
public:
    std::shared_ptr<StreamCache_> _scache;
    void RequestIFrame();
};
#endif // end of _RTC_PUBLISHER_