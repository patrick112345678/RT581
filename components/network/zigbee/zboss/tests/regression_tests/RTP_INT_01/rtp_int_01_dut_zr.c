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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_INT_01_DUT_ZR

#define ZB_TRACE_FILE_ID 40309
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_int_01_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_NSNG

#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "boards.h"

#endif  /* ZB_NSNG */

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error Define ZB_STACK_REGRESSION_TESTING_API
#endif

#ifndef ZB_NSNG

#define PIN_IN  NRF_GPIO_PIN_MAP(TEST_PORT1_DUT, TEST_PIN1_DUT)
#define PIN_OUT NRF_GPIO_PIN_MAP(TEST_PORT1_DUT, TEST_PIN2_DUT)
#define PIN_OUT_DEBUG BSP_LED_0

#endif  /* ZB_NSNG */

static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

typedef enum zb_test_step_e
{
  TEST_STEP_START = 0,
  TEST_STEP_SCHED_CALLBACK,
  TEST_STEP_BUF_DELAYED,
  TEST_STEP_FINISH
} zb_test_step_t;

volatile zb_uint8_t g_int_count = 0;
volatile zb_bool_t g_int_called = ZB_FALSE;
volatile zb_test_step_t g_test_step = TEST_STEP_START;

static void test_gpio_init(void);
static void test_manager(zb_uint8_t unused);

#ifndef ZB_NSNG

static void test_pin_error_timeout_handler(zb_uint8_t unused);
static void test_toggle_out_pin(zb_uint8_t unused);
void test_in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
static void test_callback_function(zb_uint8_t param);

#endif  /* ZB_NSNG */

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_dut_zr");

  zb_set_long_address(g_ieee_addr_dut_zr);
  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    test_gpio_init();
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(test_manager, 0);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

#ifndef ZB_NSNG

static void test_gpio_init(void)
{
  ret_code_t err_code;

  TRACE_MSG(TRACE_APP1, ">> test_gpio_init", (FMT__0));

  if (!nrf_drv_gpiote_is_init())
  {
  err_code = nrf_drv_gpiote_init();

    ZB_ASSERT(err_code == NRFX_SUCCESS);
  }

    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code = nrf_drv_gpiote_out_init(PIN_OUT, &out_config);

  ZB_ASSERT(err_code == NRFX_SUCCESS);

    nrf_drv_gpiote_out_config_t out_config_deb = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code = nrf_drv_gpiote_out_init(PIN_OUT_DEBUG, &out_config_deb);

  ZB_ASSERT(err_code == NRFX_SUCCESS);

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
    err_code = nrf_drv_gpiote_in_init(PIN_IN, &in_config, test_in_pin_handler);

  ZB_ASSERT(err_code == NRFX_SUCCESS);

    nrf_drv_gpiote_in_event_enable(PIN_IN, true);

  TRACE_MSG(TRACE_APP1, "<< test_gpio_init", (FMT__0));
}

static void test_manager(zb_uint8_t unused)
{
  TRACE_MSG(TRACE_APP1, ">> test_manager", (FMT__0));

  ZB_REGRESSION_TESTS_API().sched_int_cb = test_toggle_out_pin;

  switch (g_test_step)
  {
    case TEST_STEP_START:
      /* step checks that pins on the board are connected */
      ZB_SCHEDULE_ALARM(test_pin_error_timeout_handler, 0, ZB_TIME_ONE_SECOND);
      test_toggle_out_pin(0);
      break;

    case TEST_STEP_SCHED_CALLBACK:
      /* trigger zb_schedule_callback code */
      ZB_SCHEDULE_CALLBACK(test_callback_function, 0);
      break;

    case TEST_STEP_BUF_DELAYED:
      /* trigger zb_get_buf_delayed_2param code */
      zb_buf_get_out_delayed(test_callback_function);
      break;

    default:
      TRACE_MSG(TRACE_APP1, "test_manager(): TEST_FINISHED", (FMT__0));

      ZB_REGRESSION_TESTS_API().sched_int_cb = NULL;
      break;
  }
}

static void test_pin_error_timeout_handler(zb_uint8_t unused)
{
  /* If we are here need to connect pins on the board. */

  TRACE_MSG(TRACE_ERROR, "test_pin_error_timeout_handler(): TEST FAILED", (FMT__0));
}

static void test_toggle_out_pin(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">>test_toggle_out_pin, g_int_count %d", (FMT__H, g_int_count));

  /* disable interrupt on callback, clear interrupt flag and tooglo out pin */
  ZB_REGRESSION_TESTS_API().sched_int_cb = NULL;
  g_int_called = ZB_FALSE;

  nrf_drv_gpiote_out_toggle(PIN_OUT);
  nrf_drv_gpiote_out_toggle(PIN_OUT_DEBUG);
}

void test_in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  /* Set interrupt controll flag which will be disabled in the tested code if it is executed
   * in the critical section. Also this flag is disabled in the callback function
   * [test_callback_function()] in case of pending interrupt is executed after critical section.
   */
  switch (g_test_step)
  {
    case TEST_STEP_SCHED_CALLBACK:
      ZB_REGRESSION_TESTS_API().sched_cb_int_called = ZB_TRUE;
      break;
    case TEST_STEP_BUF_DELAYED:
      ZB_REGRESSION_TESTS_API().get_buf_delayed_int_called = ZB_TRUE;
      break;
    default:
      break;
  }

  if (g_test_step == TEST_STEP_START)
  {
    ZB_SCHEDULE_ALARM_CANCEL(test_pin_error_timeout_handler, ZB_ALARM_ANY_PARAM);

    g_test_step = TEST_STEP_SCHED_CALLBACK;
    ZB_SCHEDULE_CALLBACK(test_manager, 0);
  }
  else
  {
    /* count handler executions */
    g_int_count++;
    g_int_called = ZB_TRUE;
  }
}

static void test_callback_function(zb_uint8_t param)
{
  /* Check if interrupt was triggered and schedule next test attempt */
  TRACE_MSG(TRACE_APP1, ">>test_callback_function, g_int_count %d", (FMT__H, g_int_count));

  if ((!ZB_REGRESSION_TESTS_API().sched_cb_int_called && g_test_step == TEST_STEP_SCHED_CALLBACK) ||
      (!ZB_REGRESSION_TESTS_API().get_buf_delayed_int_called && g_test_step == TEST_STEP_BUF_DELAYED))
  {
    /* Interrupt wasn't triggered at all or triggered in the critical section */
    TRACE_MSG(TRACE_APP1, "test_callback_function(): interrupt controll flag is disabled", (FMT__0));
    /* We have two test attempts on the each test step. In case of no interrupt was triggered
     * interrupt counter was not incremented or test is failed. So increment counter will cause
     * next test attempt if everything is ok or exit from this test step if error occurres.
     */
    g_int_count++;

    if (g_int_called)
    {
      TRACE_MSG(TRACE_ERROR, "test_callback_function(): TEST FAILED", (FMT__0));
    }
  }
  else
  {
    /* Interrupt was postponed and triggered after critical section */
    ZB_REGRESSION_TESTS_API().sched_cb_int_called = ZB_FALSE;
    ZB_REGRESSION_TESTS_API().get_buf_delayed_int_called = ZB_FALSE;
  }

  if (g_int_count >= TEST_DUT_MAX_ATTEMPTS)
  {
    g_int_count = 0;
    g_test_step++;
  }

  ZB_SCHEDULE_ALARM(test_manager, 0, TEST_DUT_NEXT_STEP_DELAY);

  if (param)
  {
    zb_buf_free(param);
  }
}

#else

static void test_gpio_init(void)
{
}

static void test_manager(zb_uint8_t unused)
{
  ZVUNUSED(unused);
}

#endif  /* ZB_NSNG */

/*! @} */
