/*
 * ============================================================
 *  Soil Moisture Detector — 3 Screen Modes with Button
 *  Hardware:
 *    - Arduino Pro Mini (5V)
 *    - I2C SSD1306 0.96" OLED (128x64)
 *    - Capacitive Soil Moisture Sensor V2.0.0
 *    - Push Button
 *
 *  Wiring:
 *    OLED SDA    → A4
 *    OLED SCL    → A5
 *    OLED VCC    → 5V
 *    OLED GND    → GND
 *    Sensor OUT  → A0
 *    Sensor VCC  → 5V
 *    Sensor GND  → GND
 *    Button      → D2 and GND (uses internal pull-up)
 *
 *  Button press cycles through 3 screens:
 *    Screen 1 → Value + Status Display
 *    Screen 2 → Plant Emotion Display
 *    Screen 3 → Gauge / Meter Display
 *
 *  Libraries:
 *    Adafruit SSD1306
 *    Adafruit GFX
 * ============================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── OLED ─────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDR      0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── Sensor ───────────────────────────────────────────────────
#define SENSOR_PIN   A0
#define DRY_VALUE    620    // ADC value in dry air  — calibrate!
#define WET_VALUE    280    // ADC value in water    — calibrate!

// ── Button ───────────────────────────────────────────────────
#define BUTTON_PIN   2
#define DEBOUNCE_MS  50

// ── Display Modes ────────────────────────────────────────────
#define MODE_STATUS  0
#define MODE_PLANT   1
#define MODE_GAUGE   2
#define TOTAL_MODES  3

// ── Status Levels ────────────────────────────────────────────
#define LEVEL_DRY        0   //  0–25%
#define LEVEL_LOW        1   // 26–45%
#define LEVEL_GOOD       2   // 46–70%
#define LEVEL_WET        3   // 71–85%
#define LEVEL_SATURATED  4   // 86–100%

// ── Globals ──────────────────────────────────────────────────
int           currentMode    = MODE_STATUS;
int           moisturePct    = 0;
int           moistureRaw    = 0;
int           statusLevel    = LEVEL_GOOD;

bool          lastButtonState   = HIGH;
bool          buttonState       = HIGH;
unsigned long lastDebounceTime  = 0;

// Gauge needle animation
float         needleAngle       = 0.0;   // current displayed angle
float         targetAngle       = 0.0;   // target angle from moisture

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED not found!"));
    while (true);
  }

  display.clearDisplay();
  showSplash();
  delay(2000);
}

// ─────────────────────────────────────────────────────────────
void loop() {
  // Read sensor
  moistureRaw = analogRead(SENSOR_PIN);
  moisturePct = map(moistureRaw, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePct = constrain(moisturePct, 0, 100);

  // Status level
  if      (moisturePct <= 25) statusLevel = LEVEL_DRY;
  else if (moisturePct <= 45) statusLevel = LEVEL_LOW;
  else if (moisturePct <= 70) statusLevel = LEVEL_GOOD;
  else if (moisturePct <= 85) statusLevel = LEVEL_WET;
  else                        statusLevel = LEVEL_SATURATED;

  // Gauge needle smooth target
  // Map 0–100% to 150°–30° (left to right sweep, 120° arc)
  targetAngle = map(moisturePct, 0, 100, 150, 30);

  // Smooth needle animation
  if (needleAngle < targetAngle) needleAngle += 2.0;
  if (needleAngle > targetAngle) needleAngle -= 2.0;

  // Button debounce
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();
  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        currentMode = (currentMode + 1) % TOTAL_MODES;
        display.clearDisplay();
      }
    }
  }
  lastButtonState = reading;

  // Draw
  display.clearDisplay();
  switch (currentMode) {
    case MODE_STATUS: drawStatusScreen(); break;
    case MODE_PLANT:  drawPlantScreen();  break;
    case MODE_GAUGE:  drawGaugeScreen();  break;
  }

  // Mode indicator dots (bottom right)
  drawModeIndicator();

  display.display();
  delay(30);
}

// ══════════════════════════════════════════════════════════════
//  SCREEN 1 — Status Screen
// ══════════════════════════════════════════════════════════════
void drawStatusScreen() {
  // Title bar
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(16, 3);
  display.print(F("SOIL MOISTURE"));
  display.setTextColor(SSD1306_WHITE);

  // Large percentage
  display.setTextSize(3);
  int px = (moisturePct < 10) ? 44 : (moisturePct < 100) ? 28 : 16;
  display.setCursor(px, 17);
  display.print(moisturePct);
  display.setTextSize(2);
  display.setCursor(98, 20);
  display.print(F("%"));

  // Bar outline
  display.drawRect(4, 44, 110, 11, SSD1306_WHITE);
  // Bar fill
  int fillW = map(moisturePct, 0, 100, 0, 108);
  if (fillW > 0) display.fillRect(5, 45, fillW, 9, SSD1306_WHITE);

  // Tick marks at 25, 50, 75%
  for (int t = 1; t <= 3; t++) {
    int tx = 5 + (108 * t / 4);
    display.drawPixel(tx, 44, SSD1306_BLACK);
    display.drawPixel(tx, 54, SSD1306_BLACK);
  }

  // Status label with box
  const char* lbl = getStatusLabel();
  int lw = strlen(lbl) * 6 + 6;
  int lx = (110 - lw) / 2 + 4;
  display.drawRect(lx, 56, lw, 9, SSD1306_WHITE);
  display.setCursor(lx + 3, 57);
  display.setTextSize(1);
  display.print(lbl);

  // Emoji icon right of bar
  drawStatusIcon(114, 44);
}

// Status icon (small face)
void drawStatusIcon(int x, int y) {
  display.drawCircle(x + 5, y + 5, 5, SSD1306_WHITE);
  switch (statusLevel) {
    case LEVEL_GOOD:
      display.fillCircle(x + 3, y + 4, 1, SSD1306_WHITE);
      display.fillCircle(x + 7, y + 4, 1, SSD1306_WHITE);
      display.drawLine(x + 2, y + 7, x + 4, y + 9, SSD1306_WHITE);
      display.drawLine(x + 4, y + 9, x + 8, y + 7, SSD1306_WHITE);
      break;
    case LEVEL_DRY:
    case LEVEL_LOW:
      display.fillCircle(x + 3, y + 4, 1, SSD1306_WHITE);
      display.fillCircle(x + 7, y + 4, 1, SSD1306_WHITE);
      display.drawLine(x + 2, y + 9, x + 4, y + 7, SSD1306_WHITE);
      display.drawLine(x + 4, y + 7, x + 8, y + 9, SSD1306_WHITE);
      break;
    default:
      display.fillCircle(x + 3, y + 4, 1, SSD1306_WHITE);
      display.fillCircle(x + 7, y + 4, 1, SSD1306_WHITE);
      display.drawLine(x + 2, y + 8, x + 8, y + 8, SSD1306_WHITE);
      break;
  }
}

// ══════════════════════════════════════════════════════════════
//  SCREEN 2 — Plant Emotion Screen (Big Face, Full Width)
// ══════════════════════════════════════════════════════════════
void drawPlantScreen() {
  drawBigPot();
  drawBigPlantBody();
  drawBigEmotion();
  drawPlantStatusLabel();
}

// Big pot — centered, wider (full 128px width)
void drawBigPot() {
  // Pot rim
  display.fillRect(14, 40, 100, 4, SSD1306_WHITE);
  // Pot body (trapezoid filled)
  for (int y = 44; y <= 62; y++) {
    int lx = map(y, 44, 62, 14, 20);
    int rx = map(y, 44, 62, 114, 108);
    display.drawLine(lx, y, rx, y, SSD1306_WHITE);
  }
  // Soil line (dark stripe inside pot top)
  display.drawLine(16, 47, 112, 47, SSD1306_BLACK);
  display.drawLine(16, 46, 112, 46, SSD1306_BLACK);
  display.drawLine(16, 45, 112, 45, SSD1306_BLACK);
}

// Big plant body — centered at x=64
void drawBigPlantBody() {
  int cx = 64;
  switch (statusLevel) {

    case LEVEL_DRY:
      // Thin wilted stem, drooping tips
      display.drawLine(cx, 40, cx, 22, SSD1306_WHITE);
      display.drawLine(cx, 22, cx+4, 16, SSD1306_WHITE);
      display.drawLine(cx+4, 16, cx+9, 20, SSD1306_WHITE);
      // Droopy right leaf
      display.drawLine(cx, 30, cx+18, 38, SSD1306_WHITE);
      display.drawLine(cx+18, 38, cx+14, 40, SSD1306_WHITE);
      // Droopy left leaf
      display.drawLine(cx, 32, cx-18, 39, SSD1306_WHITE);
      display.drawLine(cx-18, 39, cx-13, 40, SSD1306_WHITE);
      // Crack lines on stem (dry effect)
      display.drawPixel(cx+1, 26, SSD1306_BLACK);
      display.drawPixel(cx+1, 34, SSD1306_BLACK);
      break;

    case LEVEL_LOW:
      // Upright stem, small leaves
      display.drawLine(cx, 40, cx, 18, SSD1306_WHITE);
      // Right leaf
      display.drawLine(cx, 28, cx+14, 22, SSD1306_WHITE);
      display.drawLine(cx+14, 22, cx+10, 30, SSD1306_WHITE);
      display.drawLine(cx+10, 30, cx, 30, SSD1306_WHITE);
      // Left leaf
      display.drawLine(cx, 31, cx-14, 25, SSD1306_WHITE);
      display.drawLine(cx-14, 25, cx-10, 33, SSD1306_WHITE);
      display.drawLine(cx-10, 33, cx, 33, SSD1306_WHITE);
      // Small bud top
      display.drawCircle(cx, 15, 4, SSD1306_WHITE);
      display.drawLine(cx-2, 13, cx+2, 13, SSD1306_WHITE);
      break;

    case LEVEL_GOOD:
      // Full healthy plant with big leaves and flower
      display.drawLine(cx, 40, cx, 12, SSD1306_WHITE);
      // Big right leaf
      display.drawLine(cx, 27, cx+22, 16, SSD1306_WHITE);
      display.drawLine(cx+22, 16, cx+18, 28, SSD1306_WHITE);
      display.drawLine(cx+18, 28, cx, 29, SSD1306_WHITE);
      // Big left leaf
      display.drawLine(cx, 30, cx-22, 19, SSD1306_WHITE);
      display.drawLine(cx-22, 19, cx-18, 31, SSD1306_WHITE);
      display.drawLine(cx-18, 31, cx, 32, SSD1306_WHITE);
      // Flower petals
      display.fillCircle(cx,    8, 4, SSD1306_WHITE);
      display.fillCircle(cx-6,  5, 3, SSD1306_WHITE);
      display.fillCircle(cx+6,  5, 3, SSD1306_WHITE);
      display.fillCircle(cx-4, 12, 3, SSD1306_WHITE);
      display.fillCircle(cx+4, 12, 3, SSD1306_WHITE);
      // Flower center black
      display.fillCircle(cx, 8, 2, SSD1306_BLACK);
      break;

    case LEVEL_WET:
      // Heavy wide leaves, drooping slightly
      display.drawLine(cx, 40, cx, 14, SSD1306_WHITE);
      // Heavy right leaf
      display.drawLine(cx, 26, cx+24, 20, SSD1306_WHITE);
      display.drawLine(cx+24, 20, cx+20, 32, SSD1306_WHITE);
      display.drawLine(cx+20, 32, cx, 30, SSD1306_WHITE);
      // Heavy left leaf
      display.drawLine(cx, 28, cx-24, 22, SSD1306_WHITE);
      display.drawLine(cx-24, 22, cx-20, 34, SSD1306_WHITE);
      display.drawLine(cx-20, 34, cx, 32, SSD1306_WHITE);
      // Flower
      display.fillCircle(cx, 10, 5, SSD1306_WHITE);
      display.fillCircle(cx, 10, 2, SSD1306_BLACK);
      // Water drops on leaf tips
      display.fillCircle(cx+24, 20, 2, SSD1306_WHITE);
      display.fillCircle(cx-24, 22, 2, SSD1306_WHITE);
      break;

    case LEVEL_SATURATED:
      // Completely drooping, stem bent, drops falling
      display.drawLine(cx, 40, cx, 24, SSD1306_WHITE);
      display.drawLine(cx, 24, cx-3, 18, SSD1306_WHITE);
      // Drooping right leaf heavy
      display.drawLine(cx, 30, cx+22, 39, SSD1306_WHITE);
      display.drawLine(cx+22, 39, cx+18, 40, SSD1306_WHITE);
      // Drooping left leaf heavy
      display.drawLine(cx, 32, cx-22, 40, SSD1306_WHITE);
      // Water drops falling
      display.fillCircle(cx+22, 40, 2, SSD1306_WHITE);
      display.fillCircle(cx-10, 40, 2, SSD1306_WHITE);
      display.fillCircle(cx+10, 38, 2, SSD1306_WHITE);
      // Wilted tip
      display.drawLine(cx-3, 18, cx-7, 13, SSD1306_WHITE);
      display.drawLine(cx-7, 13, cx-5, 11, SSD1306_WHITE);
      break;
  }
}

// Big emotion face drawn ON the pot — large and expressive
// Pot face area: centered x=64, y=53, pot is filled white so draw BLACK
void drawBigEmotion() {
  int cx = 64, cy = 53;

  switch (statusLevel) {

    case LEVEL_DRY:
      // X eyes (distressed)
      display.drawLine(cx-16, cy-6, cx-10, cy, SSD1306_BLACK);
      display.drawLine(cx-10, cy-6, cx-16, cy, SSD1306_BLACK);
      display.drawLine(cx+10, cy-6, cx+16, cy, SSD1306_BLACK);
      display.drawLine(cx+16, cy-6, cx+10, cy, SSD1306_BLACK);
      // Big frown
      display.drawLine(cx-8, cy+5, cx-3, cy+8, SSD1306_BLACK);
      display.drawLine(cx-3, cy+8, cx+3, cy+8, SSD1306_BLACK);
      display.drawLine(cx+3, cy+8, cx+8, cy+5, SSD1306_BLACK);
      // Sweat drops
      display.fillCircle(cx+18, cy-4, 3, SSD1306_BLACK);
      display.fillCircle(cx+20, cy+1, 2, SSD1306_BLACK);
      // Worry lines
      display.drawLine(cx-18, cy-8, cx-12, cy-5, SSD1306_BLACK);
      display.drawLine(cx+12, cy-5, cx+18, cy-8, SSD1306_BLACK);
      break;

    case LEVEL_LOW:
      // Sad dot eyes
      display.fillCircle(cx-12, cy-3, 3, SSD1306_BLACK);
      display.fillCircle(cx+12, cy-3, 3, SSD1306_BLACK);
      // Worried frown
      display.drawLine(cx-8, cy+4, cx-3, cy+2, SSD1306_BLACK);
      display.drawLine(cx-3, cy+2, cx+3, cy+4, SSD1306_BLACK);
      display.drawLine(cx+3, cy+4, cx+8, cy+2, SSD1306_BLACK);
      // Raised worried brows
      display.drawLine(cx-16, cy-9, cx-8,  cy-6, SSD1306_BLACK);
      display.drawLine(cx+8,  cy-6, cx+16, cy-9, SSD1306_BLACK);
      // Single tear
      display.fillCircle(cx-12, cy+2, 2, SSD1306_BLACK);
      break;

    case LEVEL_GOOD:
      // Big happy eyes
      display.fillCircle(cx-12, cy-3, 4, SSD1306_BLACK);
      display.fillCircle(cx+12, cy-3, 4, SSD1306_BLACK);
      // Eye shine
      display.fillCircle(cx-10, cy-5, 1, SSD1306_WHITE);
      display.fillCircle(cx+14, cy-5, 1, SSD1306_WHITE);
      // Big smile
      display.drawLine(cx-10, cy+3, cx-6,  cy+8,  SSD1306_BLACK);
      display.drawLine(cx-6,  cy+8, cx,    cy+10, SSD1306_BLACK);
      display.drawLine(cx,    cy+10,cx+6,  cy+8,  SSD1306_BLACK);
      display.drawLine(cx+6,  cy+8, cx+10, cy+3,  SSD1306_BLACK);
      // Cheek blush
      display.drawLine(cx-20, cy+2, cx-16, cy+4, SSD1306_BLACK);
      display.drawLine(cx-19, cy+3, cx-15, cy+5, SSD1306_BLACK);
      display.drawLine(cx+16, cy+2, cx+20, cy+4, SSD1306_BLACK);
      display.drawLine(cx+15, cy+3, cx+19, cy+5, SSD1306_BLACK);
      break;

    case LEVEL_WET:
      // Flat eyes, one brow raised
      display.fillCircle(cx-12, cy-3, 3, SSD1306_BLACK);
      display.fillCircle(cx+12, cy-3, 3, SSD1306_BLACK);
      // Flat mouth
      display.drawLine(cx-8, cy+5, cx+8, cy+5, SSD1306_BLACK);
      display.drawLine(cx-8, cy+6, cx+8, cy+6, SSD1306_BLACK);
      // One raised eyebrow
      display.drawLine(cx-16, cy-9, cx-7, cy-6, SSD1306_BLACK);
      display.drawLine(cx-16, cy-8, cx-7, cy-5, SSD1306_BLACK);
      // Water drip from eye
      display.drawLine(cx+12, cy+1, cx+12, cy+4, SSD1306_BLACK);
      display.fillCircle(cx+12, cy+5, 2, SSD1306_BLACK);
      break;

    case LEVEL_SATURATED:
      // Spiral dizzy eyes
      display.drawCircle(cx-12, cy-3, 5, SSD1306_BLACK);
      display.drawCircle(cx-12, cy-3, 3, SSD1306_BLACK);
      display.fillCircle(cx-12, cy-3, 1, SSD1306_BLACK);
      display.drawCircle(cx+12, cy-3, 5, SSD1306_BLACK);
      display.drawCircle(cx+12, cy-3, 3, SSD1306_BLACK);
      display.fillCircle(cx+12, cy-3, 1, SSD1306_BLACK);
      // Wavy overwhelmed mouth
      display.drawLine(cx-9, cy+4,  cx-5, cy+8,  SSD1306_BLACK);
      display.drawLine(cx-5, cy+8,  cx,   cy+5,  SSD1306_BLACK);
      display.drawLine(cx,   cy+5,  cx+5, cy+8,  SSD1306_BLACK);
      display.drawLine(cx+5, cy+8,  cx+9, cy+4,  SSD1306_BLACK);
      // Water drops around face
      display.fillCircle(cx-22, cy-2, 2, SSD1306_BLACK);
      display.fillCircle(cx+22, cy-2, 2, SSD1306_BLACK);
      display.fillCircle(cx,    cy-10, 2, SSD1306_BLACK);
      break;
  }
}

// Status label at top of plant screen
void drawPlantStatusLabel() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  const char* lbl = getStatusLabel();
  // Left: moisture %
  display.setCursor(0, 0);
  display.print(moisturePct);
  display.print(F("%"));
  // Center: status
  int lx = (128 - strlen(lbl) * 6) / 2;
  display.setCursor(lx, 0);
  display.print(lbl);
  // Right: channel name small
  display.setCursor(88, 0);
  display.print(F("MARADI"));
}

// ══════════════════════════════════════════════════════════════
//  SCREEN 3 — Gauge / Meter Screen
// ══════════════════════════════════════════════════════════════
void drawGaugeScreen() {
  // Title
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 0);
  display.print(F("MOISTURE GAUGE"));

  // Gauge arc center
  int cx = 64, cy = 52, r = 38;

  // Draw arc segments (color zones)
  // We draw filled arc using line segments — 150° to 30° sweep
  // Zones: DRY(red-ish)=150–105, LOW=105–78, GOOD=78–54, WET=54–36, SAT=36–30
  // On monochrome OLED we draw with dotted/solid patterns

  // Outer arc (thick)
  for (float a = 30; a <= 150; a += 1.5) {
    float rad = a * PI / 180.0;
    int x1 = cx - (int)(r * cos(rad));
    int y1 = cy - (int)(r * sin(rad));
    int x2 = cx - (int)((r - 5) * cos(rad));
    int y2 = cy - (int)((r - 5) * sin(rad));

    // Zone pattern
    if (a >= 120) {
      // DRY zone — dotted (every other pixel)
      if ((int)a % 3 == 0) display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    } else if (a >= 90) {
      // LOW zone — thin solid
      if ((int)a % 2 == 0) display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    } else if (a >= 60) {
      // GOOD zone — fully solid
      display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    } else if (a >= 42) {
      // WET zone — thin
      if ((int)a % 2 == 0) display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    } else {
      // SAT zone — dotted
      if ((int)a % 3 == 0) display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
    }
  }

  // Tick marks at 0,25,50,75,100%
  int tickAngles[] = {150, 120, 90, 60, 30};
  const char* tickLabels[] = {"0", "25", "50", "75", "100"};
  int labelOffsetX[] = {-12, -8, -4, 2, 2};
  int labelOffsetY[] = {-4, -10, -14, -10, -4};

  for (int t = 0; t < 5; t++) {
    float rad = tickAngles[t] * PI / 180.0;
    int tx1 = cx - (int)((r + 1) * cos(rad));
    int ty1 = cy - (int)((r + 1) * sin(rad));
    int tx2 = cx - (int)((r - 7) * cos(rad));
    int ty2 = cy - (int)((r - 7) * sin(rad));
    display.drawLine(tx1, ty1, tx2, ty2, SSD1306_WHITE);
    display.setCursor(tx1 + labelOffsetX[t], ty1 + labelOffsetY[t]);
    display.setTextSize(1);
    display.print(tickLabels[t]);
  }

  // Zone labels
  display.setTextSize(1);
  display.setCursor(2,  36); display.print(F("DRY"));
  display.setCursor(100, 36); display.print(F("WET"));

  // Needle (smooth animated)
  float needleRad = needleAngle * PI / 180.0;
  int nx = cx - (int)((r - 8) * cos(needleRad));
  int ny = cy - (int)((r - 8) * sin(needleRad));
  // Needle line (thick — draw 3 lines close)
  display.drawLine(cx, cy, nx, ny, SSD1306_WHITE);
  display.drawLine(cx + 1, cy, nx + 1, ny, SSD1306_WHITE);
  display.drawLine(cx, cy + 1, nx, ny + 1, SSD1306_WHITE);

  // Center pivot circle
  display.fillCircle(cx, cy, 4, SSD1306_WHITE);
  display.fillCircle(cx, cy, 2, SSD1306_BLACK);

  // Percentage readout box
  display.drawRect(45, 54, 38, 11, SSD1306_WHITE);
  display.fillRect(46, 55, 36, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  int valX = (moisturePct < 10) ? 56 : (moisturePct < 100) ? 50 : 47;
  display.setCursor(valX, 56);
  display.print(moisturePct);
  display.print(F("%"));
  display.setTextColor(SSD1306_WHITE);

  // Status label below gauge
  const char* lbl = getStatusLabel();
  int lx = (128 - strlen(lbl) * 6) / 2;
  display.setCursor(lx, 56);
  // Only if not overlapping value box
  // We already have the box — skip, use status icon instead
  // Draw status symbol on left
  display.setCursor(2, 56);
  switch (statusLevel) {
    case LEVEL_DRY:       display.print(F("!! DRY")); break;
    case LEVEL_LOW:       display.print(F("! LOW")); break;
    case LEVEL_GOOD:      display.print(F("OK"));    break;
    case LEVEL_WET:       display.print(F("WET"));   break;
    case LEVEL_SATURATED: display.print(F("FLOOD")); break;
  }
}

// ── Mode indicator dots ───────────────────────────────────────
void drawModeIndicator() {
  // Three small dots bottom right
  for (int i = 0; i < TOTAL_MODES; i++) {
    int dx = 116 + i * 5;
    int dy = 61;
    if (i == currentMode)
      display.fillCircle(dx, dy, 2, SSD1306_WHITE);
    else
      display.drawCircle(dx, dy, 2, SSD1306_WHITE);
  }
}

// ── Helpers ───────────────────────────────────────────────────
const char* getStatusLabel() {
  switch (statusLevel) {
    case LEVEL_DRY:       return "DRY!";
    case LEVEL_LOW:       return "LOW";
    case LEVEL_GOOD:      return "GOOD";
    case LEVEL_WET:       return "WET";
    case LEVEL_SATURATED: return "FLOOD";
    default:              return "----";
  }
}

// ── Boot Splash ───────────────────────────────────────────────
void showSplash() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);

  // Plant icon
  display.drawLine(64, 50, 64, 35, SSD1306_WHITE);
  display.fillCircle(64, 30, 6, SSD1306_WHITE);
  display.fillCircle(58, 26, 4, SSD1306_WHITE);
  display.fillCircle(70, 26, 4, SSD1306_WHITE);
  display.fillCircle(64, 30, 3, SSD1306_BLACK);

  display.setTextSize(1);
  display.setCursor(14, 42);
  display.print(F("SOIL MOISTURE"));
  display.setCursor(20, 52);
  display.print(F("Press BTN to switch"));

  display.display();
}
