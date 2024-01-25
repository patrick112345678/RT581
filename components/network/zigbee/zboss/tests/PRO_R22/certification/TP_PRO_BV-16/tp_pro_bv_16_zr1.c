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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_16_ZR1
#define ZB_TRACE_FILE_ID 40590

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"

#include "../nwk/nwk_internal.h"
#include "../common/zb_cert_test_globals.h"


#define START_RANDOM 0x13579bdf

static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

static zb_uint16_t g_zr2_short_addr;
int g_test_completed = 0;

#if 0  /* unused */
static void test_get_peer_addr_resp(zb_uint8_t param);
static void test_send_nwk_addr_req_wrap(zb_uint8_t param);
static void test_send_nwk_addr_req(zb_uint8_t param);
#endif  /* unused */
static void device_annce_cb(zb_zdo_device_annce_t *da);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_r1);

  /* join as a router */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_max_children(0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_zdo_register_device_annce_cb(device_annce_cb);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_APP1, ">> device_annce_cb, da %p", (FMT__P, da));

  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_r2))
  {
    g_zr2_short_addr = da->nwk_addr;
  }

  TRACE_MSG(TRACE_APP1, "<< device_annce_cb", (FMT__0));
}


static void send_dev_annc(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr;
  zb_bool_t secure = ZB_FALSE;

  zb_cert_test_set_network_addr(g_zr2_short_addr);

  TRACE_MSG(TRACE_ERROR, "Send dev_annce %d 0x%x", (FMT__D_D, param, ZB_PIBCACHE_NETWORK_ADDRESS()));
#ifdef ZB_SECURE
  secure = (zb_bool_t)(nldereq->security_enable
                       && ZG->aps.authenticated && ZB_NIB().secure_all_frames
                       && ZB_NIB_SECURITY_LEVEL());
#endif
  nwhdr = nwk_alloc_and_fill_hdr(param,
                                 ZB_PIBCACHE_NETWORK_ADDRESS(),
                                 ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                 ZB_FALSE,
                                 secure,
                                 ZB_TRUE, ZB_FALSE);

  nwhdr->radius = (zb_uint8_t)(zb_cert_test_nib_get_max_depth() << 1);
  ZB_SCHEDULE_CALLBACK(zdo_send_device_annce, param);
}


static void change_short_addr(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);

  TRACE_MSG(TRACE_ERROR, ">>change_short_addr", (FMT__0));

  if (buf)
  {
    g_test_completed = 1;
    zb_cert_test_set_network_addr(g_zr2_short_addr);

    zb_nwk_pib_set(buf,
                   ZB_PIB_ATTRIBUTE_SHORT_ADDRESS,
                   &g_zr2_short_addr,
                   2,
                   send_dev_annc);

  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ERROR, "<<change_short_addr", (FMT__0));
}


#if 0  /* unused */
static void test_get_peer_addr_resp(zb_uint8_t param)
{
  zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t*) zb_buf_begin(param);

  TRACE_MSG(TRACE_ZDO1, ">>test_get_peer_addr_resp, param = %d, status = %d",
            (FMT__D_D, param, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH16(&g_zr2_short_addr, &resp->nwk_addr);
  }
  zb_buf_free(param);

  TRACE_MSG(TRACE_ZDO1, "<<test_get_peer_addr_resp", (FMT__0));
}


static void test_send_nwk_addr_req_wrap(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(test_send_nwk_addr_req);
}


static void test_send_nwk_addr_req(zb_uint8_t param)
{
  zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);
  zb_callback_t cb;

  TRACE_MSG(TRACE_ZDO1, ">>test_send_secur_nwk_addr_req, param = %h", (FMT__H, param));

  req_param->dst_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
  req_param->start_index = 0;
  req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
  ZB_IEEE_ADDR_COPY(req_param->ieee_addr, g_ieee_addr_r2);
  cb = test_get_peer_addr_resp;
  zb_zdo_nwk_addr_req(param, cb);

  TRACE_MSG(TRACE_ZDO1, "<<test_send_secur_nwk_addr_req", (FMT__0));
}
#endif


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

	if (!g_test_completed)
        {
          /* ZB_SCHEDULE_ALARM(test_send_nwk_addr_req_wrap, 0, 20*ZB_TIME_ONE_SECOND); */
          ZB_SCHEDULE_ALARM(change_short_addr, 0, 40*ZB_TIME_ONE_SECOND);
        }
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  zb_buf_free(param);
}
