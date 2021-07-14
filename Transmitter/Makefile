FQBN=adafruit:nrf52:feather52832
BUILD=build/$(subst :,.,$(FQBN))
PORT=$$(ls -1 /dev/tty.usb*|tail -n1)
NAME=Transmitter
OPT=--build-property 'build.extra_flags=-DNRF52832_XXAA -DNRF52'

.PHONY: build

build:
	arduino-cli compile $(OPT) -b $(FQBN) -e .

flash:
	@echo "press enter key and reset button:"; read
	arduino-cli upload -b $(FQBN) -i $(BUILD)/$(NAME).ino.zip -p $(PORT) .

mon:
	pyserial-miniterm $(PORT) 115200

property:
	arduino-cli compile --show-properties -b $(FQBN) .
