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


#define ZB_TEST_NAME TP_R22_BV_16_ZR2
#define ZB_TRACE_FILE_ID 40082

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../nwk/nwk_internal.h"

#include "../common/zb_cert_test_globals.h"

#define TEST_STEP_1_DELAY (20 * ZB_TIME_ONE_SECOND)
#define TEST_STEP_2_DELAY_REL (3 * ZB_TIME_ONE_SECOND)
#define TEST_STEP_3_DELAY_REL (3 * ZB_TIME_ONE_SECOND)
#define TEST_STEP_4_DELAY_REL (9 * ZB_TIME_ONE_SECOND)

static zb_uint8_t g_received_parent_dev_annce = 0;
static zb_uint16_t g_dut_zr_address = 0;
static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00};

static void test_broadcast_buffer_test_req_delayed(zb_uint8_t unused);
static void send_spoofed_buffer_test_req(zb_uint8_t param);
static void send_spoofed_buffer_test_req_delayed(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zr2");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_max_children(0);
  MAC_ADD_INVISIBLE_SHORT(0x0000);

  ZB_CERT_HACKS().nwk_disable_passive_acks = 1;

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


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  ZVUNUSED(da);

  TRACE_MSG(TRACE_APS1, "dev_annce_cb %x",(FMT__D, da->nwk_addr));
  if (da->nwk_addr == g_dut_zr_address)
  {
    g_received_parent_dev_annce++;

    if (g_received_parent_dev_annce == 1)
    {
      ZB_SCHEDULE_ALARM(test_broadcast_buffer_test_req_delayed, 0, TEST_STEP_2_DELAY_REL);
    }
    else
    {
      ZB_SCHEDULE_ALARM(send_spoofed_buffer_test_req_delayed, 0, TEST_STEP_3_DELAY_REL);
      ZB_SCHEDULE_ALARM(send_spoofed_buffer_test_req_delayed, 0, TEST_STEP_3_DELAY_REL + TEST_STEP_4_DELAY_REL);
    }
  }
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        g_dut_zr_address = zb_cert_test_get_parent_short_addr();

        zb_zdo_register_device_annce_cb(dev_annce_cb);
        ZB_SCHEDULE_ALARM(test_broadcast_buffer_test_req_delayed, 0, TEST_STEP_1_DELAY);
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      if (status == 0)
      {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        TRACE_MSG(TRACE_APS1, "Dev annce, addr %x", (FMT__D, dev_annce_params->device_short_addr));
      }

      break;
    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}

static void test_broadcast_buffer_test_req(zb_uint8_t param)
{
  zb_buffer_test_req_param_t *req_param;

  req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;

  TRACE_MSG(TRACE_ERROR, "send to 0x%x", (FMT__H, req_param->dst_addr));

  zb_tp_buffer_test_request(param, NULL);
}

static void test_broadcast_buffer_test_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">> test_broadcast_buffer_test_req_delayed", (FMT__0));

  if (zb_buf_get_out_delayed(test_broadcast_buffer_test_req) != RET_OK)
  {
    TRACE_MSG(TRACE_APP1, "test_broadcast_buffer_test_req_delayed: zb_buf_get_out_delayed failed", (FMT__0));
  }
}

static void send_spoofed_buffer_test_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">> send_spoofed_buffer_test_req_delayed", (FMT__0));

  if (zb_buf_get_out_delayed(send_spoofed_buffer_test_req) != RET_OK)
  {
    TRACE_MSG(TRACE_APP1, "send_spoofed_buffer_test_req_delayed: zb_buf_get_out_delayed failed", (FMT__0));
  }
}

static void generate_buffer_test_req_nwk(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr;
  zb_bufid_t nsdu = param;

  nwhdr = nwk_alloc_and_fill_hdr(nsdu,
                         g_dut_zr_address,
                         ZB_NWK_BROADCAST_ALL_DEVICES,
                         ZB_FALSE,
                         ZB_TRUE,
                         ZB_FALSE,
                         ZB_FALSE);
  nwhdr->radius = MAX_NWK_RADIUS;
}

void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t apsdu);
static void generate_buffer_test_req_aps(zb_uint8_t param)
{
  zb_uint8_t fc = 0;
  zb_apsde_data_req_t apsreq;
  zb_bufid_t apsdu = param;

  apsreq.profileid = ZB_TEST_PROFILE_ID;
  apsreq.clusterid = TP_BUFFER_TEST_REQUEST_CLID;
  apsreq.dst_endpoint = ZB_TEST_PROFILE_EP;
  apsreq.src_endpoint = ZB_TEST_PROFILE_EP;
  apsreq.dst_addr.addr_short = ZB_NWK_BROADCAST_ALL_DEVICES;
  apsreq.tx_options = ZB_APSDE_TX_OPT_SECURITY_ENABLED;
  apsreq.addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  apsreq.radius = MAX_NWK_RADIUS;

  ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_BROADCAST);

  aps_data_hdr_fill_datareq(fc, &apsreq, apsdu);
}

static void generate_buffer_test_req_payload(zb_bufid_t buf)
{
  zb_buffer_test_req_t *req;
  req = zb_buf_initial_alloc(buf, sizeof(zb_buffer_test_req_t));
  req->len = 1;
  req->req_data[0] = 0xFF; /* let it be really small */
}

static void send_spoofed_buffer_test_req(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> send_spoofed_buffer_test_req (param %hd)", (FMT__H, param));

  generate_buffer_test_req_payload(param);
  generate_buffer_test_req_aps(param);
  generate_buffer_test_req_nwk(param);

  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
}
