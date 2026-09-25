/*
  Matrix Rain — сплошные струи, без мигания хвоста
  ESP32 + OLED 0.96"  SDA=21 SCL=22
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char glyphs[] = "01";
const int  GLYPH_COUNT = 2;

const int COLS       = 6;    // мало
const int CHAR_W     = 20;   // широкий зазор
const int CHAR_H     = 8;
const int STEP_EVERY = 5;    // медленно
const int FRAME_MS   = 50;

struct Drop {
  int  headY;
  int  length;
  int  frameCnt;
  char cells[12];
};

Drop drops[COLS];

void setup() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true) delay(1000);
  }
  display.clearDisplay();
  display.display();
  randomSeed(esp_random());

  for (int i = 0; i < COLS; i++) spawn(i, true);
}

void spawn(int i, bool first) {
  drops[i].headY    = first ? random(-150, -40) : random(-130, -60);
  drops[i].length   = random(5, 8);   // короткий сплошной хвост
  drops[i].frameCnt = random(0, STEP_EVERY);

  for (int k = 0; k < 12; k++)
    drops[i].cells[k] = glyphs[random(GLYPH_COUNT)];
}

void loop() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  for (int i = 0; i < COLS; i++) {
    Drop &d = drops[i];

    d.frameCnt++;
    if (d.frameCnt >= STEP_EVERY) {
      d.frameCnt = 0;
      d.headY += CHAR_H;

      // сдвиг содержимого только вместе с движением
      for (int k = 11; k > 0; k--)
        d.cells[k] = d.cells[k - 1];
      d.cells[0] = glyphs[random(GLYPH_COUNT)];
    }

    int x = i * CHAR_W + 8;

    // ВСЕ клетки хвоста — всегда (никакого random и никакого % от Y)
    for (int j = 0; j < d.length; j++) {
      int y = d.headY - j * CHAR_H;
      if (y < 0 || y >= SCREEN_HEIGHT) continue;

      display.setCursor(x, y);
      display.write(d.cells[j]);
    }

    if (d.headY - d.length * CHAR_H > SCREEN_HEIGHT + 8)
      spawn(i, false);
  }

  display.display();
  delay(FRAME_MS);
}
