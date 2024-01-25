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
/* PURPOSE: P/154/MAC/CHANNEL-ACCESS-03 DUT FFD1
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_CHANNEL_ACCESS_03_DUT_FFD1
#define ZB_TRACE_FILE_ID 63882

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac_globals.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_channel_access_03_common.h"
#include "tp_154_mac_channel_access_03_dut_hal.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_prepare_resp_packet(zb_uint8_t param, zb_uint32_t r_timestamp);

static zb_ieee_addr_t g_ieee_addr_dut_ffd1 = TEST_DUT_FFD1_MAC_ADDRESS;

typedef ZB_PACKED_PRE struct test_resp_payload_s
{
  zb_uint8_t lead_ch;
  zb_uint8_t req_copy[TEST_DUT_MSDU_COPY_LEN];
  zb_uint16_t ramp_up;
  zb_uint16_t ramp_down;
  zb_uint32_t r_timestamp;
  zb_uint32_t t_timestamp;
} ZB_PACKED_STRUCT  test_resp_payload_t;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_channel_access_03_dut_ffd1");

  ZB_SCHEDULE_CALLBACK(test_started_cb, 0);

  ZB_SET_TRACE_LEVEL(1);
  ZB_SET_TRACE_MASK(0);
  ZB_SET_TRAF_DUMP_OFF();

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

  TRACE_MSG(TRACE_MAC_API1, "Device STARTED OK", (FMT__0));

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
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr_dut_ffd1, sizeof(zb_ieee_addr_t));

  req->confirm_cb_u.cb = test_set_short_addr;
  zb_mlme_set_request(param);
}


static void test_set_short_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t short_addr = TEST_DUT_FFD1_SHORT_ADDRESS;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &short_addr, sizeof(zb_ieee_addr_t));

  req->confirm_cb_u.cb = test_set_rx_on_when_idle;
  zb_mlme_set_request(param);
}

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t rx_on_when_idle = TEST_RX_ON_WHEN_IDLE;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &rx_on_when_idle, sizeof(zb_uint16_t));

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
  req->pan_coordinator = 0;      /* will be router */
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;

  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

static void test_prepare_resp_packet(zb_uint8_t param, zb_uint32_t timestamp)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t mhr_size = zb_parse_mhr(&mhr, param);
  zb_uint8_t *req_msdu = (zb_uint8_t *)zb_buf_begin(param) + mhr_size + sizeof(zb_uint8_t);
  zb_uint8_t req_msdu_copy[TEST_DUT_MSDU_COPY_LEN];
  test_resp_payload_t *msdu;
  zb_mcps_data_req_params_t *data_req;
  zb_uint8_t *ptr;
  zb_uint8_t i;

  TRACE_MSG(TRACE_APP1, "MCPS-DATA.request", (FMT__0));

  /* msdu: |ascii '@' | 10 bytes copied from incoming packet | rampUP length | rampDOWN length | timestamp of 1st packet received | timestamp of start of transmission | padded with '5' ascii */

  ZB_MEMCPY(req_msdu_copy, req_msdu, TEST_DUT_MSDU_COPY_LEN);
  msdu = zb_buf_initial_alloc(param, TEST_MAX_MSDU_LENGTH);

  msdu->lead_ch = TEST_RESP_FLAG;
  ZB_MEMCPY(&(msdu->req_copy), req_msdu_copy, TEST_DUT_MSDU_COPY_LEN);
  msdu->ramp_up = ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
  msdu->ramp_down = ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;
  msdu->r_timestamp = timestamp / ZB_EU_FSK_SYMBOL_DURATION_USEC;  /* timestamp in symbols */

  ptr = (zb_uint8_t*)msdu;
  for (i = sizeof(test_resp_payload_t); i < TEST_MAX_MSDU_LENGTH; i++)
  {
    ptr[i] = TEST_FILL_CH;
  }

  data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

  data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->src_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
  data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->dst_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;
  data_req->dst_pan_id = TEST_PAN_ID;
  data_req->msdu_handle = 1;
  data_req->tx_options = 0x00;
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

  /* Call the next test step */
  ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
  zb_mac_mhr_t mhr;
  zb_uint8_t mhr_size = zb_parse_mhr(&mhr, param);
  zb_uint8_t *msdu = (zb_uint8_t *)zb_buf_begin(param) + mhr_size;
  zb_uint32_t r_timestamp;
  test_resp_payload_t *resp_msdu;

  TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));

  if (msdu[0] == TEST_RQST_FLAG)
  {
    TEST_HAL_INDICATE_ACTION(DATA_INDICATION_EV_ID);
    TRACE_MSG(TRACE_APP1, "RQST received", (FMT__0));

#ifdef ZB_CONFIG_LINUX_NSNG
    r_timestamp = osif_transceiver_time_get();
#else
    r_timestamp = *ZB_BUF_GET_PARAM(param, zb_uint32_t);
#endif
    test_prepare_resp_packet(param, r_timestamp);

    if (osif_transceiver_time_get() - r_timestamp < TEST_DUT_RESPONSE_START_SYMB * ZB_EU_FSK_SYMBOL_DURATION_USEC)
    {
      TRACE_MSG(TRACE_APP1, "Wait ResponseStart", (FMT__0));
      osif_sleep_using_transc_timer(TEST_DUT_RESPONSE_START_SYMB * ZB_EU_FSK_SYMBOL_DURATION_USEC - (osif_transceiver_time_get() - r_timestamp));
    }

    /* fill timestamp before send */
    resp_msdu = (test_resp_payload_t*)zb_buf_begin(param);
    resp_msdu->t_timestamp = osif_transceiver_time_get() / ZB_EU_FSK_SYMBOL_DURATION_USEC;

    TRACE_MSG(TRACE_APP1, "transmit response packet", (FMT__0));
    TEST_HAL_INDICATE_ACTION(LBT_START_EV_ID);
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
  }
  else
  {
    zb_buf_free(param);
  }
}

/*! @} */
