#ifndef HAB_ISP4520C_GW_R1_H
#define HAB_ISP4520C_GW_R1_H

#define _PINNUM(port, pin) ((port)*32 + (pin))

/*------------------------------------------------------------------*/
/* LED
 *------------------------------------------------------------------*/
#define LEDS_NUMBER 1
#define LED_PRIMARY_PIN _PINNUM(0, 13)   // Red
#define LED_SECONDARY_PIN _PINNUM(0, 9)  // Green
#define LED_STATE_ON 0

/*------------------------------------------------------------------*/
/* BUTTON
 *------------------------------------------------------------------*/
#define BUTTONS_NUMBER 2

#define BUTTON_1 _PINNUM(0, 30)
#define BUTTON_2 _PINNUM(0, 31)
#define BUTTON_PULL NRF_GPIO_PIN_PULLUP

/*------------------------------------------------------------------*/
/* UART
 *------------------------------------------------------------------*/
#define RX_PIN_NUMBER 8
#define TX_PIN_NUMBER 6
#define CTS_PIN_NUMBER 0
#define RTS_PIN_NUMBER 0
#define HWFC false

// Used as model string in OTA mode
#define BLEDIS_MANUFACTURER "InsightSiP"
#define BLEDIS_MODEL "isp4520"

#define UF2_PRODUCT_NAME "InsightSiP ISP4520C_GW_R1"
#define UF2_INDEX_URL \
  "https://www.insightsip.com/products/combo-smart-modules/isp4520"

#endif  // HAB_ISP4520C_GW_R1_H
