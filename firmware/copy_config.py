"""
PlatformIO pre-build script — patches K3NG defaults for the 10W707 build.

All other settings use K3NG's defaults unchanged.  The patches below are the
only divergences from a stock K3NG installation following the standard wiki
schematic (https://radioartisan.wordpress.com).

FEATURE_4_BIT_LCD_DISPLAY must be defined in rotator_features.h (included at line
1136 of the main sketch), not only in rotator_k3ngdisplay.h (included at line 1168).
rotator_dependencies.h runs at line 1139 and uses the define to set FEATURE_LCD_DISPLAY,
which gates all display instantiation.  If the define arrives too late (only from
rotator_k3ngdisplay.h), FEATURE_LCD_DISPLAY is never set and K3NG shows nothing.

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
    "rotator_features.h": [
        # Must be defined here (before rotator_dependencies.h) so FEATURE_LCD_DISPLAY is set
        ("// #define FEATURE_4_BIT_LCD_DISPLAY // Uncomment for classic 4 bit LCD display (most common)",
         "#define FEATURE_4_BIT_LCD_DISPLAY // Uncomment for classic 4 bit LCD display (most common)"),
        # GS-232A is sufficient (we use rotctld -m 603); B emulation adds ~100+ bytes
        ("#define OPTION_GS_232B_EMULATION          // comment this out to default to Yaesu GS-232A emulation when using FEATURE_YAESU_EMULATION above",
         "// #define OPTION_GS_232B_EMULATION          // comment this out to default to Yaesu GS-232A emulation when using FEATURE_YAESU_EMULATION above"),
        # Display slots for features we don't have (no RTC, no GPS, no moon/sun tracking)
        ("#define OPTION_DISPLAY_HHMM_CLOCK  // display HH:MM clock  (set position with #define LCD_HHMM_CLOCK_POSITION)",
         "// #define OPTION_DISPLAY_HHMM_CLOCK  // display HH:MM clock  (set position with #define LCD_HHMM_CLOCK_POSITION)"),
        ("#define OPTION_DISPLAY_GPS_INDICATOR  // display GPS indicator on LCD - set position with LCD_GPS_INDICATOR_POSITION and LCD_GPS_INDICATOR_ROW",
         "// #define OPTION_DISPLAY_GPS_INDICATOR  // display GPS indicator on LCD - set position with LCD_GPS_INDICATOR_POSITION and LCD_GPS_INDICATOR_ROW"),
        ("#define OPTION_DISPLAY_MOON_OR_SUN_OR_SAT_TRACKING_CONDITIONAL  // LCD",
         "// #define OPTION_DISPLAY_MOON_OR_SUN_OR_SAT_TRACKING_CONDITIONAL  // LCD"),
    ],
    "rotator_k3ngdisplay.h": [
        # Wrap with #ifndef so the define from rotator_features.h above isn't duplicated
        ("#define FEATURE_4_BIT_LCD_DISPLAY\n",
         "#ifndef FEATURE_4_BIT_LCD_DISPLAY\n#define FEATURE_4_BIT_LCD_DISPLAY\n#endif\n"),
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
        if old in content:
            content = content.replace(old, new, 1)
            print(f"copy_config.py: {filename}: {old.strip()!r} -> {new.strip()!r}")
        elif new in content:
            print(f"copy_config.py: {filename}: already patched, skipping {new.strip()!r}")
        else:
            print(f"copy_config.py ERROR: expected string not found in {filename}:")
            print(f"  {old!r}")
            errors += 1
    with open(path, "w") as f:
        f.write(content)

if errors:
    env.Exit(1)
