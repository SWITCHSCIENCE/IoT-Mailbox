#pragma once

#include <LoRaWan-Arduino.h>

RadioEvents_t DefaultConf();
void SetupLoRa(uint8_t ch, RadioEvents_t *conf);
void LoRaIrqProc();
void SetupTx();
void SetupRx();
void StartRx();
void SendLoRa(uint8_t *buff, uint16_t len);
