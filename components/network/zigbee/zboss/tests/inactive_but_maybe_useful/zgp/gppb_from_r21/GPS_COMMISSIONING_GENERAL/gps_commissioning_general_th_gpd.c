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

#define ZB_TEST_NAME GPS_COMMISSIONING_GENERAL_TH_GPD
#define ZB_TRACE_FILE_ID 41270

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

static zb_uint8_t g_key_nwk[] = TEST_NWK_KEY;
static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIATE,
  TEST_STATE_COMMISSIONING1,
  TEST_STATE_SEND_CMD_TOGGLE1,
  TEST_STATE_DECOMMISSIONING1,
  TEST_STATE_COMMISSIONING2A,
  TEST_STATE_COMMISSIONING2B,
  TEST_STATE_COMMISSIONING2C,
  TEST_STATE_COMMISSIONING2D,
  TEST_STATE_COMMISSIONING3,
  TEST_STATE_COMMISSIONING4A,
  TEST_STATE_COMMISSIONING4B,
  TEST_STATE_COMMISSIONING5,
  TEST_STATE_COMMISSIONING5_2,
  TEST_STATE_DECOMMISSIONING2,
  TEST_STATE_COMMISSIONING6,
  TEST_STATE_SEND_CMD_TOGGLE2,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 3000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

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
    case TEST_STATE_DECOMMISSIONING1:
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 3*ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_COMMISSIONING6:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_comm_extra_payload = 5;
      ZGPD->ch_comm_extra_payload_start_byte = 0xe0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_COMM_EXTRA_PLD);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_COMM_OPT_RESERVED_SET);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(1);
      break;
    case TEST_STATE_DECOMMISSIONING2:
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 3 * ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_COMMISSIONING5:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    case TEST_STATE_COMMISSIONING5_2:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(1);
      break;
    case TEST_STATE_COMMISSIONING4B:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(4);
      break;
    case TEST_STATE_COMMISSIONING4A:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(5);
      break;
    case TEST_STATE_COMMISSIONING3:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(4);
      break;
    case TEST_STATE_COMMISSIONING2D:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(4);
      break;
    case TEST_STATE_COMMISSIONING2C:
    {
      zb_uint32_t sfc;

      ZB_ZGPD_CHACK_RESET_ALL();
      sfc = ZGPD->security_frame_counter;
      ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
      ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
      ZB_ZGPD_SET_OOB_KEY(g_oob_key);
      ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
      ZGPD->security_frame_counter=sfc;
      ZGPD->ch_replace_security_key_encrypted = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKE);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    }
    case TEST_STATE_COMMISSIONING2B:
    {
      zb_uint32_t sfc;

      ZB_ZGPD_CHACK_RESET_ALL();
      sfc = ZGPD->security_frame_counter;
      ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
      ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
      ZB_ZGPD_SET_OOB_KEY(g_oob_key);
      ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
      ZGPD->security_frame_counter=sfc;
      ZGPD->ch_insert_security_key = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_SK);
      ZGPD->ch_replace_security_key_present = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SKP);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    }
    case TEST_STATE_COMMISSIONING2A:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_REDUCED);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    case TEST_STATE_COMMISSIONING1:
      ZB_ZGPD_CHACK_RESET_ALL();
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(1);
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
    case TEST_STATE_SEND_CMD_TOGGLE2:
    case TEST_STATE_SEND_CMD_TOGGLE1:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(1);
    break;
  };
}

static void zgp_custom_startup()
{
  #if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);

  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);

  ZB_ZGPD_SET_OOB_KEY(g_oob_key);

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
