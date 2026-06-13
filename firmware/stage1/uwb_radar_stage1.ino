/*
 * ============================================================
 *  UWB Through-Wall Radar — Stage 1
 *  DW3000 5-Module Array Initialisation & CIR Capture
 * ============================================================
 *  Hardware:
 *    ESP32-S3  (JC3248W535C 3.5" touchscreen)
 *    DW3000 x5 (1 Tx + 4 Rx) via 74HC138 CS decoder
 *    Vivaldi antennas x5
 *
 *  SPI wiring:
 *    GPIO11 → CLK   (all modules shared)
 *    GPIO13 → MOSI  (all modules shared)
 *    GPIO12 → MISO  (all modules shared)
 *    GPIO4  → 74HC138 A0
 *    GPIO5  → 74HC138 A1
 *    GPIO6  → 74HC138 A2
 *    GPIO7  → DW3000 Tx  IRQ
 *    GPIO8  → DW3000 Rx1 IRQ
 *    GPIO9  → DW3000 Rx2 IRQ
 *    GPIO10 → DW3000 Rx3 IRQ
 *    GPIO14 → DW3000 Rx4 IRQ
 * ============================================================
 */

#include <SPI.h>
#include <Arduino.h>

// ── Pin definitions ──────────────────────────────────────────
#define PIN_SPI_CLK   11
#define PIN_SPI_MOSI  13
#define PIN_SPI_MISO  12

// 74HC138 address lines — select which DW3000 CS is active
#define PIN_CS_A0     4
#define PIN_CS_A1     5
#define PIN_CS_A2     6

// Individual IRQ pins per module
#define PIN_IRQ_TX    7
#define PIN_IRQ_RX1   8
#define PIN_IRQ_RX2   9
#define PIN_IRQ_RX3   10
#define PIN_IRQ_RX4   14

// ── Module index constants ───────────────────────────────────
#define MOD_TX   0   // 74HC138 Y0
#define MOD_RX1  1   // 74HC138 Y1
#define MOD_RX2  2   // 74HC138 Y2
#define MOD_RX3  3   // 74HC138 Y3
#define MOD_RX4  4   // 74HC138 Y4

// ── DW3000 Register addresses ────────────────────────────────
// Fast access registers
#define DW3000_REG_DEV_ID       0x00   // Device ID — should return 0xDECA0302
#define DW3000_REG_SYS_CFG      0x04   // System configuration
#define DW3000_REG_SYS_TIME     0x06   // System time counter
#define DW3000_REG_TX_FCTRL     0x24   // TX frame control
#define DW3000_REG_SYS_CTRL     0x26   // System control
#define DW3000_REG_SYS_STATUS   0x07   // System status
#define DW3000_REG_RX_TIME      0x14   // RX timestamp
#define DW3000_REG_TX_TIME      0x17   // TX timestamp
#define DW3000_REG_CIR_PTR      0x29   // CIR memory pointer
#define DW3000_REG_ACC_MEM      0x15   // Accumulator memory (CIR data)
#define DW3000_REG_CHAN_CTRL    0x19   // Channel control (frequency, PRF)
#define DW3000_REG_AGC_CTRL    0x36   // AGC control
#define DW3000_REG_DRX_CONF    0x27   // Digital receiver config
#define DW3000_REG_RF_CONF     0x28   // RF configuration
#define DW3000_REG_TX_CAL      0x2A   // TX calibration
#define DW3000_REG_FS_CTRL     0x2B   // Frequency synthesiser control
#define DW3000_REG_AON         0x29   // Always-on register
#define DW3000_REG_OTP_IF      0x2C   // OTP interface
#define DW3000_REG_LDE_CTRL    0x2E   // LDE algorithm control
#define DW3000_REG_DIG_DIAG    0x3A   // Digital diagnostics

// SYS_STATUS bit masks
#define SYS_STATUS_TXFRS       (1UL << 7)   // TX frame sent
#define SYS_STATUS_RXDFR       (1UL << 13)  // RX data frame ready
#define SYS_STATUS_RXFCE       (1UL << 14)  // RX FCS error
#define SYS_STATUS_RXPHE       (1UL << 12)  // RX PHR error
#define SYS_STATUS_LDEDONE     (1UL << 10)  // LDE processing done
#define SYS_STATUS_RXOVRR      (1UL << 20)  // RX buffer overrun

// ── CIR data structure ───────────────────────────────────────
#define CIR_SAMPLES     1016   // DW3000 accumulator length
#define CIR_SAMPLE_SIZE 4      // 4 bytes per complex sample (I+Q 16-bit each)

struct CIRSample {
  int16_t real;
  int16_t imag;
};

struct CIRFrame {
  CIRSample samples[CIR_SAMPLES];
  uint64_t  timestamp;      // DW3000 RX timestamp (high precision)
  uint8_t   moduleId;
  bool      valid;
};

// ── Radar data structures ────────────────────────────────────
struct RadarTarget {
  float x;           // metres left/right  (-4 to +4)
  float y;           // metres depth       (0 to 10)
  float z;           // metres height      (0 to 2)
  float confidence;  // 0.0 to 1.0
  float distance;    // direct range metres
  bool  active;
};

#define MAX_TARGETS 8

// ── Global state ─────────────────────────────────────────────
CIRFrame  cirData[5];           // CIR frames from each module
CIRFrame  baseline[5];          // Background baseline (empty room)
float     cirMagnitude[5][CIR_SAMPLES];   // Magnitude after subtraction
RadarTarget targets[MAX_TARGETS];

bool      baselineCalibrated = false;
uint32_t  frameCount         = 0;
uint32_t  lastPulseTime      = 0;
bool      irqFlags[5]        = {false};

SPIClass  vspi(VSPI);

// ── 74HC138 CS selection ─────────────────────────────────────
// Sets A0/A1/A2 to select module 0-4
// 74HC138 outputs are active LOW — perfect for DW3000 CS
void selectModule(uint8_t moduleId) {
  digitalWrite(PIN_CS_A0, (moduleId >> 0) & 1);
  digitalWrite(PIN_CS_A1, (moduleId >> 1) & 1);
  digitalWrite(PIN_CS_A2, (moduleId >> 2) & 1);
  delayMicroseconds(1);  // Setup time for 74HC138
}

// Deselect all — set address to 7 (Y7 = unused output)
void deselectAll() {
  digitalWrite(PIN_CS_A0, HIGH);
  digitalWrite(PIN_CS_A1, HIGH);
  digitalWrite(PIN_CS_A2, HIGH);
}

// ── Low level SPI register read/write ───────────────────────
// DW3000 SPI transaction format:
//   Byte 0: [RW bit][6-bit reg addr][extended flag]
//   Byte 1: (if extended) sub-register offset
//   Then:   data bytes

uint32_t dw3000ReadReg32(uint8_t moduleId, uint8_t reg) {
  selectModule(moduleId);
  vspi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  vspi.transfer(reg & 0x3F);  // Read: bit7=0, reg addr bits 6:0
  uint32_t val = 0;
  val  = (uint32_t)vspi.transfer(0x00);
  val |= (uint32_t)vspi.transfer(0x00) << 8;
  val |= (uint32_t)vspi.transfer(0x00) << 16;
  val |= (uint32_t)vspi.transfer(0x00) << 24;

  vspi.endTransaction();
  deselectAll();
  return val;
}

void dw3000WriteReg32(uint8_t moduleId, uint8_t reg, uint32_t value) {
  selectModule(moduleId);
  vspi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  vspi.transfer(0x80 | (reg & 0x3F));  // Write: bit7=1
  vspi.transfer((value >>  0) & 0xFF);
  vspi.transfer((value >>  8) & 0xFF);
  vspi.transfer((value >> 16) & 0xFF);
  vspi.transfer((value >> 24) & 0xFF);

  vspi.endTransaction();
  deselectAll();
}

uint64_t dw3000ReadReg40(uint8_t moduleId, uint8_t reg) {
  selectModule(moduleId);
  vspi.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  vspi.transfer(reg & 0x3F);
  uint64_t val = 0;
  for (int i = 0; i < 5; i++) {
    val |= (uint64_t)vspi.transfer(0x00) << (i * 8);
  }

  vspi.endTransaction();
  deselectAll();
  return val;
}

// ── Read CIR accumulator memory ──────────────────────────────
// CIR data is in the accumulator memory register
// Each sample is 4 bytes: 16-bit real + 16-bit imaginary
void dw3000ReadCIR(uint8_t moduleId, CIRFrame &frame) {
  selectModule(moduleId);
  vspi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  // Set accumulator memory address (sub-register access)
  vspi.transfer(0x40 | (DW3000_REG_ACC_MEM & 0x3F));
  vspi.transfer(0x00);  // Sub-register offset 0
  vspi.transfer(0x00);  // Dummy byte required by DW3000

  for (int i = 0; i < CIR_SAMPLES; i++) {
    uint8_t lo, hi;
    lo = vspi.transfer(0x00);
    hi = vspi.transfer(0x00);
    frame.samples[i].real = (int16_t)(lo | (hi << 8));
    lo = vspi.transfer(0x00);
    hi = vspi.transfer(0x00);
    frame.samples[i].imag = (int16_t)(lo | (hi << 8));
  }

  vspi.endTransaction();
  deselectAll();

  frame.moduleId = moduleId;
  frame.valid    = true;
  frame.timestamp = dw3000ReadReg40(moduleId, DW3000_REG_RX_TIME);
}

// ── DW3000 soft reset ────────────────────────────────────────
void dw3000Reset(uint8_t moduleId) {
  // Write reset via SYS_CTRL
  dw3000WriteReg32(moduleId, DW3000_REG_SYS_CTRL, 0x01000000);
  delay(5);
  dw3000WriteReg32(moduleId, DW3000_REG_SYS_CTRL, 0x00000000);
  delay(5);
}

// ── DW3000 initialise one module ─────────────────────────────
bool dw3000Init(uint8_t moduleId, bool isTx) {
  Serial.printf("  Initialising module %d (%s)...\n", moduleId, isTx?"TX":"RX");

  // Check device ID
  uint32_t devId = dw3000ReadReg32(moduleId, DW3000_REG_DEV_ID);
  Serial.printf("  Device ID: 0x%08X ", devId);

  if ((devId & 0xFFFF0000) != 0xDECA0000) {
    Serial.println("❌ FAIL — not a DW3000");
    return false;
  }
  Serial.println("✓ OK");

  // Soft reset
  dw3000Reset(moduleId);

  // ── Channel configuration ─────────────────────────────────
  // Channel 5 = 6.5GHz centre frequency (best for penetration)
  // PRF = 64MHz (higher SNR)
  // Preamble length = 1024 (longer = better range through walls)
  // Data rate = 850kbps
  uint32_t chanCtrl = 0;
  chanCtrl |= (5 << 0);   // TX channel 5
  chanCtrl |= (5 << 4);   // RX channel 5
  chanCtrl |= (2 << 18);  // PRF 64MHz
  dw3000WriteReg32(moduleId, DW3000_REG_CHAN_CTRL, chanCtrl);

  // ── System configuration ──────────────────────────────────
  uint32_t sysCfg = 0;
  sysCfg |= (1 << 1);   // PHR mode — extended frames
  sysCfg |= (1 << 22);  // RX auto-re-enable after error
  if (!isTx) {
    sysCfg |= (1 << 6); // Receiver double buffering
  }
  dw3000WriteReg32(moduleId, DW3000_REG_SYS_CFG, sysCfg);

  // ── TX frame control (Tx module only) ────────────────────
  if (isTx) {
    uint32_t txFCtrl = 0;
    txFCtrl |= (127 << 0);   // Frame length = 127 bytes (max standard)
    txFCtrl |= (0   << 13);  // Data rate 110kbps (best penetration)
    txFCtrl |= (2   << 15);  // PRF 64MHz
    txFCtrl |= (8   << 18);  // Preamble length 1024
    dw3000WriteReg32(moduleId, DW3000_REG_TX_FCTRL, txFCtrl);
  }

  // ── LDE algorithm — Leading Edge Detection ────────────────
  // Critical for accurate TOA measurement through walls
  uint16_t ldeCtrl = 0x1607;  // Default LDE config
  dw3000WriteReg32(moduleId, DW3000_REG_LDE_CTRL, ldeCtrl);

  // ── AGC — Auto Gain Control ───────────────────────────────
  // Helps receiver handle attenuated signals through brick
  dw3000WriteReg32(moduleId, DW3000_REG_AGC_CTRL, 0x8D8);

  // ── Digital receiver config ───────────────────────────────
  dw3000WriteReg32(moduleId, DW3000_REG_DRX_CONF, 0x000A0120);

  Serial.printf("  Module %d configured as %s ✓\n", moduleId, isTx?"TRANSMITTER":"RECEIVER");
  return true;
}

// ── Initialise all 5 modules ─────────────────────────────────
bool initAllModules() {
  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║  UWB Radar Array Initialisation  ║");
  Serial.println("╚══════════════════════════════════╝\n");

  bool allOk = true;

  // Module 0 = Transmitter
  if (!dw3000Init(MOD_TX, true)) {
    Serial.println("❌ TX module failed!");
    allOk = false;
  }

  // Modules 1-4 = Receivers
  for (int i = MOD_RX1; i <= MOD_RX4; i++) {
    if (!dw3000Init(i, false)) {
      Serial.printf("❌ RX%d module failed!\n", i);
      allOk = false;
    }
    delay(10);
  }

  if (allOk) {
    Serial.println("\n✅ All 5 modules initialised successfully");
  } else {
    Serial.println("\n⚠️  Some modules failed — check wiring");
  }

  return allOk;
}

// ── Transmit a UWB pulse ─────────────────────────────────────
void transmitPulse() {
  // Trigger TX on module 0
  uint32_t sysCtrl = dw3000ReadReg32(MOD_TX, DW3000_REG_SYS_CTRL);
  sysCtrl |= (1 << 1);  // TXSTRT bit — start transmission immediately
  dw3000WriteReg32(MOD_TX, DW3000_REG_SYS_CTRL, sysCtrl);
}

// ── Enable receiver on all Rx modules ────────────────────────
void enableAllReceivers() {
  for (int i = MOD_RX1; i <= MOD_RX4; i++) {
    uint32_t sysCtrl = dw3000ReadReg32(i, DW3000_REG_SYS_CTRL);
    sysCtrl |= (1 << 8);  // RXENAB bit — enable receiver
    dw3000WriteReg32(i, DW3000_REG_SYS_CTRL, sysCtrl);
  }
}

// ── Check if module has received data ────────────────────────
bool moduleHasData(uint8_t moduleId) {
  uint32_t status = dw3000ReadReg32(moduleId, DW3000_REG_SYS_STATUS);
  return (status & SYS_STATUS_RXDFR) != 0;
}

bool moduleHasError(uint8_t moduleId) {
  uint32_t status = dw3000ReadReg32(moduleId, DW3000_REG_SYS_STATUS);
  return (status & (SYS_STATUS_RXFCE | SYS_STATUS_RXPHE)) != 0;
}

// Clear RX status flags on a module
void clearRXStatus(uint8_t moduleId) {
  dw3000WriteReg32(moduleId, DW3000_REG_SYS_STATUS,
    SYS_STATUS_RXDFR | SYS_STATUS_RXFCE | SYS_STATUS_RXPHE | SYS_STATUS_RXOVRR);
}

// ── Capture one full radar frame ─────────────────────────────
// Returns true if all 4 receivers got data
bool captureFrame() {
  // Step 1: Enable all receivers first
  enableAllReceivers();
  delayMicroseconds(100);

  // Step 2: Fire TX pulse
  transmitPulse();
  lastPulseTime = micros();

  // Step 3: Wait for TX complete
  uint32_t timeout = micros() + 5000;  // 5ms timeout
  while (!(dw3000ReadReg32(MOD_TX, DW3000_REG_SYS_STATUS) & SYS_STATUS_TXFRS)) {
    if (micros() > timeout) {
      Serial.println("⚠️  TX timeout");
      return false;
    }
  }

  // Step 4: Wait for all receivers to get data
  // UWB propagation at 10m = ~33ns — negligible, main delay is processing
  timeout = micros() + 10000;  // 10ms timeout
  bool rxDone[4] = {false};
  int rxCount = 0;

  while (rxCount < 4 && micros() < timeout) {
    for (int i = 0; i < 4; i++) {
      if (!rxDone[i]) {
        if (moduleHasData(i + 1)) {
          rxDone[i] = true;
          rxCount++;
        } else if (moduleHasError(i + 1)) {
          clearRXStatus(i + 1);
          Serial.printf("⚠️  RX%d error\n", i+1);
        }
      }
    }
  }

  // Step 5: Read CIR from each receiver that got data
  for (int i = 0; i < 4; i++) {
    if (rxDone[i]) {
      dw3000ReadCIR(i + 1, cirData[i + 1]);
      clearRXStatus(i + 1);
    } else {
      cirData[i + 1].valid = false;
    }
  }

  frameCount++;
  return rxCount == 4;
}

// ── Background calibration ───────────────────────────────────
// Call this with an empty room — no people behind the wall
// Captures average of N frames as static baseline
void calibrateBaseline(int numFrames = 50) {
  Serial.println("\n📡 Baseline calibration starting...");
  Serial.println("   Ensure no people are behind the wall!");
  Serial.println("   Capturing 50 frames...");

  // Accumulate samples
  float accumReal[5][CIR_SAMPLES] = {0};
  float accumImag[5][CIR_SAMPLES] = {0};

  for (int f = 0; f < numFrames; f++) {
    captureFrame();
    for (int m = 1; m <= 4; m++) {
      if (cirData[m].valid) {
        for (int s = 0; s < CIR_SAMPLES; s++) {
          accumReal[m][s] += cirData[m].samples[s].real;
          accumImag[m][s] += cirData[m].samples[s].imag;
        }
      }
    }
    delay(20);
    if (f % 10 == 0) Serial.printf("   Frame %d/50\n", f);
  }

  // Average into baseline
  for (int m = 1; m <= 4; m++) {
    for (int s = 0; s < CIR_SAMPLES; s++) {
      baseline[m].samples[s].real = (int16_t)(accumReal[m][s] / numFrames);
      baseline[m].samples[s].imag = (int16_t)(accumImag[m][s] / numFrames);
    }
    baseline[m].valid = true;
  }

  baselineCalibrated = true;
  Serial.println("✅ Baseline calibration complete!\n");
}

// ── Background subtraction ────────────────────────────────────
// Subtracts static baseline from live frame
// Result = moving targets only (wall, furniture removed)
void subtractBaseline() {
  if (!baselineCalibrated) return;

  for (int m = 1; m <= 4; m++) {
    if (!cirData[m].valid) continue;
    for (int s = 0; s < CIR_SAMPLES; s++) {
      float dReal = cirData[m].samples[s].real - baseline[m].samples[s].real;
      float dImag = cirData[m].samples[s].imag - baseline[m].samples[s].imag;
      // Magnitude of complex difference
      cirMagnitude[m][s] = sqrtf(dReal*dReal + dImag*dImag);
    }
  }
}

// ── Find peak in CIR (strongest reflection) ──────────────────
// Returns index of peak and its magnitude
int findPeak(int moduleId, float &peakMag, int startSample = 50) {
  peakMag = 0;
  int peakIdx = startSample;
  // Skip first 50 samples — direct path / antenna coupling
  for (int s = startSample; s < CIR_SAMPLES; s++) {
    if (cirMagnitude[moduleId][s] > peakMag) {
      peakMag = cirMagnitude[moduleId][s];
      peakIdx = s;
    }
  }
  return peakIdx;
}

// ── Convert CIR sample index to distance (metres) ────────────
// DW3000 at 499.2MHz sampling = 2.0ns per sample
// Speed of light = 0.299792m per ns
// Round trip: divide by 2
float sampleToMetres(int sampleIdx) {
  const float SAMPLE_PERIOD_NS = 1.0f / 0.4992f;  // ~2.0ns
  const float C = 0.299792f;                        // m/ns
  float timeNs = sampleIdx * SAMPLE_PERIOD_NS;
  return (timeNs * C) / 2.0f;                       // one-way distance
}

// ── Simple 3D multilateration ─────────────────────────────────
// Array geometry (metres from centre):
//   Rx1 = (-0.35, 0, 0)   left
//   Rx2 = (+0.35, 0, 0)   right
//   Rx3 = (0, 0, +0.225)  top
//   Rx4 = (0, 0, -0.225)  bottom

struct Vec3 { float x, y, z; };

const Vec3 RX_POS[5] = {
  {0,      0, 0},     // Tx (not used for trilat)
  {-0.35f, 0, 0},     // Rx1 left
  {+0.35f, 0, 0},     // Rx2 right
  {0,      0, +0.225f}, // Rx3 top
  {0,      0, -0.225f}, // Rx4 bottom
};

// Threshold below which we ignore a peak (noise floor)
#define MAGNITUDE_THRESHOLD  200.0f
#define MIN_DISTANCE_M       0.3f
#define MAX_DISTANCE_M       10.0f

int multilaterateTargets() {
  // Get peak distance from each receiver
  float dist[5] = {0};
  float peakMag[5] = {0};
  bool  valid[5]   = {false};

  for (int m = 1; m <= 4; m++) {
    int peakIdx = findPeak(m, peakMag[m]);
    dist[m] = sampleToMetres(peakIdx);
    valid[m] = (peakMag[m] > MAGNITUDE_THRESHOLD) &&
               (dist[m] > MIN_DISTANCE_M) &&
               (dist[m] < MAX_DISTANCE_M);
  }

  // Need at least Rx1 and Rx2 for horizontal position
  if (!valid[1] || !valid[2]) return 0;

  // ── Horizontal position (X axis) from Rx1/Rx2 difference ──
  // Time Difference of Arrival (TDOA)
  // d1 = dist to Rx1, d2 = dist to Rx2
  // baseline = 0.70m between Rx1 and Rx2
  float d1 = dist[1], d2 = dist[2];
  float baseline_h = 0.70f;
  float tdoa_h = d2 - d1;  // positive = target right of centre
  float x = (tdoa_h * (d1 + d2)) / (2.0f * baseline_h);
  x = constrain(x, -4.0f, 4.0f);

  // ── Depth (Y axis) from average of all receivers ──────────
  float avgDist = 0; int cnt = 0;
  for (int m = 1; m <= 4; m++) {
    if (valid[m]) { avgDist += dist[m]; cnt++; }
  }
  float y = (cnt > 0) ? avgDist / cnt : 0;

  // ── Height (Z axis) from Rx3/Rx4 difference ───────────────
  float z = 1.0f;  // Default mid height
  if (valid[3] && valid[4]) {
    float d3 = dist[3], d4 = dist[4];
    float baseline_v = 0.45f;
    float tdoa_v = d4 - d3;  // positive = target above centre
    z = 1.0f + (tdoa_v * (d3 + d4)) / (2.0f * baseline_v);
    z = constrain(z, 0.1f, 2.2f);
  }

  // ── Confidence score ───────────────────────────────────────
  float confidence = 0;
  for (int m = 1; m <= 4; m++) {
    if (valid[m]) confidence += 0.25f;
  }

  // Store target
  if (confidence > 0.5f) {
    targets[0].x = x;
    targets[0].y = y;
    targets[0].z = z;
    targets[0].distance   = y;
    targets[0].confidence = confidence;
    targets[0].active     = true;
    return 1;
  }

  targets[0].active = false;
  return 0;
}

// ── IRQ handlers (called on rising edge from DW3000) ─────────
void IRAM_ATTR irqTx()  { irqFlags[0] = true; }
void IRAM_ATTR irqRx1() { irqFlags[1] = true; }
void IRAM_ATTR irqRx2() { irqFlags[2] = true; }
void IRAM_ATTR irqRx3() { irqFlags[3] = true; }
void IRAM_ATTR irqRx4() { irqFlags[4] = true; }

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   UWB Through-Wall Radar v1.0         ║");
  Serial.println("║   5-Module DW3000 Array               ║");
  Serial.println("╚═══════════════════════════════════════╝");

  // ── CS decoder pins ───────────────────────────────────────
  pinMode(PIN_CS_A0, OUTPUT);
  pinMode(PIN_CS_A1, OUTPUT);
  pinMode(PIN_CS_A2, OUTPUT);
  deselectAll();

  // ── IRQ pins ──────────────────────────────────────────────
  pinMode(PIN_IRQ_TX,  INPUT);
  pinMode(PIN_IRQ_RX1, INPUT);
  pinMode(PIN_IRQ_RX2, INPUT);
  pinMode(PIN_IRQ_RX3, INPUT);
  pinMode(PIN_IRQ_RX4, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_TX),  irqTx,  RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_RX1), irqRx1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_RX2), irqRx2, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_RX3), irqRx3, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_IRQ_RX4), irqRx4, RISING);

  // ── SPI bus ───────────────────────────────────────────────
  vspi.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);
  Serial.println("SPI bus initialised");

  // ── Initialise all DW3000 modules ─────────────────────────
  if (!initAllModules()) {
    Serial.println("\n❌ Initialisation failed — halting");
    Serial.println("Check wiring and power supply");
    while(1) delay(1000);
  }

  delay(100);

  // ── Baseline calibration ───────────────────────────────────
  Serial.println("\nStarting baseline calibration in 3 seconds...");
  Serial.println("Make sure NO ONE is behind the wall!\n");
  delay(3000);
  calibrateBaseline(50);

  Serial.println("🚀 Radar pipeline active\n");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {
  // Capture frame from all 5 modules
  bool frameOk = captureFrame();

  if (frameOk && baselineCalibrated) {
    // Remove static reflections (wall, furniture)
    subtractBaseline();

    // Find targets using multilateration
    int numTargets = multilaterateTargets();

    // Print results every 10 frames
    if (frameCount % 10 == 0) {
      Serial.printf("\n── Frame %d ──────────────────\n", frameCount);
      if (numTargets > 0 && targets[0].active) {
        Serial.printf("🎯 TARGET DETECTED\n");
        Serial.printf("   Position: X=%.2fm  Y=%.2fm  Z=%.2fm\n",
          targets[0].x, targets[0].y, targets[0].z);
        Serial.printf("   Distance: %.2fm\n", targets[0].distance);
        Serial.printf("   Confidence: %.0f%%\n", targets[0].confidence * 100);
        Serial.printf("   Range: %s\n",
          targets[0].distance < 3.0f ? "CLOSE (RED)" :
          targets[0].distance < 6.0f ? "MID (YELLOW)" : "FAR (GREEN)");
      } else {
        Serial.println("   No targets detected");
      }
    }
  }

  // Next frame — adjust rate for your needs
  // 50ms = 20 frames/sec — good for moving targets
  delay(50);
}
