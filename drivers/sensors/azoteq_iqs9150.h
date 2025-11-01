// Copyright 2025 Azoteq (Pty) Ltd
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include "compiler_support.h"
#include "i2c_master.h"
#include "pointing_device.h"
#include "util.h"

#define POINTING_DEVICE_DEBUG

// Product numbers
typedef enum {
    AZOTEQ_IQS9150_UNKNOWN,
    AZOTEQ_IQS9150 = 0x1000,
    AZOTEQ_IQS9151 = 0x1001,
} azoteq_iqs9150_product_numbers_t;

// Communication modes
typedef enum {
    AZOTEQ_IQS9150_COMMS_MODE_WAIT,
    AZOTEQ_IQS9150_COMMS_MODE_FREE,
    AZOTEQ_IQS9150_COMMS_MODE_FORCE,
} azoteq_iqs9150_comms_mode_t;

// Charging/power modes
typedef enum {
    AZOTEQ_IQS9150_ACTIVE,
    AZOTEQ_IQS9150_IDLE_TOUCH,
    AZOTEQ_IQS9150_IDLE,
    AZOTEQ_IQS9150_LP1,
    AZOTEQ_IQS9150_LP2,
} azoteq_iqs9150_charging_modes_t;

// Version information structure
typedef struct PACKED {
    uint16_t prod_num;
    uint16_t major;
    uint16_t minor;
    uint32_t patch;
} azoteq_iqs9150_ver_info_t;

// Touch data for single contact
typedef struct PACKED {
    uint16_t abs_x;
    uint16_t abs_y;
    uint16_t pressure;
    uint16_t area;
} azoteq_iqs9150_touch_data_t;

// System info flags
typedef struct PACKED {
    bool global_touch : 1;      // Global touch status
    bool show_reset : 1;        // Device reset indication
    bool alp_ati_again : 1;     // ALP ATI reoccurrence
    bool alp_ati_error : 1;     // ALP ATI error
    bool tp_ati_again : 1;      // TP ATI reoccurrence
    bool tp_ati_error : 1;      // TP ATI error
    uint8_t charge_mode : 3;    // Current charging mode
    uint8_t _unused : 6;        // unused bits
} azoteq_iqs9150_system_info_t;

// Gesture events structure
typedef struct PACKED {
    bool tap : 1;               // Single tap gesture
    bool hold : 1;              // Press and hold gesture
    bool swipe_x_pos : 1;       // Swipe positive X
    bool swipe_x_neg : 1;       // Swipe negative X
    bool swipe_y_pos : 1;       // Swipe positive Y
    bool swipe_y_neg : 1;       // Swipe negative Y
    bool two_finger_tap : 1;    // Two finger tap
    bool scroll : 1;            // Scroll gesture
    bool zoom : 1;              // Zoom gesture
    uint8_t _unused : 7;        // unused bits
} azoteq_iqs9150_gesture_events_t;

// Status report structure
typedef struct PACKED {
    uint16_t gesture_x;
    uint16_t gesture_y;
    azoteq_iqs9150_system_info_t system_info;
    azoteq_iqs9150_gesture_events_t gesture_events;
    uint8_t num_contacts;
    azoteq_iqs9150_touch_data_t touch_data[7]; // Max 7 contacts
} azoteq_iqs9150_status_t;

// Base report data (minimal for QMK)
typedef struct {
    uint8_t num_contacts;
    int16_t x;
    int16_t y;
    azoteq_iqs9150_gesture_events_t gestures;
    azoteq_iqs9150_system_info_t info;
} azoteq_iqs9150_base_data_t;

// Resolution structure
typedef struct {
    uint16_t x_resolution;
    uint16_t y_resolution;
} azoteq_iqs9150_resolution_t;

// Control structure
typedef struct PACKED {
    bool suspend : 1;
    bool ack_reset : 1;
    bool ati_alp : 1;
    bool ati_tp : 1;
    uint8_t _unused : 4;
} azoteq_iqs9150_control_t;

// Configuration structure
typedef struct PACKED {
    bool event_mode : 1;
    bool forced_comms : 1;
    bool event_ati : 1;
    uint8_t _unused : 5;
} azoteq_iqs9150_config_t;

// Byte manipulation macros
#define AZOTEQ_IQS9150_COMBINE_H_L_BYTES(h, l) ((int16_t)((h << 8) | l))
#define AZOTEQ_IQS9150_SWAP_H_L_BYTES(b) ((uint16_t)(((b) & 0xff) << 8) | ((b) >> 8))

// Default configurations
#ifndef AZOTEQ_IQS9150_ADDRESS
#    define AZOTEQ_IQS9150_ADDRESS (0x56 << 1)
#endif

#ifndef AZOTEQ_IQS9150_TIMEOUT_MS
#    define AZOTEQ_IQS9150_TIMEOUT_MS 10
#endif

#ifndef AZOTEQ_IQS9150_REPORT_RATE
#    define AZOTEQ_IQS9150_REPORT_RATE 10
#endif

// Register buffer definitions
#define IQS9150_REG_BUF_START           0x115C

// Gesture configuration defaults
#ifndef AZOTEQ_IQS9150_TAP_ENABLE
#    define AZOTEQ_IQS9150_TAP_ENABLE true
#endif

#ifndef AZOTEQ_IQS9150_HOLD_ENABLE
#    define AZOTEQ_IQS9150_HOLD_ENABLE false
#endif

#ifndef AZOTEQ_IQS9150_TWO_FINGER_TAP_ENABLE
#    define AZOTEQ_IQS9150_TWO_FINGER_TAP_ENABLE true
#endif

#ifndef AZOTEQ_IQS9150_SCROLL_ENABLE
#    define AZOTEQ_IQS9150_SCROLL_ENABLE true
#endif

#ifndef AZOTEQ_IQS9150_SWIPE_X_ENABLE
#    define AZOTEQ_IQS9150_SWIPE_X_ENABLE false
#endif

#ifndef AZOTEQ_IQS9150_SWIPE_Y_ENABLE
#    define AZOTEQ_IQS9150_SWIPE_Y_ENABLE false
#endif

#ifndef AZOTEQ_IQS9150_ZOOM_ENABLE
#    define AZOTEQ_IQS9150_ZOOM_ENABLE false
#endif

// Resolution defaults (need to be set based on physical trackpad)
#ifndef AZOTEQ_IQS9150_WIDTH_MM
#    define AZOTEQ_IQS9150_WIDTH_MM 65
#endif

#ifndef AZOTEQ_IQS9150_HEIGHT_MM
#    define AZOTEQ_IQS9150_HEIGHT_MM 49
#endif

#ifndef AZOTEQ_IQS9150_RESOLUTION_X
#    define AZOTEQ_IQS9150_RESOLUTION_X 3072
#endif

#ifndef AZOTEQ_IQS9150_RESOLUTION_Y
#    define AZOTEQ_IQS9150_RESOLUTION_Y 2048
#endif

// CPI conversion macros
#define DIVIDE_UNSIGNED_ROUND(numerator, denominator) (((numerator) + ((denominator) / 2)) / (denominator))
#define AZOTEQ_IQS9150_INCH_TO_RESOLUTION_X(inch) (DIVIDE_UNSIGNED_ROUND((inch) * (uint32_t)AZOTEQ_IQS9150_WIDTH_MM * 10, 254))
#define AZOTEQ_IQS9150_RESOLUTION_X_TO_INCH(px) (DIVIDE_UNSIGNED_ROUND((px) * (uint32_t)254, AZOTEQ_IQS9150_WIDTH_MM * 10))
#define AZOTEQ_IQS9150_INCH_TO_RESOLUTION_Y(inch) (DIVIDE_UNSIGNED_ROUND((inch) * (uint32_t)AZOTEQ_IQS9150_HEIGHT_MM * 10, 254))
#define AZOTEQ_IQS9150_RESOLUTION_Y_TO_INCH(px) (DIVIDE_UNSIGNED_ROUND((px) * (uint32_t)254, AZOTEQ_IQS9150_HEIGHT_MM * 10))

// Throttle to match report rate
#if !defined(POINTING_DEVICE_TASK_THROTTLE_MS) && !defined(POINTING_DEVICE_MOTION_PIN)
#    define POINTING_DEVICE_TASK_THROTTLE_MS AZOTEQ_IQS9150_REPORT_RATE + 1
#endif

// Driver interface
extern const pointing_device_driver_t azoteq_iqs9150_pointing_device_driver;

// Function declarations
void           azoteq_iqs9150_init(void);
report_mouse_t azoteq_iqs9150_get_report(report_mouse_t mouse_report);
void           azoteq_iqs9150_set_cpi(uint16_t cpi);
uint16_t       azoteq_iqs9150_get_cpi(void);
uint16_t       azoteq_iqs9150_get_product(void);
i2c_status_t   azoteq_iqs9150_wake(void);
i2c_status_t   azoteq_iqs9150_end_session(void);
i2c_status_t   azoteq_iqs9150_reset_suspend(bool reset, bool suspend, bool end_session);
i2c_status_t   azoteq_iqs9150_get_base_data(azoteq_iqs9150_base_data_t *base_data);
i2c_status_t   azoteq_iqs9150_set_report_rate(uint16_t report_rate_ms, azoteq_iqs9150_charging_modes_t mode, bool end_session);
i2c_status_t   azoteq_iqs9150_set_event_mode(bool enabled, bool end_session);
i2c_status_t   azoteq_iqs9150_set_xy_config(bool flip_x, bool flip_y, bool switch_xy, bool end_session);
