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
/* PURPOSE: Blinking LEDs. Upper layer (using scheduler to implement blinking)
*/

#define ZB_TRACE_FILE_ID 2148
#include "zboss_api_core.h"
/*! \addtogroup ZB_DEBUG */
/*! @{ */

#ifdef ZB_USE_BUTTONS

#include "zb_common.h"
#include "zb_led_button.h"

void zb_led_blink_on_cb(zb_uint8_t led_arg);
void zb_led_blink_off_cb(zb_uint8_t led_arg);

void zb_led_blink_off(zb_uint8_t led_arg)
{
    ZB_SCHEDULE_ALARM_CANCEL(zb_led_blink_on_cb, led_arg);
    ZB_SCHEDULE_ALARM_CANCEL(zb_led_blink_off_cb, led_arg);
    zb_osif_led_off(ZB_LED_ARG_NUMBER(led_arg));
}


void zb_led_blink_on(zb_uint8_t led_arg)
{
    zb_led_blink_off(ZB_LED_ARG_NUMBER(led_arg));
    /* 09/18/2015 CR [AS] Block start. */
    /*
      ZB_SCHEDULE_CALLBACK(zb_led_blink_on_cb, led_arg);
    */
    zb_led_blink_on_cb(led_arg);
    /* 09/18/2015 CR [AS] end */
}


void zb_led_blink_on_cb(zb_uint8_t led_arg)
{
    zb_osif_led_on(ZB_LED_ARG_NUMBER(led_arg));
    ZB_SCHEDULE_ALARM(zb_led_blink_off_cb, led_arg, ZB_LED_ARG_PERIOD((zb_uint32_t)led_arg));
}


void zb_led_blink_off_cb(zb_uint8_t led_arg)
{
    zb_osif_led_off(ZB_LED_ARG_NUMBER(led_arg));
    ZB_SCHEDULE_ALARM(zb_led_blink_on_cb, led_arg, ZB_LED_ARG_PERIOD((zb_uint32_t)led_arg));
}


void zb_button_on_cb(zb_uint8_t butt_no)
{
    if (butt_no >= ZB_N_BUTTONS)
    {
        return;
    }

    if (ZG->button.buttons[butt_no].is_on == 0U)
    {
        ZG->button.buttons[butt_no].is_on = ZB_TRUE;
        ZG->button.buttons[butt_no].on_time = ZB_TIMER_GET();
    }
}


void zb_button_off_cb(zb_uint8_t butt_no)
{

    if (butt_no >= ZB_N_BUTTONS)
    {
        return;
    }

    if (ZG->button.buttons[butt_no].is_on)
    {
        zb_time_t to = ZB_TIMER_GET();
        zb_uint_t i;

        ZG->button.buttons[butt_no].is_on = ZB_FALSE;
        to = ZB_TIME_SUBTRACT(to, ZG->button.buttons[butt_no].on_time);
        to /= ZB_TIME_ONE_SECOND;
        for (i = ZB_BUTT_N_CBS - 1U ; i > 0U && ((1UL << i) > to) ; --i)
        {
        }
        while (ZG->button.buttons[butt_no].handlers[i] == NULL
                && i > 0U)
        {
            i--;
        }
        if (ZG->button.buttons[butt_no].handlers[i] != NULL)
        {
            ZB_SCHEDULE_CALLBACK(ZG->button.buttons[butt_no].handlers[i], 0);
        }
    }

}


void zb_button_register_handler(zb_uint8_t butt_no, zb_uint8_t pressed_sec_pow2, zb_callback_t cb)
{
    if (butt_no >= ZB_N_BUTTONS)
    {
        return;
    }

    /* [VS] I am not sure zb_osif_led_button_init() call is needed: application itself initializes
       peripheral devices if needed. For example, LCGW app calls zb_osif_led_button_init() on startup.
       [EES] move it's intialisation code into gw_hw_init_after_zb_init() funtion

       [NK] Pretty sure it is needed! See onoff_server application for example.
       It is very bad code style to use ZB_USE_BUTTONS define mixed with some HAL code in
       application. ZB_USE_BUTTONS originally means that leds/buttons are implemented generally in
       stack, inited automatically, and works via special api. So there are 2 options here:
       - if you want some app-specific HAL, get rid of this API and use app-specific calls (as it is
       done in light_sample application),
       - if you want to use general API for leds/buttons, move peripherals init to osif and use it as-is
       (as it is done in onoff_server application).
    */

    if (zb_setup_buttons_cb(zb_osif_button_cb))
    {
        zb_osif_led_button_init();
    }
    if (pressed_sec_pow2 >= ZB_BUTT_N_CBS)
    {
        pressed_sec_pow2 = ZB_BUTT_N_CBS - 1U;
    }
    ZG->button.buttons[butt_no].handlers[pressed_sec_pow2] = cb;
}

#endif  /* ZB_USE_BUTTONS */

/*! @} */
