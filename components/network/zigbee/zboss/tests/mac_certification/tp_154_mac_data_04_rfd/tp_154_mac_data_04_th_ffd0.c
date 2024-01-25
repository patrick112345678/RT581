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
/* PURPOSE: TP/154/MAC/DATA-04 TH FFD0 (case 1)
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_04_RFD_TH_FFD0
#define ZB_TRACE_FILE_ID 57566
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_04_common.h"
#include "zb_mac_globals.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_COMM_STATUS_INDICATION
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_ffd0 = TEST_FFD0_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_set_pan_id(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_data_04_rfd_th_ffd0");

  ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
#if UART_CONTROL
  test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
  while(1)
  {
    zb_sched_loop_iteration();
  }


  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static void test_started_cb(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

  zb_buf_get_out_delayed(test_mlme_reset_request);
}

/***************** Test functions *****************/
static void test_mlme_reset_request(zb_uint8_t param)
{
  zb_mlme_reset_request_t *reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);
  reset_req->set_default_pib = 1;

  TRACE_MSG(TRACE_APP1, "MLME-RESET.request()", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);
}


static void test_set_ieee_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
  req->pib_length = sizeof(zb_ieee_addr_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr_ffd0, sizeof(zb_ieee_addr_t));

  req->confirm_cb_u.cb = test_set_short_addr;
  zb_mlme_set_request(param);
}


static void test_set_short_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t short_addr = TEST_FFD0_SHORT_ADDRESS;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &short_addr, sizeof(zb_uint16_t));

  req->confirm_cb_u.cb = test_set_pan_id;
  zb_mlme_set_request(param);
}

static void test_set_pan_id(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t panid = TEST_PAN_ID;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() PANID", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_PANID;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &panid, sizeof(zb_uint16_t));

  req->confirm_cb_u.cb = test_set_rx_on_when_idle;
  zb_mlme_set_request(param);
}

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t rx_on_when_idle = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &rx_on_when_idle, sizeof(zb_uint8_t));

  req->confirm_cb_u.cb = test_set_association_permit;
  zb_mlme_set_request(param);
}

static void test_set_association_permit(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t association_permit = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() Association permit", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &association_permit, sizeof(zb_uint8_t));

  req->confirm_cb_u.cb = test_mlme_start_request;

  zb_mlme_set_request(param);
}

static void test_mlme_start_request(zb_uint8_t param)
{
  zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  TRACE_MSG(TRACE_APP1, "MLME-START.request()", (FMT__0));

  ZB_BZERO(req, sizeof(zb_mlme_start_req_t));

  req->pan_id = TEST_PAN_ID;
  req->logical_channel = TEST_CHANNEL;
  req->channel_page = TEST_PAGE;
  req->pan_coordinator = 1;      /* will be coordinator */
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;

  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

static void test_mcps_data_request(zb_uint8_t param)
{

  zb_mcps_data_req_params_t *data_req;
  zb_uint8_t *msdu;
  zb_uint8_t msdu_length;
  zb_uint8_t i;

  TRACE_MSG(TRACE_APP1, "MCPS-DATA.request", (FMT__0));
  msdu_length = TEST_MSDU_LENGTH;

  msdu = zb_buf_initial_alloc(param, msdu_length);
  for (i = 0; i < msdu_length; i++)
  {
    msdu[i] = i;
  }

  data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

  TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK", (FMT__0));
  data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->src_addr.addr_short = TEST_FFD0_SHORT_ADDRESS;
  data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->dst_addr.addr_short = TEST_RFD1_SHORT_ADDRESS;
  data_req->dst_pan_id = TEST_PAN_ID;
  data_req->msdu_handle = 0;
  data_req->tx_options = 0x05;
  MAC_CTX().cert_hacks.delay_frame_transmission = 1;
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}


/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

  /* Call the next test step */
  ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));
  zb_buf_free(param);
}

ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param)
{
  zb_ieee_addr_t device_address;

  zb_mlme_associate_indication_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);

  TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.indication()", (FMT__0));
  /*
    Very simple implementation: accept anybody, assign address 0x3344
   */
  ZB_IEEE_ADDR_COPY(device_address, request->device_address);

  TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.response()", (FMT__0));
  ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address, TEST_RFD1_SHORT_ADDRESS, 0);
  ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm()", (FMT__0));
  MAC_CTX().cert_hacks.delay_frame_transmission = 0;
  zb_buf_free(param);
}

ZB_MLME_COMM_STATUS_INDICATION(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-COMM-STATUS.indication()", (FMT__0));
  ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
}

/*! @} */
