.PHONY: build

build:
	@rm -rf build
	docker build --rm -t localhost/nrf52bootloader .
	docker run -it -v $$PWD/build:/root/Adafruit_nRF52_Bootloader/_build/ localhost/nrf52bootloader
