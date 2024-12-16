#include "otaupdata.h"
#define TIP_LED 2

OTAUpdate::OTAUpdate() {}

OTAUpdate::~OTAUpdate() {}

void OTAUpdate::updataBin(String updateURL) {
  Serial.println("Start Update!");
  pinMode(2, OUTPUT);

  httpUpdate.onStart(updataStart);
  httpUpdate.onEnd(updataEnd);
  httpUpdate.onProgress(updataProgress);
  httpUpdate.onError(updataError);

  t_httpUpdate_return ret = httpUpdate.update(UpdateClient, updateURL);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.println("[update] Update Failed.");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[update] Update No Update.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("[update] Update Success.");
      break;
  }
}

void OTAUpdate::updataEnd() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void OTAUpdate::updataError(int error) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", error);
}

void OTAUpdate::updataProgress(int progress, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n",
                progress, total);
  digitalWrite(TIP_LED, !digitalRead(TIP_LED));
}

void OTAUpdate::updataStart() {
  Serial.println("CALLBACK:  HTTP update process started");
}
