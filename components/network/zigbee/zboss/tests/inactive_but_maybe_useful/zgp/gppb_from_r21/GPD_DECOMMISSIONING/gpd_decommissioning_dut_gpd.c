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
/* PURPOSE: Simple ZGPD for send GPDF as described
in 2.4.5 test specification.
*/

#define ZB_TEST_NAME GPD_DECOMMISSIONING_DUT_GPD
#define ZB_TRACE_FILE_ID 41524

#include "test_config.h"

#include "../include/zgp_test_templates.h"

static zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;
static zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;
static zb_uint8_t   g_key_nwk[] = NWK_KEY;

enum
{
  TEST_STATE_INITIATE,
  TEST_STATE_COMMISSIONING1,
  TEST_STATE_COMMISSIONING2,
  TEST_STATE_SEND_CMD_TOGGLE1,
  TEST_STATE_SEND_CMD_TOGGLE2,
  TEST_STATE_DECOMMISSIONING1,
  TEST_STATE_DECOMMISSIONING2,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_CMD_TOGGLE1:
      g_zgpd_ctx.id.endpoint = FIRST_ENDPOINT;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
    case TEST_STATE_SEND_CMD_TOGGLE2:
      g_zgpd_ctx.id.endpoint = SECOND_ENDPOINT;
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      ZB_ZGPD_SET_PAUSE(2);
      break;
  };
}

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void init_device(zb_uint8_t endpoint)
{
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0010, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zgpd_addr);
  ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
  g_zgpd_ctx.id.endpoint = endpoint;
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_zgpd_key);

  ZGPD->channel = TEST_CHANNEL;
}

static void perform_next_state(zb_uint8_t param)
{
  ZVUNUSED(param);

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
    case TEST_STATE_COMMISSIONING1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(4);
      break;
    case TEST_STATE_COMMISSIONING2:
      init_device(SECOND_ENDPOINT);
      ZGPD->security_frame_counter = 0xabc3;
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(1);
      break;
    case TEST_STATE_DECOMMISSIONING1:
      g_zgpd_ctx.id.endpoint = FIRST_ENDPOINT;
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 3*ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_DECOMMISSIONING2:
      g_zgpd_ctx.id.endpoint = SECOND_ENDPOINT;
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
  };
}

static void zgp_custom_startup()
{
  #if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("dut_gpd");


  init_device(FIRST_ENDPOINT);
  ZGPD->security_frame_counter = 0xaac3;
}

ZB_ZGPD_DECLARE_STARTUP_PROCESS()
