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


#define ZB_TEST_NAME TP_R20_BV_10_ZR1
#define ZB_TRACE_FILE_ID 40919

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

//#define TEST_CHANNEL (1l << 24)

/*! \addtogroup ZB_TESTS */
/*! @{ */
//#define TEST_WITH_EMBER_COORDINATOR
#ifdef TEST_WITH_EMBER_COORDINATOR
/* Ember EM-ISA3-01 */
static const zb_ieee_addr_t g_ieee_addr_c = {0xf0, 0x23, 0x07, 0x00, 0x00, 0xed, 0x21, 0x00};
#else
static const zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#endif

static const zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

#if 0
static const zb_ieee_addr_t g_ieee_addr_zed = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

static const zb_uint8_t g_key_c[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

#if 0
static zb_uint8_t aasscb_flag = 0;
#endif

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr);
  //zb_aps_set_global_tc_key_type(ZB_FALSE);

  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_bdb_set_legacy_device_support(ZB_TRUE);

  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  zb_set_max_children(3);

#if 0
  ZB_AIB().enable_alldoors_key = ZB_TRUE;
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr_c,
                             (zb_uint8_t*) g_key_c,
                             ZB_SECUR_GLOBAL_KEY,
                             ZB_SECUR_VERIFIED_KEY,
                             ZB_SECUR_KEY_SRC_UNKNOWN);
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


#if 0 /* for debug */
static void send_data_cb(zb_uint8_t param)
{
  if (!param)
  {
    TRACE_MSG(TRACE_INFO1, "###send_data_cb: status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_INFO1, "###send_data_cb: status FAILED", (FMT__0));
  }
}


static void send_test_request(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_buffer_test_req_param_t *req_param;
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APS1, "###send_data to 0xFFFF", (FMT__0));
  if (buf)
  {
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = 0xABCD;
    zb_tp_buffer_test_request(buf, send_data_cb);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "###send_data: unable to get data buffer", (FMT__0));
  }
}


#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  zb_uint16_t res = (zb_uint16_t)~0;
  TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));
  if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_zed) && !aasscb_flag)
  {
    res = 0xABCD;
    ZB_SCHEDULE_ALARM(send_test_request, 0, 35*ZB_TIME_ONE_SECOND);
    ++aasscb_flag;
  }
  TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
  return res;
}
#endif
#endif /* 0 - debug mode */


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

#if 0 /* for debug */
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
        zb_nwk_set_address_assignment_cb(addr_ass_cb);
#endif
#endif
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

/*! @} */
