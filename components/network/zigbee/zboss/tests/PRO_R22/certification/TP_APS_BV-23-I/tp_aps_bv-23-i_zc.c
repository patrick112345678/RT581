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
/* PURPOSE: 11.23 TP/APS/ BV-23-I Binding with Groups (HC V1 RX DUT)
*/

#define ZB_TEST_NAME TP_APS_BV_23_I_ZC
#define ZB_TRACE_FILE_ID 40687

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_aps_bv-23-i_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_pan_id(TEST_PAN_ID);
  zb_set_long_address(g_ieee_addr_c);
  zb_set_max_children(3);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* zb_cert_test_set_security_level(0); */

  ZB_NIB_SET_USE_MULTICAST(USE_NWK_MULTICAST);

//  TRACE_MSG(TRACE_COMMON1,
//            "aps_designated_coordinator %d",
//            (FMT__D, ZB_AIB().aps_designated_coordinator));

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


static void unbind_req_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "unbind_req_cb", (FMT__0));
  zb_buf_free(param);
}

static void unbind_req(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_zdo_bind_req_param_t *bind_param = NULL;
  zb_uint16_t addr = 0;

  TRACE_MSG(TRACE_APP1, ">>unbind_req addr = 0x%x", (FMT__H, param));
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "unbind_req: error - unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }

  //! [zb_zdo_unbind_req]
  addr = param;
  bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
  ZB_MEMCPY(bind_param->src_address, g_ieee_addr_r1, sizeof(zb_ieee_addr_t));
  bind_param->src_endp = ZC_EP_SRC;
  bind_param->cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
  bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
  bind_param->dst_address.addr_short = addr;
  bind_param->req_dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_r1);

  zb_zdo_unbind_req(buf, unbind_req_cb);
  //! [zb_zdo_unbind_req]

  TRACE_MSG(TRACE_APP1, "<<unbind_req", (FMT__0));
}

static void bind_req_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "bind_req_cb", (FMT__0));
  zb_buf_free(param);
}

static void bind_req(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_zdo_bind_req_param_t *bind_param = NULL;
  zb_uint16_t addr = 0;

  TRACE_MSG(TRACE_APP1, ">>bind_req addr = 0x%x", (FMT__H, param));
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "bind_req: error - unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }

  addr = param;
  bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
  //ZB_MEMCPY(bind_param->src_address, g_ieee_addr_c, sizeof(zb_ieee_addr_t));
  ZB_MEMCPY(bind_param->src_address, g_ieee_addr_r1, sizeof(zb_ieee_addr_t));
  bind_param->src_endp = ZC_EP_SRC;
  bind_param->cluster_id = TP_BUFFER_TEST_REQUEST_CLID;
  bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
  bind_param->dst_address.addr_short = addr;
  bind_param->req_dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_r1);

  zb_zdo_bind_req(buf, bind_req_cb);
  TRACE_MSG(TRACE_APP1, "<<bind_req", (FMT__0));
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

        test_step_register(bind_req,   GROUP_3, TP_APS_BV_23_I_STEP_1_TIME_ZC);
        test_step_register(bind_req,   GROUP_4, TP_APS_BV_23_I_STEP_7_TIME_ZC);
        test_step_register(unbind_req, GROUP_3, TP_APS_BV_23_I_STEP_9_TIME_ZC);
        test_control_start(TEST_MODE, TP_APS_BV_23_I_STEP_1_DELAY_ZC);
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

  if (param)
  {
    zb_buf_free(param);
  }
}

