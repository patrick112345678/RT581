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
/* PURPOSE: Test for ZC application written using ZDO.
*/

#define ZB_TEST_NAME TP_R20_BV_12_ZC
#define ZB_TRACE_FILE_ID 40859

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

//#define TEST_CHANNEL (1l << 24)

/* IEEE address */
static const zb_ieee_addr_t g_ieee_addr      = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

/* Child device */
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
#if 0
static const zb_ieee_addr_t g_ieee_child     = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#endif
#endif /* ZB_PRO_ADDRESS_ASSIGNMENT_CB */

/* apsUseExtendedPANID */
static const zb_ieee_addr_t g_aps_ext_pan_id = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const zb_uint8_t g_nwk_key[16]        = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

static const zb_ieee_addr_t g_ieee_addr1     = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const zb_uint8_t g_key1[16]           = {0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39};
static const zb_ieee_addr_t g_ieee_addr2     = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const zb_uint8_t g_key2[16]           = {0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C, 0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39};

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_pan_id(0x1aaa);
  zb_set_use_extended_pan_id(g_aps_ext_pan_id);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);
  zb_cert_test_set_security_level(0x05);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);


  zb_set_max_children(1);

  zb_set_nvram_erase_at_start(ZB_TRUE);
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    ZB_CERT_HACKS().use_preconfigured_aps_link_key = 1U;

    zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr1,
                             (zb_uint8_t*) g_key1,
                             ZB_SECUR_GLOBAL_KEY,
                             ZB_SECUR_VERIFIED_KEY,
                             ZB_SECUR_KEY_SRC_UNKNOWN);
    zb_secur_update_key_pair((zb_uint8_t*) g_ieee_addr2,
                             (zb_uint8_t*) g_key2,
                             ZB_SECUR_GLOBAL_KEY,
                             ZB_SECUR_VERIFIED_KEY,
                             ZB_SECUR_KEY_SRC_UNKNOWN);
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
#if 0
static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  zb_uint16_t res = (zb_uint16_t)~0;

  TRACE_MSG(TRACE_APS3, "###addr_assignmnet_cb", (FMT__0));
  if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_child))
  {
    res = 0xABCD;
  }
  TRACE_MSG(TRACE_APS3, "###addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
  return res;
}
#endif
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

#if 0 //def ZB_PRO_ADDRESS_ASSIGNMENT_CB
        zb_nwk_set_address_assignment_cb(addr_ass_cb);
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
