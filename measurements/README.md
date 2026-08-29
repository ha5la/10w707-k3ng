# DDS bench verification — 2026-07-19

Scope captures from the firmware2 quadrature-DDS bring-up (README step 1).

**Setup:** Arduino Uno (same ATmega328P as the target Nano), firmware2 @ 3cc9ff4,
D9/D10 through 1.2 kΩ + 100 nF RC filters (f_c ≈ 1.3 kHz), NI VirtualBench
VB-8012 (CH1 = D9/sin, CH2 = D10/cos, digital D6 = MUTE, D7 = STBY), driven
over GS-232 serial. A0 jumpered to GND (CW tests) or 5 V (CCW tests).

| File | What it shows |
|------|---------------|
| `dds-01-idle-carrier.png` | Idle: bare 62.5 kHz Timer1 PWM carrier at 50% duty on both pins (no RC filter yet), D6/D7 in the muted state |
| `dds-02-cw-60hz.png` | CW drive: 60 Hz sines, ~5 Vpp centered on 2.5 V, CH2 (cos) leading CH1 (sin) by 90° |
| `dds-03-ccw-60hz.png` | CCW drive: identical but CH2 lags by 90° — direction reversal by phase flip alone, no relays |
| `dds-04-taper-16hz.png` | Servo taper zone (`M357` with A0 at 5 V): ~16 Hz at ~2 Vpp — the V/f law holding flux constant |
| `dds-05-soft-start.png` | Start ramp: MUTE/STBY release first, then amplitude grows to full in ~0.5 s, quadrature intact throughout |
| `dds-06-soft-stop.png` | Stop ramp (triggered on D6 falling edge): ~0.5 s decay to flat 2.5 V; MUTE/STBY drop only after the amplitude reaches zero |

Setup photos (1936 px wide to match the screenshots, EXIF stripped):

| File | What it shows |
|------|---------------|
| `setup-01-bench-overview.jpg` | The whole bench: VirtualBench VB-8012 with MSO ribbon + logic pod, Linux laptop, Uno + breadboard, station gear behind |
| `setup-02-rc-filters-closeup.jpg` | The two 1.2 kΩ + 100 nF RC filters on the breadboard, scope probe attached |
| `setup-03-uno-wiring.jpg` | Arduino Uno (bench stand-in for the Nano) wired to the filters and probes |

## TDA7294 amplifier bring-up — 2026-08-28/29

Channel A on a 52 V single supply into a 22 Ω 25 W dummy load, driven
by firmware2 over GS-232 serial. CH1 = amplifier output, CH2 as noted. Written
up in `blog/2026-08-29-chasing-a-notch.md`.

| File | What it shows |
|------|---------------|
| `amp-01-30v-clean.png` | First light at a 30 V rail: 14.43 V pk / 10.2 V RMS, 0.56 % deviation from an ideal sine, crest shape matching to 2 % |
| `amp-02-52v-crest-notch.png` | 52 V, 17.7 V RMS: a 2 V, 0.5 ms V-notch at every crest, second horn higher than the first (1.9 % residual) |
| `amp-03-pwm-carrier.png` | 100 µs/div on the rising slope: 62.5 kHz Timer1 carrier surviving the single input RC pole, 2.96 V p-p — smallest at the crest, as d·(1−d) predicts |
| `amp-04-rail-burst-at-boost.png` | CH2 AC-coupled at the boost output: 15 V p-p, 0.5 ms burst of ringing at exactly the notch, plus ~1 V of 60 Hz sag from the half-wave load |
| `amp-05-rail-clean-at-chip.png` | Decoupling moved onto pins 13/15: ripple at the chip 0.88 V p-p worst case, against 1.6 V p-p there beforehand — the 14 V p-p burst is a converter-terminal phenomenon, not a chip-side one. Crest notch unchanged at −1.67 V: the negative result that redirected the hunt |
| `amp-06-bootstrap-constant.png` | CH1 pin 14, CH2 pin 6, both DC at 20 V/div: the gap holds 14.75–15.05 V right through the crest, ruling out bootstrap collapse |
| `amp-07-fixed-17v5.png` | 4700 µF at the boost output: crest deviation −0.14 V, residual 0.69 %, 24.74 V pk = 17.5 V RMS, rail sag at the chip 0.93 V p-p |

Amplifier setup photos:

| File | What it shows |
|------|---------------|
| `amp-setup-01-channel-a-dummy-load.jpg` | Channel A perfboard on the bench: TDA7294 bolted bare to its heatsink, output cap, gold dummy-load resistor, scope probe on the output |
| `amp-setup-02-perfboard-closeup.jpg` | Close-up: 1000 µF rail bulk, level trimmer, input network, Zobel and feedback parts |
| `amp-setup-03-bench-overview.jpg` | The whole bench: VirtualBench VB-8012, laptop on the serial port, Uno + breadboard driving the amplifier board |

All PNGs pngquant-compressed (`--quality=70-90 --strip`), JPEGs
jpegoptim-compressed (`--strip-all --max=85`); originals live in the
VirtualBench session exports.
