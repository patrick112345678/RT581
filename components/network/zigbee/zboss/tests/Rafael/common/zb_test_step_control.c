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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 40694

#include "zb_common.h"
#include "zb_test_step.h"
//#include "zb_test_nrf52_led_button.h"

#define BUTTON0 0
#define BUTTON1 1

#define LED0 0
#define LED1 1

typedef struct zb_test_control_ctx_s
{
  zb_uint8_t steps_num;
  zb_uint8_t step_idx;
  zb_callback_t steps_action_cb;
  zb_test_control_mode_t used_mode;
} zb_test_control_ctx_t;

static zb_test_control_ctx_t test_control_ctx;

static void test_control_handle_steps(zb_uint8_t unused);
static void test_control_left_button_handler(zb_uint8_t unused);
static void test_control_step_action(zb_uint8_t unused);

zb_ret_t test_control_force_next(void)
{
  zb_test_step_info_t *step_info;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_APS1, ">> test_control_force_next", (FMT__0));

  if (test_control_ctx.step_idx < test_control_ctx.steps_num)
  {
    if (test_control_ctx.steps_action_cb)
    {
      ZB_SCHEDULE_CALLBACK(test_control_ctx.steps_action_cb, 0);
    }

    step_info = test_step_get_info(test_control_ctx.step_idx);
    ZB_SCHEDULE_CALLBACK(step_info->callback, step_info->param);
    test_control_ctx.step_idx++;
  }
  else
  {
    ret = RET_ERROR;
  }

  TRACE_MSG(TRACE_APS1, "<< test_control_force_next: ret %hd", (FMT__H, ret));
  return ret;
}

zb_ret_t test_control_force_step(zb_uint8_t idx)
{
  zb_test_step_info_t *step_info;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_APS1, ">> test_control_force_step", (FMT__0));

  if (idx < test_control_ctx.steps_num)
  {
    if (test_control_ctx.steps_action_cb)
    {
      ZB_SCHEDULE_CALLBACK(test_control_ctx.steps_action_cb, 0);
    }

    test_control_ctx.step_idx = idx;
    step_info = test_step_get_info(idx);
    ZB_SCHEDULE_CALLBACK(step_info->callback, step_info->param);
  }
  else
  {
    ret = RET_ERROR;
  }

  TRACE_MSG(TRACE_APS1, "<< test_control_force_step: ret %hd", (FMT__H, ret));
  return ret;
}

void test_control_set_action_on_steps(zb_callback_t callback)
{
  test_control_ctx.steps_action_cb = callback;
}

void test_control_start(zb_test_control_mode_t method, zb_uint32_t timeout)
{
  TRACE_MSG(TRACE_APS1, ">> test_control_start", (FMT__0));

  test_control_ctx.steps_num = test_step_count();
  test_control_ctx.step_idx = 0;
  test_control_ctx.steps_action_cb = NULL;
  test_control_ctx.used_mode = method;

  //JJ zb_osif_led_button_init(test_control_left_button_handler);

  switch (test_control_ctx.used_mode)
  {
    case ZB_TEST_CONTROL_ALARMS:
      ZB_SCHEDULE_ALARM(test_control_handle_steps, 0, timeout);
      break;
    case ZB_TEST_CONTROL_BUTTONS:
      zb_osif_led_on(LED0);
      test_control_set_action_on_steps(test_control_step_action);
      break;
    default:
      break;
  }

  TRACE_MSG(TRACE_APS1, "<< test_control_start", (FMT__0));
}

static void test_control_handle_steps(zb_uint8_t unused)
{
  zb_ret_t step_res;
  zb_test_step_info_t *step_info;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APS1, ">> test_control_handle_steps", (FMT__0));

  switch (test_control_ctx.used_mode)
  {
    case ZB_TEST_CONTROL_ALARMS:
      step_res = test_control_force_step(test_control_ctx.step_idx);
      if (step_res == RET_OK)
      {
        step_info = test_step_get_info(test_control_ctx.step_idx);
        ZB_SCHEDULE_ALARM(test_control_handle_steps, 0, step_info->step_time);
        test_control_ctx.step_idx++;
      }
      break;
    case ZB_TEST_CONTROL_BUTTONS:
      step_res = test_control_force_next();
      break;
    default:
      step_res = RET_ERROR;
      break;
  }

  if (step_res != RET_OK)
  {
    TRACE_MSG(TRACE_APS1, "test_control_handle_steps: test finished", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "<< test_control_handle_steps", (FMT__0));
}

static void test_control_left_button_handler(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  if (test_control_ctx.used_mode != ZB_TEST_CONTROL_BUTTONS)
  {
    test_control_ctx.used_mode = ZB_TEST_CONTROL_BUTTONS;
    zb_osif_led_on(LED0);
    test_control_set_action_on_steps(test_control_step_action);
    ZB_SCHEDULE_ALARM_CANCEL(test_control_handle_steps, 0);
  }
  else
  {
    test_control_handle_steps(0);
  }
}

static void test_control_step_action(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_osif_led_on(LED1);
  ZB_SCHEDULE_ALARM(zb_osif_led_off, LED1, ZB_TIME_ONE_SECOND);
}
