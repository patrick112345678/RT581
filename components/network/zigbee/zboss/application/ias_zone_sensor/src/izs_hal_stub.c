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
/* PURPOSE: General IAS zone device HAL stub/emulation functions
*/

#define ZB_TRACE_FILE_ID 63297
#include "izs_device.h"

void izs_hal_hw_init(void)
{
  /* TODO: Init HW parts if needed. */
}


void izs_emulation_motion_alarm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "izs_emulation_motion_alarm: alarm %hd", (FMT__H, param));
  izs_update_ias_zone_status(ZB_ZCL_IAS_ZONE_ZONE_STATUS_ALARM1, param);
  ZB_SCHEDULE_APP_ALARM(izs_emulation_motion_alarm, !param, ZB_TIME_ONE_SECOND * ZB_RANDOM_VALUE(20));
}

void izs_hal_hw_enable(void)
{
  /* Start Motion alarms emulation. */
  ZB_SCHEDULE_APP_CALLBACK(izs_emulation_motion_alarm, 0);
}

void izs_hal_hw_disable(void)
{
  /* Stop Motion alarms emulation. */
  ZB_SCHEDULE_APP_ALARM_CANCEL(izs_emulation_motion_alarm, ZB_ALARM_ANY_PARAM);
}

zb_uint16_t izs_get_current_time_qsec(void)
{
  zb_uint16_t t = 0;

  TRACE_MSG(TRACE_APP1, "> izs_get_current_time_qsec", (FMT__0));
  t = ZB_TIME_QUARTERECONDS( ZB_TIMER_GET() );
  TRACE_MSG(TRACE_APP1, "< izs_get_current_time_qsec", (FMT__0));
  return t;
}

zb_bool_t izs_hal_get_button_state(zb_uint16_t button)
{
  ZVUNUSED(button);
  return ZB_FALSE;
}

void izs_hal_led_blink(zb_uint8_t times)
{
  ZVUNUSED(times);
}

void izs_hal_power_led_on(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void izs_hal_power_led_off(zb_uint8_t param)
{
  ZVUNUSED(param);
}

