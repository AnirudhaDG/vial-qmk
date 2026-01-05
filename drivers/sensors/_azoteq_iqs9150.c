// Copyright 2025 Azoteq (Pty) Ltd
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdbool.h>
#include "azoteq_iqs9150.h"
#include "pointing_device_internal.h"
#include "wait.h"
#include "print.h"
#include <gpio.h>

// #define IQS9150_INIT_H

// #define SETUP

#define AZOTEQ_IQS9150_USE_DEFAULT_SETTINGS

// Bit manipulation macro
#ifndef BIT
#define BIT(n) (1U << (n))
#endif

#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (32 - 1 - (h))))
#endif

// Register definitions based on 9150 Linux driver
#define IQS9150_PROD_NUM                    0x1000
#define IQS9150_STATUS                      0x1018
#define IQS9150_REG_BUF_START               0x115C
#define IQS9150_REG_BUF_LEN                 (0x14F0 - IQS9150_REG_BUF_START)
#define IQS9150_SETTINGS_MINOR              0x1178
#define IQS9150_SETTINGS_MAJOR              0x1179
#define IQS9150_RATE_IDLE_TOUCH             0x11A4
#define IQS9150_TIMEOUT_COMMS               0x11B8
#define IQS9150_CONTROL                     0x11BC
#define IQS9150_CONFIG                      0x11BE
#define IQS9150_OTHER                       0x11C0
#define IQS9150_ALP_SETUP                   0x11C5
#define IQS9150_NUM_CONTACTS                0x11E5
#define IQS9150_X_RES                       0x11E6
#define IQS9150_Y_RES                       0x11E8
#define IQS9150_END_COMMS                   0xEEEE

#define IQS9150_9151_MM_INFO_FLAGS                       0x1020
#define IQS9150_9151_MM_TRACKPAD_FLAGS                   0x1022

// Control register bits
#define IQS9150_CONTROL_SUSPEND             BIT(11)
// #define IQS9150_CONTROL_ACK_RESET           BIT(7)
#define IQS9150_CONTROL_ACK_RESET           0x80
#define IQS9150_CONTROL_ATI_ALP             BIT(6)
#define IQS9150_CONTROL_ATI_TP              BIT(5)

// Config register bits
#define IQS9150_CONFIG_EVENT_MASK           GENMASK(15, 9)
#define IQS9150_CONFIG_EVENT_ATI            BIT(11)
#define IQS9150_CONFIG_EVENT_MODE           BIT(8)
#define IQS9150_CONFIG_FORCED_COMMS         BIT(4)

// Status info bits
#define IQS9150_INFO_GLOBAL_TOUCH           BIT(9)
#define IQS9150_INFO_SHOW_RESET             BIT(7)
#define IQS9150_INFO_ALP_ATI_AGAIN          BIT(6)
#define IQS9150_INFO_ALP_ATI_ERROR          BIT(5)
#define IQS9150_INFO_TP_ATI_AGAIN           BIT(4)
#define IQS9150_INFO_TP_ATI_ERROR           BIT(3)
#define IQS9150_INFO_CHARGE_MODE            GENMASK(2, 0)
#define IQS9150_INFO_CHARGE_MODE_LP1        3

// Other register bits
#define IQS9150_OTHER_SW_ENABLE             BIT(15)
#define IQS9150_ALP_SETUP_ENABLE            BIT(7)

// Communication constants
#define IQS9150_COMMS_ERROR                 0xEEEE
#define IQS9150_COMMS_RETRY_MS              50
#define IQS9150_NUM_RETRIES                 5
#define IQS9150_MAX_LEN                     256
#define IQS9150_MAX_CONTACTS                7

static i2c_status_t azoteq_iqs9150_load_settings(void) {
#ifdef AZOTEQ_IQS9150_USE_DEFAULT_SETTINGS
    i2c_status_t status = I2C_STATUS_SUCCESS;
    uint8_t settings_buffer[IQS9150_REG_BUF_LEN];
    uint16_t buffer_pos = 0;
    
    uprintf("IQS9150: Loading complete device settings\n");
    
    // ALP ATI Compensation (0x115C - 0x1175)
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX0_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX0_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX1_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX1_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX2_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX2_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX3_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX3_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX4_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX4_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX5_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX5_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX6_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX6_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX7_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX7_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX8_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX8_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX9_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX9_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX10_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX10_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX11_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX11_1;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX12_0;
    settings_buffer[buffer_pos++] = ALP_COMPENSATION_RX12_1;
    
    // I2C Slave Address (0x1176 - 0x1177)
    settings_buffer[buffer_pos++] = I2C_KEY;
    settings_buffer[buffer_pos++] = I2C_SLAVE_ADR;
    
    // Settings Version Numbers (0x1178 - 0x1179)
    settings_buffer[buffer_pos++] = MINOR_VERSION;
    settings_buffer[buffer_pos++] = MAJOR_VERSION;
    
    // ATI Multipliers / Dividers (0x117A - 0x1195)
    settings_buffer[buffer_pos++] = TP_ATI_MULTDIV_L;
    settings_buffer[buffer_pos++] = TP_ATI_MULTDIV_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX0_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX0_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX1_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX1_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX2_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX2_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX3_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX3_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX4_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX4_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX5_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX5_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX6_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX6_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX7_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX7_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX8_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX8_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX9_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX9_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX10_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX10_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX11_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX11_H;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX12_L;
    settings_buffer[buffer_pos++] = ALP_ATI_COARSE_RX12_H;
    
    // ATI Settings (0x1196 - 0x11A1)
    settings_buffer[buffer_pos++] = TP_ATI_TARGET_0;
    settings_buffer[buffer_pos++] = TP_ATI_TARGET_1;
    settings_buffer[buffer_pos++] = ALP_ATI_TARGET_0;
    settings_buffer[buffer_pos++] = ALP_ATI_TARGET_1;
    settings_buffer[buffer_pos++] = ALP_BASE_TARGET_0;
    settings_buffer[buffer_pos++] = ALP_BASE_TARGET_1;
    settings_buffer[buffer_pos++] = TP_NEG_DELTA_REATI_0;
    settings_buffer[buffer_pos++] = TP_NEG_DELTA_REATI_1;
    settings_buffer[buffer_pos++] = TP_POS_DELTA_REATI_0;
    settings_buffer[buffer_pos++] = TP_POS_DELTA_REATI_1;
    settings_buffer[buffer_pos++] = TP_REF_DRIFT_LIMIT;
    settings_buffer[buffer_pos++] = ALP_LTA_DRIFT_LIMIT;
    
    // Sampling Periods and Timing (0x11A2 - 0x11BB)
    settings_buffer[buffer_pos++] = ACTIVE_MODE_SAMPLING_PERIOD_0;
    settings_buffer[buffer_pos++] = ACTIVE_MODE_SAMPLING_PERIOD_1;
    settings_buffer[buffer_pos++] = IDLE_TOUCH_MODE_SAMPLING_PERIOD_0;
    settings_buffer[buffer_pos++] = IDLE_TOUCH_MODE_SAMPLING_PERIOD_1;
    settings_buffer[buffer_pos++] = IDLE_MODE_SAMPLING_PERIOD_0;
    settings_buffer[buffer_pos++] = IDLE_MODE_SAMPLING_PERIOD_1;
    settings_buffer[buffer_pos++] = LP1_MODE_SAMPLING_PERIOD_0;
    settings_buffer[buffer_pos++] = LP1_MODE_SAMPLING_PERIOD_1;
    settings_buffer[buffer_pos++] = LP2_MODE_SAMPLING_PERIOD_0;
    settings_buffer[buffer_pos++] = LP2_MODE_SAMPLING_PERIOD_1;
    settings_buffer[buffer_pos++] = STATIONARY_TOUCH_TIMEOUT_0;
    settings_buffer[buffer_pos++] = STATIONARY_TOUCH_TIMEOUT_1;
    settings_buffer[buffer_pos++] = IDLE_TOUCH_MODE_TIMEOUT_0;
    settings_buffer[buffer_pos++] = IDLE_TOUCH_MODE_TIMEOUT_1;
    settings_buffer[buffer_pos++] = IDLE_MODE_TIMEOUT_0;
    settings_buffer[buffer_pos++] = IDLE_MODE_TIMEOUT_1;
    settings_buffer[buffer_pos++] = LP1_MODE_TIMEOUT_0;
    settings_buffer[buffer_pos++] = LP1_MODE_TIMEOUT_1;
    settings_buffer[buffer_pos++] = ACTIVE_MODE_TIMEOUT_0;
    settings_buffer[buffer_pos++] = ACTIVE_MODE_TIMEOUT_1;
    settings_buffer[buffer_pos++] = REATI_RETRY_TIME;
    settings_buffer[buffer_pos++] = REF_UPDATE_TIME;
    settings_buffer[buffer_pos++] = I2C_TIMEOUT_0;
    settings_buffer[buffer_pos++] = I2C_TIMEOUT_1;
    settings_buffer[buffer_pos++] = SNAP_TIMEOUT;
    settings_buffer[buffer_pos++] = OPEN_TIMING;
    
    // System Settings (0x11BC - 0x11C1)
    settings_buffer[buffer_pos++] = SYSTEM_CONTROL_0;
    settings_buffer[buffer_pos++] = SYSTEM_CONTROL_1;
    settings_buffer[buffer_pos++] = CONFIG_SETTINGS_0;
    settings_buffer[buffer_pos++] = CONFIG_SETTINGS_1;
    settings_buffer[buffer_pos++] = OTHER_SETTINGS_0;
    settings_buffer[buffer_pos++] = OTHER_SETTINGS_1;
    
    // ALP Settings (0x11C2 - 0x11CB)
    settings_buffer[buffer_pos++] = ALP_SETUP_0;
    settings_buffer[buffer_pos++] = ALP_SETUP_1;
    settings_buffer[buffer_pos++] = ALP_SETUP_2;
    settings_buffer[buffer_pos++] = ALP_SETUP_3;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_0;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_1;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_2;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_3;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_4;
    settings_buffer[buffer_pos++] = ALP_TX_ENABLE_5;
    
    // Thresholds and Debounce Settings (0x11CC - 0x11D3)
    settings_buffer[buffer_pos++] = TRACKPAD_TOUCH_SET_THRESHOLD;
    settings_buffer[buffer_pos++] = TRACKPAD_TOUCH_CLEAR_THRESHOLD;
    settings_buffer[buffer_pos++] = ALP_THRESHOLD;
    settings_buffer[buffer_pos++] = ALP_AUTOPROX_THRESHOLD;
    settings_buffer[buffer_pos++] = ALP_SET_DEBOUNCE;
    settings_buffer[buffer_pos++] = ALP_CLEAR_DEBOUNCE;
    settings_buffer[buffer_pos++] = SNAP_SET_THRESHOLD;
    settings_buffer[buffer_pos++] = SNAP_CLEAR_THRESHOLD;
    
    // ALP Count and LTA Betas (0x11D4 - 0x11D7)
    settings_buffer[buffer_pos++] = ALP_COUNT_BETA_LP1;
    settings_buffer[buffer_pos++] = ALP_LTA_BETA_LP1;
    settings_buffer[buffer_pos++] = ALP_COUNT_BETA_LP2;
    settings_buffer[buffer_pos++] = ALP_LTA_BETA_LP2;
    
    // Hardware Settings (0x11D8 - 0x11E1)
    settings_buffer[buffer_pos++] = TP_FRAC;
    settings_buffer[buffer_pos++] = TP_PERIOD1;
    settings_buffer[buffer_pos++] = TP_PERIOD2;
    settings_buffer[buffer_pos++] = ALP_FRAC;
    settings_buffer[buffer_pos++] = ALP_PERIOD1;
    settings_buffer[buffer_pos++] = ALP_PERIOD2;
    settings_buffer[buffer_pos++] = TRACKPAD_HARDWARE_SETTINGS_0;
    settings_buffer[buffer_pos++] = TRACKPAD_HARDWARE_SETTINGS_1;
    settings_buffer[buffer_pos++] = ALP_HARDWARE_SETTINGS_0;
    settings_buffer[buffer_pos++] = ALP_HARDWARE_SETTINGS_1;
    
    // Trackpad Settings (0x11E2 - 0x11F5) - Skip NUM_CONTACTS at 0x11E5
    settings_buffer[buffer_pos++] = TRACKPAD_SETTINGS_0_0;
    settings_buffer[buffer_pos++] = TRACKPAD_SETTINGS_0_1;
    settings_buffer[buffer_pos++] = TRACKPAD_SETTINGS_1_0;
    settings_buffer[buffer_pos++] = TRACKPAD_SETTINGS_1_1;
    settings_buffer[buffer_pos++] = 0x00; // 0x11E5 - NUM_CONTACTS (read-only, skip)
    settings_buffer[buffer_pos++] = X_RESOLUTION_0;
    settings_buffer[buffer_pos++] = X_RESOLUTION_1;
    settings_buffer[buffer_pos++] = Y_RESOLUTION_0;
    settings_buffer[buffer_pos++] = Y_RESOLUTION_1;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_0;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_BOTTOM_SPEED_1;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_TOP_SPEED_0;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_TOP_SPEED_1;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_BOTTOM_BETA;
    settings_buffer[buffer_pos++] = XY_DYNAMIC_FILTER_STATIC_FILTER_BETA;
    settings_buffer[buffer_pos++] = STATIONARY_TOUCH_MOV_THRESHOLD;
    settings_buffer[buffer_pos++] = FINGER_SPLIT_FACTOR;
    settings_buffer[buffer_pos++] = X_TRIM_VALUE;
    settings_buffer[buffer_pos++] = Y_TRIM_VALUE;
    settings_buffer[buffer_pos++] = JITTER_FILTER_DELTA;
    settings_buffer[buffer_pos++] = FINGER_CONFIDENCE_THRESHOLD;
    
    // Gesture Settings (0x11F6 - 0x1217)
    settings_buffer[buffer_pos++] = GESTURE_ENABLE_0;
    settings_buffer[buffer_pos++] = GESTURE_ENABLE_1;
    settings_buffer[buffer_pos++] = GESTURE_ENABLE_2F_0;
    settings_buffer[buffer_pos++] = GESTURE_ENABLE_2F_1;
    settings_buffer[buffer_pos++] = TAP_TOUCH_TIME_0;
    settings_buffer[buffer_pos++] = TAP_TOUCH_TIME_1;
    settings_buffer[buffer_pos++] = TAP_WAIT_TIME_0;
    settings_buffer[buffer_pos++] = TAP_WAIT_TIME_1;
    settings_buffer[buffer_pos++] = TAP_DISTANCE_0;
    settings_buffer[buffer_pos++] = TAP_DISTANCE_1;
    settings_buffer[buffer_pos++] = HOLD_TIME_0;
    settings_buffer[buffer_pos++] = HOLD_TIME_1;
    settings_buffer[buffer_pos++] = SWIPE_TIME_0;
    settings_buffer[buffer_pos++] = SWIPE_TIME_1;
    settings_buffer[buffer_pos++] = SWIPE_X_DISTANCE_0;
    settings_buffer[buffer_pos++] = SWIPE_X_DISTANCE_1;
    settings_buffer[buffer_pos++] = SWIPE_Y_DISTANCE_0;
    settings_buffer[buffer_pos++] = SWIPE_Y_DISTANCE_1;
    settings_buffer[buffer_pos++] = SWIPE_X_CONS_DIST_0;
    settings_buffer[buffer_pos++] = SWIPE_X_CONS_DIST_1;
    settings_buffer[buffer_pos++] = SWIPE_Y_CONS_DIST_0;
    settings_buffer[buffer_pos++] = SWIPE_Y_CONS_DIST_1;
    settings_buffer[buffer_pos++] = SWIPE_ANGLE;
    settings_buffer[buffer_pos++] = SCROLL_ANGLE;
    settings_buffer[buffer_pos++] = ZOOM_INIT_DIST_0;
    settings_buffer[buffer_pos++] = ZOOM_INIT_DIST_1;
    settings_buffer[buffer_pos++] = ZOOM_CONSECUTIVE_DIST_0;
    settings_buffer[buffer_pos++] = ZOOM_CONSECUTIVE_DIST_1;
    settings_buffer[buffer_pos++] = SCROLL_INIT_DIST_0;
    settings_buffer[buffer_pos++] = SCROLL_INIT_DIST_1;
    settings_buffer[buffer_pos++] = SCROLL_CONSECUTIVE_DIST_0;
    settings_buffer[buffer_pos++] = SCROLL_CONSECUTIVE_DIST_1;
    settings_buffer[buffer_pos++] = PALM_GESTURE_THRESHOLD_0;
    settings_buffer[buffer_pos++] = PALM_GESTURE_THRESHOLD_1;
    
    // Rx/Tx Mapping (0x1218 - 0x1245)
    settings_buffer[buffer_pos++] = RX_TX_MAP_0;
    settings_buffer[buffer_pos++] = RX_TX_MAP_1;
    settings_buffer[buffer_pos++] = RX_TX_MAP_2;
    settings_buffer[buffer_pos++] = RX_TX_MAP_3;
    settings_buffer[buffer_pos++] = RX_TX_MAP_4;
    settings_buffer[buffer_pos++] = RX_TX_MAP_5;
    settings_buffer[buffer_pos++] = RX_TX_MAP_6;
    settings_buffer[buffer_pos++] = RX_TX_MAP_7;
    settings_buffer[buffer_pos++] = RX_TX_MAP_8;
    settings_buffer[buffer_pos++] = RX_TX_MAP_9;
    settings_buffer[buffer_pos++] = RX_TX_MAP_10;
    settings_buffer[buffer_pos++] = RX_TX_MAP_11;
    settings_buffer[buffer_pos++] = RX_TX_MAP_12;
    settings_buffer[buffer_pos++] = RX_TX_MAP_13;
    settings_buffer[buffer_pos++] = RX_TX_MAP_14;
    settings_buffer[buffer_pos++] = RX_TX_MAP_15;
    settings_buffer[buffer_pos++] = RX_TX_MAP_16;
    settings_buffer[buffer_pos++] = RX_TX_MAP_17;
    settings_buffer[buffer_pos++] = RX_TX_MAP_18;
    settings_buffer[buffer_pos++] = RX_TX_MAP_19;
    settings_buffer[buffer_pos++] = RX_TX_MAP_20;
    settings_buffer[buffer_pos++] = RX_TX_MAP_21;
    settings_buffer[buffer_pos++] = RX_TX_MAP_22;
    settings_buffer[buffer_pos++] = RX_TX_MAP_23;
    settings_buffer[buffer_pos++] = RX_TX_MAP_24;
    settings_buffer[buffer_pos++] = RX_TX_MAP_25;
    settings_buffer[buffer_pos++] = RX_TX_MAP_26;
    settings_buffer[buffer_pos++] = RX_TX_MAP_27;
    settings_buffer[buffer_pos++] = RX_TX_MAP_28;
    settings_buffer[buffer_pos++] = RX_TX_MAP_29;
    settings_buffer[buffer_pos++] = RX_TX_MAP_30;
    settings_buffer[buffer_pos++] = RX_TX_MAP_31;
    settings_buffer[buffer_pos++] = RX_TX_MAP_32;
    settings_buffer[buffer_pos++] = RX_TX_MAP_33;
    settings_buffer[buffer_pos++] = RX_TX_MAP_34;
    settings_buffer[buffer_pos++] = RX_TX_MAP_35;
    settings_buffer[buffer_pos++] = RX_TX_MAP_36;
    settings_buffer[buffer_pos++] = RX_TX_MAP_37;
    settings_buffer[buffer_pos++] = RX_TX_MAP_38;
    settings_buffer[buffer_pos++] = RX_TX_MAP_39;
    settings_buffer[buffer_pos++] = RX_TX_MAP_40;
    settings_buffer[buffer_pos++] = RX_TX_MAP_41;
    settings_buffer[buffer_pos++] = RX_TX_MAP_42;
    settings_buffer[buffer_pos++] = RX_TX_MAP_43;
    settings_buffer[buffer_pos++] = RX_TX_MAP_44;
    settings_buffer[buffer_pos++] = RX_TX_OPEN;
    
    // TP Channel Disables (0x1246 - 0x129D) - 88 bytes of zeros in init file
    for (int i = 0; i < 88; i++) {
        settings_buffer[buffer_pos++] = 0x00;
    }
    
    // TP Snap Enable (0x129E - 0x12F5) - 88 bytes of zeros in init file
    for (int i = 0; i < 88; i++) {
        settings_buffer[buffer_pos++] = 0x00;
    }
    
    // Individual Touch Threshold Adjustments (0x12F6 - 0x14EF) - 506 bytes of zeros
    for (int i = 0; i < 506; i++) {
        settings_buffer[buffer_pos++] = 0x00;
    }
    
    uprintf("IQS9150: Writing %d bytes of settings starting at 0x%04X\n", buffer_pos, IQS9150_REG_BUF_START);
    
    // Write the entire settings buffer in one burst
    status = azoteq_iqs9150_write_burst(IQS9150_REG_BUF_START, settings_buffer, buffer_pos);
    // status = i2c_write_register16(AZOTEQ_IQS9150_ADDRESS, settings_buffer[0], 0, 1, AZOTEQ_IQS9150_TIMEOUT_MS);
    
    if (status == I2C_STATUS_SUCCESS) {
        uprintf("IQS9150: Settings loaded successfully\n");
    } else {
        uprintf("IQS9150: Failed to load settings, status: %d\n", status);
    }
    
    return status;
#else
    uprintf("IQS9150: IQS9150_init.h not included, skipping settings load\n");
    return I2C_STATUS_SUCCESS;
#endif
}


// Driver interface structure
const pointing_device_driver_t azoteq_iqs9150_pointing_device_driver = {
    .init       = azoteq_iqs9150_init,
    // .get_report = azoteq_iqs9150_get_report,
    // .set_cpi    = azoteq_iqs9150_set_cpi,
    // .get_cpi    = azoteq_iqs9150_get_cpi,
};

static void azoteq_iqs9150_scan_bus(void) {
    uprintf("IQS9150: I2C scan start\n");
    for (uint8_t addr7 = 0x03; addr7 <= 0x77; addr7++) {
        i2c_status_t s = i2c_ping_address((uint8_t)(addr7 << 1), 1); // QMK uses 8-bit (7-bit << 1)
        if (s == I2C_STATUS_SUCCESS) {
            uprintf("IQS9150: Found device at 0x%02X (8-bit 0x%02X)\n", addr7, (addr7 << 1));
        }
        wait_ms(2);
    }
    uprintf("IQS9150: I2C scan done\n");
}

// static void tester(void) {
//     uint8_t data[2];
//     i2c_status_t status = i2c_read_register16(0x56, 0x1000, (uint8_t *)data, 2, 10);
//     azoteq_iqs9150_end_session();
//     if (status != I2C_STATUS_SUCCESS) {
//         uprintf("niggesh\n");
//     }
//     else {
//         uprintf("read success: %d\n", 11);
//     }
// }

i2c_status_t azoteq_iqs9150_end_session(void) {
    const uint8_t END_BYTE = 1; // any data
    return i2c_write_register16(AZOTEQ_IQS9150_ADDRESS, IQS9150_END_COMMS, &END_BYTE, 1, AZOTEQ_IQS9150_TIMEOUT_MS);
}


// static void tester(void) {
//     // Wait for RDY low (A9) before starting I2C, per datasheet
//     // const uint16_t rdy_timeout_ms = 1000;
//     // uint16_t waited = 0;
//     // while (readPin(A9)) {
//     //     if (waited++ >= rdy_timeout_ms) {
//     //         uprintf("IQS9150: RDY high for %dms, proceeding anyway\n", rdy_timeout_ms);
//     //         break;
//     //     }
//     //     wait_ms(1);
//     // }

//     uprintf("tester");

//     // Read the 16-bit Product Number register (0x1000)
//     uint8_t buf[2] = {0};
//     i2c_status_t status = i2c_read_register16(AZOTEQ_IQS9150_ADDRESS, IQS9150_PROD_NUM, buf, sizeof(buf), 100);

//     // azoteq_iqs9150_end_session();

//     if (status == I2C_STATUS_SUCCESS) {
//         uint16_t product = ((uint16_t)buf[0] << 8) | buf[1]; // MSB first
//         uprintf("IQS9150: Product number read OK: 0x%04X\n", product);
//         azoteq_iqs9150_end_session();
//     } else {
//         uprintf("IQS9150: Product number read failed, i2c status: %d (addr 0x%02X)\n",
//                 status, AZOTEQ_IQS9150_ADDRESS);
//     }
// }

uint16_t azoteq_iqs9150_get_product(void) {
    // uprintf("Pin:%ld\n", readPin(A9));
    uint8_t ret = 0;
    uint8_t buf[2] = {0};
    i2c_status_t status = i2c_read_register16(AZOTEQ_IQS9150_ADDRESS, IQS9150_PROD_NUM, buf, sizeof(buf), 100);
    wait_ms(10);
    azoteq_iqs9150_end_session();
    
    if (status != I2C_STATUS_SUCCESS) {
        ret = 69;
        uprintf("Read unsuccessful\n");
    }
    uprintf("IQS9150: Product number 0x%04X\n", ret);
    return ret;
}

void azoteq_iqs9150_init(void){
    wait_ms(3000);
    uprintf("IQS9150: Starting init\n");
    i2c_init();

    i2c_ping_address(AZOTEQ_IQS9150_ADDRESS, 1);
    uprintf("Pinging: 0x%02X\n", AZOTEQ_IQS9150_ADDRESS);
    azoteq_iqs9150_scan_bus();
    azoteq_iqs9150_get_product();
}