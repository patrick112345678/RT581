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

#define ZB_TEST_NAME GPS_GPDF_BASIC_APP_000_TH_GPD
#define ZB_TRACE_FILE_ID 41511
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"
#include "zb_mac_globals.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;

static zb_uint8_t g_oob_key[16] = TEST_OOB_KEY;

static zb_uint8_t g_mac_start;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIATE,
  TEST_STATE_COMMISSIONING,
  TEST_STATE_SEND_CMD_TOGGLE1,
  TEST_STATE_SEND_CMD_TOGGLE2,
  TEST_STATE_SEND_CMD_TOGGLE3A,
  TEST_STATE_SEND_CMD_TOGGLE3B,
  TEST_STATE_SEND_CMD_TOGGLE3C,
  TEST_STATE_SEND_CMD_TOGGLE3D,
  TEST_STATE_SEND_CMD_TOGGLE4,
  TEST_STATE_SEND_CMD_TOGGLE5,
  TEST_STATE_SEND_CMD_TOGGLE6,
  TEST_STATE_SEND_CMD_TOGGLE7,
  TEST_STATE_SEND_CMD_TOGGLE8,
  TEST_STATE_SEND_SET_MAC_DSN91,
  TEST_STATE_SEND_CMD_TOGGLE91,
  TEST_STATE_SEND_SET_MAC_DSN92,
  TEST_STATE_SEND_CMD_TOGGLE92,
  TEST_STATE_SEND_CMD_TOGGLE10,
  TEST_STATE_SEND_CMD_TOGGLE11A,
  TEST_STATE_SEND_CMD_TOGGLE11B,
  TEST_STATE_SEND_CMD_TOGGLE12,
  TEST_STATE_SEND_CMD_TOGGLE13,
  TEST_STATE_SEND_CMD_TOGGLE14,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void set_dsn_and_call(zb_uint8_t param);

static void perform_next_state(zb_uint8_t param)
{
  ZVUNUSED(param);
  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_COMMISSIONING:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    case TEST_STATE_SEND_SET_MAC_DSN91:
    case TEST_STATE_SEND_SET_MAC_DSN92:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->mac_dsn = g_mac_start+1;
      ZB_GET_OUT_BUF_DELAYED(set_dsn_and_call);
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
  };
}

static void next_state(zb_uint8_t param)
{
  TEST_DEVICE_CTX.test_state++;
  test_send_command(param);
}

static void set_dsn_and_call(zb_uint8_t param)
{
  zgpd_set_dsn_and_call(param, next_state);
}

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_CMD_TOGGLE1:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE2:
      ZB_ZGPD_CHACK_RESET_ALL();
      g_mac_start = ZB_MAC_DSN();
      ZGPD->ch_replace_extnwk_flag = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZGPD->ch_insert_extnwk_data = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE3A:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
	  ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;

    case TEST_STATE_SEND_CMD_TOGGLE3B:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED2;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE3C:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_MAINTENANCE;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE3D:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_proto_version = 2;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE4:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_extnwk_flag = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE5:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_insert_extnwk_data = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE6:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_app_id = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE7:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_app_id = 3;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE8:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_direction = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(3);
	  ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;

    case TEST_STATE_SEND_CMD_TOGGLE91:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(1);
	  ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    case TEST_STATE_SEND_CMD_TOGGLE92:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(1);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE10:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_rxaftertx = 1;
      ZGPD->ch_replace_autocomm  = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
	  ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;

    case TEST_STATE_SEND_CMD_TOGGLE11A:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_src_id = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE11B:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_src_id=(ZGPD->id.addr.src_id)^1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SRCID);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE12:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->security_frame_counter = ZB_MAC_DSN();
      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
      ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE13:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
      ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
      ZGPD->ch_replace_rxaftertx = 0;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;

    case TEST_STATE_SEND_CMD_TOGGLE14:
      ZB_ZGPD_CHACK_RESET_ALL();
      ZGPD->ch_replace_rxaftertx = 1;
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
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
