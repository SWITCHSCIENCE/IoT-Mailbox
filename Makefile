.PHONY: receiver transmitter

receiver:
	cd Receiver && $(MAKE) build flash mon

transmitter:
	cd Transmitter && $(MAKE) build flash mon
