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
/* PURPOSE: P/154/MAC/CHANNEL-ACCESS-03 TH FFD0
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_CHANNEL_ACCESS_03_TH_FFD0
#define ZB_TRACE_FILE_ID 63881

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_channel_access_03_common.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_prepare_second_packet(zb_uint8_t param, zb_uint8_t size, zb_bool_t leading_fragment);
static void test_send_first_packet(zb_uint8_t param);
static void test_incr_test_step(zb_uint8_t tx_timeout);
static void test_send_packet(zb_uint8_t param);
static void test_sp_transmit_step(zb_uint8_t param, zb_bool_t need_time_gap);
static void test_send_packet_pair(zb_uint8_t param_sp);

static zb_ieee_addr_t g_ieee_addr_th_ffd0 = TEST_TH_FFD0_MAC_ADDRESS;
static zb_uint16_t g_time_offset[] = TEST_TH_OFFSET_SYMB;
static zb_uint8_t g_sp_nof_fragments[] = TEST_TH_NOF_FRAGMENTS;
static zb_uint16_t g_size_of_first[] = TEST_TH_SIZE_OF_FIRST_FR_SYMB;
static zb_uint16_t g_size_of_last[] = TEST_TH_SIZE_OF_LAST_FR_SYMB;
static zb_uint16_t g_fragment_gap[] = TEST_TH_GAP_SYMB;
static zb_uint16_t g_test_scen;
static zb_uint32_t g_timestamp;

typedef ZB_PACKED_PRE struct test_fp_payload_s
{
  zb_uint8_t lead_ch;
  zb_uint16_t packet_number;
  zb_uint32_t timestamp;
  zb_uint16_t offset;
  zb_uint16_t length_sp;
  zb_uint16_t ramp_up;
  zb_uint16_t ramp_down;
} ZB_PACKED_STRUCT  test_fp_payload_t;

typedef ZB_PACKED_PRE struct test_sp_payload_s
{
  zb_uint8_t lead_ch;
  zb_uint16_t packet_number;
  zb_uint16_t offset;
  zb_uint16_t length_sp;
  zb_uint8_t fragment_count;
  zb_uint16_t gap;
  zb_uint32_t timestamp;
  zb_uint16_t ramp_up;
  zb_uint16_t ramp_down;
} ZB_PACKED_STRUCT  test_sp_payload_t;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_channel_access_03_th_ffd0");

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

  g_test_scen = 0;
  g_timestamp = osif_transceiver_time_get();

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
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr_th_ffd0, sizeof(zb_ieee_addr_t));

  req->confirm_cb_u.cb = test_set_short_addr;
  zb_mlme_set_request(param);
}

static void test_set_short_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t short_addr = TEST_TH_FFD0_SHORT_ADDRESS;

  TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &short_addr, sizeof(zb_ieee_addr_t));

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

static void test_prepare_second_packet(zb_uint8_t param, zb_uint8_t size, zb_bool_t leading_fragment)
{
  zb_uint8_t *ptr;
  zb_uint8_t packet_length = size - ZB_MAC_EU_FSK_SHR_LEN_BYTES - ZB_MAC_EU_FSK_PHR_LEN_BYTES - TEST_MFR_LEN;
  zb_uint16_t length_sp = 0;
  zb_uint8_t i;
  test_sp_payload_t *payload;

  TRACE_MSG(TRACE_APP1, "test_prepare_second_packet(): g_test_scen: %hx, size %hd", (FMT__H_D, g_test_scen, size));

  if (!leading_fragment)
  {
    TRACE_MSG(TRACE_APP1, "test_prepare_second_packet(): not leading fragment", (FMT__0));

    ptr = zb_buf_initial_alloc(param, packet_length);
    for (i = 0; i < packet_length; i++)
    {
      ptr[i] = 0x35;
    }
  }
  else
  {
    /* payload: |ascii '#' | packet number | Offset between first and second packets | Length of second packet | fragment count(only one fragment) | gap in symbols(not used) | timestamp | rampUP length | rampDOWN length | padded with '5' ascii */
    payload = zb_buf_initial_alloc(param, packet_length);

    payload->lead_ch = TEST_INTERF_FLAG;
    payload->packet_number = g_test_scen;
    payload->offset = g_time_offset[g_test_scen];

    length_sp = g_size_of_last[g_test_scen] + g_size_of_first[g_test_scen];
    if (g_sp_nof_fragments[g_test_scen] > 2)
    {
      length_sp += (g_sp_nof_fragments[g_test_scen] - 2) * TEST_TH_PACKET_LENGTH * ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET;
    }

    payload->length_sp = length_sp;
    payload->fragment_count = ((g_size_of_last[g_test_scen]) ? (g_sp_nof_fragments[g_test_scen] - 1) : 1);
    payload->gap = g_fragment_gap[g_test_scen];
    payload->ramp_up = ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
    payload->ramp_down = ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;

    ptr = (zb_uint8_t*)payload;
    for (i = sizeof(test_sp_payload_t); i < packet_length; i++)
    {
      ptr[i] = TEST_FILL_CH;
    }
  }
}

static void test_send_first_packet(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_mac_mhr_t mhr;
  zb_uint8_t mhr_len;
  zb_uint32_t timestamp = osif_transceiver_time_get() / ZB_EU_FSK_SYMBOL_DURATION_USEC; /* timestamp in symbols */
  zb_uint16_t length_sp = 0;
  zb_uint8_t i;
  test_fp_payload_t *msdu;

  TRACE_MSG(TRACE_APP1, "test_send_first_packet(): g_test_scen: %hx", (FMT__H, g_test_scen));

  /* MHR: FC | SNr | PAN ID | DST | SRC */
  mhr_len = zb_mac_calculate_mhr_length(ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZB_ADDR_16BIT_DEV_OR_BROADCAST, 1);

  ZB_BZERO2(mhr.frame_control);
  ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_DATA);

  /* check: PAN ID Compression */
  ZB_FCF_SET_PANID_COMPRESSION_BIT(mhr.frame_control, ZB_TRUE);
  ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
  ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);

  mhr.seq_number = ZB_MAC_DSN();
  ZB_INC_MAC_DSN();

  mhr.dst_pan_id = TEST_PAN_ID;
  mhr.dst_addr.addr_short = TEST_DUT_FFD1_SHORT_ADDRESS;
  mhr.src_pan_id = TEST_PAN_ID;
  mhr.src_addr.addr_short = TEST_TH_FFD0_SHORT_ADDRESS;

  ptr = zb_buf_initial_alloc(param, mhr_len);
  zb_mac_fill_mhr(ptr, &mhr);

   /* msdu: |ascii '?' | packet number | timestamp | Offset between first and second packets | Length of second packet | rampUP length | rampDOWN length | padded with '5' ascii */

  msdu = zb_buf_alloc_right(param, TEST_MAX_MSDU_LENGTH);

  msdu->lead_ch = TEST_RQST_FLAG;
  msdu->packet_number = g_test_scen;
  msdu->timestamp = timestamp;
  msdu->offset = g_time_offset[g_test_scen];

  length_sp = g_size_of_last[g_test_scen] + g_size_of_first[g_test_scen];
  if (g_sp_nof_fragments[g_test_scen] > 2)
  {
    length_sp += (g_sp_nof_fragments[g_test_scen] - 2) * TEST_TH_PACKET_LENGTH * ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET;
  }


  msdu->length_sp = length_sp;
  msdu->ramp_up = ZB_MAC_DUTY_CYCLE_RAMP_UP_SYMBOLS;
  msdu->ramp_down = ZB_MAC_DUTY_CYCLE_RAMP_DOWN_SYMBOLS;

  ptr = (zb_uint8_t*)msdu;
  for (i = sizeof(test_fp_payload_t); i < TEST_MAX_MSDU_LENGTH; i++)
  {
    ptr[i] = TEST_FILL_CH;
  }

  if (RET_OK != ZB_SCHEDULE_TX_CB(test_send_packet, param))
  {
    TRACE_MSG(TRACE_ERROR, "MAC TX Q overflow", (FMT__0));
    ZB_ASSERT(0);
  }
}

static void test_incr_test_step(zb_uint8_t tx_timeout)
{
  TRACE_MSG(TRACE_APP1, "test_incr_test_step()", (FMT__0));

  g_test_scen++;

  if (g_test_scen < TEST_MAX_SCEN)
  {
    TRACE_MSG(TRACE_APP1, "test_incr_test_step(): call next test scen", (FMT__0));
    ZB_SCHEDULE_ALARM(test_send_packet_pair, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(TEST_TH_TIMEOUT + tx_timeout));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Test finished", (FMT__0));
  }
}

static void test_send_packet(zb_uint8_t param)
{
  zb_uint8_t packet_length = zb_buf_len(param);
  zb_uint8_t *ptr = NULL;
  test_sp_payload_t *interf;

  TRACE_MSG(TRACE_APP1, "test_send_packet(): packet_length %hd", (FMT__D, packet_length));

  ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);

  ZB_BZERO(ptr, packet_length);

  ZB_MEMCPY(ptr, zb_buf_begin(param), packet_length);

  if (ptr[0] == TEST_INTERF_FLAG)
  {
    /* fill timestamp before send */
    interf = (test_sp_payload_t*)ptr;
    interf->timestamp = osif_transceiver_time_get() / ZB_EU_FSK_SYMBOL_DURATION_USEC;
  }

  MAC_ADD_FCS(MAC_CTX().operation_buf);

/*  MAC_CTX().flags.tx_q_busy = ZB_TRUE; */
  MAC_CTX().flags.tx_radio_busy = ZB_TRUE;

  ZB_MAC_CLEAR_ACK_NEEDED();
  ZB_MAC_CLEAR_ACK_TIMED_OUT();

  ZB_SET_MAC_STATUS(MAC_SUCCESS);

  g_timestamp = osif_transceiver_time_get();
  ZB_TRANS_SEND_FRAME_SUB_GHZ(0, MAC_CTX().operation_buf, ZB_MAC_TX_WAIT_NONE);

  zb_buf_free(param);
}

static void test_sp_transmit_step(zb_uint8_t param, zb_bool_t need_time_gap)
{
  zb_time_t callbacks_tmo, frame_sent_tmo;
  TRACE_MSG(TRACE_APP1, "test_transmit_step(): need_time_gap %hd", (FMT__D, need_time_gap));

  callbacks_tmo = 0;
  frame_sent_tmo = TEST_TH_PACKET_LENGTH * ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET * ZB_EU_FSK_SYMBOL_DURATION_USEC;

#ifndef ZB_CONFIG_LINUX_NSNG
  callbacks_tmo = osif_transceiver_time_get() - g_timestamp;
  if (frame_sent_tmo > callbacks_tmo)
  {
    osif_sleep_using_transc_timer(frame_sent_tmo - callbacks_tmo);
  }
  else
  {
    callbacks_tmo -= frame_sent_tmo;
  }
#else
  osif_sleep_using_transc_timer(frame_sent_tmo);
#endif

  if (need_time_gap && g_fragment_gap[g_test_scen] * ZB_EU_FSK_SYMBOL_DURATION_USEC > callbacks_tmo)
  {
    /* sleep gap between second frame fragments */
    osif_sleep_using_transc_timer(g_fragment_gap[g_test_scen] * ZB_EU_FSK_SYMBOL_DURATION_USEC - callbacks_tmo);
  }
  else if (!need_time_gap && g_time_offset[g_test_scen] * ZB_EU_FSK_SYMBOL_DURATION_USEC > callbacks_tmo)
  {
    /* sleep after sending first frame */
    osif_sleep_using_transc_timer(g_time_offset[g_test_scen] * ZB_EU_FSK_SYMBOL_DURATION_USEC - callbacks_tmo);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "test_transmit_step(): time between callbacks is too big", (FMT__0));
  }

  if (RET_OK != ZB_SCHEDULE_TX_CB(test_send_packet, param))
  {
    TRACE_MSG(TRACE_ERROR, "MAC TX Q overflow", (FMT__0));
    ZB_ASSERT(0);
  }
}

static void test_send_packet_pair(zb_uint8_t unused)
{
  zb_bufid_t buf;
  zb_uint8_t param_fp;
  zb_uint8_t param_sp_lead;
  zb_uint8_t param_sp_fr1;
  zb_uint8_t param_sp_fr2;
  zb_uint8_t param_sp_last;
  zb_uint16_t tx_timeout = 0;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, "test_send_packet_pair(): g_test_scen: %hx", (FMT__H, g_test_scen));

  /* prepare second frame */
  if (g_sp_nof_fragments[g_test_scen])
  {
    buf = zb_buf_get_out();
    if (buf)
    {
      param_sp_lead = buf;
    }
    else
    {
      TRACE_MSG(TRACE_APP1, "failed while buffer alloc", (FMT__0));
      ZB_ASSERT(0);
    }
    test_prepare_second_packet(param_sp_lead, (g_size_of_first[g_test_scen] / ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET), ZB_TRUE);
    tx_timeout += (((test_sp_payload_t*)zb_buf_begin(param_sp_lead))->length_sp);

    if (g_sp_nof_fragments[g_test_scen] > 1)
    {
      buf = zb_buf_get_out();
      if (buf)
      {
	param_sp_last = buf;
      }
      else
      {
	TRACE_MSG(TRACE_APP1, "failed while buffer alloc", (FMT__0));
	ZB_ASSERT(0);
      }
      test_prepare_second_packet(param_sp_last, (g_size_of_last[g_test_scen] / ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET), ZB_FALSE);
    }

    if (g_sp_nof_fragments[g_test_scen] > 2)
    {
      buf = zb_buf_get_out();
      if (buf)
      {
	param_sp_fr1 = buf;
      }
      else
      {
	TRACE_MSG(TRACE_APP1, "failed while buffer alloc", (FMT__0));
        ZB_ASSERT(0);
      }
      test_prepare_second_packet(param_sp_fr1, TEST_TH_PACKET_LENGTH, ZB_FALSE);
    }

    if (g_sp_nof_fragments[g_test_scen] > 3)
    {
      buf = zb_buf_get_out();
      if (buf)
      {
	param_sp_fr2 = buf;
      }
      else
      {
	TRACE_MSG(TRACE_APP1, "failed while buffer alloc", (FMT__0));
        ZB_ASSERT(0);
      }
      test_prepare_second_packet(param_sp_fr2, TEST_TH_PACKET_LENGTH, ZB_FALSE);
    }
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "test_send_packet_pair(): buffers for second frame not allocated", (FMT__0));
  }

  /* send first frame */
  buf = zb_buf_get_out();
  if (buf)
  {
    param_fp = buf;
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "failed while buffer alloc", (FMT__0));
    ZB_ASSERT(0);
  }
  ZB_SCHEDULE_CALLBACK(test_send_first_packet, param_fp);
  tx_timeout += TEST_TH_PACKET_LENGTH * ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET;

  /* send second frame */
  if (g_sp_nof_fragments[g_test_scen] == 1)
  {
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_lead, ZB_FALSE);
  }
  else if (g_sp_nof_fragments[g_test_scen] == 2)
  {
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_lead, ZB_FALSE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_last, ZB_TRUE);
  }
  else if (g_sp_nof_fragments[g_test_scen] == 3)
  {
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_fr1, ZB_FALSE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_lead, ZB_TRUE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_last, ZB_TRUE);
  }
  else if (g_sp_nof_fragments[g_test_scen] == 4)
  {
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_fr1, ZB_FALSE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_fr2, ZB_TRUE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_lead, ZB_TRUE);
    ZB_SCHEDULE_CALLBACK2(test_sp_transmit_step, param_sp_last, ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "test_send_packet_pair(): only first frame transmitted", (FMT__0));
  }

  tx_timeout *= ZB_EU_FSK_SYMBOL_DURATION_USEC;

  ZB_SCHEDULE_CALLBACK(test_incr_test_step, ZB_USECS_TO_MILLISECONDS(tx_timeout));
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

  zb_mac_set_tx_power(0x0E);
  ZB_SCHEDULE_ALARM(test_send_packet_pair, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));

  zb_buf_free(param);
}

/*! @} */
