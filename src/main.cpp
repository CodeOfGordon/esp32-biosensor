#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "model.h"

#define SDA_PIN 21
#define SCL_PIN 22

MAX30105 sensor;

const byte RATE_SIZE = 5;  
byte rates[RATE_SIZE]; 
byte rateIndex = 0;
byte validBeats = 0;
long lastBeat = 0;
float currentBPM = 0;
int averageBPM = 0;

unsigned long lastPrint = 0;
const float MIN_BPM = 45;
const float MAX_BPM = 180;

void postBPM(int bpm, float prediction) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  JsonDocument doc;
  http.begin(String(SERVER_URL) + "/bpm");
  http.setConnectTimeout(300);
  http.setTimeout(300);
  http.addHeader("Content-Type", "application/json");

  doc["bpm"] = bpm;
  doc["prediction"] = prediction;
  String jsonRequestStr;
  serializeJson(doc, jsonRequestStr);
  int code = http.POST(jsonRequestStr);
  if (code != 200) {
    Serial.printf("POST failed: %d\n", code);
  }
  http.end();
}

float window[MODEL_WINDOW];
int readingsInWindow = 0;

float predict(const float* readings) {
  float now = readings[MODEL_WINDOW - 1];
  float change = readings[MODEL_WINDOW - 1] - readings[0];
  return MODEL_B + MODEL_W[0] * now + MODEL_W[1] * change;
}


void setup() {
  Serial.begin(115200);
  Serial.printf("MAX30102 Heart Rate Sensor\n[Place your finger on the sensor]\n");
  Wire.begin(SDA_PIN, SCL_PIN);
  while (!sensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found, check wiring...");
    delay(1000);
  }
  sensor.setup(); 
  sensor.setPulseAmplitudeRed(0);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi OK, ESP32 IP: %s\n", WiFi.localIP().toString().c_str());
}

void loop() {

  // read BPM rate
  long irValue = sensor.getIR();
  if (irValue < 50000) {
    validBeats = 0;
    averageBPM = 0;
    readingsInWindow = 0;
  }
  if (checkForBeat(irValue)) {
    long timeBetweenBeats = millis() - lastBeat;
    lastBeat = millis();
    currentBPM = 60.0 / (timeBetweenBeats / 1000.0);
    Serial.printf("beat: %.0f BPM\n", currentBPM);  // debug: remove once BPM shows up

    // filter real BPM
    if (MIN_BPM <= currentBPM && currentBPM <= MAX_BPM) {
      rates[rateIndex] = (byte)currentBPM;
      rateIndex++;
      if (rateIndex>=RATE_SIZE) {
        rateIndex = 0;
      }
      if (validBeats < RATE_SIZE) {
        validBeats++;
      }
    }

    // avg the BPM (window=5 for smoothing)
    if (validBeats >= RATE_SIZE) {
      averageBPM = 0;
      for (byte i=0; i<RATE_SIZE; i++) {
        averageBPM += rates[i];
      }
      averageBPM /= RATE_SIZE;
    }
  }

  // To User
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    if (irValue<50000) {
      Serial.printf("Waiting for finger... (IR=%ld)\n", irValue);  // debug: IR value
    } else if (averageBPM == 0) {
      Serial.println("[Measuring Finger In Progress]");
    } else {
      Serial.printf("Heart rate: %d BPM\n", averageBPM);

      for (int i = 0; i < MODEL_WINDOW - 1; i++) {
        window[i] = window[i + 1];
      }
      window[MODEL_WINDOW - 1] = averageBPM;
      if (readingsInWindow < MODEL_WINDOW) {
        readingsInWindow++;
      }

      float prediction = 0;
      if (readingsInWindow == MODEL_WINDOW) {
        prediction = predict(window);
      }
      postBPM(averageBPM, prediction);
    }
  }
}
