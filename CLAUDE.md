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

**These assignments are inferred from the schematic and must be verified
with a multimeter before wiring the replacement unit.**

| TB1 Pin | Signal (inferred) | Measurement to verify |
|---------|--------------------|----------------------|
| 1 | Motor winding A (CCW sense) | Continuity to one motor winding |
| 2 | Motor winding B (CW sense) | Continuity to other motor winding |
| 3 | AC neutral / common | Common to both motor windings and pot |
| 4 | Potentiometer high-side terminal | Variable resistance to pin 3 across full travel |
| 5 | Potentiometer signal output | Verify grounded wiper assumption |

Original schematic shows 6–8 V range at the pot interface (Note 3).
Measure resistance of the pot from stop to stop (between pins 4 and 3, and 5 and 3) before building.

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

The drive unit potentiometer has a grounded wiper (rheostat configuration).
K3NG requires 0–5 V on Arduino pin A0, with 0 V = full CCW and 5 V = full CW.

**Step 1 — measure the potentiometer:**
Connect an ohmmeter between TB1 pin 4 and pin 3 (or 5) and rotate the antenna
stop to stop. Record R_min (near 0 Ω), R_max, and R_mid. The nominal value is
probably 500 Ω or 1 kΩ (typical CDE-era rotator).

**Step 2 — voltage divider:**

Per K3NG wiki recommendation: place a fixed resistor R_fixed equal to R_max in
series with the pot (rheostat), supply the divider from a stable 10 V reference,
and read the centre tap with Arduino A0.

```
+10 V ─── R_fixed (= R_max) ─── TB1 pin 4
                                       │
                               pot wiper (TB1 pin 3 or 5, whichever is ground)
                                       │
                              A0 ─────── junction of R_fixed and pot
                                       │
                                      GND
```

This gives 0 V at full CCW, ~5 V at full CW. Adjust R_fixed or add a trim pot
if the range is off.

**If pot resistance < 500 Ω** (loading the Arduino ADC): buffer with an LM358
op-amp (unity-gain follower) between the divider output and A0.

**10 V reference**: generate from 12 V supply with a 7810 (or 78L10) regulator,
or use a resistor divider from a precise 12 V rail if pot loading is acceptable.

### K3NG calibration

After wiring, run the rotator to both stops and record the raw ADC values shown
in K3NG's calibration mode. Set `az_full_ccw` and `az_full_cw` in the firmware.
The servo loop handles everything else.

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

Enable external analog reference for better ADC accuracy:
```cpp
#define OPTION_EXTERNAL_ANALOG_REFERENCE
```

### Protocol for rotctld

K3NG emulates **Yaesu GS-232A** protocol by default. In `hamlib` / `rotctld`:

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
| 1 | Transformer | 230 VAC → 20 VAC, ≥3 VA (motor current ~100 mA) — or reuse original T1 |
| 1 | Capacitor | 120 µF / 250 VAC motor-run type (replaces original C1) |
| 1 | Regulator | 78L10 or 7810 for 10 V pot supply |
| 1 | Regulator | 7805 / LM7805 or switching 5 V module for Arduino |
| 1 | Rectifier bridge | 1 A / 50 V, for DC supply from transformer secondary |
| 1 | Filter cap | 470–1000 µF / 25 V for DC rail |
| 1 | Resistor R_fixed | = pot R_max (measure first), ¼ W |
| 1 | LM358 op-amp | Optional; needed if pot R_max < 500 Ω |
| 5 | Terminal blocks | 5-way for tower cable; fused IEC inlet for 230 VAC |
| 1 | Enclosure | Metal preferred (AC mains inside) |
| 1 | Fuse | 125 mA fast-blow on 230 VAC line |

**Reusing original T1**: The original transformer has a 120 V primary — it cannot be
used directly on 230 V mains. Either rewind it (not worth it), or replace it with
a standard 230 V → 20–24 VAC EI-core transformer (≥3 VA, very cheap).
A 230 V → 2×12 V transformer with the secondaries in series gives 24 VAC,
which is close enough (motor will run slightly faster and warmer; acceptable).

## Power Supply Architecture

```
230 VAC ──[Fuse]──[Power switch]──── T1 primary (230 V)
                                         │
                           T1 secondary (20–24 VAC, ≥3 VA)
                                    ┌────┴────┐
                              20 VAC to       Rectifier bridge
                              relay contacts  → 26–30 VDC
                                              → 7810 → 10 V (pot supply)
                                              → 7805 → 5 V (Arduino + relay coils)
```

If relay coils are 12 V: add a 7812 regulator between the DC rail and the relay coils.
Arduino can alternatively be powered from the PC USB port, keeping the 230 VAC
supply only for the motor — simplifies the low-voltage side.

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

1. **Measure pot resistance** stop-to-stop (pins 4–3 or 4–5 of TB1).
2. **Verify cable pinout**: with motor disconnected, apply low-voltage AC to
   each pair of wires and observe which motor terminals buzz/warm.
3. **Confirm winding direction**: after first power-up, verify that K3NG CW
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
