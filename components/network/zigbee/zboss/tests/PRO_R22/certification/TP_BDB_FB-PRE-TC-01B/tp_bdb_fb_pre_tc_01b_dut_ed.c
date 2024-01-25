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

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_01B_DUT_ED
#define ZB_TRACE_FILE_ID 40052

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
#include "tp_bdb_fb_pre_tc_01b_common.h"
#include "../common/zb_cert_test_globals.h"

#include "zb_console_monitor.h"
#ifndef ZB_ED_ROLE
#error End device role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define CERTIFICATION_HACKS!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_01b_dut_ed_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_01b_dut_ed_identify_attr_list, &attr_identify_time);

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(fb_pre_tc_01b_dut_ed_on_off_attr_list, &attr_on_off);

/********************* Declare device **************************/
DECLARE_ON_OFF_SERVER_CLUSTER_LIST(fb_pre_tc_01b_dut_ed_on_off_device_clusters,
                                   fb_pre_tc_01b_dut_ed_basic_attr_list,
                                   fb_pre_tc_01b_dut_ed_identify_attr_list,
                                   fb_pre_tc_01b_dut_ed_on_off_attr_list);

DECLARE_ON_OFF_SERVER_EP(fb_pre_tc_01b_dut_ed_on_off_device_ep,
                         DUT_ENDPOINT,
                         fb_pre_tc_01b_dut_ed_on_off_device_clusters);

DECLARE_ON_OFF_SERVER_CTX(fb_pre_tc_01b_dut_ed_on_off_device_ctx, fb_pre_tc_01b_dut_ed_on_off_device_ep);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);


MAIN()
{
  ARGV_UNUSED;

  char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_dut");
#if UART_CONTROL	
	test_control_init();
#endif


  zb_set_long_address(g_ieee_addr_dut);

  zb_set_network_ed_role((1l << TEST_CHANNEL));

  ZB_CERT_HACKS().force_ext_addr_req = 1;
  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_01b_dut_ed_on_off_device_ctx);

  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

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
  zb_bdb_finding_binding_initiator(DUT_ENDPOINT, finding_binding_cb);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        ZB_BDB().bdb_commissioning_time = FB_INITIATOR_DURATION;
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_DELAY1);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
      ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_DELAY2);
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
