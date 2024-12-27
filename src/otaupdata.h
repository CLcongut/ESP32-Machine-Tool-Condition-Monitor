#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <httpUpdate.h>
#include "mtcm_pins.h"

class OTAUpdate {
 private:
  WiFiClient UpdateClient;
  static void updataEnd();
  static void updataError(int error);
  static void updataProgress(int progress, int total);
  static void updataStart();

  int _stateLed;

 public:
  OTAUpdate();
  ~OTAUpdate();
  void updataBin(String updateURL);
  void setStateLed(const int led);
};