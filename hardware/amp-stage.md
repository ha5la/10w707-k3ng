# Quadrature amplifier stage — two TDA7294, single supply, perfboard

Drives the 10W707's two motor windings with sin/cos from the firmware2 DDS.
References: ST TDA7294 datasheet (April 2003) — Figure 1 (typical application,
our base circuit), Figure 16 (turn on/off sequence — implemented by firmware2's
MUTE/STBY sequencing), Figure 17 (single-signal mute/stby — we drive them
separately instead). Note: the TDA7294 datasheet itself has *no* single-supply
figure; the single-supply pattern below follows the TDA7293 datasheet's
"single supply amplifier" configuration, adapted.

## Supply

13.8 V input → DX200W-6 boost set to **52 V** → both channels.
The chip sees 52 V total (= ±26 V equivalent; allowed 20–80 V).
Bring-up: start with the boost at ~30 V, raise to 52 V when clean.

## TDA7294 pinout (Multiwatt15, top view; TAB = −Vs → bolts bare to grounded heatsink)

| Pin | Function | Our connection |
|-----|----------|----------------|
| 1 | STBY-GND | GND |
| 2 | IN− | feedback network |
| 3 | IN+ | signal in (biased to VREF) |
| 4 | IN+MUTE | VREF (same DC as IN+ → no step when muting) |
| 5, 11, 12 | N.C. | — |
| 6 | BOOTSTRAP | 22 µF to OUT |
| 7 | +Vs (signal) | +52 V |
| 8 | −Vs (signal) | GND |
| 9 | ST-BY | 22 kΩ ← Arduino D7 line (+10 µF at pin to GND) |
| 10 | MUTE | 10 kΩ ← Arduino D6 line (+10 µF at pin to GND) |
| 13 | +PWVs (power) | +52 V |
| 14 | OUT | bootstrap, feedback, Zobel, output cap |
| 15 | −PWVs (power) | GND |

## Per-channel schematic (build two; identical except the DDS pin and winding)

```
                 +52 V ──┬── 1000 µF ─ GND          (bulk, at pins 7+13)
                         ├── 100 nF ─ GND
                         ├──────────────── pins 7, 13
                         │
                        22k
                         ├────────────────  VREF  (≈ 26 V) ── pin 4
                        22k                  │
                         │                  47 µF
                        GND                  │
                                            GND

  from DDS ──── 10k trim ──── 470 nF ───┬── pin 3 (IN+)
  (D9 or D10 →                          │
   1.2k + 100 nF RC)                   22k
                                        │
                                      VREF

  pin 2 (IN−) ──┬── 22k ─────────────────────┐   gain = 1 + 22k/1.2k
                │                            │        ≈ 19.3 (25.7 dB)
                └── 1.2k ── 22 µF ── GND     │   DC gain = 1 → OUT sits at VREF
                                             │
  pin 6 ── 22 µF (bootstrap) ──┬─────────────┤
                               │             │
  pin 14 (OUT) ────────────────┴─────────────┴──┬── + 4700 µF/63 V ──► winding hot
                                                │        (tower wire 1 or 4)
                                                └── 10 Ω ── 100 nF ── GND  (Zobel)

  pins 1, 8, 15 ── GND
  pin 9 (STBY) ── 22k ── STBY line (Arduino D7)     10 µF from pin 9 to GND
  pin 10 (MUTE) ── 10k ── MUTE line (Arduino D6)    10 µF from pin 10 to GND
```

## Shared connections

| Net | Connection |
|-----|-----------|
| GND | boost −out, both channels, motor common (tower wire 5 + shield), Arduino GND |
| MUTE line | Arduino D6 → both pin-10 resistors; 10 kΩ pull-down to GND |
| STBY line | Arduino D7 → both pin-9 resistors; 10 kΩ pull-down to GND |
| Channel A in | Arduino D9 (sin) via 1.2 kΩ + 100 nF RC |
| Channel B in | Arduino D10 (cos) via 1.2 kΩ + 100 nF RC |
| Channel A out | tower wire 1 (winding A) |
| Channel B out | tower wire 4 (winding B) |

Star-ground at the boost output: power returns (Zobel, output caps' loads,
bulk caps) and signal returns (VREF divider, feedback caps, input networks)
meet at one point. Keep the two 4700 µF caps' return path short.

## Levels

DDS fundamental after RC ≈ 1.77 V RMS max → trimmer to ≈ 0.93 V RMS →
×19.3 → **18 V RMS** at the winding at 60 Hz (design flux, 0.3 V/Hz).
Set each channel's trimmer on the dummy load; match the two channels.

The output cap + 20 Ω winding form a 1.7 Hz high-pass: at the 5 Hz crawl this
costs ~0.5 dB and ~19° phase — equal in both channels, so the 90° quadrature
between them is preserved.

## Bring-up checklist

1. Boost at ~30 V, no load, both TDA7294 muted: check VREF ≈ Vs/2, OUT ≈ Vs/2.
2. 22 Ω dummy loads, 60 Hz drive: trim both channels to equal amplitude;
   scope for oscillation (Zobel fitted), feel heatsink.
3. Boost to 52 V, repeat; verify ~18 V RMS, mute sequencing (no thump).
4. Motor: starting behaviour at 5–15 Hz, reversal, winding currents
   (expect symmetric ~1 A), motor + heatsink temperature after full rotations.
5. RX noise check on the HF bands while rotating.
