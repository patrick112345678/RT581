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
/* PURPOSE: TH GPD, app_id = 000
*/

#define ZB_TEST_NAME GPS_APP_SIMPLE_GENERIC_ONE_STATE_SWITCH_TH_GPD

#define ZB_TRACE_FILE_ID 63539
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

/*============================================================================*/
/*                             DECLARATIONS                                   */
/*============================================================================*/

static zb_uint32_t  g_zgpd_srcId  = TEST_ZGPD_SRC_ID;
static zb_uint8_t   g_oob_key[16] = TEST_OOB_KEY;
static zb_uint8_t   g_key_nwk[]   = NWK_KEY;

static void start_comm(zb_uint8_t comm_type);
static void schedule_delay(zb_uint32_t timeout);

/*============================================================================*/
/*                             FSM CORE                                       */
/*============================================================================*/
/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIATE,
  /* STEP 1A */
  TEST_STATE_START_AUTOCOMMISSIONING_1A,
  TEST_STATE_COMMISSIOIN_WITH_PRESS_1_OF_1_1A,
  TEST_STATE_CLEAR_AUTO_COMM_FLAG_1A,
  TEST_STATE_SHORT_DELAY1_1A,
  TEST_STATE_SEND_PRESS_1_OF_1_1A,
  TEST_STATE_SHORT_DELAY2_1A,
  TEST_STATE_SEND_RELEASE_1_OF_1_1A,
  TEST_STATE_SHORT_DELAY3_1A,
  TEST_STATE_LONG_DELAY_1A,
  /* STEP 1B */
  TEST_STATE_START_AUTOCOMMISSIONING_1B,
  TEST_STATE_COMMISSIOIN_WITH_RELEASE_1_OF_1_1B,
  TEST_STATE_CLEAR_AUTO_COMM_FLAG_1B,
  TEST_STATE_SHORT_DELAY1_1B,
  TEST_STATE_SEND_PRESS_1_OF_1_1B,
  TEST_STATE_SHORT_DELAY2_1B,
  TEST_STATE_SEND_RELEASE_1_OF_1_1B,
  TEST_STATE_SHORT_DELAY3_1B,
  TEST_STATE_LONG_DELAY_1B,
  /* STEP 2 */
  TEST_STATE_START_COMMISSIONING_2,
  TEST_STATE_SHORT_DELAY1_2,
  TEST_STATE_SEND_PRESS_1_OF_1_2,
  TEST_STATE_SHORT_DELAY2_2,
  TEST_STATE_SEND_RELEASE_1_OF_1_2,
  TEST_STATE_SHORT_DELAY3_2,
  TEST_STATE_LONG_DELAY_2,
  /* STEP 3 */
  TEST_STATE_LONG_DELAY_3,
  TEST_STATE_SEND_PRESS_1_OF_1_3,
  TEST_STATE_SHORT_DELAY_3,
  TEST_STATE_SEND_RELEASE_1_OF_1_3,
  /* FINISH */
  TEST_STATE_FINISHED
};


ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void perform_next_state(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SHORT_DELAY1_1A:
    case TEST_STATE_SHORT_DELAY2_1A:
    case TEST_STATE_SHORT_DELAY3_1A:
    case TEST_STATE_SHORT_DELAY1_1B:
    case TEST_STATE_SHORT_DELAY2_1B:
    case TEST_STATE_SHORT_DELAY3_1B:
    case TEST_STATE_SHORT_DELAY1_2:
    case TEST_STATE_SHORT_DELAY2_2:
    case TEST_STATE_SHORT_DELAY3_2:
    case TEST_STATE_SHORT_DELAY_3:
    {
      schedule_delay(TEST_PARAM_TH_GPD_SHORT_DELAY);
      break;
    }

    case TEST_STATE_LONG_DELAY_1A:
    case TEST_STATE_LONG_DELAY_1B:
    case TEST_STATE_LONG_DELAY_2:
    case TEST_STATE_LONG_DELAY_3:
    {
      schedule_delay(TEST_PARAM_TH_GPD_LONG_DELAY);
      break;
    }

    case TEST_STATE_CLEAR_AUTO_COMM_FLAG_1A:
    case TEST_STATE_CLEAR_AUTO_COMM_FLAG_1B:
    {
      ZGPD->auto_commissioning_pending = 0;
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      break;
    }

    case TEST_STATE_START_AUTOCOMMISSIONING_1A:
    case TEST_STATE_START_AUTOCOMMISSIONING_1B:
    {
      start_comm(ZB_ZGPD_COMMISSIONING_AUTO);
      break;
    }
    case TEST_STATE_START_COMMISSIONING_2:
    {
      start_comm(ZB_ZGPD_COMMISSIONING_UNIDIR);
      break;
    }

    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
  };
}

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_COMMISSIOIN_WITH_PRESS_1_OF_1_1A:
    case TEST_STATE_SEND_PRESS_1_OF_1_1A:
    case TEST_STATE_SEND_PRESS_1_OF_1_1B:
    case TEST_STATE_SEND_PRESS_1_OF_1_2:
    case TEST_STATE_SEND_PRESS_1_OF_1_3:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_PRESS_1_OF_1);
      break;
    }
    case TEST_STATE_COMMISSIOIN_WITH_RELEASE_1_OF_1_1B:
    case TEST_STATE_SEND_RELEASE_1_OF_1_1A:
    case TEST_STATE_SEND_RELEASE_1_OF_1_1B:
    case TEST_STATE_SEND_RELEASE_1_OF_1_2:
    case TEST_STATE_SEND_RELEASE_1_OF_1_3:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_RELEASE_1_OF_1);
      break;
    }
  }
}

/*============================================================================*/
/*                            TEST IMPLEMENTATION                             */
/*============================================================================*/
static void start_comm(zb_uint8_t comm_type)
{
  zb_callback_t cb = NULL;

  if (comm_type != ZB_ZGPD_COMMISSIONING_AUTO)
  {
    cb = comm_cb;
  }
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, comm_type, ZB_ZGP_SIMPLE_GEN_1_STATE_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);

  zb_zgpd_start_commissioning(cb);
  if (comm_type == ZB_ZGPD_COMMISSIONING_AUTO)
  {
    ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
  }
}

static void schedule_delay(zb_uint32_t timeout)
{
  ZB_ZGPD_SET_PAUSE(timeout);
  ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
}

/*============================================================================*/
/*                          STARTUP                                           */
/*============================================================================*/

static void zgp_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gpd");

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
