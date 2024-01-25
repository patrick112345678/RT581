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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME RTP_ZDO_01_DUT_ZC

#define ZB_TRACE_FILE_ID 40416
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

#include "device_dut.h"
#include "rtp_zdo_01_common.h"
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

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT_ZC;
static zb_ieee_addr_t g_ieee_addr_th_zr = IEEE_ADDR_TH_ZR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_zdo_01_dut_zc_basic_attr_list, &attr_zcl_version, &attr_power_source);

/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(rtp_zdo_01_dut_zc_device_clusters,
                         rtp_zdo_01_dut_zc_basic_attr_list);

DECLARE_DUT_EP(rtp_zdo_01_dut_zc_device_ep,
               DUT_ENDPOINT,
               rtp_zdo_01_dut_zc_device_clusters);

DECLARE_DUT_CTX(rtp_zdo_01_dut_zc_device_ctx, rtp_zdo_01_dut_zc_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_bind_req_delayed(zb_uint8_t unused);
static void send_bind_req(zb_uint8_t param);
static zb_uint8_t test_data_indication_cb(zb_uint8_t param);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_ZCL | TRACE_SUBSYSTEM_ZDO);
  ZB_SET_TRACE_LEVEL(1);

  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut_zc");


  zb_set_long_address(g_ieee_addr_dut);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_zdo_01_dut_zc_device_ctx);

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
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

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
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
        ZB_SCHEDULE_ALARM(send_bind_req_delayed, 0, (DUT_FB_INITIATOR_DELAY + 5 * ZB_TIME_ONE_SECOND));
        zb_af_set_data_indication(test_data_indication_cb);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}

static void trigger_fb_initiator(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}

static void send_bind_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_bind_req);
}

static void send_bind_req(zb_uint8_t param)
{
  zb_zdo_bind_req_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

  TRACE_MSG(TRACE_ZDO2, ">>send_bind_req: buf_param = %d",
            (FMT__D, param));

  ZB_IEEE_ADDR_COPY(req->src_address, g_ieee_addr_th_zr);
  req->src_endp = THR1_ENDPOINT;
  req->cluster_id = ZB_ZCL_CLUSTER_ID_IDENTIFY;
  req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_ieee_addr_dut);
  req->dst_endp = DUT_ENDPOINT;
  req->req_dst_addr = zb_address_short_by_ieee(g_ieee_addr_th_zr);

  zb_zdo_bind_req(param, NULL);

  TRACE_MSG(TRACE_ZDO2, "<<send_bind_req", (FMT__0));
}

static zb_uint8_t test_data_indication_cb(zb_uint8_t param)
{
  zb_uint8_t param_tmp = zb_buf_get_out();
  zb_uint8_t aps_counter, tsn;
  zb_apsde_data_indication_t *ind;
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_status_t status;

  TRACE_MSG(TRACE_APP1, "test_data_indication_cb(): param %d", (FMT__D, param));

  if (param_tmp)
  {
    zb_buf_copy(param_tmp, param);
  }
  else
  {
    return ZB_FALSE;
  }

  ind = ZB_BUF_GET_PARAM(param_tmp, zb_apsde_data_indication_t);
  aps_counter = ind->aps_counter;

  switch (ind->profileid)
  {
    case ZB_AF_HA_PROFILE_ID:
      ZB_BZERO(&cmd_info, sizeof(cmd_info));
      status = zb_zcl_parse_header(param_tmp, &cmd_info);
      tsn = cmd_info.seq_number;

      if (status == ZB_ZCL_STATUS_SUCCESS &&
          cmd_info.is_common_command == ZB_TRUE &&
          cmd_info.cmd_id == ZB_ZCL_CMD_READ_ATTRIB)
      {
        TRACE_MSG(TRACE_APP1, "test_data_indication_cb(): read_attr received, param %d, aps_counter %d, tsn %d",
                  (FMT__D_D_D, param, aps_counter, tsn));
      }
      break;

    case ZB_AF_ZDO_PROFILE_ID:
      if (ind->dst_endpoint == 0 &&
          ind->clusterid == ZDO_MGMT_LQI_REQ_CLID)
      {
        tsn = *(zb_uint8_t *)zb_buf_begin(param_tmp);

        TRACE_MSG(TRACE_APP1, "test_data_indication_cb(): lqi_req received, param %d, aps_counter %d, tsn %d",
                  (FMT__D_D_D, param, aps_counter, tsn));
      }
      break;
    default:
      break;
  }

  zb_buf_free(param_tmp);

  return ZB_FALSE;
}

/*! @} */
