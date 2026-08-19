# Ethernet interface plan (W5500) — firmware2

Add a wired Ethernet interface to the quadrature-drive controller: a TCP control
socket speaking the **rotctld (NETROTCTL) protocol**, plus periodic telemetry to
a UDP multicast group. GS-232 is dropped.

Status: **planned, not built.**

## Why Ethernet at all

`rotctld` on any host is already a TCP server, so the network access itself is
not new. Putting the interface in the box buys three things:

1. No host PC in the control path — Gpredict/rotctl connect straight to the box.
2. 100 m reach instead of USB's 5 m; the box can live at the tower base.
3. **Transformer isolation.** USB bonds the shack PC's ground to a box with 52 V
   rails driving 1.7 A down 35 m of cable. Given the measured 0.31 V ground pump
   (see CLAUDE.md), breaking that loop is the strongest reason here.

## Protocol: rotctld, not GS-232

GS-232 over a raw socket needs hamlib's `host:port` port-spec handling and still
leaves a `rotctld` daemon in the chain. The rotctld protocol is what network
clients actually speak: Gpredict has it built in with no hamlib linkage, and
hamlib clients reach it with `rotctl -m 2 -r <ip>:4533`.

Verified against hamlib 4.6.2 (stub server + real `rotctl -m 2`). Complete set
of what the client sends:

| Command | Reply | Note |
|---|---|---|
| `\dump_state` | see below | once, on connect |
| `p` | `123.4\n0\n` | **always two lines**; elevation is always `0` |
| `P <az> <el>` | `RPRT 0\n` | el ignored |
| `S` | `RPRT 0\n` | stop |
| `_` | `10W707\n` | get_info |
| `M <dir> <speed>` | `RPRT 0\n` | optional jog; `8` = CCW, `16` = CW (`ROT_MOVE_LEFT`/`RIGHT`) |
| `q` | — | client closes |
| anything else | `RPRT -1\n` | |

`\dump_state` — emit the **legacy positional form**. Real rotctld 4.6.2 sends a
newer `key=value ... done` form, but the 4.6.2 client accepts both, and old
clients accept only this one:

```
0
0.000000 360.000000
0.000000 0.000000
```

Two verified formatting facts:

- `123.4\n0\n` parses identically to `123.40\n0.00\n`. Format from deci-degrees
  with integer arithmetic (`"%d.%d", d/10, d%10`) and hand-parse `P`.
  **Do not pull `%f` into AVR printf** — ~1.5 kB of flash for nothing.
- `P 123.4 0` gives 0.1 deg command resolution, matching the servo's internal
  deci-degrees. GS-232's `Maaa` was capped at whole degrees.

### Calibration and the serial port

The protocol has no endpoint-calibration command. Keep `O` / `F` as extension
verbs replying `RPRT 0`; neither letter collides with anything hamlib sends.

The same parser runs on `Serial` and on the TCP socket (`Stream&`). Model 2 is
TCP-only, so USB stops being hamlib-usable — bridge it with
`socat TCP-LISTEN:4533 /dev/ttyUSB0` if that fallback is ever needed. Otherwise
USB is the bench/calibration console, typing the same verbs by hand.

### Behaviour the serial port never needed

- **Single client.** Accept one connection, refuse extras: two clients issuing
  conflicting `P` commands is a realistic LAN failure.
- **Dead-peer timeout in manual mode.** `M` rotates until stopped; if the session
  dies mid-jog the soft limits are the only thing left. Stop after ~5 s of
  silence while jogging.

No authentication. Trusted LAN only — do not port-forward.

## Hardware

W5500 (hardware TCP/IP offload). ENC28J60 + UIPEthernet is rejected: ~20 kB of
flash for a software stack, next to an 8 kHz ISR.

**Measured cost** (`Ethernet@2.0.2`, TCP server + UDP, minus a bare sketch):
**+10.9 kB flash, +286 B RAM.** Against the current 7 240 B / 319 B that lands
near **18.2 kB (59 %) flash, ~605 B (30 %) RAM**. This breaks the README's
"keep flash < 50 %" budget; that number existed to escape K3NG's 99.7 %, so
revise it rather than drop the feature.

### Pin remap

SPI needs D11/D12/D13, which currently carry the LCD's E and RS. The library's
default CS is D10 = OC1B = a drive output. Exactly enough pins remain:

| Signal | Was | Becomes |
|---|---|---|
| LCD RS | D12 | **D7** |
| LCD E | D11 | **D8** |
| SPI MOSI / MISO / SCK | — | D11 / D12 / D13 |
| W5500 CS | — | **A1** (`Ethernet.init(A1)`) |

Leaves only A4/A5 spare. Alternative: an I2C LCD backpack on A4/A5 frees six
pins for ~2 kB more flash.

### Power and RF

- W5500 draws ~150–180 mA, roughly doubling the box's steady draw (CLAUDE.md:
  ~150 mA, 315–400 mA fuse) and adding ~1.9 W in the 7805. Needs a heatsink or
  a small buck ahead of the regulator. Re-check the mains fuse rating.
- Module I/O is 3.3 V. MISO at 3.3 V against the ATmega's 3.0 V V_IH is only
  0.3 V of margin — universally done, but it is thin. Prefer a module with
  level shifting.
- A 25 MHz PHY in a metal box near an HF antenna: not worse than the existing
  USB PHY, but use shielded cable and ferrites, and re-run bring-up step 5
  (RX noise check) after fitting.
- Static IP in EEPROM. DHCP is ~4 kB and adds failure modes a rotator does not
  want.
- SPI transfers do not mask interrupts, so the 8 kHz Timer2 DDS ISR is
  unaffected.

## Telemetry

UDP to an admin-scoped multicast group, e.g. `239.10.7.7:10707`, TTL 1, 5–10 Hz.
Sending needs no IGMP join. Cost on top of the above is ~0 — `EthernetUDP` is
already inside the 10.9 kB.

ASCII line, so `socat UDP4-RECV:10707` debugs it with no tooling:

```
az=1234 tgt=1800 st=CW f=600 amp=255 raw=2048
```

The point is that N monitors and loggers can watch without any of them opening a
socket or perturbing the control path. hamlib will not consume it — telemetry is
additive, never a control path.

Caveats: multicast is unreliable over WiFi (basic-rate, frequently dropped), and
managed switches with IGMP snooping but no querier will prune it. If receivers
sit on WiFi, use subnet broadcast or a configured unicast collector instead.

## Steps

1. `gs232` -> `netrotctl`, parameterised on `Stream&`, still on `Serial` only.
   *Verify:* `pio test -e native` green; `rotctl -m 2` against a socat bridge
   returns the right azimuth.
2. LCD pin remap (structural, no behaviour change).
   *Verify:* display still correct on hardware; tests green before and after.
3. W5500 init + TCP server, second parser instance.
   *Verify:* `rotctl -m 2 -r <ip>:4533` does `p`, `P`, `S` end to end; Gpredict
   tracks; pulling the cable mid-jog stops the motor within the timeout.
4. UDP multicast telemetry.
   *Verify:* `socat UDP4-RECV:10707,ip-add-membership=239.10.7.7:0.0.0.0 -`
   shows a clean stream during a full 360 deg move.
5. Re-run bring-up step 5 (HF RX noise) with the PHY live.
