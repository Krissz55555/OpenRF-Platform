#pragma once

// OpenRF Platform ESP32-S3 1:1 port reference wiring.
// This first port uses one CC1101 only; Radio 2 and LoRa are intentionally
// not enabled yet. The existing v1.2.0 behavior is preserved.
//
// CC1101 VCC  -> external regulated 3V3 (never 5 V)
// CC1101 GND  -> GND (common with ESP32-S3)
// CC1101 SCK  -> GPIO12
// CC1101 MISO -> GPIO13
// CC1101 MOSI -> GPIO11
// CC1101 CSN  -> GPIO10
// CC1101 GDO0 -> GPIO4
// CC1101 GDO2 -> GPIO5

#define OPENRF_CC1101_SCK_PIN  12
#define OPENRF_CC1101_MISO_PIN 13
#define OPENRF_CC1101_MOSI_PIN 11
#define OPENRF_CC1101_CS_PIN   10
#define OPENRF_CC1101_GDO0_PIN 4
#define OPENRF_CC1101_GDO2_PIN 5

#define OPENRF_RADIO_FREQUENCY_MHZ 433.92F
#define OPENRF_RADIO_BIT_RATE_KBPS 4.8F
#define OPENRF_RADIO_FREQUENCY_DEVIATION_KHZ 5.0F
#define OPENRF_RADIO_RX_BANDWIDTH_KHZ 325.0F
#define OPENRF_RADIO_OUTPUT_POWER_DBM 10
#define OPENRF_RADIO_PREAMBLE_BITS 16
