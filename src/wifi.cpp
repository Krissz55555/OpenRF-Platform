#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "openrf_wifi.h"

namespace {
const char* AP_SSID = "OpenRF-Platform";
wl_status_t lastStatus = WL_IDLE_STATUS;
bool setupApRunning = false;
uint32_t stationConnectStartedMs = 0;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

void startSetupAccessPoint() {
  if (setupApRunning) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  setupApRunning = true;
  Serial.print(F("Setup AP started: "));
  Serial.print(AP_SSID);
  Serial.print(F(" at "));
  Serial.println(WiFi.softAPIP());
}

void stopSetupAccessPoint() {
  // Do not rely only on our bookkeeping flag. After configuration changes or
  // unusual WiFi transitions the SDK can still have SoftAP enabled while the
  // flag is stale. Once STA is connected, force SoftAP off.
  const WiFiMode_t modeBefore = WiFi.getMode();
  if (setupApRunning || modeBefore == WIFI_AP || modeBefore == WIFI_AP_STA) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    Serial.println(F("Setup AP disabled after successful WiFi connection"));
  }
  setupApRunning = false;
}
}

void wifiBegin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(config.hostname.c_str());

  if (config.wifiSsid.length() == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    setupApRunning = true;
    Serial.print(F("AP SSID: "));
    Serial.println(AP_SSID);
    Serial.print(F("AP IP: "));
    Serial.println(WiFi.softAPIP());
    lastStatus = WiFi.status();
    return;
  }

  startSetupAccessPoint();
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  stationConnectStartedMs = millis();
  Serial.print(F("WiFi STA connecting to: "));
  Serial.println(config.wifiSsid);
  lastStatus = WiFi.status();
}

void wifiLoop() {
  const wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    if (lastStatus != WL_CONNECTED) {
      Serial.print(F("WiFi STA connected, IP: "));
      Serial.println(WiFi.localIP());
    }
    stopSetupAccessPoint();
  } else if (config.wifiSsid.length() > 0 && setupApRunning &&
             millis() - stationConnectStartedMs > WIFI_CONNECT_TIMEOUT_MS) {
    // Keep the setup AP available when the configured network cannot be reached.
    if (lastStatus != status) {
      Serial.print(F("WiFi STA not connected, setup AP remains available. Status: "));
      Serial.println(static_cast<int>(status));
    }
  }

  lastStatus = status;
}

bool wifiStationConnected() { return WiFi.status() == WL_CONNECTED; }

const char* wifiModeName() {
  if (wifiStationConnected()) return "STA";
  return setupApRunning ? "AP" : "STA_CONNECTING";
}

String wifiIpAddress() {
  if (wifiStationConnected()) return WiFi.localIP().toString();
  return WiFi.softAPIP().toString();
}
