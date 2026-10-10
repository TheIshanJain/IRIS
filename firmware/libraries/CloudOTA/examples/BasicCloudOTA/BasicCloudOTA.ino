/*
 * CloudOTA minimal example.
 * Tools > Partition Scheme: any scheme with two app (OTA) slots.
 */
#include <WiFi.h>
#include <CloudOTA.h>

#define FW_VERSION   "1.0.0"
#define MANIFEST_URL "https://github.com/YOUR_USER/YOUR_REPO/releases/latest/download/manifest.json"

CloudOTA ota;

void setup() {
  Serial.begin(115200);
  WiFi.begin("YOUR_WIFI", "YOUR_PASSWORD");

  ota.begin(FW_VERSION, MANIFEST_URL);
  ota.setInsecure();                           // use setRootCA(pem) in production
  ota.setCheckInterval(60UL * 60UL * 1000UL);  // every hour
  ota.onProgress([](int pct) { Serial.printf("OTA %d%%\n", pct); });
  ota.onStateChange([](CloudOTA::State s, const String &msg) {
    Serial.printf("[%s] %s\n", CloudOTA::stateName(s), msg.c_str());
  });
}

void loop() {
  ota.loop();   // checks 10 s after Wi-Fi connects, then every hour

  // Type 'u' in the Serial Monitor to check right now
  if (Serial.available() && Serial.read() == 'u') ota.requestCheck();
}
