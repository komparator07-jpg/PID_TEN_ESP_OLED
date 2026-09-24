/*
  ESP32 OLED Digital Clock — Minimal стиль
  Мигающее двоеточие
  Если нет времени → --:--

  Подключение:
    OLED  VCC→3.3V  GND→GND  SDA→21  SCL→22
*/

#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===================== НАСТРОЙКИ WI-FI =====================
const char* WIFI_SSID     = "ВАШ_SSID";        // <-- сюда SSID
const char* WIFI_PASSWORD = "ВАШ_ПАРОЛЬ";      // <-- сюда пароль
// ===========================================================

// NTP (Россия)
const char* ntpServer1 = "0.ru.pool.ntp.org";
const char* ntpServer2 = "1.ru.pool.ntp.org";
const char* ntpServer3 = "ntp1.vniiftri.ru";

// Часовой пояс: Москва UTC+3
const long  gmtOffset_sec = 3 * 3600;
const int   daylightOffset_sec = 0;

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool timeSynced = false;
unsigned long lastNtpTry = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED не найден"));
    while (true) delay(1000);
  }

  showDashes();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi OK"));
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);

    struct tm timeinfo;
    int wait = 0;
    while (!getLocalTime(&timeinfo) && wait < 15) {
      delay(500);
      wait++;
    }
    timeSynced = getLocalTime(&timeinfo);
  } else {
    Serial.println(F("WiFi FAIL — показываем --:--"));
  }
}

void loop() {
  if (!timeSynced && millis() - lastNtpTry > 15000) {
    lastNtpTry = millis();
    if (WiFi.status() == WL_CONNECTED) {
      struct tm ti;
      timeSynced = getLocalTime(&ti);
    }
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    timeSynced = true;

    char tH[3], tM[3];
    strftime(tH, sizeof(tH), "%H", &timeinfo);
    strftime(tM, sizeof(tM), "%M", &timeinfo);

    // Мигание двоеточия раз в секунду
    bool colonOn = (timeinfo.tm_sec % 2 == 0);

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(4);
    display.setCursor(8, 16);
    display.print(tH);
    display.print(colonOn ? ":" : " ");
    display.print(tM);
    display.display();
  } else {
    showDashes();
  }

  delay(200);
}

void showDashes() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(4);
  display.setCursor(8, 16);
  display.print("--:--");
  display.display();
}
