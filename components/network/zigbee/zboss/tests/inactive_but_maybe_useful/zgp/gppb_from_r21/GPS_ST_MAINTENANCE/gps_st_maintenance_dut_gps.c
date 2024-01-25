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
/* PURPOSE: DUT gps
*/

#define ZB_TEST_NAME GPS_ST_MAINTENANCE_DUT_GPS
#define ZB_TRACE_FILE_ID 41492

#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT 10

static zb_ieee_addr_t g_dut_gps_addr = DUT_GPS_IEEE_ADDR;

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result);
static void start_comm(zb_uint32_t timeout);
/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,                /* 0 */
  /* INIT */
  TEST_STATE_INIT_COMM_MODE,         /* 1 */
  TEST_STATE_INIT_COMM_SUCC,         /* 2 */
  /* STEP 2 */
  TEST_STATE_COMM_MODE_2,            /* 3 */
  TEST_STATE_WINDOW_2,               /* 4 */
  /* STEP 3 */
  TEST_STATE_PRE_COMM_MODE_3,        /* 5 */
  TEST_STATE_PRE_COMM_SUCC_3,        /* 6 */
  TEST_STATE_WINDOW_3,               /* 7 */
  /* STEP 4A, 4B and 4C */
  TEST_STATE_PRE_COMM_MODE_4,        /* 8 */
  TEST_STATE_PRE_COMM_SUCC_4,        /* 9 */
  TEST_STATE_COMM_MODE_4,            /* 10 */
  TEST_STATE_WINDOW_4,               /* 11 */
  /* STEP 5 */
  TEST_STATE_PRE_CLEAN_SINK_TABLE_5, /* 12 */
  TEST_STATE_PRE_COMM_MODE_5_NE,     /* 13 */
  TEST_STATE_PRE_COMM_SUCC_5_NE,     /* 14 */
  TEST_STATE_PRE_COMM_MODE_5_NF,     /* 15 */
  TEST_STATE_PRE_COMM_SUCC_5_NF,     /* 16 */
  TEST_STATE_COMM_MODE_5,            /* 17 */
  TEST_STATE_WINDOW_5,               /* 18 */
  /* STEP 6 */
  TEST_STATE_PRE_CLEAN_SINK_TABLE_6, /* 19 */
  TEST_STATE_PRE_COMM_MODE_6_NE,     /* 20 */
  TEST_STATE_PRE_COMM_SUCC_6_NE,     /* 21 */
  TEST_STATE_PRE_COMM_MODE_6_NF,     /* 22 */
  TEST_STATE_PRE_COMM_SUCC_6_NF,     /* 23 */
  TEST_STATE_COMM_MODE_6,            /* 24 */
  TEST_STATE_WINDOW_6,               /* 25 */
  /* STEP 7 */
  TEST_STATE_PRE_COMM_MODE_7_NE,     /* 26 */
  TEST_STATE_PRE_COMM_SUCC_7_NE,     /* 27 */
  TEST_STATE_COMM_MODE_7,            /* 28 */
  TEST_STATE_WINDOW_7,               /* 29 */
  /* FINISH */
  TEST_STATE_FINISHED                /* 30 */
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 4000)

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
    case TEST_STATE_INIT_COMM_SUCC:
    case TEST_STATE_PRE_COMM_SUCC_3:
    case TEST_STATE_PRE_COMM_SUCC_4:
    case TEST_STATE_PRE_COMM_SUCC_5_NE:
    case TEST_STATE_PRE_COMM_SUCC_5_NF:
    case TEST_STATE_PRE_COMM_SUCC_6_NE:
    case TEST_STATE_PRE_COMM_SUCC_6_NF:
    case TEST_STATE_PRE_COMM_SUCC_7_NE:
      /* Waiting for commissioning complete */
      break;
    case TEST_STATE_INIT_COMM_MODE:
    case TEST_STATE_COMM_MODE_2:
    case TEST_STATE_PRE_COMM_MODE_3:
    case TEST_STATE_PRE_COMM_MODE_4:
      start_comm(ZGP_GPS_GET_COMMISSIONING_WINDOW());
      break;

    case TEST_STATE_COMM_MODE_4:
      ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION;
      start_comm(25);
      break;

    case TEST_STATE_PRE_COMM_MODE_5_NE:
    case TEST_STATE_PRE_COMM_MODE_5_NF:
    case TEST_STATE_COMM_MODE_5:
    case TEST_STATE_PRE_COMM_MODE_6_NE:
    case TEST_STATE_PRE_COMM_MODE_6_NF:
    case TEST_STATE_COMM_MODE_6:
    case TEST_STATE_PRE_COMM_MODE_7_NE:
    case TEST_STATE_COMM_MODE_7:
      start_comm(ZGP_GPS_GET_COMMISSIONING_WINDOW());
      break;

    case TEST_STATE_WINDOW_2:
    case TEST_STATE_WINDOW_3:
    case TEST_STATE_WINDOW_4:
    case TEST_STATE_WINDOW_5:
    case TEST_STATE_WINDOW_6:
      ZB_ZGPC_SET_PAUSE(DUT_ACTIONS_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_PRE_CLEAN_SINK_TABLE_5:
    case TEST_STATE_PRE_CLEAN_SINK_TABLE_6:
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
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

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  zb_bool_t ret = ZB_FALSE;

  ZVUNUSED(zgpd_id);

  TRACE_MSG(TRACE_APP1, "<custom_comm_cb: state = %d, result = %d",
            (FMT__D, TEST_DEVICE_CTX.test_state, result));

  if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
  {
    switch (TEST_DEVICE_CTX.test_state)
    {
      case TEST_STATE_INIT_COMM_SUCC:
      case TEST_STATE_PRE_COMM_SUCC_3:
      case TEST_STATE_PRE_COMM_SUCC_4:
        ret = ZB_TRUE;
#ifdef ZB_USE_BUTTONS
        zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        break;

      case TEST_STATE_PRE_COMM_SUCC_5_NE:
      case TEST_STATE_PRE_COMM_SUCC_5_NF:
      case TEST_STATE_PRE_COMM_SUCC_6_NE:
      case TEST_STATE_PRE_COMM_SUCC_6_NF:
      case TEST_STATE_PRE_COMM_SUCC_7_NE:
        ZB_ZGPC_SET_PAUSE(2);
        ret = ZB_TRUE;
#ifdef ZB_USE_BUTTONS
        zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        break;
    }

  }

  TRACE_MSG(TRACE_APP1, ">custom_comm_cb", (FMT__0));

  return ret;
}


static void start_comm(zb_uint32_t timeout)
{
  zb_zgps_start_commissioning(timeout * ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
  ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("dut_gps");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_dut_gps_addr);
  zb_set_default_ffd_descriptor_values(ZB_ROUTER);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);
  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
