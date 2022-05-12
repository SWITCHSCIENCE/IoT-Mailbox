GOOS=linux
GOARCH=arm
GOARM=6
export GOOS GOARCH GOARM

build:
	go build .

deploy:
	scp $(shell go list .) pi@raspberrypi.local:/home/pi/
