# 🔭 The Genesis Radar

> *The first open-source multi-modal sensor fusion system for through-wall biometric identification.*

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Status: Active Development](https://img.shields.io/badge/Status-Active%20Development-blue.svg)]()
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-red.svg)]()
[![Author: simonsayspeace](https://img.shields.io/badge/Author-simonsayspeace-orange.svg)](https://github.com/simonsayspeace)

---

## What Is The Genesis Radar?

The Genesis Radar is an independent research project developing the world's first 
low-cost, open-source multi-modal sensor fusion system capable of through-wall 
human detection and biometric identification.

It combines multiple sensing technologies into a single unified mathematical 
framework — the more sensor inputs, the higher the resolution and identification 
confidence.

**Built by an independent Australian researcher. No university. No funding. No lab.**

---

## The Core Insight

Every sensor modality adds another dimension of biometric information:

```
CSI WiFi Mapping    →  Human silhouette + movement pattern
UWB Through-Wall    →  Precise X/Y/Z position + micro-Doppler
Facial Recognition  →  Identity confirmation
Voice Recognition   →  Audio biometric signature
─────────────────────────────────────────────────────────────
Fusion Algorithm    →  Biometric identification through walls
```

Each additional input narrows the identity cone toward a single individual —
achieving biometric identification without subject cooperation, line of sight,
or physical contact.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  GENESIS RADAR SYSTEM                    │
├──────────────┬──────────────┬──────────────┬────────────┤
│  UWB RADAR   │  CSI WiFi    │  FACIAL      │  VOICE     │
│  MODULE      │  MAPPING     │  RECOGNITION │  RECOGNITION│
│              │              │              │            │
│  DW3000 x5   │  802.11n CSI │  CV Pipeline │  NLP Audio │
│  ESP32-S3    │  Android App │              │            │
├──────────────┴──────────────┴──────────────┴────────────┤
│              UNIFIED FUSION ALGORITHM                    │
│         Bayesian Multi-Modal Sensor Fusion               │
│    P(ID|all) = P(uwb) × P(csi) × P(face) × P(voice)   │
├─────────────────────────────────────────────────────────┤
│                  GENESIS DASHBOARD                       │
│        Real-time tactical display + avatar overlay       │
└─────────────────────────────────────────────────────────┘
```

---

## Hardware — The Genesis Radar Device

### Specifications

| Parameter | Value |
|---|---|
| UWB Modules | 5× Qorvo DW3000 |
| Centre Frequency | 6.5 GHz |
| Antennas | 5× Vivaldi wideband (3-10 GHz) |
| Controller | ESP32-S3 (8MB PSRAM, 16MB Flash) |
| Display | 3.5" IPS capacitive touchscreen 480×320 |
| Form Factor | Foldable cross/drone frame (Mavic Mini style) |
| Deployed Size | 880×880mm |
| Folded Size | ~140×140×90mm |
| Weight | ~580g |
| Power | USB-C 10000mAh power bank |
| Battery Life | ~6-8 hours |
| Penetration | 3-5m through single brick, 1-2m double brick |
| Position Accuracy | ±0.35m at 40cm baseline |

### Array Configuration

```
         [Rx3 TOP]
              |
            40cm
              |
[Rx1]──40cm──[Tx]──40cm──[Rx2]
        LEFT   |   RIGHT
             40cm
              |
         [Rx4 BOTTOM]

[SCREEN] mounted on back face of centre hub
```

### Display Panels

```
┌─────────────────────────┬──────────────┐
│                         │  RADAR SWEEP │
│   THROUGH-WALL          │  half circle │
│   X-RAY VIEW            ├──────────────┤
│   forward facing        │   COMPASS    │
│   human blob targets    ├──────────────┤
│   colour coded by range │   TOP VIEW   │
└─────────────────────────┴──────────────┘
```

Target colours:
- 🔴 **RED** — close range (under 3m)
- 🟡 **YELLOW** — mid range (3-6m)
- 🟢 **GREEN** — far range (over 6m)

---

## Software Components

### 1. UWB Radar Firmware (ESP32-S3 Arduino)
- DW3000 5-module array initialisation
- CIR (Channel Impulse Response) capture
- Background subtraction (static clutter removal)
- 3D multilateration (X/Y/Z target positioning)
- LovyanGFX tactical display rendering
- Touchscreen calibration controls

### 2. CSI WiFi Mapping (Android)
- 802.11n Channel State Information extraction
- Human silhouette reconstruction
- Skeletal avatar overlay
- Real-time movement tracking

### 3. Fusion Algorithm (In Development)
- Bayesian multi-modal sensor fusion
- Biometric feature vector construction
- Identity confidence scoring
- Multi-target tracking

### 4. Genesis Dashboard
- Unified display of all sensor modalities
- Through-wall avatar overlay
- Target identification and tracking
- Tactical situational awareness

---

## Repository Structure

```
genesis-radar/
│
├── README.md                    ← You are here
├── LICENSE                      ← MIT License
├── CONTRIBUTING.md              ← How to contribute
├── PRIOR_ART.md                 ← Establishes invention timeline
│
├── hardware/
│   ├── parts_list.md            ← Complete BOM with costs
│   ├── wiring_diagram.html      ← Interactive wiring diagram
│   ├── array_geometry.md        ← Antenna array specifications
│   └── 3d_prints/               ← STL files (coming soon)
│
├── firmware/
│   ├── stage1/
│   │   └── uwb_radar_stage1.ino ← DW3000 init + CIR capture
│   └── stage2/
│       └── uwb_radar_stage2.ino ← LovyanGFX display pipeline
│
├── software/
│   ├── csi_wifi/
│   │   └── README.md            ← CSI WiFi mapping app docs
│   ├── fusion_algorithm/
│   │   └── README.md            ← Fusion algorithm (in dev)
│   └── dashboard/
│       └── radar_tactical.html  ← Tactical display demo
│
├── research/
│   ├── references.md            ← Academic references
│   ├── theory.md                ← Signal processing theory
│   └── results/                 ← Experimental results
│
├── docs/
│   ├── getting_started.md       ← Build guide
│   ├── calibration.md           ← Calibration procedure
│   └── troubleshooting.md       ← Common issues
│
└── media/
    ├── wiring_diagram.png
    ├── device_renders/
    └── demo_videos/
```

---

## Prior Art Declaration

This repository was created on **June 2026** by **simonsayspeace** (GitHub).

The multi-modal sensor fusion architecture described herein — combining UWB radar,
CSI WiFi mapping, facial recognition, and voice recognition into a unified 
biometric identification framework — represents original independent research.

All concepts, implementations, and architectural designs documented here were
conceived and developed independently without institutional support, funding,
or collaboration.

**This public repository establishes prior art. Any subsequent patent claims
on this specific combination of technologies and architectural approach must
acknowledge this prior art.**

---

## Why Open Source?

This technology has significant implications. Making it open source means:

1. **Prior art is established** — no corporation can patent what is already public
2. **Community can improve it** — better algorithms, better hardware
3. **Ethical oversight** — open development invites scrutiny
4. **Knowledge belongs to everyone** — not locked behind paywalls or defence contracts

---

## Current Status

- [x] UWB radar firmware Stage 1 — DW3000 init + CIR capture
- [x] UWB radar firmware Stage 2 — LovyanGFX tactical display
- [x] CSI WiFi mapping Android app
- [x] Tactical display demo
- [ ] Hardware build — parts ordered
- [ ] Stage 3 — touchscreen controls
- [ ] Stage 4 — fusion algorithm v1
- [ ] Stage 5 — biometric identification
- [ ] 3D print files

---

## Author

**simonsayspeace** — Independent researcher, Australia.

*"The best research happens when you have nothing to lose and everything to prove."*

---

## License

MIT License — free to use, modify, and distribute with attribution.

See [LICENSE](LICENSE) for full terms.

---

## Contributing

Issues, pull requests, and research contributions welcome.

If this project has value to you — consider starring ⭐ the repository.
It costs nothing and helps others find it.
