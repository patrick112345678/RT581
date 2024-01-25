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
/* PURPOSE: TH ZR1 - joining to network, verifying DUT neighbor table
*/


#define ZB_TEST_NAME CS_NFS_TC_02_THR1
#define ZB_TRACE_FILE_ID 40959
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_nfs_tc_02_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error define ZB_SECURITY
#endif


enum test_state_e
{
  REQUEST_KEY_NOT_SEND,
  REQUEST_KEY_UNPROTECTED,
  REQUEST_KEY_WITH_MIC_ERROR,
  REQUEST_KEY_PROTECTED_WITH_KEY_LOAD_KEY,
  REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY,
  REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL,
  REQUEST_KEY_FOR_NWK_KEY,
  REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC,
  REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1,
  VERIFY_KEY_NOT_SEND,
  VERIFICATION_FAILS,
  CORRECT_TCLK_RETENTION,
  TEST_STEPS_COUNT
};


static void test_fsm();
static void verify_step(zb_bool_t verdict, zb_bool_t enable_trace);
static void device_annce_cb(zb_zdo_device_annce_t *da);

static void verify_dut_nbr_table(zb_uint8_t unused);
static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp_cb(zb_uint8_t param);

static void verify_new_tclk_retention(zb_uint8_t unused);
static void send_buffer_test_req(zb_uint8_t param);
static void buffer_test_resp_cb(zb_uint8_t status);


static int s_step_idx;
static int s_error_count;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_AIB().aps_use_nvram = 1;

  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_NIB().max_children = 0;

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}



static void test_fsm()
{
  TRACE_MSG(TRACE_ZDO1, ">>test_fsm: step = %d", (FMT__D, s_step_idx));

  switch (s_step_idx)
  {
    case REQUEST_KEY_NOT_SEND:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 not send Request Key", (FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case REQUEST_KEY_UNPROTECTED:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key unprotected", (FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case REQUEST_KEY_WITH_MIC_ERROR:
      s_step_idx = REQUEST_KEY_FOR_NWK_KEY;
      break;

    case REQUEST_KEY_FOR_NWK_KEY:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for NWK Key", (FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for Application Link Key with TC",(FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for Application Link Key with THr1",(FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case VERIFY_KEY_NOT_SEND:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 not send Verify Key",(FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case VERIFICATION_FAILS:
      TRACE_MSG(TRACE_ZDO1, "TEST: THr2 failed to Verify Key (Wrong MIC)",(FMT__0));
      ZB_SCHEDULE_ALARM(verify_dut_nbr_table, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    case CORRECT_TCLK_RETENTION:
      TRACE_MSG(TRACE_ZDO1,
                "TEST: correct TCLK retention (for multiple devices) after updated TCLK confirmation",
                (FMT__0));
      ZB_SCHEDULE_ALARM(verify_new_tclk_retention, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
      break;

    default:
      TRACE_MSG(TRACE_ZDO1, "TEST: unknown state transition", (FMT__0));
  }

  TRACE_MSG(TRACE_ZDO1, "<<test_fsm", (FMT__0));
}


static void verify_step(zb_bool_t verdict, zb_bool_t enable_trace)
{
  if (enable_trace == ZB_TRUE)
  {
    switch (verdict)
    {
      case ZB_FALSE:
        TRACE_MSG(TRACE_ZDO1, "TEST: VERDICT - STEP FAILED", (FMT__0));
        ++s_error_count;
        break;
      case ZB_TRUE:
        TRACE_MSG(TRACE_ZDO1, "TEST: VERDICT - STEP PASSED", (FMT__0));
        break;
    }
  }
  ++s_step_idx;

  if (s_step_idx == TEST_STEPS_COUNT)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: completed, errors - %d", (FMT__D, s_error_count));
  }
}


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = 0x%x",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
  if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr2, da->ieee_addr) == ZB_TRUE)
  {
    ZB_SCHEDULE_ALARM_CANCEL(verify_dut_nbr_table, 0);
    test_fsm();
    TRACE_MSG(TRACE_ZDO2, "dev_annce_cb: TH ZR2 joined to network!", (FMT__0));
  }
  TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void verify_dut_nbr_table(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_GET_OUT_BUF_DELAYED(send_mgmt_lqi_req);
}


static void send_mgmt_lqi_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_lqi_param_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_zdo_mgmt_lqi_param_t));

  TRACE_MSG(TRACE_ZDO1, "TEST: sending MgmtLqiReq to the DUT", (FMT__0));

  req->dst_addr = 0x0000;
  req->start_index = 0;
  zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp_cb);
}


static void mgmt_lqi_resp_cb(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t *zdp_cmd = ZB_BUF_BEGIN(buf);
  zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t*)(zdp_cmd);
  zb_zdo_neighbor_table_record_t *record = (zb_zdo_neighbor_table_record_t*)(resp + 1);
  zb_uint_t i, found;

  TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp_cb: neigbors = %d, list_size = %d",
            (FMT__D_D, resp->neighbor_table_entries, resp->neighbor_table_list_count));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on MgmtLqiReq", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on MgmtLqiReq", (FMT__0));
    verify_step(ZB_FALSE, ZB_TRUE);
  }

  for (i = 0, found = 0; i < resp->neighbor_table_list_count; i++)
  {
    found = ZB_IEEE_ADDR_CMP(record->ext_addr, g_ieee_addr_thr2);
    record++;
  }

  if (found)
  {
    verify_step(ZB_FALSE, ZB_TRUE);
  }
  else
  {
    verify_step(ZB_TRUE, ZB_TRUE);
  }

  zb_free_buf(buf);
}


static void verify_new_tclk_retention(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_GET_OUT_BUF_DELAYED(send_buffer_test_req);
}


static void send_buffer_test_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_buffer_test_req_param_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_buffer_test_req_param_t));

  TRACE_MSG(TRACE_ZDO1, "TEST: Sending Buffer Test Request", (FMT__0));

  BUFFER_TEST_REQ_SET_DEFAULT(req);
  req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  zb_tp_buffer_test_request(param, buffer_test_resp_cb);
}


static void buffer_test_resp_cb(zb_uint8_t status)
{
  if (!status)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on command", (FMT__0));
    verify_step(ZB_TRUE, ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on command", (FMT__0));
    verify_step(ZB_FALSE, ZB_TRUE);
  }
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
        zb_zdo_register_device_annce_cb(device_annce_cb);
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal - %hd", (FMT__H, sig));
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

  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
