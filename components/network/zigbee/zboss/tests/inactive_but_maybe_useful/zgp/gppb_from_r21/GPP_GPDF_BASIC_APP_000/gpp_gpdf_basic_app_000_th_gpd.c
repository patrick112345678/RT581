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

#define ZB_TEST_NAME GPP_GPDF_BASIC_APP_000_TH_GPD
#define ZB_TRACE_FILE_ID 41342
#include "zb_common.h"
#include "zgpd/zb_zgpd.h"
#include "test_config.h"
#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
static zb_uint32_t  g_zgpd_srcId2 = 0x12345679;
static zb_uint32_t  g_zgpd_srcId3 = 0x00000000;
static zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_SET_MAC_DSN,
  TEST_STATE_SEND_DATA_GPDF1,
  TEST_STATE_SEND_DATA_GPDF2,
  TEST_STATE_SEND_DATA_GPDF3,
  TEST_STATE_SET_MAC_DSN1,
  TEST_STATE_SEND_DATA_GPDF4,
  TEST_STATE_SET_MAC_DSN2,
  TEST_STATE_SEND_DATA_GPDF5,
  TEST_STATE_SET_MAC_DSN3,
  TEST_STATE_SEND_DATA_GPDF6,
  TEST_STATE_SEND_DATA_GPDF7,
  TEST_STATE_SEND_DATA_GPDF8,
  TEST_STATE_SEND_DATA_GPDF9,
  TEST_STATE_SEND_DATA_GPDF10,
  TEST_STATE_SET_MAC_DSN4,
  TEST_STATE_SEND_DATA_GPDF11,
  TEST_STATE_SEND_DATA_GPDF12,
  TEST_STATE_SEND_DATA_GPDF13,
  TEST_STATE_SET_MAC_DSN5,
  TEST_STATE_SEND_DATA_GPDF14,
  TEST_STATE_SEND_DATA_GPDF15,
  TEST_STATE_SEND_DATA_GPDF16,
  TEST_STATE_SEND_DATA_GPDF17,
  TEST_STATE_SEND_DATA_GPDF18,
  TEST_STATE_SEND_DATA_GPDF19,
  TEST_STATE_SEND_DATA_GPDF20,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 2000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_DATA_GPDF1:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
	  ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF2:
    {
      ZGPD->ext_nwk_present = 0;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF3:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED2;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF4:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_RESERVED1;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF5:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE);
      ZGPD->ch_replace_frame_type = ZGP_FRAME_TYPE_MAINTENANCE;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF6:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION);
      ZGPD->ch_replace_proto_version = 2;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF7:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZGPD->ch_replace_extnwk_flag = 0;
      ZGPD->ch_insert_extnwk_data = 1;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF8:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA);
      ZGPD->ch_replace_extnwk_flag = 1;
      ZGPD->ch_insert_extnwk_data = 0;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF9:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZGPD->ch_replace_app_id = 1;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF10:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_APPID);
      ZGPD->ch_replace_app_id = 3;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF11:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_KEY_TYPE);
      ZGPD->ch_replace_sec_level = 2;
      ZGPD->ch_replace_key_type = ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF12:
    {
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_DIRECTION);
      ZGPD->ch_replace_direction = 1;
      ZB_ZGPD_SET_PAUSE(4);
	  ZB_ZGP_RESET_PASSED_STATE_SEQUENCE();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF13:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF14:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
	  ZB_ZGP_SET_PASSED_STATE_SEQUENCE();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF15:
    {
      zb_uint8_t i;

      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_ATTR_REPORT);
      for (i = 0; i < 59; i++)
      {
        ZB_GPDF_PUT_UINT8(*ptr, i);
      }
      ZB_ZGPD_CHACK_RESET_ALL();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF16:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_AUTO_COMM);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
      ZGPD->ch_replace_rxaftertx = 1;
      ZGPD->ch_replace_autocomm = 1;
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF17:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId2);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_RESET_ALL();
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF18:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId3);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF19:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
    }
    case TEST_STATE_SEND_DATA_GPDF20:
    {
      ZGPD->ext_nwk_present = 1;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_CHACK_SET(ZB_ZGPD_CH_REPLACE_RXAFTERTX);
      ZGPD->ch_replace_rxaftertx = 1;
      break;
    }
  };
}

static void zgpd_set_dsn(zb_uint8_t param, zb_callback_t cb)
{
  zgpd_set_dsn_and_call(param, cb);
}

static void perform_next_state(zb_uint8_t param)
{
  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SET_MAC_DSN:
    {
      if (param == 0)
      {
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        TEST_DEVICE_CTX.test_state--;
        break;
      }
      ZGPD->mac_dsn = 0xc3;
      zgpd_set_dsn(param, perform_next_state);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    }
    case TEST_STATE_SET_MAC_DSN1:
    case TEST_STATE_SET_MAC_DSN2:
    case TEST_STATE_SET_MAC_DSN3:
    {
      if (param == 0)
      {
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        TEST_DEVICE_CTX.test_state--;
        break;
      }
      ZGPD->mac_dsn = 0xc5;
      zgpd_set_dsn(param, perform_next_state);
      break;
    }
    case TEST_STATE_SET_MAC_DSN4:
    {
      if (param == 0)
      {
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        TEST_DEVICE_CTX.test_state--;
        break;
      }
      ZGPD->mac_dsn = 0xc9;
      zgpd_set_dsn(param, perform_next_state);
      break;
    }
    case TEST_STATE_SET_MAC_DSN5:
    {
      if (param == 0)
      {
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        TEST_DEVICE_CTX.test_state--;
        break;
      }
      ZGPD->mac_dsn = 0xcb;
      zgpd_set_dsn(param, perform_next_state);
      break;
    }
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
  };
}

static void zgp_custom_startup()
{
  ZB_INIT("th_gpd");

  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_GROUP);
  ZB_ZGPD_SET_SECURITY_KEY(g_zgpd_key);
  ZB_ZGPD_SET_OOB_KEY(g_zgpd_key);
  g_zgpd_ctx.security_frame_counter = 10;

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
