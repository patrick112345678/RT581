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
/* PURPOSE: osif layer for buttons & leds
*/

#define ZB_TRACE_FILE_ID 8002
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

extern void led_hw_init(void);
extern void key_hw_init(void);

void zb_osif_led_button_init(void)
{
  led_hw_init();
  key_hw_init();
}

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
