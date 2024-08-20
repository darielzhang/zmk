/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

enum zmk_ppt_conn_state {
    ZMK_PPT_CONN_NONE,
    ZMK_PPT_CONN,
};

int zmk_ppt_send_keyboard_report(void);
int zmk_ppt_send_consumer_report(void);

void zmk_rtk_ppt_key_handler(uint32_t row, uint32_t column, bool pressed);
