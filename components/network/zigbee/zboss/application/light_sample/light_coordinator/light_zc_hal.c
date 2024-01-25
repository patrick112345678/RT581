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

#define ZB_TRACE_FILE_ID 40141
#include "light_zc.h"
#include "light_zc_hal.h"

#define BULB_LED_POVER 0
#define BULB_BUTTON_2 ZB_BOARD_BUTTON_1

#ifdef ZB_USE_BUTTONS

/* Private functions */
static void light_zc_hal_device_started(void)
{
  zb_osif_led_on(BULB_LED_POVER);
}

static void light_zc_hal_gpio_init(void)
{
  zb_osif_led_button_init();
}

/* Public interface */
void light_zc_hal_init(void)
{
  volatile  zb_uint16_t i = 0;
  for (i = 16000; i>0; i--)
  {
   light_zc_hal_gpio_init();
  }
  light_zc_hal_gpio_init();

  light_zc_hal_device_started();
}

zb_bool_t light_zc_hal_is_button_pressed(zb_uint8_t button_no)
{
  zb_bool_t ret = ZB_FALSE;
  ret = (zb_bool_t) zb_osif_button_state(button_no);
  return ret;
}

#endif /* ZB_USE_BUTTONS */
