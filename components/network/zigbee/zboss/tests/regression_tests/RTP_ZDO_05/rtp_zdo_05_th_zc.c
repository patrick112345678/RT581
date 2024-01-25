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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_ZDO_05_TH_ZC
#define ZB_TRACE_FILE_ID 63989

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
#include "rtp_zdo_05_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_th = IEEE_ADDR_TH;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zdo_05_th_zc_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zdo_05_th_zc_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(rtp_zdo_05_th_zc_device_clusters,
                        rtp_zdo_05_th_zc_basic_attr_list,
                        rtp_zdo_05_th_zc_identify_attr_list);

DECLARE_TH_EP(rtp_zdo_05_th_zc_device_ep,
               TH_ENDPOINT,
               rtp_zdo_05_th_zc_device_clusters);

DECLARE_TH_CTX(rtp_zdo_05_th_zc_device_ctx, rtp_zdo_05_th_zc_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

typedef struct test_bind_entry_s
{
  zb_uint16_t cluster_id;
  zb_uint8_t expected_bind_status;
  zb_uint8_t expected_unbind_status;
} test_bind_entry_t;

static void send_bind_unbind_req_to_dut(zb_uint8_t param, zb_uint16_t cluster_id, zb_bool_t do_bind, zb_callback_t cb);

static void test_perform_binding_unbinding(zb_uint8_t param);
static void test_perform_binding_unbinding_cb(zb_uint8_t param);

static void test_check_dut_binding_table(zb_uint8_t param);
static void test_check_dut_binding_table_cb(zb_uint8_t param);

static test_bind_entry_t g_test_bind_entries[] =
{
  { ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZDP_STATUS_SUCCESS, ZB_ZDP_STATUS_SUCCESS },
  { DUT_INVALID_CLUSTER_ID, ZB_ZDP_STATUS_SUCCESS, ZB_ZDP_STATUS_SUCCESS },
  { DUT_NOT_DECLARED_CLUSTER_ID, ZB_ZDP_STATUS_SUCCESS, ZB_ZDP_STATUS_SUCCESS },
};

static zb_int8_t g_test_bind_entry_index = 0;
static zb_bool_t g_perform_binding = ZB_TRUE;

static zb_int8_t g_test_received_bind_entry_index = 0;

static zb_uint16_t g_test_dut_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;


MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_th_zc");


  zb_set_long_address(g_ieee_addr_th);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_zdo_05_th_zc_device_ctx);

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


/***********************************Implementation**********************************/
ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

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
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APP1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_zdo_signal_device_annce_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

        ZB_ASSERT(g_test_dut_short_addr == ZB_NWK_BROADCAST_ALL_DEVICES);
        g_test_dut_short_addr = params->device_short_addr;

        test_step_register(test_perform_binding_unbinding, 0, RTP_ZDO_05_STEP_1_TIME_ZC);
        test_control_start(TEST_MODE, RTP_ZDO_05_STEP_1_DELAY_ZC);
      }
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void send_bind_unbind_req_to_dut(zb_uint8_t param, zb_uint16_t cluster_id, zb_bool_t do_bind, zb_callback_t cb)
{
  zb_zdo_bind_req_param_t *req;

  req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

  ZB_IEEE_ADDR_COPY(req->src_address, g_ieee_addr_dut);
  req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_th);
  req->req_dst_addr = g_test_dut_short_addr;
  req->src_endp = DUT_ENDPOINT;
  req->dst_endp = TH_ENDPOINT;
  req->cluster_id = cluster_id;

  if (do_bind)
  {
    zb_zdo_bind_req(param, cb);
  }
  else
  {
    zb_zdo_unbind_req(param, cb);
  }
}

static void test_perform_binding_unbinding(zb_uint8_t param)
{
  if (param == 0)
  {
    zb_buf_get_out_delayed(test_perform_binding_unbinding);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">>test_perform_binding_unbinding", (FMT__0));

  send_bind_unbind_req_to_dut(param,
                              g_test_bind_entries[g_test_bind_entry_index].cluster_id,
                              g_perform_binding,
                              test_perform_binding_unbinding_cb);

  TRACE_MSG(TRACE_APP1, "<<test_perform_binding_unbinding", (FMT__0));
}

static void test_perform_binding_unbinding_cb(zb_uint8_t param)
{
  zb_zdo_bind_resp_t* bind_resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);

  TRACE_MSG(TRACE_APP1, ">>test_perform_binding_unbinding_cb: param = %hd, status = %d",
            (FMT__H_D, param, bind_resp->status, g_test_bind_entries[g_test_bind_entry_index]));

  if (g_perform_binding)
  {
    ZB_ASSERT(bind_resp->status == g_test_bind_entries[g_test_bind_entry_index].expected_bind_status);
  }
  else
  {
    ZB_ASSERT(bind_resp->status == g_test_bind_entries[g_test_bind_entry_index].expected_unbind_status);
  }

  if (g_perform_binding)
  {
    g_test_bind_entry_index++;
  }
  else
  {
    g_test_bind_entry_index--;
  }

  zb_buf_reuse(param);

  if (g_test_bind_entry_index == ZB_ARRAY_SIZE(g_test_bind_entries))
  {
    ZB_SCHEDULE_CALLBACK(test_check_dut_binding_table, param);
  }
  else if (g_test_bind_entry_index == -1)
  {
    ZB_SCHEDULE_CALLBACK(test_check_dut_binding_table, param);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(test_perform_binding_unbinding, param);
  }

  TRACE_MSG(TRACE_APP1, "<<test_perform_binding_unbinding_cb", (FMT__0));
}

static void test_check_dut_binding_table(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_param_t *req_params;

  if (param == 0)
  {
    zb_buf_get_out_delayed(test_check_dut_binding_table);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">>test_check_dut_binding_table: param = %i", (FMT__D, param));

  req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
  req_params->start_index = g_test_received_bind_entry_index;
  req_params->dst_addr = g_test_dut_short_addr;
  zb_zdo_mgmt_bind_req(param, test_check_dut_binding_table_cb);

  TRACE_MSG(TRACE_APP1, "<<test_check_dut_binding_table", (FMT__0));
}

static void test_check_dut_binding_table_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_resp_t *resp;
  zb_zdo_binding_table_record_t *records;
  zb_uindex_t record_index = 0;
  zb_bool_t is_binding_table_correct = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">>test_check_dut_binding_table_cb: param = %i", (FMT__D, param));

  resp = (zb_zdo_mgmt_bind_resp_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_APP1, "mgmt_bind_resp_cb: status = %i", (FMT__D, resp->status));

  ZB_ASSERT(resp->status == resp->status);

  if (g_perform_binding)
  {
    ZB_ASSERT(resp->binding_table_list_count > 0);

    records = (zb_zdo_binding_table_record_t*) (resp + 1);

    for (record_index = 0; record_index < resp->binding_table_list_count; record_index++)
    {
      if (records[record_index].cluster_id != g_test_bind_entries[g_test_received_bind_entry_index].cluster_id)
      {
        is_binding_table_correct = ZB_FALSE;
      }

      g_test_received_bind_entry_index++;
    }

    ZB_ASSERT(is_binding_table_correct);

    zb_buf_reuse(param);

    if (g_test_received_bind_entry_index == ZB_ARRAY_SIZE(g_test_bind_entries))
    {
      g_test_received_bind_entry_index = 0;
      g_perform_binding = ZB_FALSE;
      g_test_bind_entry_index--;

      ZB_SCHEDULE_CALLBACK(test_perform_binding_unbinding, param);
    }
    else
    {
      ZB_SCHEDULE_CALLBACK(test_check_dut_binding_table, param);
    }
  }
  else
  {
    ZB_ASSERT(resp->binding_table_list_count == 0);
  }

  TRACE_MSG(TRACE_APP1, "<<mgmt_bind_resp_cb", (FMT__0));
}

/*! @} */
