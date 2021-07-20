
#include "common/error.hpp"
#include "log/log.hpp"
#include "net/layer7/coco_http.hpp"
#include "coco_api.h"
#include "rtc_server.hpp"
#include "signalling.hpp"
#include <iostream>
#include <map>

using namespace std;

int main() {
  int ret = COCO_SUCCESS;
  log_level = log_dbg;
  CocoInit();

  // rtc media server
  auto rtcServer = make_shared<RTCServer>("0.0.0.0", 18000, "49.232.131.173");
  if ((ret = rtcServer->run()) != COCO_SUCCESS) {
    coco_error("run rtc media server error: %d, terminate", ret);
    return ret;
  }

  // rtc signalling server
  auto signalling = std::unique_ptr<WebRTCSignalling>(
      new WebRTCSignalling("0.0.0.0", 18080, rtcServer));
  if ((ret = signalling->run()) != COCO_SUCCESS) {
    coco_error("run signalling server error: %d, terminate", ret);
    return ret;
  }

  uint32_t dur = 1000 * 1000;
  while (true) {
    coco_info("sleep %u us", dur);
    for (auto it : rtcServer->SarsPeersUfragMaps) {
      it.second->OnTimer();
    }
    st_usleep(dur);
  }

  return 0;
}