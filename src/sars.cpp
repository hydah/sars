#include "coco/net/http_stack.hpp"
#include "coco/base/log.hpp"
#include "coco/base/st_socket.hpp"
#include "coco/base/error.hpp"
#include "coco/net/net.hpp"
#include "signalling.hpp"
#include "rtc_server.hpp"
#include <iostream>
#include <map>

using namespace std;

int main()
{
    int ret = ERROR_SUCCESS;
    log_level = log_dbg;
    coco_st_init();

    // rtc media server
    auto rtcServer = make_shared<RTCServer>("0.0.0.0", 8000, "9.134.218.127");
    if ((ret = rtcServer->run()) != ERROR_SUCCESS) {
        coco_error("run rtc media server error: %d, terminate", ret);
        return ret;
    }

    // rtc signalling server
    auto signalling = std::unique_ptr<WebRTCSignalling>(new WebRTCSignalling("0.0.0.0", 8080, rtcServer));
    if ((ret = signalling->run()) != ERROR_SUCCESS) {
        coco_error("run signalling server error: %d, terminate", ret);
        return ret;
    }


    uint32_t dur = 1000*1000;
    while(true) {
        coco_info("sleep %u us", dur);
        for (auto it : rtcServer->SarsPeersUfragMaps) {
            it.second->OnTimer();
        }
        st_usleep(dur);
    }

    return 0;
}