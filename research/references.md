# Research References — The Genesis Radar

## Related Academic Work

### UWB Through-Wall Radar

- Amin, M.G. (Ed.) (2011). *Through-the-Wall Radar Imaging*. CRC Press.
- Soldovieri, F., et al. (2012). "Microwave Tomography for Through-the-Wall Imaging." 
  *IEEE Geoscience and Remote Sensing Letters.*
- Withington, P., et al. (1999). "Enhanced through-wall radar for law enforcement."
  *Enforcement and Security Technologies, SPIE.*

### CSI WiFi Sensing

- Adib, F., Katabi, D. (2013). "See Through Walls with WiFi."
  *ACM SIGCOMM.* MIT CSAIL.
- Zhao, M., et al. (2018). "Through-Wall Human Pose Estimation Using Radio Signals."
  *IEEE CVPR.* MIT CSAIL. (RF-Pose)
- Wang, W., et al. (2015). "Understanding and Modeling of WiFi Signal Based Human
  Activity Recognition." *ACM MobiCom.*

### Sensor Fusion

- Hall, D.L., Llinas, J. (1997). "An introduction to multisensor data fusion."
  *Proceedings of the IEEE.*
- Castanedo, F. (2013). "A Review of Data Fusion Techniques."
  *The Scientific World Journal.*

### Biometric Identification

- Gafurov, D. (2007). "A Survey of Biometric Gait Recognition: Approaches,
  Security and Challenges." *Norwegian Information Security Conference.*
- Li, C., et al. (2019). "Radio Frequency Fingerprinting for Device Identification."
  *IEEE Transactions.*

### Passive Radar

- Griffiths, H.D., Baker, C.J. (2005). "Passive coherent location radar systems."
  *IEE Proceedings Radar, Sonar and Navigation.*

---

## The Genesis Radar Novelty

What distinguishes this work from prior art:

1. **Integration** — no prior work combines UWB + CSI + facial + voice into
   single unified framework on commodity hardware

2. **Cost** — prior implementations require $10,000-$150,000 hardware.
   Genesis Radar targets ~$400-500 AUD

3. **Form factor** — foldable handheld cross-array with integrated display
   is novel physical design

4. **Open source** — all prior comparable systems are proprietary or classified

5. **Fusion algorithm** — mathematical framework for combining all modalities
   into single biometric confidence score is original work

---

## Signal Processing Theory

### Channel Impulse Response (CIR)

The DW3000 accumulator provides complex CIR samples:

```
CIR(t) = Σ aₙ · δ(t - τₙ) · e^(jφₙ)
```

Where:
- aₙ = amplitude of nth multipath component
- τₙ = time delay of nth component
- φₙ = phase of nth component

### Background Subtraction

Static clutter removal:

```
Target(t) = Live(t) - Baseline
```

Where Baseline is the average of N frames captured in empty environment.

### Multilateration

3D position from Time Difference of Arrival (TDOA):

```
X position: x = (TDOA_h × (d1 + d2)) / (2 × baseline_h)
Z position: z = 1.0 + (TDOA_v × (d3 + d4)) / (2 × baseline_v)
Y position: y = mean(d1, d2, d3, d4)
```

### Bayesian Sensor Fusion

Combined identity probability:

```
P(ID | sensors) = P(uwb) × P(csi) × P(face) × P(voice)
                  ────────────────────────────────────────
                              P(sensors)
```

Each modality multiplies confidence toward certainty.
