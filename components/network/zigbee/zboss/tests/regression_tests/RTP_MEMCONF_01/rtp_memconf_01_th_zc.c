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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_MEMCONF_01_TH_ZC

#define ZB_TRACE_FILE_ID 40315
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_memconf_01_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_th_zc = IEEE_ADDR_TH_ZC;
static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void trigger_steering(zb_uint8_t unused);
static void test_send_mgmt_bind_req_delayed(zb_uint8_t unused);
static void test_send_mgmt_bind_req(zb_uint8_t param);
static void test_mgmt_bind_resp_cb(zb_uint8_t param);

static zb_uint8_t g_start_idx = 0;

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_th_zc");

  zb_set_long_address(g_ieee_addr_th_zc);

zb_set_pan_id(0x1aaa);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

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
        ZB_SCHEDULE_CALLBACK(trigger_steering, 0);

        test_step_register(test_send_mgmt_bind_req_delayed, 0, RTP_MEMCONF_01_STEP_1_TIME_ZC);
        test_step_register(test_send_mgmt_bind_req_delayed, 0, RTP_MEMCONF_01_STEP_2_TIME_ZC);

        test_control_start(TEST_MODE, RTP_MEMCONF_01_STEP_1_DELAY_ZC);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void test_send_mgmt_bind_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(test_send_mgmt_bind_req);
}

static void test_send_mgmt_bind_req(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>test_send_mgmt_bind_req: param = %i", (FMT__D, param));

  req_params = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_bind_param_t);
  req_params->start_index = g_start_idx;
  req_params->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut_zr);
  zb_zdo_mgmt_bind_req(param, test_mgmt_bind_resp_cb);

  TRACE_MSG(TRACE_ZDO3, "<<test_send_mgmt_bind_req", (FMT__0));
}

static void test_mgmt_bind_resp_cb(zb_uint8_t param)
{
  zb_zdo_mgmt_bind_resp_t *resp;
  zb_callback_t call_cb = 0;

  TRACE_MSG(TRACE_ZDO1, ">>test_mgmt_bind_resp_cb: param = %i", (FMT__D, param));

  resp = (zb_zdo_mgmt_bind_resp_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, "test_mgmt_bind_resp_cb: status = %i", (FMT__D, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t nbrs = resp->binding_table_list_count;

    g_start_idx += nbrs;
    if (g_start_idx < resp->binding_table_entries)
    {
      TRACE_MSG(TRACE_ZDO2, "test_mgmt_bind_resp_cb: retrieved = %d, total = %d",
                (FMT__D_D, nbrs, resp->binding_table_entries));
      call_cb = test_send_mgmt_bind_req;
    }
    else
    {
      TRACE_MSG(TRACE_ZDO2, "test_mgmt_bind_resp_cb: retrieved all entries - %d",
                (FMT__D, resp->binding_table_entries));
      g_start_idx = 0;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "test_mgmt_bind_resp_cb: TEST_FAILED", (FMT__0));
  }

  if (call_cb)
  {
    zb_buf_reuse(param);
    ZB_SCHEDULE_CALLBACK(call_cb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_ZDO1, "<<test_mgmt_bind_resp_cb", (FMT__0));
}

/*! @} */
