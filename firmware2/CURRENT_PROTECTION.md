# Coil current sensing & stall protection — firmware2

Audio-amp motor drive · 13.8 V DC supply · variable-frequency AC to the motor
coils. Two ACS712 hall sensors measure winding current; firmware computes RMS
and trips the drive on overcurrent.

Status: **planned, not built.** (Application note rev C, 2026-07-26.)

## How it works

Each ACS712 sits **in series** between one amplifier output and its motor coil.
The sensor outputs 2.5 V at 0 A and swings ±185 mV/A (5 A version). Because the
amp synthesizes variable-frequency AC, the output is a sine centred on 2.5 V —
the firmware samples fast and computes RMS in software. The RC filter
(1 k / 100 n, f_c ≈ 1.6 kHz) strips switching noise while passing the
drive-frequency signal untouched.

The hall sensor measures without breaking isolation and is fully bidirectional,
so the same hardware would also work on the relay/grid version of the controller.

```
AMP A OUT (sin, D9) ──┤IP+  U1  IP−├── WINDING W1 (wire 1) ── COMMON (wire 5 = GND)
                        ACS712-5A-M
                      VCC=+5V  GND
                          OUT ──1k(R1)──┬── A6
                                    100n(C1)
                                        │
                                       GND

AMP B OUT (cos, D10) ─┤IP+  U2  IP−├── WINDING W2 (wire 4) ── COMMON (wire 5 = GND)
                        ACS712-5A-M
                      VCC=+5V  GND
                          OUT ──1k(R2)──┬── A7
                                    100n(C2)
                                        │
                                       GND
```

## Bill of materials

| Ref | Part | Notes |
|---|---|---|
| U1, U2 | ACS712-5A-M module (Hestore 100.355.26) | 185 mV/A, 80 kHz bandwidth, 1.2 mΩ internal conductor, 5 V single supply |
| R1, R2 | 1 kΩ resistor | any 1/4 W film part |
| C1, C2 | 100 nF ceramic | close to the Arduino analog pin |
| MCU | Arduino (existing controller board) | uses **A6, A7** (Nano analog-only pins; A0 = position pot, A1 reserved for wire-3 divider, A2/A3 = buttons per firmware2 `config.h`). Default VCC ADC reference. Bench Uno has no A6/A7 — use A4/A5 there. |

**Why the 5 A part:** under quadrature drive expect ≈1 A RMS symmetric per
winding at 60 Hz (18 V RMS into |Z| ≈ 20 Ω; the 1.5 A figure was the relay
version with the run capacitor). Peaks of ±1.4 A sit comfortably inside ±5 A
with ~26 mA/ADC-count resolution. Stall is bounded by 18 V across the ~6 Ω
winding + leakage ≈ 2–3 A — unlike a line-start stall the sensor never
saturates, so fault currents can be measured accurately as well as tripped on.

## Placement & stray magnetic fields

The ACS712 is a single hall element with no differential rejection — external
fields couple in at roughly 1.2 mT ≈ 1 A of false reading. Near-field sources
fall off between 1/r² and 1/r³, so distance solves most of it.

- Keep both sensors a hand-width (≥10 cm) from the mains transformer, relays,
  and the step-up converter's inductor — stray pickup then drops to tens of mA.
- Orient the packages so offending flux runs *in the plane* of the IC; the
  sensitive axis is perpendicular to the package face.
- Twist the wire pair going to each sensor's IP+/IP− terminals.
- Boot-time zero calibration removes any static offset. AC pickup can't be
  zeroed out — only distance, orientation, or a small steel bracket as a shield.

## Variable-frequency RMS measurement

Both controller versions put AC through the coils — the amp version just varies
the frequency. The RMS averaging window therefore can't be tuned to one cycle:
size it to **at least 2 full cycles at the lowest drive frequency**. firmware2's
`DRIVE_FREQ_MIN_DHZ` is 5 Hz, so use ≥400 ms; it averages correctly at every
higher frequency too. Shorter windows wobble as partial cycles slide through.
Since the dds module exposes `dds_get_freq_dhz()`, the window can also be
derived live from the actual drive frequency for faster trips at speed.

> **Ratiometric trick:** keep the default VCC ADC reference. The ACS712 output
> scales with its supply, so when sensor and ADC ride the same 5 V rail, supply
> drift cancels out of the amps calculation.

## Firmware sketch

Drop in as `src/current.h/.cpp`; call `currentInit()` from `setup()` (drive OFF,
after `dds_init()`) and `currentTask()` from `loop()` next to `dds_tick()`.
Repo convention: write the Unity test first (test mocks control ADC + clock).

```cpp
const uint8_t  PIN_I[2]     = {A6, A7};   // A0 = pos pot, A1 reserved, A2/A3 = buttons
const float    SENS_V_A     = 0.185;   // ACS712-5A-M: 185 mV/A
const float    I_TRIP_RMS   = 2.0;     // amps RMS: 2x the ~1 A symmetric winding current
const uint16_t WINDOW_MS    = 400;     // >= 2 cycles at DRIVE_FREQ_MIN (5 Hz)
const uint8_t  TRIP_WINDOWS = 3;       // consecutive over-limit windows before trip
const uint16_t BLANK_MS     = 1500;    // ignore inrush after motor start

float    zeroAdc[2];                   // ADC counts at 0 A, calibrated at boot
float    sumSq[2];
uint32_t nSamp[2];
uint32_t winStart;
uint8_t  overCnt[2];
uint32_t motorStartMs;                 // set to millis() when you enable the drive
float    iRms[2];                      // last computed values, for telemetry

// ---- call once in setup(), drive must be OFF ----
void currentInit() {
  for (uint8_t ch = 0; ch < 2; ch++) {
    long acc = 0;
    for (uint16_t i = 0; i < 500; i++) { acc += analogRead(PIN_I[ch]); delay(1); }
    zeroAdc[ch] = acc / 500.0;         // should sit near 512 (2.5 V midpoint)
  }
  winStart = millis();
}

// ---- call as often as possible from loop() (aim for >= 1 kHz effective) ----
void currentTask() {
  for (uint8_t ch = 0; ch < 2; ch++) {
    float d = analogRead(PIN_I[ch]) - zeroAdc[ch];
    sumSq[ch] += d * d;
    nSamp[ch]++;
  }

  if (millis() - winStart < WINDOW_MS) return;

  for (uint8_t ch = 0; ch < 2; ch++) {
    // counts -> volts -> amps (ratiometric: sensor out and ADC ref track VCC)
    iRms[ch] = sqrt(sumSq[ch] / nSamp[ch]) * (5.0 / 1023.0) / SENS_V_A;
    sumSq[ch] = 0; nSamp[ch] = 0;

    bool blanked = (millis() - motorStartMs) < BLANK_MS;
    if (!blanked && iRms[ch] > I_TRIP_RMS) {
      if (++overCnt[ch] >= TRIP_WINDOWS) tripProtection(ch, iRms[ch]);
    } else {
      overCnt[ch] = 0;
    }
  }
  winStart = millis();
}

void tripProtection(uint8_t ch, float amps) {
  // D6 (MUTE/STBY) belongs to the dds module -- stop through its API:
  dds_stop();   // fast ramp-down (~0.17 s), amps mute after the ramp completes
  // hard backstop for a dead-short fault: digitalWrite(PIN_AMP_ENABLE, LOW);
  Serial.print(F("OVERCURRENT coil "));
  Serial.print(ch + 1);
  Serial.print(F(": "));
  Serial.print(amps, 2);
  Serial.println(F(" A rms - drive halted"));
  // latch here, or set a fault flag your main state machine clears manually
}
```

## Integration & calibration checklist

- Set `motorStartMs = millis()` every time the drive is enabled, so inrush
  blanking works per start, not just at power-up.
- Run `currentInit()` in `setup()` before the amp is enabled — it needs true
  zero current for calibration.
- Verify the idle reading: both channels near 0 A with the drive off. A large
  static offset means a nearby field source — move or reorient the sensor.
- Run the motor and confirm `iRms` reads ≈1 A *symmetric* on both windings
  (doubles as bring-up step 4 in the README); set `I_TRIP_RMS` to ~2 A.
- Optional: derive `WINDOW_MS` from `dds_get_freq_dhz()` for faster trips at
  high speed while staying accurate at the 5 Hz crawl.
- Have the fault latch feed the servo state machine (like the soft end-stop
  limits do), so a goto can't silently restart the drive after a trip.
- Test the trip by stalling briefly at low speed — under quadrature drive the
  stall current stays ≈2–3 A (bounded by winding impedance), well inside sensor
  range, so you get a real reading rather than saturation.

## Feature roadmap — low effort, high value

| Feature | Cost | Why |
|---|---|---|
| Position stall detect | software only | If dds is running but filtered position hasn't moved for ~1–2 s, trip. Cross-checks the current detector: current catches jams and shorted windings fast; position catches slip and mechanical breakage the current sensor can't see. |
| Current telemetry | software only | Print `iRms[]` over serial / spare LCD characters. Stiff winter grease, corroding connectors and winding asymmetry all show up as current signatures before failure. Also verifies bring-up step 4 for free. |
| Battery monitor | 1 divider on A4 | Full-speed drive pulls ~3 A from a 13.8 V battery (2 × 18 W through the boost at ~85%). Low-voltage warning + cutoff protects the battery; LCD readout makes the unit field-standalone. |
| Eco speed cap | software only | GS-232 `Xn` presets are parsed and ignored today; mapping them to a max drive frequency roughly halves energy per rotation on battery when not in a hurry. |
| Wire-3 cable check | 1 resistor to A1 | Second divider per amp-stage.md: the two pot segments must sum to the ≈1048 Ω track — a continuous cable-health check that flags degradation before position readings go bad. |
| Rotation lag comp | software only | Position filter lags ~0.3 s. Firmware knows the drive frequency → rotation speed; subtracting lag × deg/s while moving makes reported azimuth accurate during rotation, not just at rest. |

## Cable length changes

**Position:** lead resistance of wires 2 and 5 adds in series with the pot
segment, shifting both endpoints by the same constant. Calibration is two-point
and stored in ohms, and R is linear in angle, so **recalibrating O/F absorbs it
exactly** — no hardware change. Magnitude is small anyway: ~30 m of 0.75 mm²
adds ~1.5 Ω round trip against a ~790 Ω span (≈2.2 Ω/degree) — under a degree
even uncalibrated. The subtler effect is AC: wire 5 carries the vector sum of
both winding currents *and* is the pot's ground reference, so a longer cable
injects more drive-frequency ripple into A0. The two-stage RC filter + burst
averaging exist for exactly this; keep effective averaging longer than one drive
cycle at crawl.

**Drive:** cable resistance is in series with the winding. At 60 Hz it's a few
percent (1.5 Ω vs |Z| ≈ 20 Ω). At the 5 Hz crawl it isn't — 1.5 Ω against ~6 Ω
costs a quarter of the winding voltage right where `DRIVE_AMP_FLOOR` compensates
winding IR. Simple fix: treat the floor as a per-installation constant —
remeasure crawl current after a cable change and adjust. Elegant fix (cheap once
current sensing exists): a slow closed-loop amplitude trim nudging V/f output
toward a target crawl current, clamped between hard amplitude limits —
auto-absorbs cable changes, copper temperature drift (~0.4%/°C), and boost droop
on battery.

> **Control target:** don't regulate for constant current — load current
> legitimately varies with torque demand, and flattening it fights the motor.
> Constant *flux* is the target, which the V/f law already provides; the current
> loop should only supply the slow IR-compensation term (bounded, integrating
> over seconds), leaving fast current variation free to carry load information —
> exactly what the stall detector needs to see.
