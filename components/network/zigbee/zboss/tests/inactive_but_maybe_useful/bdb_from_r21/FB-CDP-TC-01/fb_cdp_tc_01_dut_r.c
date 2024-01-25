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
/* PURPOSE: DUT ZED (initiator)
*/


#define ZB_TEST_NAME FB_CDP_TC_01_DUT_R
#define ZB_TRACE_FILE_ID 41029
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
#include "temp_meas_server.h"
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

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

/* Temperature measurements cluster attributes data */
static zb_int16_t attr_temp_value = 0;
static zb_int16_t attr_min_temp_value = 0;
static zb_int16_t attr_max_temp_value = 0;


ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_meas_attr_list,
                                            &attr_temp_value,
                                            &attr_min_temp_value,
                                            &attr_max_temp_value,
                                            NULL);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(on_off_controller_clusters,
                                   basic_attr_list,
                                   identify_attr_list);


DECLARE_TEMP_MEAS_SERVER_CLUSTER_LIST(temp_meas_server_clusters,
                                      temp_meas_attr_list);

ZB_DECLARE_SIMPLE_DESC(2, 1);
ZB_DECLARE_SIMPLE_DESC(1, 0);

ZB_AF_SIMPLE_DESC_TYPE(ON_OFF_CLIENT_IN_CLUSTER_NUM, ON_OFF_CLIENT_OUT_CLUSTER_NUM) simple_desc_ep1 =
{
  DUT_ENDPOINT1,
  ZB_AF_HA_PROFILE_ID,
  DEVICE_ID_ON_OFF_CLIENT,
  DEVICE_VER_ON_OFF_CLIENT,
  0,
  ON_OFF_CLIENT_IN_CLUSTER_NUM,
  ON_OFF_CLIENT_OUT_CLUSTER_NUM,
  {
    ZB_ZCL_CLUSTER_ID_BASIC,
    ZB_ZCL_CLUSTER_ID_IDENTIFY,
    ZB_ZCL_CLUSTER_ID_ON_OFF
  }
};

ZB_AF_SIMPLE_DESC_TYPE(TEMP_MEAS_SERVER_IN_CLUSTER_NUM, TEMP_MEAS_SERVER_OUT_CLUSTER_NUM) simple_desc_ep2 =
{
  DUT_ENDPOINT2,
  ZB_AF_HA_PROFILE_ID,
  DEVICE_ID_TEMP_MEAS_SERVER,
  DEVICE_VER_TEMP_MEAS_SERVER,
  0,
  TEMP_MEAS_SERVER_IN_CLUSTER_NUM,
  TEMP_MEAS_SERVER_OUT_CLUSTER_NUM,
  {
    ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT
  }
};

ZB_AF_START_DECLARE_ENDPOINT_LIST(controller_ep_list)
ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT1, ZB_AF_HA_PROFILE_ID,
  0,
  NULL,
  ZB_ZCL_ARRAY_SIZE(on_off_controller_clusters, zb_zcl_cluster_desc_t),
  on_off_controller_clusters,
  (zb_af_simple_desc_1_1_t*)&simple_desc_ep1),
ZB_AF_SET_ENDPOINT_DESC(DUT_ENDPOINT2, ZB_AF_HA_PROFILE_ID,
  0,
  NULL,
  ZB_ZCL_ARRAY_SIZE(temp_meas_server_clusters, zb_zcl_cluster_desc_t),
  temp_meas_server_clusters,
  (zb_af_simple_desc_1_1_t*)&simple_desc_ep2)
ZB_AF_FINISH_DECLARE_ENDPOINT_LIST;

DECLARE_ON_OFF_CLIENT_CTX(on_off_controller_ctx, controller_ep_list);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);
static void send_read_attr_req_delayed(zb_uint8_t param);
static void send_read_attr_req(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_dut);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;

  /* Assignment required to force Distributed formation */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);
  ZB_AIB().aps_use_nvram = 1;

  ZB_AF_REGISTER_DEVICE_CTX(&on_off_controller_ctx);

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
  zb_bdb_finding_binding_initiator(DUT_ENDPOINT1, finding_binding_cb);
}


static void send_read_attr_req_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_GET_OUT_BUF_DELAYED(send_read_attr_req);
}


static void send_read_attr_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *cmd_ptr;

  TRACE_MSG(TRACE_ZCL1, ">>send_read_attr_req: buf = %d", (FMT__D, param));
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, 0x0000);
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf, cmd_ptr,
                                   0,
                                   ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
                                   0,
                                   DUT_ENDPOINT1,
                                   ZB_AF_HA_PROFILE_ID,
                                   DUT_MATCHING_CLUSTER,
                                   NULL);

  TRACE_MSG(TRACE_ZCL1, "<<send_read_attr_req", (FMT__0));
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
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_STARTUP_DELAY + TEST_SEND_CMD_SKEW);
        ZB_SCHEDULE_ALARM(send_read_attr_req_delayed, 0,
                          TEST_DUT_START_COMMUNICATION_DELAY);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
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
