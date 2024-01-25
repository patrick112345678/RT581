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
/* PURPOSE: TH ZR2 (target)
*/

#define ZB_TEST_NAME TP_BDB_FB_CDP_TC_01_THR2
#define ZB_TRACE_FILE_ID 40888

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

#include "on_off_server.h"
#include "tp_bdb_fb_cdp_tc_01_common.h"
#include "../common/zb_cert_test_globals.h"

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

/******************* Declare attributes ************************/

static zb_ieee_addr_t g_ieee_addr_thr2 = IEEE_ADDR_THR2;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint8_t attr_zcl_version_ep1  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_ep1 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
static zb_uint16_t attr_identify_time_ep1 = 0;
static zb_bool_t attr_on_off_ep1 = 0;


static zb_uint8_t attr_zcl_version_ep2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_ep2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
static zb_uint16_t attr_identify_time_ep2 = 0;
static zb_bool_t attr_on_off_ep2 = 1;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_cdp_tc_01_thr2_basic_attr_list_ep1,
                                 &attr_zcl_version_ep1, &attr_power_source_ep1);
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_cdp_tc_01_thr2_basic_attr_list_ep2,
                                 &attr_zcl_version_ep2, &attr_power_source_ep2);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_cdp_tc_01_thr2_identify_attr_list_ep1, &attr_identify_time_ep1);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_cdp_tc_01_thr2_identify_attr_list_ep2, &attr_identify_time_ep2);

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_cdp_tc_01_thr2_on_off_attr_list_ep1, &attr_on_off_ep1);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_cdp_tc_01_thr2_on_off_attr_list_ep2, &attr_on_off_ep2);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(fb_cdp_tc_01_thr2_on_off_device_clusters_ep1,
                                   fb_cdp_tc_01_thr2_basic_attr_list_ep1,
                                   fb_cdp_tc_01_thr2_identify_attr_list_ep1,
                                   fb_cdp_tc_01_thr2_on_off_attr_list_ep1);

DECLARE_ON_OFF_SERVER_CLUSTER_LIST(fb_cdp_tc_01_thr2_on_off_device_clusters_ep2,
                                   fb_cdp_tc_01_thr2_basic_attr_list_ep2,
                                   fb_cdp_tc_01_thr2_identify_attr_list_ep2,
                                   fb_cdp_tc_01_thr2_on_off_attr_list_ep2);

ZB_DECLARE_SIMPLE_DESC(3, 1);
DECLARE_SERVER_SIMPLE_DESC(fb_cdp_tc_01_thr2_ep1, THR2_ENDPOINT1,
                           ON_OFF_SERVER_IN_CLUSTER_NUM, ON_OFF_SERVER_OUT_CLUSTER_NUM);
DECLARE_SERVER_SIMPLE_DESC(fb_cdp_tc_01_thr2_ep2, THR2_ENDPOINT2,
                           ON_OFF_SERVER_IN_CLUSTER_NUM, ON_OFF_SERVER_OUT_CLUSTER_NUM);

DECLARE_SERVER_EP(fb_cdp_tc_01_thr2_ep1, THR2_ENDPOINT1, fb_cdp_tc_01_thr2_on_off_device_clusters_ep1);
DECLARE_SERVER_EP(fb_cdp_tc_01_thr2_ep2, THR2_ENDPOINT2, fb_cdp_tc_01_thr2_on_off_device_clusters_ep2);

DECLARE_ON_OFF_SERVER_CTX(fb_cdp_tc_01_thr2_on_off_device_ctx,
                          fb_cdp_tc_01_thr2_ep1, fb_cdp_tc_01_thr2_ep2);

static void trigger_fb_target(zb_uint8_t unused);
static void add_group(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr2);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Assignment required to force Distributed formation */
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_cdp_tc_01_thr2_on_off_device_ctx);

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

static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_BDB().bdb_commissioning_time = TEST_FB_TARGET_DURATION;
  zb_bdb_finding_binding_target(THR2_ENDPOINT1);
}


static void add_group(zb_uint8_t param)
{
  zb_apsme_add_group_req_t *req_param = NULL;

  req_param = ZB_BUF_GET_PARAM(param, zb_apsme_add_group_req_t);
  ZB_BZERO(req_param, sizeof(*req_param));
  TRACE_MSG(TRACE_ZDO3, "Add group 0x%x", (FMT__H, GROUP_ADDRESS));

  req_param->endpoint = THR2_ENDPOINT2;
        req_param->group_address = GROUP_ADDRESS;
        zb_apsme_add_group_request(param);
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

        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr2, extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
        }
        zb_buf_get_out_delayed(add_group);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
