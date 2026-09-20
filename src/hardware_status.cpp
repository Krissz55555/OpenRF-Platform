#include <Arduino.h>
#include <SPI.h>

#include "hardware.h"
#include "hardware_status.h"
#include "radio.h"
#include "config.h"

namespace {

OpenRFHardwareStatus status;

constexpr uint8_t CC1101_PARTNUM = 0x30;
constexpr uint8_t CC1101_VERSION = 0x31;
constexpr uint8_t CC1101_STATUS_READ = 0xC0;

constexpr uint8_t SX1276_REG_VERSION = 0x42;
constexpr uint8_t SX1276_EXPECTED_VERSION = 0x12;

uint8_t readCc1101StatusRegister(uint8_t csPin, uint8_t address) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(csPin, LOW);

  // CC1101 SPI transactions are valid only after MISO/SO goes LOW following
  // CS assertion. The old boot-only probe waited a fixed 2 us and could read
  // 0xFF from a perfectly working module, permanently labelling it OFFLINE.
  // Keep this probe bounded; runtime RadioLib initialization remains the
  // authoritative fallback proof that the chip is physically responding.
  const uint32_t startedUs = micros();
  while (digitalRead(OPENRF_CC1101_MISO_PIN) == HIGH &&
         static_cast<uint32_t>(micros() - startedUs) < 2000U) {
    delayMicroseconds(1);
  }

  if (digitalRead(OPENRF_CC1101_MISO_PIN) == HIGH) {
    digitalWrite(csPin, HIGH);
    SPI.endTransaction();
    return 0xFF;
  }

  SPI.transfer(address | CC1101_STATUS_READ);
  const uint8_t value = SPI.transfer(0x00);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
  return value;
}

// Minimal software SPI used only for boot-time SX1276 presence detection.
// It does not configure or enable the LoRa radio.
uint8_t loraTransferByte(uint8_t value) {
  uint8_t result = 0;

  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(OPENRF_LORA_SCK_PIN, LOW);
    digitalWrite(OPENRF_LORA_MOSI_PIN, (value >> bit) & 0x01);
    delayMicroseconds(1);

    digitalWrite(OPENRF_LORA_SCK_PIN, HIGH);
    result = static_cast<uint8_t>(
        (result << 1) | (digitalRead(OPENRF_LORA_MISO_PIN) ? 1 : 0));
    delayMicroseconds(1);
  }

  digitalWrite(OPENRF_LORA_SCK_PIN, LOW);
  return result;
}

uint8_t readSx1276Register(uint8_t address) {
  digitalWrite(OPENRF_LORA_CS_PIN, LOW);
  delayMicroseconds(2);

  loraTransferByte(address & 0x7F);  // bit 7 = 0 -> register read
  const uint8_t value = loraTransferByte(0x00);

  digitalWrite(OPENRF_LORA_CS_PIN, HIGH);
  return value;
}

}  // namespace

void hardwareStatusPrepareBus() {
  // Both CC1101 modules share MOSI/MISO/SCK. Their CS lines must be explicitly
  // deselected before either device is initialized; a floating second CS can
  // corrupt RadioLib transactions and leave Radio 1 in ERROR mode.
  pinMode(OPENRF_CC1101_CS_PIN, OUTPUT);
  digitalWrite(OPENRF_CC1101_CS_PIN, HIGH);

  pinMode(OPENRF_CC1101_2_CS_PIN, OUTPUT);
  digitalWrite(OPENRF_CC1101_2_CS_PIN, HIGH);

  SPI.begin(OPENRF_CC1101_SCK_PIN, OPENRF_CC1101_MISO_PIN,
            OPENRF_CC1101_MOSI_PIN, OPENRF_CC1101_CS_PIN);
}

void hardwareStatusBegin() {
  // The shared SPI bus and both CS lines were prepared before Radio.begin().
  // Probe hardware presence independently from whether a module is enabled.
  status.radio1Part =
      readCc1101StatusRegister(OPENRF_CC1101_CS_PIN, CC1101_PARTNUM);
  status.radio1Version =
      readCc1101StatusRegister(OPENRF_CC1101_CS_PIN, CC1101_VERSION);
  const bool radio1ProbeOnline =
      status.radio1Part == 0x00 &&
      status.radio1Version != 0x00 &&
      status.radio1Version != 0xFF;
  // A successful RadioLib initialization is also direct proof that the
  // CC1101 is present. This prevents a transient raw SPI probe miss from
  // overriding a radio that is demonstrably receiving real RF traffic.
  status.radio1Online =
      radio1ProbeOnline || Radio.getChannelDiagnostics(1).initialized;

  status.radio2Part =
      readCc1101StatusRegister(OPENRF_CC1101_2_CS_PIN, CC1101_PARTNUM);
  status.radio2Version =
      readCc1101StatusRegister(OPENRF_CC1101_2_CS_PIN, CC1101_VERSION);

  // Typical CC1101: PARTNUM = 0x00, VERSION = valid non-00/non-FF value.
  const bool radio2ProbeOnline =
      status.radio2Part == 0x00 &&
      status.radio2Version != 0x00 &&
      status.radio2Version != 0xFF;
  status.radio2Online =
      radio2ProbeOnline || Radio.getChannelDiagnostics(2).initialized;

  // Probe SX1276 RegVersion without enabling the radio.
  pinMode(OPENRF_LORA_CS_PIN, OUTPUT);
  digitalWrite(OPENRF_LORA_CS_PIN, HIGH);

  pinMode(OPENRF_LORA_SCK_PIN, OUTPUT);
  digitalWrite(OPENRF_LORA_SCK_PIN, LOW);

  pinMode(OPENRF_LORA_MOSI_PIN, OUTPUT);
  digitalWrite(OPENRF_LORA_MOSI_PIN, LOW);

  pinMode(OPENRF_LORA_MISO_PIN, INPUT);

  status.loraVersion = readSx1276Register(SX1276_REG_VERSION);
  status.loraOnline = status.loraVersion == SX1276_EXPECTED_VERSION;

  status.radio1Enabled = config.radio1Enabled;
  status.radio2Enabled = config.radio2Enabled;
  status.loraEnabled = config.loraEnabled;

  status.radio1Active = config.radio1Enabled && Radio.isRadioActive(1);
  status.radio2Active = config.radio2Enabled && Radio.isRadioActive(2);
  status.loraActive = false;

  Serial.println(F("=== OpenRF RF Hardware Detection ==="));

  Serial.print(F("CC1101 Radio 1: "));
  Serial.print(status.radio1Online ? F("ONLINE") : F("OFFLINE"));
  Serial.print(F(" (PARTNUM 0x"));
  if (status.radio1Part < 0x10) Serial.print('0');
  Serial.print(status.radio1Part, HEX);
  Serial.print(F(", VERSION 0x"));
  if (status.radio1Version < 0x10) Serial.print('0');
  Serial.print(status.radio1Version, HEX);
  Serial.print(F(", "));
  Serial.print(status.radio1Enabled ? F("ENABLED") : F("DISABLED"));
  Serial.println(')');

  Serial.print(F("CC1101 Radio 2: "));
  Serial.print(status.radio2Online ? F("ONLINE") : F("OFFLINE"));
  Serial.print(F(" (PARTNUM 0x"));
  if (status.radio2Part < 0x10) Serial.print('0');
  Serial.print(status.radio2Part, HEX);
  Serial.print(F(", VERSION 0x"));
  if (status.radio2Version < 0x10) Serial.print('0');
  Serial.print(status.radio2Version, HEX);
  Serial.println(')');

  Serial.print(F("SX1276 LoRa: "));
  Serial.print(status.loraOnline ? F("ONLINE") : F("OFFLINE"));
  Serial.print(F(" (RegVersion 0x"));
  if (status.loraVersion < 0x10) Serial.print('0');
  Serial.print(status.loraVersion, HEX);
  Serial.println(')');

  Serial.println(F("================================="));
}

OpenRFHardwareStatus hardwareStatusGet() {
  OpenRFHardwareStatus current = status;
  current.radio1Enabled = config.radio1Enabled;
  current.radio2Enabled = config.radio2Enabled;
  current.loraEnabled = config.loraEnabled;

  const RadioChannelDiagnostics radio1 = Radio.getChannelDiagnostics(1);
  const RadioChannelDiagnostics radio2 = Radio.getChannelDiagnostics(2);

  // "Online" means hardware presence, not merely the one-time boot probe.
  // If the runtime engine initialized a CC1101 successfully, the module is
  // physically responding even when a raw PARTNUM/VERSION probe was missed.
  current.radio1Online = current.radio1Online || radio1.initialized;
  current.radio2Online = current.radio2Online || radio2.initialized;

  current.radio1Active = config.radio1Enabled && radio1.receiving;
  current.radio2Active = config.radio2Enabled && radio2.receiving;
  current.loraActive = false;
  return current;
}
