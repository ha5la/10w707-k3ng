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

All PNGs pngquant-compressed (`--quality=70-90 --strip`); originals live in the
VirtualBench session exports.
