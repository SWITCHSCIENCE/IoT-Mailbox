#include <Arduino.h>
#include <LoRaWan-Arduino.h>

const int PIN_LED = 13;  // LED

RadioEvents_t DefaultConf();
void SetupLoRa(uint8_t ch, RadioEvents_t *conf);
void SetupRx();
void LoRaIrqProc();
void BoardGetUniqueId(uint8_t *id);

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size == 11) {
    Serial.print("RX:\t");
    Serial.printf("rssi=%d\tsnr=%d\tid=", size, rssi, snr);
    Serial.printBuffer(payload, 8);
    uint16_t v = *(uint16_t *)(&payload[8]);
    Serial.printf("\tbattery=%4.2f", float(v) / 100.0);
    Serial.printf("\tstate=%d\n", payload[10]);
  }
}

void setup() {
  static RadioEvents_t conf;
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  Serial.println("boot!");
  // Initialize LoRa
  conf = DefaultConf();
  conf.RxDone = onRxDone;
  SetupLoRa(0, &conf);
  SetupRx();
  uint8_t deviceId[8];
  BoardGetUniqueId(deviceId);
  Serial.print("SerialNumber: ");
  Serial.printBuffer(deviceId, 8);
  Serial.println();
}

void loop() { LoRaIrqProc(); }