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

#define ZB_TRACE_FILE_ID 40217
#include "zboss_api.h"
#include "bulb_hal.h"

#ifdef ZB_USE_BUTTONS

/* Public interface */
void bulb_hal_init()
{
  zb_osif_led_button_init();
  //zb_osif_led_on(BULB_LED_POVER);
}

void bulb_hal_set_level(zb_uint8_t level)
{
  ZVUNUSED(level);
}

void bulb_hal_set_on_off(zb_bool_t on)
{
#ifndef BOARD_PCA10059 
  if (on)
  {
    zb_osif_led_on(BULB_LED_LIGHT);
  }
  else
  {
    zb_osif_led_off(BULB_LED_LIGHT);
  }
#endif
}

void bulb_hal_set_connect(zb_uint8_t on)
{
#ifndef BOARD_PCA10059 
  if (on)
  {
    zb_osif_led_on(BULB_LED_CONNECT);
  }
  else
  {
    zb_osif_led_off(BULB_LED_CONNECT);
  }
#endif
}

void bulb_hal_set_power(zb_uint8_t on)
{
#ifndef BOARD_PCA10059 
  if (on)
  {
    zb_osif_led_on(BULB_LED_POVER);
  }
  else
  {
    zb_osif_led_off(BULB_LED_POVER);
  }
#endif
}

zb_bool_t bulb_hal_is_button_pressed()
{
  zb_bool_t ret = ZB_FALSE;
  ret = (zb_bool_t) zb_osif_button_state(BULB_BUTTON);
  return ret;
}

#endif /* ZB_USE_BUTTONS */
