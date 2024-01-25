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


#define ZB_TEST_NAME TP_R20_BV_14_ZC
#define ZB_TRACE_FILE_ID 40783

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../common/zb_cert_test_globals.h"

#ifndef ZB_CERTIFICATION_HACKS
#error This test is not applicable without defined macro ZB_CERTIFICATION_HACKS
#endif

#define CONNECTION_TIMEOUT  25
//#define TEST_CHANNEL (1l << 24)

static const zb_ieee_addr_t g_ieee_addr      = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t dut_ieee_addr    = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_ieee_addr_t g_aps_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  /* let's always be coordinator */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_pan_id(0x1aaa);
  zb_set_use_extended_pan_id(g_aps_ext_pan_id);

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* accept only one child */
  zb_set_max_children(1);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  zb_set_nvram_erase_at_start(ZB_TRUE);
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


static void buffer_test_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "buffer_test_cb", (FMT__H, param));

  if (param == ZB_TP_BUFFER_TEST_OK)
  {
    TRACE_MSG(TRACE_APS1, "buffer_test_cb: status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "buffer_test_cb: status ERROR(%h)", (FMT__H, param));
  }
}


/* Param - value of nwkProtocolVersion, protocol version hardcoded in core stack */
static void send_data(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  TRACE_MSG(TRACE_APS1, "send_test_request with nwkProtocol version =  0x%x", (FMT__H, param));

  if (!buf)
  {
    TRACE_MSG(TRACE_APS1, "send_data: error - unable to get data buffer", (FMT__0));
    return;
  }

  ZB_CERT_HACKS().override_nwk_protocol_version = param;

  if (param == 0x02)
  {
    zb_buffer_test_req_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->len = 0x0A;
    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) dut_ieee_addr);
    ZB_CERT_HACKS().override_nwk_protocol_version = param;

    zb_tp_buffer_test_request(buf, buffer_test_cb);
  }
  else
  {
    zb_nlde_data_req_t *nldereq;
    zb_address_ieee_ref_t ref;
    zb_uint8_t *nwk_payload;
    int i;

    /* fill 10 bytes by zeroes */

    nwk_payload = zb_buf_initial_alloc(buf, 10 * sizeof(zb_uint8_t));
    for (i = 0; i < 10; ++i)
    {
      nwk_payload[i] = 0;
    }

    nldereq = ZB_BUF_GET_PARAM(buf, zb_nlde_data_req_t);
    ZB_BZERO(nldereq, sizeof(zb_nlde_data_req_t));
    nldereq->radius = MAX_NWK_RADIUS;
    nldereq->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    nldereq->nonmember_radius = 0; /* if multicast, get it from APS IB */
    nldereq->discovery_route = 1;  /* always! see 2.2.4.1.1.3 */
    ZB_ASSERT(zb_address_by_ieee((zb_uint8_t*)dut_ieee_addr, ZB_FALSE, ZB_TRUE, &ref) == RET_OK);
    {
      zb_address_map_t *ent = &ZG->addr.addr_map[ref];
      nldereq->dst_addr = ent->addr;
    }
    /* nldereq->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) dut_ieee_addr); */
    nldereq->ndsu_handle = 0;
    ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, buf);
  }
}


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

        /* Protocol version 0x03 */
        ZB_SCHEDULE_ALARM(send_data, 0x03, (CONNECTION_TIMEOUT+5) * ZB_TIME_ONE_SECOND);
        /* Protocol version 0x02(Pro version) */
        ZB_SCHEDULE_ALARM(send_data, 0x02, (CONNECTION_TIMEOUT+10) * ZB_TIME_ONE_SECOND);
        /* Protocol version 0x04 */
        ZB_SCHEDULE_ALARM(send_data, 0x04, (CONNECTION_TIMEOUT+15) * ZB_TIME_ONE_SECOND);
        /* Protocol version 0x02(Pro version) */
        ZB_SCHEDULE_ALARM(send_data, 0x02, (CONNECTION_TIMEOUT+20) * ZB_TIME_ONE_SECOND);
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
