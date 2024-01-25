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
/* PURPOSE: DUT GPS
*/

#define ZB_TEST_NAME GPS_DTCLK_INVOLVE_TC_SUB_FIELD_CLEARED_DUT_GPS

#define ZB_TRACE_FILE_ID 63520
#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT DUT_ENDPOINT

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_ieee_addr_t g_dut_gps_addr = DUT_GPS_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_ic[16+2] = SETUP_IC;

static void start_comm(zb_uint32_t timeout);
static void switch_working_mode(zb_bool_t skip_gpdf, zb_bool_t use_proxies);
static void schedule_delay(zb_uint32_t timeout);
static void sink_table_req_handler_cb(zb_uint8_t buf_ref);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_WAIT_1,
  /* STEP 2A */
  TEST_STATE_START_COMM_2A,
  TEST_STATE_WAIT_SINK_TBL_REQ_2A,
  TEST_STATE_WAIT_2A,
  /* STEP 2B */
  TEST_STATE_CLEAR_ST_2B,
  TEST_STATE_START_COMM_2B,
  TEST_STATE_WAIT_SINK_TBL_REQ_2B,
  TEST_STATE_WAIT_2B,
  /* STEP 3A */
  TEST_STATE_CLEAR_ST_3A,
  TEST_STATE_START_COMM_STUB_3A,
  TEST_STATE_WAIT_SINK_TBL_REQ_3A,
  TEST_STATE_WAIT_3A,
  /* STEP 3B */
  TEST_STATE_CLEAR_ST_3B,
  TEST_STATE_START_COMM_STUB_3B,
  TEST_STATE_WAIT_SINK_TBL_REQ_3B,
  TEST_STATE_WAIT_3B,
  /* STEP 4 */
  TEST_STATE_CLEAR_ST_4,
  TEST_STATE_WAIT_4,
  /* FINISH */
  TEST_STATE_FINISHED
};


ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(buf_ref);
  ZVUNUSED(cb);
}

static void perform_next_state(zb_uint8_t param)
{
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  TRACE_MSG(TRACE_APP1, ">perform_next_state: state = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_SINK_TBL_REQ_2A:
    case TEST_STATE_WAIT_SINK_TBL_REQ_2B:
    case TEST_STATE_WAIT_SINK_TBL_REQ_3A:
    case TEST_STATE_WAIT_SINK_TBL_REQ_3B:
    {
      TRACE_MSG(TRACE_APP1, "Waiting for Sink Table request from TH", (FMT__0));
      break;
    }
    case TEST_STATE_WAIT_1:
    {
      schedule_delay(TEST_STEP1_DUT_DELAY);
      break;
    }
    case TEST_STATE_WAIT_2A:
    {
      schedule_delay(TEST_STEP2A_DUT_DELAY);
      break;
    }
    case TEST_STATE_WAIT_2B:
    {
      schedule_delay(TEST_STEP2B_DUT_DELAY);
      break;
    }
    case TEST_STATE_WAIT_3A:
    {
      schedule_delay(TEST_STEP3A_DUT_DELAY);
      break;
    }
    case TEST_STATE_WAIT_3B:
    {
      schedule_delay(TEST_STEP3B_DUT_DELAY);
      break;
    }
    case TEST_STATE_WAIT_4:
    {
      schedule_delay(TEST_STEP4_DUT_DELAY);
      break;
    }

    case TEST_STATE_START_COMM_2A:
    {
      start_comm(100);
      switch_working_mode(ZB_FALSE, ZB_FALSE);
      break;
    }
    case TEST_STATE_START_COMM_2B:
    {
      start_comm(100);
      switch_working_mode(ZB_TRUE, ZB_TRUE);
      break;
    }
    case TEST_STATE_START_COMM_STUB_3A:
    {
      switch_working_mode(ZB_FALSE, ZB_FALSE);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    }
    case TEST_STATE_START_COMM_STUB_3B:
    {
      switch_working_mode(ZB_TRUE, ZB_TRUE);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    }

    case TEST_STATE_CLEAR_ST_2B:
    case TEST_STATE_CLEAR_ST_3A:
    case TEST_STATE_CLEAR_ST_3B:
    case TEST_STATE_CLEAR_ST_4:
    {
      TRACE_MSG(TRACE_APP1, "CLEAR SINK TABLE", (FMT__0));
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    }

    case TEST_STATE_FINISHED:
    {
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    }
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }

  TRACE_MSG(TRACE_APP1, "<perform_next_state", (FMT__0));
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/

static void start_comm(zb_uint32_t timeout)
{
  zb_zgps_start_commissioning(timeout * ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
  ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

static void switch_working_mode(zb_bool_t skip_gpdf, zb_bool_t use_proxies)
{
  ZVUNUSED(skip_gpdf);
  ZVUNUSED(use_proxies);
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = skip_gpdf;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = use_proxies;
#endif
}

static void schedule_delay(zb_uint32_t timeout)
{
  ZB_ZGPC_SET_PAUSE(timeout);
  ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

static void sink_table_req_handler_cb(zb_uint8_t buf_ref)
{
  TRACE_MSG(TRACE_APP1, ">sink_table_req_handler_cb: buf_ref = %d, test_state = %d",
            (FMT__D_D, buf_ref, TEST_DEVICE_CTX.test_state));

  ZVUNUSED(buf_ref);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_SINK_TBL_REQ_2A:
    case TEST_STATE_WAIT_SINK_TBL_REQ_2B:
    case TEST_STATE_WAIT_SINK_TBL_REQ_3A:
    case TEST_STATE_WAIT_SINK_TBL_REQ_3B:
    {
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    }
  }

  TRACE_MSG(TRACE_APP1, "<sink_table_req_handler_cb", (FMT__0));
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("dut_gps");

  zb_set_long_address(g_dut_gps_addr);
  zb_set_network_router_role(1l << TEST_CHANNEL);

  /* Need to block GPDF recv directly */
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_TRUE /* ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY */,
                             ZB_TRUE /* ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC */);

  ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
  TEST_DEVICE_CTX.gp_sink_tbl_req_cb = sink_table_req_handler_cb;

  ZB_BDB().bdb_join_uses_install_code_key = 1;
  zb_secur_ic_set(g_ic);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
