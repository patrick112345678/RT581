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
in 2.4.4.3 test specification.
*/

#define ZB_TEST_NAME GPD_COMMISSIONING_BIDIR_PANID_REQ_DUT_GPD
#define ZB_TRACE_FILE_ID 41402

#include "test_config.h"

#include "../include/zgp_test_templates.h"

static zb_uint32_t  g_zgpd_srcId = TEST_ZGPD_SRC_ID;
static zb_uint8_t   g_zgpd_key[] = TEST_SEC_KEY;
static zb_uint8_t   g_key_nwk[] = NWK_KEY;

enum
{
  TEST_STATE_INITIATE,
  TEST_STATE_COMMISSIONING,
  TEST_STATE_SEND_CMD_TOGGLE,
  TEST_STATE_FINISHED
};

ZB_ZGPD_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void make_gpdf(zb_buf_t *buf, zb_uint8_t **ptr)
{
  ZVUNUSED(buf);
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_CMD_TOGGLE:
      ZB_GPDF_PUT_UINT8(*ptr, ZB_GPDF_CMD_TOGGLE);
      break;
  };
}

ZB_ZGPD_DECLARE_COMMISSIONING_CALLBACK()

static void init_device(zb_uint8_t comm_type)
{
  ZB_ZGPD_INIT_ZGPD_CTX(ZB_ZGP_APP_ID_0000, comm_type, ZB_ZGP_ON_OFF_SWITCH_DEV_ID);

  ZB_ZGPD_SET_SRC_ID(g_zgpd_srcId);
  ZB_ZGPD_SET_SECURITY_LEVEL(ZB_ZGP_SEC_LEVEL_FULL_NO_ENC);
  ZB_ZGPD_SET_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_ZGPD_SET_SECURITY_KEY(g_key_nwk);
  ZB_ZGPD_SET_OOB_KEY(g_zgpd_key);

  ZGPD->channel = TEST_CHANNEL;

  ZGPD->security_frame_counter = 0xaac3;
}

static void perform_next_state(zb_uint8_t param)
{
  ZVUNUSED(param);
  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_COMMISSIONING:
      ZGPD->pan_id_request = 1;
      zb_zgpd_start_commissioning(comm_cb);
      ZB_ZGPD_SET_PAUSE(2);
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


  init_device(ZB_ZGPD_COMMISSIONING_BIDIR);
}

ZB_ZGPD_DECLARE_STARTUP_PROCESS()
