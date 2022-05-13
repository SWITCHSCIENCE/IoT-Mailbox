#include "lora_handler.h"

#include <Arduino.h>
#include <SPI.h>

// LoRa Parameters
#define TX_OUTPUT_POWER 14  // dBm (max:14dBm for JP & EU)
#define LORA_BANDWIDTH 0    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 12  // [SF7..SF12]
#define LORA_CODINGRATE 4         // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8    // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0     // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

const uint32_t CHANNELS[] = {
    921000000, 921200000, 921400000, 921600000, 921800000, 922000000, 922200000,
};

// nRF52832 - SX126x pin configuration
#define PIN_LORA_RESET 19  // LORA RESET
#define PIN_LORA_NSS 24    // LORA SPI CS
#define PIN_LORA_SCLK 23   // LORA SPI CLK
#define PIN_LORA_MISO 25   // LORA SPI MISO
#define PIN_LORA_DIO_1 11  // LORA DIO_1
#define PIN_LORA_BUSY 27   // LORA SPI BUSY
#define PIN_LORA_MOSI 26   // LORA SPI MOSI
#define RADIO_TXEN -1      // LORA ANTENNA TX ENABLE
#define RADIO_RXEN -1      // LORA ANTENNA RX ENABLE

SPIClass SPI_LORA(NRF_SPIM2, PIN_LORA_MISO, PIN_LORA_SCLK, PIN_LORA_MOSI);

static uint8_t sendBuffer[256];
static uint16_t sendLength = 0;
static uint32_t cadTime;
static uint32_t sendStart;
static uint8_t cadRepeat;  // CAD repeat counter

static void startCad() {
  Radio.Standby();
  Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10,
                     LORA_CAD_ONLY, 0);
  SX126xSetDioIrqParams(IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED,
                        IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  cadTime = millis();
  Radio.StartCad();
}

// Event declarations
static void onTxDone(void) { Radio.Rx(RX_TIMEOUT_VALUE); }
static void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi,
                     int8_t snr) {}
static void onTxTimeout(void) { Radio.Rx(RX_TIMEOUT_VALUE); }
static void onRxTimeout(void) {}
static void onRxError(void) {}
static void onCadDone(bool cadResult) {
  cadRepeat++;
  Radio.Standby();
  if (cadResult) {
    if (cadRepeat < 6) {
      // Retry CAD
      startCad();
    }
  } else {
    uint32_t cadtime = millis() - cadTime;
    cadRepeat = 0;
    if (sendLength > 0) {
      sendStart = millis();
      Radio.Send(sendBuffer, sendLength);
      sendLength = 0;
    }
  }
}

void StartRx() { Radio.Rx(RX_TIMEOUT_VALUE); }

RadioEvents_t DefaultConf() {
  static RadioEvents_t RadioEvents;
  RadioEvents.TxDone = onTxDone;
  RadioEvents.RxDone = onRxDone;
  RadioEvents.TxTimeout = onTxTimeout;
  RadioEvents.RxTimeout = onRxTimeout;
  RadioEvents.RxError = onRxError;
  RadioEvents.CadDone = onCadDone;
  return RadioEvents;
}

void SetupLoRa(uint8_t ch, RadioEvents_t *conf) {
  lora_isp4520_init(SX1261);
  Radio.Init(conf);
  Radio.SetChannel(CHANNELS[ch]);
}

void SetupTx() {
  // Set Radio TX configuration
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON, true, 0,
                    0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
}

void SetupRx() {
  // Set Radio RX configuration
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, true, 0,
                    0, LORA_IQ_INVERSION_ON, true);
}

void LoRaIrqProc() { Radio.IrqProcess(); }

void SendLoRa(uint8_t *buff, uint16_t len) {
  memcpy(sendBuffer, buff, len);
  sendLength = len;
  startCad();
}
