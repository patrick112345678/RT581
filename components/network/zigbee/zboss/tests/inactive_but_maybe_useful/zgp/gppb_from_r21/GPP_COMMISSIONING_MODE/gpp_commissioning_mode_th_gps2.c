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

#define ZB_TEST_NAME GPP_COMMISSIONING_MODE_TH_GPS2
#define ZB_TRACE_FILE_ID 41472

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_gps_addr = TH_GPS2_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_1,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_1,
  TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2A,
  TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2B,
  TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_2B,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 0)

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_1:
      ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_BROADCAST);
      zgp_cluster_send_proxy_commissioning_mode_leave_req(buf_ref, cb);
      ZB_ZGPC_SET_PAUSE(1);
      break;
    case TEST_STATE_SEND_GP_PROXY_COMM_MODE_EXIT_2B:
      ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_UNICAST);
      zgp_cluster_send_proxy_commissioning_mode_leave_req(buf_ref, cb);
      ZB_ZGPC_SET_PAUSE(1);
      break;
  };
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
    case TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_1:
    case TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2A:
    case TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2B:
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

static void gp_prx_comm_mode_handler_cb(zb_uint8_t param)
{
  zb_buf_t      *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t    *ptr = ZB_BUF_BEGIN(buf);
  zb_uint8_t     options;

  ZB_ZCL_PACKET_GET_DATA8(&options, ptr);

  if (ZB_ZGP_COMM_MODE_OPT_GET_ACTION(options) == ZGP_PROXY_COMM_MODE_ENTER)
  {
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_1 ||
        TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2A ||
        TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_PROXY_COMM_MODE_ENTER_2B)
    {
      ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_gps2");

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

  TEST_DEVICE_CTX.gp_prx_comm_mode_hndlr_cb = gp_prx_comm_mode_handler_cb;
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
