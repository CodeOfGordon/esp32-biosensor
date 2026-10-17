# ESP32 Heart Rate + Tiny ML

An ESP32 reads your heart rate from a MAX30102 sensor and sends it over WiFi to a small FastAPI dashboard. A tiny model, trained on your own data, then runs on the ESP32 and predicts your BPM 10 seconds ahead.

## Hardware

ESP32 DevKit (`esp32dev`) + MAX30102 pulse sensor.

| MAX30102 | ESP32 |
|---|---|
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

## 1. Setup

You need [PlatformIO](https://platformio.org/) and Python 3.11+.

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

Create `include/secrets.h` (it's git-ignored, so it's not in the repo):

```cpp
#pragma once
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"
#define SERVER_URL "http://192.168.1.23:8000"  // your computer's local IP (macOS: ipconfig getifaddr en0)
```

The ESP32 only connects to **2.4 GHz** WiFi, on the same network as your computer.

## 2. Run the server and flash the ESP32

From the project folder:

```bash
.venv/bin/uvicorn server.app:app --host 0.0.0.0 --port 8000
pio run -t upload
pio device monitor
```

Open http://localhost:8000 and put your finger on the sensor. Once the monitor shows `Heart rate: XX BPM`, every reading (one a second) is saved to `server/bpm_log.csv`.

## 3. Record training data

1. Do ~30 jumping jacks.
2. Right away, put your finger on the sensor and hold still for **~35 seconds**.
3. Lift your finger for **10+ seconds** (that ends the recording).

Repeat 4–6 times.

## 4. Train and flash the model

```bash
.venv/bin/python ml/train.py   # writes include/model.h
pio run -t upload              # close the device monitor first
```

`train.py` prints how far off the model is, next to the lazy guess "same as now". The model is only useful if it beats that.

After flashing, the dashboard shows a dashed **Predicted** line.

Before you train, `include/model.h` is an all-zero placeholder that predicts 0 (no prediction).

**How long until you see predictions** (after your finger goes on):

| After | You see |
|---|---|
| ~5 s | the first heart rate (the ESP32 averages 5 beats) |
| ~15 s | "Predicted for 10 s from now" (the model needs 10 readings first) |
| ~25 s | the dashed line and its shaded band (each prediction is drawn at the moment it predicted) |
| ~35 s | the accuracy % (needs 10 predictions it can check) |

Lifting your finger resets this.

## Tests

```bash
.venv/bin/python server/test_app.py
```
