/*
  Snow — падающий снег
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

const int NUM_FLAKES = 40;

struct Flake {
  float x, y;
  float speedY;
  float drift;   // лёгкое покачивание по X
  float phase;
  uint8_t size;  // 1 или 2 px
};

Flake flakes[NUM_FLAKES];

void setup() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true) delay(1000);
  }
  randomSeed(esp_random());

  for (int i = 0; i < NUM_FLAKES; i++) {
    flakes[i].x = random(0, SCREEN_WIDTH);
    flakes[i].y = random(-SCREEN_HEIGHT, SCREEN_HEIGHT);
    flakes[i].speedY = 0.3f + random(0, 50) / 100.0f;
    flakes[i].drift = 0.15f + random(0, 25) / 100.0f;
    flakes[i].phase = random(0, 628) / 100.0f; // 0..2π
    flakes[i].size = (random(0, 5) == 0) ? 2 : 1;
  }
}

void loop() {
  display.clearDisplay();

  for (int i = 0; i < NUM_FLAKES; i++) {
    Flake &f = flakes[i];

    f.phase += 0.08f;
    f.y += f.speedY;
    f.x += sin(f.phase) * f.drift;

    // границы по X
    if (f.x < 0) f.x = SCREEN_WIDTH - 1;
    if (f.x >= SCREEN_WIDTH) f.x = 0;

    if (f.y >= SCREEN_HEIGHT) {
      f.y = random(-12, -2);
      f.x = random(0, SCREEN_WIDTH);
      f.speedY = 0.3f + random(0, 50) / 100.0f;
      f.size = (random(0, 5) == 0) ? 2 : 1;
    }

    int ix = (int)f.x;
    int iy = (int)f.y;

    if (f.size == 2) {
      display.fillRect(ix, iy, 2, 2, SSD1306_WHITE);
    } else {
      display.drawPixel(ix, iy, SSD1306_WHITE);
    }
  }

  display.display();
  delay(30);
}
