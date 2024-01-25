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
/* PURPOSE: DUT ZED (target)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_04B_DUT_ED
#define ZB_TRACE_FILE_ID 40833

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
#include "test_device_target.h"
#include "tp_bdb_fb_pre_tc_04b_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ED_ROLE
#error ED role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error Define ZB_USE_NVRAM
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04b_dut_ed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04b_dut_ed_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_04b_dut_ed_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST(fb_pre_tc_04b_dut_ed_target_device_clusters,
                            fb_pre_tc_04b_dut_ed_basic_attr_list,
                            fb_pre_tc_04b_dut_ed_identify_attr_list,
                            fb_pre_tc_04b_dut_ed_on_off_attr_list);

DECLARE_TARGET_EP(fb_pre_tc_04b_dut_ed_target_device_ep,
                  DUT_ENDPOINT,
                  fb_pre_tc_04b_dut_ed_target_device_clusters);

DECLARE_TARGET_CTX(fb_pre_tc_04b_dut_ed_target_device_ctx, fb_pre_tc_04b_dut_ed_target_device_ep);


static void trigger_fb_target(zb_uint8_t unused);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_set_long_address(g_ieee_addr_dut);
  /*
  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;

  zb_cert_test_set_aps_use_nvram();
  */
	zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);
	
  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_04b_dut_ed_target_device_ctx);

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
  ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION;
  zb_bdb_finding_binding_target(DUT_ENDPOINT);
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
        ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION;
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, DUT_FB_TARGET_DELAY);
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
  zb_buf_free(param);
}


/*! @} */
