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
/* PURPOSE: TH ZR1 (target)
*/


#define ZB_TEST_NAME FB_CDP_TC_01_THR2
#define ZB_TRACE_FILE_ID 41032
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
#include "fb_cdp_tc_01_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */


/******************* Declare attributes ************************/


static zb_uint8_t attr_zcl_version_ep1  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_ep1 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
static zb_uint16_t attr_identify_time_ep1 = 0;
static zb_bool_t attr_on_off_ep1 = 0;


static zb_uint8_t attr_zcl_version_ep2  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source_ep2 = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
static zb_uint16_t attr_identify_time_ep2 = 0;
static zb_bool_t attr_on_off_ep2 = 1;


ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_ep1, &attr_zcl_version_ep1, &attr_power_source_ep1);
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list_ep2, &attr_zcl_version_ep2, &attr_power_source_ep2);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_ep1, &attr_identify_time_ep1);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list_ep2, &attr_identify_time_ep2);

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_ep1, &attr_on_off_ep1);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list_ep2, &attr_on_off_ep2);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(on_off_device_clusters_ep1,
                                   basic_attr_list_ep1,
                                   identify_attr_list_ep1,
                                   on_off_attr_list_ep1);

DECLARE_ON_OFF_SERVER_CLUSTER_LIST(on_off_device_clusters_ep2,
                                   basic_attr_list_ep2,
                                   identify_attr_list_ep2,
                                   on_off_attr_list_ep2);

ZB_DECLARE_SIMPLE_DESC(3, 0);


ZB_AF_SIMPLE_DESC_TYPE(3, 0) simple_desc_ep1 =
{
  THR2_ENDPOINT1,
  ZB_AF_HA_PROFILE_ID,
  DEVICE_ID_ON_OFF_SERVER,
  DEVICE_VER_ON_OFF_SERVER,
  0,
  3,
  0,
  {
    ZB_ZCL_CLUSTER_ID_BASIC,
    ZB_ZCL_CLUSTER_ID_IDENTIFY,
    ZB_ZCL_CLUSTER_ID_ON_OFF
  }
};

ZB_AF_SIMPLE_DESC_TYPE(3, 0) simple_desc_ep2 =
{
  THR2_ENDPOINT2,
  ZB_AF_HA_PROFILE_ID,
  DEVICE_ID_ON_OFF_SERVER,
  DEVICE_VER_ON_OFF_SERVER,
  0,
  3,
  0,
  {
    ZB_ZCL_CLUSTER_ID_BASIC,
    ZB_ZCL_CLUSTER_ID_IDENTIFY,
    ZB_ZCL_CLUSTER_ID_ON_OFF
  }
};

ZB_AF_START_DECLARE_ENDPOINT_LIST(device_ep_list)
ZB_AF_SET_ENDPOINT_DESC(THR2_ENDPOINT1, ZB_AF_HA_PROFILE_ID,
  0,
  NULL,
  ZB_ZCL_ARRAY_SIZE(on_off_device_clusters_ep1, zb_zcl_cluster_desc_t),
  on_off_device_clusters_ep1,
  (zb_af_simple_desc_1_1_t*)&simple_desc_ep1),
ZB_AF_SET_ENDPOINT_DESC(THR2_ENDPOINT2, ZB_AF_HA_PROFILE_ID,
  0,
  NULL,
  ZB_ZCL_ARRAY_SIZE(on_off_device_clusters_ep2, zb_zcl_cluster_desc_t),
  on_off_device_clusters_ep2,
  (zb_af_simple_desc_1_1_t*)&simple_desc_ep2)
ZB_AF_FINISH_DECLARE_ENDPOINT_LIST;

DECLARE_ON_OFF_SERVER_CTX(on_off_device_ctx, device_ep_list);


static void trigger_fb_target(zb_uint8_t unused);
static void add_group(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr2");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr2);
  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_AIB().aps_use_nvram = 1;

  /* Assignment required to force Distributed formation */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ;
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_device_ctx);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
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
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_apsme_add_group_req_t *req_param = NULL;

  req_param = ZB_GET_BUF_PARAM(buf, zb_apsme_add_group_req_t);
  ZB_BZERO(req_param, sizeof(*req_param));
  TRACE_MSG(TRACE_ZDO3, "Add group 0x%x", (FMT__H, GROUP_ADDRESS));

  req_param->endpoint = THR2_ENDPOINT2;
	req_param->group_address = GROUP_ADDRESS;
	zb_apsme_add_group_request(param);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr2, ZB_NIB().extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_ALARM(trigger_fb_target, 0, TEST_STARTUP_DELAY);
        }
        ZB_GET_OUT_BUF_DELAYED(add_group);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, TEST_STARTUP_DELAY);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
        TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
