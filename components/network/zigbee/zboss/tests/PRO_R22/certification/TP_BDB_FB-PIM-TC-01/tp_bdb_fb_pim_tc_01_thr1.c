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
/* PURPOSE: TH ZR1 (initiator)
*/

#define ZB_TEST_NAME TP_BDB_FB_PIM_TC_01_THR1
#define ZB_TRACE_FILE_ID 40544

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
#include "tp_bdb_fb_pim_tc_01_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pim_tc_01_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pim_tc_01_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pim_tc_01_thr1_on_off_controller_clusters,
                                   fb_pim_tc_01_thr1_basic_attr_list,
                                   fb_pim_tc_01_thr1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pim_tc_01_thr1_on_off_controller_ep,
                         THR1_ENDPOINT,
                         fb_pim_tc_01_thr1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pim_tc_01_thr1_on_off_controller_ctx, fb_pim_tc_01_thr1_on_off_controller_ep);


enum used_profile_id_e
{
  USED_PROFILE_ID_0104,
  USED_PROFILE_ID_WILDCARD,
  USED_PROFILE_ID_ZSE,
  USED_PROFILE_ID_GW,
  USED_PROFILE_ID_MSP,
  USED_PROFILE_ID_COUNT
};


static const zb_uint16_t s_arr_prof_id[] =
{
  0x0104, 0xffff, 0x0109, 0x7f02, 0x8090
};

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);

static void send_read_attr_req(zb_uint8_t param);
static zb_uint8_t zcl_app_cmd_handler(zb_uint8_t param);
static void read_attr_resp_timeout(zb_uint8_t unused);

static zb_uint8_t s_target_ep;
static zb_uint16_t s_target_cluster;
static int s_matching_clusters;
/* test step index - matching with index of s_arr_prof_id */
static int s_step_idx = USED_PROFILE_ID_0104;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Assignment required to force Distributed formation */
  zb_aib_set_trust_center_address(g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pim_tc_01_thr1_on_off_controller_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, zcl_app_cmd_handler);

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
  if (status == ZB_BDB_COMM_BIND_ASK_USER)
  {
    s_target_ep = ep;
    s_target_cluster = cluster;
    ++s_matching_clusters;
  }
  return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}


static void send_read_attr_req(zb_uint8_t param)
{
  zb_uint8_t *cmd_ptr;
  zb_uint16_t addr = zb_address_short_by_ieee(g_ieee_addr_dut);

  TRACE_MSG(TRACE_ZCL1, ">>send_read_attr_req: buf = %d, target prof_id = 0x%x",
            (FMT__D_H, param, s_arr_prof_id[s_step_idx]));
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, 0x0000);
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(param, cmd_ptr,
                                   addr,
                                   ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT /* ZB_APS_ADDR_MODE_16_ENDP_PRESENT */,
                                   s_target_ep,
                                   THR1_ENDPOINT,
                                   s_arr_prof_id[s_step_idx],
                                   s_target_cluster,
                                   NULL);
  ZB_SCHEDULE_ALARM(read_attr_resp_timeout, 0, CMD_RESP_TIMEOUT);

  TRACE_MSG(TRACE_ZCL1, "<<send_read_attr_req", (FMT__0));
}


static zb_uint8_t zcl_app_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_bool_t processed = ZB_FALSE;


  TRACE_MSG(TRACE_ZCL1, ">>zcl_app_cmd_handler: buf = %i", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, zb_buf_len(param)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    if (cmd_info->is_common_command)
    {
      if (cmd_info->cmd_id == ZB_ZCL_CMD_READ_ATTRIB_RESP)
      {
        ZB_SCHEDULE_ALARM_CANCEL(read_attr_resp_timeout, ZB_ALARM_ANY_PARAM);
        ++s_step_idx;
        if (s_step_idx < USED_PROFILE_ID_COUNT)
        {
          zb_buf_get_out_delayed(send_read_attr_req);
        }
        else
        {
          TRACE_MSG(TRACE_ZCL1, "TEST FINISHED", (FMT__0));
        }
        processed = ZB_TRUE;
      }
    }
  }

  TRACE_MSG(TRACE_ZCL1, "<<zcl_app_cmd_handler", (FMT__0));

  return processed;
}


static void read_attr_resp_timeout(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZCL1, ">>read_attr_resp_timeout: prof_id = %d",
            (FMT__D, s_arr_prof_id[s_step_idx]));

  ++s_step_idx;
  if (s_step_idx < USED_PROFILE_ID_COUNT)
  {
    zb_buf_get_out_delayed(send_read_attr_req);
  }
  else
  {
    TRACE_MSG(TRACE_ZCL1, "TEST FINISHED", (FMT__0));
  }

  TRACE_MSG(TRACE_ZCL1, "<<read_attr_resp_timeout", (FMT__0));
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

        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_TRIGGER_FB_INITIATOR_DELAY);
        }
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      TRACE_MSG(TRACE_APS1, "signal: ZB_ZDO_SIGNAL_DEVICE_ANNCE, status %d", (FMT__D, status));
      ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, TEST_TRIGGER_FB_INITIATOR_DELAY);
      break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED, status %d", (FMT__D, status));
      if (status == 0)
      {
        zb_buf_get_out_delayed(send_read_attr_req);
      }
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
