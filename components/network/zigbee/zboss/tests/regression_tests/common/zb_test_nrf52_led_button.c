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
/* PURPOSE: LEDs and buttons low-level routines for nRF52
*/

#define ZB_TRACE_FILE_ID 63629

#include "zb_common.h"
#include "zb_test_nrf52_led_button.h"

#ifndef ZB_NSNG
#define ZB_USE_BUTTONS
#endif

#ifdef ZB_USE_BUTTONS

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "boards.h"

#define LED0 BSP_BOARD_LED_0
#define LED1 BSP_BOARD_LED_1
#define LED2 BSP_BOARD_LED_2
#define LED3 BSP_BOARD_LED_3

#define BUTTON0 BSP_BUTTON_0
#define BUTTON1 BSP_BUTTON_1
#define BUTTON2 BSP_BUTTON_2
#define BUTTON3 BSP_BUTTON_3

void zb_osif_led_button_init(zb_callback_t callback)
{
    static zb_uint8_t inited = 0;
    ret_code_t err_code;
    zb_uint8_t i;
    nrf_drv_gpiote_in_config_t config = NRFX_GPIOTE_RAW_CONFIG_IN_SENSE_LOTOHI(true);

    config.pull = NRF_GPIO_PIN_PULLUP;

    if (!inited)
    {
        bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);

        /* Additional check if it was initialised
           somewhere else in the tests. */
        if (!nrf_drv_gpiote_is_init())
        {
            err_code = nrf_drv_gpiote_init();

            ZB_ASSERT(err_code == NRFX_SUCCESS);
        }

        nrf_drv_gpiote_in_init(BUTTON0, &config, (nrfx_gpiote_evt_handler_t)callback);
        nrf_drv_gpiote_in_event_enable(BUTTON0, true);

        inited = 1;
    }
}

static zb_ret_t led_on_off(zb_uint8_t led_no, zb_uint8_t on)
{
    uint32_t led_idx;
    zb_ret_t ret = RET_OK;

    if (led_no == 0)
    {
        led_idx = LED0;
    }
    else if (led_no == 1)
    {
        led_idx = LED1;
    }
    else if (led_no == 2)
    {
        led_idx = LED2;
    }
    else if (led_no == 3)
    {
        led_idx = LED3;
    }
    else
    {
        ret = RET_ERROR;
    }

    if (ret == RET_OK)
    {
        if (on)
        {
            bsp_board_led_on(led_idx);
        }
        else
        {
            bsp_board_led_off(led_idx);
        }
    }

    return ret;
}

void zb_osif_led_on(zb_uint8_t led_no)
{
    led_on_off(led_no, 1);
}

void zb_osif_led_off(zb_uint8_t led_no)
{
    led_on_off(led_no, 0);
}

#else

void zb_osif_led_button_init(zb_callback_t callback)
{
    ZVUNUSED(callback);
}

static zb_ret_t led_on_off(zb_uint8_t led_no, zb_uint8_t on)
{
    ZVUNUSED(led_no);
    ZVUNUSED(on);
}

void zb_osif_led_on(zb_uint8_t led_no)
{
    ZVUNUSED(led_no);
}

void zb_osif_led_off(zb_uint8_t led_no)
{
    ZVUNUSED(led_no);
}

#endif /* ZB_USE_BUTTONS */
