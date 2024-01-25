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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 63291
#include "izs_device.h"
#include "zb_led_button.h"

#define BUTTON_EVENT ZB_ZCL_IAS_ZONE_ZONE_STATUS_ALARM1
#define LED_BLINK_PERIOD ZB_MILLISECONDS_TO_BEACON_INTERVAL(200)
#define BUTTON_0 ZB_BOARD_BUTTON_0
#define BUTTON_1 ZB_BOARD_BUTTON_1

static void izs_hal_led_blink_on(zb_uint8_t cnt);
static void izs_hal_led_blink_off(zb_uint8_t cnt);

static void izs_generate_event(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "izs_generate_event: alarm %hd", (FMT__H, param));

  IZS_DEVICE_GET_ZONE_STATUS() |= ZB_ZCL_IAS_ZONE_ZONE_STATUS_RESTORE;
  izs_update_ias_zone_status(BUTTON_EVENT, !(IZS_DEVICE_GET_ZONE_STATUS() & BUTTON_EVENT));
}

void button1_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  izs_start_button_released();
}

void button2_handler(zb_uint8_t param)
{
  ZVUNUSED(param);

  if (ZB_JOINED())
  {
    ZB_SCHEDULE_APP_CALLBACK(izs_generate_event, 0);
  }
  else if (g_device_ctx.join_counter >= IZS_JOIN_LIMIT)
  {
    ZB_SCHEDULE_APP_CALLBACK(izs_start_join, IZS_RETRY_JOIN_ATTEMPT);
  }
}

void izs_hal_hw_init(void)
{
  zb_osif_led_button_init();
#ifdef ZB_USE_BUTTONS
  zb_button_register_handler(BUTTON_0, 0, button1_handler);
  zb_button_register_handler(BUTTON_1, 0, button2_handler);
#endif
  zb_osif_led_on(IZS_LED_POWER);
}


void izs_hal_hw_enable(void)
{
}

void izs_hal_hw_disable(void)
{
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
  zb_bool_t ret;

  ret = (zb_bool_t) zb_osif_button_state(button);
  return ret;
}

void izs_hal_power_led_on(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_osif_led_on(IZS_LED_POWER);
}

void izs_hal_power_led_off(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_osif_led_off(IZS_LED_POWER);
}

static void izs_hal_led_blink_on(zb_uint8_t cnt)
{
  izs_hal_power_led_on(0);
  ZB_SCHEDULE_APP_ALARM(izs_hal_led_blink_off, cnt, LED_BLINK_PERIOD);
}

static void izs_hal_led_blink_off(zb_uint8_t cnt)
{
  izs_hal_power_led_off(0);

  cnt--;
  if (cnt > 0)
  {
    ZB_SCHEDULE_APP_ALARM(izs_hal_led_blink_on, cnt, LED_BLINK_PERIOD);
  }
}

void izs_hal_led_blink(zb_uint8_t times)
{
  izs_hal_led_blink_on(times);
}
