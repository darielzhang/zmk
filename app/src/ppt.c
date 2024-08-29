/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/usb.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/ppt.h>
#include <zmk/event_manager.h>
#include <zmk/ppt/keyboard_ppt_app.h>
#include "trace.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static uint8_t *get_keyboard_report(size_t *len) {

    struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    *len = sizeof(*report);
    return (uint8_t *)report;
}

static int zmk_ppt_send_report(uint8_t *report, size_t len) {
    // for(uint8_t i=0; i<len; i++)
    // {
    //     DBG_DIRECT("ppt send data:0x%x", report[i]);
    // }
    int err = ppt_app_send_data(SYNC_MSG_TYPE_INFINITE_RETRANS, 0, report, len);
    return err;
}

int zmk_ppt_send_keyboard_report(void) {
    size_t len;
    uint8_t *report = get_keyboard_report(&len);
    DBG_DIRECT("zmk ppt send keyboard data, len is %d", len);

    uint8_t opcode = SYNC_OPCODE_KEYBOARD;
    uint8_t ppt_report[len + 1];
    ppt_report[0] = opcode;
    memcpy(&ppt_report[1], report, len);
    return zmk_ppt_send_report(ppt_report, (len + 1));
}

int zmk_ppt_send_consumer_report(void) {
    struct zmk_hid_consumer_report *report = zmk_hid_get_consumer_report();
    uint16_t len = sizeof(*report);
    DBG_DIRECT("zmk ppt send consumer data, len is %d", len);

    uint8_t opcode = SYNC_OPCODE_CONSUMER;
    uint8_t ppt_report[len + 1];
    ppt_report[0] = opcode;
    memcpy(&ppt_report[1], report, len);
    return zmk_ppt_send_report(ppt_report, (len + 1));
}

#if FEATURE_SUPPORT_2_4G_FAST_KEYSTROKE_PROCESS
void zmk_rtk_ppt_key_handler(uint32_t row, uint32_t column, bool pressed) {
    int32_t position = zmk_matrix_transform_row_column_to_position(row, column);

    if (position < 0) {
        LOG_WRN("Not found in transform: row: %d, col: %d, pressed: %s", row, column,
                (pressed ? "true" : "false"));
    }

    // LOG_DBG("Row: %d, col: %d, position: %d, pressed: %s", row, column, position,
    //         (pressed ? "true" : "false"));

	struct zmk_position_state_changed zmk_position = {.source = 
                                                          ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                                      .state = pressed,
                                                      .position = position,
                                                      .timestamp = k_uptime_get()};
    zmk_keymap_position_state_changed(zmk_position.source, zmk_position.position,
                                      zmk_position.state, zmk_position.timestamp);
}
#endif