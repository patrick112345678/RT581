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

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_01B_THR1
#define ZB_TRACE_FILE_ID 40698

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
#include "tp_bdb_fb_pre_tc_01b_common.h"
#include "../common/zb_cert_test_globals.h"

#include "zb_console_monitor.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not defined!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;

#if 0
static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;
#endif

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_01b_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_01b_thr1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pre_tc_01b_thr1_on_off_controller_clusters,
                                   fb_pre_tc_01b_thr1_basic_attr_list,
                                   fb_pre_tc_01b_thr1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pre_tc_01b_thr1_on_off_controller_ep,
                         THE1_ENDPOINT,
                         fb_pre_tc_01b_thr1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pre_tc_01b_thr1_on_off_controller_ctx, fb_pre_tc_01b_thr1_on_off_controller_ep);


extern void zdo_send_resp_by_short(zb_uint16_t command_id,
                                   zb_uint8_t param,
                                   zb_uint16_t addr);


/* static void disable_frame_retransmission(zb_uint8_t param); */
#if 0
static void enable_frame_retransmission(zb_uint8_t unused);

static zb_uint8_t identify_handler(zb_uint8_t param);
static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);
static void send_ieee_addr_resp(zb_uint8_t param);

static int s_ieee_resp_format;
#endif

static void start_top_level_commissioning(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  char command_buffer[100], *command_ptr;
  char next_cmd[40];
  zb_bool_t res;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");

#if UART_CONTROL	
	test_control_init();
#endif

  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_max_children(1);

  TRACE_MSG(TRACE_APP1, "Send 'erase' for flash erase or just press enter to be continued \n", (FMT__0));
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);
  zb_console_monitor_get_cmd((zb_uint8_t*)command_buffer, sizeof(command_buffer));
  command_ptr = (char *)(&command_buffer);
  res = parse_command_token(&command_ptr, next_cmd, sizeof(next_cmd));
  if (strcmp(next_cmd, "erase") == 0)
    zb_set_nvram_erase_at_start(ZB_TRUE);
  else
    zb_set_nvram_erase_at_start(ZB_FALSE);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_01b_thr1_on_off_controller_ctx);

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

#if 0
static void disable_frame_retransmission(zb_uint8_t param)
{
  s_ieee_resp_format = param;
  TRACE_MSG(TRACE_ZDO1, "Toggle frame retransmission", (FMT__0));

  ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, identify_handler);
}

static void enable_frame_retransmission(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO1, "Enable frame retransmission", (FMT__0));

  ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, NULL);
  ZB_CERT_HACKS().disable_frame_retransmission = 0;
  ZB_CERT_HACKS().force_frame_indication = 0;
  ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
  ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
}

static zb_uint8_t identify_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
  ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;
  ZB_CERT_HACKS().disable_frame_retransmission = 0;
  ZB_CERT_HACKS().force_frame_indication = 1;
  return ZB_FALSE;
}

static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{

  TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = %hd", (FMT__H, param));

  ZB_CERT_HACKS().disable_frame_retransmission = 1;
  ZB_CERT_HACKS().disable_frame_retransmission_countdown = 1;

  switch (cluster_id)
  {
    case ZDO_IEEE_ADDR_REQ_CLID:
      send_ieee_addr_resp(param);
      break;
    default:
      enable_frame_retransmission(0);
      zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));

  return ZB_TRUE;
}

static void send_ieee_addr_resp(zb_uint8_t param)
{
  zb_zdo_ieee_addr_resp_t *resp;
  zb_apsde_data_indication_t ind;
  zb_uint8_t *packet;
  zb_uint8_t status;
  zb_uint16_t nwk_addr;

  TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_resp: buf_param = %d, resp_type = %d",
            (FMT__D_D, param, s_ieee_resp_format));

  packet = zb_buf_begin(param);
  ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
  packet = zb_buf_initial_alloc(param, sizeof(zb_zdo_ieee_addr_resp_t));
  resp = (zb_zdo_ieee_addr_resp_t*) packet;

  if (!s_ieee_resp_format)
  {
    status = ZB_ZDP_STATUS_SUCCESS;
    nwk_addr = zb_cert_test_get_network_addr();
  }
  else
  {
    status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
    nwk_addr = zb_address_short_by_ieee(g_ieee_addr_the1);
  }

  resp->tsn = ind.tsn;
  resp->status = status;
  ZB_IEEE_ADDR_COPY(resp->ieee_addr_remote_dev, g_ieee_addr_the1);
  ZB_HTOLE16((zb_uint8_t*) &resp->nwk_addr_remote_dev, &nwk_addr);

  zdo_send_resp_by_short(ZDO_IEEE_ADDR_RESP_CLID, param, ind.src_addr);

  TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_resp", (FMT__0));
}
#endif

static void start_top_level_commissioning(zb_uint8_t param)
{
  bdb_start_top_level_commissioning(param);
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
        /* See negative test (test steps 3b and 4b) in the1 */

        /* ZB_SCHEDULE_ALARM_CANCEL(disable_frame_retransmission, ZB_ALARM_ALL_CB); */
        /* ZB_SCHEDULE_ALARM(disable_frame_retransmission, 0, THE1_FB_DELAY1); */
        /* ZB_SCHEDULE_ALARM(disable_frame_retransmission, 1, THE1_FB_DELAY2); */
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      if (status == 0)
      {
        ZB_SCHEDULE_CALLBACK(start_top_level_commissioning, ZB_BDB_NETWORK_STEERING);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
