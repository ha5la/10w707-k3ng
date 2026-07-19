# firmware2 — quadrature-drive controller for the RCA 10W707

From-scratch minimal firmware replacing the K3NG build in `../firmware`.
Motivation: the K3NG build fills the Nano's flash to 99.7% (30 624 / 30 720 B)
and adding even the variable-frequency outputs overflows it (102.3%), while the
new drive concept needs custom code K3NG doesn't have anyway.

## Drive concept

The 10W707 motor is a two-phase machine; the original 120 µF run capacitor is
just a fixed 90° phase shifter that is only correct at one frequency. This
firmware synthesizes **two sine waves in true quadrature** (Timer1 PWM on
D9/D10 → RC low-pass → attenuator → two TDA7294 class-AB amplifiers in
single-supply single-ended mode, ~52 V rail → windings; motor common to GND):

- frequency = rotation speed, **5–60 Hz** (60 Hz = motor design point)
- amplitude follows a V/f law (18 V RMS at 60 Hz = design flux 0.3 V/Hz,
  with a floor compensating the ~6 Ω winding IR at crawl speed)
- direction = sign of the 90° phase offset — **no relays**
- no run capacitor — full torque at every speed, soft start from standstill

Amp MUTE/STBY are sequenced directly from D6/D7 (HIGH = play; no ULN2803 —
nothing needs a coil driver anymore). 10 k pull-downs on the amp pins keep the
amps muted during MCU reset, so the motor is dead silent and
disconnected-equivalent when idle, and there is no power-on thump.

## Module map

| Module | Job |
|---|---|
| `dds` | Timer1 dual PWM @ 62.5 kHz carrier; Timer2 ISR @ 8 kHz updates both channels from a 256-entry sine table; V/f amplitude; soft amplitude slew; amp MUTE/STBY sequencing |
| `position` | ADC burst average on A0 (two-stage RC filter does the real work), endpoint calibration in EEPROM |
| `servo` | goto/manual state machine, speed taper near target, soft end-stop limits |
| `gs232` | GS-232A subset for `rotctld -m 603`: `C`, `Maaa`, `R`, `L`, `A`, `S`, plus `O`/`F` calibration |
| `display` | 16×2 LCD: azimuth + state |
| `buttons` | hold-to-rotate CW/CCW, release stops |

Differences from K3NG worth knowing: `O`/`F` store the endpoint **immediately**
(no interactive keystroke step), and `Xn` speed presets are accepted but
ignored — the servo profiles speed itself.

## Bring-up plan (bench, in order)

1. **DDS on a scope**: D9/D10 through the RC filters — verify two clean sines,
   90° apart, amplitude tracking frequency, direction flip on `L`/`R`.
2. **Amps into dummy load** (power resistor ~20 Ω): verify 18 V RMS at 60 Hz,
   mute sequencing, heatsink temperature.
3. **Motor on the bench**: starting behaviour at 5–15 Hz, full-speed run,
   direction reversal, current draw per winding (expect symmetric now).
4. **Closed loop**: calibrate `O`/`F` at the stops, then `Maaa` moves and
   `rotctld -m 603` end-to-end.
5. **RX noise check** on the HF bands while rotating (class AB should be clean).

## Budgets

ATmega328P: 30 720 B flash / 2 048 B RAM. Track with `pio run` after every
change; keep flash < 50% — headroom is the point of this rewrite.
