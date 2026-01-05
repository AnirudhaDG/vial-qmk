// Copyright 2025 Azoteq (Pty) Ltd
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdbool.h>
#include "azoteq_iqs9150.h"
#include "pointing_device_internal.h"
#include "wait.h"
#include "print.h"
#include <gpio.h>

#define IQS9150_INIT_H

#define SETUP

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
#define IQS9150_CONTROL_ACK_RESET           0x82
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


#ifdef SETUP
    #define ALP_COMPENSATION_RX0_0                   0xD8
    #define ALP_COMPENSATION_RX0_1                   0x12
    #define ALP_COMPENSATION_RX1_0                   0xC6
    #define ALP_COMPENSATION_RX1_1                   0x12
    #define ALP_COMPENSATION_RX2_0                   0xD4
    #define ALP_COMPENSATION_RX2_1                   0x12
    #define ALP_COMPENSATION_RX3_0                   0xD8
    #define ALP_COMPENSATION_RX3_1                   0x12
    #define ALP_COMPENSATION_RX4_0                   0xCD
    #define ALP_COMPENSATION_RX4_1                   0x12
    #define ALP_COMPENSATION_RX5_0                   0xCC
    #define ALP_COMPENSATION_RX5_1                   0x12
    #define ALP_COMPENSATION_RX6_0                   0xC4
    #define ALP_COMPENSATION_RX6_1                   0x12
    #define ALP_COMPENSATION_RX7_0                   0xD3
    #define ALP_COMPENSATION_RX7_1                   0x12
    #define ALP_COMPENSATION_RX8_0                   0xDC
    #define ALP_COMPENSATION_RX8_1                   0x12
    #define ALP_COMPENSATION_RX9_0                   0xEA
    #define ALP_COMPENSATION_RX9_1                   0x12
    #define ALP_COMPENSATION_RX10_0                  0xBB
    #define ALP_COMPENSATION_RX10_1                  0x12
    #define ALP_COMPENSATION_RX11_0                  0xDB
    #define ALP_COMPENSATION_RX11_1                  0x12
    #define ALP_COMPENSATION_RX12_0                  0xC9
    #define ALP_COMPENSATION_RX12_1                  0x12

    /* I2C Slave Address */
    /* Memory Map Position 0x1176 - 0x1177 */
    #define I2C_KEY                                  0xFF
    #define I2C_SLAVE_ADR                            0xFF

    /* Settings Version Numbers */
    /* Memory Map Position 0x1178 - 0x1179 */
    #define MINOR_VERSION                            0x00
    #define MAJOR_VERSION                            0x00

    /* ATI Multipliers / Dividers */
    /* Memory Map Position 0x117A - 0x1195 */
    #define TP_ATI_MULTDIV_L                         0x21
    #define TP_ATI_MULTDIV_H                         0x4D
    #define ALP_ATI_COARSE_RX0_L                     0x02
    #define ALP_ATI_COARSE_RX0_H                     0x65
    #define ALP_ATI_COARSE_RX1_L                     0x02
    #define ALP_ATI_COARSE_RX1_H                     0x65
    #define ALP_ATI_COARSE_RX2_L                     0x02
    #define ALP_ATI_COARSE_RX2_H                     0x65
    #define ALP_ATI_COARSE_RX3_L                     0x02
    #define ALP_ATI_COARSE_RX3_H                     0x65
    #define ALP_ATI_COARSE_RX4_L                     0x02
    #define ALP_ATI_COARSE_RX4_H                     0x65
    #define ALP_ATI_COARSE_RX5_L                     0x02
    #define ALP_ATI_COARSE_RX5_H                     0x65
    #define ALP_ATI_COARSE_RX6_L                     0x02
    #define ALP_ATI_COARSE_RX6_H                     0x65
    #define ALP_ATI_COARSE_RX7_L                     0x02
    #define ALP_ATI_COARSE_RX7_H                     0x65
    #define ALP_ATI_COARSE_RX8_L                     0x04
    #define ALP_ATI_COARSE_RX8_H                     0x59
    #define ALP_ATI_COARSE_RX9_L                     0x04
    #define ALP_ATI_COARSE_RX9_H                     0x5B
    #define ALP_ATI_COARSE_RX10_L                    0x04
    #define ALP_ATI_COARSE_RX10_H                    0x5D
    #define ALP_ATI_COARSE_RX11_L                    0x04
    #define ALP_ATI_COARSE_RX11_H                    0x5B
    #define ALP_ATI_COARSE_RX12_L                    0x04
    #define ALP_ATI_COARSE_RX12_H                    0x5D

    /* ATI Settings */
    /* Memory Map Position 0x1196 - 0x11A1 */
    #define TP_ATI_TARGET_0                          0x2C
    #define TP_ATI_TARGET_1                          0x01
    #define ALP_ATI_TARGET_0                         0x2C
    #define ALP_ATI_TARGET_1                         0x01
    #define ALP_BASE_TARGET_0                        0x32
    #define ALP_BASE_TARGET_1                        0x00
    #define TP_NEG_DELTA_REATI_0                     0x32
    #define TP_NEG_DELTA_REATI_1                     0x00
    #define TP_POS_DELTA_REATI_0                     0xE8
    #define TP_POS_DELTA_REATI_1                     0x03
    #define TP_REF_DRIFT_LIMIT                       0x32
    #define ALP_LTA_DRIFT_LIMIT                      0x14

    /* Sampling Periods and Timing */
    /* Memory Map Position 0x11A2 - 0x11BB */
    #define ACTIVE_MODE_SAMPLING_PERIOD_0            0x0A
    #define ACTIVE_MODE_SAMPLING_PERIOD_1            0x00
    #define IDLE_TOUCH_MODE_SAMPLING_PERIOD_0        0x32
    #define IDLE_TOUCH_MODE_SAMPLING_PERIOD_1        0x00
    #define IDLE_MODE_SAMPLING_PERIOD_0              0x32
    #define IDLE_MODE_SAMPLING_PERIOD_1              0x00
    #define LP1_MODE_SAMPLING_PERIOD_0               0x32
    #define LP1_MODE_SAMPLING_PERIOD_1               0x00
    #define LP2_MODE_SAMPLING_PERIOD_0               0x32
    #define LP2_MODE_SAMPLING_PERIOD_1               0x00
    #define STATIONARY_TOUCH_TIMEOUT_0               0x0A
    #define STATIONARY_TOUCH_TIMEOUT_1               0x00
    #define IDLE_TOUCH_MODE_TIMEOUT_0                0x3C
    #define IDLE_TOUCH_MODE_TIMEOUT_1                0x00
    #define IDLE_MODE_TIMEOUT_0                      0x05
    #define IDLE_MODE_TIMEOUT_1                      0x00
    #define LP1_MODE_TIMEOUT_0                       0x28
    #define LP1_MODE_TIMEOUT_1                       0x00
    #define ACTIVE_MODE_TIMEOUT_0                    0xDC
    #define ACTIVE_MODE_TIMEOUT_1                    0x05
    #define REATI_RETRY_TIME                         0x05
    #define REF_UPDATE_TIME                          0x08
    #define I2C_TIMEOUT_0                            0x64
    #define I2C_TIMEOUT_1                            0x00
    #define SNAP_TIMEOUT                             0x14
    #define OPEN_TIMING                              0x00

    /* System Settings */
    /* Memory Map Position 0x11BC - 0x11C1 */
    #define SYSTEM_CONTROL_0                         0x00
    #define SYSTEM_CONTROL_1                         0x00
    #define CONFIG_SETTINGS_0                        0x1E
    #define CONFIG_SETTINGS_1                        0x06
    #define OTHER_SETTINGS_0                         0xA4
    #define OTHER_SETTINGS_1                         0x00

    /* ALP Settings */
    /* Memory Map Position 0x11C2 - 0x11CB */
    #define ALP_SETUP_0                              0xFF
    #define ALP_SETUP_1                              0xFF
    #define ALP_SETUP_2                              0xFF
    #define ALP_SETUP_3                              0xD3
    #define ALP_TX_ENABLE_0                          0x00
    #define ALP_TX_ENABLE_1                          0x00
    #define ALP_TX_ENABLE_2                          0x00
    #define ALP_TX_ENABLE_3                          0xE0
    #define ALP_TX_ENABLE_4                          0xFF
    #define ALP_TX_ENABLE_5                          0x0F

    /* Thresholds and Debounce Settings */
    /* Memory Map Position 0x11CC - 0x11D3 */
    #define TRACKPAD_TOUCH_SET_THRESHOLD             0x2C
    #define TRACKPAD_TOUCH_CLEAR_THRESHOLD           0x26
    #define ALP_THRESHOLD                            0x08
    #define ALP_AUTOPROX_THRESHOLD                   0x08
    #define ALP_SET_DEBOUNCE                         0x02
    #define ALP_CLEAR_DEBOUNCE                       0x02
    #define SNAP_SET_THRESHOLD                       0x32
    #define SNAP_CLEAR_THRESHOLD                     0x32

    /* ALP Count and LTA Betas */
    /* Memory Map Position 0x11D4 - 0x11D7 */
    #define ALP_COUNT_BETA_LP1                       0x46
    #define ALP_LTA_BETA_LP1                         0x04
    #define ALP_COUNT_BETA_LP2                       0xB4
    #define ALP_LTA_BETA_LP2                         0x64

    /* Hardware Settings */
    /* Memory Map Position 0x11D8 - 0x11E1 */
    #define TP_FRAC                                  0x28
    #define TP_PERIOD1                               0x02
    #define TP_PERIOD2                               0x02
    #define ALP_FRAC                                 0x10
    #define ALP_PERIOD1                              0x07
    #define ALP_PERIOD2                              0x07
    #define TRACKPAD_HARDWARE_SETTINGS_0             0x00
    #define TRACKPAD_HARDWARE_SETTINGS_1             0x44
    #define ALP_HARDWARE_SETTINGS_0                  0x40
    #define ALP_HARDWARE_SETTINGS_1                  0x4B

    /* Trackpad Settings */
    /* Memory Map Position 0x11E2 - 0x11F5 */
    #define TRACKPAD_SETTINGS_0_0                    0x28
    #define TRACKPAD_SETTINGS_0_1                    0x08
    #define TRACKPAD_SETTINGS_1_0                    0x04
    #define TRACKPAD_SETTINGS_1_1                    0x01
    #define X_RESOLUTION_0                           0xD0
    #define X_RESOLUTION_1                           0x07
    #define Y_RESOLUTION_0                           0xE8
    #define Y_RESOLUTION_1                           0x03
    #define XY_DYNAMIC_FILTER_BOTTOM_SPEED_0         0x06
    #define XY_DYNAMIC_FILTER_BOTTOM_SPEED_1         0x00
    #define XY_DYNAMIC_FILTER_TOP_SPEED_0            0x7C
    #define XY_DYNAMIC_FILTER_TOP_SPEED_1            0x00
    #define XY_DYNAMIC_FILTER_BOTTOM_BETA            0x07
    #define XY_DYNAMIC_FILTER_STATIC_FILTER_BETA     0x80
    #define STATIONARY_TOUCH_MOV_THRESHOLD           0x14
    #define FINGER_SPLIT_FACTOR                      0x03
    #define X_TRIM_VALUE                             0x14
    #define Y_TRIM_VALUE                             0x14
    #define JITTER_FILTER_DELTA                      0x05
    #define FINGER_CONFIDENCE_THRESHOLD              0x14

    /* Gesture Settings */
    /* Memory Map Position 0x11F6 - 0x1217 */
    #define GESTURE_ENABLE_0                         0x1F
    #define GESTURE_ENABLE_1                         0xFF
    #define GESTURE_ENABLE_2F_0                      0xFF
    #define GESTURE_ENABLE_2F_1                      0x00
    #define TAP_TOUCH_TIME_0                         0x96
    #define TAP_TOUCH_TIME_1                         0x00
    #define TAP_WAIT_TIME_0                          0x96
    #define TAP_WAIT_TIME_1                          0x00
    #define TAP_DISTANCE_0                           0x32
    #define TAP_DISTANCE_1                           0x00
    #define HOLD_TIME_0                              0x2C
    #define HOLD_TIME_1                              0x01
    #define SWIPE_TIME_0                             0x96
    #define SWIPE_TIME_1                             0x00
    #define SWIPE_X_DISTANCE_0                       0x2C
    #define SWIPE_X_DISTANCE_1                       0x01
    #define SWIPE_Y_DISTANCE_0                       0x96
    #define SWIPE_Y_DISTANCE_1                       0x00
    #define SWIPE_X_CONS_DIST_0                      0x64
    #define SWIPE_X_CONS_DIST_1                      0x00
    #define SWIPE_Y_CONS_DIST_0                      0x32
    #define SWIPE_Y_CONS_DIST_1                      0x00
    #define SWIPE_ANGLE                              0x25
    #define SCROLL_ANGLE                             0x25
    #define ZOOM_INIT_DIST_0                         0x64
    #define ZOOM_INIT_DIST_1                         0x00
    #define ZOOM_CONSECUTIVE_DIST_0                  0x0A
    #define ZOOM_CONSECUTIVE_DIST_1                  0x00
    #define SCROLL_INIT_DIST_0                       0x32
    #define SCROLL_INIT_DIST_1                       0x00
    #define SCROLL_CONSECUTIVE_DIST_0                0x05
    #define SCROLL_CONSECUTIVE_DIST_1                0x00
    #define PALM_GESTURE_THRESHOLD_0                 0x64
    #define PALM_GESTURE_THRESHOLD_1                 0x00

    /* Rx/Tx Mapping */
    /* Memory Map Position 0x1218 - 0x1245 */
    #define RX_TX_MAP_0                              0x19
    #define RX_TX_MAP_1                              0x0C
    #define RX_TX_MAP_2                              0x18
    #define RX_TX_MAP_3                              0x0B
    #define RX_TX_MAP_4                              0x17
    #define RX_TX_MAP_5                              0x0A
    #define RX_TX_MAP_6                              0x16
    #define RX_TX_MAP_7                              0x09
    #define RX_TX_MAP_8                              0x24
    #define RX_TX_MAP_9                              0x23
    #define RX_TX_MAP_10                             0x22
    #define RX_TX_MAP_11                             0x21
    #define RX_TX_MAP_12                             0x13
    #define RX_TX_MAP_13                             0x06
    #define RX_TX_MAP_14                             0x12
    #define RX_TX_MAP_15                             0x05
    #define RX_TX_MAP_16                             0x11
    #define RX_TX_MAP_17                             0x04
    #define RX_TX_MAP_18                             0x10
    #define RX_TX_MAP_19                             0x03
    #define RX_TX_MAP_20                             0x0F
    #define RX_TX_MAP_21                             0x02
    #define RX_TX_MAP_22                             0x0E
    #define RX_TX_MAP_23                             0x01
    #define RX_TX_MAP_24                             0x0D
    #define RX_TX_MAP_25                             0x00
    #define RX_TX_MAP_26                             0x1F
    #define RX_TX_MAP_27                             0x1E
    #define RX_TX_MAP_28                             0x1D
    #define RX_TX_MAP_29                             0x20
    #define RX_TX_MAP_30                             0x21
    #define RX_TX_MAP_31                             0x22
    #define RX_TX_MAP_32                             0x23
    #define RX_TX_MAP_33                             0x24
    #define RX_TX_MAP_34                             0x25
    #define RX_TX_MAP_35                             0x26
    #define RX_TX_MAP_36                             0x27
    #define RX_TX_MAP_37                             0x28
    #define RX_TX_MAP_38                             0x29
    #define RX_TX_MAP_39                             0x2A
    #define RX_TX_MAP_40                             0x2B
    #define RX_TX_MAP_41                             0x2D
    #define RX_TX_MAP_42                             0x00
    #define RX_TX_MAP_43                             0x00
    #define RX_TX_MAP_44                             0x00
    #define RX_TX_OPEN                               0x00

    /* TP Channel Disables */
    /* Memory Map Position 0x1246 - 0x129D */
    #define TPCHANNELDISABLE_0                       0x00
    #define TPCHANNELDISABLE_1                       0x00
    #define TPCHANNELDISABLE_2                       0x00
    #define TPCHANNELDISABLE_3                       0x00
    #define TPCHANNELDISABLE_4                       0x00
    #define TPCHANNELDISABLE_5                       0x00
    #define TPCHANNELDISABLE_6                       0x00
    #define TPCHANNELDISABLE_7                       0x00
    #define TPCHANNELDISABLE_8                       0x00
    #define TPCHANNELDISABLE_9                       0x00
    #define TPCHANNELDISABLE_10                      0x00
    #define TPCHANNELDISABLE_11                      0x00
    #define TPCHANNELDISABLE_12                      0x00
    #define TPCHANNELDISABLE_13                      0x00
    #define TPCHANNELDISABLE_14                      0x00
    #define TPCHANNELDISABLE_15                      0x00
    #define TPCHANNELDISABLE_16                      0x00
    #define TPCHANNELDISABLE_17                      0x00
    #define TPCHANNELDISABLE_18                      0x00
    #define TPCHANNELDISABLE_19                      0x00
    #define TPCHANNELDISABLE_20                      0x00
    #define TPCHANNELDISABLE_21                      0x00
    #define TPCHANNELDISABLE_22                      0x00
    #define TPCHANNELDISABLE_23                      0x00
    #define TPCHANNELDISABLE_24                      0x00
    #define TPCHANNELDISABLE_25                      0x00
    #define TPCHANNELDISABLE_26                      0x00
    #define TPCHANNELDISABLE_27                      0x00
    #define TPCHANNELDISABLE_28                      0x00
    #define TPCHANNELDISABLE_29                      0x00
    #define TPCHANNELDISABLE_30                      0x00
    #define TPCHANNELDISABLE_31                      0x00
    #define TPCHANNELDISABLE_32                      0x00
    #define TPCHANNELDISABLE_33                      0x00
    #define TPCHANNELDISABLE_34                      0x00
    #define TPCHANNELDISABLE_35                      0x00
    #define TPCHANNELDISABLE_36                      0x00
    #define TPCHANNELDISABLE_37                      0x00
    #define TPCHANNELDISABLE_38                      0x00
    #define TPCHANNELDISABLE_39                      0x00
    #define TPCHANNELDISABLE_40                      0x00
    #define TPCHANNELDISABLE_41                      0x00
    #define TPCHANNELDISABLE_42                      0x00
    #define TPCHANNELDISABLE_43                      0x00
    #define TPCHANNELDISABLE_44                      0x00
    #define TPCHANNELDISABLE_45                      0x00
    #define TPCHANNELDISABLE_46                      0x00
    #define TPCHANNELDISABLE_47                      0x00
    #define TPCHANNELDISABLE_48                      0x00
    #define TPCHANNELDISABLE_49                      0x00
    #define TPCHANNELDISABLE_50                      0x00
    #define TPCHANNELDISABLE_51                      0x00
    #define TPCHANNELDISABLE_52                      0x00
    #define TPCHANNELDISABLE_53                      0x00
    #define TPCHANNELDISABLE_54                      0x00
    #define TPCHANNELDISABLE_55                      0x00
    #define TPCHANNELDISABLE_56                      0x00
    #define TPCHANNELDISABLE_57                      0x00
    #define TPCHANNELDISABLE_58                      0x00
    #define TPCHANNELDISABLE_59                      0x00
    #define TPCHANNELDISABLE_60                      0x00
    #define TPCHANNELDISABLE_61                      0x00
    #define TPCHANNELDISABLE_62                      0x00
    #define TPCHANNELDISABLE_63                      0x00
    #define TPCHANNELDISABLE_64                      0x00
    #define TPCHANNELDISABLE_65                      0x00
    #define TPCHANNELDISABLE_66                      0x00
    #define TPCHANNELDISABLE_67                      0x00
    #define TPCHANNELDISABLE_68                      0x00
    #define TPCHANNELDISABLE_69                      0x00
    #define TPCHANNELDISABLE_70                      0x00
    #define TPCHANNELDISABLE_71                      0x00
    #define TPCHANNELDISABLE_72                      0x00
    #define TPCHANNELDISABLE_73                      0x00
    #define TPCHANNELDISABLE_74                      0x00
    #define TPCHANNELDISABLE_75                      0x00
    #define TPCHANNELDISABLE_76                      0x00
    #define TPCHANNELDISABLE_77                      0x00
    #define TPCHANNELDISABLE_78                      0x00
    #define TPCHANNELDISABLE_79                      0x00
    #define TPCHANNELDISABLE_80                      0x00
    #define TPCHANNELDISABLE_81                      0x00
    #define TPCHANNELDISABLE_82                      0x00
    #define TPCHANNELDISABLE_83                      0x00
    #define TPCHANNELDISABLE_84                      0x00
    #define TPCHANNELDISABLE_85                      0x00
    #define TPCHANNELDISABLE_86                      0x00
    #define TPCHANNELDISABLE_87                      0x00

    /* TP Snap Enable */
    /* Memory Map Position 0x129E - 0x12F5 */
    #define SNAPCHANNELENABLE_0                      0x00
    #define SNAPCHANNELENABLE_1                      0x00
    #define SNAPCHANNELENABLE_2                      0x00
    #define SNAPCHANNELENABLE_3                      0x00
    #define SNAPCHANNELENABLE_4                      0x00
    #define SNAPCHANNELENABLE_5                      0x00
    #define SNAPCHANNELENABLE_6                      0x00
    #define SNAPCHANNELENABLE_7                      0x00
    #define SNAPCHANNELENABLE_8                      0x00
    #define SNAPCHANNELENABLE_9                      0x00
    #define SNAPCHANNELENABLE_10                     0x00
    #define SNAPCHANNELENABLE_11                     0x00
    #define SNAPCHANNELENABLE_12                     0x00
    #define SNAPCHANNELENABLE_13                     0x00
    #define SNAPCHANNELENABLE_14                     0x00
    #define SNAPCHANNELENABLE_15                     0x00
    #define SNAPCHANNELENABLE_16                     0x00
    #define SNAPCHANNELENABLE_17                     0x00
    #define SNAPCHANNELENABLE_18                     0x00
    #define SNAPCHANNELENABLE_19                     0x00
    #define SNAPCHANNELENABLE_20                     0x00
    #define SNAPCHANNELENABLE_21                     0x00
    #define SNAPCHANNELENABLE_22                     0x00
    #define SNAPCHANNELENABLE_23                     0x00
    #define SNAPCHANNELENABLE_24                     0x00
    #define SNAPCHANNELENABLE_25                     0x00
    #define SNAPCHANNELENABLE_26                     0x00
    #define SNAPCHANNELENABLE_27                     0x00
    #define SNAPCHANNELENABLE_28                     0x00
    #define SNAPCHANNELENABLE_29                     0x00
    #define SNAPCHANNELENABLE_30                     0x00
    #define SNAPCHANNELENABLE_31                     0x00
    #define SNAPCHANNELENABLE_32                     0x00
    #define SNAPCHANNELENABLE_33                     0x00
    #define SNAPCHANNELENABLE_34                     0x00
    #define SNAPCHANNELENABLE_35                     0x00
    #define SNAPCHANNELENABLE_36                     0x00
    #define SNAPCHANNELENABLE_37                     0x00
    #define SNAPCHANNELENABLE_38                     0x00
    #define SNAPCHANNELENABLE_39                     0x00
    #define SNAPCHANNELENABLE_40                     0x00
    #define SNAPCHANNELENABLE_41                     0x00
    #define SNAPCHANNELENABLE_42                     0x00
    #define SNAPCHANNELENABLE_43                     0x00
    #define SNAPCHANNELENABLE_44                     0x00
    #define SNAPCHANNELENABLE_45                     0x00
    #define SNAPCHANNELENABLE_46                     0x00
    #define SNAPCHANNELENABLE_47                     0x00
    #define SNAPCHANNELENABLE_48                     0x00
    #define SNAPCHANNELENABLE_49                     0x00
    #define SNAPCHANNELENABLE_50                     0x00
    #define SNAPCHANNELENABLE_51                     0x00
    #define SNAPCHANNELENABLE_52                     0x00
    #define SNAPCHANNELENABLE_53                     0x00
    #define SNAPCHANNELENABLE_54                     0x00
    #define SNAPCHANNELENABLE_55                     0x00
    #define SNAPCHANNELENABLE_56                     0x00
    #define SNAPCHANNELENABLE_57                     0x00
    #define SNAPCHANNELENABLE_58                     0x00
    #define SNAPCHANNELENABLE_59                     0x00
    #define SNAPCHANNELENABLE_60                     0x00
    #define SNAPCHANNELENABLE_61                     0x00
    #define SNAPCHANNELENABLE_62                     0x00
    #define SNAPCHANNELENABLE_63                     0x00
    #define SNAPCHANNELENABLE_64                     0x00
    #define SNAPCHANNELENABLE_65                     0x00
    #define SNAPCHANNELENABLE_66                     0x00
    #define SNAPCHANNELENABLE_67                     0x00
    #define SNAPCHANNELENABLE_68                     0x00
    #define SNAPCHANNELENABLE_69                     0x00
    #define SNAPCHANNELENABLE_70                     0x00
    #define SNAPCHANNELENABLE_71                     0x00
    #define SNAPCHANNELENABLE_72                     0x00
    #define SNAPCHANNELENABLE_73                     0x00
    #define SNAPCHANNELENABLE_74                     0x00
    #define SNAPCHANNELENABLE_75                     0x00
    #define SNAPCHANNELENABLE_76                     0x00
    #define SNAPCHANNELENABLE_77                     0x00
    #define SNAPCHANNELENABLE_78                     0x00
    #define SNAPCHANNELENABLE_79                     0x00
    #define SNAPCHANNELENABLE_80                     0x00
    #define SNAPCHANNELENABLE_81                     0x00
    #define SNAPCHANNELENABLE_82                     0x00
    #define SNAPCHANNELENABLE_83                     0x00
    #define SNAPCHANNELENABLE_84                     0x00
    #define SNAPCHANNELENABLE_85                     0x00
    #define SNAPCHANNELENABLE_86                     0x00
    #define SNAPCHANNELENABLE_87                     0x00

    /* Individual Touch Threshold Adjustments */
    /* Memory Map Position 0x12F6 - 0x14EF */
    #define TOUCHTHRADJUST_0                         0x00
    #define TOUCHTHRADJUST_1                         0x00
    #define TOUCHTHRADJUST_2                         0x00
    #define TOUCHTHRADJUST_3                         0x00
    #define TOUCHTHRADJUST_4                         0x00
    #define TOUCHTHRADJUST_5                         0x00
    #define TOUCHTHRADJUST_6                         0x00
    #define TOUCHTHRADJUST_7                         0x00
    #define TOUCHTHRADJUST_8                         0x00
    #define TOUCHTHRADJUST_9                         0x00
    #define TOUCHTHRADJUST_10                        0x00
    #define TOUCHTHRADJUST_11                        0x00
    #define TOUCHTHRADJUST_12                        0x00
    #define TOUCHTHRADJUST_13                        0x00
    #define TOUCHTHRADJUST_14                        0x00
    #define TOUCHTHRADJUST_15                        0x00
    #define TOUCHTHRADJUST_16                        0x00
    #define TOUCHTHRADJUST_17                        0x00
    #define TOUCHTHRADJUST_18                        0x00
    #define TOUCHTHRADJUST_19                        0x00
    #define TOUCHTHRADJUST_20                        0x00
    #define TOUCHTHRADJUST_21                        0x00
    #define TOUCHTHRADJUST_22                        0x00
    #define TOUCHTHRADJUST_23                        0x00
    #define TOUCHTHRADJUST_24                        0x00
    #define TOUCHTHRADJUST_25                        0x00
    #define TOUCHTHRADJUST_26                        0x00
    #define TOUCHTHRADJUST_27                        0x00
    #define TOUCHTHRADJUST_28                        0x00
    #define TOUCHTHRADJUST_29                        0x00
    #define TOUCHTHRADJUST_30                        0x00
    #define TOUCHTHRADJUST_31                        0x00
    #define TOUCHTHRADJUST_32                        0x00
    #define TOUCHTHRADJUST_33                        0x00
    #define TOUCHTHRADJUST_34                        0x00
    #define TOUCHTHRADJUST_35                        0x00
    #define TOUCHTHRADJUST_36                        0x00
    #define TOUCHTHRADJUST_37                        0x00
    #define TOUCHTHRADJUST_38                        0x00
    #define TOUCHTHRADJUST_39                        0x00
    #define TOUCHTHRADJUST_40                        0x00
    #define TOUCHTHRADJUST_41                        0x00
    #define TOUCHTHRADJUST_42                        0x00
    #define TOUCHTHRADJUST_43                        0x00
    #define TOUCHTHRADJUST_44                        0x00
    #define TOUCHTHRADJUST_45                        0x00
    #define TOUCHTHRADJUST_46                        0x00
    #define TOUCHTHRADJUST_47                        0x00
    #define TOUCHTHRADJUST_48                        0x00
    #define TOUCHTHRADJUST_49                        0x00
    #define TOUCHTHRADJUST_50                        0x00
    #define TOUCHTHRADJUST_51                        0x00
    #define TOUCHTHRADJUST_52                        0x00
    #define TOUCHTHRADJUST_53                        0x00
    #define TOUCHTHRADJUST_54                        0x00
    #define TOUCHTHRADJUST_55                        0x00
    #define TOUCHTHRADJUST_56                        0x00
    #define TOUCHTHRADJUST_57                        0x00
    #define TOUCHTHRADJUST_58                        0x00
    #define TOUCHTHRADJUST_59                        0x00
    #define TOUCHTHRADJUST_60                        0x00
    #define TOUCHTHRADJUST_61                        0x00
    #define TOUCHTHRADJUST_62                        0x00
    #define TOUCHTHRADJUST_63                        0x00
    #define TOUCHTHRADJUST_64                        0x00
    #define TOUCHTHRADJUST_65                        0x00
    #define TOUCHTHRADJUST_66                        0x00
    #define TOUCHTHRADJUST_67                        0x00
    #define TOUCHTHRADJUST_68                        0x00
    #define TOUCHTHRADJUST_69                        0x00
    #define TOUCHTHRADJUST_70                        0x00
    #define TOUCHTHRADJUST_71                        0x00
    #define TOUCHTHRADJUST_72                        0x00
    #define TOUCHTHRADJUST_73                        0x00
    #define TOUCHTHRADJUST_74                        0x00
    #define TOUCHTHRADJUST_75                        0x00
    #define TOUCHTHRADJUST_76                        0x00
    #define TOUCHTHRADJUST_77                        0x00
    #define TOUCHTHRADJUST_78                        0x00
    #define TOUCHTHRADJUST_79                        0x00
    #define TOUCHTHRADJUST_80                        0x00
    #define TOUCHTHRADJUST_81                        0x00
    #define TOUCHTHRADJUST_82                        0x00
    #define TOUCHTHRADJUST_83                        0x00
    #define TOUCHTHRADJUST_84                        0x00
    #define TOUCHTHRADJUST_85                        0x00
    #define TOUCHTHRADJUST_86                        0x00
    #define TOUCHTHRADJUST_87                        0x00
    #define TOUCHTHRADJUST_88                        0x00
    #define TOUCHTHRADJUST_89                        0x00
    #define TOUCHTHRADJUST_90                        0x00
    #define TOUCHTHRADJUST_91                        0x00
    #define TOUCHTHRADJUST_92                        0x00
    #define TOUCHTHRADJUST_93                        0x00
    #define TOUCHTHRADJUST_94                        0x00
    #define TOUCHTHRADJUST_95                        0x00
    #define TOUCHTHRADJUST_96                        0x00
    #define TOUCHTHRADJUST_97                        0x00
    #define TOUCHTHRADJUST_98                        0x00
    #define TOUCHTHRADJUST_99                        0x00
    #define TOUCHTHRADJUST_100                       0x00
    #define TOUCHTHRADJUST_101                       0x00
    #define TOUCHTHRADJUST_102                       0x00
    #define TOUCHTHRADJUST_103                       0x00
    #define TOUCHTHRADJUST_104                       0x00
    #define TOUCHTHRADJUST_105                       0x00
    #define TOUCHTHRADJUST_106                       0x00
    #define TOUCHTHRADJUST_107                       0x00
    #define TOUCHTHRADJUST_108                       0x00
    #define TOUCHTHRADJUST_109                       0x00
    #define TOUCHTHRADJUST_110                       0x00
    #define TOUCHTHRADJUST_111                       0x00
    #define TOUCHTHRADJUST_112                       0x00
    #define TOUCHTHRADJUST_113                       0x00
    #define TOUCHTHRADJUST_114                       0x00
    #define TOUCHTHRADJUST_115                       0x00
    #define TOUCHTHRADJUST_116                       0x00
    #define TOUCHTHRADJUST_117                       0x00
    #define TOUCHTHRADJUST_118                       0x00
    #define TOUCHTHRADJUST_119                       0x00
    #define TOUCHTHRADJUST_120                       0x00
    #define TOUCHTHRADJUST_121                       0x00
    #define TOUCHTHRADJUST_122                       0x00
    #define TOUCHTHRADJUST_123                       0x00
    #define TOUCHTHRADJUST_124                       0x00
    #define TOUCHTHRADJUST_125                       0x00
    #define TOUCHTHRADJUST_126                       0x00
    #define TOUCHTHRADJUST_127                       0x00
    #define TOUCHTHRADJUST_128                       0x00
    #define TOUCHTHRADJUST_129                       0x00
    #define TOUCHTHRADJUST_130                       0x00
    #define TOUCHTHRADJUST_131                       0x00
    #define TOUCHTHRADJUST_132                       0x00
    #define TOUCHTHRADJUST_133                       0x00
    #define TOUCHTHRADJUST_134                       0x00
    #define TOUCHTHRADJUST_135                       0x00
    #define TOUCHTHRADJUST_136                       0x00
    #define TOUCHTHRADJUST_137                       0x00
    #define TOUCHTHRADJUST_138                       0x00
    #define TOUCHTHRADJUST_139                       0x00
    #define TOUCHTHRADJUST_140                       0x00
    #define TOUCHTHRADJUST_141                       0x00
    #define TOUCHTHRADJUST_142                       0x00
    #define TOUCHTHRADJUST_143                       0x00
    #define TOUCHTHRADJUST_144                       0x00
    #define TOUCHTHRADJUST_145                       0x00
    #define TOUCHTHRADJUST_146                       0x00
    #define TOUCHTHRADJUST_147                       0x00
    #define TOUCHTHRADJUST_148                       0x00
    #define TOUCHTHRADJUST_149                       0x00
    #define TOUCHTHRADJUST_150                       0x00
    #define TOUCHTHRADJUST_151                       0x00
    #define TOUCHTHRADJUST_152                       0x00
    #define TOUCHTHRADJUST_153                       0x00
    #define TOUCHTHRADJUST_154                       0x00
    #define TOUCHTHRADJUST_155                       0x00
    #define TOUCHTHRADJUST_156                       0x00
    #define TOUCHTHRADJUST_157                       0x00
    #define TOUCHTHRADJUST_158                       0x00
    #define TOUCHTHRADJUST_159                       0x00
    #define TOUCHTHRADJUST_160                       0x00
    #define TOUCHTHRADJUST_161                       0x00
    #define TOUCHTHRADJUST_162                       0x00
    #define TOUCHTHRADJUST_163                       0x00
    #define TOUCHTHRADJUST_164                       0x00
    #define TOUCHTHRADJUST_165                       0x00
    #define TOUCHTHRADJUST_166                       0x00
    #define TOUCHTHRADJUST_167                       0x00
    #define TOUCHTHRADJUST_168                       0x00
    #define TOUCHTHRADJUST_169                       0x00
    #define TOUCHTHRADJUST_170                       0x00
    #define TOUCHTHRADJUST_171                       0x00
    #define TOUCHTHRADJUST_172                       0x00
    #define TOUCHTHRADJUST_173                       0x00
    #define TOUCHTHRADJUST_174                       0x00
    #define TOUCHTHRADJUST_175                       0x00
    #define TOUCHTHRADJUST_176                       0x00
    #define TOUCHTHRADJUST_177                       0x00
    #define TOUCHTHRADJUST_178                       0x00
    #define TOUCHTHRADJUST_179                       0x00
    #define TOUCHTHRADJUST_180                       0x00
    #define TOUCHTHRADJUST_181                       0x00
    #define TOUCHTHRADJUST_182                       0x00
    #define TOUCHTHRADJUST_183                       0x00
    #define TOUCHTHRADJUST_184                       0x00
    #define TOUCHTHRADJUST_185                       0x00
    #define TOUCHTHRADJUST_186                       0x00
    #define TOUCHTHRADJUST_187                       0x00
    #define TOUCHTHRADJUST_188                       0x00
    #define TOUCHTHRADJUST_189                       0x00
    #define TOUCHTHRADJUST_190                       0x00
    #define TOUCHTHRADJUST_191                       0x00
    #define TOUCHTHRADJUST_192                       0x00
    #define TOUCHTHRADJUST_193                       0x00
    #define TOUCHTHRADJUST_194                       0x00
    #define TOUCHTHRADJUST_195                       0x00
    #define TOUCHTHRADJUST_196                       0x00
    #define TOUCHTHRADJUST_197                       0x00
    #define TOUCHTHRADJUST_198                       0x00
    #define TOUCHTHRADJUST_199                       0x00
    #define TOUCHTHRADJUST_200                       0x00
    #define TOUCHTHRADJUST_201                       0x00
    #define TOUCHTHRADJUST_202                       0x00
    #define TOUCHTHRADJUST_203                       0x00
    #define TOUCHTHRADJUST_204                       0x00
    #define TOUCHTHRADJUST_205                       0x00
    #define TOUCHTHRADJUST_206                       0x00
    #define TOUCHTHRADJUST_207                       0x00
    #define TOUCHTHRADJUST_208                       0x00
    #define TOUCHTHRADJUST_209                       0x00
    #define TOUCHTHRADJUST_210                       0x00
    #define TOUCHTHRADJUST_211                       0x00
    #define TOUCHTHRADJUST_212                       0x00
    #define TOUCHTHRADJUST_213                       0x00
    #define TOUCHTHRADJUST_214                       0x00
    #define TOUCHTHRADJUST_215                       0x00
    #define TOUCHTHRADJUST_216                       0x00
    #define TOUCHTHRADJUST_217                       0x00
    #define TOUCHTHRADJUST_218                       0x00
    #define TOUCHTHRADJUST_219                       0x00
    #define TOUCHTHRADJUST_220                       0x00
    #define TOUCHTHRADJUST_221                       0x00
    #define TOUCHTHRADJUST_222                       0x00
    #define TOUCHTHRADJUST_223                       0x00
    #define TOUCHTHRADJUST_224                       0x00
    #define TOUCHTHRADJUST_225                       0x00
    #define TOUCHTHRADJUST_226                       0x00
    #define TOUCHTHRADJUST_227                       0x00
    #define TOUCHTHRADJUST_228                       0x00
    #define TOUCHTHRADJUST_229                       0x00
    #define TOUCHTHRADJUST_230                       0x00
    #define TOUCHTHRADJUST_231                       0x00
    #define TOUCHTHRADJUST_232                       0x00
    #define TOUCHTHRADJUST_233                       0x00
    #define TOUCHTHRADJUST_234                       0x00
    #define TOUCHTHRADJUST_235                       0x00
    #define TOUCHTHRADJUST_236                       0x00
    #define TOUCHTHRADJUST_237                       0x00
    #define TOUCHTHRADJUST_238                       0x00
    #define TOUCHTHRADJUST_239                       0x00
    #define TOUCHTHRADJUST_240                       0x00
    #define TOUCHTHRADJUST_241                       0x00
    #define TOUCHTHRADJUST_242                       0x00
    #define TOUCHTHRADJUST_243                       0x00
    #define TOUCHTHRADJUST_244                       0x00
    #define TOUCHTHRADJUST_245                       0x00
    #define TOUCHTHRADJUST_246                       0x00
    #define TOUCHTHRADJUST_247                       0x00
    #define TOUCHTHRADJUST_248                       0x00
    #define TOUCHTHRADJUST_249                       0x00
    #define TOUCHTHRADJUST_250                       0x00
    #define TOUCHTHRADJUST_251                       0x00
    #define TOUCHTHRADJUST_252                       0x00
    #define TOUCHTHRADJUST_253                       0x00
    #define TOUCHTHRADJUST_254                       0x00
    #define TOUCHTHRADJUST_255                       0x00
    #define TOUCHTHRADJUST_256                       0x00
    #define TOUCHTHRADJUST_257                       0x00
    #define TOUCHTHRADJUST_258                       0x00
    #define TOUCHTHRADJUST_259                       0x00
    #define TOUCHTHRADJUST_260                       0x00
    #define TOUCHTHRADJUST_261                       0x00
    #define TOUCHTHRADJUST_262                       0x00
    #define TOUCHTHRADJUST_263                       0x00
    #define TOUCHTHRADJUST_264                       0x00
    #define TOUCHTHRADJUST_265                       0x00
    #define TOUCHTHRADJUST_266                       0x00
    #define TOUCHTHRADJUST_267                       0x00
    #define TOUCHTHRADJUST_268                       0x00
    #define TOUCHTHRADJUST_269                       0x00
    #define TOUCHTHRADJUST_270                       0x00
    #define TOUCHTHRADJUST_271                       0x00
    #define TOUCHTHRADJUST_272                       0x00
    #define TOUCHTHRADJUST_273                       0x00
    #define TOUCHTHRADJUST_274                       0x00
    #define TOUCHTHRADJUST_275                       0x00
    #define TOUCHTHRADJUST_276                       0x00
    #define TOUCHTHRADJUST_277                       0x00
    #define TOUCHTHRADJUST_278                       0x00
    #define TOUCHTHRADJUST_279                       0x00
    #define TOUCHTHRADJUST_280                       0x00
    #define TOUCHTHRADJUST_281                       0x00
    #define TOUCHTHRADJUST_282                       0x00
    #define TOUCHTHRADJUST_283                       0x00
    #define TOUCHTHRADJUST_284                       0x00
    #define TOUCHTHRADJUST_285                       0x00
    #define TOUCHTHRADJUST_286                       0x00
    #define TOUCHTHRADJUST_287                       0x00
    #define TOUCHTHRADJUST_288                       0x00
    #define TOUCHTHRADJUST_289                       0x00
    #define TOUCHTHRADJUST_290                       0x00
    #define TOUCHTHRADJUST_291                       0x00
    #define TOUCHTHRADJUST_292                       0x00
    #define TOUCHTHRADJUST_293                       0x00
    #define TOUCHTHRADJUST_294                       0x00
    #define TOUCHTHRADJUST_295                       0x00
    #define TOUCHTHRADJUST_296                       0x00
    #define TOUCHTHRADJUST_297                       0x00
    #define TOUCHTHRADJUST_298                       0x00
    #define TOUCHTHRADJUST_299                       0x00
    #define TOUCHTHRADJUST_300                       0x00
    #define TOUCHTHRADJUST_301                       0x00
    #define TOUCHTHRADJUST_302                       0x00
    #define TOUCHTHRADJUST_303                       0x00
    #define TOUCHTHRADJUST_304                       0x00
    #define TOUCHTHRADJUST_305                       0x00
    #define TOUCHTHRADJUST_306                       0x00
    #define TOUCHTHRADJUST_307                       0x00
    #define TOUCHTHRADJUST_308                       0x00
    #define TOUCHTHRADJUST_309                       0x00
    #define TOUCHTHRADJUST_310                       0x00
    #define TOUCHTHRADJUST_311                       0x00
    #define TOUCHTHRADJUST_312                       0x00
    #define TOUCHTHRADJUST_313                       0x00
    #define TOUCHTHRADJUST_314                       0x00
    #define TOUCHTHRADJUST_315                       0x00
    #define TOUCHTHRADJUST_316                       0x00
    #define TOUCHTHRADJUST_317                       0x00
    #define TOUCHTHRADJUST_318                       0x00
    #define TOUCHTHRADJUST_319                       0x00
    #define TOUCHTHRADJUST_320                       0x00
    #define TOUCHTHRADJUST_321                       0x00
    #define TOUCHTHRADJUST_322                       0x00
    #define TOUCHTHRADJUST_323                       0x00
    #define TOUCHTHRADJUST_324                       0x00
    #define TOUCHTHRADJUST_325                       0x00
    #define TOUCHTHRADJUST_326                       0x00
    #define TOUCHTHRADJUST_327                       0x00
    #define TOUCHTHRADJUST_328                       0x00
    #define TOUCHTHRADJUST_329                       0x00
    #define TOUCHTHRADJUST_330                       0x00
    #define TOUCHTHRADJUST_331                       0x00
    #define TOUCHTHRADJUST_332                       0x00
    #define TOUCHTHRADJUST_333                       0x00
    #define TOUCHTHRADJUST_334                       0x00
    #define TOUCHTHRADJUST_335                       0x00
    #define TOUCHTHRADJUST_336                       0x00
    #define TOUCHTHRADJUST_337                       0x00
    #define TOUCHTHRADJUST_338                       0x00
    #define TOUCHTHRADJUST_339                       0x00
    #define TOUCHTHRADJUST_340                       0x00
    #define TOUCHTHRADJUST_341                       0x00
    #define TOUCHTHRADJUST_342                       0x00
    #define TOUCHTHRADJUST_343                       0x00
    #define TOUCHTHRADJUST_344                       0x00
    #define TOUCHTHRADJUST_345                       0x00
    #define TOUCHTHRADJUST_346                       0x00
    #define TOUCHTHRADJUST_347                       0x00
    #define TOUCHTHRADJUST_348                       0x00
    #define TOUCHTHRADJUST_349                       0x00
    #define TOUCHTHRADJUST_350                       0x00
    #define TOUCHTHRADJUST_351                       0x00
    #define TOUCHTHRADJUST_352                       0x00
    #define TOUCHTHRADJUST_353                       0x00
    #define TOUCHTHRADJUST_354                       0x00
    #define TOUCHTHRADJUST_355                       0x00
    #define TOUCHTHRADJUST_356                       0x00
    #define TOUCHTHRADJUST_357                       0x00
    #define TOUCHTHRADJUST_358                       0x00
    #define TOUCHTHRADJUST_359                       0x00
    #define TOUCHTHRADJUST_360                       0x00
    #define TOUCHTHRADJUST_361                       0x00
    #define TOUCHTHRADJUST_362                       0x00
    #define TOUCHTHRADJUST_363                       0x00
    #define TOUCHTHRADJUST_364                       0x00
    #define TOUCHTHRADJUST_365                       0x00
    #define TOUCHTHRADJUST_366                       0x00
    #define TOUCHTHRADJUST_367                       0x00
    #define TOUCHTHRADJUST_368                       0x00
    #define TOUCHTHRADJUST_369                       0x00
    #define TOUCHTHRADJUST_370                       0x00
    #define TOUCHTHRADJUST_371                       0x00
    #define TOUCHTHRADJUST_372                       0x00
    #define TOUCHTHRADJUST_373                       0x00
    #define TOUCHTHRADJUST_374                       0x00
    #define TOUCHTHRADJUST_375                       0x00
    #define TOUCHTHRADJUST_376                       0x00
    #define TOUCHTHRADJUST_377                       0x00
    #define TOUCHTHRADJUST_378                       0x00
    #define TOUCHTHRADJUST_379                       0x00
    #define TOUCHTHRADJUST_380                       0x00
    #define TOUCHTHRADJUST_381                       0x00
    #define TOUCHTHRADJUST_382                       0x00
    #define TOUCHTHRADJUST_383                       0x00
    #define TOUCHTHRADJUST_384                       0x00
    #define TOUCHTHRADJUST_385                       0x00
    #define TOUCHTHRADJUST_386                       0x00
    #define TOUCHTHRADJUST_387                       0x00
    #define TOUCHTHRADJUST_388                       0x00
    #define TOUCHTHRADJUST_389                       0x00
    #define TOUCHTHRADJUST_390                       0x00
    #define TOUCHTHRADJUST_391                       0x00
    #define TOUCHTHRADJUST_392                       0x00
    #define TOUCHTHRADJUST_393                       0x00
    #define TOUCHTHRADJUST_394                       0x00
    #define TOUCHTHRADJUST_395                       0x00
    #define TOUCHTHRADJUST_396                       0x00
    #define TOUCHTHRADJUST_397                       0x00
    #define TOUCHTHRADJUST_398                       0x00
    #define TOUCHTHRADJUST_399                       0x00
    #define TOUCHTHRADJUST_400                       0x00
    #define TOUCHTHRADJUST_401                       0x00
    #define TOUCHTHRADJUST_402                       0x00
    #define TOUCHTHRADJUST_403                       0x00
    #define TOUCHTHRADJUST_404                       0x00
    #define TOUCHTHRADJUST_405                       0x00
    #define TOUCHTHRADJUST_406                       0x00
    #define TOUCHTHRADJUST_407                       0x00
    #define TOUCHTHRADJUST_408                       0x00
    #define TOUCHTHRADJUST_409                       0x00
    #define TOUCHTHRADJUST_410                       0x00
    #define TOUCHTHRADJUST_411                       0x00
    #define TOUCHTHRADJUST_412                       0x00
    #define TOUCHTHRADJUST_413                       0x00
    #define TOUCHTHRADJUST_414                       0x00
    #define TOUCHTHRADJUST_415                       0x00
    #define TOUCHTHRADJUST_416                       0x00
    #define TOUCHTHRADJUST_417                       0x00
    #define TOUCHTHRADJUST_418                       0x00
    #define TOUCHTHRADJUST_419                       0x00
    #define TOUCHTHRADJUST_420                       0x00
    #define TOUCHTHRADJUST_421                       0x00
    #define TOUCHTHRADJUST_422                       0x00
    #define TOUCHTHRADJUST_423                       0x00
    #define TOUCHTHRADJUST_424                       0x00
    #define TOUCHTHRADJUST_425                       0x00
    #define TOUCHTHRADJUST_426                       0x00
    #define TOUCHTHRADJUST_427                       0x00
    #define TOUCHTHRADJUST_428                       0x00
    #define TOUCHTHRADJUST_429                       0x00
    #define TOUCHTHRADJUST_430                       0x00
    #define TOUCHTHRADJUST_431                       0x00
    #define TOUCHTHRADJUST_432                       0x00
    #define TOUCHTHRADJUST_433                       0x00
    #define TOUCHTHRADJUST_434                       0x00
    #define TOUCHTHRADJUST_435                       0x00
    #define TOUCHTHRADJUST_436                       0x00
    #define TOUCHTHRADJUST_437                       0x00
    #define TOUCHTHRADJUST_438                       0x00
    #define TOUCHTHRADJUST_439                       0x00
    #define TOUCHTHRADJUST_440                       0x00
    #define TOUCHTHRADJUST_441                       0x00
    #define TOUCHTHRADJUST_442                       0x00
    #define TOUCHTHRADJUST_443                       0x00
    #define TOUCHTHRADJUST_444                       0x00
    #define TOUCHTHRADJUST_445                       0x00
    #define TOUCHTHRADJUST_446                       0x00
    #define TOUCHTHRADJUST_447                       0x00
    #define TOUCHTHRADJUST_448                       0x00
    #define TOUCHTHRADJUST_449                       0x00
    #define TOUCHTHRADJUST_450                       0x00
    #define TOUCHTHRADJUST_451                       0x00
    #define TOUCHTHRADJUST_452                       0x00
    #define TOUCHTHRADJUST_453                       0x00
    #define TOUCHTHRADJUST_454                       0x00
    #define TOUCHTHRADJUST_455                       0x00
    #define TOUCHTHRADJUST_456                       0x00
    #define TOUCHTHRADJUST_457                       0x00
    #define TOUCHTHRADJUST_458                       0x00
    #define TOUCHTHRADJUST_459                       0x00
    #define TOUCHTHRADJUST_460                       0x00
    #define TOUCHTHRADJUST_461                       0x00
    #define TOUCHTHRADJUST_462                       0x00
    #define TOUCHTHRADJUST_463                       0x00
    #define TOUCHTHRADJUST_464                       0x00
    #define TOUCHTHRADJUST_465                       0x00
    #define TOUCHTHRADJUST_466                       0x00
    #define TOUCHTHRADJUST_467                       0x00
    #define TOUCHTHRADJUST_468                       0x00
    #define TOUCHTHRADJUST_469                       0x00
    #define TOUCHTHRADJUST_470                       0x00
    #define TOUCHTHRADJUST_471                       0x00
    #define TOUCHTHRADJUST_472                       0x00
    #define TOUCHTHRADJUST_473                       0x00
    #define TOUCHTHRADJUST_474                       0x00
    #define TOUCHTHRADJUST_475                       0x00
    #define TOUCHTHRADJUST_476                       0x00
    #define TOUCHTHRADJUST_477                       0x00
    #define TOUCHTHRADJUST_478                       0x00
    #define TOUCHTHRADJUST_479                       0x00
    #define TOUCHTHRADJUST_480                       0x00
    #define TOUCHTHRADJUST_481                       0x00
    #define TOUCHTHRADJUST_482                       0x00
    #define TOUCHTHRADJUST_483                       0x00
    #define TOUCHTHRADJUST_484                       0x00
    #define TOUCHTHRADJUST_485                       0x00
    #define TOUCHTHRADJUST_486                       0x00
    #define TOUCHTHRADJUST_487                       0x00
    #define TOUCHTHRADJUST_488                       0x00
    #define TOUCHTHRADJUST_489                       0x00
    #define TOUCHTHRADJUST_490                       0x00
    #define TOUCHTHRADJUST_491                       0x00
    #define TOUCHTHRADJUST_492                       0x00
    #define TOUCHTHRADJUST_493                       0x00
    #define TOUCHTHRADJUST_494                       0x00
    #define TOUCHTHRADJUST_495                       0x00
    #define TOUCHTHRADJUST_496                       0x00
    #define TOUCHTHRADJUST_497                       0x00
    #define TOUCHTHRADJUST_498                       0x00
    #define TOUCHTHRADJUST_499                       0x00
    #define TOUCHTHRADJUST_500                       0x00
    #define TOUCHTHRADJUST_501                       0x00
    #define TOUCHTHRADJUST_502                       0x00
    #define TOUCHTHRADJUST_503                       0x00
    #define TOUCHTHRADJUST_504                       0x00
    #define TOUCHTHRADJUST_505                       0x00

    /* Virtual Buttons 0 */
    /* Memory Map Position 0x14F0 - 0x1511 */
    #define NUMBER_OF_VIRTUAL_SENSORS_0              0x00
    #define NUMBER_OF_VIRTUAL_SENSORS_1              0x00
    #define B0_TOP_LEFT_X_0                          0x00
    #define B0_TOP_LEFT_X_1                          0x00
    #define B0_TOP_LEFT_Y_0                          0x00
    #define B0_TOP_LEFT_Y_1                          0x00
    #define B0_BOT_RIGHT_X_0                         0x00
    #define B0_BOT_RIGHT_X_1                         0x00
    #define B0_BOT_RIGHT_Y_0                         0x00
    #define B0_BOT_RIGHT_Y_1                         0x00
    #define B1_TOP_LEFT_X_0                          0x00
    #define B1_TOP_LEFT_X_1                          0x00
    #define B1_TOP_LEFT_Y_0                          0x00
    #define B1_TOP_LEFT_Y_1                          0x00
    #define B1_BOT_RIGHT_X_0                         0x00
    #define B1_BOT_RIGHT_X_1                         0x00
    #define B1_BOT_RIGHT_Y_0                         0x00
    #define B1_BOT_RIGHT_Y_1                         0x00
    #define B2_TOP_LEFT_X_0                          0x00
    #define B2_TOP_LEFT_X_1                          0x00
    #define B2_TOP_LEFT_Y_0                          0x00
    #define B2_TOP_LEFT_Y_1                          0x00
    #define B2_BOT_RIGHT_X_0                         0x00
    #define B2_BOT_RIGHT_X_1                         0x00
    #define B2_BOT_RIGHT_Y_0                         0x00
    #define B2_BOT_RIGHT_Y_1                         0x00
    #define B3_TOP_LEFT_X_0                          0x00
    #define B3_TOP_LEFT_X_1                          0x00
    #define B3_TOP_LEFT_Y_0                          0x00
    #define B3_TOP_LEFT_Y_1                          0x00
    #define B3_BOT_RIGHT_X_0                         0x00
    #define B3_BOT_RIGHT_X_1                         0x00
    #define B3_BOT_RIGHT_Y_0                         0x00
    #define B3_BOT_RIGHT_Y_1                         0x00

    /* Virtual Buttons 1 */
    /* Memory Map Position 0x1512 - 0x1541 */
    #define B4_TOP_LEFT_X_0                          0x00
    #define B4_TOP_LEFT_X_1                          0x00
    #define B4_TOP_LEFT_Y_0                          0x00
    #define B4_TOP_LEFT_Y_1                          0x00
    #define B4_BOT_RIGHT_X_0                         0x00
    #define B4_BOT_RIGHT_X_1                         0x00
    #define B4_BOT_RIGHT_Y_0                         0x00
    #define B4_BOT_RIGHT_Y_1                         0x00
    #define B5_TOP_LEFT_X_0                          0x00
    #define B5_TOP_LEFT_X_1                          0x00
    #define B5_TOP_LEFT_Y_0                          0x00
    #define B5_TOP_LEFT_Y_1                          0x00
    #define B5_BOT_RIGHT_X_0                         0x00
    #define B5_BOT_RIGHT_X_1                         0x00
    #define B5_BOT_RIGHT_Y_0                         0x00
    #define B5_BOT_RIGHT_Y_1                         0x00
    #define B6_TOP_LEFT_X_0                          0x00
    #define B6_TOP_LEFT_X_1                          0x00
    #define B6_TOP_LEFT_Y_0                          0x00
    #define B6_TOP_LEFT_Y_1                          0x00
    #define B6_BOT_RIGHT_X_0                         0x00
    #define B6_BOT_RIGHT_X_1                         0x00
    #define B6_BOT_RIGHT_Y_0                         0x00
    #define B6_BOT_RIGHT_Y_1                         0x00
    #define B7_TOP_LEFT_X_0                          0x00
    #define B7_TOP_LEFT_X_1                          0x00
    #define B7_TOP_LEFT_Y_0                          0x00
    #define B7_TOP_LEFT_Y_1                          0x00
    #define B7_BOT_RIGHT_X_0                         0x00
    #define B7_BOT_RIGHT_X_1                         0x00
    #define B7_BOT_RIGHT_Y_0                         0x00
    #define B7_BOT_RIGHT_Y_1                         0x00
    #define B8_TOP_LEFT_X_0                          0x00
    #define B8_TOP_LEFT_X_1                          0x00
    #define B8_TOP_LEFT_Y_0                          0x00
    #define B8_TOP_LEFT_Y_1                          0x00
    #define B8_BOT_RIGHT_X_0                         0x00
    #define B8_BOT_RIGHT_X_1                         0x00
    #define B8_BOT_RIGHT_Y_0                         0x00
    #define B8_BOT_RIGHT_Y_1                         0x00
    #define B9_TOP_LEFT_X_0                          0x00
    #define B9_TOP_LEFT_X_1                          0x00
    #define B9_TOP_LEFT_Y_0                          0x00
    #define B9_TOP_LEFT_Y_1                          0x00
    #define B9_BOT_RIGHT_X_0                         0x00
    #define B9_BOT_RIGHT_X_1                         0x00
    #define B9_BOT_RIGHT_Y_0                         0x00
    #define B9_BOT_RIGHT_Y_1                         0x00

    /* Virtual Buttons 2 */
    /* Memory Map Position 0x1542 - 0x1571 */
    #define B10_TOP_LEFT_X_0                         0x00
    #define B10_TOP_LEFT_X_1                         0x00
    #define B10_TOP_LEFT_Y_0                         0x00
    #define B10_TOP_LEFT_Y_1                         0x00
    #define B10_BOT_RIGHT_X_0                        0x00
    #define B10_BOT_RIGHT_X_1                        0x00
    #define B10_BOT_RIGHT_Y_0                        0x00
    #define B10_BOT_RIGHT_Y_1                        0x00
    #define B11_TOP_LEFT_X_0                         0x00
    #define B11_TOP_LEFT_X_1                         0x00
    #define B11_TOP_LEFT_Y_0                         0x00
    #define B11_TOP_LEFT_Y_1                         0x00
    #define B11_BOT_RIGHT_X_0                        0x00
    #define B11_BOT_RIGHT_X_1                        0x00
    #define B11_BOT_RIGHT_Y_0                        0x00
    #define B11_BOT_RIGHT_Y_1                        0x00
    #define B12_TOP_LEFT_X_0                         0x00
    #define B12_TOP_LEFT_X_1                         0x00
    #define B12_TOP_LEFT_Y_0                         0x00
    #define B12_TOP_LEFT_Y_1                         0x00
    #define B12_BOT_RIGHT_X_0                        0x00
    #define B12_BOT_RIGHT_X_1                        0x00
    #define B12_BOT_RIGHT_Y_0                        0x00
    #define B12_BOT_RIGHT_Y_1                        0x00
    #define B13_TOP_LEFT_X_0                         0x00
    #define B13_TOP_LEFT_X_1                         0x00
    #define B13_TOP_LEFT_Y_0                         0x00
    #define B13_TOP_LEFT_Y_1                         0x00
    #define B13_BOT_RIGHT_X_0                        0x00
    #define B13_BOT_RIGHT_X_1                        0x00
    #define B13_BOT_RIGHT_Y_0                        0x00
    #define B13_BOT_RIGHT_Y_1                        0x00
    #define B14_TOP_LEFT_X_0                         0x00
    #define B14_TOP_LEFT_X_1                         0x00
    #define B14_TOP_LEFT_Y_0                         0x00
    #define B14_TOP_LEFT_Y_1                         0x00
    #define B14_BOT_RIGHT_X_0                        0x00
    #define B14_BOT_RIGHT_X_1                        0x00
    #define B14_BOT_RIGHT_Y_0                        0x00
    #define B14_BOT_RIGHT_Y_1                        0x00
    #define B15_TOP_LEFT_X_0                         0x00
    #define B15_TOP_LEFT_X_1                         0x00
    #define B15_TOP_LEFT_Y_0                         0x00
    #define B15_TOP_LEFT_Y_1                         0x00
    #define B15_BOT_RIGHT_X_0                        0x00
    #define B15_BOT_RIGHT_X_1                        0x00
    #define B15_BOT_RIGHT_Y_0                        0x00
    #define B15_BOT_RIGHT_Y_1                        0x00

    /* Virtual Sliders 0 */
    /* Memory Map Position 0x1572 - 0x15A5 */
    #define SLIDER_DEADZONE_0                        0x00
    #define SLIDER_DEADZONE_1                        0x00
    #define S0_TOP_LEFT_X_0                          0x00
    #define S0_TOP_LEFT_X_1                          0x00
    #define S0_TOP_LEFT_Y_0                          0x00
    #define S0_TOP_LEFT_Y_1                          0x00
    #define S0_BOT_RIGHT_X_0                         0x00
    #define S0_BOT_RIGHT_X_1                         0x00
    #define S0_BOT_RIGHT_Y_0                         0x00
    #define S0_BOT_RIGHT_Y_1                         0x00
    #define S0_RESOLUTION_0                          0x00
    #define S0_RESOLUTION_1                          0x00
    #define S1_TOP_LEFT_X_0                          0x00
    #define S1_TOP_LEFT_X_1                          0x00
    #define S1_TOP_LEFT_Y_0                          0x00
    #define S1_TOP_LEFT_Y_1                          0x00
    #define S1_BOT_RIGHT_X_0                         0x00
    #define S1_BOT_RIGHT_X_1                         0x00
    #define S1_BOT_RIGHT_Y_0                         0x00
    #define S1_BOT_RIGHT_Y_1                         0x00
    #define S1_RESOLUTION_0                          0x00
    #define S1_RESOLUTION_1                          0x00
    #define S2_TOP_LEFT_X_0                          0x00
    #define S2_TOP_LEFT_X_1                          0x00
    #define S2_TOP_LEFT_Y_0                          0x00
    #define S2_TOP_LEFT_Y_1                          0x00
    #define S2_BOT_RIGHT_X_0                         0x00
    #define S2_BOT_RIGHT_X_1                         0x00
    #define S2_BOT_RIGHT_Y_0                         0x00
    #define S2_BOT_RIGHT_Y_1                         0x00
    #define S2_RESOLUTION_0                          0x00
    #define S2_RESOLUTION_1                          0x00
    #define S3_TOP_LEFT_X_0                          0x00
    #define S3_TOP_LEFT_X_1                          0x00
    #define S3_TOP_LEFT_Y_0                          0x00
    #define S3_TOP_LEFT_Y_1                          0x00
    #define S3_BOT_RIGHT_X_0                         0x00
    #define S3_BOT_RIGHT_X_1                         0x00
    #define S3_BOT_RIGHT_Y_0                         0x00
    #define S3_BOT_RIGHT_Y_1                         0x00
    #define S3_RESOLUTION_0                          0x00
    #define S3_RESOLUTION_1                          0x00
    #define S4_TOP_LEFT_X_0                          0x00
    #define S4_TOP_LEFT_X_1                          0x00
    #define S4_TOP_LEFT_Y_0                          0x00
    #define S4_TOP_LEFT_Y_1                          0x00
    #define S4_BOT_RIGHT_X_0                         0x00
    #define S4_BOT_RIGHT_X_1                         0x00
    #define S4_BOT_RIGHT_Y_0                         0x00
    #define S4_BOT_RIGHT_Y_1                         0x00
    #define S4_RESOLUTION_0                          0x00
    #define S4_RESOLUTION_1                          0x00

    /* Virtual Sliders 1 */
    /* Memory Map Position 0x15A6 - 0x15C3 */
    #define S5_TOP_LEFT_X_0                          0x00
    #define S5_TOP_LEFT_X_1                          0x00
    #define S5_TOP_LEFT_Y_0                          0x00
    #define S5_TOP_LEFT_Y_1                          0x00
    #define S5_BOT_RIGHT_X_0                         0x00
    #define S5_BOT_RIGHT_X_1                         0x00
    #define S5_BOT_RIGHT_Y_0                         0x00
    #define S5_BOT_RIGHT_Y_1                         0x00
    #define S5_RESOLUTION_0                          0x00
    #define S5_RESOLUTION_1                          0x00
    #define S6_TOP_LEFT_X_0                          0x00
    #define S6_TOP_LEFT_X_1                          0x00
    #define S6_TOP_LEFT_Y_0                          0x00
    #define S6_TOP_LEFT_Y_1                          0x00
    #define S6_BOT_RIGHT_X_0                         0x00
    #define S6_BOT_RIGHT_X_1                         0x00
    #define S6_BOT_RIGHT_Y_0                         0x00
    #define S6_BOT_RIGHT_Y_1                         0x00
    #define S6_RESOLUTION_0                          0x00
    #define S6_RESOLUTION_1                          0x00
    #define S7_TOP_LEFT_X_0                          0x00
    #define S7_TOP_LEFT_X_1                          0x00
    #define S7_TOP_LEFT_Y_0                          0x00
    #define S7_TOP_LEFT_Y_1                          0x00
    #define S7_BOT_RIGHT_X_0                         0x00
    #define S7_BOT_RIGHT_X_1                         0x00
    #define S7_BOT_RIGHT_Y_0                         0x00
    #define S7_BOT_RIGHT_Y_1                         0x00
    #define S7_RESOLUTION_0                          0x00
    #define S7_RESOLUTION_1                          0x00

    /* Virtual Wheels */
    /* Memory Map Position 0x15C4 - 0x15EB */
    #define W0_CENTRE_X_0                            0x00
    #define W0_CENTRE_X_1                            0x00
    #define W0_CENTRE_Y_0                            0x00
    #define W0_CENTRE_Y_1                            0x00
    #define W0_INNER_RADIUS_0                        0x00
    #define W0_INNER_RADIUS_1                        0x00
    #define W0_OUTER_RADIUS_0                        0x00
    #define W0_OUTER_RADIUS_1                        0x00
    #define W0_RESOLUTION_0                          0x00
    #define W0_RESOLUTION_1                          0x00
    #define W1_CENTRE_X_0                            0x00
    #define W1_CENTRE_X_1                            0x00
    #define W1_CENTRE_Y_0                            0x00
    #define W1_CENTRE_Y_1                            0x00
    #define W1_INNER_RADIUS_0                        0x00
    #define W1_INNER_RADIUS_1                        0x00
    #define W1_OUTER_RADIUS_0                        0x00
    #define W1_OUTER_RADIUS_1                        0x00
    #define W1_RESOLUTION_0                          0x00
    #define W1_RESOLUTION_1                          0x00
    #define W2_CENTRE_X_0                            0x00
    #define W2_CENTRE_X_1                            0x00
    #define W2_CENTRE_Y_0                            0x00
    #define W2_CENTRE_Y_1                            0x00
    #define W2_INNER_RADIUS_0                        0x00
    #define W2_INNER_RADIUS_1                        0x00
    #define W2_OUTER_RADIUS_0                        0x00
    #define W2_OUTER_RADIUS_1                        0x00
    #define W2_RESOLUTION_0                          0x00
    #define W2_RESOLUTION_1                          0x00
    #define W3_CENTRE_X_0                            0x00
    #define W3_CENTRE_X_1                            0x00
    #define W3_CENTRE_Y_0                            0x00
    #define W3_CENTRE_Y_1                            0x00
    #define W3_INNER_RADIUS_0                        0x00
    #define W3_INNER_RADIUS_1                        0x00
    #define W3_OUTER_RADIUS_0                        0x00
    #define W3_OUTER_RADIUS_1                        0x00
    #define W3_RESOLUTION_0                          0x00
    #define W3_RESOLUTION_1                          0x00

    /* Eng Settings */
    /* Memory Map Position 0x2000 - 0x2005 */
    #define ENG_CONFIG_0                             0x01
    #define ENG_CONFIG_1                             0x02
    #define ENG_0                                    0x03
    #define ENG_1                                    0x03
    #define ENG_2                                    0x14
    #define ENG_3                                    0x00

#endif // SETUP

// Driver interface structure
const pointing_device_driver_t azoteq_iqs9150_pointing_device_driver = {
    .init       = azoteq_iqs9150_init,
    .get_report = azoteq_iqs9150_get_report,
    .set_cpi    = azoteq_iqs9150_set_cpi,
    .get_cpi    = azoteq_iqs9150_get_cpi,
};

// Global variables
static uint16_t azoteq_iqs9150_product_number = AZOTEQ_IQS9150_UNKNOWN;
static azoteq_iqs9150_comms_mode_t azoteq_iqs9150_comms_mode = AZOTEQ_IQS9150_COMMS_MODE_WAIT;
// static azoteq_iqs9150_ver_info_t azoteq_iqs9150_ver_info = {0};

static struct {
    uint16_t resolution_x;
    uint16_t resolution_y;
} azoteq_iqs9150_device_resolution_t;

static i2c_status_t azoteq_iqs9150_init_status = 1;

// Communication helper functions
i2c_status_t azoteq_iqs9150_end_session(void) {
    const uint8_t END_BYTE = 1; // any data
    return i2c_write_register16(AZOTEQ_IQS9150_ADDRESS, IQS9150_END_COMMS, &END_BYTE, 1, AZOTEQ_IQS9150_TIMEOUT_MS);
}

i2c_status_t azoteq_iqs9150_wake(void) {
    uprintf("Pinging:%02X\n", AZOTEQ_IQS9150_ADDRESS);
    return i2c_ping_address(AZOTEQ_IQS9150_ADDRESS, 1);
}

static i2c_status_t azoteq_iqs9150_force_comms(void) {
    uint8_t msg_buf[] = { 0xFF, };

    switch (azoteq_iqs9150_comms_mode) {
        case AZOTEQ_IQS9150_COMMS_MODE_WAIT:
            // For simplicity in QMK, we'll just wait a bit
            wait_us(100);
            return I2C_STATUS_SUCCESS;

        case AZOTEQ_IQS9150_COMMS_MODE_FREE:
            return I2C_STATUS_SUCCESS;

        case AZOTEQ_IQS9150_COMMS_MODE_FORCE:
            // Send force communication command
            /* Ensure we pass a pointer of type 'const uint8_t *' to match
             * the i2c_transmit prototype. Passing '&msg_buf' would produce
             * a 'uint8_t (*)[1]' type which is incompatible. */
            return i2c_transmit(AZOTEQ_IQS9150_ADDRESS, (const uint8_t *)msg_buf,
                                sizeof(msg_buf), AZOTEQ_IQS9150_TIMEOUT_MS);

        default:
            return I2C_STATUS_ERROR;
    }
}

static i2c_status_t azoteq_iqs9150_read_burst(uint16_t reg, void *val, uint16_t val_len) {
    i2c_status_t status;
    uint16_t rem_len = val_len;
    uint8_t *val_ptr = (uint8_t *)val;

    while (rem_len) {
        uint16_t burst_len = MIN(rem_len, IQS9150_MAX_LEN);
        uint16_t reg_offs = val_len - rem_len;

        status = azoteq_iqs9150_force_comms();
        if (status != I2C_STATUS_SUCCESS) {
            return status;
        }

        status = i2c_read_register16(AZOTEQ_IQS9150_ADDRESS, reg + reg_offs,
                                   val_ptr + reg_offs, burst_len, AZOTEQ_IQS9150_TIMEOUT_MS);
        if (status != I2C_STATUS_SUCCESS) {
            return status;
        }

        // Check for communication error - use direct casting like 5XX driver
        if (burst_len >= 2) {
            uint16_t *error_check_ptr = (uint16_t *)(val_ptr + reg_offs);
            if (*error_check_ptr == IQS9150_COMMS_ERROR) {
                return I2C_STATUS_ERROR;
            }
        }

        rem_len -= burst_len;
    }

    return I2C_STATUS_SUCCESS;
}

static i2c_status_t azoteq_iqs9150_read_word(uint16_t reg, uint16_t *val) {
    uint16_t val_buf;
    i2c_status_t status = azoteq_iqs9150_read_burst(reg, &val_buf, sizeof(val_buf));
    if (status == I2C_STATUS_SUCCESS) {
        // Use SWAP_H_L_BYTES like the 5XX driver for endianness handling
        *val = AZOTEQ_IQS9150_SWAP_H_L_BYTES(val_buf);
    }
    return status;
}

static i2c_status_t azoteq_iqs9150_write_burst(uint16_t reg, const void *val, uint16_t val_len) {
    i2c_status_t status;
    uint16_t rem_len = val_len;
    const uint8_t *val_ptr = (const uint8_t *)val;

    while (rem_len) {
        uint16_t burst_len = MIN(rem_len, IQS9150_MAX_LEN);
        uint16_t reg_offs = val_len - rem_len;

        status = azoteq_iqs9150_force_comms();
        if (status != I2C_STATUS_SUCCESS) {
            return status;
        }

        status = i2c_write_register16(AZOTEQ_IQS9150_ADDRESS, reg + reg_offs,
                                    val_ptr + reg_offs, burst_len, AZOTEQ_IQS9150_TIMEOUT_MS);
        if (status != I2C_STATUS_SUCCESS) {
            return status;
        }

        rem_len -= burst_len;
    }

    return I2C_STATUS_SUCCESS;
}

static i2c_status_t azoteq_iqs9150_write_word(uint16_t reg, uint16_t val) {
    uint16_t val_buf = AZOTEQ_IQS9150_SWAP_H_L_BYTES(val);
    return azoteq_iqs9150_write_burst(reg, &val_buf, sizeof(val_buf));
}

// Configuration functions
i2c_status_t azoteq_iqs9150_reset_suspend(bool reset, bool suspend, bool end_session) {
    uint16_t control = 0;
    i2c_status_t status;

    uprintf("IQS9150: Reset/suspend - reset:%d, suspend:%d\n", reset, suspend);

    status = azoteq_iqs9150_read_word(IQS9150_CONTROL, &control);
    if (status == I2C_STATUS_SUCCESS) {
        uprintf("IQS9150: Read control register: 0x%04X\n", control);
        if (reset) {
            // control |= IQS9150_CONTROL_ACK_RESET;
            control = IQS9150_CONTROL_ACK_RESET;
        }
        if (suspend) {
            control |= IQS9150_CONTROL_SUSPEND;
        } else {
            control &= ~IQS9150_CONTROL_SUSPEND;
        }

        status = azoteq_iqs9150_write_word(IQS9150_CONTROL, control);
        if (status == I2C_STATUS_SUCCESS) {
            uprintf("IQS9150: Wrote control register: 0x%04X\n", control);
        } else {
            uprintf("IQS9150: Failed to write control register, status: %d\n", status);
        }
    } else {
        uprintf("IQS9150: Failed to read control register, status: %d\n", status);
    }

    if (end_session) {
        azoteq_iqs9150_end_session();
    }

    return status;
}

i2c_status_t azoteq_iqs9150_set_report_rate(uint16_t report_rate_ms, azoteq_iqs9150_charging_modes_t mode, bool end_session) {
    uprintf("IQS9150: Set report rate %dms, mode: %d\n", report_rate_ms, mode);
    if (end_session) {
        azoteq_iqs9150_end_session();
    }
    return I2C_STATUS_SUCCESS;
}

i2c_status_t azoteq_iqs9150_set_event_mode(bool enabled, bool end_session) {
    uint16_t config = 0;
    i2c_status_t status;

    uprintf("IQS9150: Set event mode: %d\n", enabled);

    status = azoteq_iqs9150_read_word(IQS9150_CONFIG, &config);
    if (status == I2C_STATUS_SUCCESS) {
        uprintf("IQS9150: Read config register: 0x%04X\n", config);
        if (enabled) {
            config |= IQS9150_CONFIG_EVENT_MODE;
        } else {
            config &= ~IQS9150_CONFIG_EVENT_MODE;
        }

        status = azoteq_iqs9150_write_word(IQS9150_CONFIG, config);
        if (status == I2C_STATUS_SUCCESS) {
            uprintf("IQS9150: Wrote config register: 0x%04X\n", config);
        } else {
            uprintf("IQS9150: Failed to write config register, status: %d\n", status);
        }
    } else {
        uprintf("IQS9150: Failed to read config register, status: %d\n", status);
    }

    if (end_session) {
        azoteq_iqs9150_end_session();
    }

    return status;
}

i2c_status_t azoteq_iqs9150_set_xy_config(bool flip_x, bool flip_y, bool switch_xy, bool end_session) {
    uprintf("IQS9150: Set XY config - flip_x:%d, flip_y:%d, switch_xy:%d\n", flip_x, flip_y, switch_xy);
    // This would need to be implemented based on specific register map
    // For now, return success as a placeholder
    if (end_session) {
        azoteq_iqs9150_end_session();
    }
    return I2C_STATUS_SUCCESS;
}

// CPI and resolution functions
void azoteq_iqs9150_set_cpi(uint16_t cpi) {
    if (azoteq_iqs9150_product_number != AZOTEQ_IQS9150_UNKNOWN) {
        azoteq_iqs9150_resolution_t resolution = {0};
        resolution.x_resolution = AZOTEQ_IQS9150_SWAP_H_L_BYTES(MIN(azoteq_iqs9150_device_resolution_t.resolution_x,
                                                                   AZOTEQ_IQS9150_INCH_TO_RESOLUTION_X(cpi)));
        resolution.y_resolution = AZOTEQ_IQS9150_SWAP_H_L_BYTES(MIN(azoteq_iqs9150_device_resolution_t.resolution_y,
                                                                   AZOTEQ_IQS9150_INCH_TO_RESOLUTION_Y(cpi)));
        azoteq_iqs9150_write_burst(IQS9150_X_RES, &resolution, sizeof(azoteq_iqs9150_resolution_t));
    }
}

uint16_t azoteq_iqs9150_get_cpi(void) {
    if (azoteq_iqs9150_product_number != AZOTEQ_IQS9150_UNKNOWN) {
        azoteq_iqs9150_resolution_t resolution = {0};
        i2c_status_t status = azoteq_iqs9150_read_burst(IQS9150_X_RES, &resolution, sizeof(azoteq_iqs9150_resolution_t));
        if (status == I2C_STATUS_SUCCESS) {
            return AZOTEQ_IQS9150_RESOLUTION_X_TO_INCH(AZOTEQ_IQS9150_SWAP_H_L_BYTES(resolution.x_resolution));
        }
    }
    return 0;
}

uint16_t azoteq_iqs9150_get_product(void) {
    // uprintf("Pin:%ld\n", readPin(A9));
    i2c_status_t status = azoteq_iqs9150_read_word(IQS9150_PROD_NUM, &azoteq_iqs9150_product_number);
    wait_ms(10);
    azoteq_iqs9150_end_session();
    
    if (status != I2C_STATUS_SUCCESS) {
        azoteq_iqs9150_product_number = AZOTEQ_IQS9150_UNKNOWN;
        uprintf("Read unsuccessful\n");
    }
    uprintf("IQS9150: Product number 0x%04X\n", azoteq_iqs9150_product_number);
    return azoteq_iqs9150_product_number;
}

static void azoteq_iqs9150_setup_resolution(void) {
    uprintf("IQS9150: Setting up resolution for product 0x%04X\n", azoteq_iqs9150_product_number);

    // Set default resolution based on product
    switch (azoteq_iqs9150_product_number) {
        case AZOTEQ_IQS9150:
            azoteq_iqs9150_device_resolution_t.resolution_x = AZOTEQ_IQS9150_RESOLUTION_X;
            azoteq_iqs9150_device_resolution_t.resolution_y = AZOTEQ_IQS9150_RESOLUTION_Y;
            break;
        case AZOTEQ_IQS9151:
            azoteq_iqs9150_device_resolution_t.resolution_x = AZOTEQ_IQS9150_RESOLUTION_X;
            azoteq_iqs9150_device_resolution_t.resolution_y = AZOTEQ_IQS9150_RESOLUTION_Y;
            break;
        default:
            azoteq_iqs9150_device_resolution_t.resolution_x = AZOTEQ_IQS9150_RESOLUTION_X;
            azoteq_iqs9150_device_resolution_t.resolution_y = AZOTEQ_IQS9150_RESOLUTION_Y;
            break;
    }

    uprintf("IQS9150: Resolution set to %dx%d\n",
               azoteq_iqs9150_device_resolution_t.resolution_x,
               azoteq_iqs9150_device_resolution_t.resolution_y);
}

// Data reading functions
i2c_status_t azoteq_iqs9150_get_base_data(azoteq_iqs9150_base_data_t *base_data) {
    azoteq_iqs9150_status_t status_report = {0};
    i2c_status_t status;

    status = azoteq_iqs9150_read_burst(IQS9150_STATUS, &status_report, sizeof(status_report));
    if (status == I2C_STATUS_SUCCESS) {
        // Extract basic data from full status report
        base_data->num_contacts = status_report.num_contacts;
        base_data->gestures = status_report.gesture_events;
        base_data->info = status_report.system_info;

        // Get touch coordinates from first contact
        if (status_report.num_contacts > 0) {
            base_data->x = (int16_t)AZOTEQ_IQS9150_SWAP_H_L_BYTES(status_report.touch_data[0].abs_x);
            base_data->y = (int16_t)AZOTEQ_IQS9150_SWAP_H_L_BYTES(status_report.touch_data[0].abs_y);
        } else {
            base_data->x = 0;
            base_data->y = 0;
        }

        azoteq_iqs9150_end_session();
    }

    return status;
}

// Load complete device configuration from IQS9150_init.h
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

static void tester(void) {
    // Wait for RDY low (A9) before starting I2C, per datasheet
    const uint16_t rdy_timeout_ms = 1000;
    uint16_t waited = 0;
    while (readPin(A9)) {
        if (waited++ >= rdy_timeout_ms) {
            uprintf("IQS9150: RDY high for %dms, proceeding anyway\n", rdy_timeout_ms);
            break;
        }
        wait_ms(1);
    }

    // Read the 16-bit Product Number register (0x1000)
    uint8_t buf[2] = {0};
    i2c_status_t status = i2c_read_register16(AZOTEQ_IQS9150_ADDRESS, IQS9150_PROD_NUM, buf, sizeof(buf), 100);

    // azoteq_iqs9150_end_session();

    if (status == I2C_STATUS_SUCCESS) {
        uint16_t product = ((uint16_t)buf[0] << 8) | buf[1]; // MSB first
        uprintf("IQS9150: Product number read OK: 0x%04X\n", product);
        azoteq_iqs9150_end_session();
    } else {
        uprintf("IQS9150: Product number read failed, i2c status: %d (addr 0x%02X)\n",
                status, AZOTEQ_IQS9150_ADDRESS);
    }
}

// Main driver functions
void azoteq_iqs9150_init(void) {
    wait_ms(3000);
    uprintf("IQS9150: Starting init\n");
    i2c_init();
    azoteq_iqs9150_wake();
    azoteq_iqs9150_init_status = azoteq_iqs9150_load_settings();
    azoteq_iqs9150_scan_bus();
    azoteq_iqs9150_reset_suspend(true, false, true);
    
    tester();
    
    // azoteq_iqs9150_get_product();

    wait_ms(3000);
    
    if (azoteq_iqs9150_get_product() != AZOTEQ_IQS9150_UNKNOWN) {
        uprintf("IQS9150: Device detected, product: 0x%04X\n", azoteq_iqs9150_product_number);
        
        // Load complete device configuration from IQS9150_init.h if available
        
        if (azoteq_iqs9150_init_status != I2C_STATUS_SUCCESS) {
            uprintf("IQS9150: Settings load failed, applying minimal config\n");
        }
        
        azoteq_iqs9150_setup_resolution();

        // Set communication mode based on device capabilities
        azoteq_iqs9150_comms_mode = AZOTEQ_IQS9150_COMMS_MODE_FREE;
        uprintf("IQS9150: Set communication mode to FREE\n");

        // Configure basic settings (if not loaded from init file)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_report_rate(AZOTEQ_IQS9150_REPORT_RATE, AZOTEQ_IQS9150_ACTIVE, false);
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_event_mode(false, false); // Use streaming mode for QMK

#if defined(AZOTEQ_IQS9150_ROTATION_90)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(false, true, true, false);
        uprintf("IQS9150: Applied 90 degree rotation\n");
#elif defined(AZOTEQ_IQS9150_ROTATION_180)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(true, true, false, false);
        uprintf("IQS9150: Applied 180 degree rotation\n");
#elif defined(AZOTEQ_IQS9150_ROTATION_270)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(true, false, true, false);
        uprintf("IQS9150: Applied 270 degree rotation\n");
#else
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(false, false, false, false);
        uprintf("IQS9150: No rotation applied\n");
#endif

        azoteq_iqs9150_end_session();
        wait_ms(AZOTEQ_IQS9150_REPORT_RATE + 1);

        if (azoteq_iqs9150_init_status == I2C_STATUS_SUCCESS) {
            uprintf("IQS9150: Init completed successfully\n");
        } else {
            uprintf("IQS9150: Init completed with errors, status: %d\n", azoteq_iqs9150_init_status);
        }
    } else {
        uprintf("IQS9150: Device not detected or unknown product\n");
    }
}

report_mouse_t azoteq_iqs9150_get_report(report_mouse_t mouse_report) {
    report_mouse_t temp_report = {0};
    // uprintf("HI\n");
    

    if (azoteq_iqs9150_init_status == I2C_STATUS_SUCCESS) {
        // uprintf("nigger\n");
        azoteq_iqs9150_base_data_t base_data = {0};
        i2c_status_t status = azoteq_iqs9150_get_base_data(&base_data);
        bool ignore_movement = false;

        if (status == I2C_STATUS_SUCCESS) {
            // Handle reset condition

            if (base_data.info.show_reset) {
                uprintf("IQS9150 - Device reset detected\n");
                azoteq_iqs9150_init(); // Reinitialize
                return temp_report;
            }

            // Handle gestures
            if (base_data.gestures.tap) {
                uprintf("IQS9150 - Single tap\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON1);
            } else if (base_data.gestures.hold) {
                uprintf("IQS9150 - Hold gesture\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON1);
            } else if (base_data.gestures.two_finger_tap) {
                uprintf("IQS9150 - Two finger tap\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON2);
            } else if (base_data.gestures.swipe_x_neg) {
                uprintf("IQS9150 - Swipe X-\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON4);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_x_pos) {
                uprintf("IQS9150 - Swipe X+\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON5);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_y_neg) {
                uprintf("IQS9150 - Swipe Y-\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON6);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_y_pos) {
                uprintf("IQS9150 - Swipe Y+\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON3);
                ignore_movement = true;
            } else if (base_data.gestures.scroll) {
                uprintf("IQS9150 - Scroll\n");
                // For scroll, use the gesture coordinates
                temp_report.h = CONSTRAIN_HID(base_data.x / 100); // Scale down
                temp_report.v = CONSTRAIN_HID(base_data.y / 100); // Scale down
            } else if (base_data.gestures.zoom) {
                uprintf("IQS9150 - Zoom\n");
                if (base_data.x < 0) {
                    temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON7);
                } else if (base_data.x > 0) {
                    temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON8);
                }
            }

            // Handle trackpad movement
            if (base_data.num_contacts == 1 && !ignore_movement) {
                temp_report.x = CONSTRAIN_HID_XY(base_data.x);
                temp_report.y = CONSTRAIN_HID_XY(base_data.y);
            }

        } else {
            uprintf("IQS9150 - Get report failed, i2c status: %d\n", status);
        }
    } else {
        uprintf("IQS9150 - Init failed, i2c status: %d\n", azoteq_iqs9150_init_status);
    }

    return temp_report;
}
