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
/* PURPOSE:
*/


#define ZB_TEST_NAME S_NWK_01_ZED2

#define APS_FRAGMENTATION

#define ZB_TRACE_FILE_ID 18003
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define PACKET_MAX_LENGTH 55

#define ZR1_SHORT_ADDR            0x1111

#define ZED1_SHORT_ADDR            0xAAAA
#define ZED2_SHORT_ADDR            0xBBBB

static zb_ieee_addr_t g_ieee_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint32_t current_len=0, err_cnt=0, ok_cnt=0;

static zb_uint8_t g_buffer_test_req_len = 10;
static zb_uint16_t g_buffer_test_req_dst_addr = 0x0000;
static zb_bool_t g_buffer_test_req_discovery_route = ZB_FALSE;
static void send_buffer_test_req_manually_delayed(zb_uint8_t unused);
static void send_buffer_test_req_manually(zb_uint8_t param);
static void buffer_test_cb(zb_uint8_t param);
static void send_data_ZC(zb_uint8_t param);
static void send_data_ZR(zb_uint8_t param);
static void send_data_ZED1(zb_uint8_t param);


static uint32_t runID=0;

static void system_breath(zb_uint8_t param)
{
    zb_osif_led_toggle(2);
    ZB_SCHEDULE_ALARM_CANCEL(system_breath, 0);
    ZB_SCHEDULE_ALARM(system_breath, 0, ZB_TIME_ONE_SECOND);
}

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zed");

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zed_role();
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Set end-device configuration parameters */
  zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
  

  #if defined ZB_USE_SLEEP
  zb_set_rx_on_when_idle(ZB_FALSE);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
  #else
  zb_set_rx_on_when_idle(ZB_TRUE);
  #endif

  MAC_ADD_INVISIBLE_SHORT(0x0000);

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

  //TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        test_step_register(send_data_ZC, 0, 5 * ZB_TIME_ONE_SECOND);

        test_control_start(TEST_MODE, 5 * ZB_TIME_ONE_SECOND);

        ZB_SCHEDULE_ALARM(system_breath, 0, ZB_TIME_ONE_SECOND);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
#ifdef ZB_USE_SLEEP
        zb_sleep_now();
#endif
        break;
      }

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void send_buffer_test_req_manually_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_buffer_test_req_manually);
}

static void send_buffer_test_req_manually(zb_uint8_t param)
{
  int i;
  zb_buffer_test_req_t *req;
  zb_ret_t ret = RET_OK;
  zb_uint8_t fc = 0;
  zb_nlde_data_req_t nldereq;
  zb_apsde_data_req_t *apsreq;
  zb_apsde_data_req_t *dreq;
  zb_address_ieee_ref_t addr_ref;

  req = zb_buf_initial_alloc(param, sizeof(zb_buffer_test_req_t) + g_buffer_test_req_len - 1);
  req->len = g_buffer_test_req_len;

  for (i = 0; i < g_buffer_test_req_len; ++i)
  {
    req->req_data[i] = i;
  }

#ifndef NCP_MODE_HOST
  ZG->zdo.test_prof_ctx.zb_tp_buffer_test_request.user_cb = NULL;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: enable test profile for NCP");
#endif

  dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

  TRACE_MSG(TRACE_ZDO2, "tp_send_req_by_short param %hd", (FMT__H, param));
  ZB_BZERO(dreq, sizeof(*dreq));

  dreq->profileid = ZB_TEST_PROFILE_ID;
  dreq->clusterid = TP_BUFFER_TEST_REQUEST_CLID;
  dreq->dst_endpoint = ZB_TEST_PROFILE_EP;
  dreq->src_endpoint = ZB_TEST_PROFILE_EP;
  dreq->dst_addr.addr_short = g_buffer_test_req_dst_addr;
  dreq->tx_options = 0;
  dreq->tx_options |= ZB_APSDE_TX_OPT_SECURITY_ENABLED;
  dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  dreq->radius = MAX_NWK_RADIUS;

  apsreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

  nldereq.radius = apsreq->radius;
  nldereq.addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  nldereq.nonmember_radius = 0; /* if multicast, get it from APS IB */
  nldereq.discovery_route = g_buffer_test_req_discovery_route;

  zb_buf_flags_or(param, ZB_BUF_HAS_APS_PAYLOAD);

  ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);
  nldereq.dst_addr = apsreq->dst_addr.addr_short;

#ifdef ZB_USEALIAS
    nldereq.use_alias = apsreq->use_alias;
    nldereq.alias_src_addr = apsreq->alias_src_addr;
    nldereq.alias_seq_num = apsreq->alias_seq_num;
#endif

  ZB_APS_FC_SET_ACK_FORMAT(fc, 0);

  aps_data_hdr_fill_datareq(fc, apsreq, param);

  ZB_CHK_ARR(ZB_BUF_BEGIN(param), 8); /* check hdr fill */

  nldereq.security_enable = ZB_TRUE;
  nldereq.ndsu_handle = 0;

  ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_nlde_data_req_t), &nldereq, sizeof(nldereq));

  ret = zb_address_by_short(nldereq.dst_addr, ZB_TRUE, ZB_TRUE, &addr_ref);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APS1, "Schedule packet to NWK", (FMT__0));
  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
}

static void buffer_test_cb(zb_uint8_t param)
{
  zb_bufid_t buf = 0;
  zb_apsme_add_group_req_t *req_param = NULL;

  if (param == ZB_TP_BUFFER_TEST_OK)
  {
      ok_cnt++;
      zb_osif_led_off(0);
  }
  else
  {
      err_cnt++;  
      zb_osif_led_on(0);
  }

  TRACE_MSG(TRACE_APP1, "status OK: %08X, status ERROR: %08X", (FMT__A_A, ok_cnt, err_cnt));

  if(runID == 3)
  {
    ZB_SCHEDULE_ALARM_CANCEL(send_data_ZC, 0);
    ZB_SCHEDULE_ALARM(send_data_ZC, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
  }
  else if(runID == 1)
  {
    ZB_SCHEDULE_ALARM_CANCEL(send_data_ZR, 0);
    ZB_SCHEDULE_ALARM(send_data_ZR, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
  }
  else if(runID == 2)
  {
    ZB_SCHEDULE_ALARM_CANCEL(send_data_ZED1, 0);
    ZB_SCHEDULE_ALARM(send_data_ZED1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
  }

}

static void send_data_ZC(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);

  if(current_len > PACKET_MAX_LENGTH)
    current_len = 0;

  req_param->len = current_len;
  current_len ++;

  zb_osif_led_toggle(3);

  runID = 1;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}

static void send_data_ZR(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);

  req_param->dst_addr = ZR1_SHORT_ADDR;

  if(current_len > PACKET_MAX_LENGTH)
    current_len = 0;

  req_param->len = current_len;
  current_len ++;

  zb_osif_led_toggle(3);

  runID = 2;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}

static void send_data_ZED1(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, "send_data: %hd", (FMT__H, buf));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);

  req_param->dst_addr = ZED1_SHORT_ADDR;

  if(current_len > PACKET_MAX_LENGTH)
    current_len = 0;

  req_param->len = current_len;
  current_len ++;

  zb_osif_led_toggle(3);

  runID = 3;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}
