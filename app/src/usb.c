/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static enum usb_dc_status_code usb_status = USB_DC_UNKNOWN;

static void raise_usb_status_changed_event(struct k_work *_work) {
    ZMK_EVENT_RAISE(new_zmk_usb_conn_state_changed(
        (struct zmk_usb_conn_state_changed){.conn_state = zmk_usb_get_conn_state()}));
}

K_WORK_DEFINE(usb_status_notifier_work, raise_usb_status_changed_event);

enum usb_dc_status_code zmk_usb_get_status() { return usb_status; }

enum zmk_usb_conn_state zmk_usb_get_conn_state() {
    LOG_DBG("state: %d", usb_status);
    switch (usb_status) {
    case USB_DC_SUSPEND:
    case USB_DC_CONFIGURED:
        return ZMK_USB_CONN_HID;

    case USB_DC_DISCONNECTED:
    case USB_DC_UNKNOWN:
        return ZMK_USB_CONN_NONE;

    default:
        return ZMK_USB_CONN_POWERED;
    }
}

void usb_status_cb(enum usb_dc_status_code status, const uint8_t *params) {
    usb_status = status;
    k_work_submit(&usb_status_notifier_work);
};

static int zmk_usb_init(const struct device *_arg) {
    int usb_enable_ret;

    usb_enable_ret = usb_enable(usb_status_cb);

    if (usb_enable_ret != 0) {
        LOG_ERR("Unable to enable USB");
        return -EINVAL;
    }

    return 0;
}

SYS_INIT(zmk_usb_init, APPLICATION, CONFIG_ZMK_USB_INIT_PRIORITY);
