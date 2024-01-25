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
/* PURPOSE: DUT ZC (binding table size)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_07_DUT_C
#define ZB_TRACE_FILE_ID 40798

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
#include "zb_console_monitor.h"

#include "device_dut.h"
#include "tp_bdb_fb_pre_tc_07_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compliled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compliled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_07_dut_c_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_07_dut_c_identify_attr_list, &attr_identify_time);

/* Temperature Measurement attributes data */
static zb_int16_t  attr_temp_value     = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
static zb_int16_t  attr_min_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_MIN_VALUE;
static zb_int16_t  attr_max_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_MAX_VALUE;
static zb_uint16_t attr_temp_tolerance = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_MAX_VALUE;
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(fb_pre_tc_07_dut_c_temp_meas_attr_list,
                                            &attr_temp_value,
                                            &attr_min_temp_value,
                                            &attr_max_temp_value,
                                            &attr_temp_tolerance);

static zb_uint16_t attr_illum_value     = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MEASURED_VALUE_DEFAULT;
static zb_uint16_t attr_min_illum_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MIN_MEASURED_VALUE_MIN_VALUE;
static zb_uint16_t attr_max_illum_value = ZB_ZCL_ATTR_ILLUMINANCE_MEASUREMENT_MAX_MEASURED_VALUE_MAX_VALUE;
ZB_ZCL_DECLARE_ILLUMINANCE_MEASUREMENT_ATTRIB_LIST(fb_pre_tc_07_dut_c_illum_meas_attr_list,
                                                   &attr_illum_value,
                                                   &attr_min_illum_value,
                                                   &attr_max_illum_value);

static zb_uint16_t attr_rel_humidity_value     = ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_UNKNOWN;
static zb_uint16_t attr_min_rel_humidity_value = ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_MIN_VALUE_MAX_VALUE;
static zb_uint16_t attr_max_rel_humidity_value = ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_MAX_VALUE_MIN_VALUE;
ZB_ZCL_DECLARE_REL_HUMIDITY_MEASUREMENT_ATTRIB_LIST(fb_pre_tc_07_dut_c_rel_humidity_meas_attr_list,
                                                   &attr_rel_humidity_value,
                                                   &attr_min_rel_humidity_value,
                                                   &attr_max_rel_humidity_value);


/********************* Declare device **************************/
DECLARE_DUT_CLUSTER_LIST(fb_pre_tc_07_dut_c_device_clusters,
                         fb_pre_tc_07_dut_c_basic_attr_list,
                         fb_pre_tc_07_dut_c_identify_attr_list,
                         fb_pre_tc_07_dut_c_temp_meas_attr_list,
                         fb_pre_tc_07_dut_c_illum_meas_attr_list,
                         fb_pre_tc_07_dut_c_rel_humidity_meas_attr_list);

DECLARE_DUT_EP(fb_pre_tc_07_dut_c_device_ep,
               DUT_ENDPOINT,
               fb_pre_tc_07_dut_c_device_clusters);

DECLARE_DUT_CTX(fb_pre_tc_07_dut_c_device_ctx, fb_pre_tc_07_dut_c_device_ep);
/*************************************************************************/

/*******************Definitions for Test***************************/

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

  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_07_dut_c_device_ctx);

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
  ZB_BDB().bdb_commissioning_time = DUT_FB_DURATION;
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
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, DUT_FB_INITIATOR_DELAY);
      }
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


/*! @} */
