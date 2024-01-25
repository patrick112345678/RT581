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
/* PURPOSE: TH GPD
*/

#define ZB_TEST_NAME GPS_MAC_SEQ_NUM_FILTERING_INC_TH_GPD
#define ZB_TRACE_FILE_ID 41464
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

static zb_uint8_t g_oob_key[] = TEST_OOB_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIATE,
  TEST_STATE_SET_MAC_INIT,
  TEST_STATE_COMMISSIONING,
  TEST_STATE_SEND_CMD_TOGGLE1,
  TEST_STATE_SEND_SET_MAC_DSN_C3,
  TEST_STATE_SEND_CMD_TOGGLE2,
  TEST_STATE_SEND_SET_MAC_DSN_C2,
  TEST_STATE_SEND_CMD_TOGGLE3,
  TEST_STATE_SEND_SET_MAC_DSN_C5,
  TEST_STATE_SEND_CMD_TOGGLE4,
  TEST_STATE_SEND_SET_MAC_DSN_05,
  TEST_STATE_SEND_CMD_TOGGLE5,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 3000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void set_dsn_and_call(zb_uint8_t param)
{
  zgpd_set_dsn_and_call(param, perform_next_state);
}

static void perform_next_state(zb_uint8_t param)
{
  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
    param = 0;
  }

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
    case TEST_STATE_SET_MAC_INIT:
      ZGPD->mac_dsn = 188;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
    case TEST_STATE_COMMISSIONING:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    case TEST_STATE_SEND_SET_MAC_DSN_C3:
      ZGPD->mac_dsn = 0xc3;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
    case TEST_STATE_SEND_SET_MAC_DSN_C2:
      ZGPD->mac_dsn = 0xc2;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
    case TEST_STATE_SEND_SET_MAC_DSN_C5:
      ZGPD->mac_dsn = 0xc5;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
    case TEST_STATE_SEND_SET_MAC_DSN_05:
      ZGPD->mac_dsn = 0x05;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
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
    case TEST_STATE_SEND_CMD_TOGGLE1:
    case TEST_STATE_SEND_CMD_TOGGLE2:
    case TEST_STATE_SEND_CMD_TOGGLE3:
    case TEST_STATE_SEND_CMD_TOGGLE4:
    case TEST_STATE_SEND_CMD_TOGGLE5:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(3);
    break;
  };
}

static void zgp_custom_startup()
{
  #if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_BIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()

