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

#define ZB_TEST_NAME RTP_ZDO_02_DUT_ZED
#define ZB_TRACE_FILE_ID 40355

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
#include "device_th.h"

#include "device_dut.h"
#include "rtp_zdo_02_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_zed = IEEE_ADDR_DUT_ZED;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zdo_02_dut_zed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_zdo_02_dut_zed_device_clusters,
                        rtp_zdo_02_dut_zed_basic_attr_list);

DECLARE_DUT_EP(rtp_zdo_02_dut_zed_device_ep,
              DUT_ENDPOINT,
              rtp_zdo_02_dut_zed_device_clusters);

DECLARE_DUT_CTX(rtp_zdo_02_dut_zed_device_ctx, rtp_zdo_02_dut_zed_device_ep);

static void test_find_bulb_delayed(zb_uint8_t unused);
static void test_find_bulb(zb_uint8_t param);
static void test_find_bulb_cb(zb_uint8_t param);
static void test_bulb_ieee_addr_req(zb_uint8_t param);
static void test_bulb_ieee_addr_req_cb(zb_uint8_t param);
static void test_format_buffer_for_bind_req(zb_uint8_t param);

static void test_bind_bulb(zb_uint8_t param);
static void test_bind_bulb_cb(zb_uint8_t param);

static void test_send_command_alarm_delayed(zb_uint8_t unused);
static void test_send_on_off(zb_uint8_t param);
static void test_send_on_off_cb(zb_uint8_t param);

static zb_bool_t g_dut_cmd_on_off = ZB_FALSE;

static zb_uint16_t g_bind_clusters_list[] = {
  ZB_ZCL_CLUSTER_ID_ON_OFF,
  ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL
};

static zb_uindex_t g_bind_cluster_index = 0;

/************************Main*************************************/
MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut_zed");


  zb_set_long_address(g_ieee_addr_dut_zed);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_ed_role(1l << TEST_CHANNEL);
  zb_set_nvram_erase_at_start(ZB_FALSE);

  zb_set_rx_on_when_idle(ZB_FALSE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_zdo_02_dut_zed_device_ctx);

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

  TRACE_MSG(TRACE_APS1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

        test_step_register(test_find_bulb_delayed, 0, RTP_ZDO_02_STEP_1_TIME_ZED);
        test_step_register(test_send_command_alarm_delayed, 0, RTP_ZDO_02_STEP_2_TIME_ZED);
        test_step_register(test_send_command_alarm_delayed, 0, RTP_ZDO_02_STEP_3_TIME_ZED);

        test_control_start(TEST_MODE, RTP_ZDO_02_STEP_1_DELAY_ZED);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_step_register(test_send_command_alarm_delayed, 0, RTP_ZDO_02_STEP_4_TIME_ZED);
        test_step_register(test_send_command_alarm_delayed, 0, RTP_ZDO_02_STEP_5_TIME_ZED);

        test_control_start(TEST_MODE, RTP_ZDO_02_STEP_4_DELAY_ZED);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      TRACE_MSG(TRACE_APS1, "signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status %d", (FMT__D, status));
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

static void test_find_bulb_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);

  zb_buf_get_out_delayed(test_find_bulb);
}

static void test_find_bulb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_APP1, ">> test_find_bulb %hd", (FMT__H, param));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

  req->nwk_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req->profile_id = ZB_AF_HA_PROFILE_ID;
  /* We are searching for On/Off of Level Control Server */
  req->num_in_clusters = 2;
  req->num_out_clusters = 0;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_ON_OFF;
  req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;

  zb_zdo_match_desc_req(param, test_find_bulb_cb);

  TRACE_MSG(TRACE_APP1, "<< test_find_bulb %hd", (FMT__H, param));
}

static void test_find_bulb_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(buf);
  zb_uint8_t *match_ep;

  TRACE_MSG(TRACE_APP1, ">> test_find_bulb_cb param %hd, resp match_len %hd", (FMT__H_H, param, resp->match_len));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0)
  {
    TRACE_MSG(TRACE_APP1, "Server is found, continue normal work...", (FMT__0));

    /* Match EP list follows right after response header */
    match_ep = (zb_uint8_t*)(resp + 1);

    /* we are searching for exact cluster, so only 1 EP maybe found */
    TRACE_MSG(TRACE_APP1, "find bulb addr %d ep %hd",
              (FMT__D_H, resp->nwk_addr, *match_ep));

    /* Next step is to resolve the IEEE address of the bulb */
    ZB_SCHEDULE_APP_CALLBACK(test_bulb_ieee_addr_req, param);
  }
  else
  {
    zb_buf_free(buf);
  }
}

static void test_bulb_ieee_addr_req(zb_uint8_t param)
{
  zb_bufid_t  buf = param;
  zb_zdo_ieee_addr_req_param_t *req_param;
  zb_zdo_match_desc_resp_t *match_desc_resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(buf);

  req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

  req_param->nwk_addr = match_desc_resp->nwk_addr;
  req_param->dst_addr = req_param->nwk_addr;
  req_param->start_index = 0;
  req_param->request_type = 0;
  zb_zdo_ieee_addr_req(buf, test_bulb_ieee_addr_req_cb);
}

static void test_format_buffer_for_bind_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_apsme_binding_req_t bind_req;

  ZB_BZERO(&bind_req, sizeof(bind_req));

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(param);

  ZB_IEEE_ADDR_COPY(bind_req.src_addr, &g_ieee_addr_dut_zed);
  ZB_IEEE_ADDR_COPY(bind_req.dst_addr.addr_long, resp->ieee_addr);

  bind_req.src_endpoint = DUT_ENDPOINT;
  bind_req.addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  bind_req.dst_endpoint = TH_ZR_ENDPOINT;
  bind_req.confirm_cb = test_bind_bulb_cb;

  zb_buf_reuse(param);
  ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t), &bind_req, sizeof(bind_req));
}

static void test_bulb_ieee_addr_req_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_nwk_addr_resp_head_t *resp;

  TRACE_MSG(TRACE_APP1, ">> test_bulb_ieee_addr_req_cb param %hd", (FMT__H, param));

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(buf);
  TRACE_MSG(TRACE_APP1, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    /* The next step is to bind the Light control to the bulb */
    test_format_buffer_for_bind_req(param);
    ZB_SCHEDULE_APP_CALLBACK(test_bind_bulb, param);
  }

  TRACE_MSG(TRACE_APP1, "<< test_bulb_ieee_addr_req_cb", (FMT__0));
}

static void test_bind_bulb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_apsme_binding_req_t *req;

  TRACE_MSG(TRACE_APP2, ">> test_bind_bulb param %hd", (FMT__H, param));

  req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);
  req->clusterid = g_bind_clusters_list[g_bind_cluster_index];

  zb_apsme_bind_request(param);

  TRACE_MSG(TRACE_APP2, "<< test_bind_bulb", (FMT__0));
}

static void test_bind_bulb_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP2, ">> test_bind_bulb_cb param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID && zb_buf_get_status(param) == RET_OK);

  if (g_bind_cluster_index < ZB_ARRAY_SIZE(g_bind_clusters_list))
  {
    g_bind_cluster_index++;
    ZB_SCHEDULE_CALLBACK(test_bind_bulb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP2, "<< test_bind_bulb_cb", (FMT__0));
}

static void test_send_command_alarm_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  g_dut_cmd_on_off = !g_dut_cmd_on_off;

  zb_buf_get_out_delayed(test_send_on_off);
}

static void test_send_on_off(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_uint8_t cmd_id = (g_dut_cmd_on_off) ? (ZB_ZCL_CMD_ON_OFF_ON_ID) : (ZB_ZCL_CMD_ON_OFF_OFF_ID);
  zb_uint16_t addr = 0;

  TRACE_MSG(TRACE_APP1, ">> test_send_on_off, cmd_id %hd", (FMT__H, cmd_id));

  /* Dst addr and endpoint are unknown; command will be sent via binding */
  ZB_ZCL_ON_OFF_SEND_REQ(
    buf,
    addr,
    ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    0,
    DUT_ENDPOINT,
    ZB_AF_HA_PROFILE_ID,
    ZB_TRUE,
    cmd_id,
    test_send_on_off_cb
    );

  TRACE_MSG(TRACE_APP1, "<< test_send_on_off", (FMT__0));
}

static void test_send_on_off_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);

  TRACE_MSG(TRACE_APP1, ">> test_send_on_off_cb param %hd status %hd",
            (FMT__H_H, param, cmd_send_status->status));

  if (cmd_send_status->status != RET_OK)
  {
    TRACE_MSG(TRACE_APP1, "test_send_on_off_cb: transmission failed", (FMT__0));
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP1, "<< test_send_on_off_cb", (FMT__0));
}

/*! @} */
