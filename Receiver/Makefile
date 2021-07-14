FQBN=adafruit:nrf52:feather52832
BUILD=build/$(subst :,.,$(FQBN))
PORT=$$(ls -1 /dev/tty.usb*|head -n1)
NAME=Receiver
OPT=

.PHONY: build

build:
	arduino-cli compile $(OPT) -b $(FQBN) -e .

flash:
	@echo "press enter key and reset button:"; read
	arduino-cli upload -b $(FQBN) -i $(BUILD)/$(NAME).ino.zip -p $(PORT) .

mon:
	pyserial-miniterm $(PORT) 115200
