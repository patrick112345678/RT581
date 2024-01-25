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

#define ZB_TEST_NAME TP_BDB_FB_CDP_TC_01_DUT_C
#define ZB_TRACE_FILE_ID 40889

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
#include "tp_bdb_fb_cdp_tc_01_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_cdp_tc_01_dut_c_basic_attr_list, &attr_zcl_version, &attr_power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_cdp_tc_01_dut_c_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_cdp_tc_01_dut_c_on_off_controller_clusters,
                                   fb_cdp_tc_01_dut_c_basic_attr_list,
                                   fb_cdp_tc_01_dut_c_identify_attr_list);

DECLARE_ON_OFF_CLIENT_SIMPLE_DESC(fb_cdp_tc_01_dut_c_ep1, DUT_ENDPOINT1,
                                  ON_OFF_CLIENT_IN_CLUSTER_NUM, ON_OFF_CLIENT_OUT_CLUSTER_NUM);

DECLARE_ON_OFF_CLIENT_EP(fb_cdp_tc_01_dut_c_ep1, DUT_ENDPOINT1,
                         fb_cdp_tc_01_dut_c_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_cdp_tc_01_dut_c_on_off_controller_ctx,
                              fb_cdp_tc_01_dut_c_ep1);

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
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_dut);

  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_cdp_tc_01_dut_c_on_off_controller_ctx);

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
  zb_buf_get_out_delayed(send_read_attr_req);
}


static void send_read_attr_req(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_uint16_t addr = 0x0000;

  TRACE_MSG(TRACE_ZCL1, ">>send_read_attr_req: buf = %d", (FMT__D, param));
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, 0x0000);
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(param, cmd_ptr,
                                    addr,
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
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_STARTUP_DELAY + TEST_SEND_CMD_SKEW);
        ZB_SCHEDULE_ALARM(send_read_attr_req_delayed, 0,
                          TEST_DUT_START_COMMUNICATION_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
