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
/* PURPOSE: TP/R21/BV-02 - Update Device upon Network Leave, gZC
*/

#define ZB_TEST_NAME TP_R21_BV_02_GZC
#define ZB_TRACE_FILE_ID 40539
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#define TEST_REJOIN_FLAG ((zb_uint8_t) 0x80)

static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;
static const zb_ieee_addr_t g_ieee_addr_gzr = IEEE_ADDR_gZR;
static const zb_ieee_addr_t g_ieee_addr_gzed = IEEE_ADDR_gZED;

/* Key for gZC */
static zb_uint8_t g_gzc_key[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

static void test_send_mgmt_leave(zb_uint8_t options);
static void send_leave_cmd(zb_uint8_t param, zb_uint16_t options);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_gzc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_gzc);

  zb_set_extended_pan_id(g_ext_pan_id);
  zb_set_use_extended_pan_id(g_ext_pan_id);

  zb_aib_set_trust_center_address(g_addr_tc);

  zb_set_pan_id(TEST_PAN_ID);

  /* let's always be coordinator */
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  zb_set_max_children(2);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  /* [zb_secur_setup_preconfigured_key] */
  zb_secur_setup_nwk_key(g_gzc_key, 0);

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

        ZB_SCHEDULE_ALARM(test_send_mgmt_leave, TEST_REJOIN_FLAG + 0, TEST_SEND_MGMT_LEAVE1_TO_ZED);
        ZB_SCHEDULE_ALARM(test_send_mgmt_leave, 1,                    TEST_SEND_MGMT_LEAVE2_TO_ZED);
        ZB_SCHEDULE_ALARM(test_send_mgmt_leave, TEST_REJOIN_FLAG + 2, TEST_SEND_MGMT_LEAVE1_TO_ZR);
        ZB_SCHEDULE_ALARM(test_send_mgmt_leave, 3,                    TEST_SEND_MGMT_LEAVE2_TO_ZR);
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


static void test_send_mgmt_leave(zb_uint8_t options)
{
  zb_uint16_t ext_opt = 0x0000 | options;
  /* Meaning of index:
   * 0 - leave to gZED with rejoin
   * 1 - leave without rejoin for gZED
   * 2 - leave with rejoin to DUT ZR
   * 3 - leave without rejoin to DUT ZR
   */
  TRACE_MSG(TRACE_APS1, "test_send_mgmt_leave options %d", (FMT__D, options));
  zb_buf_get_out_delayed_ext(send_leave_cmd, ext_opt, 0);
}

static void send_leave_cmd(zb_uint8_t param, zb_uint16_t options)
{
  zb_zdo_mgmt_leave_param_t *req = NULL;
  zb_ieee_addr_t dest_ieee_addr;
  zb_ret_t ret;
  zb_bool_t rejoin_flag = ZB_FALSE;
  zb_address_ieee_ref_t ieee_ref;
  zb_uint16_t short_addr;

  ZB_64BIT_ADDR_ZERO(dest_ieee_addr);

  /* ZC should not accept any rejoin from this time. */
  zb_cert_test_nib_set_disable_rejoin(ZB_TRUE);

  switch(options & (~TEST_REJOIN_FLAG))
  {
    case 0:
      ZB_MEMCPY(dest_ieee_addr, g_ieee_addr_gzed, sizeof(zb_ieee_addr_t));
      break;
    case 1:
      ZB_MEMCPY(dest_ieee_addr, g_ieee_addr_gzed, sizeof(zb_ieee_addr_t));
      break;
    case 2:
      ZB_MEMCPY(dest_ieee_addr, g_ieee_addr_gzr, sizeof(zb_ieee_addr_t));
      break;
    case 3:
      ZB_MEMCPY(dest_ieee_addr, g_ieee_addr_gzr, sizeof(zb_ieee_addr_t));
      break;
    default:
        TRACE_MSG(TRACE_ERROR, "Invalid leave", (FMT__0));
  }

  rejoin_flag = !!(options & TEST_REJOIN_FLAG);

  ret = zb_address_by_ieee(dest_ieee_addr, ZB_FALSE, ZB_FALSE, &ieee_ref);
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_APS1, "Leave (buffer = %d) was sent out", (FMT__D, param));

    req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_leave_param_t);
    req->remove_children = ZB_FALSE;
    req->rejoin = rejoin_flag;
    ZB_MEMCPY(req->device_address, dest_ieee_addr, sizeof(zb_ieee_addr_t));

    zb_address_short_by_ref((zb_uint16_t*) &short_addr, ieee_ref);
    req->dst_addr = short_addr;

    zdo_mgmt_leave_req(param, NULL);

    TRACE_MSG(TRACE_APS1, "Leave (buffer = %hd, dest = 0x%x) was sent out, rejoin = %d",
              (FMT__H_H_D, param, short_addr, rejoin_flag));
  }
  else
  {
    zb_buf_free(param);
    TRACE_MSG(TRACE_ERROR, "Error: can not sent buffer %d - unknown destination, ret code = %d.",
              (FMT__D_D, param, ret));
  }
}


/*! @} */

