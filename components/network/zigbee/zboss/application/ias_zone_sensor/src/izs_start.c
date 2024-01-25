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
/* PURPOSE: Routines for initial IAS Zone sensor device actions
*/

#define ZB_TRACE_FILE_ID 63298

#include "izs_device.h"
#include "zboss_api.h"

/* IZS device start states enumeration */
typedef enum izs_start_state_e
{
  IZS_START_STATE_NOT_ENROLLED,
  IZS_START_STATE_ENROLLED_BUTTON_NOT_PRESSED,
  IZS_START_STATE_ENROLLED_BUTTON_PRESSED,
  IZS_START_STATE_STARTED,
} izs_start_state_t;

static zb_uint8_t button_pressed_cnt;
static izs_start_state_t current_state;

static void izs_do_reset(void)
{
  TRACE_MSG(TRACE_APP1, ">> izs_do_reset", (FMT__0));

  zb_nvram_erase();
  zb_reset(0);

  TRACE_MSG(TRACE_APP1, "<< izs_do_reset", (FMT__0));
}

static void izs_indicate_boot(zb_uint8_t seconds)
{
  TRACE_MSG(TRACE_APP1, "indicate boot %hd sec.", (FMT__H, seconds));

  izs_hal_power_led_on(0);
  ZB_SCHEDULE_APP_ALARM(izs_hal_power_led_off, 0, ZB_TIME_ONE_SECOND * seconds);
}

static void izs_start_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, ">> izs_start_cb, %hd", (FMT__H, param));

  current_state = IZS_START_STATE_STARTED;
  izs_start_join(IZS_FIRST_JOIN_ATTEMPT);

  TRACE_MSG(TRACE_APP1, "<< izs_start_cb", (FMT__0));
}

void izs_start_button_released(void)
{
  switch (current_state)
  {
    case IZS_START_STATE_ENROLLED_BUTTON_NOT_PRESSED:
      /* Wait 10 seconds for 5 button presses
       * Reset and begin joining if 5 button presses are detected
       * */
      button_pressed_cnt++;
      TRACE_MSG(TRACE_APP1, "button pressed %hd times", (FMT__H, button_pressed_cnt));
      if (button_pressed_cnt == 5)
      {
        izs_do_reset();
      }
      break;

    case IZS_START_STATE_ENROLLED_BUTTON_PRESSED:
      /* Reset and begin joining if the tamper switch IS released
       * within this 4-second window */
      TRACE_MSG(TRACE_APP1, "button released", (FMT__0));
      izs_do_reset();
      break;
      
    default:
      break;
  }
}

void izs_device_start(zb_uint8_t param)
{
  zb_bool_t button_pressed = izs_hal_get_button_state(IZS_RESET_BUTTON);
  button_pressed_cnt = 0;
  ZVUNUSED(param);

  if (!IZS_DEVICE_IS_ENROLLED())
  {
    TRACE_MSG(TRACE_APP1, "device not enrolled, leave and find a network", (FMT__0));

    current_state = IZS_START_STATE_NOT_ENROLLED;
    if (!zb_bdb_is_factory_new())
    {
      izs_do_reset();
    }

    izs_indicate_boot(2);
    izs_start_cb(0);
  }

  if (IZS_DEVICE_IS_ENROLLED() && !button_pressed)
  {
    TRACE_MSG(TRACE_APP1, "device enrolled, button not pressed", (FMT__0));

    current_state = IZS_START_STATE_ENROLLED_BUTTON_NOT_PRESSED;
    izs_indicate_boot(2);

    /* Wait 10 seconds for 5 button presses  */
    ZB_SCHEDULE_APP_ALARM(izs_start_cb, 0, ZB_TIME_ONE_SECOND * 10);
  }

  if (IZS_DEVICE_IS_ENROLLED() && button_pressed)
  {
    TRACE_MSG(TRACE_APP1, "device enrolled, button pressed", (FMT__0));

    current_state = IZS_START_STATE_ENROLLED_BUTTON_PRESSED;
    izs_indicate_boot(4);

    /* Wait 4 seconds for button release */
    ZB_SCHEDULE_APP_ALARM(izs_start_cb, 0, ZB_TIME_ONE_SECOND * 4);
  }
}
