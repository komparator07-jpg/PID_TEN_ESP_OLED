#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== РЕГУЛЯТОР СКОРОСТИ ==========
float ANIM_SPEED = 0.35;

void drawRandomNoise(uint8_t intensity = 40) {
  for (int i = 0; i < intensity; i++) {
    display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), random(2));
  }
}

void horizontalTear() {
  int y = random(SCREEN_HEIGHT);
  for (int x = 0; x < SCREEN_WIDTH; x += 2) {
    if (random(100) < 50) {
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }
}

// ========== ПРАВИЛЬНОЕ СВОРАЧИВАНИЕ В ЛИНИЮ ==========
void verticalCollapse() {
  // Сохраняем текущий кадр с рябью
  uint8_t* buf = display.getBuffer();
  uint8_t saved[1024];
  memcpy(saved, buf, 1024);

  // Сжимаем большими шагами (быстро + выглядит нормально)
  for (int h = SCREEN_HEIGHT; h >= 2; h -= 5) {
    display.clearDisplay();

    int startY = (SCREEN_HEIGHT - h) / 2;

    // Масштабируем сохранённое изображение по вертикали
    for (int y = 0; y < h; y++) {
      // Откуда брать строку из оригинала
      int srcY = map(y, 0, h - 1, 0, SCREEN_HEIGHT - 1);

      for (int x = 0; x < SCREEN_WIDTH; x++) {
        int byteIndex = (srcY / 8) * SCREEN_WIDTH + x;
        int bit = srcY % 8;

        if (saved[byteIndex] & (1 << bit)) {
          display.drawPixel(x, startY + y, SSD1306_WHITE);
        }
      }
    }

    // Лёгкие помехи во время сжатия (не забивают экран)
    if (h > 10) {
      drawRandomNoise(8);
    }

    display.display();
    delay(11 * ANIM_SPEED);
  }
}

void lineToPoint() {
  int centerY = SCREEN_HEIGHT / 2;

  for (int w = SCREEN_WIDTH; w >= 2; w -= 8) {
    display.clearDisplay();
    int startX = (SCREEN_WIDTH - w) / 2;
    display.drawFastHLine(startX, centerY, w, SSD1306_WHITE);

    if (random(100) < 35) {
      display.drawPixel(startX + random(w), centerY, SSD1306_WHITE);
    }

    display.display();
    delay(10 * ANIM_SPEED);
  }

  // Точка
  for (int i = 0; i < 5; i++) {
    display.clearDisplay();
    if (i % 2 == 0) {
      display.fillRect(SCREEN_WIDTH / 2 - 1, centerY - 1, 3, 3, SSD1306_WHITE);
    }
    display.display();
    delay(25 * ANIM_SPEED);
  }

  display.clearDisplay();
  display.display();
}

void terminatorShutdown() {
  // 1. Рябь
  for (int frame = 0; frame < 18; frame++) {
    drawRandomNoise(45 + random(25));

    if (frame % 2 == 0) {
      horizontalTear();
    }

    if (random(100) < 25) {
      display.fillRect(random(SCREEN_WIDTH - 25), random(SCREEN_HEIGHT - 8),
                       random(8, 28), random(2, 6), SSD1306_INVERSE);
    }

    display.display();
    delay(14 * ANIM_SPEED);
  }

  // 2. Нормальное сворачивание текущего изображения в линию
  verticalCollapse();

  // 3. Линия → точка
  lineToPoint();

  delay(300 * ANIM_SPEED);
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  terminatorShutdown();
  delay(2000);
}
