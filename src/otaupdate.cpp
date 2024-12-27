#include "otaupdata.h"

static int otaled;

OTAUpdate::OTAUpdate() {}

OTAUpdate::~OTAUpdate() {}

void OTAUpdate::updataBin(String updateURL) {
  Serial.println("Start Update!");

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
  digitalWrite(otaled, LOW);
}

void OTAUpdate::updataError(int error) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", error);
}

void OTAUpdate::updataProgress(int progress, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n",
                progress, total);
  digitalWrite(otaled, !digitalRead(otaled));
}

void OTAUpdate::updataStart() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void OTAUpdate::setStateLed(const int led) {
  otaled = led;
  pinMode(otaled, OUTPUT);
}
