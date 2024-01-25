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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPP_PERSISTENT_STORAGE_TH_TOOL
#define ZB_TRACE_FILE_ID 41482

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

#define ENDPOINT 10

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_GPP_PROXY_TABLE1,
  TEST_STATE_READ_GPP_PROXY_TABLE2,
  TEST_STATE_READ_GPP_PROXY_TABLE3,
  TEST_STATE_READ_GPP_PROXY_TABLE4,
  TEST_STATE_READ_GPP_PROXY_TABLE5,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 1000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPP_PROXY_TABLE1:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_SKIP_AFTER_CMD;
      break;
    case TEST_STATE_READ_GPP_PROXY_TABLE2:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(3);
      break;
    case TEST_STATE_READ_GPP_PROXY_TABLE3:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(8);
      break;
    case TEST_STATE_READ_GPP_PROXY_TABLE4:
    case TEST_STATE_READ_GPP_PROXY_TABLE5:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(6);
      break;
  };
}

static void gp_pairing_handler_cb(zb_uint8_t buf_ref)
{
  ZVUNUSED(buf_ref);
  PERFORM_NEXT_STATE(0);
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

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_tool");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  zb_set_default_ed_descriptor_values();

  TEST_DEVICE_CTX.gp_pairing_hndlr_cb = gp_pairing_handler_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
