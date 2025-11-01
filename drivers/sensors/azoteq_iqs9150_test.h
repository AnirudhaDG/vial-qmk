#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "compiler_support.h"
#include "i2c_master.h"
#include "pointing_device.h"
#include "util.h"

// TEST
#define rxs 9
#define txs 4
#define x_res 1000
#define y_res 1000


#define IQS9150_I2C_ADDRESS (0x56 << 1)  // Default I2C address

// Key register addresses from memory map
#define IQS9150_PRODUCT_NUM      0x1000
#define IQS9150_RELATIVE_X       0x1014
#define IQS9150_RELATIVE_Y       0x1016
#define IQS9150_GESTURE_X        0x1018
#define IQS9150_GESTURE_Y        0x101A
#define IQS9150_SINGLE_GESTURES  0x101C
#define IQS9150_TWO_GESTURES     0x101E
#define IQS9150_INFO_FLAGS       0x1020
#define IQS9150_TRACKPAD_FLAGS   0x1022
#define IQS9150_FINGER1_X        0x1024
#define IQS9150_FINGER1_Y        0x1026
#define IQS9150_TOUCH_STATUS     0x105C
#define IQS9150_BUTTON_OUTPUT    0x112A
#define IQS9150_SYSTEM_CONTROL   0x11BC
#define IQS9150_CONFIG_SETTINGS  0x11BE

// Configuration registers
#define IQS9150_TOTAL_RXS        0x11E3
#define IQS9150_TOTAL_TXS        0x11E4
#define IQS9150_X_RESOLUTION     0x11E6
#define IQS9150_Y_RESOLUTION     0x11E8
#define IQS9150_RXTX_MAPPING     0x1218

// Gesture bits (Single Finger Gestures register)
#define IQS9150_GESTURE_SINGLE_TAP       (1 << 0)
#define IQS9150_GESTURE_DOUBLE_TAP       (1 << 1)
#define IQS9150_GESTURE_TRIPLE_TAP       (1 << 2)
#define IQS9150_GESTURE_PRESS_HOLD       (1 << 3)
#define IQS9150_GESTURE_PALM             (1 << 4)
#define IQS9150_GESTURE_SWIPE_X_PLUS     (1 << 8)
#define IQS9150_GESTURE_SWIPE_X_MINUS    (1 << 9)
#define IQS9150_GESTURE_SWIPE_Y_PLUS     (1 << 10)
#define IQS9150_GESTURE_SWIPE_Y_MINUS    (1 << 11)

// Info Flags bits
#define IQS9150_INFO_SHOW_RESET          (1 << 7)
#define IQS9150_INFO_ALP_PROX_STATUS     (1 << 8)
#define IQS9150_INFO_GLOBAL_TP_TOUCH     (1 << 9)
#define IQS9150_INFO_SWITCH_PRESSED      (1 << 10)
#define IQS9150_INFO_GLOBAL_SNAP         (1 << 11)
#define IQS9150_INFO_ALP_PROX_TOGGLED    (1 << 12)
#define IQS9150_INFO_TP_TOUCH_TOGGLED    (1 << 13)
#define IQS9150_INFO_SWITCH_TOGGLED      (1 << 14)
#define IQS9150_INFO_SNAP_TOGGLED        (1 << 15)

// Trackpad Flags bits
#define IQS9150_TP_SATURATION            (1 << 7)
#define IQS9150_TP_TOO_MANY_FINGERS      (1 << 5)
#define IQS9150_TP_MOVEMENT_DETECTED     (1 << 4)

#ifndef IQS9150_I2C_TIMEOUT
#    define IQS9150_I2C_TIMEOUT 100
#endif

// Configuration structure (similar to iqs5xx)
typedef struct {
    uint8_t  total_rxs;      // Number of RX electrodes (columns)
    uint8_t  total_txs;      // Number of TX electrodes (rows)
    uint16_t x_resolution;   // X resolution in pixels
    uint16_t y_resolution;   // Y resolution in pixels
    bool     flip_x;         // Flip X axis
    bool     flip_y;         // Flip Y axis
    bool     swap_xy;        // Swap X and Y axes
} iqs9150_config_t;

// Data structure
typedef struct {
    int16_t  relative_x;
    int16_t  relative_y;
    int16_t  gesture_x;
    int16_t  gesture_y;
    uint16_t single_gestures;
    uint16_t two_gestures;
    uint16_t info_flags;
    uint16_t trackpad_flags;
    uint16_t button_output;
    uint8_t  touch_count;
} iqs9150_data_t;

// Function prototypes (similar to iqs5xx pattern)
void iqs9150_init(void);
bool iqs9150_get_data(iqs9150_data_t *data);
void iqs9150_set_resolution(void);
bool iqs9150_set_orientation(bool flip_x, bool flip_y, bool swap_xy);

// Utility functions
uint8_t iqs9150_get_gesture(iqs9150_data_t *data);
bool iqs9150_is_touch_active(iqs9150_data_t *data);
bool iqs9150_is_button_pressed(iqs9150_data_t *data, uint8_t button);

// Low-level functions
bool iqs9150_read_register(uint16_t address, uint8_t *data, uint16_t length);
bool iqs9150_write_register(uint16_t address, uint8_t *data, uint16_t length);
bool iqs9150_write_register16(uint16_t address, uint16_t data);

const pointing_device_driver_t azoteq_iqs9150_pointing_device_driver;

extern iqs9150_config_t iqs9150_config;
