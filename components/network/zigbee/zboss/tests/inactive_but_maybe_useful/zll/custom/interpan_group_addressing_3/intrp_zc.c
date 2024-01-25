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
/* PURPOSE: Dimmable light for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41668
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_time.h"
#include "zb_bufpool.h"
#include "zb_mac.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_aps.h"
#include "zb_aps_interpan.h"
#include "zb_zdo.h"

#include "intrp_test.h"

#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

void send_intrp_data(zb_uint8_t param);

enum test_states_e
{
  TEST_STATE_INITIAL            = 0,
  TEST_STATE_PACKET1_SENT       = 1,
  TEST_STATE_FINISHED           = 3,
};

#define PACKET_SENT_MASK        0x01
#define DEVICE1_RESPONDED_MASK  0x02
#define DEVICE2_RESPONDED_MASK  0x04

#define TEST_STEP_SUCCESS_VALUE (PACKET_SENT_MASK | DEVICE1_RESPONDED_MASK | DEVICE2_RESPONDED_MASK)

#define TEST_STEP_SCHEDULE_INTERVAL (ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000))

/* Address test result for test step by reported step number */
zb_uint8_t g_test_result = 0;

zb_uint8_t g_test_state = TEST_STATE_INITIAL;

zb_ieee_addr_t g_ieee_addr_my = INTRP_TEST_IEEE_ADDR_1;
zb_ieee_addr_t g_ieee_addr_peer2 = INTRP_TEST_IEEE_ADDR_2;
zb_ieee_addr_t g_ieee_addr_peer3 = INTRP_TEST_IEEE_ADDR_3;

zb_uint16_t g_pan_id = INTRP_TEST_PAN_ID_1;

zb_uint16_t g_dst_pan_id = INTRP_TEST_PAN_ID_2;
/* Address GroupId for current transaction via step number (before step change) */
zb_uint16_t g_dst_group_id = INTRP_TEST_GROUP_ID_1;

zb_uint8_t g_error_cnt = 0;

zb_uint16_t g_test_profile_id = TEST_PROFILE_ID;
zb_uint16_t g_test_cluster_id = TEST_CLUSTER_ID;

MAIN()
{
  ARGV_UNUSED;

#ifndef KEIL
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("intrp_zc");

  ZB_SET_NIB_SECURITY_LEVEL(0);

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_my);
  //ZB_IEEE_ADDR_COPY(ZB_NIB_EXT_PAN_ID(), &g_ieee_addr_my);
  ZB_PIBCACHE_PAN_ID() = INTRP_TEST_PAN_ID_1;
#ifndef ZB_ALIEN_MAC
  MAC_PIB().mac_pan_id = INTRP_TEST_PAN_ID_1;
#endif
  ZG->nwk.handle.joined = 1;

  /* let's always be coordinator */
  //ZB_AIB().aps_designated_coordinator = 1;

  zb_set_default_ed_descriptor_values();


  ZG->nwk.nib.security_level = 0;

  if (zb_zll_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
    ++g_error_cnt;
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/*! Test step */
void next_step(zb_uint8_t param)
{
  zb_buf_t * zcl_cmd_buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APS3, "> next_step param %hd", (FMT__H, param));

  switch (g_test_state)
  {
    case TEST_STATE_INITIAL:
      TRACE_MSG(TRACE_APS3, "Sending packet 1", (FMT__0));

      send_intrp_data(param);
      g_test_state = TEST_STATE_PACKET1_SENT;
      break;
    case TEST_STATE_PACKET1_SENT:
      TRACE_MSG(TRACE_APS3, "Finishing test", (FMT__0));
      g_test_state = TEST_STATE_FINISHED;
      break;
    case TEST_STATE_FINISHED:
    default:
      TRACE_MSG(TRACE_ERROR, "ERROR Next step when finished", (FMT__0));
      ++g_error_cnt;
      zb_free_buf(zcl_cmd_buf);
      break;
  }

  if (g_test_state == TEST_STATE_FINISHED)
  {
    if (g_test_result != TEST_STEP_SUCCESS_VALUE)
    {
      TRACE_MSG(
          TRACE_ERROR,
          "ERROR Test failed with result %hd",
          (FMT__H, g_test_result));
      ++g_error_cnt;
    }

    if (!g_error_cnt)
    {
      TRACE_MSG(TRACE_APS1, "Test finished. Status: OK", (FMT__0));
    }
    else
    {
      TRACE_MSG(
          TRACE_APS1,
          "Test finished. Status: FAILED, error number %hd",
          (FMT__H, g_error_cnt));
    }
  }

  TRACE_MSG(TRACE_APS3, "< next_step: g_test_state: %hd", (FMT__H, g_test_state));
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZDO1, "> zb_zdo_startup_complete param %hd", (FMT__H, param));

  if ((buf->u.hdr.status == 0) || (buf->u.hdr.status == ZB_NWK_STATUS_ALREADY_PRESENT))
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
    next_step(param);
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }

  TRACE_MSG(TRACE_ZDO1, "< zb_zdo_startup_complete", (FMT__0));
}

void send_intrp_data(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_intrp_data_req_t* request = ZB_GET_BUF_PARAM(buffer, zb_intrp_data_req_t);
  zb_uint8_t* data;

  TRACE_MSG(TRACE_APS1, "> send_aps_data param %hd", (FMT__H, param));

  ZB_BUF_REUSE(buffer);
  ZB_BZERO(request, sizeof(*request));
  request->dst_addr_mode = ZB_INTRP_ADDR_GROUP;
  ZB_MEMCPY(
      &(request->dst_addr.addr_short),
      &g_dst_group_id,
      sizeof(g_dst_group_id));
  ZB_MEMCPY(&(request->dst_pan_id), &g_dst_pan_id, sizeof(g_dst_pan_id));
  ZB_MEMCPY(&(request->profile_id), &g_test_profile_id, sizeof(zb_uint16_t));
  ZB_MEMCPY(&(request->cluster_id), &g_test_cluster_id, sizeof(zb_uint16_t));
  ZB_BUF_INITIAL_ALLOC(buffer, sizeof(zb_uint8_t), data);
  *data = g_test_state;
  request->asdu_handle = 0;
  ZB_SCHEDULE_CALLBACK(zb_intrp_data_request, param);

  TRACE_MSG(TRACE_APS1, "< send_aps_data", (FMT__0));
}/* void send_aps_data(zb_uint8_t param) */

void zb_intrp_data_confirm(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t* data = ZB_BUF_BEGIN(buffer);

  TRACE_MSG(TRACE_APS1, "> zb_intrp_data_confirm param %hd", (FMT__H, param));

  if (sizeof(zb_uint8_t) != ZB_BUF_LEN(buffer))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Wrong ASDU size %hd (should be 8)", (FMT__H, ZB_BUF_LEN(buffer)));
    ++g_error_cnt;
  }
  if (buffer->u.hdr.status != MAC_SUCCESS)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Packet send status %hd (not success)",
        (FMT__H, buffer->u.hdr.status));
    ++g_error_cnt;
  }
  else if (! *data)
  {
    g_test_result |= PACKET_SENT_MASK;
  }

  ZB_SCHEDULE_ALARM(next_step, param, TEST_STEP_SCHEDULE_INTERVAL);

  TRACE_MSG(TRACE_APS1, "< zb_intrp_data_confirm", (FMT__0));
}/* void zb_intrp_data_confirm(zb_uint8_t param) */

void zb_intrp_data_indication(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_intrp_data_ind_t* indication = ZB_GET_BUF_PARAM(buffer, zb_intrp_data_ind_t);
  zb_uint8_t header_size;
  zb_uint16_t tmp_val;
  zb_uint8_t* data_ptr;

  TRACE_MSG(TRACE_APS1, "> zb_intrp_data_indication param %hd", (FMT__H, param));

  header_size = ZB_INTRP_HEADER_SIZE(indication->dst_addr_mode == ZB_INTRP_ADDR_GROUP);
  ZB_BUF_CUT_LEFT(buffer, header_size, data_ptr);

  if (indication->dst_addr_mode != ZB_INTRP_ADDR_IEEE)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR INTRP-DATA.indication.DstAddrMode %hd (!= %hd)",
        (FMT__H_H, indication->dst_addr_mode, (zb_uint8_t)ZB_INTRP_ADDR_IEEE));
    ++g_error_cnt;
  }
  if (! *data_ptr)
  {
    if (ZB_IEEE_ADDR_CMP(indication->src_addr, g_ieee_addr_peer2))
    {
      TRACE_MSG(TRACE_APS3, "Device #2 responded", (FMT__0));
      g_test_result |= DEVICE1_RESPONDED_MASK;
    }
    else if (ZB_IEEE_ADDR_CMP(indication->src_addr, g_ieee_addr_peer3))
    {
      TRACE_MSG(TRACE_APS3, "Device #3 responded", (FMT__0));
      g_test_result |= DEVICE2_RESPONDED_MASK;
    }
    else
    {
      TRACE_MSG(
          TRACE_ERROR,
          "ERROR Response from unknown device %s",
          (FMT__A, indication->src_addr));
      ++g_error_cnt;
    }
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR %hd reported wrong step number %hd",
        (FMT__A_H, (ZB_IEEE_ADDR_CMP(indication->src_addr, g_ieee_addr_peer2) ? 2 : 3), *data_ptr));
    ++g_error_cnt;
  }
  ZB_MEMCPY(&tmp_val, &g_test_profile_id, sizeof(g_test_profile_id));
  if (tmp_val != g_test_profile_id)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Wrong INTRP-DATA.indication.ProfileID 0x%04x (should be 0x%04x)",
        (FMT__D_D, tmp_val, g_test_profile_id));
    ++g_error_cnt;
  }
  ZB_MEMCPY(&tmp_val, &g_test_cluster_id, sizeof(g_test_cluster_id));
  if (tmp_val != g_test_cluster_id)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Wrong INTRP-DATA.indication.ClusterID 0x%04x (should be 0x%04x)",
        (FMT__D_D, tmp_val, g_test_cluster_id));
    ++g_error_cnt;
  }

  TRACE_MSG(TRACE_APS1, "< zb_intrp_data_indication", (FMT__0));
}/* void zb_intrp_data_indication(zb_uint8_t param) */

#else /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */

#include <stdio.h>
int main()
{
  printf(" Inter-PAN exchange is not supported\n");
  return 0;
}

#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
