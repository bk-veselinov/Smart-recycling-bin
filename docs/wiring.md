# Wiring Reference — XIAO ESP32S3 Sense + Peripherals

## Board: Seeed Studio XIAO ESP32S3 Sense

---

## Project Description

The **AI-Powered Smart Recycling Bin** is an IoT edge-AI system that automatically classifies waste into distinct categories — such as paper, glass, and plastic — to facilitate proper sorting and recycling.

The core of the system is the **Seeed Studio XIAO ESP32S3 Sense**, which leverages its integrated **OV2640 camera module** to capture images of discarded items. A **Convolutional Neural Network (CNN)**, trained via Edge Impulse and deployed locally on the microcontroller, processes the visual data and determines the waste category in real time.

### How It Works (Web UI Trigger)

1. The XIAO ESP32S3 Sense connects to a Wi-Fi network and hosts an HTTP web server.
2. A user navigates to the ESP32's IP address in any browser on the same network.
3. The web page displays a **"Capture"** button and a result area.
4. When the button is clicked, the device lights up the **NeoPixel LED ring** (bright white flash for consistent lighting), captures a JPEG frame via the **OV2640 camera**, and runs the Edge Impulse CNN classifier on-device.
5. The **captured image** and **classification result** (label + confidence) are sent back to the browser and displayed dynamically on the page.
6. Simultaneously, the prediction is shown on the local **SSD1306 OLED display** and transmitted to a remote ESP32 node via **ESP-NOW**.

> **Note:** The physical tactile button previously on D0 (GPIO 1) has been removed in favour of the web-based trigger described above.

---

## 1. NeoPixel WS2812B LED Ring (16 LEDs)

| Ring Pad           | ESP32S3 Pin        | Notes                            |
|--------------------|--------------------|----------------------------------|
| **DIN** (Data In)  | **D1 (GPIO 2)**    | Data signal line                 |
| **VCC** / **5V**   | **3.3V** or **5V** | Power supply (3.3V works for short rings) |
| **GND**            | **GND**            | Ground                           |

> **Tip**: For best reliability with 16 LEDs, place a **300–470Ω resistor** between GPIO2 and DIN.  
> A **100–1000µF capacitor** across VCC/GND near the ring protects against power surges.

```
 ESP32S3               NeoPixel Ring
 ───────               ─────────────
 GPIO2  ──[330Ω]────── DIN
 3.3V   ────────────── VCC
 GND    ──┬─────────── GND
             │
             [100µF cap]  (optional but recommended)
             │
             GND
```

---

## 2. SSD1306 OLED Display (128×32, I²C)

| OLED Pin | ESP32S3 Pin | Notes |
|----------|-------------|-------|
| **SDA** | **GPIO 5**  | I²C data line |
| **SCL** | **GPIO 6**  | I²C clock line |
| **VCC** | **3.3V**    | Power supply |
| **GND** | **GND**     | Ground |

> **I²C Address**: `0x3C`  
> No external pull-up resistors needed — the OLED module has them built in.

```
 ESP32S3               SSD1306 OLED
 ───────               ────────────
 GPIO5  ────────────── SDA
 GPIO6  ────────────── SCL
 3.3V   ────────────── VCC
 GND    ────────────── GND
```

---

## 3. OV2640 Camera (On-Board — No Wiring Needed)

The camera is built into the XIAO ESP32S3 Sense expansion board. These pins are fixed:

| Function   | GPIO | Function   | GPIO |
|------------|------|------------|------|
| XCLK       | 10   | D0 (Y2)    | 15   |
| SIOD (SDA) | 40   | D1 (Y3)    | 17   |
| SIOC (SCL) | 39   | D2 (Y4)    | 18   |
| D7 (Y9)    | 48   | D3 (Y5)    | 16   |
| D6 (Y8)    | 11   | D4 (Y6)    | 14   |
| D5 (Y7)    | 12   | VSYNC      | 38   |
| PCLK       | 13   | HREF       | 47   |
| PWDN       | N/C  | RESET      | N/C  |

---

## 4. Wi-Fi & Web Server (No Wiring — Software Only)

The XIAO ESP32S3 has a built-in Wi-Fi radio. No additional hardware is required.

| Feature         | Details                                                   |
|-----------------|-----------------------------------------------------------|
| **Mode**        | Wi-Fi STA (Station) — connects to an existing network     |
| **Web Server**  | HTTP on port 80 — serves the capture UI and image results |
| **ESP-NOW**     | Runs alongside Wi-Fi STA on the same radio                |

> **Setup**: Update `WIFI_SSID` and `WIFI_PASS` in `file.c` with your network credentials.  
> After boot, the device's IP address is displayed on the OLED and printed to the serial monitor.

---

## Complete Wiring Summary

```
          XIAO ESP32S3 Sense
        ┌──────────────────────┐
        │                      │
  NEO ──┤ D1  (GPIO 2)         │
        │                      │
  SDA ──┤ D3  (GPIO 5)         │── SSD1306 OLED
  SCL ──┤ D4  (GPIO 6)         │
        │                      │
  3.3V ─┤ 3V3            GND   ├── Common Ground
        │                      │
        │   [OV2640 Camera     │
        │    on-board, fixed]  │
        │                      │
        │   [Wi-Fi / Web UI    │
        │    built-in radio]   │
        └──────────────────────┘
```

### Power Notes
- All peripherals run on **3.3V** from the XIAO's 3V3 pin.
- If the LED ring is too dim at 3.3V, you can power it from **5V** (VUSB pin) instead — the WS2812B data line still accepts 3.3V logic from GPIO2.
- Total current draw at full white (16 LEDs × ~60mA) = ~960mA — only sustained briefly as a flash, but use 5V/VUSB if powering from USB.
