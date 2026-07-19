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

![Amplifier stage schematic](amp-stage.svg)

Gain = 1 + 22k/1.2k ≈ 19.3 (25.7 dB); the AC-coupled feedback leg makes DC
gain unity, so OUT self-centers at VREF.

Component notes — corners chosen for 5 Hz drive, not audio:

- **C_in = 4.7 µF** (≥50 V; film, or electrolytic + toward the amp). With the
  22 k bias impedance the corner is 1.5 Hz. (A 470 nF "audio" value would put
  the corner at 15 Hz and lose 10 dB at the 5 Hz crawl.) This cap is also what
  aligns the levels: the Arduino side sits at 2.5 V DC, the amp side at
  VREF ≈ 26 V — the cap absorbs the difference, only the AC swing passes.
- **Feedback cap = 220 µF / 50 V, + toward IN−**: in single-supply operation it
  charges to VREF (≈26 V), hence the voltage rating; 1.2 k × 220 µF puts the
  gain corner at 0.6 Hz. (The datasheet's 22 µF would roll gain off below 6 Hz.)
- Both channels share the same corner frequencies, so whatever phase shift the
  coupling networks add is identical in both — the 90° quadrature is preserved.

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

## Position sensing under this drive

Grounding wire 5 (motor common **and** pot wiper) would pin the relay-era
wiper-on-A0 reading to 0 V. Rewire the sensing at the terminal block:

- 5 V → 820 Ω (doubles as the first filter resistor) → A0 node → **tower wire 2**
- tower wire 3: spare (optionally a second divider to A1 later — the two
  segments must sum to the ≈1 048 Ω track, a free cable-health check)
- tower wire 5: GND (star point), as this schematic requires

The divider's hyperbolic V(R) is inverted exactly in firmware
(`position_ohms()`), and R is linear in angle — no difference amplifier
needed. R_fixed = 820 Ω (near-optimal worst-case slope, ~18% more span than
1.2 k): expected span ≈ 0.57–2.61 V for the measured 106–896 Ω swing.
Recalibrate `O`/`F` after the rewire (calibration is stored in ohms now).

## Bring-up checklist

1. Boost at ~30 V, no load, both TDA7294 muted: check VREF ≈ Vs/2, OUT ≈ Vs/2.
2. 22 Ω dummy loads, 60 Hz drive: trim both channels to equal amplitude;
   scope for oscillation (Zobel fitted), feel heatsink. (22 Ω ≈ |Z| of a
   winding at 60 Hz — measured 20 V / 1 A, mostly inductive; the 6 Ω figure
   is DC resistance only and is not the operating point.)
3. Boost to 52 V, repeat; verify ~18 V RMS, mute sequencing (no thump).
4. Motor: starting behaviour at 5–15 Hz, reversal, winding currents
   (expect symmetric ~1 A), motor + heatsink temperature after full rotations.
5. RX noise check on the HF bands while rotating.
