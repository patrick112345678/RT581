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
/* PURPOSE: DUT ZED
*/

#define ZB_TEST_NAME RTP_APS_14_DUT_ZED
#define ZB_TRACE_FILE_ID 64911

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"

#include "rtp_aps_14_common.h"
#include "../common/zb_reg_test_globals.h"
#include "device_dut.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_aps_14_dut_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_aps_14_dut_zed_device_clusters,
                        rtp_aps_14_dut_zed_basic_attr_list);

DECLARE_DUT_EP(rtp_aps_14_dut_zed_device_ep,
               DUT_ENDPOINT,
               rtp_aps_14_dut_zed_device_clusters);

DECLARE_DUT_CTX(rtp_aps_14_dut_zed_device_ctx, rtp_aps_14_dut_zed_device_ep);
/*************************************************************************/

#if defined(ZB_APS_USER_PAYLOAD) && !defined(NCP_MODE_HOST)
static void test_send_request_with_aps_user_payload_tx_cb(zb_uint8_t param);
static void test_send_request_with_aps_user_payload(zb_uint8_t param);
#endif /* ZB_APS_USER_PAYLOAD && !NCP_MODE_HOST */

static void test_send_mgmt_lqi_req(zb_uint8_t param);
static void test_send_mgmt_lqi_resp_cb(zb_uint8_t param);

void test_send_on_off_toggle_req(zb_uint8_t param);
void test_send_on_off_toggle_resp_cb(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr_dut = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_dut_zed");

  zb_set_long_address(g_ieee_addr_dut);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);

#if defined(ZB_APS_USER_PAYLOAD) && !defined(NCP_MODE_HOST)
  zb_aps_set_user_data_tx_cb(test_send_request_with_aps_user_payload_tx_cb);
#endif /* ZB_APS_USER_PAYLOAD && !NCP_MODE_HOST */

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_aps_14_dut_zed_device_ctx);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/********************ZDO Startup*****************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
#if defined(ZB_APS_USER_PAYLOAD) && !defined(NCP_MODE_HOST)
        test_step_register(test_send_mgmt_lqi_req, 0, RTP_APS_14_STEP_1_TIME_ZED);
        test_step_register(test_send_on_off_toggle_req, 0, RTP_APS_14_STEP_2_TIME_ZED);
        test_step_register(test_send_request_with_aps_user_payload, 0, RTP_APS_14_STEP_3_TIME_ZED);
        test_step_register(test_send_request_with_aps_user_payload, 0, RTP_APS_14_STEP_4_TIME_ZED);
        test_step_register(test_send_request_with_aps_user_payload, 0, RTP_APS_14_STEP_5_TIME_ZED);

        test_control_start(TEST_MODE, RTP_APS_14_STEP_1_DELAY_ZED);

#else
        ZB_ASSERT(ZB_FALSE && "NCP doesn't support APS User Payload feature");
#endif /* ZB_APS_USER_PAYLOAD && !NCP_MODE_HOST */
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      if (status == 0)
      {
        zb_sleep_now();
      }
      break; /* ZB_COMMON_SIGNAL_CAN_SLEEP */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


#if defined(ZB_APS_USER_PAYLOAD) && !defined(NCP_MODE_HOST)
static void test_send_request_with_aps_user_payload_tx_cb(zb_uint8_t param)
{
  zb_uint8_t aps_payload_size;
  zb_uint8_t *aps_payload_ptr;
  zb_ret_t buf_status;

  ZB_ASSERT(param != ZB_BUF_INVALID);

  TRACE_MSG(TRACE_APP1, ">> test_send_request_with_aps_user_payload_tx_cb, param: %hd", (FMT__H, param));

  buf_status = zb_buf_get_status(param);
  aps_payload_ptr = zb_aps_get_aps_payload(param, &aps_payload_size);

  TRACE_MSG(TRACE_APP1, "buf_status %d, buf_len: %hd, aps_payload_size: %hd", (FMT__D_H_H,
    buf_status, zb_buf_len(param), aps_payload_size));

  switch ((zb_aps_user_payload_cb_status_t)buf_status)
  {
    case ZB_APS_USER_PAYLOAD_CB_STATUS_SUCCESS:
      TRACE_MSG(TRACE_APP1, "Transmission status: SUCCESS", (FMT__0));
      break;

    case ZB_APS_USER_PAYLOAD_CB_STATUS_NO_APS_ACK:
      TRACE_MSG(TRACE_APP1, "Transmission status: NO_APS_ACK", (FMT__0));
      break;

    default:
      TRACE_MSG(TRACE_APP1, "Transmission status: INVALID", (FMT__0));
      ZB_ASSERT(ZB_FALSE);
      break;
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP1, "<< test_send_request_with_aps_user_payload_tx_cb", (FMT__0));
}


static void test_send_request_with_aps_user_payload(zb_uint8_t param)
{
  zb_uint8_t payload_size = RTP_APS_14_APS_USER_PAYLOAD_SIZE;
  zb_uint8_t payload_ptr[RTP_APS_14_APS_USER_PAYLOAD_SIZE] = RTP_APS_14_APS_USER_PAYLOAD;
  zb_addr_u dst_addr;
  zb_ret_t status;

  if (param == 0)
  {
    zb_buf_get_out_delayed(test_send_request_with_aps_user_payload);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_send_request_with_aps_user_payload, param %hd", (FMT__H, param));

  dst_addr.addr_short = 0x0000;

  status = zb_aps_send_user_payload(
    param,
    dst_addr,                          /* dst_addr */
    ZB_AF_HA_PROFILE_ID,               /* profile id */
    ZB_ZCL_CLUSTER_ID_BASIC,           /* cluster id */
    TH_ENDPOINT,                       /* destination endpoint */
    DUT_ENDPOINT,                      /* source endpoint */
    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    ZB_TRUE,                           /* APS ACK enabled */
    payload_ptr,
    payload_size);

  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< test_send_request_with_aps_user_payload", (FMT__0));
}
#endif /* ZB_APS_USER_PAYLOAD && !NCP_MODE_HOST */


static void test_send_mgmt_lqi_resp_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_resp_t *resp;

  TRACE_MSG(TRACE_APP1, ">> test_send_mgmt_lqi_resp_cb, param %hd", (FMT__H, param));

  resp = (zb_zdo_mgmt_lqi_resp_t*) zb_buf_begin(param);

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_APP1, " test_send_mgmt_lqi_resp_cb: success", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, " test_send_mgmt_lqi_resp_cb: failure", (FMT__0));
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP1, "<< test_send_mgmt_lqi_resp_cb", (FMT__0));
}


static void test_send_mgmt_lqi_req(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_param_t *req_param;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_send_mgmt_lqi_req);
    return;
  }

  TRACE_MSG(TRACE_APP1, "> >test_send_mgmt_lqi_req, param %hd", (FMT__H, param));

  req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

  req_param->dst_addr = 0x0000;
  req_param->start_index = 0;

  zb_zdo_mgmt_lqi_req(param, test_send_mgmt_lqi_resp_cb);

  TRACE_MSG(TRACE_APP1, "<< test_send_mgmt_lqi_req", (FMT__0));
}


void test_send_on_off_toggle_resp_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status;

  TRACE_MSG(TRACE_APP1, ">> test_send_on_off_toggle_req_cb, param %hd", (FMT__H, param));

  cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
  TRACE_MSG(TRACE_APP1, "  test_send_on_off_toggle_req_cb: status %d", (FMT__D, cmd_send_status->status));

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP1, "<< test_send_on_off_toggle_req_cb", (FMT__0));
}


void test_send_on_off_toggle_req(zb_uint8_t param)
{
  zb_uint16_t dst_addr;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_send_on_off_toggle_req);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_send_on_off_toggle_req, param %hd", (FMT__H, param));

  dst_addr = 0x0000;

  ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(
    param,
    dst_addr,
    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    TH_ENDPOINT,
    DUT_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    ZB_FALSE, test_send_on_off_toggle_resp_cb);

  TRACE_MSG(TRACE_APP1, "<< test_send_on_off_toggle_req", (FMT__0));
}

/*! @} */
