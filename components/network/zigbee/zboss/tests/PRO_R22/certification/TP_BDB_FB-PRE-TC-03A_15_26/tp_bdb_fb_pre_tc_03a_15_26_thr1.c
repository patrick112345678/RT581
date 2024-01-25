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
/* PURPOSE: TH ZR1 (initiator)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_03A_15_26_THR1
#define ZB_TRACE_FILE_ID 40062

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

#include "on_off_client.h"
#include "tp_bdb_fb_pre_tc_03a_15_26_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not defined!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_03a_15_26_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_03a_15_26_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pre_tc_03a_15_26_thr1_on_off_controller_clusters,
                                   fb_pre_tc_03a_15_26_thr1_basic_attr_list,
                                   fb_pre_tc_03a_15_26_thr1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pre_tc_03a_15_26_thr1_on_off_controller_ep,
                         THR1_ENDPOINT,
                         fb_pre_tc_03a_15_26_thr1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pre_tc_03a_15_26_thr1_on_off_controller_ctx, fb_pre_tc_03a_15_26_thr1_on_off_controller_ep);

static void trigger_fb_target(zb_uint8_t unused);
static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);
static void send_active_ep_resp(zb_uint8_t param);
extern void zdo_send_resp_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t addr);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Assignment required to force Distributed formation */
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  zb_set_max_children(2);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_03a_15_26_thr1_on_off_controller_ctx);

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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_ext_pan_id_t extended_pan_id;
        zb_get_extended_pan_id(extended_pan_id);

        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
        }
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THR1_FB_TARGET_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d",
                (FMT__D, status));
      if (status == 0)
      {
          ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
          ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;
      }
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION;
  zb_bdb_finding_binding_target(THR1_ENDPOINT);
}

static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{

  TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = %hd", (FMT__H, param));

  switch (cluster_id)
  {
    case ZDO_ACTIVE_EP_REQ_CLID:
      send_active_ep_resp(param);

      ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
      ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
      break;
    default:
      zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));

  return ZB_TRUE;
}

static void send_active_ep_resp(zb_uint8_t param)
{
  zb_uint8_t status = ZB_ZDP_STATUS_SUCCESS;
  zb_apsde_data_indication_t ind;
  zb_zdo_active_ep_req_t req;
  zb_uint8_t *aps_body;
  zb_uint8_t tsn;
  zb_uint16_t nwk_addr;
  zb_uint8_t *ptr = NULL;
  zb_uint8_t  len = 0;

  TRACE_MSG(TRACE_ZDO3, "zdo_active_ep_res %hd", (FMT__H, param));

  aps_body = zb_buf_begin(param);
  tsn = *aps_body;
  aps_body++;

  ZB_MEMCPY(&req, (zb_zdo_active_ep_req_t *)aps_body, sizeof(req));
  ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
  ZB_HTOLE16(&nwk_addr, &req.nwk_addr);

  len  = 1+1+2+1+1; /* TSN + Status NWK_addr + ep_count + ep_list */

  ptr = zb_buf_initial_alloc(param, len);
  *ptr = tsn;
  ptr++;
  *ptr = status;
  ptr++;

  ptr = zb_put_next_htole16(ptr, nwk_addr);

  *ptr = 1;
  ptr++;

  *ptr = THE1_ENDPOINT;
  ptr++;

  zdo_send_resp_by_short(ZDO_ACTIVE_EP_RESP_CLID, param, ind.src_addr);
}

/*! @} */
