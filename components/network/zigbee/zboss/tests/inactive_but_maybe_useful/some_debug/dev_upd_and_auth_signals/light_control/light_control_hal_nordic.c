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
/* PURPOSE: Dimmable light sample HAL
*/

#define ZB_TRACE_FILE_ID 41671
#include "light_control.h"
#include "light_control_hal.h"

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "boards.h"

#define BULB_LED_POVER BSP_BOARD_LED_0
#define BULB_LED_CONNECT BSP_BOARD_LED_1

#define BULB_BUTTON_1 BSP_BUTTON_0
#define BULB_BUTTON_2 BSP_BUTTON_1

void light_control_send_on_off(zb_uint8_t param, zb_uint16_t on_off);

void button1_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    ZVUNUSED(pin);
    ZVUNUSED(action);

#ifdef LIGHT_SAMPLE_BUTTONS
    light_control_button_pressed(LIGHT_CONTROL_BUTTON_ON);
#endif
}

void button2_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    ZVUNUSED(pin);
    ZVUNUSED(action);

#ifdef LIGHT_SAMPLE_BUTTONS
    light_control_button_pressed(LIGHT_CONTROL_BUTTON_OFF);
#endif
}



/* Private functions */
void light_control_hal_device_started()
{
    bsp_board_led_on(BULB_LED_POVER);
}

void light_control_hal_gpio_init()
{
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);

    bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
    if (bsp_board_button_state_get(bsp_board_pin_to_button_idx(BSP_BUTTON_0)))
    {
        if (bsp_board_button_state_get(bsp_board_pin_to_button_idx(BSP_BUTTON_1)))
        {
            bsp_board_led_off(BULB_LED_POVER);
        }
    }
    nrf_drv_gpiote_init();

    in_config.pull = NRF_GPIO_PIN_PULLUP;
    nrf_drv_gpiote_in_init(BSP_BUTTON_0, &in_config, button1_handler);
    nrf_drv_gpiote_in_event_enable(BSP_BUTTON_0, true);

    in_config.pull = NRF_GPIO_PIN_PULLUP;
    nrf_drv_gpiote_in_init(BSP_BUTTON_1, &in_config, button2_handler);
    nrf_drv_gpiote_in_event_enable(BSP_BUTTON_1, true);
}

/* Public interface */
void light_control_hal_init()
{
    light_control_hal_gpio_init();
    light_control_hal_device_started();
}

zb_bool_t light_control_hal_is_button_pressed(zb_uint8_t button_no)
{
    zb_bool_t ret;

    ret = (zb_bool_t) bsp_board_button_state_get(button_no);

    return ret;
}

void bulb_hal_set_connect(zb_bool_t on)
{
    if (on)
    {
        bsp_board_led_on(BULB_LED_CONNECT);
    }
    else
    {
        bsp_board_led_off(BULB_LED_CONNECT);
    }
}
