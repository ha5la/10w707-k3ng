# RCA 10W707 Rotator — K3NG Arduino Control Unit Replacement

## Project Goal

Replace the original RCA 10W707 analog control unit with a K3NG Arduino-based controller,
exposing azimuth control to a PC via USB and `rotctld` (hamlib).

The original control unit is a bench-top box connected to the tower drive unit by a **5-wire cable**.
The replacement is a new box containing an Arduino, relay board, signal conditioning, and power supply.
Nothing in the drive unit (motor, potentiometer) is modified.

## The Drive Unit (tower end — do not modify)

From the original schematic (Fig. 15, doc 964307):

| Component | Value | Location | Note |
|-----------|-------|----------|------|
| M1 | 20 V, 60 Hz single-phase AC | Drive unit | Capacitor-run, two-winding; runs fine at 50 Hz (≈17% slower) |
| R1 | 1 000 Ω | Drive unit | Snubber / speed-limit resistor across motor |
| Position sensor | Bridge potentiometer | Drive unit | Grounded-wiper rheostat; actual resistance TBD |

The run capacitor **C1 (120 µF)** was in the original *control* unit, not the drive unit.
It must be replicated in the replacement control unit.

### 5-Wire Cable Pinout

Motor winding direction (pins 1/2) must be verified with a multimeter.
Pot wiring (pins 3/4/5) is confirmed from the original schematic.

| TB1 Pin | Signal | Note |
|---------|--------|------|
| 1 | Motor winding A | Verify which direction is CW at first power-up |
| 2 | Motor winding B | Swap with pin 1 if rotation direction is reversed |
| 3 | AC neutral / motor common / pot wiper | Wiper confirmed grounded to motor common in original schematic |
| 4 | Potentiometer terminal A | One fixed end of resistive track |
| 5 | Potentiometer terminal B | Other fixed end of resistive track (complementary to pin 4) |

Measure R(pin4–pin3) and R(pin5–pin3) stop-to-stop before building to determine R_max.

## Motor Control — Relay Design

### Why relays

For a 20 V AC capacitor-run motor, relays are the standard and correct choice.
PWM / H-bridge motor drivers are for DC motors. AC phase controllers (TRIAC) can
theoretically vary speed but add complexity, risk motor overheating, and are
unnecessary — the K3NG servo loop stops the motor at the right heading without speed control.

**Speed control is not implemented.** The servo loop provides accurate positioning through
dead-band and calibration, not speed adjustment.

### Relay circuit — two DPDT relays

Two DPDT (double-pole double-throw) relays implement CW and CCW independently,
matching K3NG's two-pin control model. When both relays are de-energised the motor
is fully disconnected (unlike SPDT schemes where the capacitor remains in circuit).

```
K3NG pin 6 (rotate_cw)  → ULN2803 driver → RLY-CW coil
K3NG pin 7 (rotate_ccw) → ULN2803 driver → RLY-CCW coil
```

#### Wiring (20 V AC motor, run capacitor C1 = 120 µF)

Net assignments (KiCad net names in schematic):

| Net | Connected to |
|-----|-------------|
| AC_HOT | T1 secondary hot, K1.NO1(pin14), K2.NO1(pin14), C1.pin1 |
| C1_JCT | C1.pin2, K1.NO2(pin24), K2.NO2(pin24) |
| MOTOR_W1 | K1.COM1(pin12), K2.COM2(pin22), J2.pin1 |
| MOTOR_W2 | K2.COM1(pin12), K1.COM2(pin22), J2.pin2 |
| MOTOR_COM | J2.pin3 = AC Neutral |

Pin numbering follows IEC relay convention: x2=COM, x4=NO, x1=NC; coil=A1/A2.

```
AC_HOT ──┬── K1.pin14(NO1) ── K1.pin12(COM1) ── MOTOR_W1 ── K2.pin22(COM2) ── K2.pin24(NO2) ──┐
          │                                                                                        │
          │   K2.pin14(NO1) ── K2.pin12(COM1) ── MOTOR_W2 ── K1.pin22(COM2) ── K1.pin24(NO2) ──┤
          │                                                                                        │
          └───────────────────── C1 (120 µF) ──────────────────────────────────────── C1_JCT ─────┘

K1.pin11(NC1), K1.pin21(NC2), K2.pin11(NC1), K2.pin21(NC2) → no_connect
MOTOR_COM → AC Neutral (Tower wire 3)
```

**CW active** (K1 energised, K2 off):
- MOTOR_W1 ← K1.pin12 ← K1.pin14(NO, closed) ← AC_HOT  (direct)
- MOTOR_W2 ← K1.pin22 ← K1.pin24(NO, closed) ← C1_JCT ← C1 ← AC_HOT  (phase-shifted)

**CCW active** (K2 energised, K1 off):
- MOTOR_W2 ← K2.pin12 ← K2.pin14(NO, closed) ← AC_HOT  (direct)
- MOTOR_W1 ← K2.pin22 ← K2.pin24(NO, closed) ← C1_JCT ← C1 ← AC_HOT  (phase-shifted)

**Both off**: all NO contacts open — motor fully disconnected.

If the winding polarity turns out to be inverted (motor runs the wrong direction for CW command),
swap W1 and W2 in the wiring; no firmware change needed.

### Relay specifications

| Parameter | Minimum |
|-----------|---------|
| Contact rating | 1 A at 24 V AC (motor current is low — small fan motor class) |
| Coil voltage | 5 V or 12 V (match your supply) |
| Type | DPDT (2 Form C) |

Suitable parts: Omron G2R-2, Fujitsu FTR-B3, TE/Tyco RT424005. A 4-channel relay
module (which contains four SPDT relays on one board) does **not** work directly —
you need genuine DPDT parts or wire two SPDT boards as DPDT (described below).

#### Using SPDT relay modules (fallback)

Standard 2-channel or 4-channel relay boards have SPDT relays only. To emulate a
DPDT using two SPDTs for each direction: wire RLY-CW-A as Pole 1 of RLY-CW and
RLY-CW-B as Pole 2, both coils driven together from the same Arduino pin via a
Y-split or a second ULN2803 channel. Use a 4-channel board for CW (2 relays) +
CCW (2 relays).

## Position Sensing — Potentiometer

### Signal conditioning

The drive unit pot has its wiper grounded to motor common (pin 3 = GND = DC−).
Both fixed terminals are available on pins 4 and 5. As the antenna rotates CCW→CW:
- R(pin4–GND) increases from 0 to R_max  → V4 rises
- R(pin5–GND) decreases from R_max to 0  → V5 falls

These signals are complementary and are combined in a difference amplifier for
better linearity and noise rejection than a single divider.

**Step 1 — measure the potentiometer:**
Connect an ohmmeter between pin 4 and pin 3 (then pin 5 and pin 3) and rotate
stop to stop. Record R_max. Nominal value is probably 500 Ω or 1 kΩ.

**Step 2 — two voltage dividers (V_ref = 5 V from Arduino AREF):**

```
+5 V ── R_fixed ──┬── pin 4 ── R(pin4→GND) ── GND    V4 read at ┬
                  ┘                                               │
+5 V ── R_fixed ──┬── pin 5 ── R(pin5→GND) ── GND    V5 read at ┬
```

R_fixed = R_max (measure first). V4 swings 0–2.5 V, V5 swings 2.5–0 V.

**Why not a single divider?** With R_fixed = R_max the transfer function is
`V = 5 × pos/(1+pos)`, a hyperbola. K3NG calibrates only the endpoints and
linearly interpolates, producing up to **~60° heading error at the midpoint**.

**Step 3 — difference amplifier (one half of LM358):**

```
V5 ──R(10k)──┬── IN−
              │ LM358           R_f (10k)
              └─────────────────/\/\/──┬── V_out → A0
                                       │
V4 ──R(10k)──┬── IN+                  │
              │                        │
2.5V─R(10k)──┘    (2.5 V = two 10 kΩ from +5 V to GND)
```

`V_out = V4 − V5 + 2.5 V`

| Position | V4 | V5 | V_out |
|----------|----|----|-------|
| Full CCW | 0 V | 2.5 V | 0 V |
| Midpoint | 1.67 V | 1.67 V | 2.5 V |
| Full CW | 2.5 V | 0 V | 5 V |

Maximum heading error after K3NG linear calibration: **≈ 8°** (at quarter-points).
Common-mode noise from the motor on the 35 m cable cancels in the subtraction.

### K3NG calibration

Calibration is interactive over the serial port (9600 baud) using standard
Yaesu GS-232 commands.  No firmware recompile needed — values are stored in EEPROM.

1. Open a serial terminal: `pio device monitor --project-dir firmware` (or `make monitor`)
2. Rotate the antenna manually to the **full CCW** hard stop
3. Send **`O`** → K3NG prints `Rotate to full CCW and send keystroke...` → press any key
   → K3NG reads the ADC, saves `analog_az_full_ccw` to EEPROM, replies `Wrote to memory`
4. Rotate to the **full CW** hard stop
5. Send **`F`** → K3NG prints `Rotate to full CW and send keystroke...` → press any key
   → saves `analog_az_full_cw` to EEPROM

The servo loop then linearly interpolates all positions between the two endpoints.

## K3NG Firmware Configuration

Repository: `https://github.com/k3ng/k3ng_rotator_controller`

### Feature flags (`features_and_options.h`)

```cpp
#define FEATURE_AZ_POSITION_POTENTIOMETER   // enable pot feedback
// Remove or comment out:
// #define FEATURE_ELEVATION_CONTROL        // azimuth-only build
```

### Pin assignments (`rotator_pins.h`)

```cpp
#define rotate_cw   6    // drives RLY-CW coil (via ULN2803)
#define rotate_ccw  7    // drives RLY-CCW coil (via ULN2803)
#define rotator_analog_az A0  // pot signal (0-5 V)
```

### Settings (`rotator_settings.h`)

```cpp
// Set after physical calibration:
#define AZ_STARTING_POINT  180   // or 0, depending on your antenna orientation
#define AZ_ROTATION_CAPABILITY_CW  360
#define AZ_ROTATION_CAPABILITY_CCW 360
```

Use the internal VCC (5 V) as ADC reference — the pot supply and AREF are the same
rail, so the measurement is inherently ratiometric. Do not enable external reference
unless a precision 5 V reference IC is fitted to AREF.

### Protocol for rotctld

K3NG has `OPTION_GS_232B_EMULATION` enabled but responds correctly to GS-232A
commands.  Use model **603** (GS-232A, AZ-only) — model 604 (GS-232B) sends the
`C2` command expecting an AZ+EL pair, which K3NG does not return in an AZ-only
build, causing hamlib to report "Feature not available".

```bash
rotctld -m 603 -r /dev/ttyUSB0 -s 9600
```

Model 603 = `ROT_MODEL_GS232A`. Verify the USB serial port with `dmesg | grep tty`
after plugging in the Arduino.

## Hardware Bill of Materials

| Qty | Part | Value / Notes |
|-----|------|---------------|
| 1 | Arduino Uno R3 | or Nano; provides USB-serial for rotctld |
| 2 | DPDT relay | 5 V coil, ≥1 A/24 VAC contacts; e.g. Omron G2R-2-5V |
| 1 | ULN2803A | Darlington array, relay driver |
| 1 | T1 — transformer, motor | 230 VAC → 18–24 VAC, ≥3 VA |
| 1 | T2 — transformer, electronics | 230 VAC → 12–15 VAC, ≥5 VA |
| 1 | Capacitor | 120 µF / 250 VAC motor-run type (replaces original C1) |
| 1 | Regulator | 7805 / LM7805 — 5 V for Arduino, relay coils, pot supply |
| 1 | Rectifier bridge | 1 A / 50 V, for T2 secondary |
| 1 | Filter cap | 470–1000 µF / 25 V for DC rail |
| 2 | Resistor R_fixed | = pot R_max each (measure first), ¼ W — one per divider |
| 2 | Resistor 10 kΩ | 2.5 V reference divider for op-amp offset |
| 4 | Resistor 10 kΩ | Difference amplifier (R1, R2, R3, R_f) |
| 1 | LM358 op-amp | Difference amplifier for pot signal conditioning |
| 5 | Terminal blocks | 5-way for tower cable; fused IEC inlet for 230 VAC |
| 1 | Enclosure | Metal preferred (AC mains inside) |
| 1 | Fuse | 125 mA fast-blow on 230 VAC line |

The original T1 has a 120 V primary and cannot be reused on 230 V mains.

## Power Supply Architecture

Two separate transformers are required. Tying a single secondary's neutral to DC−
would cause the bridge rectifier to operate as half-wave. Two separate secondaries
avoid this: T1 powers the motor only; T2 powers the electronics only. T1's neutral
(= motor common = pin 3) is tied to T2's DC− to establish a shared ground.

```
230 VAC ──[Fuse]──[Switch]──┬── T1 primary
                             │     secondary HOT  ──── relay contacts (motor AC)
                             │     secondary NEUTRAL ── pin 3 (motor common) ──┐
                             │                                                  │= DC−
                             └── T2 primary                                    │
                                   secondary ──── bridge rectifier ────────────┘
                                                  → filter cap (470–1000 µF)
                                                  → 7805 → 5 V (Arduino, relay coils,
                                                                 pot V_ref, op-amp)
```

| | T1 (motor) | T2 (DC electronics) |
|-|------------|---------------------|
| Primary | 230 VAC | 230 VAC |
| Secondary | 18–24 VAC | 12–15 VAC |
| Power | ≥ 3 VA | ≥ 5 VA |

T2 secondary: 12 V AC → rectified ≈ 15.6 V DC → 7805 → 5 V.
If relay coils are 12 V, add a 7812 between the DC rail and the relay coils.

## Battery Operation

The motor is a capacitor-run AC motor and cannot run on DC without generating AC first.
Battery operation is not part of the primary design. Options if needed:

- **Simplest**: plug a small 12 V → 230 V modified-sine-wave inverter (15–30 W class)
  upstream of the control unit. The existing transformer then steps down to 20 V.
  Three conversion stages are inefficient but zero extra circuit design is required.
  Rotation is brief so total energy draw is low.

- **Cleaner**: generate 50 Hz square wave at ~28–30 V directly from a boosted 12/24 V
  DC bus through an H-bridge. The run capacitor smooths harmonics enough for the motor
  to start and run. Requires a boost converter + H-bridge IC and careful dead-time
  management; reasonable if battery operation is a firm requirement.

- **Fixed station backup**: put a UPS on the 230 VAC mains feed. No changes to the
  control unit at all.

## Open Questions / Verification Steps

Before building the relay board:

1. **Measure pot resistance** stop-to-stop: R(pin4–pin3) and R(pin5–pin3).
   Use R_max value for R_fixed in both voltage dividers.
2. **Verify motor winding assignment** (pins 1 and 2): with motor disconnected,
   apply low-voltage AC to each pair and observe which terminals buzz/warm.
3. **Confirm rotation direction**: after first power-up, verify that K3NG CW
   command rotates the antenna clockwise. If reversed, swap W1/W2.
4. **Check motor current** with a clamp meter before closing the enclosure.
   Should be < 200 mA at 20 VAC under no load.
5. **Calibrate ADC**: run antenna to both hard stops; record K3NG raw ADC values.

## Repository Structure (planned)

```
/
├── original-schematics.jpg     # factory schematic (serial ≥86344)
├── hardware/
│   ├── schematic.kicad_sch     # replacement control unit schematic
│   └── bom.csv
└── firmware/
    └── k3ng_config/
        ├── features_and_options.h
        ├── rotator_pins.h
        └── rotator_settings.h
```

K3NG firmware lives in its own repository — only the three config headers are tracked here.
