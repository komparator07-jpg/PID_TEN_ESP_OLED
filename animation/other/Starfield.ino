/*
  Starfield — звёздное небо с параллаксом
  ESP32 + OLED 0.96"  SDA=21 SCL=22
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int NUM_STARS = 45;

struct Star {
  float x, y;
  float speed;   // слой: ближние быстрее
  uint8_t layer; // 0=дальние, 1=средние, 2=ближние
};

Star stars[NUM_STARS];

void setup() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true) delay(1000);
  }
  randomSeed(esp_random());

  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(0, SCREEN_WIDTH);
    stars[i].y = random(0, SCREEN_HEIGHT);
    stars[i].layer = random(0, 3);
    // дальние медленнее, ближние быстрее
    if (stars[i].layer == 0) stars[i].speed = 0.15f + random(0, 10) / 100.0f;
    else if (stars[i].layer == 1) stars[i].speed = 0.4f + random(0, 20) / 100.0f;
    else stars[i].speed = 0.9f + random(0, 40) / 100.0f;
  }
}

void loop() {
  display.clearDisplay();

  for (int i = 0; i < NUM_STARS; i++) {
    Star &s = stars[i];
    s.y += s.speed;

    if (s.y >= SCREEN_HEIGHT) {
      s.y = 0;
      s.x = random(0, SCREEN_WIDTH);
    }

    int ix = (int)s.x;
    int iy = (int)s.y;

    // дальние — точка, средние — точка, ближние — чуть крупнее
    if (s.layer == 2) {
      display.fillRect(ix, iy, 2, 2, SSD1306_WHITE);
    } else {
      display.drawPixel(ix, iy, SSD1306_WHITE);
      // средние иногда «мигают» очень редко — стабильно по индексу кадра не надо
    }
  }

  display.display();
  delay(30);
}
