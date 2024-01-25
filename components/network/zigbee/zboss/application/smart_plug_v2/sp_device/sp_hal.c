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
/* PURPOSE: Smart plug hal
*/

#define ZB_TRACE_FILE_ID 40250

#include "sp_device.h"
#include "sp_hal.h"
#include "zb_led_button.h"

#define SP_LED_ON_OFF  3
#define SP_LED_CONNECT 1
#define SP_LED_POWER   0

#define BUTTON_0 ZB_BOARD_BUTTON_0
#define BUTTON_1 ZB_BOARD_BUTTON_1
#define BUTTON_2 ZB_BOARD_BUTTON_2

#define SP_SAMPLE_BUTTONS

void button1_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void button2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void button3_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void sp_hal_device_started()
{
    zb_osif_led_on(SP_LED_POWER);
}

void sp_hal_gpio_init()
{
    zb_osif_led_button_init();

#ifdef SP_SAMPLE_BUTTONS
    if (zb_osif_button_state(BUTTON_0))
    {
        if (zb_osif_button_state(BUTTON_1))
        {
            zb_osif_led_off(SP_LED_POWER);
        }
    }
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(BUTTON_0, 0, button1_handler);
    zb_button_register_handler(BUTTON_1, 0, button2_handler);
    zb_button_register_handler(BUTTON_2, 0, button3_handler);
#endif
#endif  /* SP_SAMPLE_BUTTONS */
}

void sp_platform_init()
{
    TRACE_MSG(TRACE_APP2, "sp_platform_init", (FMT__0));

    sp_hw_init(0);
}

zb_ret_t sp_hal_init()
{
    sp_hal_gpio_init();
    sp_hal_device_started();

    return RET_OK;
}

zb_bool_t sp_get_button_state(zb_uint16_t button)
{
    zb_bool_t ret = RET_OK;

    TRACE_MSG(TRACE_APP2, "sp_get_button_state: button %d", (FMT__D, button));

#ifdef SP_SAMPLE_BUTTONS
    ret = (zb_bool_t) zb_osif_button_state(button);
#endif

    return ret;
}

void sp_relay_on_off(zb_bool_t is_on)
{
    if (is_on)
    {
        zb_osif_led_on(SP_LED_ON_OFF);
    }
    else
    {
        zb_osif_led_off(SP_LED_ON_OFF);
    }
}

void sp_update_button_state_ctx(zb_uint8_t button_state)
{
}

void sp_hal_set_connect(zb_bool_t on)
{
    if (on)
    {
        zb_osif_led_on(SP_LED_CONNECT);
    }
    else
    {
        zb_osif_led_off(SP_LED_CONNECT);
    }
}
