.PHONY: build upload monitor clean

build:
	pio run --project-dir firmware -e nano_new

upload:
	pio run --project-dir firmware -e nano_new --target upload

monitor:
	pio device monitor --project-dir firmware --baud 9600

# Reset K3NG submodule after building (copy_config.py patches 6 lines)
clean:
	git -C firmware/k3ng checkout -- k3ng_rotator_controller/rotator_pins.h k3ng_rotator_controller/rotator_settings.h 2>/dev/null || true
	rm -rf firmware/.pio
