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

#define ZB_TRACE_FILE_ID 40191
#include "zboss_api.h"
#include "light_controller_hal.h"
#include "zb_led_button.h"

#define BUTTON_1 ZB_BOARD_BUTTON_1
#define BUTTON_2 ZB_BOARD_BUTTON_2
#define BUTTON_3 ZB_BOARD_BUTTON_3


void start_touchlink_commissioning(zb_uint8_t param);
void send_on_off_cmd(zb_uint8_t param);
void send_move_to_level_cmd(zb_uint8_t param);

/* button2 - start touchlink */
void button2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    start_touchlink_commissioning(0);
}

/* button3 - on/off */
void button3_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    send_on_off_cmd(0);
}

/* button4 - level control */
void button4_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    send_move_to_level_cmd(0);
}

/* Private functions */
void light_control_hal_device_started()
{
}

void light_control_led_on_off(zb_uint8_t led_idx, zb_uint8_t on_state)
{
    if (on_state)
    {
        zb_osif_led_on(led_idx);
    }
    else
    {
        zb_osif_led_off(led_idx);
    }
}

void light_control_hal_gpio_init()
{
    zb_osif_led_button_init();
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(BUTTON_1, 0, button2_handler);
    zb_button_register_handler(BUTTON_2, 0, button3_handler);
    zb_button_register_handler(BUTTON_3, 0, button4_handler);
#endif
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

    ret = (zb_bool_t) zb_osif_button_state(button_no);

    return ret;
}
