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

#define ZB_TEST_NAME RTP_HA_04_DUT_ZC

#define ZB_TRACE_FILE_ID 40477
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

#include "ha/zb_ha_door_lock.h"

#include "rtp_ha_04_common.h"
#include "../common/zb_reg_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compliled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_ha_04_dut_zc_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_ha_04_dut_zc_identify_attr_list, &attr_identify_time);

zb_zcl_attr_t rtp_ha_04_dut_zc_door_lock_attr_list[] = { {ZB_ZCL_NULL_ID, 0, 0, NULL} };
zb_zcl_attr_t rtp_ha_04_dut_zc_groups_attr_list[] = { {ZB_ZCL_NULL_ID, 0, 0, NULL} };
zb_zcl_attr_t rtp_ha_04_dut_zc_scenes_attr_list[] = { {ZB_ZCL_NULL_ID, 0, 0, NULL} };

/********************* Declare device **************************/

ZB_HA_DECLARE_DOOR_LOCK_CLUSTER_LIST(rtp_ha_04_dut_zc_device_clusters,
                                     rtp_ha_04_dut_zc_door_lock_attr_list,
                                     rtp_ha_04_dut_zc_basic_attr_list,
                                     rtp_ha_04_dut_zc_identify_attr_list,
                                     rtp_ha_04_dut_zc_groups_attr_list,
                                     rtp_ha_04_dut_zc_scenes_attr_list);

ZB_HA_DECLARE_DOOR_LOCK_EP(rtp_ha_04_dut_zc_device_ep,
                           DUT_ENDPOINT,
                           rtp_ha_04_dut_zc_device_clusters);

ZB_HA_DECLARE_DOOR_LOCK_CTX(rtp_ha_04_dut_zc_device_ctx, rtp_ha_04_dut_zc_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

static void trigger_fb_target(zb_uint8_t unused);

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut_zc");


  zb_set_long_address(g_ieee_addr_dut_zc);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_ha_04_dut_zc_device_ctx);

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
        ZB_SCHEDULE_CALLBACK(trigger_fb_target, 0);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_fb_target(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  ZB_BDB().bdb_commissioning_time = TEST_DUT_FB_DURATION;
  zb_bdb_finding_binding_target(DUT_ENDPOINT);
}

/*! @} */
