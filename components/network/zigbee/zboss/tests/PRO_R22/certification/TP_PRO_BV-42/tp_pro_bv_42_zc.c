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


#define ZB_TEST_NAME TP_PRO_BV_42_ZC

#define APS_FRAGMENTATION

#define ZB_TRACE_FILE_ID 40074
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define TEST_ASDU_LENGTH 240
#define TEST_FRAGMENTS_NUM 8
#define TEST_APSC_MAX_WINDOW_SIZE 3
#define TEST_INTERFRAME_DELAY 100 /* milliseconds */
#define TEST_SOURCE_EP 1
#define TEST_FRAG_BLOCK_LEN 30

#define TP_PRO_BV_42_STEP_1_DELAY_ZC  (360 * ZB_TIME_ONE_SECOND)
#define TP_PRO_BV_42_STEP_1_TIME_ZC   (5 * ZB_TIME_ONE_SECOND)

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

static zb_ieee_addr_t r1_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static zb_ieee_addr_t r2_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static void send_frag_data(zb_bufid_t buf, zb_uint16_t user_data);
static void send_frag_data_delayed(zb_bufid_t unused);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* accept only one child */
  zb_set_max_children(1);
  ZB_CERT_HACKS().frag_skip_node_descr = 1;
  ZB_CERT_HACKS().frag_block_size = TEST_FRAG_BLOCK_LEN;

  MAC_ADD_VISIBLE_LONG(r1_ieee_addr);
  zb_bdb_set_legacy_device_support(ZB_TRUE);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  if ( zboss_start() != RET_OK )
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
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
#ifndef NCP_MODE_HOST
        ZB_ZDO_NODE_DESC()->max_incoming_transfer_size = TEST_ASDU_LENGTH;
        ZB_ZDO_NODE_DESC()->max_outgoing_transfer_size = TEST_ASDU_LENGTH;
        ZG->aps.aib.aps_max_window_size = TEST_APSC_MAX_WINDOW_SIZE;
        ZG->aps.aib.aps_interframe_delay = TEST_INTERFRAME_DELAY;
#else
        ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

        test_step_register(send_frag_data_delayed, 0, TP_PRO_BV_42_STEP_1_TIME_ZC);
        test_control_start(TEST_MODE, TP_PRO_BV_42_STEP_1_DELAY_ZC);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void send_frag_data_delayed(zb_bufid_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed_ext(send_frag_data, 0, TEST_ASDU_LENGTH + sizeof(zb_apsde_data_req_t));
}

static void send_frag_data(zb_bufid_t buf, zb_uint16_t user_data)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_uint8_t i;

  ZVUNUSED(user_data);

  ptr = zb_buf_initial_alloc(buf, TEST_ASDU_LENGTH);
  req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);

  req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX | ZB_APSDE_TX_OPT_FRAG_PERMITTED;
  req->profileid = ZB_TEST_PROFILE_ID;
  req->src_endpoint = TEST_SOURCE_EP;
  req->dst_endpoint = ZB_TEST_PROFILE_EP;
  req->clusterid = TP_TRANSMIT_COUNTED_PACKETS_REQ_CLID;

  ZB_MEMCPY(&req->dst_addr, r2_ieee_addr, sizeof(zb_addr_u));

  for (i = 0; i < TEST_ASDU_LENGTH; ++i)
  {
    ptr[i] = i + 1;
  }

  TRACE_MSG(TRACE_APS3, "Sending a fragmented request buf %hd", (FMT__H, buf));
  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
}
