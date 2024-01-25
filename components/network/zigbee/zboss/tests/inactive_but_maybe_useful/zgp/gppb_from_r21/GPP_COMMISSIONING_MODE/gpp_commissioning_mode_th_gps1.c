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
/* PURPOSE: TH GPS
*/

#define ZB_TEST_NAME GPP_COMMISSIONING_MODE_TH_GPS1
#define ZB_TRACE_FILE_ID 41473

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

#define ENDPOINT 10

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_gps_addr = TH_GPS1_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_GPP_PROXY_TABLE_1,
  TEST_STATE_READY_TO_COMMISSIONING_1,
  TEST_STATE_WAIT_FOR_DECOMMISSIONING_1,
  TEST_STATE_READ_GPP_PROXY_TABLE_2A,
  TEST_STATE_READY_TO_COMMISSIONING_2A,
  TEST_STATE_WAIT_FOR_DECOMMISSIONING_2A,
  TEST_STATE_READ_GPP_PROXY_TABLE_2B,
  TEST_STATE_READY_TO_COMMISSIONING_2B,
  TEST_STATE_WAIT_FOR_DECOMMISSIONING_2B,
  TEST_STATE_READ_GPP_PROXY_TABLE_3A,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3A,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3B,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3C,
  TEST_STATE_SEND_GP_PAIRING_3C,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3D,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3E,
  TEST_STATE_SEND_GP_PAIRING_3E,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3F,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3G,
  TEST_STATE_SEND_GP_PAIRING_3G,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_3H,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3I,
  TEST_STATE_SEND_GP_PAIRING_3I,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3J,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_3J,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3K,
  TEST_STATE_SEND_GP_PAIRING_3K,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3L,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 3000)

static void zgpc_send_pairing(zb_uint8_t param, zb_bool_t add_sink)
{
  zb_zgp_sink_tbl_ent_t ent;

  ZB_BZERO(&ent, sizeof(zb_zgp_sink_tbl_ent_t));

  ent.options = ZGP_TBL_SINK_FILL_OPTIONS(
    ZB_ZGP_APP_ID_0000,
    ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST,
    ZGPD_SEQ_NUM_CAP,
    ZGPD_RX_ON_CAP,
    ZGPD_FIX_LOC,
    0, /* ZGPD_USE_ASSIGNED_ALIAS */
    ZGPD_USE_SECURITY);

  ZB_MEMSET(ent.u.sink.sgrp, 0xff, sizeof(ent.u.sink.sgrp));
  ent.zgpd_id.src_id = TEST_ZGPD2_SRC_ID;
  ent.security_counter = TEST_MAC_DSN_VALUE - 1;
  ent.u.sink.device_id = ZB_ZGP_ON_OFF_SWITCH_DEV_ID;
  ent.u.sink.match_dev_tbl_idx = 0xff;
  ent.groupcast_radius = 0;

  if (add_sink)
  {
    zb_zgp_gp_pairing_send_req_t *req;

    ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, NULL);
    ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 1, 0, 1);
    zgp_cluster_send_gp_pairing(param);
  }
  else
  {
    zb_zgp_gp_pairing_send_req_t *req;

    ZB_ZGP_GP_PAIRING_SEND_REQ_CREATE(ZB_BUF_FROM_REF(param), req, &ent, NULL);
    ZB_ZGP_GP_PAIRING_MAKE_SEND_OPTIONS(req, 0, 0, 1);
    zgp_cluster_send_gp_pairing(param);
  }
}

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPP_PROXY_TABLE_1:
    case TEST_STATE_READ_GPP_PROXY_TABLE_2A:
    case TEST_STATE_READ_GPP_PROXY_TABLE_2B:
    case TEST_STATE_READ_GPP_PROXY_TABLE_3A:
      zgp_cluster_read_attr(buf_ref, DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
      ZB_ZGPC_SET_PAUSE(1);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3A:
      ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_BROADCAST);
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref, 0, 0, cb);
      ZB_ZGPC_SET_PAUSE(TEST_DEFAULT_COMM_WINDOW + 2);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3B:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION,
                                                          10, cb);
      ZB_ZGPC_SET_PAUSE(12);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3C:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_COMMISSIONING_WINDOW_EXPIRATION,
                                                          10, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3D:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS, 0, cb);
      ZB_ZGPC_SET_PAUSE(TEST_DEFAULT_COMM_WINDOW + 3);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3E:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS, 0, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3F:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT,
                                                          0, cb);
      ZB_ZGPC_SET_PAUSE(TEST_DEFAULT_COMM_WINDOW + 3);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3G:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_GP_PROXY_COMMISSIONING_MODE_EXIT,
                                                          0, cb);
      ZB_ZGPC_SET_PAUSE(2);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_3H:
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_3J:
      zgp_cluster_send_proxy_commissioning_mode_leave_req(buf_ref, cb);
      ZB_ZGPC_SET_PAUSE(4);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3I:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_CWE_OR_PS,
                                                          TEST_DEFAULT_COMM_WINDOW + 20, cb);
      ZB_ZGPC_SET_PAUSE(TEST_DEFAULT_COMM_WINDOW + 2);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3J:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ON_CWE_OR_PCM,
                                                          15, cb);
      ZB_ZGPC_SET_PAUSE(5);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3K:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ALL,
                                                          TEST_DEFAULT_COMM_WINDOW + 20, cb);
      ZB_ZGPC_SET_PAUSE(1);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_ENTER_3L:
      zgp_cluster_send_proxy_commissioning_mode_enter_req(buf_ref,
                                                          ZGP_COMMISSIONING_EXIT_MODE_ALL,
                                                          15, cb);
      break;
    case TEST_STATE_SEND_GP_PAIRING_3C:
      zgpc_send_pairing(buf_ref, ZB_TRUE);
      ZB_ZGPC_SET_PAUSE(10);
      break;
    case TEST_STATE_SEND_GP_PAIRING_3E:
      zgpc_send_pairing(buf_ref, ZB_FALSE);
      ZB_ZGPC_SET_PAUSE(4);
      break;
    case TEST_STATE_SEND_GP_PAIRING_3G:
      zgpc_send_pairing(buf_ref, ZB_TRUE);
      ZB_ZGPC_SET_PAUSE(4);
      break;
    case TEST_STATE_SEND_GP_PAIRING_3I:
      zgpc_send_pairing(buf_ref, ZB_FALSE);
      ZB_ZGPC_SET_PAUSE(4);
      break;
    case TEST_STATE_SEND_GP_PAIRING_3K:
      zgpc_send_pairing(buf_ref, ZB_TRUE);
      ZB_ZGPC_SET_PAUSE(4);
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
  if (result == ZB_ZGP_ZGPD_DECOMMISSIONED)
  {
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_DECOMMISSIONING_1 ||
        TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_DECOMMISSIONING_2A ||
        TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_DECOMMISSIONING_2B)
    {
      return ZB_TRUE;
    }
  }
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
    case TEST_STATE_READY_TO_COMMISSIONING_1:
      ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_BROADCAST);
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case TEST_STATE_READY_TO_COMMISSIONING_2A:
    case TEST_STATE_READY_TO_COMMISSIONING_2B:
      ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_UNICAST);
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case TEST_STATE_WAIT_FOR_DECOMMISSIONING_1:
    case TEST_STATE_WAIT_FOR_DECOMMISSIONING_2A:
    case TEST_STATE_WAIT_FOR_DECOMMISSIONING_2B:
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
      ZB_SCHEDULE_ALARM(test_send_command, 0, (zb_time_t)(0.1f * ZB_TIME_ONE_SECOND));
    }
  }
}

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gps1");

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

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_NO_SECURITY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NWK);
  ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_key_nwk, ZB_CCM_KEY_SIZE);

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
