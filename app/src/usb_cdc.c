/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample echo app for CDC ACM class
 *
 * Sample app for USB CDC ACM class driver. The received data is echoed back
 * to the serial port.
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
#include <zmk/mp_test/single_tone.h>
#include "mem_types.h"
#include "trace.h"
LOG_MODULE_REGISTER(usb_cdc, CONFIG_ZMK_LOG_LEVEL);

static void interrupt_handler(const struct device *dev, void *user_data) {
	LOG_DBG("usb cdc interrupt handler");
	ARG_UNUSED(user_data);
	int recv_len, rb_len;
	uint8_t buffer[64];
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {

			size_t len = sizeof(buffer);
			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				recv_len = 0;
			}
			uart_irq_tx_enable(dev);
		}
		if (uart_irq_tx_ready(dev)) {
			if(buffer[0] == 0x14 && buffer[1] == 0x56 && buffer[2] == 0x43) {
				mp_test_command_handler(&buffer[0], recv_len);
			} else {
				extern void *(*os_mem_alloc_intern)(RAM_TYPE ram_type, size_t size,
                        const char *p_func, uint32_t file_line);
				uint8_t *p_buf = os_mem_alloc_intern(RAM_TYPE_DATA_ON,
                                        recv_len,__func__,__LINE__);
				memcpy(p_buf, &buffer[0], recv_len);

				extern bool (*vhci_send)(void *p_buf, uint32_t len);
        		vhci_send((uint8_t *)p_buf, recv_len);
				uart_irq_tx_disable(dev);
			}
		}
	}
}

int usb_cdc_init(void) {
	const struct device *dev;
	uint32_t baudrate, dtr = 0U;
	int ret;

	dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		LOG_ERR("CDC ACM device not ready");
		return 0;
	}

	LOG_INF("usb cdc init");

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	vhci_init();
	return 0;
}

SYS_INIT(usb_cdc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);