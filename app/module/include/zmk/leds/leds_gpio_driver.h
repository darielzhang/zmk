/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

#define LED_GPIO_BLINK_MS 500
#define LED_GPIO_PAIR_CNT 600 //600*50ms = 30s

#define LEDS_GPIO_NUM 4
#define LED_CAPSLOCK  0
#define LED_NUMLOCK   1
#define LED_PPT       2
#define LED_BT        3

typedef struct gpio_leds_global_data {
    bool led_flag;
    struct gpio_dt_spec *led;
    uint8_t blink_cnt;

}T_GPIO_LEDS_GLOBAL_DATA;

void led_blink_start(uint8_t led_index, uint8_t cnt);
void led_blink_exit(void);
void gpio_led_on(uint8_t led_index);
void gpio_led_off(uint8_t led_index);

#define LED_ON(led_index) gpio_led_on(led_index)
#define LED_OFF(led_index) gpio_led_off(led_index)
#define LED_BLINK(led_index, cnt) led_blink_start(led_index, cnt)
#define LED_BLINK_EXIT()    led_blink_exit()