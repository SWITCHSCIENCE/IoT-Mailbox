FROM alpine:3.15

RUN echo "@testing http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories
RUN apk add -U git
WORKDIR /root
RUN git clone --recursive https://github.com/adafruit/Adafruit_nRF52_Bootloader.git
RUN apk add make python3 py3-pip gcc-arm-none-eabi@testing newlib-arm-none-eabi@testing
WORKDIR /root/Adafruit_nRF52_Bootloader
ADD isp4520c-gw-r1 ./src/boards/isp4520c-gw-r1
RUN python3 -m pip install adafruit-nrfutil intelhex
RUN sed -i 's|1000|5000|g' src/main.c
CMD ["make", "BOARD=isp4520c-gw-r1"]
