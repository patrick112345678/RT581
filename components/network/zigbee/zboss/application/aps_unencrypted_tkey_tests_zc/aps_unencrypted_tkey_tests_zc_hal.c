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
/* PURPOSE: APS Unencrypted Transport Key feature coordinator HAL
*/

#define ZB_TRACE_FILE_ID 40138
#include "aps_unencrypted_tkey_tests_zc.h"
#include "aps_unencrypted_tkey_tests_zc_hal.h"
#include "zb_led_button.h"

#define NVRAM_ERASE_BUTTON 2
#define SHIFT_TO_APS_LED 2

#define LED_0 0
#define LED_1 1
#define LED_2 2
#define LED_3 3

#define BUTTON_0 ZB_BOARD_BUTTON_0
#define BUTTON_1 ZB_BOARD_BUTTON_1
#define BUTTON_2 ZB_BOARD_BUTTON_2
#define BUTTON_3 ZB_BOARD_BUTTON_3


zb_uint8_t aps_unencrypted_tkey_zc_device_joined_indication()
{
    zb_uint8_t i;
    zb_uint32_t leds[] = {LED_0, LED_1, LED_2, LED_3};
    for (i = 0; i < ZB_N_LEDS && zb_osif_led_state(leds[i]); i++);
    if (i < ZB_N_LEDS)
    {
        zb_osif_led_on(leds[i]);
    }
    else
    {
        i = 0xFF;
    }
    return i;
}

void aps_unencrypted_tkey_zc_device_leaved_indication(zb_uint8_t led_idx)
{
    zb_uint32_t leds[] = {LED_0, LED_1, LED_2, LED_3};
    if (led_idx >= ZB_N_LEDS)
    {
        ZB_ASSERT(0);
    }
    else
    {
        zb_osif_led_off(leds[led_idx]);
    }
}

void aps_unencrypted_tkey_zc_device_message_indication(zb_uint8_t led_idx)
{
    zb_uint32_t leds[] = {LED_0, LED_1, LED_2, LED_3};
    led_idx += SHIFT_TO_APS_LED;
    if (led_idx >= ZB_N_LEDS)
    {
        ZB_ASSERT(0);
    }
    else
    {
        zb_osif_led_toggle(leds[led_idx]);
    }
}

void button1_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    aps_unencrypted_tkey_tests_zc_button_pressed(APS_TKEY_SECURITY_BUTTON);
}

void button2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    aps_unencrypted_tkey_tests_zc_button_pressed(ZDO_LEAVE_BUTTON);
}

void button3_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    aps_unencrypted_tkey_tests_zc_button_pressed(SEND_TO_FIRST_DEVICE);
}

void button4_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    aps_unencrypted_tkey_tests_zc_button_pressed(SEND_TO_SECOND_DEVICE);
}

void aps_unencrypted_tkey_zc_hal_gpio_init()
{
    zb_osif_led_button_init();
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(BUTTON_0, 0, button1_handler);
    zb_button_register_handler(BUTTON_1, 0, button2_handler);
    zb_button_register_handler(BUTTON_2, 0, button3_handler);
    zb_button_register_handler(BUTTON_3, 0, button4_handler);
#endif
}

/* Public interface */
void aps_unencrypted_tkey_zc_hal_init()
{
    aps_unencrypted_tkey_zc_hal_gpio_init();
}

zb_bool_t aps_unencrypted_tkey_zc_hal_is_button_pressed()
{
    zb_bool_t ret = ZB_FALSE;
    ret = (zb_bool_t) zb_osif_button_state(NVRAM_ERASE_BUTTON);
    return ret;
}
