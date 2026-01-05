#pragma once

#define LED_CAPS_LOCK_PIN A8

// #include "iqs9151.h"

#define MATRIX_ROWS 4
#define MATRIX_COLS 11


// WS2812 RGB LED configuration
#define WS2812_DI_PIN B11
#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_GRB
#define WS2812_TIMING 2000
#define WS2812_T1H 900
#define WS2812_T0H 280
#define WS2812_T1L 1100
#define WS2812_T0L 1720
#define WS2812_TRST_US 80
#define WS2812_BITBANG_NOP_FUDGE 0.4

// #define POINTING_DEVICE_DEBUG
// #define CONSOLE_ENABLE

#define RGBLIGHT_LED_COUNT 10

#define RGBLIGHT_LED_MAP { \
    0, 1,                 /* Top row: 2 corner LEDs */ \
    2, 3, 4, 5,           /* Between row 1-2: 4 LEDs */ \
    6, 7, 8, 9            /* Between row 3-4: 4 LEDs */ \
}

// #define RGB_MATRIX_ENABLE

#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_SNAKE

#define RGBLIGHT_GRADIENT_RANGE 100  // Adjust gradient range
#define RGBLIGHT_HUE_STEP 10
#define RGBLIGHT_SAT_STEP 17
#define RGBLIGHT_VAL_STEP 17

// #define POINTING_DEVICE_DRIVER_azoteq_iqs9150
#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN B7
#define I2C1_SCL_PIN A15
#define I2C1_TIMINGR_PRESC 0U
#define I2C1_TIMINGR_SCLDEL 3U
#define I2C1_TIMINGR_SDADEL 1U
#define I2C1_TIMINGR_SCLH 3U
#define I2C1_TIMINGR_SCLL 9U
#define AZOTEQ_IQS9150_ADDRESS (0x56 << 1)  // I2C address (7-bit address shifted)
#define AZOTEQ_IQS9150_TIMEOUT_MS 100

#define I2C1_CLOCK_SPEED 400000

#define AZOTEQ_IQS9150_USE_DEFAULT_SETTINGS

#define IQS9150_INIT_H



// #define AZOTEQ_IQS5XX_ADDRESS 0x56

// #define AZOTEQ_IQS5XX_TPS65

// #define POINTING_DEVICE_ENABLE
// #define POINTING_DEVICE_DRIVER custom

// #define TRACKPAD_RXS 9   // Number of RX electrodes (columns)
// #define TRACKPAD_TXS 4   // Number of TX electrodes (rows)
// #define TRACKPAD_X_RES 1000  // X resolution
// #define TRACKPAD_Y_RES 1000  // Y resolution
