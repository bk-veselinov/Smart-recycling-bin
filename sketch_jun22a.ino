// Smart Recycling Bin - XIAO ESP32S3 Sense
// Arduino IDE: Board XIAO_ESP32S3, PSRAM OPI PSRAM, Partition Huge APP

#include <kartof-project-1_inferencing.h>
#include "esp_camera.h"

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASS = "YOUR_WIFI_PASSWORD";

#define IMG_WIDTH   EI_CLASSIFIER_INPUT_WIDTH
#define IMG_HEIGHT  EI_CLASSIFIER_INPUT_HEIGHT

#define CONFIDENCE_THRESHOLD  0.60f

// Camera pins
#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   10
#define SIOD_GPIO_NUM   40
#define SIOC_GPIO_NUM   39

#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15

#define VSYNC_GPIO_NUM  38
#define HREF_GPIO_NUM   47
#define PCLK_GPIO_NUM   13

// LED ring
#define NEOPIXEL_PIN      2
#define NEOPIXEL_COUNT    16
#define RING_BRIGHTNESS   200

static uint8_t *snapshot_buf  = nullptr;
static uint8_t *last_jpeg_buf = nullptr;
static size_t   last_jpeg_len = 0;

Adafruit_NeoPixel ring(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer         server(80);

typedef struct {
    bool        success;
    const char *label;
    float       confidence;
} classify_result_t;

static bool              camera_init(void);
static classify_result_t camera_capture_and_classify(void);
static int               raw_feature_get_data(size_t offset, size_t length, float *out);
static void              ring_on(void);
static void              ring_off(void);
static void              handleRoot(void);
static void              handleCapture(void);
static void              handleImage(void);

static void ring_on(void) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        ring.setPixelColor(i, ring.Color(255, 255, 255));
    ring.show();
}

static void ring_off(void) {
    ring.clear();
    ring.show();
}

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Recycling Bin</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
      color: #e0e0e0;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    h1 { font-size: 1.8rem; margin-bottom: 6px; color: #4fc3f7; }
    .subtitle { font-size: 0.9rem; color: #90a4ae; margin-bottom: 28px; }
    .container {
      background: rgba(255,255,255,0.05);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: 16px;
      padding: 30px;
      max-width: 480px;
      width: 100%;
      text-align: center;
    }
    #captureBtn {
      background: linear-gradient(135deg, #4fc3f7, #0288d1);
      color: #fff;
      border: none;
      padding: 14px 40px;
      font-size: 1.1rem;
      font-weight: 600;
      border-radius: 50px;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(79,195,247,0.4);
    }
    #captureBtn:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(79,195,247,0.6); }
    #captureBtn:disabled { background: #555; cursor: not-allowed; box-shadow: none; transform: none; }
    #status { margin-top: 16px; font-size: 0.95rem; color: #b0bec5; min-height: 1.2em; }
    #result {
      display: none;
      margin-top: 24px;
      padding: 20px;
      background: rgba(0,0,0,0.3);
      border-radius: 12px;
      border: 1px solid rgba(255,255,255,0.08);
    }
    #capturedImg {
      max-width: 100%;
      border-radius: 8px;
      margin-bottom: 16px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.4);
    }
    .category-badge {
      display: inline-block;
      padding: 6px 18px;
      border-radius: 20px;
      font-size: 1.4rem;
      font-weight: 700;
      margin-bottom: 8px;
      background: rgba(102,187,106,0.2);
      color: #66bb6a;
      border: 1px solid #66bb6a44;
    }
    .category-badge.trash {
      background: rgba(239,83,80,0.2);
      color: #ef5350;
      border-color: #ef535044;
    }
    .confidence { font-size: 0.95rem; color: #90a4ae; }
    .spinner {
      display: inline-block;
      width: 16px; height: 16px;
      border: 2px solid #4fc3f7;
      border-top-color: transparent;
      border-radius: 50%;
      animation: spin 0.8s linear infinite;
      vertical-align: middle;
      margin-right: 6px;
    }
    @keyframes spin { to { transform: rotate(360deg); } }
  </style>
</head>
<body>
  <h1>&#9851; Smart Recycling Bin</h1>
  <p class="subtitle">AI-Powered Waste Classification</p>
  <div class="container">
    <button id="captureBtn" onclick="capture()">&#128248; Capture &amp; Classify</button>
    <div id="status"></div>
    <div id="result">
      <img id="capturedImg" alt="Captured image" />
      <div class="category-badge" id="labelBadge"></div>
      <div class="confidence" id="confText"></div>
    </div>
  </div>
  <script>
    async function capture() {
      var btn    = document.getElementById('captureBtn');
      var status = document.getElementById('status');
      var result = document.getElementById('result');

      btn.disabled = true;
      status.innerHTML = '<span class="spinner"></span> Capturing &amp; classifying...';
      result.style.display = 'none';

      try {
        var res  = await fetch('/capture');
        var data = await res.json();

        if (data.success) {
          var badge = document.getElementById('labelBadge');
          badge.textContent = data.label;
          badge.className = 'category-badge' + (data.label === 'Trash' ? ' trash' : '');

          document.getElementById('confText').textContent =
            'Confidence: ' + data.confidence + '%';
          document.getElementById('capturedImg').src = '/image?t=' + Date.now();
          result.style.display = 'block';
          status.textContent = 'Done!';
        } else {
          status.textContent = 'Classification failed — please try again.';
        }
      } catch (e) {
        status.textContent = 'Network error: ' + e.message;
      }

      btn.disabled = false;
    }
  </script>
</body>
</html>
)rawliteral";

static void handleRoot(void) {
    server.send(200, "text/html", INDEX_HTML);
}

static void handleCapture(void) {
    Serial.println("══════════════════════════════════════");
    Serial.println("  Web UI: Capture requested");
    Serial.println("══════════════════════════════════════");

    ring_on();
    delay(200);  // let exposure settle

    classify_result_t result = camera_capture_and_classify();

    ring_off();

    if (result.success) {
        String json = "{\"success\":true,\"label\":\"";
        json += result.label;
        json += "\",\"confidence\":\"";
        json += String(result.confidence * 100.0f, 1);
        json += "\"}";
        server.send(200, "application/json", json);
    } else {
        server.send(200, "application/json", "{\"success\":false}");
    }
}

static void handleImage(void) {
    if (last_jpeg_buf && last_jpeg_len > 0) {
        WiFiClient client = server.client();
        server.setContentLength(last_jpeg_len);
        server.send(200, "image/jpeg", "");
        client.write(last_jpeg_buf, last_jpeg_len);
    } else {
        server.send(404, "text/plain", "No image captured yet.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("==============================================");
    Serial.println("  Smart Recycling Bin — XIAO ESP32S3 Sense");
    Serial.println("==============================================");
    Serial.printf("  Model input : %d x %d\n", IMG_WIDTH, IMG_HEIGHT);
    Serial.printf("  Labels      : %d  [ ", EI_CLASSIFIER_LABEL_COUNT);
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.print(ei_classifier_inferencing_categories[i]);
        if (i < EI_CLASSIFIER_LABEL_COUNT - 1) Serial.print(", ");
    }
    Serial.println(" ]");
    Serial.println("==============================================\n");

    ring.begin();
    ring.setBrightness(RING_BRIGHTNESS);
    ring.clear();
    ring.show();
    Serial.println("NeoPixel ring ready (16 LEDs, GPIO2).");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("  Connected! IP: ");
    Serial.println(WiFi.localIP());

    if (!camera_init()) {
        Serial.println("ERROR: Camera init failed — halting.");
        while (true) { delay(1000); }
    }

    snapshot_buf = (uint8_t *)ps_malloc(IMG_WIDTH * IMG_HEIGHT * 3);
    if (!snapshot_buf) {
        Serial.println("ERROR: PSRAM alloc failed — check OPI PSRAM is enabled.");
        while (true) { delay(1000); }
    }

    server.on("/",        handleRoot);
    server.on("/capture", handleCapture);
    server.on("/image",   handleImage);
    server.begin();

    Serial.printf("\nReady — open http://%s in your browser.\n\n",
                  WiFi.localIP().toString().c_str());

    // green flash = ready
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
        ring.setPixelColor(i, ring.Color(0, 180, 0));
    ring.show();
    delay(600);
    ring.clear();
    ring.show();
}

void loop() {
    server.handleClient();
}

static bool camera_init(void) {
    camera_config_t cfg;

    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;

    cfg.pin_d0       = Y2_GPIO_NUM;
    cfg.pin_d1       = Y3_GPIO_NUM;
    cfg.pin_d2       = Y4_GPIO_NUM;
    cfg.pin_d3       = Y5_GPIO_NUM;
    cfg.pin_d4       = Y6_GPIO_NUM;
    cfg.pin_d5       = Y7_GPIO_NUM;
    cfg.pin_d6       = Y8_GPIO_NUM;
    cfg.pin_d7       = Y9_GPIO_NUM;

    cfg.pin_xclk     = XCLK_GPIO_NUM;
    cfg.pin_pclk     = PCLK_GPIO_NUM;
    cfg.pin_vsync    = VSYNC_GPIO_NUM;
    cfg.pin_href     = HREF_GPIO_NUM;
    cfg.pin_sccb_sda = SIOD_GPIO_NUM;
    cfg.pin_sccb_scl = SIOC_GPIO_NUM;
    cfg.pin_pwdn     = PWDN_GPIO_NUM;
    cfg.pin_reset    = RESET_GPIO_NUM;

    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
    cfg.frame_size   = FRAMESIZE_QVGA;  // 320x240

    if (esp_camera_init(&cfg) != ESP_OK) {
        Serial.println("Camera init failed.");
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_exposure_ctrl(s, 1);
        s->set_gain_ctrl(s, 1);
        s->set_wpc(s, 1);
        s->set_raw_gma(s, 1);
        s->set_lenc(s, 1);
        s->set_dcw(s, 1);
    }

    Serial.println("Camera ready.");
    return true;
}

static classify_result_t camera_capture_and_classify(void) {
    classify_result_t res = { false, nullptr, 0.0f };

    Serial.println("Capturing image...");

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("ERROR: Camera capture failed.");
        return res;
    }
    Serial.printf("  %u bytes  (%u x %u)\n", fb->len, fb->width, fb->height);

    // keep a copy of the jpeg for /image
    if (last_jpeg_buf) { free(last_jpeg_buf); last_jpeg_buf = nullptr; last_jpeg_len = 0; }
    last_jpeg_buf = (uint8_t *)ps_malloc(fb->len);
    if (last_jpeg_buf) {
        memcpy(last_jpeg_buf, fb->buf, fb->len);
        last_jpeg_len = fb->len;
    }

    uint8_t *rgb_buf = (uint8_t *)ps_malloc(fb->width * fb->height * 3);
    if (!rgb_buf) {
        Serial.println("ERROR: RGB buffer alloc failed.");
        esp_camera_fb_return(fb);
        return res;
    }

    bool decoded = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb_buf);
    esp_camera_fb_return(fb);

    if (!decoded) {
        Serial.println("ERROR: JPEG decode failed.");
        free(rgb_buf);
        return res;
    }

    // crop to square and resize to model input
    uint16_t src_w     = 320;
    uint16_t src_h     = 240;
    uint16_t crop_size = (src_w < src_h) ? src_w : src_h;
    uint16_t x_off     = (src_w - crop_size) / 2;
    uint16_t y_off     = (src_h - crop_size) / 2;

    for (int dy = 0; dy < IMG_HEIGHT; dy++) {
        for (int dx = 0; dx < IMG_WIDTH; dx++) {
            int sx = x_off + (dx * crop_size) / IMG_WIDTH;
            int sy = y_off + (dy * crop_size) / IMG_HEIGHT;
            int si = (sy * src_w + sx) * 3;
            int di = (dy * IMG_WIDTH + dx) * 3;
            snapshot_buf[di + 0] = rgb_buf[si + 0];
            snapshot_buf[di + 1] = rgb_buf[si + 1];
            snapshot_buf[di + 2] = rgb_buf[si + 2];
        }
    }
    free(rgb_buf);

    ei::signal_t signal;
    signal.total_length = IMG_WIDTH * IMG_HEIGHT;
    signal.get_data     = &raw_feature_get_data;

    ei_impulse_result_t ei_result = { 0 };
    EI_IMPULSE_ERROR err = run_classifier(&signal, &ei_result, false);
    if (err != EI_IMPULSE_OK) {
        Serial.printf("ERROR: Classifier error %d\n", err);
        return res;
    }

    float best_val = 0.0f;
    int   best_idx = 0;

    Serial.println("  Results:");
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        float v = ei_result.classification[i].value;
        Serial.printf("    %-18s %.1f%%\n", ei_classifier_inferencing_categories[i], v * 100.0f);
        if (v > best_val) { best_val = v; best_idx = i; }
    }

    const char *final_label;
    if (best_val < CONFIDENCE_THRESHOLD) {
        final_label = "Trash";
        Serial.printf("  ▶ Low confidence (%.1f%%) — reporting as Trash\n\n", best_val * 100.0f);
    } else {
        final_label = ei_classifier_inferencing_categories[best_idx];
        Serial.printf("  ▶ %s  (%.1f%%)\n\n", final_label, best_val * 100.0f);
    }

    res.success    = true;
    res.label      = final_label;
    res.confidence = best_val;
    return res;
}

// pack RGB pixels as 0xRRGGBB floats for Edge Impulse
static int raw_feature_get_data(size_t offset, size_t length, float *out) {
    for (size_t i = 0; i < length; i++) {
        size_t pi = (offset + i) * 3;
        out[i] = (float)(((uint32_t)snapshot_buf[pi] << 16) |
                         ((uint32_t)snapshot_buf[pi + 1] << 8) |
                          (uint32_t)snapshot_buf[pi + 2]);
    }
    return 0;
}
