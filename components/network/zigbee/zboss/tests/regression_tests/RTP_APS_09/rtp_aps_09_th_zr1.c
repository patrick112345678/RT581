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
/* PURPOSE: TH ZR1
*/

#define ZB_TEST_NAME RTP_APS_09_TH_ZR1

#define ZB_TRACE_FILE_ID 40110
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
#include "rtp_aps_09_common.h"
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
static zb_ieee_addr_t g_ieee_addr_th_zr1 = IEEE_ADDR_TH_ZR1;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_aps_09_th_zr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_aps_09_th_zr1_identify_attr_list, &attr_identify_time);

/* On/Off cluster attributes data */
static zb_bool_t attr_on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;
static zb_bool_t global_scene_ctrl = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
static zb_uint16_t on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
static zb_uint16_t off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(rtp_aps_09_th_zr1_on_off_attr_list,
                                      &attr_on_off,
                                      &global_scene_ctrl,
                                      &on_time,
                                      &off_wait_time);

/* Level control attributes data */
static zb_uint8_t attr_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
static zb_uint16_t attr_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(rtp_aps_09_th_zr1_level_control_attr_list,
                                         &attr_current_level, &attr_remaining_time);

/********************* Declare device **************************/
DECLARE_TH_CLUSTER_LIST(rtp_aps_09_th_zr1_device_clusters,
                        rtp_aps_09_th_zr1_basic_attr_list,
                        rtp_aps_09_th_zr1_identify_attr_list,
                        rtp_aps_09_th_zr1_on_off_attr_list,
                        rtp_aps_09_th_zr1_level_control_attr_list);

DECLARE_TH_EP(rtp_aps_09_th_zr1_device_ep,
               TH_ENDPOINT,
               rtp_aps_09_th_zr1_device_clusters);

DECLARE_TH_CTX(rtp_aps_09_th_zr1_device_ctx, rtp_aps_09_th_zr1_device_ep);
/*************************************************************************/

static zb_bool_t g_test_stop_th = ZB_TRUE;

static void test_device_cb(zb_uint8_t param);
static void test_zboss_main_loop_stop(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_th_zr1");


  zb_set_long_address(g_ieee_addr_th_zr1);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_aps_09_th_zr1_device_ctx);
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

  zb_set_max_children(0);

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

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_device_cb(zb_uint8_t param)
{
  zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
      TRACE_MSG(TRACE_APP1, "Entered SET_ATTR_VALUE", (FMT__0));
      if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
          device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
          (zb_bool_t)device_cb_param->cb_param.set_attr_value_param.values.data8)
      {
        if (g_test_stop_th)
        {
          ZB_SCHEDULE_ALARM(test_zboss_main_loop_stop, 0, ZB_TIME_ONE_SECOND);
        }
        g_test_stop_th = !g_test_stop_th;
      }
      break;

    default:
      TRACE_MSG(TRACE_APP1, "default for status = %d", (FMT__D, device_cb_param->status));
      break;
  }
}

static void test_zboss_main_loop_stop(zb_uint8_t unused)
{
  TRACE_MSG(TRACE_APP1, ">>test_zboss_main_loop_stop", (FMT__0));

  ZVUNUSED(unused);

  ZG->sched.stop = ZB_TRUE;

  osif_sleep_using_transc_timer(TH_SLEEP_TIME);

  TRACE_MSG(TRACE_APP1, "test_zboss_main_loop_stop(): main loop started", (FMT__0));

  ZG->sched.stop = ZB_FALSE;
}

/*! @} */
