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
/* PURPOSE: TH ZR1 (test driver)
*/


#define ZB_TEST_NAME FB_CDP_TC_01_THR1
#define ZB_TRACE_FILE_ID 41033
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
#include "fb_cdp_tc_01_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

enum test_steps_e
{
  CHECK_BIND_TABLE_INIT,
  CHECK_REPORTING,
  UNBIND_DEFAULT_BINDING,
  CHECK_BIND_TABLE_0,
  CREATE_UNICAST_BIND_1,
  CREATE_UNICAST_BIND_2,
  CREATE_GROUP_BIND_3,
  CHECK_BIND_TABLE_1,
  DELAY_FOR_DUT,
  UNBIND_WRONG_SRC_ADDR,
  UNBIND_WRONG_SRC_EP,
  UNBIND_WRONG_CLUSTER,
  UNBIND_WRONG_GROUP,
  UNBIND_WRONG_DEST_ADDR,
  UNBIND_WRONG_DEST_EP,
  UNBIND_BRCAST_DEST,
  CHECK_BIND_TABLE_2,
  MGMT_BIND_MIDDLE_IDX,
  MGMT_BIND_OVERFLOW_IDX,
  UNBIND_GROUP_ADDR,
  CHECK_BIND_TABLE_3,
  UNBIND_WRONG_ADDR_MODE,
  UNBIND_UNICAST_BIND_2,
  CHECK_BIND_TABLE_4,
  UNBIND_UNICAST_BIND_1,
  CHECK_BIND_TABLE_5,
  BIND_INACTIVE_EP,
  BIND_UNSUP_CLUSTER,
  BIND_USING_BRCAST,
  CHECK_BIND_TABLE_6,
  BIND_FOR_ANOTHER_DEVICE,
  CHECK_BIND_TABLE_7,
  TEST_STEPS_COUNT
};


enum bind_unbind_req_type_e
{
  SEND_UNBIND_REQ,
  SEND_BIND_REQ
};


static void get_peer_addr(zb_uint8_t unused);
static void request_nb_table(zb_uint8_t param);
static void request_nb_table_resp(zb_uint8_t param);

static void fill_bind_req(zb_uint16_t cluster_id, zb_uint8_t ep);
static void store_bind_table(zb_uint8_t *table_p, zb_uint8_t start_idx, zb_uint8_t entries);
static void test_thr1_fsm(zb_uint8_t unused);

static void send_bind_unbind_req(zb_uint8_t param, zb_uint16_t bind);
static void bind_unbind_resp_cb(zb_uint8_t param);
/* Called at test steps 0a - 0b: removing of default binding */
static void remove_default_binding(zb_uint8_t param);
static void unbind_default_bindings_resp_cb(zb_uint8_t param);
static void check_bind_table(zb_uint8_t param, zb_uint16_t start_at);
static void check_bind_table_resp_cb(zb_uint8_t param);

static zb_uint16_t s_dut_short_addr;
static int s_step_idx;
static zb_zdo_bind_req_param_t s_temp_bind_req;
static zb_zdo_binding_table_record_t s_bind_table_cache[MAX_THR1_BIND_TABLE_CACHE_SIZE];
static int s_bind_table_cache_sz;
static int s_global_idx; /* global counter: used in some test steps */


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_AIB().aps_use_nvram = 1;

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


static void get_peer_addr(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  s_dut_short_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  if (s_dut_short_addr == 0xffff)
  {
    TRACE_MSG(TRACE_ZDO1, "THR1: Can not obtain DUT short address from addr_map", (FMT__0));
    TRACE_MSG(TRACE_ZDO1, "THR1: request parents neighbor table.", (FMT__0));
    ZB_GET_OUT_BUF_DELAYED(request_nb_table);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "DUT short addr = 0x%x", (FMT__H, s_dut_short_addr));
  }
}


static void request_nb_table(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_lqi_param_t *req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_lqi_param_t);
  zb_uint16_t dest_addr;

  zb_address_short_by_ref(&dest_addr, ZG->nwk.handle.parent);
  req_param->dst_addr = dest_addr;
  TRACE_MSG(TRACE_ZDO1, "request_nb_table: parent addr = 0x%x",
            (FMT__H, dest_addr));
  req_param->start_index = 0;
  zb_zdo_mgmt_lqi_req(param, request_nb_table_resp);
}


static void request_nb_table_resp(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t*) ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO1, ">>request_bv_table_resp buf = %d", (FMT__D, param));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    int N = resp->neighbor_table_list_count;
    int i;
    zb_ieee_addr_t temp_addr;
    zb_zdo_neighbor_table_record_t *nbt = (zb_zdo_neighbor_table_record_t*) (resp + 1);

    for (i = 0; i < N; ++i, ++nbt)
    {
      ZB_IEEE_ADDR_COPY(temp_addr, nbt->ext_addr);
      if (ZB_IEEE_ADDR_CMP(temp_addr, g_ieee_addr_dut))
      {
        s_dut_short_addr = nbt->network_addr;
        TRACE_MSG(TRACE_ZDO1, "DUT is found: 0x%x", (FMT__H, s_dut_short_addr));
        break;
      }
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "request failure with status = %d", (FMT__D, resp->status));
  }
  zb_free_buf(buf);

  TRACE_MSG(TRACE_ZDO1, "<<request_nb_table_resp", (FMT__0));
}


static void fill_bind_req(zb_uint16_t cluster_id, zb_uint8_t ep)
{
  ZB_IEEE_ADDR_COPY(s_temp_bind_req.src_address, g_ieee_addr_dut);
  s_temp_bind_req.src_endp = DUT_ENDPOINT1;
  s_temp_bind_req.cluster_id = cluster_id;
  /* 64-bit extended address for DstAddress and DstEndp present */
  s_temp_bind_req.dst_addr_mode = 0x03;
  ZB_IEEE_ADDR_COPY(s_temp_bind_req.dst_address.addr_long, g_ieee_addr_thr2);
  s_temp_bind_req.req_dst_addr = s_dut_short_addr;
  s_temp_bind_req.dst_endp = ep;
}


static void store_bind_table(zb_uint8_t *table_p, zb_uint8_t start_idx, zb_uint8_t entries)
{
  int i;
  zb_zdo_binding_table_record_t *ptr;

  for (i = start_idx, ptr = (zb_zdo_binding_table_record_t*) table_p;
       (i < start_idx + entries) && (i < MAX_THR1_BIND_TABLE_CACHE_SIZE);
       ++i, ++ptr)
  {
    TRACE_MSG(TRACE_ZDO3, "Add binding#%d to cache", (FMT__D, i));
    ZB_MEMCPY(&s_bind_table_cache[i], ptr, sizeof(zb_zdo_binding_table_record_t));

    TRACE_MSG(TRACE_ZDO3, "Src_address = " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(s_bind_table_cache[i].src_address)));
    TRACE_MSG(TRACE_ZDO3, "Src_endp = %d", (FMT__D, s_bind_table_cache[i].src_endp));
    TRACE_MSG(TRACE_ZDO3, "Cluster_id = %d", (FMT__D, s_bind_table_cache[i].cluster_id));
    TRACE_MSG(TRACE_ZDO3, "dst_addr_mode = %d", (FMT__D, s_bind_table_cache[i].dst_addr_mode));
    TRACE_MSG(TRACE_ZDO3, "dst_endp = %d", (FMT__D, s_bind_table_cache[i].dst_endp));

    ++s_bind_table_cache_sz;
  }
}


/* Main test logic */
static void test_thr1_fsm(zb_uint8_t unused)
{
  zb_callback2_t call_cb = NULL;
  zb_uint16_t param2;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, ">>test_step_fsm: step = %d", (FMT__D, s_step_idx));
  fill_bind_req(DUT_MATCHING_CLUSTER, THR2_ENDPOINT1);

  switch (s_step_idx)
  {
    case CHECK_REPORTING:
      TRACE_MSG(TRACE_ZDO2, "Read default reporting from DUT (Omitted)", (FMT__0));
      ++s_step_idx;
      ZB_SCHEDULE_CALLBACK(test_thr1_fsm, 0);
      break;

    case UNBIND_DEFAULT_BINDING:
      TRACE_MSG(TRACE_ZDO2, "Remove default bindings: cache size = %d.",
                (FMT__D, s_bind_table_cache_sz));
      {
        s_global_idx = 0;
        if (s_global_idx < s_bind_table_cache_sz)
        {
          ZB_GET_OUT_BUF_DELAYED(remove_default_binding);
        }
        else
        {
          ++s_step_idx;
          ZB_SCHEDULE_CALLBACK(test_thr1_fsm, 0);
        }
      }
      break;

    case CREATE_UNICAST_BIND_1:
      TRACE_MSG(TRACE_ZDO2, "Create unicast binding for endpoint T1", (FMT__0));
      {
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case CREATE_UNICAST_BIND_2:
      TRACE_MSG(TRACE_ZDO2, "Create unicast binding for endpoint T2", (FMT__0));
      {
        s_temp_bind_req.dst_endp = THR2_ENDPOINT2;
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case CREATE_GROUP_BIND_3:
      TRACE_MSG(TRACE_ZDO2, "Create groupcast binding", (FMT__0));
      {
        s_temp_bind_req.dst_addr_mode = 0x01; /* 0x01 = 16-bit group address for
                                                 DstAddress and DstEndp not present  */
        s_temp_bind_req.dst_address.addr_short = GROUP_ADDRESS;
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case CHECK_BIND_TABLE_INIT:
    case CHECK_BIND_TABLE_0:
    case CHECK_BIND_TABLE_1:
    case CHECK_BIND_TABLE_2:
    case CHECK_BIND_TABLE_3:
    case CHECK_BIND_TABLE_4:
    case CHECK_BIND_TABLE_5:
    case CHECK_BIND_TABLE_6:
    case CHECK_BIND_TABLE_7:
      param2 = 0; /* start at first entry */
      TRACE_MSG(TRACE_ZDO2, "Check binding table: start at index %d", (FMT__D, param2));
      call_cb = check_bind_table;
      break;

    case DELAY_FOR_DUT:
      TRACE_MSG(TRACE_ZDO2, "Wait fot DUT send messages via bindings", (FMT__0));
      ZB_SCHEDULE_ALARM(test_thr1_fsm, 0, TEST_THR1_WAITS_DUT_DELAY + TEST_SEND_CMD_SKEW);
      ++s_step_idx;
      break;

    case UNBIND_WRONG_SRC_ADDR:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong src addr)", (FMT__0));
      {
        ZB_IEEE_ADDR_COPY(s_temp_bind_req.src_address, g_ieee_addr_other);
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_SRC_EP:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong src endpoint)", (FMT__0));
      {
        s_temp_bind_req.src_endp = TEST_WRONG_SRC_EP;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_CLUSTER:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong cluster)", (FMT__0));
      {
        s_temp_bind_req.cluster_id = TEST_WRONG_CLUSTER;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_GROUP:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong group address)", (FMT__0));
      {
        s_temp_bind_req.dst_addr_mode = 0x01; /* 0x01 = 16-bit group address for
                                                 DstAddress and DstEndp not present  */
        s_temp_bind_req.dst_address.addr_short = ~GROUP_ADDRESS;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_DEST_ADDR:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong unicast destination address)", (FMT__0));
      {
        ZB_IEEE_ADDR_COPY(s_temp_bind_req.dst_address.addr_long, g_ieee_addr_other);
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_DEST_EP:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong dest endpoint)", (FMT__0));
      {
        s_temp_bind_req.dst_endp = TEST_WRONG_DEST_EP;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_BRCAST_DEST:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (broadcast destination address)", (FMT__0));
      {
        s_temp_bind_req.req_dst_addr = 0xffff;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case MGMT_BIND_MIDDLE_IDX:
      param2 = 2; /* start at first entry */
      TRACE_MSG(TRACE_ZDO2, "Check binding table: start at index %d", (FMT__D, param2));
      call_cb = check_bind_table;
      break;

    case MGMT_BIND_OVERFLOW_IDX:
      param2 = 5; /* start at first entry */
      TRACE_MSG(TRACE_ZDO2, "Check binding table: start at index %d", (FMT__D, param2));
      call_cb = check_bind_table;
      break;

    case UNBIND_GROUP_ADDR:
      TRACE_MSG(TRACE_ZDO2, "Unbind the groupcast binding", (FMT__0));
      {
        s_temp_bind_req.dst_addr_mode = 0x01; /* 0x01 = 16-bit group address for
                                                 DstAddress and DstEndp not present  */
        s_temp_bind_req.dst_address.addr_short = GROUP_ADDRESS;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_WRONG_ADDR_MODE:
      TRACE_MSG(TRACE_ZDO2, "Remove binding (wrong dest addr mode)", (FMT__0));
      {
        s_temp_bind_req.dst_addr_mode = 0x01; /* 0x01 = 16-bit group address for
                                                 DstAddress and DstEndp not present  */
        s_temp_bind_req.dst_address.addr_short = GROUP_ADDRESS;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_UNICAST_BIND_2:
      TRACE_MSG(TRACE_ZDO2, "Remove binding for endpoint T2", (FMT__0));
      {
        s_temp_bind_req.dst_endp = THR2_ENDPOINT2;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case UNBIND_UNICAST_BIND_1:
      TRACE_MSG(TRACE_ZDO2, "Remove binding for endpoint T1", (FMT__0));
      {
        s_temp_bind_req.dst_endp = THR2_ENDPOINT1;
        param2 = SEND_UNBIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case BIND_INACTIVE_EP:
      TRACE_MSG(TRACE_ZDO2, "Create binding (inactive DUT endpoint)", (FMT__0));
      {
        s_temp_bind_req.src_endp = TEST_INACTIVE_DUT_EP;
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case BIND_UNSUP_CLUSTER:
      TRACE_MSG(TRACE_ZDO2, "Create binding (unsupported cluster by DUT)", (FMT__0));
      {
        s_temp_bind_req.cluster_id = TEST_UNSUP_CLUSTER;
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case BIND_USING_BRCAST:
      TRACE_MSG(TRACE_ZDO2, "Create binding (by broadcast)", (FMT__0));
      {
        s_temp_bind_req.req_dst_addr = 0xffff;
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    case BIND_FOR_ANOTHER_DEVICE:
      TRACE_MSG(TRACE_ZDO2, "Create binding (wrong src addr)", (FMT__0));
      {
        ZB_IEEE_ADDR_COPY(s_temp_bind_req.src_address, g_ieee_addr_other);
        param2 = SEND_BIND_REQ;
        call_cb = send_bind_unbind_req;
      }
      break;

    default:
      if (s_step_idx == TEST_STEPS_COUNT)
      {
        TRACE_MSG(TRACE_ZDO1, "test_thr1_fsm: TEST FINISHED", (FMT__0));
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "test_thr1_fsm: TEST ERROR - illegal state", (FMT__0));
      }
      break;
  }

  if (call_cb != NULL)
  {
    ZB_GET_OUT_BUF_DELAYED2(call_cb, param2);
  }

  TRACE_MSG(TRACE_ZDO1, "<<test_step_fsm", (FMT__0));
}


static void send_bind_unbind_req(zb_uint8_t param, zb_uint16_t bind)
{
  zb_buf_t *buf;
  zb_zdo_bind_req_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>send_bind_unbind_req: param = %i", (FMT__D, param));

  buf = ZB_BUF_FROM_REF(param);
  req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);

  ZB_MEMCPY(req_params, &s_temp_bind_req, sizeof(zb_zdo_bind_req_param_t));
  if (bind)
  {
    zb_zdo_bind_req(param, bind_unbind_resp_cb);
  }
  else
  {
    zb_zdo_unbind_req(param, bind_unbind_resp_cb);
  }

  TRACE_MSG(TRACE_ZDO3, "<<send_bind_unbind_req", (FMT__0));
}


static void bind_unbind_resp_cb(zb_uint8_t param)
{
  zb_zdo_bind_resp_t *resp;
  zb_buf_t *buf;

  TRACE_MSG(TRACE_ZDO2, ">>bind_unbind_resp_cb: test step = %i", (FMT__D, s_step_idx));

  buf = ZB_BUF_FROM_REF(param);
  resp = (zb_zdo_bind_resp_t*)ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO2, "bind_unbind_resp_cb: resp status 0x%x", (FMT__H, resp->status));

  zb_free_buf(buf);
  ++s_step_idx;
  ZB_SCHEDULE_ALARM(test_thr1_fsm, 0, TEST_SEND_CMD_SKEW);

  TRACE_MSG(TRACE_ZDO2, "<<bind_unbind_resp_cb", (FMT__0));
}


static void remove_default_binding(zb_uint8_t param)
{
  zb_buf_t *buf;
  zb_zdo_bind_req_param_t *req_params;
  int i = s_global_idx;

  TRACE_MSG(TRACE_ZDO3, ">>remove_default_binding: param = %i, bind#%d",
            (FMT__D_D, param, s_global_idx));

  buf = ZB_BUF_FROM_REF(param);
  req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_bind_req_param_t);

  {
    ZB_IEEE_ADDR_COPY(req_params->src_address, &s_bind_table_cache[i].src_address);
    req_params->src_endp = s_bind_table_cache[i].src_endp;
    req_params->cluster_id = s_bind_table_cache[i].cluster_id;
    req_params->dst_addr_mode = s_bind_table_cache[i].dst_addr_mode;
    if (req_params->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
    {
      req_params->dst_address.addr_short = s_bind_table_cache[i].dst_address.addr_short;
    }
    else
    {
      ZB_IEEE_ADDR_COPY(req_params->dst_address.addr_long,
      &s_bind_table_cache[i].dst_address.addr_long);
    }
    req_params->dst_endp = s_bind_table_cache[i].dst_endp;
    req_params->req_dst_addr = s_dut_short_addr;
  }

  TRACE_MSG(TRACE_ZDO3, ">>Src_address = " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(s_bind_table_cache[i].src_address)));
  TRACE_MSG(TRACE_ZDO3, ">>Src_endp = %d", (FMT__D, s_bind_table_cache[i].src_endp));
  TRACE_MSG(TRACE_ZDO3, ">>Cluster_id = %d", (FMT__D, s_bind_table_cache[i].cluster_id));
  TRACE_MSG(TRACE_ZDO3, ">>dst_addr_mode = %d", (FMT__D, s_bind_table_cache[i].dst_addr_mode));
  TRACE_MSG(TRACE_ZDO3, ">>dst_endp = %d", (FMT__D, s_bind_table_cache[i].dst_endp));

  zb_zdo_unbind_req(param, unbind_default_bindings_resp_cb);

  TRACE_MSG(TRACE_ZDO3, "<<remove_default_binding", (FMT__0));
}


static void unbind_default_bindings_resp_cb(zb_uint8_t param)
{
  zb_zdo_bind_resp_t *resp;
  zb_buf_t *buf;
  int next_step = 0;

  TRACE_MSG(TRACE_ZDO2, ">>unbind_default_bindings_resp_cb: test step = %i", (FMT__D, s_step_idx));

  buf = ZB_BUF_FROM_REF(param);
  resp = (zb_zdo_bind_resp_t*)ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO2, "unbind_default_bindings_resp_cb: resp status 0x%x", (FMT__H, resp->status));
  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ++s_global_idx;
    if (s_global_idx < s_bind_table_cache_sz)
    {
      TRACE_MSG(TRACE_ZDO2, "unbind_default_bindings_resp_cb: remove default binding#%d",
                (FMT__D, s_global_idx));
      ZB_GET_OUT_BUF_DELAYED(remove_default_binding);
    }
    else
    {
      TRACE_MSG(TRACE_ZDO2, "unbind_default_bindings_resp_cb: done",(FMT__0));
      next_step = 1;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "unbind_default_bindings_resp_cb: failed to check", (FMT__0));
    next_step = 1;
  }

  zb_free_buf(buf);
  if (next_step)
  {
    ++s_step_idx;
    ZB_SCHEDULE_ALARM(test_thr1_fsm, 0, TEST_SEND_CMD_SKEW);
  }

  TRACE_MSG(TRACE_ZDO2, "<<unbind_default_bindings_resp_cb", (FMT__0));
}


static void check_bind_table(zb_uint8_t param, zb_uint16_t start_at)
{
  zb_buf_t *buf;
  zb_zdo_mgmt_bind_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>check_bind_table: buf = %i, start_at = %d",
            (FMT__D_D, param, start_at));

  buf = ZB_BUF_FROM_REF(param);
  req_params = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_bind_param_t);
  req_params->start_index = start_at;
  req_params->dst_addr = s_dut_short_addr;
  zb_zdo_mgmt_bind_req(param, check_bind_table_resp_cb);

  TRACE_MSG(TRACE_ZDO3, "<<check_bind_table", (FMT__0));
}


static void check_bind_table_resp_cb(zb_uint8_t param)
{
  zb_buf_t *buf;
  zb_zdo_mgmt_bind_resp_t *resp;
  int next_step = 0;

  TRACE_MSG(TRACE_ZDO1, ">>check_bind_table_resp_cb: param = %i", (FMT__D, param));

  buf = ZB_BUF_FROM_REF(param);
  resp = (zb_zdo_mgmt_bind_resp_t*) ZB_BUF_BEGIN(buf);

  TRACE_MSG(TRACE_ZDO1, "check_bind_table_resp_cb: status = %i", (FMT__D, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t nbrs = resp->binding_table_list_count;
    zb_uint8_t start_idx = resp->start_index;

    if (start_idx + nbrs < resp->binding_table_entries)
    {
      ZB_GET_OUT_BUF_DELAYED2(check_bind_table, start_idx + nbrs);
    }
    else
    {
      next_step = 1;
    }

    if (s_step_idx == CHECK_BIND_TABLE_INIT)
    {
      store_bind_table((zb_uint8_t*) (resp + 1), start_idx, nbrs);
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "check_bind_table_resp_cb: failed to check", (FMT__0));
    next_step = 1;
  }

  zb_free_buf(buf);
  if (next_step)
  {
    ++s_step_idx;
    ZB_SCHEDULE_ALARM(test_thr1_fsm, 0, TEST_SEND_CMD_SKEW);
  }

  TRACE_MSG(TRACE_ZDO1, "<<check_bind_table_resp_cb", (FMT__0));
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
        ZB_SCHEDULE_ALARM(get_peer_addr, 0, TEST_SEND_CMD_SKEW);
        ZB_SCHEDULE_ALARM(test_thr1_fsm, 0, TEST_THR1_START_TEST_DELAY + TEST_SEND_CMD_SKEW);
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
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
