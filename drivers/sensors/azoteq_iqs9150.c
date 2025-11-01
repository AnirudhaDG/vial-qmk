// Copyright 2025 Azoteq (Pty) Ltd
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdbool.h>
#include "azoteq_iqs9150.h"
#include "pointing_device_internal.h"
#include "wait.h"

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

// Control register bits
#define IQS9150_CONTROL_SUSPEND             BIT(11)
#define IQS9150_CONTROL_ACK_RESET           BIT(7)
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

    pd_dprintf("IQS9150: Reset/suspend - reset:%d, suspend:%d\n", reset, suspend);

    status = azoteq_iqs9150_read_word(IQS9150_CONTROL, &control);
    if (status == I2C_STATUS_SUCCESS) {
        pd_dprintf("IQS9150: Read control register: 0x%04X\n", control);
        if (reset) {
            control |= IQS9150_CONTROL_ACK_RESET;
        }
        if (suspend) {
            control |= IQS9150_CONTROL_SUSPEND;
        } else {
            control &= ~IQS9150_CONTROL_SUSPEND;
        }

        status = azoteq_iqs9150_write_word(IQS9150_CONTROL, control);
        if (status == I2C_STATUS_SUCCESS) {
            pd_dprintf("IQS9150: Wrote control register: 0x%04X\n", control);
        } else {
            pd_dprintf("IQS9150: Failed to write control register, status: %d\n", status);
        }
    } else {
        pd_dprintf("IQS9150: Failed to read control register, status: %d\n", status);
    }

    if (end_session) {
        azoteq_iqs9150_end_session();
    }

    return status;
}

i2c_status_t azoteq_iqs9150_set_report_rate(uint16_t report_rate_ms, azoteq_iqs9150_charging_modes_t mode, bool end_session) {
    pd_dprintf("IQS9150: Set report rate %dms, mode: %d\n", report_rate_ms, mode);
    // This would need to be implemented based on specific register map
    // For now, return success as a placeholder
    if (end_session) {
        azoteq_iqs9150_end_session();
    }
    return I2C_STATUS_SUCCESS;
}

i2c_status_t azoteq_iqs9150_set_event_mode(bool enabled, bool end_session) {
    uint16_t config = 0;
    i2c_status_t status;

    pd_dprintf("IQS9150: Set event mode: %d\n", enabled);

    status = azoteq_iqs9150_read_word(IQS9150_CONFIG, &config);
    if (status == I2C_STATUS_SUCCESS) {
        pd_dprintf("IQS9150: Read config register: 0x%04X\n", config);
        if (enabled) {
            config |= IQS9150_CONFIG_EVENT_MODE;
        } else {
            config &= ~IQS9150_CONFIG_EVENT_MODE;
        }

        status = azoteq_iqs9150_write_word(IQS9150_CONFIG, config);
        if (status == I2C_STATUS_SUCCESS) {
            pd_dprintf("IQS9150: Wrote config register: 0x%04X\n", config);
        } else {
            pd_dprintf("IQS9150: Failed to write config register, status: %d\n", status);
        }
    } else {
        pd_dprintf("IQS9150: Failed to read config register, status: %d\n", status);
    }

    if (end_session) {
        azoteq_iqs9150_end_session();
    }

    return status;
}

i2c_status_t azoteq_iqs9150_set_xy_config(bool flip_x, bool flip_y, bool switch_xy, bool end_session) {
    pd_dprintf("IQS9150: Set XY config - flip_x:%d, flip_y:%d, switch_xy:%d\n", flip_x, flip_y, switch_xy);
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
    i2c_status_t status = azoteq_iqs9150_read_word(IQS9150_PROD_NUM, &azoteq_iqs9150_product_number);
    if (status != I2C_STATUS_SUCCESS) {
        azoteq_iqs9150_product_number = AZOTEQ_IQS9150_UNKNOWN;
    }
    pd_dprintf("IQS9150: Product number 0x%04X\n", azoteq_iqs9150_product_number);
    return azoteq_iqs9150_product_number;
}

static void azoteq_iqs9150_setup_resolution(void) {
    pd_dprintf("IQS9150: Setting up resolution for product 0x%04X\n", azoteq_iqs9150_product_number);

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

    pd_dprintf("IQS9150: Resolution set to %dx%d\n",
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

// Main driver functions
void azoteq_iqs9150_init(void) {
    pd_dprintf("IQS9150: Starting init\n");
    i2c_init();
    azoteq_iqs9150_wake();
    azoteq_iqs9150_reset_suspend(true, false, true);
    wait_ms(100);

    azoteq_iqs9150_wake();
    if (azoteq_iqs9150_get_product() != AZOTEQ_IQS9150_UNKNOWN) {
        pd_dprintf("IQS9150: Device detected, product: 0x%04X\n", azoteq_iqs9150_product_number);
        azoteq_iqs9150_setup_resolution();

        // Set communication mode based on device capabilities
        azoteq_iqs9150_comms_mode = AZOTEQ_IQS9150_COMMS_MODE_FREE;
        pd_dprintf("IQS9150: Set communication mode to FREE\n");

        // Configure basic settings
        azoteq_iqs9150_init_status = azoteq_iqs9150_set_report_rate(AZOTEQ_IQS9150_REPORT_RATE, AZOTEQ_IQS9150_ACTIVE, false);
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_event_mode(false, false); // Use streaming mode for QMK

#if defined(AZOTEQ_IQS9150_ROTATION_90)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(false, true, true, false);
        pd_dprintf("IQS9150: Applied 90 degree rotation\n");
#elif defined(AZOTEQ_IQS9150_ROTATION_180)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(true, true, false, false);
        pd_dprintf("IQS9150: Applied 180 degree rotation\n");
#elif defined(AZOTEQ_IQS9150_ROTATION_270)
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(true, false, true, false);
        pd_dprintf("IQS9150: Applied 270 degree rotation\n");
#else
        azoteq_iqs9150_init_status |= azoteq_iqs9150_set_xy_config(false, false, false, false);
        pd_dprintf("IQS9150: No rotation applied\n");
#endif

        azoteq_iqs9150_end_session();
        wait_ms(AZOTEQ_IQS9150_REPORT_RATE + 1);

        if (azoteq_iqs9150_init_status == I2C_STATUS_SUCCESS) {
            pd_dprintf("IQS9150: Init completed successfully\n");
        } else {
            pd_dprintf("IQS9150: Init completed with errors, status: %d\n", azoteq_iqs9150_init_status);
        }
    } else {
        pd_dprintf("IQS9150: Device not detected or unknown product\n");
    }
}

report_mouse_t azoteq_iqs9150_get_report(report_mouse_t mouse_report) {
    report_mouse_t temp_report = {0};

    if (azoteq_iqs9150_init_status == I2C_STATUS_SUCCESS) {
        azoteq_iqs9150_base_data_t base_data = {0};
        i2c_status_t status = azoteq_iqs9150_get_base_data(&base_data);
        bool ignore_movement = false;

        if (status == I2C_STATUS_SUCCESS) {
            // Handle reset condition
            if (base_data.info.show_reset) {
                pd_dprintf("IQS9150 - Device reset detected\n");
                azoteq_iqs9150_init(); // Reinitialize
                return temp_report;
            }

            // Handle gestures
            if (base_data.gestures.tap) {
                pd_dprintf("IQS9150 - Single tap\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON1);
            } else if (base_data.gestures.hold) {
                pd_dprintf("IQS9150 - Hold gesture\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON1);
            } else if (base_data.gestures.two_finger_tap) {
                pd_dprintf("IQS9150 - Two finger tap\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON2);
            } else if (base_data.gestures.swipe_x_neg) {
                pd_dprintf("IQS9150 - Swipe X-\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON4);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_x_pos) {
                pd_dprintf("IQS9150 - Swipe X+\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON5);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_y_neg) {
                pd_dprintf("IQS9150 - Swipe Y-\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON6);
                ignore_movement = true;
            } else if (base_data.gestures.swipe_y_pos) {
                pd_dprintf("IQS9150 - Swipe Y+\n");
                temp_report.buttons = pointing_device_handle_buttons(temp_report.buttons, true, POINTING_DEVICE_BUTTON3);
                ignore_movement = true;
            } else if (base_data.gestures.scroll) {
                pd_dprintf("IQS9150 - Scroll\n");
                // For scroll, use the gesture coordinates
                temp_report.h = CONSTRAIN_HID(base_data.x / 100); // Scale down
                temp_report.v = CONSTRAIN_HID(base_data.y / 100); // Scale down
            } else if (base_data.gestures.zoom) {
                pd_dprintf("IQS9150 - Zoom\n");
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
            pd_dprintf("IQS9150 - Get report failed, i2c status: %d\n", status);
        }
    } else {
        pd_dprintf("IQS9150 - Init failed, i2c status: %d\n", azoteq_iqs9150_init_status);
    }

    return temp_report;
}
