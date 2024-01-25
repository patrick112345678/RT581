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
/* PURPOSE: HAL stubs for SP device
*/


#define ZB_TRACE_FILE_ID 63267
#include "sp_device.h"

void sp_platform_init()
{
  TRACE_MSG(TRACE_APP2, "sp_platform_init", (FMT__0));
  sp_hw_init(0);
}

zb_ret_t sp_hal_init()
{
  return RET_OK;
}

zb_bool_t sp_get_button_state(zb_uint16_t button)
{
  TRACE_MSG(TRACE_APP2, "sp_get_button_state: button %d", (FMT__D, button));
  return ZB_FALSE;
}

void sp_update_metering_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP2, "sp_update_metering_data stub", (FMT__0));
  ZB_SCHEDULE_APP_ALARM(sp_update_metering_data, 0, ZB_TIME_ONE_SECOND * 30);
}

void sp_update_button_state_ctx(zb_uint8_t button_state)
{
  TRACE_MSG(TRACE_APP2, "sp_update_button_state_ctx stub: state %hd", (FMT__H, button_state));
}

void sp_relay_on_off(zb_bool_t is_on)
{
  if (is_on)
  {
    if (g_dev_ctx.led_state == SP_LED_NORMAL)
    {
      SP_LED_OFF();
    }
  }
  else
  {
    if (g_dev_ctx.led_state == SP_LED_NORMAL)
    {
      SP_LED_ON();
    }
  }
}
