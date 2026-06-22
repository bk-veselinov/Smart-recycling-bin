# AI-Powered Smart Recycling Bin

An edge-AI recycling bin that uses a camera and an on-device Convolutional Neural Network (CNN) to classify waste items into recycling categories. Everything runs on a **Seeed Studio XIAO ESP32S3 Sense**. The model was trained with [Edge Impulse](https://edgeimpulse.com/) and runs locally, so no cloud service is needed.

You open a small web page served by the ESP32, press **Capture & Classify**, and the device lights the item with a NeoPixel ring, takes a photo, runs the CNN and shows the photo and predicted category in your browser.

---

## Features

- **On-device inference**: an Edge Impulse CNN (160×160 RGB input) runs directly on the ESP32-S3.
- **Web UI trigger**: the board connects to Wi-Fi and hosts an HTTP server on port 80. It has no physical button.
- **Consistent lighting**: a 16-LED WS2812B ring flashes bright white while the photo is taken.
- **Captured image preview**: the last JPEG is served back to the browser alongside the result.
- **Low-confidence fallback**: if the best prediction is below **60 %**, the item is reported as `Trash`.

### Model classes

| Label | Meaning |
|---|---|
| `background` | No item / empty bin view |
| `glass` | Glass bottles and jars |
| `paper_and_cardboard` | Paper, cardboard |
| `plastic_and_metal` | Plastic containers, cans |
| `trash` | Non-recyclable waste |

---

## Project Structure & File Locations

```
automated recycling bin/
├── README.md                          ← this file
├── sketch_jun22a.ino                  ← main Arduino firmware (flash this to the board)
├── cnn_arduino_package/
│   └── ei-kartof-project-1-arduino-1.0.9-impulse-#1.zip
│                                      ← Edge Impulse CNN model, exported as an Arduino library
├── docs/
│   ├── smart-recycling-bin-specs.md   ← project idea, hardware specs, system flow
│   └── wiring.md                      ← detailed wiring diagrams and pinouts
└── stand_stl_files/                   ← 3D-printable parts for the physical stand
    ├── real stand.stl                 ← main stand body
    ├── leg1.stl                       ← stand leg
    ├── leg2.stl                       ← stand leg
    ├── light bed.stl                  ← mount for the NeoPixel LED ring
    ├── Xiao Sense Case v48 - PETG - Left (Open) (1).stl   ← XIAO case, left half
    └── Xiao Sense Case v48 - PETG - Right (Open) (1).stl  ← XIAO case, right half
```

| What you need | Where to find it |
|---|---|
| Firmware source code | [sketch_jun22a.ino](sketch_jun22a.ino) |
| Trained CNN model (Arduino library) | [cnn_arduino_package/](cnn_arduino_package/) |
| Wiring / pinout | [docs/wiring.md](docs/wiring.md) |
| Hardware specs & system flow | [docs/smart-recycling-bin-specs.md](docs/smart-recycling-bin-specs.md) |
| 3D-print files for the stand and case | [stand_stl_files/](stand_stl_files/) |

---

## Hardware

| Component | Notes |
|---|---|
| Seeed Studio XIAO ESP32S3 Sense | With the camera expansion board; **OPI PSRAM** is required |
| OV2640 camera | On-board, no wiring needed |
| WS2812B NeoPixel ring (16 LEDs) | Lighting for image capture |
| 330 Ω resistor | On the NeoPixel data line (recommended) |
| 100–1000 µF capacitor | Across the ring's VCC/GND (recommended) |
| 3D-printed stand and case | See [stand_stl_files/](stand_stl_files/) |

### Wiring summary

| Component | XIAO pin |
|---|---|
| NeoPixel DIN | **D1 (GPIO 2)** through a 330 Ω resistor |
| NeoPixel VCC | 3.3 V (or 5 V / VUSB for full brightness) |
| NeoPixel GND | GND |

See [docs/wiring.md](docs/wiring.md) for full diagrams, camera pin mapping and power notes. At full white the ring can draw about 1 A, so power it from 5 V/VUSB if it looks dim or the board browns out.

---

## Software Setup

### 1. Arduino IDE and board support
1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add the ESP32 board package (Espressif `esp32`) through **Boards Manager**.
3. Install the **Adafruit NeoPixel** library through **Library Manager**.

### 2. Install the CNN model library
1. In the Arduino IDE go to **Sketch → Include Library → Add .ZIP Library…**
2. Select `cnn_arduino_package/ei-kartof-project-1-arduino-1.0.9-impulse-#1.zip`.
3. This installs the `kartof-project-1_inferencing` library that the sketch includes.

### 3. Configure Wi-Fi
Edit these lines near the top of [sketch_jun22a.ino](sketch_jun22a.ino):

```cpp
const char *WIFI_SSID = "your-network";
const char *WIFI_PASS = "your-password";
```

### 4. Board settings (Tools menu)

| Setting | Value |
|---|---|
| Board | `XIAO_ESP32S3` |
| PSRAM | `OPI PSRAM` (**required**) |
| Partition Scheme | `Huge APP (3MB No OTA / 1MB SPIFFS)` |

### 5. Upload and run
1. Upload the sketch and open the Serial Monitor at **115200 baud**.
2. Wait for `Ready — open http://<ip> in your browser.` The ring flashes green when the board is ready.
3. Open that IP address from a device on the same network and click **Capture & Classify**.

---

## How It Works

1. The board connects to Wi-Fi and starts a web server with three routes:
   - `/`: the web UI
   - `/capture`: runs the capture-and-classify pipeline and returns JSON `{success, label, confidence}`
   - `/image`: returns the last captured JPEG
2. On capture, the NeoPixel ring turns white and waits 200 ms for auto-exposure to settle.
3. The OV2640 grabs a 320×240 JPEG.
4. The JPEG is decoded to RGB888, centre-cropped to a square and resized to 160×160.
5. The Edge Impulse classifier runs on the frame, and the ring turns off.
6. The label with the highest confidence is returned. If it is below 60 %, the result is `Trash`.

---

## Roadmap

[docs/smart-recycling-bin-specs.md](docs/smart-recycling-bin-specs.md) describes features that are **not yet in the current firmware**:

- **SSD1306 OLED display** (128×32, I²C on GPIO 5/6) to show status and results on the device
- **ESP-NOW** to send the result to a second ESP32 node, for example one that drives a sorting mechanism

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `PSRAM alloc failed` | Set **Tools → PSRAM → OPI PSRAM** |
| Sketch too large to upload | Use the **Huge APP** partition scheme |
| Stuck on `Connecting to Wi-Fi....` | Check the SSID and password; the ESP32-S3 supports 2.4 GHz networks only |
| `Camera init failed` | Reseat the camera expansion board and check the ribbon cable |
| Ring dim or board resets on capture | Power the ring from 5 V / VUSB and add the capacitor |
| Poor accuracy | Use the 3D-printed stand so lighting and distance stay consistent |
