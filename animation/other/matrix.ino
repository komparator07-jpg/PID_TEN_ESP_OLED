/*
  Matrix Rain v3 — стабильная, без мешанины
  ESP32 + OLED 0.96" SSD1306
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Только «матричные» символы
const char glyphs[] = "01ABCDEF#$%*<>";
const int  GLYPH_COUNT = sizeof(glyphs) - 1;

const int COLS = 12;          // меньше колонок = меньше каши
const int CHAR_W = 10;
const int CHAR_H = 8;

struct Drop {
  int  y;             // позиция головы (в пикселях)
  int  speed;         // кадров до следующего шага
  int  counter;       // счётчик кадров
  int  length;        // длина хвоста
  char headChar;
  char trail[16];
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

  for (int i = 0; i < COLS; i++) {
    spawnDrop(i, true);
  }
}

void spawnDrop(int i, bool firstTime) {
  // Стартуем выше экрана, с разбросом
  drops[i].y       = firstTime ? random(-120, -20) : random(-100, -30);
  drops[i].speed   = random(2, 5);      // чем больше — тем медленнее
  drops[i].counter = 0;
  drops[i].length  = random(5, 11);
  drops[i].headChar = glyphs[random(GLYPH_COUNT)];

  for (int k = 0; k < 16; k++) {
    drops[i].trail[k] = glyphs[random(GLYPH_COUNT)];
  }
}

void loop() {
  display.clearDisplay();

  for (int i = 0; i < COLS; i++) {
    Drop &d = drops[i];

    // Движение только раз в несколько кадров
    d.counter++;
    if (d.counter >= d.speed) {
      d.counter = 0;
      d.y += CHAR_H;

      // Сдвигаем хвост
      for (int k = 15; k > 0; k--) {
        d.trail[k] = d.trail[k - 1];
      }
      d.trail[0] = d.headChar;
      d.headChar = glyphs[random(GLYPH_COUNT)];
    }

    int x = i * CHAR_W + 4;

    // Рисуем голову (всегда)
    int headY = d.y;
    if (headY >= 0 && headY < SCREEN_HEIGHT) {
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(x, headY);
      display.write(d.headChar);
    }

    // Рисуем хвост (реже = эффект затухания)
    for (int j = 1; j < d.length; j++) {
      int py = d.y - j * CHAR_H;
      if (py < 0 || py >= SCREEN_HEIGHT) continue;

      // Чем дальше от головы — тем реже рисуем
      int chance = 10 - j;
      if (chance < 2) chance = 2;
      if (random(0, 10) < chance) {
        display.setCursor(x, py);
        display.write(d.trail[j]);
      }
    }

    // Ушла за экран → новая
    if (d.y - d.length * CHAR_H > SCREEN_HEIGHT) {
      spawnDrop(i, false);
    }
  }

  display.display();
  delay(40);   // общая скорость (можно 50–60 для ещё медленнее)
}
