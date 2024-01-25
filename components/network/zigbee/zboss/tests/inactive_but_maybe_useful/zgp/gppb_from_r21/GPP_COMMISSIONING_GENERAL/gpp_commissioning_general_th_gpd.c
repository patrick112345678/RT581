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
/* PURPOSE: Simple ZGPD for send GPDF as described
in 2.3.4 test specification.
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_GENERAL_TH_GPD
#define ZB_TRACE_FILE_ID 41279
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t g_zgpd_srcId = TEST_ZGPD_SRC_ID;
static zb_uint32_t g_zgpd_zero_srcId = 0;
static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;
static zb_ieee_addr_t g_zgpd_zero_addr = TH_GPD_IEEE_ZERO_ADDR;
static zb_uint8_t  g_oob_key[] = TEST_OOB_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_SET_MAC_DSN_INIT,
  TEST_STATE_COMMISSIONING1,
  TEST_STATE_SEND_DATA_GPDF1,
  TEST_STATE_DECOMMISSIONING1,
  TEST_STATE_COMMISSIONING2,
  TEST_STATE_DECOMMISSIONING2,
  TEST_STATE_COMMISSIONING3,
  TEST_STATE_DECOMMISSIONING3,
  TEST_STATE_COMMISSIONING4,
  TEST_STATE_COMMISSIONING5,
  TEST_STATE_COMMISSIONING5_1,
  TEST_STATE_COMMISSIONING6,
  TEST_STATE_COMMISSIONING7,
  TEST_STATE_COMMISSIONING8,
  TEST_STATE_COMMISSIONING9,
  TEST_STATE_COMMISSIONING10,
  TEST_STATE_DECOMMISSIONING4,
  TEST_STATE_COMMISSIONING11,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_DATA_GPDF1:
      ZGPD->security_frame_counter = 0x1F00;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(1);
    break;
  };
}

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void setup_dev_type(zb_uint8_t app_id)
{
  if (app_id == ZB_ZGP_APP_ID_0010)
  {
    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
    ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
    g_zgpd_ctx.id.endpoint = 1;
  }
  else
  {
    ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);
    ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  }
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL);
  ZB_ZGPD_SET_OOB_KEY(g_oob_key);
  ZGPD->security_frame_counter = 0xaac3;
}

static void zgpd_set_dsn(zb_uint8_t param, zb_callback_t cb)
{
  zgpd_set_dsn_and_call(param, cb);
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

  TRACE_MSG(TRACE_APP1, "perform next state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_COMMISSIONING1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(1);
      break;
    case TEST_STATE_COMMISSIONING2:
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_MAX_COMM_PAYLOAD);
      ZGPD->ch_max_comm_payload = 55;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING3:
      setup_dev_type(ZB_ZGP_APP_ID_0010);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_MAX_COMM_PAYLOAD);
      ZGPD->ch_max_comm_payload = 50;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING4:
      setup_dev_type(ZB_ZGP_APP_ID_0000);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
      ZGPD->ch_replace_autocomm = 1;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_COMMISSIONING5:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED2;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING5_1:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
      ZGPD->ch_replace_proto_version = 2;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING6:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZGPD->ch_replace_app_id = 3;
      ZGPD->ext_nwk_present = 1;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING7:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
      ZGPD->ch_replace_direction = 1;
      ZB_ZGPD_SET_PAUSE(2);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_COMMISSIONING8:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ext_nwk_present = 0;
      ZB_ZGPD_SET_SRC_ID(g_zgpd_zero_srcId);
      ZB_ZGPD_SET_PAUSE(3);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING9:
      ZB_ZGPD_CHACK_RESET_ALL();
      setup_dev_type(ZB_ZGP_APP_ID_0010);
      ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_zero_addr);
      ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_zero_addr);
      ZB_ZGPD_SET_PAUSE(3);
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_COMMISSIONING10:
      setup_dev_type(ZB_ZGP_APP_ID_0000);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_COMM_EXTRA_PLD);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_COMM_OPT_RESERVED_SET);
      ZGPD->ch_comm_extra_payload = 5;
      ZGPD->ch_comm_extra_payload_start_byte = 0xe0;
      ZGPD->ext_nwk_present = 1;
      ZB_ZGPD_SET_PAUSE(1);
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_COMMISSIONING11:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ext_nwk_present = 0;
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_DECOMMISSIONING1:
      ZGPD->security_frame_counter = 0x1F11;
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 2*ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_DECOMMISSIONING2:
    case TEST_STATE_DECOMMISSIONING3:
    case TEST_STATE_DECOMMISSIONING4:
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 2*ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_SET_MAC_DSN_INIT:
    {
      if (param == 0)
      {
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        TEST_DEVICE_CTX.test_state--;
        break;
      }
      ZGPD->mac_dsn = 0x81;
      ZGPD->security_frame_counter = 0x1F10;
      zgpd_set_dsn(param, perform_next_state);
      break;
    }
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  };
}

static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


  setup_dev_type(ZB_ZGP_APP_ID_0000);
  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
