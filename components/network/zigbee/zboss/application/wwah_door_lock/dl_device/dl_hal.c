/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 40128

#include "zb_wwah_door_lock.h"
#include "zb_led_button.h"

#define DL_LED_POWER   0
#define BUTTON_0  ZB_BOARD_BUTTON_0
#define BUTTON_1  ZB_BOARD_BUTTON_1


void button1_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void button2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    zb_buf_get_out_delayed_ext(dl_send_notification, ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_LOCK, 0);
}


void dl_hal_device_started(void)
{
    zb_osif_led_on(DL_LED_POWER);
}

void dl_hal_gpio_init(void)
{
    zb_osif_led_button_init();
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(BUTTON_0, 0, button1_handler);
    zb_button_register_handler(BUTTON_1, 0, button2_handler);
#endif
}

zb_ret_t dl_hal_init(void)
{
    dl_hal_gpio_init();
    dl_hal_device_started();

    return RET_OK;
}

zb_bool_t dl_get_button_state(zb_uint16_t button)
{
    zb_bool_t ret;

    TRACE_MSG(TRACE_APP2, "dl_get_button_state: button %d", (FMT__D, button));

    ret = (zb_bool_t) zb_osif_button_state(button);

    return ret;
}

