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
/* PURPOSE: led indication of various ongoing processes and states
*/

#define ZB_TRACE_FILE_ID 63620
#include "zboss_api.h"
#include "se_indication.h"
#include "zb_led_button.h"

#define COMMISSIONING_BLINK_ARG ZB_LED_ARG_CREATE(LED_GREEN, ZB_LED_BLINK_PER_SEC)
#define COMMISSIONING_BLINK_JOINED_ARG ZB_LED_ARG_CREATE(LED_GREEN, ZB_LED_BLINK_QUATER_SEC)
#define PERMIT_JOINING_SET_ARG ZB_LED_ARG_CREATE(LED_GREEN, ZB_LED_BLINK_PER_SEC)

static void zb_se_clear_leds(zb_uint8_t param);
static void zb_se_clear_blinks(zb_uint8_t param);

void zb_se_start_nvram_erase_indication(void)
{
  zb_osif_led_on(LED_RED);
}

void zb_se_stop_nvram_erase_indication(void)
{
  /* wait 1 second more so that the indication is sure to be noticed */
  ZB_SCHEDULE_ALARM(zb_se_clear_leds, 0, 1 * ZB_TIME_ONE_SECOND);
}

void zb_se_indicate_commissioning_started(void)
{
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(COMMISSIONING_BLINK_ARG);
#endif
}

void zb_se_indicate_service_discovery_started(void)
{
#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(COMMISSIONING_BLINK_ARG);
  zb_led_blink_on(COMMISSIONING_BLINK_JOINED_ARG);
#endif
  ZB_SCHEDULE_ALARM_CANCEL(zb_se_clear_leds, 0);
}

void zb_se_indicate_default_start(void)
{
  /* turn on green light for 5 seconds */

#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(COMMISSIONING_BLINK_JOINED_ARG);
  zb_led_blink_off(COMMISSIONING_BLINK_ARG);
#endif
  ZB_SCHEDULE_ALARM_CANCEL(zb_se_clear_leds, 0);

  zb_osif_led_on(LED_GREEN);
  ZB_SCHEDULE_ALARM(zb_se_clear_leds, 0, 5*ZB_TIME_ONE_SECOND);
}

void zb_se_indicate_commissioning_fail(void)
{
  /* turn on red light for 5 seconds */

  zb_se_clear_blinks(0);
  ZB_SCHEDULE_ALARM_CANCEL(zb_se_clear_leds, 0);

  zb_osif_led_on(LED_RED);
  ZB_SCHEDULE_ALARM(zb_se_clear_leds, 0, 5*ZB_TIME_ONE_SECOND);
}

void zb_se_indicate_commissioning_stopped(void)
{
  zb_se_clear_blinks(0);
  zb_se_clear_leds(0);
}

void zb_se_indicate_permit_joining_on(zb_int_t seconds)
{
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(PERMIT_JOINING_SET_ARG);
#endif
  ZB_SCHEDULE_ALARM(zb_se_clear_blinks, 0, seconds * ZB_TIME_ONE_SECOND);
}

static void zb_se_clear_blinks(zb_uint8_t param)
{
  ZVUNUSED(param);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(PERMIT_JOINING_SET_ARG);
  zb_led_blink_off(COMMISSIONING_BLINK_ARG);
  zb_led_blink_off(COMMISSIONING_BLINK_JOINED_ARG);
#endif
}

static void zb_se_clear_leds(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_osif_led_off(LED_GREEN);
  zb_osif_led_off(LED_RED);
}
