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

#define ZB_TEST_NAME GPS_ATTRS_RW_DUT_GPS
#define ZB_TRACE_FILE_ID 41312

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#define ENDPOINT 10

static zb_ieee_addr_t g_zc_addr = DUT_GPS_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READY_TO_COMMISSIONING1,
  TEST_STATE_READY_TO_COMMISSIONING2,
  TEST_STATE_READY_TO_COMMISSIONING3,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 28000)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(buf_ref);
  ZVUNUSED(cb);
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  ZVUNUSED(zgpd_id);

  if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
  {
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
    TRACE_MSG(TRACE_APP1, "comm_cb, state: %hd", (FMT__H, TEST_DEVICE_CTX.test_state));
    if (TEST_DEVICE_CTX.test_state != TEST_STATE_READY_TO_COMMISSIONING1)
    {
      return ZB_TRUE;
    }
  }
  return ZB_FALSE;
}

static void gp_sink_table_request_handler(zb_uint8_t buf_ref)
{
  ZVUNUSED(buf_ref);

  if (TEST_DEVICE_CTX.test_state == TEST_STATE_READY_TO_COMMISSIONING1)
  {
    PERFORM_NEXT_STATE(0);
  }
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
    case TEST_STATE_READY_TO_COMMISSIONING2:
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
      ZB_ZGPC_SET_PAUSE(3);
#ifdef ZB_USE_BUTTONS
      zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case TEST_STATE_READY_TO_COMMISSIONING1:
    case TEST_STATE_READY_TO_COMMISSIONING3:
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
      zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
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
  ZB_INIT("dut_gps");

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  /* Need to block GPDF recv if want to work thu the Proxy */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
  ZGP_GP_SHARED_SECURITY_KEY_TYPE = ZB_ZGP_SEC_KEY_TYPE_NWK;

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
  TEST_DEVICE_CTX.gp_sink_tbl_req_cb = gp_sink_table_request_handler;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
