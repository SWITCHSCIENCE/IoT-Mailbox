#!/bin/sh
DEV=$(ls -1 /dev/cu.usbmodem* | head -n1)
arm-none-eabi-gdb -nx --batch \
  -ex "target extended-remote $DEV" \
  -ex 'monitor swdp_scan' \
  -ex 'attach 1' \
  -ex 'load' \
  -ex 'compare-sections' \
  -ex 'kill' \
  $1
