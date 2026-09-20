#pragma once

// OpenRF Platform ESP32-S3 RF hardware reference wiring.
//
// CC1101 Radio 1 (433 MHz) and Radio 2 (868 MHz) share SPI:
//   SCK  -> GPIO12
//   MISO -> GPIO13
//   MOSI -> GPIO11
//
// Radio 1:
//   CSN  -> GPIO10
//   GDO0 -> GPIO4
//   GDO2 -> GPIO5
//
// Radio 2:
//   CSN  -> GPIO9
//   GDO0 -> GPIO6
//   GDO2 -> GPIO7
//
// SX1276 LoRa uses dedicated pins:
//   SCK  -> GPIO14
//   MOSI -> GPIO15
//   MISO -> GPIO16
//   NSS  -> GPIO17
//   RST  -> GPIO18
//   DIO0 -> GPIO21
//   DIO1 -> GPIO2
//
// All RF modules use 3.3 V logic and supply. Never connect VCC to 5 V.

#define OPENRF_CC1101_SCK_PIN  12
#define OPENRF_CC1101_MISO_PIN 13
#define OPENRF_CC1101_MOSI_PIN 11
#define OPENRF_CC1101_CS_PIN   10
#define OPENRF_CC1101_GDO0_PIN 4
#define OPENRF_CC1101_GDO2_PIN 5

// CC1101 Radio 2 (868 MHz)
#define OPENRF_CC1101_2_CS_PIN    9
#define OPENRF_CC1101_2_GDO0_PIN  6
#define OPENRF_CC1101_2_GDO2_PIN  7

// SX1276 LoRa
#define OPENRF_LORA_SCK_PIN      14
#define OPENRF_LORA_MOSI_PIN     15
#define OPENRF_LORA_MISO_PIN     16
#define OPENRF_LORA_CS_PIN       17
#define OPENRF_LORA_RST_PIN      18
#define OPENRF_LORA_DIO0_PIN     21
#define OPENRF_LORA_DIO1_PIN      2

// Onboard addressable RGB LED.
// ESP32-S3-DevKitC-1 initial boards use GPIO48; v1.1 uses GPIO38.
// Both pins are currently otherwise unused by OpenRF, so the startup sequence
// is sent to both for board-revision compatibility.
#define OPENRF_RGB_LED_PIN_V10 48
#define OPENRF_RGB_LED_PIN_V11 38

#define OPENRF_RADIO_FREQUENCY_MHZ 433.92F
#define OPENRF_RADIO_BIT_RATE_KBPS 4.8F
#define OPENRF_RADIO_FREQUENCY_DEVIATION_KHZ 5.0F
#define OPENRF_RADIO_RX_BANDWIDTH_KHZ 325.0F
#define OPENRF_RADIO_OUTPUT_POWER_DBM 10
#define OPENRF_RADIO_PREAMBLE_BITS 16
