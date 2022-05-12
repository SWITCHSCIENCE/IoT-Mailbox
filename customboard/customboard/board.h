#ifndef HAB_CUSTOMBOARD_H
#define HAB_CUSTOMBOARD_H

#define _PINNUM(port, pin) ((port)*32 + (pin))

/*------------------------------------------------------------------*/
/* LED
 *------------------------------------------------------------------*/
#define LEDS_NUMBER 1
#define LED_PRIMARY_PIN _PINNUM(0, 4)    // Red
#define LED_SECONDARY_PIN _PINNUM(0, 5)  // Green
#define LED_STATE_ON 0

/*------------------------------------------------------------------*/
/* BUTTON
 *------------------------------------------------------------------*/
#define BUTTONS_NUMBER 2

#define BUTTON_1 _PINNUM(0, 6)
#define BUTTON_2 _PINNUM(0, 13)
#define BUTTON_PULL NRF_GPIO_PIN_PULLUP

/*------------------------------------------------------------------*/
/* UART
 *------------------------------------------------------------------*/
#define RX_PIN_NUMBER 2
#define TX_PIN_NUMBER 31
#define CTS_PIN_NUMBER 0
#define RTS_PIN_NUMBER 0
#define HWFC false

// Used as model string in OTA mode
#define BLEDIS_MANUFACTURER "SwitchScience"
#define BLEDIS_MODEL "CustomBoard"

#define UF2_PRODUCT_NAME "SwitchScience CustomBoard"
#define UF2_INDEX_URL "https://www.switchscience.com/"

#endif  // HAB_CUSTOMBOARD_H
