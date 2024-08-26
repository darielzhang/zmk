/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/matrix_transform.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/mode_monitor.h>
#include <zmk/ppt.h>
#include <zmk/ppt/keyboard_ppt_app.h>
#include <zmk/board.h>
#include "reset_reason.h"

#define ZMK_KSCAN_EVENT_STATE_PRESSED 0
#define ZMK_KSCAN_EVENT_STATE_RELEASED 1

struct zmk_kscan_event {
    uint32_t row;
    uint32_t column;
    uint32_t state;
};

struct zmk_kscan_msg_processor {
    struct k_work work;
} msg_processor;

K_MSGQ_DEFINE(zmk_kscan_msgq, sizeof(struct zmk_kscan_event), CONFIG_ZMK_KSCAN_EVENT_QUEUE_SIZE, 4);

static uint8_t key_press_num = 0;

#include "rtl_pinmux.h"
#include "trace.h"
#include "pm.h"
// #include "rtl_wdt.h"
static void zmk_kscan_callback(const struct device *dev, uint32_t row, uint32_t column,
                               bool pressed) {
    LOG_DBG("keyscan callback: row,col is (%d %d)", row, column);
    /* cpu will check dlps status modules by modules in idle task, in this case interrupt is
       disabled when the scan interval of keyscan is set <100us, it may result in unstable interrupt
       intervals in this case we can skip check before wfi/dlps
     */
    // if(row == 3 && column == 1)
    // {
    //     //AON_WDT_Disable(1);
    //     extern void WDG_SystemReset(WDTMode_TypeDef wdt_mode,
    //                         int reset_reason);		   
	//     WDG_SystemReset(RESET_ALL, 0x88);
    // }
    if (pressed) {
        key_press_num++;
        if (app_mode.is_in_usb_mode) {
            pm_no_check_status_before_enter_wfi();
        }
    } else {
        key_press_num--;
        if (key_press_num == 0) {
            pm_check_status_before_enter_wfi_or_dlps();
        }
    }
    struct zmk_kscan_event ev = {
        .row = row,
        .column = column,
        .state = (pressed ? ZMK_KSCAN_EVENT_STATE_PRESSED : ZMK_KSCAN_EVENT_STATE_RELEASED)};

    if(zmk_ppt_is_ready()) {
#if FEATURE_SUPPORT_2_4G_FAST_KEYSTROKE_PROCESS
        zmk_rtk_ppt_key_handler(row, column, pressed);
        return;
#endif
    }
        k_msgq_put(&zmk_kscan_msgq, &ev, K_NO_WAIT);
        k_work_submit(&msg_processor.work);
}

void zmk_kscan_process_msgq(struct k_work *item) {
    struct zmk_kscan_event ev;

    while (k_msgq_get(&zmk_kscan_msgq, &ev, K_NO_WAIT) == 0) {
        bool pressed = (ev.state == ZMK_KSCAN_EVENT_STATE_PRESSED);
        int32_t position = zmk_matrix_transform_row_column_to_position(ev.row, ev.column);

        if (position < 0) {
            LOG_WRN("Not found in transform: row: %d, col: %d, pressed: %s", ev.row, ev.column,
                    (pressed ? "true" : "false"));
            continue;
        }

        LOG_DBG("Row: %d, col: %d, position: %d, pressed: %s", ev.row, ev.column, position,
                (pressed ? "true" : "false"));
        raise_zmk_position_state_changed(
            (struct zmk_position_state_changed){.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                                .state = pressed,
                                                .position = position,
                                                .timestamp = k_uptime_get()});
    }
}

int zmk_kscan_init(const struct device *dev) {
    if (dev == NULL) {
        LOG_ERR("Failed to get the KSCAN device");
        return -EINVAL;
    }

    k_work_init(&msg_processor.work, zmk_kscan_process_msgq);

    kscan_config(dev, zmk_kscan_callback);
    kscan_enable_callback(dev);

    return 0;
}