/*
  EMO-style Robot Eyes для ESP32 + OLED 0.96" (SSD1306, 128x64, I2C)
  + кнопка на GPIO4 -> несколько режимов паники, переключаются по очереди

  Подключение:
    OLED VCC -> 3V3
    OLED GND -> GND
    OLED SDA -> GPIO21
    OLED SCL -> GPIO22
    Кнопка   -> между GPIO4 и GND (внутренний PULLUP, доп. резистор не нужен)

  Библиотеки:
    - Adafruit GFX Library
    - Adafruit SSD1306
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

#define BUTTON_PIN 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum Emotion {
  NEUTRAL,
  HAPPY,
  JOYFUL,
  SAD,
  GLOOMY,
  ANGRY,
  FURIOUS,
  SURPRISED,
  SLEEPY,
  SUSPICIOUS,
  EMOTION_COUNT
};

const char* emotionName(Emotion e) {
  switch (e) {
    case NEUTRAL:    return "neutral";
    case HAPPY:      return "happy";
    case JOYFUL:     return "joyful";
    case SAD:        return "sad";
    case GLOOMY:     return "gloomy";
    case ANGRY:      return "angry";
    case FURIOUS:    return "furious";
    case SURPRISED:  return "surprised";
    case SLEEPY:     return "sleepy";
    case SUSPICIOUS: return "suspicious";
    default:         return "?";
  }
}

int centerX = SCREEN_WIDTH / 2;
int centerY = SCREEN_HEIGHT / 2;
int eyeGap  = 14;
int baseW   = 34;
int baseH   = 34;

Emotion currentEmotion = NEUTRAL;
unsigned long lastEmotionChange = 0;
unsigned long emotionInterval = 4000;

// ---------- Моргание ----------
unsigned long lastBlink = 0;
unsigned long blinkInterval = 3000;
unsigned long blinkDuration = 260;
unsigned long blinkStart = 0;
bool isBlinking = false;

// текущие "живые" параметры глаз
float curW = baseW, curH = baseH, curOffsetY = 0;
float curBrowL = 0, curBrowR = 0;
float curHR_delta = 0; // разница высоты правого глаза (асимметрия)
float targetW = baseW, targetH = baseH, targetOffsetY = 0;
float targetBrowL = 0, targetBrowR = 0;
float targetHR_delta = 0;

int lookOffsetX = 0, lookOffsetY = 0;
int targetLookX = 0, targetLookY = 0;
unsigned long lastLookChange = 0;

// ---------- Кнопка ----------
bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 40;

// ---------- Паника: несколько режимов ----------
enum PanicMode {
  PANIC_SHOCK_FLEE,   // шок с дрожью -> бегство по углам (исходный режим)
  PANIC_SNEAKY,       // вспышка удивления -> хитрый прищур, бегающие глаза
  PANIC_MODE_COUNT
};

int panicMode = -1; // какой режим будет запущен при следующем нажатии (инкрементируется)
bool panicActive = false;
unsigned long panicStart = 0;

// длительности фаз для каждого режима (индекс = PanicMode)
const unsigned long PANIC_PHASE1_DURATION[PANIC_MODE_COUNT] = { 900, 500 };
const unsigned long PANIC_PHASE2_DURATION[PANIC_MODE_COUNT] = { 3200, 4000 };

unsigned long lastJitterUpdate = 0;
int jitterX = 0, jitterY = 0;

// метание по углам (режим PANIC_SHOCK_FLEE, фаза 2)
unsigned long lastCornerChange = 0;
int cornerIndex = -1;
int cornerAmpX = 20;
int cornerAmpY = 12;

// хитрое зырканье (режим PANIC_SNEAKY, фаза 2)
unsigned long lastSneakyGlanceChange = 0;
unsigned long lastSneakySquintSwap = 0;
int sneakyGlanceSide = 0; // -1 лево, 1 право
int sneakyAmpX = 14;

Emotion emotionBeforePanic = NEUTRAL;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Не найден OLED дисплей! Проверь подключение/адрес.");
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.display();
  randomSeed(analogRead(0));
  setEmotionTargets(currentEmotion);
  lastBlink = millis();
  lastEmotionChange = millis();
}

void loop() {
  unsigned long now = millis();

  handleButton(now);

  if (panicActive) {
    unsigned long elapsed = now - panicStart;
    unsigned long phase1 = PANIC_PHASE1_DURATION[panicMode];
    unsigned long phase2 = PANIC_PHASE2_DURATION[panicMode];
    unsigned long totalPanicDuration = phase1 + phase2;

    if (elapsed > totalPanicDuration) {
      panicActive = false;
      currentEmotion = emotionBeforePanic;
      setEmotionTargets(currentEmotion);
      lastEmotionChange = now;
      lastBlink = now;
      isBlinking = false;
      jitterX = 0; jitterY = 0;
    } else if (elapsed <= phase1) {
      runPanicPhase1(now);
    } else {
      runPanicPhase2(now);
    }
  } else {
    if (now - lastEmotionChange > emotionInterval) {
      lastEmotionChange = now;
      Emotion next;
      do {
        next = (Emotion)random(0, EMOTION_COUNT);
      } while (next == currentEmotion);
      currentEmotion = next;
      setEmotionTargets(currentEmotion);
      Serial.println(emotionName(currentEmotion));
    }

    if (!isBlinking && now - lastBlink > blinkInterval) {
      isBlinking = true;
      blinkStart = now;
    }
    if (isBlinking && now - blinkStart > blinkDuration) {
      isBlinking = false;
      lastBlink = now;
      blinkInterval = 2500 + random(0, 3000);
    }

    if (now - lastLookChange > 1800) {
      lastLookChange = now;
      targetLookX = random(-8, 9);
      targetLookY = random(-4, 5);
    }

    jitterX = 0;
    jitterY = 0;
  }

  bool inPhase2 = panicActive && (now - panicStart > PANIC_PHASE1_DURATION[panicMode]);
  float speed = panicActive ? (inPhase2 ? 0.25 : 0.35) : 0.18;
  curW        += (targetW - curW) * speed;
  curH        += (targetH - curH) * speed;
  curOffsetY  += (targetOffsetY - curOffsetY) * speed;
  curBrowL    += (targetBrowL - curBrowL) * speed;
  curBrowR    += (targetBrowR - curBrowR) * speed;
  curHR_delta += (targetHR_delta - curHR_delta) * speed;

  float lookSpeed = panicActive ? (inPhase2 ? 0.35 : 0.45) : 0.1;
  lookOffsetX += (int)((targetLookX - lookOffsetX) * lookSpeed);
  lookOffsetY += (int)((targetLookY - lookOffsetY) * lookSpeed);

  drawFrame();
  delay(15);
}

void handleButton(unsigned long now) {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if (now - lastDebounceTime > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        triggerPanic(now);
      }
    }
  }

  lastButtonReading = reading;
}

void triggerPanic(unsigned long now) {
  if (!panicActive) {
    emotionBeforePanic = currentEmotion;
  }

  // переключаемся на следующий режим паники по кругу
  panicMode = (panicMode + 1) % PANIC_MODE_COUNT;

  panicActive = true;
  panicStart = now;
  isBlinking = false;
  cornerIndex = -1;
  sneakyGlanceSide = 0;
  targetHR_delta = 0;

  lastJitterUpdate = now;
  lastLookChange = now;
  lastCornerChange = now;
  lastSneakyGlanceChange = now;
  lastSneakySquintSwap = now;

  // targets для фазы 1 задаются в runPanicPhase1 сразу же на этом вызове loop,
  // но выставим стартовые значения на всякий случай:
  switch (panicMode) {
    case PANIC_SHOCK_FLEE:
      targetW = baseW + 14;
      targetH = baseH + 16;
      targetOffsetY = -3;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case PANIC_SNEAKY:
      targetW = baseW + 6;
      targetH = baseH + 6;
      targetOffsetY = -2;
      targetBrowL = 0; targetBrowR = 0;
      break;
  }

  Serial.print("PANIC mode: ");
  Serial.println(panicMode == PANIC_SHOCK_FLEE ? "shock_flee" : "sneaky");
}

// ---------- Фаза 1: общая точка входа ----------
void runPanicPhase1(unsigned long now) {
  switch (panicMode) {
    case PANIC_SHOCK_FLEE:
      panicPhase1_ShockFlee(now);
      break;
    case PANIC_SNEAKY:
      panicPhase1_Sneaky(now);
      break;
  }
}

// ---------- Фаза 2: общая точка входа ----------
void runPanicPhase2(unsigned long now) {
  switch (panicMode) {
    case PANIC_SHOCK_FLEE:
      panicPhase2_ShockFlee(now);
      break;
    case PANIC_SNEAKY:
      panicPhase2_Sneaky(now);
      break;
  }
}

// ===================== РЕЖИМ 0: ШОК -> БЕГСТВО =====================

// Фаза 1 — шок: огромные глаза, мелкая быстрая дрожь, взгляд туда-сюда
void panicPhase1_ShockFlee(unsigned long now) {
  targetW = baseW + 14 + random(-2, 3);
  targetH = baseH + 16 + random(-2, 3);

  if (now - lastJitterUpdate > 40) {
    lastJitterUpdate = now;
    jitterX = random(-3, 4);
    jitterY = random(-3, 4);
  }

  if (now - lastLookChange > 90) {
    lastLookChange = now;
    targetLookX = random(-16, 17);
    targetLookY = random(-8, 9);
  }
}

// Фаза 2 — бегство: глаза уменьшаются и скачут по углам экрана
void panicPhase2_ShockFlee(unsigned long now) {
  targetW = baseW * 0.55 + random(-2, 3);
  targetH = baseH * 0.5  + random(-2, 3);
  targetOffsetY = 0;

  if (now - lastJitterUpdate > 60) {
    lastJitterUpdate = now;
    jitterX = random(-2, 3);
    jitterY = random(-2, 3);
  }

  unsigned long cornerInterval = 160 + random(0, 120);
  if (now - lastCornerChange > cornerInterval) {
    lastCornerChange = now;

    int newCorner;
    do {
      newCorner = random(0, 4);
    } while (newCorner == cornerIndex);
    cornerIndex = newCorner;

    switch (cornerIndex) {
      case 0: targetLookX = -cornerAmpX; targetLookY = -cornerAmpY; break;
      case 1: targetLookX =  cornerAmpX; targetLookY = -cornerAmpY; break;
      case 2: targetLookX = -cornerAmpX; targetLookY =  cornerAmpY; break;
      case 3: targetLookX =  cornerAmpX; targetLookY =  cornerAmpY; break;
    }
  }
}

// ===================== РЕЖИМ 1: ХИТРАЯ ПАНИКА =====================

// Фаза 1 — короткая вспышка удивления, БЕЗ дрожи, взгляд спокойно замер по центру
void panicPhase1_Sneaky(unsigned long now) {
  targetW = baseW + 6;
  targetH = baseH + 6;
  targetOffsetY = -2;
  targetBrowL = 0;
  targetBrowR = 0;
  targetHR_delta = 0;

  jitterX = 0;
  jitterY = 0;

  targetLookX = 0;
  targetLookY = 0;
}

// Фаза 2 — хитрый прищур: глаза сужаются, попеременно прищуривается то один,
// то другой глаз, взгляд резко зыркает влево-вправо, будто что-то скрывают
void panicPhase2_Sneaky(unsigned long now) {
  targetW = baseW * 0.9;
  targetH = baseH * 0.32;
  targetOffsetY = 1;

  jitterX = 0; // без дрожи — движения выглядят осознанными, а не паническими
  jitterY = 0;

  // попеременный "хитрый" прищур одного глаза сильнее другого
  unsigned long squintInterval = 450 + random(0, 300);
  if (now - lastSneakySquintSwap > squintInterval) {
    lastSneakySquintSwap = now;
    // случайно выбираем, какой глаз прищурен сильнее в этот раз
    targetHR_delta = (random(0, 2) == 0) ? -12 : 12;
  }

  // резкое зырканье влево-вправо с короткими паузами, будто проверяет,
  // не смотрит ли кто
  unsigned long glanceInterval = 260 + random(0, 260);
  if (now - lastSneakyGlanceChange > glanceInterval) {
    lastSneakyGlanceChange = now;

    int r = random(0, 10);
    if (r < 4) {
      // короткая пауза по центру, будто задумался/притворяется
      targetLookX = 0;
      targetLookY = 2;
    } else {
      sneakyGlanceSide = -sneakyGlanceSide;
      if (sneakyGlanceSide == 0) sneakyGlanceSide = 1;
      targetLookX = sneakyGlanceSide * sneakyAmpX;
      targetLookY = random(-2, 3);
    }
  }
}

// ===================== Обычные эмоции =====================

void setEmotionTargets(Emotion e) {
  targetHR_delta = 0; // по умолчанию симметрично, кроме SUSPICIOUS

  switch (e) {
    case NEUTRAL:
      targetW = baseW; targetH = baseH; targetOffsetY = 0;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case HAPPY:
      targetW = baseW + 4; targetH = baseH * 0.55; targetOffsetY = 4;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case JOYFUL:
      targetW = baseW + 8; targetH = baseH * 0.28; targetOffsetY = 6;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case SAD:
      targetW = baseW - 4; targetH = baseH * 0.75; targetOffsetY = 2;
      targetBrowL = 8; targetBrowR = -8;
      break;
    case GLOOMY:
      targetW = baseW - 6; targetH = baseH * 0.35; targetOffsetY = 9;
      targetBrowL = 10; targetBrowR = -10;
      break;
    case ANGRY:
      targetW = baseW; targetH = baseH * 0.7; targetOffsetY = 0;
      targetBrowL = -10; targetBrowR = 10;
      break;
    case FURIOUS:
      targetW = baseW - 4; targetH = baseH * 0.42; targetOffsetY = -2;
      targetBrowL = -18; targetBrowR = 18;
      break;
    case SURPRISED:
      targetW = baseW + 8; targetH = baseH + 8; targetOffsetY = -2;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case SLEEPY:
      targetW = baseW; targetH = baseH * 0.45; targetOffsetY = 5;
      targetBrowL = 0; targetBrowR = 0;
      break;
    case SUSPICIOUS:
      targetW = baseW; targetH = baseH * 0.7; targetOffsetY = 0;
      targetBrowL = -6; targetBrowR = 0;
      targetHR_delta = -14;
      break;
  }
}

float getBlinkFactor(unsigned long now) {
  if (!isBlinking) return 0.0;
  float progress = (float)(now - blinkStart) / (float)blinkDuration;
  if (progress > 1.0) progress = 1.0;
  return sin(progress * PI);
}

void drawFrame() {
  unsigned long now = millis();
  display.clearDisplay();

  float blinkFactor = panicActive ? 0.0 : getBlinkFactor(now);

  int hL = (int)(curH * (1.0 - blinkFactor * 0.93));
  if (hL < 2) hL = 2;

  int hR = (int)((curH + curHR_delta) * (1.0 - blinkFactor * 0.93));
  if (hR < 2) hR = 2;

  int leftX  = centerX - eyeGap / 2 - (int)curW + lookOffsetX + jitterX;
  int rightX = centerX + eyeGap / 2 + lookOffsetX + jitterX;
  int centerLineY = centerY + (int)curOffsetY + lookOffsetY + jitterY;

  int yL = centerLineY - hL / 2;
  int yR = centerLineY - hR / 2;

  int radiusL = min((int)curW, hL) / 3;
  if (radiusL < 2) radiusL = 2;
  int radiusR = min((int)curW, hR) / 3;
  if (radiusR < 2) radiusR = 2;

  display.fillRoundRect(leftX,  yL, (int)curW, hL, radiusL, SSD1306_WHITE);
  display.fillRoundRect(rightX, yR, (int)curW, hR, radiusR, SSD1306_WHITE);

  if (!panicActive && blinkFactor < 0.5 && (curBrowL != 0 || curBrowR != 0)) {
    drawBrow(leftX,  yL, (int)curW, hL, (int)curBrowL, true);
    drawBrow(rightX, yR, (int)curW, hR, (int)curBrowR, false);
  }

  display.display();
}

void drawBrow(int ex, int ey, int ew, int eh, int tilt, bool isLeftEye) {
  int cutHeight = eh / 2;
  if (cutHeight < 2) return;

  if (isLeftEye) {
    int x0 = ex, y0 = ey;
    int x1 = ex + ew, y1 = ey;
    int x2 = ex + ew, y2 = ey + cutHeight - tilt / 2;
    int x3 = ex, y3 = ey + cutHeight + tilt / 2;
    display.fillTriangle(x0, y0, x1, y1, x2, y2, SSD1306_BLACK);
    display.fillTriangle(x0, y0, x2, y2, x3, y3, SSD1306_BLACK);
  } else {
    int x0 = ex, y0 = ey;
    int x1 = ex + ew, y1 = ey;
    int x2 = ex + ew, y2 = ey + cutHeight + tilt / 2;
    int x3 = ex, y3 = ey + cutHeight - tilt / 2;
    display.fillTriangle(x0, y0, x1, y1, x2, y2, SSD1306_BLACK);
    display.fillTriangle(x0, y0, x2, y2, x3, y3, SSD1306_BLACK);
  }
}
