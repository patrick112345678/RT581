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
/* PURPOSE: FB-PRE-TC-04A: Service discovery - server side tests (DUT ZC)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_04A_DUT_C
#define ZB_TRACE_FILE_ID 40906

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

#include "test_target.h"
#include "tp_bdb_fb_pre_tc_04a_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#if defined(USE_NVRAM_IN_TEST) && !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version_epx[2]  =
{
  ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
  ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE
};
static zb_uint8_t attr_power_source_epx[2] =
{
  ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
  ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE
};
/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time_epx[2];

static zb_int16_t attr_temp_value = 0;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04a_dut_c_basic_attr_list_ep1,
                                 &attr_zcl_version_epx[0],
                                 &attr_power_source_epx[0]);
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04a_dut_c_basic_attr_list_ep2,
                                 &attr_zcl_version_epx[1],
                                 &attr_power_source_epx[1]);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04a_dut_c_identify_attr_list_ep1,
                                    &attr_identify_time_epx[0]);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04a_dut_c_identify_attr_list_ep2,
                                    &attr_identify_time_epx[1]);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(fb_pre_tc_04a_dut_c_temp_meas_attr_list_ep2,
                                            &attr_temp_value, NULL, NULL, NULL);

/********************* Declare device **************************/
DECLARE_TARGET_CLUSTER_LIST_EP1(fb_pre_tc_04a_dut_c_target_clusters_ep1,
                                fb_pre_tc_04a_dut_c_basic_attr_list_ep1,
                                fb_pre_tc_04a_dut_c_identify_attr_list_ep1);

DECLARE_TARGET_CLUSTER_LIST_EP2(fb_pre_tc_04a_dut_c_target_clusters_ep2,
                                fb_pre_tc_04a_dut_c_basic_attr_list_ep2,
                                fb_pre_tc_04a_dut_c_identify_attr_list_ep2,
                                fb_pre_tc_04a_dut_c_temp_meas_attr_list_ep2);

DECLARE_TARGET_SIMPLE_DESC_EP1(fb_pre_tc_04a_dut_c_simple_desc_ep1, DUT_ENDPOINT1,
                               TARGET_EP1_IN_CLUSTER_NUM, TARGET_EP1_CLUSTER_NUM);

DECLARE_TARGET_SIMPLE_DESC_EP2(fb_pre_tc_04a_dut_c_simple_desc_ep2, DUT_ENDPOINT2,
                               TARGET_EP2_IN_CLUSTER_NUM, TARGET_EP2_CLUSTER_NUM);

DECLARE_TARGET_EP(fb_pre_tc_04a_dut_c_simple_desc_ep1, DUT_ENDPOINT1,
                  fb_pre_tc_04a_dut_c_target_clusters_ep1);
DECLARE_TARGET_EP(fb_pre_tc_04a_dut_c_simple_desc_ep2, DUT_ENDPOINT2,
                  fb_pre_tc_04a_dut_c_target_clusters_ep2);

DECLARE_TARGET_NO_REP_CTX(fb_pre_tc_04a_dut_c_target_device_ctx,
                          fb_pre_tc_04a_dut_c_simple_desc_ep1,
                          fb_pre_tc_04a_dut_c_simple_desc_ep2);

/*************************Other functions**********************************/
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

  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_04a_dut_c_target_device_ctx);

#ifdef ZB_USE_NVRAM
  zb_cert_test_set_aps_use_nvram();
#endif

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
  ZB_BDB().bdb_commissioning_time = FB_DURATION;
  zb_bdb_finding_binding_target(DUT_ENDPOINT1);
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
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, DUT_FB_START_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


/*! @} */
