#include "azoteq_iqs9150.h"
#include "pointing_device_internal.h"
#include "wait.h"

// Forward declaration for pointing device driver
report_mouse_t iqs9150_get_report(report_mouse_t mouse_report);

// Global configuration
iqs9150_config_t iqs9150_config = {
    .total_rxs    = 0,
    .total_txs    = 0,
    .x_resolution = 0,
    .y_resolution = 0,
    .flip_x       = false,
    .flip_y       = false,
    .swap_xy      = false,
};

static bool iqs9150_initialized = false;

const pointing_device_driver_t azoteq_iqs9150_pointing_device_driver = {
    .init       = iqs9150_init,
    .get_report = iqs9150_get_report,
};

void iqs9150_init(void) {
    i2c_init();
    i2c_ping_address(IQS9150_I2C_ADDRESS, 1); // wake
    wait_ms(100); // Wait for device power-up

    // Verify device communication
    uint8_t data[2];
    if (!iqs9150_read_register(IQS9150_PRODUCT_NUM, data, 2)) {
#ifdef CONSOLE_ENABLE
        pd_dprintf("IQS9150: Communication failed\n");
#endif
        return false;
    }

    uint16_t product_num = (data[1] << 8) | data[0];
#ifdef CONSOLE_ENABLE
    pd_dprintf("IQS9150: Product ID: 0x%04X\n", product_num);
#endif

    // Store configuration
    iqs9150_config.total_rxs = rxs;
    iqs9150_config.total_txs = txs;
    iqs9150_config.x_resolution = x_res;
    iqs9150_config.y_resolution = y_res;

    // Configure trackpad size
    if (!iqs9150_write_register16(IQS9150_TOTAL_RXS, rxs) ||
        !iqs9150_write_register16(IQS9150_TOTAL_TXS, txs) ||
        !iqs9150_write_register16(IQS9150_X_RESOLUTION, x_res) ||
        !iqs9150_write_register16(IQS9150_Y_RESOLUTION, y_res)) {
#ifdef CONSOLE_ENABLE
        pd_dprintf("IQS9150: Configuration failed\n");
#endif
        return false;
    }

    // Acknowledge any reset
    // iqs9150_ack_reset();

    iqs9150_initialized = true;
#ifdef CONSOLE_ENABLE
    pd_dprintf("IQS9150: Initialized %dx%d trackpad, resolution %dx%d\n", rxs, txs, x_res, y_res);
#endif
    return true;
}


bool iqs9150_get_data(iqs9150_data_t *data) {
    if (!data || !iqs9150_initialized) {
        return false;
    }

    uint8_t buffer[12];

    // Read basic data packet
    if (!iqs9150_read_register(IQS9150_RELATIVE_X, buffer, 12)) {
        return false;
    }

    data->relative_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    data->relative_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    data->gesture_x = (int16_t)((buffer[5] << 8) | buffer[4]);
    data->gesture_y = (int16_t)((buffer[7] << 8) | buffer[6]);
    data->single_gestures = (uint16_t)((buffer[9] << 8) | buffer[8]);
    data->two_gestures = (uint16_t)((buffer[11] << 8) | buffer[10]);

    // Read status flags
    if (!iqs9150_read_register(IQS9150_INFO_FLAGS, buffer, 4)) {
        return false;
    }
    data->info_flags = (uint16_t)((buffer[1] << 8) | buffer[0]);
    data->trackpad_flags = (uint16_t)((buffer[3] << 8) | buffer[2]);

    // Read button states
    if (!iqs9150_read_register(IQS9150_BUTTON_OUTPUT, buffer, 2)) {
        return false;
    }
    data->button_output = (uint16_t)((buffer[1] << 8) | buffer[0]);

    // Extract touch count
    data->touch_count = (uint8_t)(data->trackpad_flags & 0x0F);

    return true;
}

report_mouse_t iqs9150_get_report(report_mouse_t mouse_report) {
    report_mouse_t out = (report_mouse_t){0};

    if (!iqs9150_initialized) {
        return out;
    }

    iqs9150_data_t data = {0};
    if (!iqs9150_get_data(&data)) {
#ifdef CONSOLE_ENABLE
        pd_dprintf("IQS9150: get_data failed\n");
#endif
        return out;
    }

    bool ignore_movement = false;

    // Map gestures to buttons similar to IQS5xx driver
    if (data.single_gestures & (IQS9150_GESTURE_SINGLE_TAP | IQS9150_GESTURE_PRESS_HOLD)) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON1);
    } else if (data.single_gestures & IQS9150_GESTURE_SWIPE_X_MINUS) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON4);
        ignore_movement = true;
    } else if (data.single_gestures & IQS9150_GESTURE_SWIPE_X_PLUS) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON5);
        ignore_movement = true;
    } else if (data.single_gestures & IQS9150_GESTURE_SWIPE_Y_MINUS) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON6);
        ignore_movement = true;
    } else if (data.single_gestures & IQS9150_GESTURE_SWIPE_Y_PLUS) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON3);
        ignore_movement = true;
    }

    // Optional: map physical button outputs (0:left,1:right,2:middle) if available
    if (iqs9150_is_button_pressed(&data, 0)) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON1);
    }
    if (iqs9150_is_button_pressed(&data, 1)) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON2);
    }
    if (iqs9150_is_button_pressed(&data, 2)) {
        out.buttons = pointing_device_handle_buttons(out.buttons, true, POINTING_DEVICE_BUTTON3);
    }

    // Too many fingers? ignore movement
    if (data.trackpad_flags & IQS9150_TP_TOO_MANY_FINGERS) {
        ignore_movement = true;
    }

    // Determine movement or scroll based on touch count
    if (!ignore_movement) {
        if (data.touch_count >= 2) {
            // Use gesture deltas for scrolling with two fingers
            int16_t gh = data.gesture_x;
            int16_t gv = data.gesture_y;
            if (iqs9150_config.swap_xy) {
                int16_t t = gh; gh = gv; gv = t;
            }
            if (iqs9150_config.flip_x) gh = -gh;
            if (iqs9150_config.flip_y) gv = -gv;
            out.h = CONSTRAIN_HID(gh);
            out.v = CONSTRAIN_HID(gv);
        } else if ((data.trackpad_flags & IQS9150_TP_MOVEMENT_DETECTED) && data.touch_count == 1) {
            int16_t dx = data.relative_x;
            int16_t dy = data.relative_y;
            if (iqs9150_config.swap_xy) {
                int16_t t = dx; dx = dy; dy = t;
            }
            if (iqs9150_config.flip_x) dx = -dx;
            if (iqs9150_config.flip_y) dy = -dy;
            out.x = CONSTRAIN_HID_XY(dx);
            out.y = CONSTRAIN_HID_XY(dy);
        }
    }

    return out;
}

void iqs9150_set_resolution(void) {
    if (!iqs9150_initialized) return false;

    bool success = iqs9150_write_register16(IQS9150_X_RESOLUTION, x_res) &&
                   iqs9150_write_register16(IQS9150_Y_RESOLUTION, y_res);

    if (success) {
        iqs9150_config.x_resolution = x_res;
        iqs9150_config.y_resolution = y_res;
    }

    return success;
}

bool iqs9150_set_orientation(bool flip_x, bool flip_y, bool swap_xy) {
    if (!iqs9150_initialized) return false;

    // Read current trackpad settings
    uint8_t current_settings;
    if (!iqs9150_read_register(0x11E2, &current_settings, 1)) {
        return false;
    }

    // Modify orientation bits (bits 0-2)
    current_settings &= ~0x07; // Clear orientation bits

    if (flip_x) current_settings |= (1 << 0);
    if (flip_y) current_settings |= (1 << 1);
    if (swap_xy) current_settings |= (1 << 2);

    bool success = iqs9150_write_register(0x11E2, &current_settings, 1);

    if (success) {
        iqs9150_config.flip_x = flip_x;
        iqs9150_config.flip_y = flip_y;
        iqs9150_config.swap_xy = swap_xy;
    }

    return success;
}

uint8_t iqs9150_get_gesture(iqs9150_data_t *data) {
    if (!data) return 0;
    return data->single_gestures;
}

bool iqs9150_is_touch_active(iqs9150_data_t *data) {
    if (!data) return false;
    return (data->info_flags & IQS9150_INFO_GLOBAL_TP_TOUCH) != 0;
}

bool iqs9150_is_button_pressed(iqs9150_data_t *data, uint8_t button) {
    if (!data || button > 15) return false;
    return (data->button_output & (1 << button)) != 0;
}

// Low-level I2C functions
bool iqs9150_read_register(uint16_t address, uint8_t *data, uint16_t length) {
    // Use QMK helper for 16-bit big-endian register access
    i2c_status_t status = i2c_read_register16(IQS9150_I2C_ADDRESS, address, data, length, IQS9150_I2C_TIMEOUT);
    return status == I2C_STATUS_SUCCESS;
}

bool iqs9150_write_register(uint16_t address, uint8_t *data, uint16_t length) {
    // Use QMK helper for 16-bit big-endian register access
    i2c_status_t status = i2c_write_register16(IQS9150_I2C_ADDRESS, address, data, length, IQS9150_I2C_TIMEOUT);
    return status == I2C_STATUS_SUCCESS;
}

bool iqs9150_write_register16(uint16_t address, uint16_t data) {
    uint8_t buffer[2] = {data & 0xFF, data >> 8};
    return iqs9150_write_register(address, buffer, 2);
}

// static bool iqs9150_ack_reset(void) {
//     uint8_t data[2];

//     // Read current system control
//     if (!iqs9150_read_register(IQS9150_SYSTEM_CONTROL, data, 2)) {
//         return false;
//     }

//     // Set ACK Reset bit (bit 7)
//     data[0] |= (1 << 7);

//     return iqs9150_write_register(IQS9150_SYSTEM_CONTROL, data, 2);
// }
