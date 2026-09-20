#pragma once

#include <Arduino.h>

struct OpenRFHardwareStatus {
  bool radio1Online = false;
  bool radio2Online = false;
  bool loraOnline = false;

  bool radio1Enabled = true;
  bool radio2Enabled = false;
  bool loraEnabled = false;

  bool radio1Active = false;
  bool radio2Active = false;
  bool loraActive = false;

  uint8_t radio1Part = 0xFF;
  uint8_t radio1Version = 0xFF;
  uint8_t radio2Part = 0xFF;
  uint8_t radio2Version = 0xFF;
  uint8_t loraVersion = 0x00;
};

// Must run before Radio.begin(): keeps both CC1101 CS lines HIGH so the
// second device cannot contend on the shared SPI bus during Radio 1 startup.
void hardwareStatusPrepareBus();

void hardwareStatusBegin();
OpenRFHardwareStatus hardwareStatusGet();
