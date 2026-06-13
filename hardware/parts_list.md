# Parts List — The Genesis Radar

Complete Bill of Materials for full 5-module build.

## Core Electronics

| Component | Model | Qty | Source | Est. AUD |
|---|---|---|---|---|
| ESP32-S3 Touchscreen | JC3248W535C 3.5" IPS 480×320 | 1 | AliExpress | $33 |
| UWB Module | DW3000 breakout board | 5 | AliExpress/Makerfabs | $45 each = $225 |
| SPI CS Decoder | 74HC138 3-to-8 line decoder | 2 | AliExpress/Element14 | $1 each = $2 |

## Antennas

| Component | Model | Qty | Source | Est. AUD |
|---|---|---|---|---|
| Vivaldi Antenna | Wideband 3-10GHz UWB SMA | 5 | AliExpress | $12 each = $60 |
| Antenna Cable | SMA male to U.FL/IPEX pigtail | 5 | AliExpress | $2 each = $10 |

## Power

| Component | Model | Qty | Source | Est. AUD |
|---|---|---|---|---|
| Power Bank | USB-C 10000mAh slim | 1 | Any | $25 |
| USB-C Cable | Short 20cm | 2 | Any | $3 |
| LDO Regulator | AMS1117-3.3V | 5 | AliExpress | $0.50 each = $3 |

## Mechanical

| Component | Model | Qty | Source | Est. AUD |
|---|---|---|---|---|
| Carbon Fibre Tube | 20×20mm square 300mm | 4 | AliExpress | $8 each = $32 |
| PETG Filament | 1kg spool | 1 | AliExpress/local | $35 |
| M4 Stainless Bolt | 40mm + nut (hinge pivot) | 8 | Bunnings | $6 |
| M3 Standoffs | 10mm + 20mm brass | 20 | AliExpress | $5 |
| M3 Screws + Nuts | Assorted pack | 1 | Bunnings | $5 |
| Ball detent spring | 4mm spring ball | 8 | AliExpress | $3 |

## Electronics Sundries

| Component | Qty | Source | Est. AUD |
|---|---|---|---|
| Decoupling caps 100nF | 20 | AliExpress | $2 |
| Silicone wire 28AWG | 1m × 4 colours | AliExpress | $5 |
| PCB proto board | 2 | AliExpress | $3 |
| JST connectors 4-pin | 10 pairs | AliExpress | $4 |
| Heat shrink assorted | 1 pack | Bunnings | $5 |
| Solder 60/40 | 1 roll | Bunnings | $8 |
| Flux pen | 1 | Bunnings | $6 |

## Total Cost Summary

| Category | AUD |
|---|---|
| Core electronics | $260 |
| Antennas + cables | $70 |
| Power | $31 |
| Mechanical | $86 |
| Sundries | $33 |
| **TOTAL** | **~$480** |

## Budget Build (3 modules, no case)

Start with minimum viable build to prove concept:

| Component | AUD |
|---|---|
| ESP32-S3 display | $33 |
| DW3000 × 3 (1 Tx + 2 Rx) | $135 |
| Antennas × 3 | $36 |
| 74HC138 | $2 |
| Basic wiring + power | $40 |
| **TOTAL** | **~$246** |

## Sourcing Notes

- **DW3000 modules** — search AliExpress for "DW3000 UWB module"
  or buy direct from Makerfabs (makerfabs.com)
- **Vivaldi antennas** — search AliExpress "Vivaldi antenna UWB SMA 3-10GHz"
- **74HC138** — Element14 Australia stocks for next day delivery
- **Carbon fibre tube** — search AliExpress "carbon fibre square tube 20mm"
- **PETG filament** — local 3D printing supplier or AliExpress

## 3D Printer (Optional — For Production)

If building multiple units:

| Printer | Cost AUD | Notes |
|---|---|---|
| Bambu Lab A1 Mini | ~$450 | Recommended — fast, reliable |
| Creality Ender 3 V3 | ~$320 | Budget option |

Break even on printer after 1 unit sold at $1000.
Filament cost per enclosure: ~$4 AUD.
