"""
PlatformIO pre-build script — patches K3NG defaults for the 10W707 build.

All other settings use K3NG's defaults unchanged.  The patches below are the
only divergences from a stock K3NG installation following the standard wiki
schematic (https://radioartisan.wordpress.com).

Note: FEATURE_4_BIT_LCD_DISPLAY does NOT need patching — rotator_k3ngdisplay.h
already defines it unconditionally as the default display type.

Run `git -C firmware/k3ng checkout -- .` to restore the submodule if needed.
"""

Import("env")

import os

SKETCH = os.path.join(env["PROJECT_DIR"], "k3ng", "k3ng_rotator_controller")

PATCHES = {
    "rotator_pins.h": [
        # Wiki schematic has buttons on A2/A3 but K3NG defaults them to 0 (disabled)
        ("#define button_cw 0",  "#define button_cw A2"),
        ("#define button_ccw 0", "#define button_ccw A3"),
    ],
    "rotator_settings.h": [
        # 1602A is 16x2, not 20x4
        ("#define LCD_COLUMNS 20 //16",    "#define LCD_COLUMNS 16"),
        ("#define LCD_ROWS 4 //2",         "#define LCD_ROWS 2"),
        # Heading is already on row 1; move status to row 2 to avoid collision
        ("#define LCD_STATUS_ROW 1",       "#define LCD_STATUS_ROW 2"),
        # 360-degree rotator — no overlap zone
        ("#define AZIMUTH_ROTATION_CAPABILITY_EEPROM_INITIALIZE 450",
         "#define AZIMUTH_ROTATION_CAPABILITY_EEPROM_INITIALIZE 360"),
    ],
}

errors = 0
for filename, patches in PATCHES.items():
    path = os.path.join(SKETCH, filename)
    with open(path) as f:
        content = f.read()
    for old, new in patches:
        if old not in content:
            print(f"copy_config.py ERROR: expected string not found in {filename}:")
            print(f"  {old!r}")
            errors += 1
            continue
        content = content.replace(old, new, 1)
        print(f"copy_config.py: {filename}: {old.strip()!r} -> {new.strip()!r}")
    with open(path, "w") as f:
        f.write(content)

if errors:
    env.Exit(1)
