# AI-Powered Smart Recycling Bin

## 1. Project Idea & Overview
The Smart Recycling Bin is an IoT edge-AI ecosystem designed to automatically categorize waste into distinct types—such as paper, glass, and plastic—to facilitate proper sorting and recycling. 

The core of the system is the **Seeed Studio XIAO ESP32S3 Sense**, leveraging its integrated OV2640 camera module. Using a Convolutional Neural Network (CNN) trained via Edge Impulse, the device captures an image of a discarded item, processes the visual data locally on the microcontroller, and determines the waste category in real time.

## 2. Hardware Specifications
* **Microcontroller:** Seeed Studio XIAO ESP32S3 Sense (with OPI PSRAM enabled for heavy image processing).
* **Camera:** On-board OV2640 (captures images for classification).
* **Display:** SSD1306 OLED (128x32, I²C) for local status and result display.
* **Lighting:** WS2812B NeoPixel 16-LED Ring (provides a bright white flash to ensure consistent lighting during image capture).
* **Communication:** * **Wi-Fi:** Hosts the Web UI for remote triggering.
    * **ESP-NOW:** Transmits classification results to a secondary ESP node.

## 3. Updated Wiring Reference
*(The physical tactile button previously on D0/GPIO1 has been removed in favor of a web-based trigger).*

| Component | Pin / Connection | Notes |
| :--- | :--- | :--- |
| **NeoPixel Ring** | **D1 (GPIO 2)** (DIN) | Connect to 3.3V or 5V and GND. A 330Ω resistor on DIN is recommended. |
| **SSD1306 OLED** | **GPIO 5** (SDA) | I²C Data line. |
| **SSD1306 OLED** | **GPIO 6** (SCL) | I²C Clock line. Power via 3.3V and GND. |
| **OV2640 Camera** | **On-board** | Built into the expansion board. |

## 4. System Flow (Digital Trigger & Web UI)
1.  **Initialization:** The XIAO ESP32S3 Sense initializes the camera, OLED, NeoPixel ring, and ESP-NOW protocol. It then connects to Wi-Fi to host a Web Server.
2.  **Web Interface:** A user navigates to the ESP32's IP address. The page displays a minimal interface with a "Capture" button.
3.  **Trigger Event:** Clicking the "Capture" button sends a request to the ESP32.
4.  **Hardware Action:** The NeoPixel ring flashes bright white, and the OV2640 captures a JPEG frame.
5.  **Processing:** The image is decoded, cropped, and passed through the Edge Impulse classification model.
6.  **Web Response:** The web server responds to the browser with the **captured image** and the **classification result**. The web page updates dynamically to display the photo and the predicted waste category.
7.  **Local & Remote Updates:** Simultaneously, the prediction is displayed on the local SSD1306 OLED and transmitted to the remote ESP32 node via ESP-NOW.

***

## Prompt for Code Implementation
**Use the following prompt to update the existing `file.c` codebase:**

> Please update my `file.c` code to replace the physical button trigger with a Web UI. 
> 
> **Requirements:**
> 1. Remove the physical button logic (`BUTTON_PIN` on GPIO 1) and any software debouncing.
> 2. Include Wi-Fi and Web Server capabilities (e.g., `WebServer` or `ESPAsyncWebServer`) to host a simple web page.
> 3. The web page should feature a single "Capture" button.
> 4. When the button is clicked via the web interface, execute the existing `camera_capture_and_classify()` sequence (turn on the NeoPixel ring, capture the image, run Edge Impulse inference, turn off the ring).
> 5. Send the classification result AND the captured JPEG image back to the web client so both are displayed dynamically on the page next to/below the button.
> 6. Retain the existing functionality that displays the classification result on the SSD1306 OLED.
> 7. Retain the ESP-NOW transmission logic so the label is still sent to the remote MAC address. 
> 8. Ensure the PSRAM camera allocation and ESP-NOW configurations remain intact.
