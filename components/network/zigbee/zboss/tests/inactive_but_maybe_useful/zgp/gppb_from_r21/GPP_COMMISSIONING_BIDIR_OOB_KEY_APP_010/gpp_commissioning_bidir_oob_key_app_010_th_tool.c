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

#define ZB_TEST_NAME GPP_COMMISSIONING_BIDIR_OOB_KEY_APP_010_TH_TOOL
#define ZB_TRACE_FILE_ID 41336

#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT 10

#define ZGPD_SEQ_NUM_CAP 1
#define ZGPD_RX_ON_CAP 0
#define ZGPD_FIX_LOC 0
#define ZGPD_USE_ASSIGNED_ALIAS 0
#define ZGPD_USE_SECURITY 0

static zb_ieee_addr_t g_th_gps_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;
static zb_uint16_t g_dut_addr = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;

static void dev_annce_cb(zb_zdo_device_annce_t *da);

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_WAIT_FOR_DUT_STARTUP,
  TEST_STATE_READY_TO_COMMISSIONING,
  TEST_STATE_READ_OUT_DUT_GPP_PROXY_TABLE,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 3000)


static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(cb);

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_OUT_DUT_GPP_PROXY_TABLE:
      zgp_cluster_read_attr(buf_ref, g_dut_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                            ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
      ZB_ZGPC_SET_PAUSE(5);
      break;
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

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
  TEST_DEVICE_CTX.test_state++;
  TRACE_MSG(TRACE_APP1, "perform_next_state: state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_WAIT_FOR_DUT_STARTUP:
      /* Transition to next state should be performed in dev_annce_cb */
      break;
    case TEST_STATE_READY_TO_COMMISSIONING:
      /* Transition to next states should be performed in custom_comm_cb */
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
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}


//==============================================================================
static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t dut_ieee = DUT_GPP_IEEE_ADDR;
  TRACE_MSG(TRACE_APP1, "dev_annce_cb: ieee = " TRACE_FORMAT_64 " NWK = 0x%x",
            (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
  TRACE_MSG(TRACE_APP1, "dev_annce_cb: state = %d", (FMT__D, TEST_DEVICE_CTX.test_state));

  if (ZB_IEEE_ADDR_CMP(dut_ieee, da->ieee_addr) == ZB_TRUE)
  {
    g_dut_addr = da->nwk_addr;
    if (TEST_DEVICE_CTX.test_state == TEST_STATE_WAIT_FOR_DUT_STARTUP)
    {
      ZB_SCHEDULE_ALARM(perform_next_state, 0, 10 * ZB_TIME_ONE_SECOND);
    }
  }
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  zb_bool_t ret_val = ZB_FALSE;
  ZVUNUSED(zgpd_id);

  TRACE_MSG(TRACE_APP1, "custom_comm_cb: state = %d, result = %d",
            (FMT__D_D, TEST_DEVICE_CTX.test_state, result));

  switch (result)
  {
    case ZB_ZGP_COMMISSIONING_COMPLETED:
      if (TEST_DEVICE_CTX.test_state == TEST_STATE_READY_TO_COMMISSIONING)
      {
#ifdef ZB_USE_BUTTONS
        zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
        ret_val = ZB_TRUE;
      }
      break;
    default:
      break;
  }

  if (ret_val == ZB_TRUE)
  {
    ZB_ZGPC_SET_PAUSE(10);
  }

  return ret_val;
}


static void zgpc_custom_startup()
{
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_tool");

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gps_addr);
  ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

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

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;
  ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZGP_PROXY_COMM_MODE_BROADCAST);
  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;
  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                             ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                             ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);
  ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

  zb_zdo_register_device_annce_cb(dev_annce_cb);
  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
