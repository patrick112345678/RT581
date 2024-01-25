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

#define ZB_TRACE_FILE_ID 40150
#include "light_control.h"
#include "light_control_hal.h"
#include "zb_led_button.h"


#define BULB_LED_POVER 0
#define BULB_LED_CONNECT 1

#define BULB_BUTTON_1 ZB_BOARD_BUTTON_0
#define BULB_BUTTON_2 ZB_BOARD_BUTTON_1
#define BUTTON_0 ZB_BOARD_BUTTON_0
#define BUTTON_1 ZB_BOARD_BUTTON_1

void light_control_send_on_off(zb_uint8_t param, zb_uint16_t on_off);

void button1_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

#ifdef ZB_USE_BUTTONS
  light_control_button_pressed(LIGHT_CONTROL_BUTTON_ON);
#endif
}

void button2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

  #ifdef ZB_USE_BUTTONS
  light_control_button_pressed(LIGHT_CONTROL_BUTTON_OFF);
#endif
}


/* Private functions */
void light_control_hal_device_started()
{
  zb_osif_led_on(BULB_LED_POVER);
}

void light_control_hal_gpio_init()
{
  zb_osif_led_button_init();

  if (zb_osif_button_state(BUTTON_0))
  {
    if (zb_osif_button_state(BUTTON_1))
    {
      zb_osif_led_off(BULB_LED_POVER);
    }
  }
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(BUTTON_0, 0, button1_handler);
  zb_button_register_handler(BUTTON_1, 0, button2_handler);
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

void bulb_hal_set_connect(zb_bool_t on)
{
  if (on)
  {
    zb_osif_led_on(BULB_LED_CONNECT);
  }
  else
  {
    zb_osif_led_off(BULB_LED_CONNECT);
  }
}
