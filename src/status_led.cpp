#include <Arduino.h>
#include <esp32-hal-rgb-led.h>

#include "hardware.h"
#include "status_led.h"

namespace {

void writeBootRgb(uint8_t red, uint8_t green, uint8_t blue) {
  // Support both ESP32-S3-DevKitC-1 RGB LED board revisions.
  neopixelWrite(OPENRF_RGB_LED_PIN_V10, red, green, blue);
  neopixelWrite(OPENRF_RGB_LED_PIN_V11, red, green, blue);
}

void ledOff() {
  writeBootRgb(0, 0, 0);
}

}  // namespace

void statusLedBootSequence() {
  // Five red flashes, one flash every 500 ms:
  // 250 ms ON + 250 ms OFF = 0.5 s cadence.
  for (uint8_t i = 0; i < 5; ++i) {
    writeBootRgb(255, 0, 0);
    delay(250);
    ledOff();
    delay(250);
  }

  // Boot sequence complete: one second green, then LED off.
  writeBootRgb(0, 255, 0);
  delay(1000);
  ledOff();
}
