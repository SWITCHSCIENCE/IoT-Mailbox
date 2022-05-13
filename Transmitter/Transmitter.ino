#include <Arduino.h>

#include "lora_handler.h"

#define TRIGGER 12
#define POWERSAVE

#ifdef POWERSAVE
#define Log(X) \
  { ; }
#define Logf(f, ...) \
  { ; }
#define Logln \
  { ; }
#define LogBuffer \
  { ; }
#else
#define Log(x) Serial.print(x)
#define Logf(f, ...) Serial.printf(f, __VA_ARGS__)
#define Logln(x) Serial.println(x)
#define LogBuffer(b, l) Serial.printBuffer(b, l)
#endif

RadioEvents_t DefaultConf();
void SetupLoRa(uint8_t ch, RadioEvents_t *conf);
void SetupTx();
void SendLoRa(uint8_t *buff, uint16_t len);
void BoardGetUniqueId(uint8_t *id);

static uint32_t doSampling() {
  volatile uint32_t result = 0;
  uint32_t res;

  // Configure result to be put in RAM at the location of "result" variable.
  NRF_SAADC->RESULT.MAXCNT = 1;
  NRF_SAADC->RESULT.PTR = (uint32_t)(&result);

  // No automatic sampling, will trigger with TASKS_SAMPLE.
  NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task
                          << SAADC_SAMPLERATE_MODE_Pos;

  // Enable SAADC (would capture analog pins if they were used in CH[0].PSELP)
  NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;

  // Calibrate the SAADC (only needs to be done once in a while)
  NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
  while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0) yield();
  NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
  while (NRF_SAADC->STATUS ==
         (SAADC_STATUS_STATUS_Busy << SAADC_STATUS_STATUS_Pos))
    yield();

  // Start the SAADC and wait for the started event.
  NRF_SAADC->TASKS_START = 1;
  while (NRF_SAADC->EVENTS_STARTED == 0) yield();
  NRF_SAADC->EVENTS_STARTED = 0;

  // Do a SAADC sample, will put the result in the configured RAM buffer.
  NRF_SAADC->TASKS_SAMPLE = 1;
  while (NRF_SAADC->EVENTS_END == 0) yield();
  NRF_SAADC->EVENTS_END = 0;

  res = result;

  // Stop the SAADC, since it's not used anymore.
  NRF_SAADC->TASKS_STOP = 1;
  while (NRF_SAADC->EVENTS_STOPPED == 0) yield();
  NRF_SAADC->EVENTS_STOPPED = 0;

  return res;
}

float measureVdd() {
  float precise_result = 0;

  NRF_SAADC->CH[0].CONFIG =
      (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
      (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
      (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
      (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
      (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos) |
      (SAADC_CH_CONFIG_TACQ_3us << SAADC_CH_CONFIG_TACQ_Pos);

  NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos;
  NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;

  // Configure the SAADC resolution.
  NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit
                          << SAADC_RESOLUTION_VAL_Pos;

  // Result = [V(p) - V(n)] * GAIN/REFERENCE * 2^(RESOLUTION)
  // Result = (VDD - 0) * (1./6) / 0.6 * 2**10
  // VDD = Result / 284.4
  precise_result = (float)(doSampling() & 0x3ff) / 284.4f;

  return precise_result;
}

void transmit() {
  static uint8_t msg[11];
  BoardGetUniqueId(msg);
  *(uint16_t *)(&msg[8]) = (uint16_t)(measureVdd() * 100);
  msg[10] = NRF_POWER->GPREGRET;
  SendLoRa(msg, 11);
}

void powerOff() {
  Logln("power off");
  yield();

  Radio.Sleep();

  nrf_gpio_pin_sense_t sense = (nrf_gpio_pin_read(TRIGGER))
                                   ? NRF_GPIO_PIN_SENSE_LOW
                                   : NRF_GPIO_PIN_SENSE_HIGH;
  NRF_POWER->GPREGRET = sense;
  nrf_gpio_cfg_sense_set(TRIGGER, sense);
  NRF_POWER->SYSTEMOFF = 1;  // sd_power_system_off();
}

bool ack;
bool transSuccess;
bool transEnd;

void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  if (size == 1 && payload[0] == 0x06) {
    ack = true;
  }
}

void transaction() {
  for (int i = 0; i < 3; i++) {
    transEnd = false;
    transmit();
    while (!transEnd) delay(1000);
    if (transSuccess) {
      ack = false;
      StartRx();
      delay(3000);
      if (ack) {
        Logln("send completed");
        break;
      }
    }
  }
  powerOff();
}

static void onTxDone(void) {
  Logf("Transmit finished: %d ms\n", millis());
  transSuccess = true;
  transEnd = true;
}

static void onTxTimeout(void) {
  Logf("Transmit timeout: %d ms\n", millis());
  transSuccess = false;
  transEnd = true;
}

void setup(void) {
  NRF_POWER->DCDCEN = 1;
  static RadioEvents_t conf;
  pinMode(TRIGGER, INPUT_PULLUP);
// Initialize Serial for debug output
#ifndef POWERSAVE
  Serial.begin(115200);
#endif
  Logln("boot!");

  // Initialize LoRa
  conf = DefaultConf();
  conf.RxDone = onRxDone;
  conf.TxDone = onTxDone;
  conf.TxTimeout = onTxTimeout;
  SetupLoRa(0, &conf);
  SetupTx();
  SetupRx();

  Logln("LoRa init success");

  Logln("transmit...");
  transaction();
}

void loop(void) {
  LoRaIrqProc();
  delay(100);
}
