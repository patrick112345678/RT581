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
/* PURPOSE: Light ZC sample HAL
*/

#define ZB_TRACE_FILE_ID 40149
#include "light_zc.h"
#include "light_zc_hal.h"

/* LED 0 */
#define BULB_LED_POVER 0
#define BULB_BUTTON_2 ZB_BOARD_BUTTON_1

/* Private functions */
void light_zc_hal_device_started()
{
    zb_osif_led_on(BULB_LED_POVER);
}

void light_zc_hal_gpio_init()
{
    zb_osif_led_button_init();
}

/* Public interface */
void light_zc_hal_init()
{
    volatile  zb_uint16_t i = 0;
    for (i = 16000; i > 0; i--)
    {
        light_zc_hal_gpio_init();
    }
    light_zc_hal_gpio_init();

    light_zc_hal_device_started();
}

zb_bool_t light_zc_hal_is_button_pressed()
{
    zb_bool_t ret = ZB_FALSE;
    ret = (zb_bool_t) zb_osif_button_state(BULB_BUTTON_2);
    return ret;
}
