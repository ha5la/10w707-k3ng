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
        # Elevation-related options — no FEATURE_ELEVATION_CONTROL in this build
        ("#define OPTION_EL_SPEED_FOLLOWS_AZ_SPEED    // changing the azimith speed with Yaesu X commands or an azimuth speed pot will also change elevation speed",
         "// #define OPTION_EL_SPEED_FOLLOWS_AZ_SPEED    // changing the azimith speed with Yaesu X commands or an azimuth speed pot will also change elevation speed"),
        ("#define OPTION_DISPLAY_HEADING_EL_ONLY",
         "// #define OPTION_DISPLAY_HEADING_EL_ONLY"),
        # AZ_ONLY and HEADING both render the same content on row 1 — keep only one
        ("#define OPTION_DISPLAY_HEADING_AZ_ONLY",
         "// #define OPTION_DISPLAY_HEADING_AZ_ONLY"),
        # Version splash uses flash for the string constant and startup delay code
        ("#define OPTION_DISPLAY_VERSION_ON_STARTUP  //code provided by Paolo, IT9IPQ",
         "// #define OPTION_DISPLAY_VERSION_ON_STARTUP  //code provided by Paolo, IT9IPQ"),
        # REQUEST_KILL on button release (vs REQUEST_STOP which adds ramp-down logic).
        # Keeps the 200 ms debounce delay but stops the relay immediately after it.
        ("// #define OPTION_BUTTON_RELEASE_NO_SLOWDOWN  // disables slowdown when CW or CCW button is released, or stop button is depressed",
         "#define OPTION_BUTTON_RELEASE_NO_SLOWDOWN  // disables slowdown when CW or CCW button is released, or stop button is depressed"),
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
    "rotator_k3ngdisplay.cpp": [
        # update() skips setCursor() when writing consecutive changed characters, relying on
        # HD44780 auto-increment.  Auto-increment does not cross the row boundary (0x0F→0x10,
        # not 0x0F→0x40), so a change run that straddles row 0→1 writes the row-1 chars to
        # off-screen DDRAM — they appear at the correct position only on the next update cycle,
        # causing a brief flash on the wrong row.  Always calling setCursor (like redraw() does)
        # eliminates the optimisation and prevents cross-row misplacement.
        ("      if (!wrote_to_lcd_last_loop){\n        lcd.setCursor(Xposition(x),Yposition(x));\n      }",
         "      lcd.setCursor(Xposition(x),Yposition(x));"),
    ],
    "rotator_settings.h": [
        # 250 ms gives responsive button feedback without saturating the LCD bus
        ("#define LCD_UPDATE_TIME 1000",   "#define LCD_UPDATE_TIME 250"),
        # 1602A is 16x2, not 20x4
        ("#define LCD_COLUMNS 20 //16",    "#define LCD_COLUMNS 16"),
        ("#define LCD_ROWS 4 //2",         "#define LCD_ROWS 2"),
        # Field sizes must match LCD_COLUMNS; at 20 on a 16-col display print_center
        # calculates a start column of (16/2)-(20/2)=-2, corrupting the adjacent row
        ("#define LCD_HEADING_FIELD_SIZE 20",    "#define LCD_HEADING_FIELD_SIZE 16"),
        ("#define LCD_STATUS_FIELD_SIZE 20",     "#define LCD_STATUS_FIELD_SIZE 16"),
        # Move combined-heading row to 1 so OPTION_DISPLAY_HEADING overlaps
        # OPTION_DISPLAY_HEADING_AZ_ONLY (same AZ content, same row — harmless),
        # leaving row 2 uncontested for OPTION_DISPLAY_STATUS.
        ("#define LCD_HEADING_ROW 2",      "#define LCD_HEADING_ROW 1"),
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
