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
/* PURPOSE: TH gps
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_GENERAL_TH_GPS
#define ZB_TRACE_FILE_ID 41280

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

#define ENDPOINT 10

static zb_ieee_addr_t g_th_gps_addr = TH_GPS_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint8_t g_shared_key[] = TEST_SEC_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READY_TO_COMMISSIONING1,
  TEST_STATE_READ_GPP_PROXY_TABLE1,
  TEST_STATE_READ_GPP_PROXY_TABLE2,
  TEST_STATE_READ_GPP_PROXY_TABLE3,
  TEST_STATE_READY_TO_COMMISSIONING2,
  TEST_STATE_READ_GPP_PROXY_TABLE4,
  TEST_STATE_READY_TO_COMMISSIONING3,
  TEST_STATE_READ_GPP_PROXY_TABLE5,
  TEST_STATE_READY_TO_COMMISSIONING4,
#ifndef ZB_NSNG
  TEST_STATE_STOP_COMMISSIONING1,
#endif
  TEST_STATE_READ_GPP_PROXY_TABLE6,
  TEST_STATE_READY_TO_COMMISSIONING5,
#ifndef ZB_NSNG
  TEST_STATE_STOP_COMMISSIONING2,
#endif
  TEST_STATE_READ_GPP_PROXY_TABLE7,
  TEST_STATE_READY_TO_COMMISSIONING6,
#ifndef ZB_NSNG
  TEST_STATE_STOP_COMMISSIONING3,
#endif
  TEST_STATE_READ_GPP_PROXY_TABLE8,
  TEST_STATE_READY_TO_COMMISSIONING7,
  TEST_STATE_READ_GPP_PROXY_TABLE9,
#ifdef ZB_CERTIFICATION_HACKS
  TEST_STATE_READY_TO_COMMISSIONING8,
#endif
  TEST_STATE_READ_GPP_PROXY_TABLE10,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 0)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPP_PROXY_TABLE1:
    case TEST_STATE_READ_GPP_PROXY_TABLE3:
    case TEST_STATE_READ_GPP_PROXY_TABLE6:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPP_PROXY_TABLE10:
#ifdef ZB_CERTIFICATION_HACKS
      ZB_CERT_HACKS().gp_sink_replace_sec_lvl_on_pairing = 0;
#endif
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      break;
    case TEST_STATE_READ_GPP_PROXY_TABLE2:
    case TEST_STATE_READ_GPP_PROXY_TABLE4:
    case TEST_STATE_READ_GPP_PROXY_TABLE5:
    case TEST_STATE_READ_GPP_PROXY_TABLE7:
    case TEST_STATE_READ_GPP_PROXY_TABLE8:
    case TEST_STATE_READ_GPP_PROXY_TABLE9:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(1);
      break;
  };
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  ZVUNUSED(zgpd_id);

  if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
  {
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
    return ZB_TRUE;
  }
  else
  if (result == ZB_ZGP_COMMISSIONING_TIMED_OUT &&
      (TEST_DEVICE_CTX.test_state == TEST_STATE_READY_TO_COMMISSIONING4 ||
       TEST_DEVICE_CTX.test_state == TEST_STATE_READY_TO_COMMISSIONING5 ||
       TEST_DEVICE_CTX.test_state == TEST_STATE_READY_TO_COMMISSIONING6))
  {
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
    return ZB_TRUE;
  }
#ifdef ZB_USE_BUTTONS
  else
  if (result == ZB_ZGP_COMMISSIONING_CANCELLED_BY_USER &&
      (TEST_DEVICE_CTX.test_state == TEST_STATE_STOP_COMMISSIONING1 ||
       TEST_DEVICE_CTX.test_state == TEST_STATE_STOP_COMMISSIONING2 ||
       TEST_DEVICE_CTX.test_state == TEST_STATE_STOP_COMMISSIONING3))
  {
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
    return ZB_TRUE;
  }
#endif
  return ZB_FALSE;
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
    case TEST_STATE_READY_TO_COMMISSIONING1:
    case TEST_STATE_READY_TO_COMMISSIONING2:
    case TEST_STATE_READY_TO_COMMISSIONING3:
    case TEST_STATE_READY_TO_COMMISSIONING7:
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case TEST_STATE_READY_TO_COMMISSIONING4:
#ifndef ZB_NSNG
      zb_zgps_start_commissioning(0);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_PASSED;
#else
      zb_zgps_start_commissioning(5 * ZB_TIME_ONE_SECOND);
#endif
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
#ifndef ZB_NSNG
    case TEST_STATE_STOP_COMMISSIONING1:
    case TEST_STATE_STOP_COMMISSIONING2:
    case TEST_STATE_STOP_COMMISSIONING3:
      zb_zgps_stop_commissioning();
      break;
#endif
    case TEST_STATE_READY_TO_COMMISSIONING5:
    case TEST_STATE_READY_TO_COMMISSIONING6:
#ifndef ZB_NSNG
      zb_zgps_start_commissioning(0);
      TEST_DEVICE_CTX.skip_next_state = NEXT_STATE_PASSED;
#else
      zb_zgps_start_commissioning(1 * ZB_TIME_ONE_SECOND);
#endif
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
#ifdef ZB_CERTIFICATION_HACKS
    case TEST_STATE_READY_TO_COMMISSIONING8:
      ZB_CERT_HACKS().gp_sink_replace_sec_lvl_on_pairing = 1;
      ZB_CERT_HACKS().gp_sink_sec_lvl_on_pairing = ZB_ZGP_SEC_LEVEL_REDUCED;
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
      ZB_ZGPC_SET_PAUSE(2);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
#endif
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      switch (TEST_DEVICE_CTX.test_state)
      {
        case TEST_STATE_READ_GPP_PROXY_TABLE1:
          test_send_command(0);
          break;
        case TEST_STATE_READ_GPP_PROXY_TABLE2:
          ZB_SCHEDULE_ALARM(test_send_command, 0, 2*ZB_TIME_ONE_SECOND);
          break;
        default:
          ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
      };
    }
  }
}

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gps");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gps_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  zb_set_default_ed_descriptor_values();

  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST;
  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_shared_key, ZB_CCM_KEY_SIZE);
  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(TEST_KEY_TYPE);

  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
