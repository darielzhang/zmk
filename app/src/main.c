/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zmk, CONFIG_ZMK_LOG_LEVEL);
//  #include "rtl_wdt.h"
#include "os_pm.h"
#include <zmk/matrix.h>
#include <zmk/kscan.h>
#include <zmk/display.h>
#include <zmk/mode_monitor.h>
#include <zmk/ble.h>
#include <drivers/ext_power.h>
#include "trace.h"
#include "pm.h"
#include "rtl_pinmux.h"

struct k_timer power_mode_timer;
void *timer_test = &power_mode_timer;
static void power_mode_monitor_timeout_cb(struct k_timer *timer);
//static K_TIMER_DEFINE(power_mode_timer, power_mode_monitor_timeout_cb, NULL);

#if IS_ENABLED(CONFIG_ZMK_PPT)
#include <zmk/ppt/keyboard_ppt_app.h>
#endif

#if CONFIG_PM
#include "power_manager_unit_platform.h"
enum PMCheckResult app_enter_dlps_check(void) {
    DBG_DIRECT("app check dlps flag %d", app_global_data.is_app_enabled_dlps);
    return PM_CHECK_PASS;//app_global_data.is_app_enabled_dlps ? PM_CHECK_PASS : PM_CHECK_FAIL;
}
static void app_dlps_check_cb_register(void) {
    platform_pm_register_callback_func((void *)app_enter_dlps_check, PLATFORM_PM_CHECK);
}
#endif

int main(void) {
    LOG_INF("Welcome to ZMK!\n");

#if CONFIG_PM
    app_dlps_check_cb_register();
#endif

    // if (zmk_kscan_init(DEVICE_DT_GET(ZMK_MATRIX_NODE_ID)) != 0) {
    //     return -ENOTSUP;
    // }
    DBG_DIRECT("app global mode usb %d ppt%d ble%d", app_mode.is_in_usb_mode,
    
               app_mode.is_in_ppt_mode, app_mode.is_in_bt_mode);
    // if (app_mode.is_in_bt_mode && !app_mode.is_in_usb_mode) {
    //     zmk_ble_init();
    // }
#if IS_ENABLED(CONFIG_ZMK_PPT)
    if (app_mode.is_in_ppt_mode && !app_mode.is_in_usb_mode) {
        //zmk_ppt_init();
    }
#endif
    // AON_WDT_Disable(1);
    DBG_DIRECT("[power_mode_set] set power down");
    LOG_ERR("set ic power down");
    // extern bool os_register_pm_excluded_handle_imp(void **handle, PlatformExcludedHandleType type);
	// k_timer_init(&power_mode_timer, power_mode_monitor_timeout_cb,NULL);
    // os_register_pm_excluded_handle_imp((void **)&timer_test, 0);
    // k_timer_start(&power_mode_timer, K_MSEC(20000), K_NO_WAIT);

    Pad_Config(P3_7, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE,
                   PAD_OUT_LOW);
    System_WakeUpPinEnable(P3_7, PAD_WAKEUP_POL_LOW, PAD_WAKEUP_DEB_DISABLE);

    power_mode_set(POWER_POWERDOWN_MODE);
#ifdef CONFIG_ZMK_DISPLAY
    zmk_display_init();
#endif /* CONFIG_ZMK_DISPLAY */

    return 0;
}

static void power_mode_monitor_timeout_cb(struct k_timer *timer) {
	uint32_t *refuse_reason = power_get_refuse_reason();
	PowerModeErrorCode err = power_get_error_code();
	LOG_ERR("power mode monitor:refuse reason is %x, %x, err is %d\n",*refuse_reason, refuse_reason, err);
    //printf("app register timer handle %x, timer %x",power_mode_monitor_timeout_cb, power_mode_timer);

}