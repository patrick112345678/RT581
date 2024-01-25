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
/* PURPOSE: Simple GPPB for GP device
*/

#define ZB_TEST_NAME GPP_ALIAS_DERIVATION_NC_DUT_GPPB
#define ZB_TRACE_FILE_ID 41327

#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_PROXY

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#define ENDPOINT 10

static zb_ieee_addr_t g_zc_addr = DUT_GPPB_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

static zb_uint8_t g_shared_key[16] = TEST_SEC_KEY;


/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_OPEN_NETWORK,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(buf_ref);
  ZVUNUSED(cb);  
}

static void perform_next_state(zb_uint8_t param)
{
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }
  
  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_OPEN_NETWORK:
    {
      zb_bdb_set_legacy_device_support(1);
      bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
      ZB_SCHEDULE_CALLBACK(PERFORM_NEXT_STATE, 0);
      TRACE_MSG(TRACE_APP1, "START STEERING", (FMT__0));
      break;
    }
    case TEST_STATE_FINISHED:
    {
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    }

    default:
    {
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("dut_gppb");

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_shared_key, ZB_CCM_KEY_SIZE);

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(TEST_KEY_TYPE);

  ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()

#endif
