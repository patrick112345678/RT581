/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: osif layer for buttons & leds support for cc2538
*/

#define ZB_TRACE_FILE_ID 8002
#include "zb_common.h"
#include "zb_osif.h"
#include "zb_led_button.h"
/*  //JJ
#include "bsp.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "interrupt.h"          // Access to IntMasterEnable function
*/
#ifdef ZB_LEDS_MASK

/*! \addtogroup ZB_DEBUG */
/*! @{ */

extern void init_RT570_LED(void);
extern void init_RT570_KEY(void);

void zb_osif_led_button_init(void)
{
    init_RT570_LED();
    init_RT570_KEY();
#if 0 //JJ
    bspLedInit();
    bspKeyInit(BSP_KEY_MODE_ISR);
    bspKeyIntRegister(BSP_KEY_ALL, &cc2538_key_isr);
    bspKeyIntEnable(BSP_KEY_ALL);
#endif
}

/**
   This callback is not used for 2538, but called by the common ZBOSS code
 */
zb_bool_t zb_setup_buttons_cb(zb_callback_t cb)
{
    static zb_uint8_t inited = 0;
    (void)cb;
    if (inited)
    {
        return ZB_FALSE;
    }
    else
    {
        inited++;
        return ZB_TRUE;
    }
}

static zb_uint_t led_no_to_led_const(zb_uint8_t led_no)
{
#if 0 //JJ
    switch (led_no)
    {
    case 0:
        return BSP_LED_1;
        break;
    case 1:
        return BSP_LED_2;
        break;
    case 2:
        return BSP_LED_3;
        break;
    case 3:
        return BSP_LED_4;
        break;
    default:
        /* probably impossible, but let's use an adequate value */
        return BSP_LED_ALL;
        break;
    }
#endif
    return 0;
}


void zb_osif_led_on(zb_uint8_t led_no)
{
    //JJ  bspLedSet(led_no_to_led_const(led_no));
}


void zb_osif_led_off(zb_uint8_t led_no)
{
    //JJ  bspLedClear(led_no_to_led_const(led_no));
}

static void cc2538_key_isr(void)
{
#if 0 //JJ
    zb_uint8_t ui8KeysPressed;

    //
    // Get bitmask of buttons pushed (clear directional keys' bitmask)
    //
    ui8KeysPressed = bspKeyPushed(BSP_KEY_ALL);

    //
    // Determine which LEDs to toggle
    //
    if (ui8KeysPressed & BSP_KEY_LEFT)
    {
        /* 2538 HW does not allow to catch button off event, so call our callbacks
         * immediately. Button press time parameter is not avaliable for
         * application, so always call zb_button_register_handler with 0 pressed_sec_pow2. */
        zb_button_on_cb(0);
        zb_button_off_cb(0);
    }
    if (ui8KeysPressed & BSP_KEY_RIGHT)
    {
        zb_button_on_cb(1);
        zb_button_off_cb(1);
    }
    if (ui8KeysPressed & BSP_KEY_UP)
    {
        zb_button_on_cb(2);
        zb_button_off_cb(2);
    }
    if (ui8KeysPressed & BSP_KEY_DOWN)
    {
        zb_button_on_cb(3);
        zb_button_off_cb(3);
    }
    if (ui8KeysPressed & BSP_KEY_SELECT)
    {
        zb_button_on_cb(4);
        zb_button_off_cb(4);
    }
#endif
}


zb_bool_t zb_osif_button_state(zb_uint8_t arg)
{
    /* can't use this function actually. TODO: check how to implement. */
    return 0;
}


void zb_osif_button_cb(zb_uint8_t arg)
{
#if 0  //JJ
    (void)arg;
    /* Now implemented by polling button state. TODO: use interrupt */
    if (zb_osif_button_state(arg))
    {
        zb_button_on_cb(0);
    }
    else
    {
        zb_button_off_cb(0);
    }
#endif
}


/*! @} */

#endif /* ZB_LEDS_MASK */
