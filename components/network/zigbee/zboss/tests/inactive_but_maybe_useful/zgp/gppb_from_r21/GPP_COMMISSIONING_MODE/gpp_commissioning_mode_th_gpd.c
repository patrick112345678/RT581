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
/* PURPOSE: Simple ZGPD for send GPDF as described in 5.3.1.2 test specification.
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_MODE_TH_GPD
#define ZB_TRACE_FILE_ID 41474

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t  g_zgpd_addr = TH_GPD_IEEE_ADDR;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_SET_IEEE_FOR_NSNG_NODE_POSITION,
  TEST_STATE_COMMISSIONING_1,
  TEST_STATE_DECOMMISSIONING_1,
  TEST_STATE_COMMISSIONING_2A,
  TEST_STATE_DECOMMISSIONING_2A,
  TEST_STATE_COMMISSIONING_2B,
  TEST_STATE_DECOMMISSIONING_2B,
  TEST_STATE_COMMISSIONING_3A_1,
  TEST_STATE_COMMISSIONING_3A_2,
  TEST_STATE_COMMISSIONING_3B_1,
  TEST_STATE_COMMISSIONING_3B_2,
  TEST_STATE_COMMISSIONING_3C_1,
  TEST_STATE_COMMISSIONING_3D_1,
  TEST_STATE_COMMISSIONING_3D_2,
  TEST_STATE_COMMISSIONING_3E_1,
  TEST_STATE_COMMISSIONING_3F_1,
  TEST_STATE_COMMISSIONING_3F_2,
  TEST_STATE_COMMISSIONING_3G_1,
  TEST_STATE_COMMISSIONING_3H_1,
  TEST_STATE_COMMISSIONING_3I_1,
  TEST_STATE_COMMISSIONING_3I_2,
  TEST_STATE_COMMISSIONING_3I_3,
  TEST_STATE_COMMISSIONING_3J_1,
  TEST_STATE_COMMISSIONING_3J_2,
  TEST_STATE_COMMISSIONING_3K_1,
  TEST_STATE_COMMISSIONING_3L_1,
  TEST_STATE_COMMISSIONING_3L_2,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 5000)

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  ZVUNUSED(ptr);
}

static void setup_ieee_for_nsng_node_position_cb(zb_uint8_t param)
{
  zb_free_buf(ZB_BUF_FROM_REF(param));
  ZB_ZGPD_SET_SRC_ID(TEST_ZGPD_SRC_ID);
  perform_next_state(0);
}

static void setup_ieee_for_nsng_node_position(zb_uint8_t param)
{
  if (param == 0)
  {
    ZB_GET_OUT_BUF_DELAYED(setup_ieee_for_nsng_node_position);
    return;
  }
  ZB_IEEE_ADDR_COPY(&g_zgpd_ctx.id.addr.ieee_addr, &g_zgpd_addr);
  zgpd_set_ieee_and_call(param, setup_ieee_for_nsng_node_position_cb);
}

static void perform_next_state(zb_uint8_t param)
{
  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
    param = 0;
  }

  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }
  TEST_DEVICE_CTX.test_state++;

  TRACE_MSG(TRACE_APP1, "perform next state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SET_IEEE_FOR_NSNG_NODE_POSITION:
      setup_ieee_for_nsng_node_position(0);
      break;
    case TEST_STATE_COMMISSIONING_1:
    case TEST_STATE_COMMISSIONING_2A:
    case TEST_STATE_COMMISSIONING_2B:
    case TEST_STATE_COMMISSIONING_3A_2:
    case TEST_STATE_COMMISSIONING_3H_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
      break;
    case TEST_STATE_COMMISSIONING_3D_2:
    case TEST_STATE_COMMISSIONING_3F_2:
    case TEST_STATE_COMMISSIONING_3I_2:
    case TEST_STATE_COMMISSIONING_3I_3:
    case TEST_STATE_COMMISSIONING_3K_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(4);
      break;
    case TEST_STATE_COMMISSIONING_3J_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(5);
      break;
    case TEST_STATE_COMMISSIONING_3B_2:
    case TEST_STATE_COMMISSIONING_3E_1:
    case TEST_STATE_COMMISSIONING_3G_1:
    case TEST_STATE_COMMISSIONING_3J_2:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(6);
      break;
    case TEST_STATE_COMMISSIONING_3A_1:
    case TEST_STATE_COMMISSIONING_3D_1:
    case TEST_STATE_COMMISSIONING_3F_1:
    case TEST_STATE_COMMISSIONING_3I_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(TEST_DEFAULT_COMM_WINDOW);
      break;
    case TEST_STATE_COMMISSIONING_3B_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(10);
      break;
    case TEST_STATE_COMMISSIONING_3L_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(15);
      break;
    case TEST_STATE_COMMISSIONING_3C_1:
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(8);
      break;
    case TEST_STATE_COMMISSIONING_3L_2:
      zb_zgpd_start_commissioning(comm_cb);
      break;
    case TEST_STATE_DECOMMISSIONING_1:
    case TEST_STATE_DECOMMISSIONING_2A:
    case TEST_STATE_DECOMMISSIONING_2B:
      zb_zgpd_decommission();
      ZB_SCHEDULE_ALARM(PERFORM_NEXT_STATE, 0, 3*ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  };
}

static void zgp_custom_startup()
{
#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpd");


  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, ZB_ZGPD_COMMISSIONING_UNIDIR, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(TEST_ZGPD_SRC_ID);
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_NO_SECURITY);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);

  ZGPD->channel = TEST_CHANNEL;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPD_TH_DECLARE_STARTUP_PROCESS()
