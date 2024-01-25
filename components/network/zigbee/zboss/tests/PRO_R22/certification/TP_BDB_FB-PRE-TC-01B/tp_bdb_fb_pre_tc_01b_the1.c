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
/* PURPOSE: TH ZED1 (initiator)
*/


#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_01B_THE1
#define ZB_TRACE_FILE_ID 40697

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

#ifndef ZB_ED_ROLE
#error End Device role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_01b_the1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_01b_the1_identify_attr_list, &attr_identify_time);

/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(fb_pre_tc_01b_the1_on_off_controller_clusters,
                                   fb_pre_tc_01b_the1_basic_attr_list,
                                   fb_pre_tc_01b_the1_identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(fb_pre_tc_01b_the1_on_off_controller_ep,
                         THE1_ENDPOINT,
                         fb_pre_tc_01b_the1_on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(fb_pre_tc_01b_the1_on_off_controller_ctx, fb_pre_tc_01b_the1_on_off_controller_ep);

extern void zdo_send_resp_by_short(zb_uint16_t command_id,
                                   zb_uint8_t param,
                                   zb_uint16_t addr);

static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);
static void send_ieee_addr_resp(zb_uint8_t param);
static void trigger_fb_target(zb_uint8_t unused);

static zb_uint16_t g_parent_addr;
static int g_ieee_resp_format;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_the1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_the1);

  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);

  ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_01b_the1_on_off_controller_ctx);

  g_ieee_resp_format = 0;

  ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
  ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;

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

  zb_bdb_finding_binding_target(THE1_ENDPOINT);
}

static zb_bool_t zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{
  zb_bool_t status = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO1, ">> zdo_rx_handler: buf_param = %hd", (FMT__H, param));

  switch (cluster_id)
  {
    case ZDO_IEEE_ADDR_REQ_CLID:
      send_ieee_addr_resp(param);
      status = ZB_TRUE;
      break;
    default:
      TRACE_MSG(TRACE_ZDO1, " ZDO command is received, cluster_id %x", (FMT__D, cluster_id));
  }

  TRACE_MSG(TRACE_ZDO1, "<< zdo_rx_handler, status %hd", (FMT__H, status));

  return status;
}

static void send_ieee_addr_resp(zb_uint8_t param)
{
  zb_zdo_ieee_addr_resp_t *resp;
  zb_apsde_data_indication_t ind;
  zb_uint8_t *packet;
  zb_uint8_t status;
  zb_uint16_t nwk_addr;

  TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_resp: buf_param = %d, resp_type = %d",
            (FMT__D_D, param, g_ieee_resp_format));

  packet = zb_buf_begin(param);
  ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t), sizeof(ind));
  packet = zb_buf_initial_alloc(param, sizeof(zb_zdo_ieee_addr_resp_t));
  resp = (zb_zdo_ieee_addr_resp_t*) packet;

  if (!g_ieee_resp_format)
  {
    status = ZB_ZDP_STATUS_SUCCESS;
    /* nwk_addr = g_parent_addr; */
    nwk_addr = 0xabcd;
  }
  else
  {
    status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
    nwk_addr = zb_cert_test_get_network_addr();
  }

  resp->tsn = ind.tsn;
  resp->status = status;
  ZB_IEEE_ADDR_COPY(resp->ieee_addr_remote_dev, g_ieee_addr_the1);
  ZB_HTOLE16((zb_uint8_t*) &resp->nwk_addr_remote_dev, &nwk_addr);

  zdo_send_resp_by_short(ZDO_IEEE_ADDR_RESP_CLID, param, ind.src_addr);

  g_ieee_resp_format = !g_ieee_resp_format;

  TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_resp", (FMT__0));
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
        g_parent_addr = zb_cert_test_get_parent_short_addr();

        ZB_BDB().bdb_commissioning_time = FB_TARGET_DURATION1;
        ZB_SCHEDULE_ALARM(trigger_fb_target, 0, THE1_FB_DELAY1);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED:
      TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_FINDING_AND_BINDING_TARGET_FINISHED */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

/*! @} */
