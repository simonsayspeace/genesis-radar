/*
 * ============================================================
 *  UWB Through-Wall Radar — Stage 2
 *  LovyanGFX Display Driver + Live Radar Pipeline
 *  ESP32-S3 JC3248W535C 3.5" IPS Touchscreen
 * ============================================================
 *  Install LovyanGFX via Arduino Library Manager before
 *  compiling this file.
 *
 *  This file builds on Stage 1 (uwb_radar_stage1.ino)
 *  Add both files to the same Arduino sketch folder.
 *
 *  Board settings:
 *    Board:          ESP32S3 Dev Module
 *    Flash Size:     16MB
 *    PSRAM:          OPI PSRAM
 *    USB CDC On Boot:Enabled
 * ============================================================
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// ── JC3248W535C Display Configuration ────────────────────────
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_RGB   _panel_instance;
  lgfx::Touch_GT911 _touch_instance;

public:
  LGFX(void) {
    // ── RGB Bus (parallel 16-bit) ─────────────────────────
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      // RGB data pins
      cfg.pin_d0  = GPIO_NUM_8;   // B0
      cfg.pin_d1  = GPIO_NUM_3;   // B1
      cfg.pin_d2  = GPIO_NUM_46;  // B2
      cfg.pin_d3  = GPIO_NUM_9;   // B3
      cfg.pin_d4  = GPIO_NUM_1;   // B4
      cfg.pin_d5  = GPIO_NUM_5;   // G0
      cfg.pin_d6  = GPIO_NUM_6;   // G1
      cfg.pin_d7  = GPIO_NUM_7;   // G2
      cfg.pin_d8  = GPIO_NUM_15;  // G3
      cfg.pin_d9  = GPIO_NUM_16;  // G4
      cfg.pin_d10 = GPIO_NUM_4;   // G5
      cfg.pin_d11 = GPIO_NUM_45;  // R0
      cfg.pin_d12 = GPIO_NUM_48;  // R1
      cfg.pin_d13 = GPIO_NUM_47;  // R2
      cfg.pin_d14 = GPIO_NUM_21;  // R3
      cfg.pin_d15 = GPIO_NUM_14;  // R4

      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_42;
      cfg.freq_write  = 16000000;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch  = 8;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch  = 8;
      cfg.pclk_active_neg   = 1;
      cfg.de_idle_high      = 0;
      cfg.pclk_idle_high    = 0;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    // ── Panel ─────────────────────────────────────────────
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 480;
      cfg.memory_height = 320;
      cfg.panel_width   = 480;
      cfg.panel_height  = 320;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      _panel_instance.config(cfg);
    }

    // ── Touch (GT911 I2C) ─────────────────────────────────
    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = 479;
      cfg.y_min      = 0;
      cfg.y_max      = 319;
      cfg.pin_int    = GPIO_NUM_38;
      cfg.pin_rst    = GPIO_NUM_NC;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port   = I2C_NUM_0;
      cfg.i2c_addr   = 0x5D;
      cfg.pin_sda    = GPIO_NUM_19;
      cfg.pin_scl    = GPIO_NUM_20;
      cfg.freq       = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

// ── Display instance ──────────────────────────────────────────
static LGFX lcd;

// ── Screen dimensions ─────────────────────────────────────────
#define SCR_W  480
#define SCR_H  320

// ── Sprite buffers (double buffering for smooth animation) ────
// Using PSRAM for large sprites
static lgfx::LGFX_Sprite sprite(&lcd);      // Main frame buffer
static lgfx::LGFX_Sprite spriteRadar(&lcd); // Half-circle radar buffer

// ── Colour palette (16-bit 565 format) ───────────────────────
#define COL_BG        lcd.color565(2,   4,  12)
#define COL_BG_RADAR  lcd.color565(0,   8,   4)
#define COL_GRID      lcd.color565(12,  30,  50)
#define COL_WALL      lcd.color565(20,  60, 120)
#define COL_FLOOR     lcd.color565(15,  50,  90)
#define COL_BRACKET   lcd.color565(40, 120, 200)
#define COL_TEXT      lcd.color565(100,160,200)
#define COL_LABEL     lcd.color565(40, 140, 220)
#define COL_GREEN     lcd.color565(0,  220,  80)
#define COL_SWEEP     lcd.color565(0,  200,  60)
#define COL_WHITE     lcd.color565(255,255, 255)
#define COL_RED_LED   lcd.color565(220,  40,  10)
#define COL_COMPASS_N lcd.color565(220,  40,  10)
#define COL_COMPASS   lcd.color565(40,  120, 200)

// Target colours by range
uint16_t targetColour(float dist) {
  if (dist < 3.0f) return lcd.color565(255, 60,  20);   // RED   — close
  if (dist < 6.0f) return lcd.color565(255,200,   0);   // YELLOW — mid
  return              lcd.color565( 40, 255,  80);       // GREEN  — far
}

// ── Layout zones ──────────────────────────────────────────────
#define FW         310   // Forward view width
#define FH         SCR_H // Forward view height
#define RX         313   // Right panel start X
#define RW         167   // Right panel width
#define RADAR_H    168   // Half radar panel height
#define COMPASS_H   72   // Compass panel height
#define TOPVIEW_H   80   // Top view panel height (320-168-72=80)

// ── Radar sweep state ─────────────────────────────────────────
float sweepAngle   = 180.0f;  // degrees, sweeps 180→0→180
float sweepDir     = -2.5f;   // degrees per frame
uint32_t frameNum  = 0;

// ── Calibration state (shown on screen) ──────────────────────
bool showCalPrompt = false;
uint32_t calStartMs = 0;

// ── Touch state ───────────────────────────────────────────────
int32_t touchX = -1, touchY = -1;
bool    touched = false;

// ── Forward declarations ──────────────────────────────────────
void drawForwardView();
void drawHalfRadar();
void drawCompass();
void drawTopView();
void drawStatusBar();
void drawCalibrationOverlay();
void handleTouch();

// ── Draw forward X-ray view (left panel 0,0 to FW,FH) ────────
void drawForwardView() {
  // Background
  sprite.fillRect(0, 0, FW, FH, COL_BG);

  // Subtle scanlines — X-ray feel
  for (int y = 0; y < FH; y += 3) {
    sprite.drawFastHLine(0, y, FW, lcd.color565(4, 10, 20));
  }

  // Wall reference line (vertical centre)
  sprite.drawFastVLine(FW/2, 0, FH, COL_WALL);

  // Floor line
  int floorY = (int)(FH * 0.87f);
  sprite.drawFastHLine(0, floorY, FW, COL_FLOOR);
  sprite.setTextColor(COL_FLOOR);
  sprite.setTextSize(1);
  sprite.setCursor(4, floorY - 8);
  sprite.print("FLOOR");

  // Centre vertical reference (faint)
  sprite.drawFastVLine(FW/2, 0, FH, lcd.color565(8, 25, 45));

  // Draw each active target as human blob
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (!targets[i].active) continue;

    float dist  = targets[i].distance;
    float tx    = targets[i].x;  // -4 to +4 metres
    float tz    = targets[i].z;  // 0 to 2 metres height

    uint16_t col = targetColour(dist);

    // Map to screen coordinates
    int sx = (int)((tx / 4.0f) * (FW/2 - 20) + FW/2);
    int topY   = (int)(FH * 0.06f);
    int sz_top = (int)(floorY - (tz / 2.0f) * (floorY - topY));

    // Size inversely proportional to distance
    int blobH = max(18, (int)(80 - dist * 5.5f));
    int blobW = (int)(blobH * 0.36f);

    // ── Head ─────────────────────────────────────────────
    int headY = sz_top - blobH + blobW;
    // Outer glow
    for (int r = blobW + 6; r >= blobW; r--) {
      uint8_t alpha = (uint8_t)(40 * (1.0f - (float)(r - blobW) / 6.0f));
      uint16_t glowCol = lcd.color565(
        ((col >> 11) & 0x1F) * alpha / 40,
        ((col >> 5)  & 0x3F) * alpha / 40,
        ((col)       & 0x1F) * alpha / 40
      );
      sprite.drawCircle(sx, headY, r, glowCol);
    }
    sprite.fillCircle(sx, headY, blobW, col);

    // ── Torso ─────────────────────────────────────────────
    int torsoY  = sz_top - (int)(blobH * 0.48f);
    int torsoH  = (int)(blobH * 0.35f);
    int torsoW  = (int)(blobW * 1.15f);
    // Glow
    for (int r = 3; r >= 0; r--) {
      sprite.drawEllipse(sx, torsoY, torsoW + r, torsoH + r,
        lcd.color565(
          min(255, ((col>>11)&0x1F)*4 * (4-r)/4),
          min(255, ((col>>5) &0x3F)*2 * (4-r)/4),
          min(255, ((col)    &0x1F)*4 * (4-r)/4)
        ));
    }
    sprite.fillEllipse(sx, torsoY, torsoW, torsoH, col);

    // ── Legs ──────────────────────────────────────────────
    int legsY = sz_top - (int)(blobH * 0.12f);
    sprite.fillEllipse(sx, legsY, blobW, (int)(blobH * 0.25f), col);

    // ── Core bright centre ────────────────────────────────
    sprite.fillCircle(sx, torsoY, 3, COL_WHITE);

    // ── Data tag ──────────────────────────────────────────
    sprite.setTextColor(col);
    sprite.setTextSize(1);
    sprite.setCursor(sx + blobW + 4, headY - 4);
    sprite.printf("T%d", i + 1);
    sprite.setCursor(sx + blobW + 4, headY + 5);
    sprite.printf("%.1fm", dist);

    // Range indicator bar (left edge)
    int barY = 20 + i * 30;
    sprite.fillRect(4, barY, 6, 20, col);
    sprite.setTextColor(col);
    sprite.setCursor(12, barY + 6);
    sprite.printf("T%d %.1fm", i+1, dist);
  }

  // ── Corner brackets ───────────────────────────────────────
  int m = 6, l = 14;
  sprite.drawFastHLine(m,     m,     l, COL_BRACKET);
  sprite.drawFastVLine(m,     m,     l, COL_BRACKET);
  sprite.drawFastHLine(FW-m-l,m,     l, COL_BRACKET);
  sprite.drawFastVLine(FW-m,  m,     l, COL_BRACKET);
  sprite.drawFastHLine(m,     FH-m,  l, COL_BRACKET);
  sprite.drawFastVLine(m,     FH-m-l,l, COL_BRACKET);
  sprite.drawFastHLine(FW-m-l,FH-m,  l, COL_BRACKET);
  sprite.drawFastVLine(FW-m,  FH-m-l,l, COL_BRACKET);

  // ── Label ─────────────────────────────────────────────────
  sprite.setTextColor(COL_LABEL);
  sprite.setTextSize(1);
  sprite.setCursor(10, 8);
  sprite.print("THROUGH-WALL  X-RAY  VIEW");

  // ── Legend ────────────────────────────────────────────────
  sprite.fillCircle(8,  FH-32, 4, lcd.color565(255,60,20));
  sprite.fillCircle(8,  FH-20, 4, lcd.color565(255,200,0));
  sprite.fillCircle(8,  FH-8,  4, lcd.color565(40,255,80));
  sprite.setTextColor(COL_TEXT);
  sprite.setCursor(16, FH-36); sprite.print("<3m");
  sprite.setCursor(16, FH-24); sprite.print("3-6m");
  sprite.setCursor(16, FH-12); sprite.print(">6m");

  // Divider
  sprite.drawFastVLine(FW+1, 0, FH, lcd.color565(20, 45, 80));
}

// ── Draw half-circle radar (top right panel) ──────────────────
void drawHalfRadar() {
  int cx = RX + RW/2;
  int cy = RADAR_H - 10;
  int r  = 72;

  sprite.fillRect(RX, 0, RW, RADAR_H, COL_BG_RADAR);

  sprite.setTextColor(COL_GREEN);
  sprite.setTextSize(1);
  sprite.setCursor(RX + 28, 6);
  sprite.print("RADAR SWEEP");

  // Range rings
  for (int ri = 1; ri <= 3; ri++) {
    int rr = (r * ri) / 3;
    // Draw half arc manually
    for (int a = 0; a <= 180; a += 2) {
      float rad = a * DEG_TO_RAD;
      int x1 = cx + (int)(rr * cos(rad + PI));
      int y1 = cy - (int)(rr * sin(rad));
      sprite.drawPixel(x1, y1, lcd.color565(0, 50+ri*20, 20+ri*10));
    }
    // Range label
    sprite.setTextColor(lcd.color565(0, 80, 40));
    sprite.setCursor(cx + rr + 2, cy - 4);
    sprite.printf("%dm", ri * 3);
  }

  // Spoke lines
  for (int a = 0; a <= 180; a += 30) {
    float rad = a * DEG_TO_RAD;
    int x2 = cx + (int)(r * cos(rad + PI));
    int y2 = cy - (int)(r * sin(rad));
    sprite.drawLine(cx, cy, x2, y2, lcd.color565(0, 25, 12));
  }

  // Sweep trail
  for (int t = 0; t < 25; t++) {
    float trailAngle = sweepAngle + t * 3.0f;
    if (trailAngle > 180) trailAngle = 180;
    float rad = trailAngle * DEG_TO_RAD;
    uint8_t alpha = (uint8_t)(60 * (1.0f - (float)t / 25.0f));
    int x2 = cx + (int)(r * cos(rad + PI));
    int y2 = cy - (int)(r * sin(rad));
    sprite.drawLine(cx, cy, x2, y2, lcd.color565(0, alpha, alpha/3));
  }

  // Active sweep line
  float sweepRad = sweepAngle * DEG_TO_RAD;
  int swx = cx + (int)(r * cos(sweepRad + PI));
  int swy = cy - (int)(r * sin(sweepRad));
  sprite.drawLine(cx, cy, swx, swy, COL_SWEEP);

  // Base line
  sprite.drawFastHLine(cx - r, cy, r * 2, lcd.color565(0, 80, 40));

  // Sensor dot
  sprite.fillCircle(cx, cy, 4, COL_WHITE);

  // Target blips
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (!targets[i].active) continue;
    float dist = targets[i].distance;
    float tx   = targets[i].x;
    float angle = atan2f(tx, targets[i].y) * RAD_TO_DEG;
    float blipAngle = 90.0f - angle;
    blipAngle = constrain(blipAngle, 0, 180);
    float blipR = min(dist / 10.0f, 1.0f) * r;
    float brad = blipAngle * DEG_TO_RAD;
    int bx = cx + (int)(blipR * cos(brad + PI));
    int by = cy - (int)(blipR * sin(brad));
    uint16_t col = targetColour(dist);
    // Glow
    sprite.fillCircle(bx, by, 6, lcd.color565(
      min(255,((col>>11)&0x1F)*3),
      min(255,((col>>5)&0x3F)*1),
      min(255,((col)&0x1F)*3)
    ));
    sprite.fillCircle(bx, by, 3, col);
  }

  // Half circle border
  for (int a = 0; a <= 180; a += 1) {
    float rad = a * DEG_TO_RAD;
    int x2 = cx + (int)(r * cos(rad + PI));
    int y2 = cy - (int)(r * sin(rad));
    sprite.drawPixel(x2, y2, lcd.color565(0, 100, 50));
  }

  // Divider
  sprite.drawFastHLine(RX, RADAR_H, RW, lcd.color565(20,45,80));
}

// ── Draw compass (middle right panel) ────────────────────────
void drawCompass() {
  int y0 = RADAR_H;
  int cx = RX + RW/2;
  int cy = y0 + COMPASS_H/2;
  int r  = 26;

  sprite.fillRect(RX, y0, RW, COMPASS_H, lcd.color565(0, 3, 8));
  sprite.setTextColor(COL_LABEL);
  sprite.setTextSize(1);
  sprite.setCursor(RX + 42, y0 + 4);
  sprite.print("COMPASS");

  // Ring
  for (int a = 0; a < 360; a += 5) {
    float rad = a * DEG_TO_RAD;
    sprite.drawPixel(
      cx + (int)(r * cosf(rad)),
      cy + (int)(r * sinf(rad)),
      lcd.color565(20, 60, 100)
    );
  }

  // Tick marks
  for (int a = 0; a < 360; a += 45) {
    float rad = a * DEG_TO_RAD;
    sprite.drawLine(
      cx + (int)((r-4)*cosf(rad)), cy + (int)((r-4)*sinf(rad)),
      cx + (int)(r*cosf(rad)),     cy + (int)(r*sinf(rad)),
      lcd.color565(40, 100, 160)
    );
  }

  // Cardinal labels
  sprite.setTextColor(COL_COMPASS_N); sprite.setCursor(cx-3, cy-r-10); sprite.print("N");
  sprite.setTextColor(COL_COMPASS);
  sprite.setCursor(cx+r+3, cy-3);  sprite.print("E");
  sprite.setCursor(cx-3,   cy+r+2); sprite.print("S");
  sprite.setCursor(cx-r-9, cy-3);  sprite.print("W");

  // North needle
  sprite.drawLine(cx, cy, cx, cy-r+2, COL_COMPASS_N);
  sprite.fillTriangle(cx-3, cy-r+8, cx+3, cy-r+8, cx, cy-r, COL_COMPASS_N);

  // South needle
  sprite.drawLine(cx, cy, cx, cy+r-2, COL_COMPASS);

  // Centre dot
  sprite.fillCircle(cx, cy, 3, COL_WHITE);

  // Heading readout
  sprite.setTextColor(COL_TEXT);
  sprite.setCursor(RX+52, y0 + COMPASS_H - 10);
  sprite.print("HDG: 000\xB0");

  sprite.drawFastHLine(RX, y0+COMPASS_H, RW, lcd.color565(20,45,80));
}

// ── Draw top-down plan view (bottom right panel) ──────────────
void drawTopView() {
  int y0  = RADAR_H + COMPASS_H;
  int ph  = SCR_H - y0;
  int pcx = RX + RW/2;
  int pcy = y0 + ph/2;
  float scaleM = (ph * 0.4f) / 10.0f;

  sprite.fillRect(RX, y0, RW, ph, COL_BG);
  sprite.setTextColor(COL_LABEL);
  sprite.setTextSize(1);
  sprite.setCursor(RX + 40, y0 + 4);
  sprite.print("TOP  VIEW");

  // Grid
  for (int gi = -2; gi <= 2; gi++) {
    sprite.drawFastVLine(pcx + (int)(gi*scaleM*2), y0+2, ph-4, COL_GRID);
    sprite.drawFastHLine(RX+2, pcy + (int)(gi*scaleM*2), RW-4, COL_GRID);
  }

  // Wall line
  int wallY = pcy - (int)(scaleM * 1.5f);
  sprite.drawFastHLine(RX+4, wallY, RW-8, COL_WALL);
  sprite.setTextColor(COL_WALL);
  sprite.setCursor(RX + RW/2 - 10, wallY - 8);
  sprite.print("WALL");

  // Sensor (you)
  sprite.fillCircle(pcx, pcy, 4, COL_WHITE);
  sprite.setTextColor(lcd.color565(80,80,80));
  sprite.setCursor(pcx - 6, pcy + 7);
  sprite.print("YOU");

  // Targets with trails
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (!targets[i].active) continue;
    float dist = targets[i].distance;
    uint16_t col = targetColour(dist);
    int tx2 = pcx + (int)(targets[i].x * scaleM);
    int ty2 = pcy - (int)(targets[i].y * scaleM);
    if (ty2 < y0+4 || ty2 > y0+ph-4) continue;
    // Glow
    sprite.fillCircle(tx2, ty2, 6, lcd.color565(
      min(255,((col>>11)&0x1F)*2),
      min(255,((col>>5)&0x3F)*1),
      min(255,((col)&0x1F)*2)
    ));
    sprite.fillCircle(tx2, ty2, 3, col);
  }
}

// ── Draw status bar (very bottom of forward view) ─────────────
void drawStatusBar() {
  int y0 = FH - 14;
  sprite.fillRect(0, y0, FW, 14, lcd.color565(3, 8, 16));
  sprite.drawFastHLine(0, y0, FW, lcd.color565(15, 35, 65));
  sprite.setTextColor(COL_TEXT);
  sprite.setTextSize(1);

  // Frame counter
  sprite.setCursor(4, y0 + 3);
  sprite.printf("FRM:%05lu", frameNum);

  // Calibration status
  sprite.setCursor(80, y0 + 3);
  if (baselineCalibrated) {
    sprite.setTextColor(COL_GREEN);
    sprite.print("CAL:OK");
  } else {
    sprite.setTextColor(lcd.color565(220,80,0));
    sprite.print("CAL:--");
  }

  // Active modules
  sprite.setTextColor(COL_TEXT);
  sprite.setCursor(140, y0 + 3);
  sprite.print("RX:4/4");

  // Frequency
  sprite.setCursor(190, y0 + 3);
  sprite.print("6.5GHz");

  // Time
  sprite.setCursor(240, y0 + 3);
  uint32_t s = millis() / 1000;
  sprite.printf("%02lu:%02lu", s/60, s%60);

  // Touch indicator
  if (touched) {
    sprite.setTextColor(lcd.color565(255,200,0));
    sprite.setCursor(285, y0 + 3);
    sprite.printf("TCH:%d,%d", touchX, touchY);
  }
}

// ── Calibration overlay ───────────────────────────────────────
void drawCalibrationOverlay() {
  if (!showCalPrompt) return;

  uint32_t elapsed = (millis() - calStartMs) / 1000;
  uint32_t remaining = 3 > elapsed ? 3 - elapsed : 0;

  // Semi-transparent overlay
  sprite.fillRect(FW/4, FH/3, FW/2, FH/3, lcd.color565(0, 10, 25));
  sprite.drawRect(FW/4, FH/3, FW/2, FH/3, COL_BRACKET);

  sprite.setTextColor(COL_WHITE);
  sprite.setTextSize(1);
  sprite.setCursor(FW/4 + 20, FH/3 + 12);
  sprite.print("CALIBRATING...");

  sprite.setTextColor(lcd.color565(255,200,0));
  sprite.setCursor(FW/4 + 10, FH/3 + 28);
  sprite.print("CLEAR AREA BEHIND WALL");

  // Progress bar
  uint32_t progress = ((millis() - calStartMs) * (FW/2 - 20)) / 3000;
  progress = min(progress, (uint32_t)(FW/2 - 20));
  sprite.fillRect(FW/4 + 10, FH/3 + 45, progress, 8, COL_GREEN);
  sprite.drawRect(FW/4 + 10, FH/3 + 45, FW/2 - 20, 8, COL_BRACKET);

  sprite.setTextColor(COL_GREEN);
  sprite.setCursor(FW/4 + 55, FH/3 + 58);
  sprite.printf("%lu%%", (millis()-calStartMs)*100/3000);

  if (millis() - calStartMs > 3000) {
    showCalPrompt = false;
    calibrateBaseline(50);
  }
}

// ── Handle touchscreen input ──────────────────────────────────
void handleTouch() {
  touched = lcd.getTouch(&touchX, &touchY);
  if (!touched) return;

  // CAL button area — top right corner of right panel
  if (touchX > RX + RW - 50 && touchY < 20) {
    showCalPrompt = true;
    calStartMs    = millis();
  }
}

// ── Display setup ─────────────────────────────────────────────
void displaySetup() {
  Serial.println("Initialising display...");
  lcd.init();
  lcd.setRotation(1);  // Landscape
  lcd.setBrightness(200);
  lcd.fillScreen(TFT_BLACK);

  // Allocate sprite in PSRAM
  sprite.setColorDepth(16);
  sprite.createSprite(SCR_W, SCR_H);

  Serial.println("Display initialised ✓");

  // Splash screen
  lcd.fillScreen(lcd.color565(2, 4, 12));
  lcd.setTextColor(lcd.color565(40, 140, 220));
  lcd.setTextSize(2);
  lcd.setCursor(80, 100);
  lcd.print("UWB THROUGH-WALL");
  lcd.setCursor(160, 126);
  lcd.print("RADAR v1.0");
  lcd.setTextSize(1);
  lcd.setTextColor(lcd.color565(0, 180, 80));
  lcd.setCursor(140, 160);
  lcd.print("Initialising modules...");
  delay(2000);
}

// ── Draw complete frame ───────────────────────────────────────
void drawFrame() {
  // Clear sprite
  sprite.fillSprite(COL_BG);

  // Draw all panels into sprite
  drawForwardView();
  drawHalfRadar();
  drawCompass();
  drawTopView();
  drawStatusBar();
  drawCalibrationOverlay();

  // Push sprite to display in one shot (no flicker)
  sprite.pushSprite(0, 0);

  // Advance radar sweep
  sweepAngle += sweepDir;
  if (sweepAngle <= 0 || sweepAngle >= 180) sweepDir = -sweepDir;

  frameNum++;
}

// ── Setup (add to existing setup() or call from it) ──────────
void displayAndRadarSetup() {
  Serial.begin(115200);
  delay(300);

  // Display first so user sees splash
  displaySetup();

  // Then init radar hardware
  // (calls initAllModules() from Stage 1)
  if (!initAllModules()) {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_RED);
    lcd.setTextSize(2);
    lcd.setCursor(60, 140);
    lcd.print("MODULE INIT FAILED");
    lcd.setTextSize(1);
    lcd.setCursor(80, 170);
    lcd.print("Check wiring and power supply");
    while(1) delay(1000);
  }

  // Show calibration prompt
  lcd.fillScreen(lcd.color565(2,4,12));
  lcd.setTextColor(lcd.color565(255,200,0));
  lcd.setTextSize(1);
  lcd.setCursor(60, 140);
  lcd.print("CLEAR AREA BEHIND WALL");
  lcd.setCursor(80, 155);
  lcd.print("Calibrating in 3 seconds...");
  delay(3000);

  calibrateBaseline(50);

  lcd.fillScreen(lcd.color565(2,4,12));
  lcd.setTextColor(lcd.color565(0,220,80));
  lcd.setCursor(140, 155);
  lcd.print("RADAR ACTIVE");
  delay(500);
}

// ── Main setup() ─────────────────────────────────────────────
void setup() {
  displayAndRadarSetup();
}

// ── Main loop() ──────────────────────────────────────────────
void loop() {
  // Handle touch input
  handleTouch();

  // Capture radar frame
  bool frameOk = captureFrame();

  if (frameOk && baselineCalibrated) {
    subtractBaseline();
    multilaterateTargets();
  }

  // Draw display frame
  drawFrame();

  // ~20fps
  delay(50);
}
