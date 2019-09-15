#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>

#define WIFI_SSID "<YOUR_SSID>"
#define WIFI_PASS "<YOUR_PASSWORD>"
#define STATS_API "http://<YOUR_SERVER>:3000/stats"

#define BTN_LEFT 12
#define BTN_MID 14
#define BTN_RIGHT 13

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const char* host;
int cores = 0;
int freeMem;
int freeDisk;
const char* uptime;
float load1;
float load5;
float load15;

const size_t capacity = JSON_OBJECT_SIZE(8) + 90;
DynamicJsonDocument doc(capacity);


void setupButtons() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_MID, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
}

void setup(void) {
  Serial.begin(9600);

  Wire.begin(5, 4); // (CLK,SDA)
  u8g2.begin();
  setupButtons();

  waitForWifi();
}

// <network functions>

void waitForWifi() {
  int counter = 0;
  do {
    u8g2.clearBuffer();

    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(3, 18, (String("Connecting to WiFi, ") + String(counter)).c_str());
    u8g2.sendBuffer();

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    counter++;
    delay(4000);
  } while (WiFi.status() != WL_CONNECTED);
  Serial.println("Connected to wifi");
}

void fetchData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(STATS_API);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      // {
      //     "host":"mrbook.fritz.box",
      //     "cores":8,
      //     "free":0,
      //     "uptime":"8d19h37",
      //     "load1":2.728515625,
      //     "load5":2.24951171875,
      //     "load15":1.970703125
      // }

      const char* json = payload.c_str();

      deserializeJson(doc, json);

      host = doc["host"];
      cores = doc["cores"];
      freeMem = doc["freeMem"];
      freeDisk = doc["freeDisk"];
      uptime = doc["uptime"];
      load1 = doc["load1"];
      load5 = doc["load5"];
      load15 = doc["load15"];

      Serial.print("freeDisk");
      Serial.println(freeDisk);
      Serial.print("freeMem");
      Serial.println(freeMem);
      Serial.print("load1");
      Serial.println(load1);
      Serial.print("load5");
      Serial.println(load5);
      Serial.print("load15");
      Serial.println(load15);

      
    }

    http.end();
  }

}

// </network functions>

// <draw functions>

void progressBar(U8G2 u8g2, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t percent)
{
  // can't draw it smaller than 10x8
  height = height < 8 ? 8 : height;
  width = width < 10 ? 10 : width;

  // draw percentage
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(x + width + 2, y + height / 2 + 2, (String(percent) + String("%")).c_str());

  // draw it
  u8g2.drawRFrame(x, y, width, height, 4);
  u8g2.drawBox(x + 2, y + 2, (width - 4) * (percent / 100.0), height - 4);
}

void gauge(U8G2 u8g2, uint8_t x, uint8_t y, uint8_t r, uint8_t percent)
{
  uint8_t rx = x + r;
  uint8_t ry = y;

  uint8_t px = rx + (r - 2) * cos(2 * PI * (percent / 2.0 + 50) / 100);
  uint8_t py = ry + (r - 2) * sin(2 * PI * (percent / 2.0 + 50) / 100);

  u8g2.drawLine(rx, ry, px, py);

  u8g2.drawCircle(rx, y, r, U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(rx, y, r, U8G2_DRAW_UPPER_RIGHT);
}

void dialog(U8G2 u8g2, uint8_t x, uint8_t y, uint8_t width, uint8_t height, String title)
{
  u8g2.drawRFrame(x, y, width, height, 2);

  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(x + (width / 2) - ((String(title).length() * (u8g2.getMaxCharWidth())) / 2), y + u8g2.getMaxCharHeight(), title.c_str());
  u8g2.drawHLine(x, y + u8g2.getMaxCharHeight() + 1, width);
}

void draw(U8G2 u8g2)
{
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(3, 18, (String("Uptime: ") + uptime).c_str());

  u8g2.setFont(u8g2_font_4x6_tf);
  int x = 0;
  int y = 27;
  u8g2.drawStr(6 + x, y, "1");
  gauge(u8g2, 8 + x, y + 10, 16, (load1 / (1.0 * cores)) * 100);
  u8g2.drawStr(44 + x, y, "5");
  gauge(u8g2, 48 + x, y + 10, 16, (load5 / (1.0 * cores)) * 100);
  u8g2.drawStr(83 + x, y, "15");
  gauge(u8g2, 88 + x, y + 10, 16,  (load15 / (1.0 * cores)) * 100);

  u8g2.setFont(u8g2_font_5x8_tf);

  u8g2.drawStr(3, 60, "HDD:");
  progressBar(u8g2, 30, 52, 76, 10, (100 - freeDisk));

  u8g2.drawStr(3, 48, "RAM:");
  progressBar(u8g2, 30, 40, 76, 10, (100 - freeMem));

  dialog(u8g2, 0, 0, 128, 64, host);
}

// </draw functions>

void loop(void) {
  Serial.println("fetching data");

  fetchData();

  if (cores > 0) {
    Serial.println("drawing data");
    u8g2.clearBuffer();
    draw(u8g2);
    // u8g2.drawStr(0, 25, digitalRead(BTN_LEFT) ? "BTN_LEFT: UP" : "BTN_LEFT: DOWN");
    // u8g2.drawStr(0, 35, digitalRead(BTN_MID) ? "BTN_MID: UP" : "BTN_MID: DOWN");
    // u8g2.drawStr(0, 45, digitalRead(BTN_RIGHT) ? "BTN_RIGHT: UP" : "BTN_RIGHT: DOWN");
    u8g2.sendBuffer();
  }

  delay(10000);
}
