#include <Arduino.h>

#include "lora_handler.h"

void BoardGetUniqueId(uint8_t *id);
uint8_t ACK = 0x06;

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size == 11) {
    Serial.print("RX:\t");
    Serial.printf("rssi=%d\tsnr=%d\t", rssi, snr);
    Serial.printf("id=%02x%02x%02x%02x%02x%02x%02x%02x\t", payload[0],
                  payload[1], payload[2], payload[3], payload[4], payload[5],
                  payload[6], payload[7]);
    uint16_t v = *(uint16_t *)(&payload[8]);
    Serial.printf("battery=%4.2f\t", float(v) / 100.0);
    Serial.printf("state=%d\n", payload[10]);
    SendLoRa(&ACK, 1);  // reply ack
  }
}

void setup() {
  static RadioEvents_t conf;
  Serial.setPins(2, 31);
  Serial.begin(115200);
  Serial.println("boot!");
  // Initialize LoRa
  conf = DefaultConf();
  conf.RxDone = onRxDone;
  SetupLoRa(0, &conf);
  SetupTx();
  SetupRx();
  uint8_t deviceId[8];
  BoardGetUniqueId(deviceId);
  Serial.print("SerialNumber: ");
  Serial.printBuffer(deviceId, 8);
  Serial.println();
  StartRx();
}

void loop() { LoRaIrqProc(); }