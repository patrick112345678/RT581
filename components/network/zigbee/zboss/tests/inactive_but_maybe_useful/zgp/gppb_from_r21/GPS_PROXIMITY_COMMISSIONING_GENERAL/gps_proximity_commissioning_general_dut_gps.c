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

#define ZB_TEST_NAME GPS_PROXIMITY_COMMISSIONING_GENERAL_DUT_GPS
#define ZB_TRACE_FILE_ID 41274

#include "../include/zgp_test_templates.h"
#include "test_config.h"

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

#define ENDPOINT 10

static zb_ieee_addr_t g_dut_gps_addr = DUT_GPS_IEEE_ADDR;

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,                /* 0 */
  /* STEP 1 */
  TEST_STATE_COMM_MODE_ENTER_1,      /* 1 */
  TEST_STATE_COMM_SUCC_1,            /* 2 */
  TEST_STATE_WINDOW_1,               /* 3 */
  /* STEP 2A */
  TEST_STATE_CLEAN_SINK_TABLE_2A,    /* 4 */
  TEST_STATE_COMM_MODE_ENTER_2A,     /* 5 */
  TEST_STATE_WINDOW_2A,              /* 6 */
  /* STEP 2B */
  TEST_STATE_CLEAN_SINK_TABLE_2B,    /* 7 */
  TEST_STATE_COMM_MODE_ENTER_2B,     /* 8 */
  TEST_STATE_WINDOW_2B,              /* 9 */
  /* STEP 2C */
  TEST_STATE_CLEAN_SINK_TABLE_2C,    /* 10 */
  TEST_STATE_COMM_MODE_ENTER_2C,     /* 11 */
  TEST_STATE_WINDOW_2C,              /* 12 */
  /* STEP 2D */
  TEST_STATE_CLEAN_SINK_TABLE_2D,    /* 13 */
  TEST_STATE_COMM_MODE_ENTER_2D,     /* 14 */
  TEST_STATE_WINDOW_2D,              /* 15 */
  /* STEP 2E */
  TEST_STATE_CLEAN_SINK_TABLE_2E,    /* 16 */
  TEST_STATE_COMM_MODE_ENTER_2E,     /* 17 */
  TEST_STATE_WINDOW_2E,              /* 18 */
  /* STEP 3A */
  TEST_STATE_CLEAN_SINK_TABLE_3A,    /* 19 */
  TEST_STATE_COMM_MODE_ENTER_3A,     /* 20 */
  TEST_STATE_WINDOW_3A,              /* 21 */
  /* STEP 3B */
  TEST_STATE_CLEAN_SINK_TABLE_3B,    /* 22 */
  TEST_STATE_COMM_MODE_ENTER_3B,     /* 23 */
  TEST_STATE_WINDOW_3B,              /* 24 */
  /* STEP 3C */
  TEST_STATE_CLEAN_SINK_TABLE_3C,    /* 25 */
  TEST_STATE_COMM_MODE_ENTER_3C,     /* 26 */
  TEST_STATE_WINDOW_3C,              /* 27 */
  /* STEP 3D */
  TEST_STATE_CLEAN_SINK_TABLE_3D,    /* 28 */
  TEST_STATE_COMM_MODE_ENTER_3D,     /* 29 */
  TEST_STATE_WINDOW_3D,              /* 30 */
  /* STEP 3E */
  TEST_STATE_CLEAN_SINK_TABLE_3E,    /* 31 */
  TEST_STATE_COMM_MODE_ENTER_3E,     /* 32 */
  TEST_STATE_WINDOW_3E,              /* 33 */
  /* STEP 4A */
  TEST_STATE_CLEAN_SINK_TABLE_4A,    /* 34 */
  TEST_STATE_COMM_MODE_ENTER_4A,     /* 35 */
  TEST_STATE_WINDOW_4A,              /* 36 */
  /* STEP 4B */
  TEST_STATE_CLEAN_SINK_TABLE_4B,    /* 37 */
  TEST_STATE_COMM_MODE_ENTER_4B,     /* 38 */
  TEST_STATE_WINDOW_4B,              /* 39 */
  /* STEP 5 */
  TEST_STATE_COMM_MODE_ENTER_5,      /* 40 */
  TEST_STATE_COMM_SUCC_5,            /* 41 */
  TEST_STATE_WINDOW_5,               /* 42 */
  /* STEP 6 */
  TEST_STATE_CLEAN_SINK_TABLE_6,     /* 43 */
  TEST_STATE_COMM_MODE_ENTER_6,      /* 44 */
  TEST_STATE_COMM_SUCC_6,            /* 45 */
  TEST_STATE_WINDOW_6,               /* 46 */
  /* STEP 7 */
  TEST_STATE_CLEAN_SINK_TABLE_7,     /* 47 */
  TEST_STATE_COMM_MODE_ENTER_7,      /* 48 */
  TEST_STATE_COMM_SUCC_7,            /* 49 */
  /* FINISH */
  TEST_STATE_FINISHED                /* 50 */
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
    case TEST_STATE_COMM_SUCC_1:
    case TEST_STATE_COMM_SUCC_5:
    case TEST_STATE_COMM_SUCC_6:
    case TEST_STATE_COMM_SUCC_7:
      /* Waiting for commissioning complete */
      break;

    case TEST_STATE_COMM_MODE_ENTER_1:
    case TEST_STATE_COMM_MODE_ENTER_2A:
    case TEST_STATE_COMM_MODE_ENTER_2B:
    case TEST_STATE_COMM_MODE_ENTER_2C:
    case TEST_STATE_COMM_MODE_ENTER_2D:
    case TEST_STATE_COMM_MODE_ENTER_2E:
    case TEST_STATE_COMM_MODE_ENTER_3A:
    case TEST_STATE_COMM_MODE_ENTER_3B:
    case TEST_STATE_COMM_MODE_ENTER_3C:
    case TEST_STATE_COMM_MODE_ENTER_3D:
    case TEST_STATE_COMM_MODE_ENTER_3E:
    case TEST_STATE_COMM_MODE_ENTER_4A:
    case TEST_STATE_COMM_MODE_ENTER_4B:
    case TEST_STATE_COMM_MODE_ENTER_5:
    case TEST_STATE_COMM_MODE_ENTER_6:
    case TEST_STATE_COMM_MODE_ENTER_7:
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
      zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_WINDOW_1:
    case TEST_STATE_WINDOW_2A:
    case TEST_STATE_WINDOW_2B:
    case TEST_STATE_WINDOW_2C:
    case TEST_STATE_WINDOW_2D:
    case TEST_STATE_WINDOW_2E:
    case TEST_STATE_WINDOW_3A:
    case TEST_STATE_WINDOW_3B:
    case TEST_STATE_WINDOW_3C:
    case TEST_STATE_WINDOW_3D:
    case TEST_STATE_WINDOW_3E:
    case TEST_STATE_WINDOW_4A:
    case TEST_STATE_WINDOW_4B:
    case TEST_STATE_WINDOW_5:
    case TEST_STATE_WINDOW_6:
      ZB_ZGPC_SET_PAUSE(DUT_ACTIONS_WINDOW);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_CLEAN_SINK_TABLE_2A:
    case TEST_STATE_CLEAN_SINK_TABLE_2B:
    case TEST_STATE_CLEAN_SINK_TABLE_3A:
    case TEST_STATE_CLEAN_SINK_TABLE_3B:
    case TEST_STATE_CLEAN_SINK_TABLE_3C:
    case TEST_STATE_CLEAN_SINK_TABLE_3D:
    case TEST_STATE_CLEAN_SINK_TABLE_3E:
    case TEST_STATE_CLEAN_SINK_TABLE_4A:
    case TEST_STATE_CLEAN_SINK_TABLE_4B:
    case TEST_STATE_CLEAN_SINK_TABLE_6:
    case TEST_STATE_CLEAN_SINK_TABLE_7:
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;

    case TEST_STATE_CLEAN_SINK_TABLE_2C:
      ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_TRUE, /* Protection with GP Link Key */
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_CLEAN_SINK_TABLE_2D:
      ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
      zgp_tbl_clear();
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    case TEST_STATE_CLEAN_SINK_TABLE_2E:
      ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
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
      case TEST_STATE_COMM_SUCC_1:
      case TEST_STATE_COMM_SUCC_5:
      case TEST_STATE_COMM_SUCC_6:
      case TEST_STATE_COMM_SUCC_7:
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
  ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_CWE_OR_PS ;
  ZGP_GPS_COMMISSIONING_WINDOW = DUT_COMM_WINDOW;


  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;
  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;
  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
