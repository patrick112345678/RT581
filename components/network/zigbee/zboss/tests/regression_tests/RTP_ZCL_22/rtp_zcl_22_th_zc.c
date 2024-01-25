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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_ZCL_22_TH_ZC

#define ZB_TEST_TAG_1 RTP_ZCL
#define ZB_TEST_TAG_2 RTP_WWAH

#define ZB_TRACE_FILE_ID 64917

#include "zboss_api.h"
#include "zb_secur_api.h"
#include "zb_common.h"
#include "device_th.h"
#include "rtp_zcl_22_common.h"
#include "../common/zb_reg_test_globals.h"
#include "se/zb_se_keep_alive.h"

#ifndef NCP_MODE_HOST

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
#error ZB_ZCL_SUPPORT_CLUSTER_WWAH is not compiled!
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error define ZB_STACK_REGRESSION_TESTING_API
#endif

/** [DECLARE_CLUSTERS] */

/*********************  Clusters' attributes  **************************/

/* Identify cluster attributes data */
static zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

/* Time cluster attributes data */
static zb_time_t g_attr_time_time = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_uint8_t g_attr_time_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
static zb_int32_t g_attr_time_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_dst_start = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_uint32_t g_attr_time_dst_end = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_int32_t g_attr_time_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_standard_time = ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_local_time = ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE;
static zb_time_t g_attr_time_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
static zb_time_t g_attr_time_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
static zb_uint8_t g_attr_keep_alive_base = 1;
static zb_uint16_t g_attr_keep_alive_jitter = 15;

/* Keep-Alive cluster attributes */
ZB_ZCL_DECLARE_KEEP_ALIVE_ATTR_LIST_FULL(rtp_zcl_22_th_zc_keep_alive_attr_list,
  &g_attr_keep_alive_base,
  &g_attr_keep_alive_jitter);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_22_th_zc_identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(rtp_zcl_22_th_zc_time_attr_list, &g_attr_time_time, &g_attr_time_time_status, &g_attr_time_time_zone, &g_attr_time_dst_start, &g_attr_time_dst_end, &g_attr_time_dst_shift, &g_attr_time_standard_time, &g_attr_time_local_time, &g_attr_time_last_set_time, &g_attr_time_valid_until_time);

/* Poll Control cluster attributes data */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST_CLIENT(rtp_zcl_22_th_zc_poll_control_attr_list);

/* OTA Upgrade server cluster attributes data */
static zb_uint8_t g_query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_MAX_VALUE;
static zb_uint32_t g_current_time = 0x12345678;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(rtp_zcl_22_th_zc_ota_upgrade_attr_list, &g_query_jitter, &g_current_time, 1);
ZB_ZCL_DECLARE_WWAH_CLIENT_ATTRIB_LIST(rtp_zcl_22_th_zc_wwah_client_attr_list);

/********************* Declare device **************************/
ZB_HA_DECLARE_WWAH_CLUSTER_LIST_ZC(rtp_zcl_22_th_zc_wwah_ha_clusters,
  rtp_zcl_22_th_zc_identify_attr_list,
  rtp_zcl_22_th_zc_time_attr_list,
  rtp_zcl_22_th_zc_keep_alive_attr_list,
  rtp_zcl_22_th_zc_poll_control_attr_list,
  rtp_zcl_22_th_zc_ota_upgrade_attr_list,
  rtp_zcl_22_th_zc_wwah_client_attr_list);
ZB_HA_DECLARE_WWAH_EP_ZC(rtp_zcl_22_th_zc_wwah_ha_ep, ZC_HA_EP, rtp_zcl_22_th_zc_wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(rtp_zcl_22_th_zc_device_ctx, rtp_zcl_22_th_zc_wwah_ha_ep);

static zb_uint8_t g_nwk_key_1[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_uint8_t g_nwk_key_2[16] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, \
                                     0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_uint8_t g_nwk_key_3[16] = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, \
                                     0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb};
static zb_uint8_t g_nwk_key_4[16] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, \
                                     0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc};
static zb_uint8_t g_nwk_key_5[16] = {0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, \
                                     0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd};
static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_DUT_ZC;

static zb_uint16_t g_dut_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
static zb_ieee_addr_t g_dut_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint16_t g_dut_ep = ZB_APS_BROADCAST_ENDPOINT_NUMBER;
static zb_bool_t g_is_transport_key_aps_encryption_was_enabled;

/*********************  Device-specific functions  **************************/

/**
 * Test 19
 */
typedef enum reg_test_step_e
{
  /* Initial configuration */
  REG_TEST_STEP_FIND_WWAH_SERVER,

  /* case a) read TCSecurityOnNtwkKeyRotationEnabled and verify that it is false */
  /* Should be NWK-encrypted with key #1 and APS-encrypted */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED,

  /* case b) broadcast NWK key #2 and switch-key command */
  /* "Transport Key" should be NWk-encrypted with key #1 */
  /* "Switch Key" should be NWk-encrypted with key #2 */
  REG_TEST_STEP_SWITCH_NWK_KEY_2,

  /* case c) verify DUT begins to use the new network key */
  /* Should be NWK-encrypted with key #2 and APS-encrypted */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_2,

  /* case d) send Enable TC Security On Network Key Rotation command to DUT */
  /* Should be NWK-encrypted with key #2 and APS-encrypted */
  REG_TEST_STEP_ENABLE_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION,

  /* case e) read TCSecurityOnNtwkKeyRotationEnabled and verify that it is true  */
  /* Should be NWK-encrypted with key #2 and APS-encrypted */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_3,

  /* case f) broadcast NWK key #3 and switch-key command */
  /* "Transport Key" should be NWk-encrypted with key #2 */
  /* "Switch Key" should be NWk-encrypted with key #3 */
  REG_TEST_STEP_SWITCH_NWK_KEY_3,

  /* case g) verify DUT does NOT begin to use the new network key
   * and continues to use the previous network key */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_4,
  REG_TEST_STEP_SET_USE_NETWORK_KEY_2_1,
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_5,

  /* case h) unicast NWK key #4 and broadcast switch-key command */
  /* "Transport Key" should be NWk-encrypted with key #2 and should not be APS-encrypted */
  /* "Switch Key" should be NWk-encrypted with key #4 */
  REG_TEST_STEP_SWITCH_NWK_KEY_4,

  /* case i) verify DUT does NOT begin to use the new network key
   * and continues to use the previous network key */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_6,
  REG_TEST_STEP_SET_USE_NETWORK_KEY_2_2,
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_7,

  /* case j) unicast NWK key #5 and broadcast switch-key command */
  /* "Transport Key" should be NWk-encrypted with key #2 and should be APS-encrypted */
  /* "Switch Key" should be NWk-encrypted with key #5 */
  REG_TEST_STEP_SWITCH_NWK_KEY_5,

  /* case k) verify DUT begins to use the new network key */
  REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_8,

  REG_TEST_STEPS_COUNT
} reg_test_step_t;

static void test_step_next(zb_uint8_t param);
static void test_step_next_extended(zb_uint8_t param, zb_uint16_t delay);
static void test_step_dispatch(zb_uint8_t param);

static void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID);

static reg_test_step_t g_test_step = REG_TEST_STEP_FIND_WWAH_SERVER;

MAIN()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | 0xffff);
  ZB_SET_TRACE_LEVEL(4);
  ARGV_UNUSED;

  ZB_INIT("zdo_th_zc");

  zb_set_long_address(g_ieee_addr_zc);

  zb_reg_test_set_common_channel_settings();
  zb_set_network_coordinator_role((1l << TEST_CHANNEL));

  zb_secur_setup_nwk_key(g_nwk_key_1, 0);
  zb_secur_setup_nwk_key(g_nwk_key_2, 1);
  zb_secur_setup_nwk_key(g_nwk_key_3, 2);
  zb_secur_setup_nwk_key(g_nwk_key_4, 3);
  zb_secur_setup_nwk_key(g_nwk_key_5, 4);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_22_th_zc_device_ctx);

  zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_CLIENT);

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
  zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(param);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler %hd sig %hd status %hd",
            (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));

  switch (sig)
  {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zboss_start_continue();
        break;

    case ZB_SIGNAL_DEVICE_FIRST_START:
    case ZB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

      if (status == 0)
      {
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        g_dut_short_addr = dev_annce_params->device_short_addr;
        ZB_IEEE_ADDR_COPY(g_dut_ieee_addr, dev_annce_params->ieee_addr);

        test_step_register(test_step_dispatch, 0, RTP_ZCL_22_STEP_TIME_ZC);
        test_control_start(TEST_MODE, RTP_ZCL_22_STEP_1_DELAY_ZC);
      }
      break;

    case ZB_BDB_SIGNAL_STEERING:
      TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
      break; /* ZB_BDB_SIGNAL_STEERING */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal %d, status %d", (FMT__D_D, sig, status));
      break;
    }

  if (param != ZB_BUF_INVALID)
  {
    zb_buf_free(param);
  }
}


static void test_step_send_wwah_match_desc_cb(zb_uint8_t param)
{
  zb_uint8_t *zdp_cmd = zb_buf_begin(param);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;

  TRACE_MSG(TRACE_APP1, ">> test_step_send_wwah_match_desc_cb, param %hd, status %d", (FMT__H_D, param, resp->status));

  ZB_ASSERT(resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len == 1);
  ZB_ASSERT(g_dut_short_addr == resp->nwk_addr);

  g_dut_ep = *((zb_uint8_t*)(resp + 1));

  ZB_SCHEDULE_APP_CALLBACK(test_step_next, param);

  TRACE_MSG(TRACE_APP1, "<< test_step_send_wwah_match_desc_cb", (FMT__0));
}


static void test_step_send_wwah_match_desc(zb_uint8_t param)
{
  zb_zdo_match_desc_param_t *req;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_step_send_wwah_match_desc);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_step_send_wwah_match_desc, param %hd", (FMT__H, param));

  req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

  req->nwk_addr = g_dut_short_addr;
  req->addr_of_interest = req->nwk_addr;
  req->profile_id = ZB_AF_HA_PROFILE_ID;
  req->num_in_clusters = 1;
  req->num_out_clusters = 0;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_WWAH;

  zb_zdo_match_desc_req(param, test_step_send_wwah_match_desc_cb);

  TRACE_MSG(TRACE_APP1, "<< test_step_send_wwah_match_desc", (FMT__0));
}


static void test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);
  send_read_attr(param, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_ID);

  if (g_test_step == REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_4 ||
      g_test_step == REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_6)
  {
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_22_KEY_CHECKING_STEP_TIME_ZC);
  }
  else
  {
    ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);
  }

  TRACE_MSG(TRACE_APP1, "<< test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled", (FMT__0));
}


static void reg_test_step_enable_wwah_tc_security_on_nwk_key_rotation(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> reg_test_step_enable_wwah_tc_security_on_nwk_key_rotation, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);
  ZB_ZCL_WWAH_SEND_ENABLE_TC_SECURITY_ON_NWK_KEY_ROTATION(param, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
    g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);

  ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);

  TRACE_MSG(TRACE_APP1, "<< reg_test_step_enable_wwah_tc_security_on_nwk_key_rotation", (FMT__0));
}

static void test_unicast_transport_next_key(zb_uint8_t param)
{
  zb_uint16_t *word_p;
  zb_apsme_transport_key_req_t *req;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_unicast_transport_next_key);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_unicast_transport_next_key, param %hd", (FMT__H, param));

  req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

  req->addr_mode = ZB_ADDR_64BIT_DEV;
  ZB_IEEE_ADDR_COPY(req->dest_address.addr_long, g_dut_ieee_addr);
  req->key.nwk.use_parent = 0; /* send key directly: we are parent! */
  req->key_type = ZB_STANDARD_NETWORK_KEY;
  req->key.nwk.key_seq_number = ZB_NIB().active_key_seq_number + 1;

  {
    zb_uint8_t *key = secur_nwk_key_by_seq(req->key.nwk.key_seq_number);

    if (!key)
    {
      TRACE_MSG(TRACE_ERROR, "No nwk key # %hd", (FMT__H, req->key.nwk.key_seq_number));
      zb_buf_free(param);
      return;
    }

    ZB_MEMCPY(req->key.nwk.key, key, ZB_CCM_KEY_SIZE);
  }

  ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param);

  TRACE_MSG(TRACE_APP1, "<< test_unicast_transport_next_key", (FMT__0));
}


static void test_broadcast_transport_next_key(zb_uint8_t param)
{
  zb_uint16_t *word_p;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_broadcast_transport_next_key);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_broadcast_transport_next_key, param %hd", (FMT__H, param));

  word_p = (zb_uint16_t*)  ZB_BUF_GET_PARAM(param, zb_uint16_t);
  *word_p = ZB_NWK_BROADCAST_ALL_DEVICES;

  zb_secur_send_nwk_key_update_br(param);

  TRACE_MSG(TRACE_APP1, "<< test_broadcast_transport_next_key", (FMT__0));
}


static void test_restore_transport_key_aps_encryption(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_broadcast_transport_next_key, param %hd", (FMT__H, param));

  ZB_ASSERT(param == ZB_BUF_INVALID);

  if (g_is_transport_key_aps_encryption_was_enabled)
  {
    zb_enable_transport_key_aps_encryption();
  }
  else
  {
    zb_disable_transport_key_aps_encryption();
  }

  TRACE_MSG(TRACE_APP1, "<< test_broadcast_transport_next_key", (FMT__0));
}


static void test_broadcast_switch_next_key(zb_uint8_t param)
{
  zb_uint16_t *word_p;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_broadcast_switch_next_key);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_broadcast_switch_next_key", (FMT__0));

  if (g_test_step == REG_TEST_STEP_SWITCH_NWK_KEY_4 ||
      g_test_step == REG_TEST_STEP_SWITCH_NWK_KEY_5)
  {
    test_restore_transport_key_aps_encryption(ZB_BUF_INVALID);
  }

  if (g_test_step == REG_TEST_STEP_SWITCH_NWK_KEY_4)
  {
    /* Switch back to third key after keys manipulations */
    secur_nwk_key_switch(2);
  }
  else if (g_test_step == REG_TEST_STEP_SWITCH_NWK_KEY_5)
  {
    /* Switch back to fourth key after keys manipulations */
    secur_nwk_key_switch(3);
  }

  word_p = (zb_uint16_t*)  ZB_BUF_GET_PARAM(param, zb_uint16_t);
  *word_p = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;

  zb_secur_send_nwk_key_switch(param);

  /* NOTE: the following code relies on stack's callbacks calling order,
   * so it is needed to be careful with its modification */

  /* Switch key manually here as the stack switches key after Switch Key command sending,
   * but the test requires to use a new key for the command */
  secur_nwk_key_switch(ZB_NIB().active_key_seq_number + 1);

  /* Prevent duplicated key switching as it was already done */
  ZG->zdo.handle.key_sw = 0;

  ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_22_SWITCH_KEY_STEP_TIME);

  TRACE_MSG(TRACE_APP1, "<< test_broadcast_switch_next_key", (FMT__0));
}


static void test_step_switch_nwk_key_2(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_switch_nwk_key_2, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);

  ZB_SCHEDULE_APP_CALLBACK(test_broadcast_transport_next_key, param);
  ZB_SCHEDULE_APP_ALARM(test_broadcast_switch_next_key, ZB_BUF_INVALID, RTP_ZCL_22_DELAY_BETWEEN_TRANSPORT_AND_SWITCH_KEY);

  TRACE_MSG(TRACE_APP1, "<< test_step_switch_nwk_key_2", (FMT__0));
}


static void test_step_switch_nwk_key_3(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_switch_nwk_key_3, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);

  ZB_SCHEDULE_APP_CALLBACK(test_broadcast_transport_next_key, param);
  ZB_SCHEDULE_APP_ALARM(test_broadcast_switch_next_key, ZB_BUF_INVALID, RTP_ZCL_22_DELAY_BETWEEN_TRANSPORT_AND_SWITCH_KEY);

  TRACE_MSG(TRACE_APP1, "<< test_step_switch_nwk_key_3", (FMT__0));
}


static void test_step_set_use_nwk_key(zb_uint8_t param, zb_uint16_t key_index)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_set_use_nwk_key, param %hd", (FMT__H, param));

  /* Keys are zero-indexed, but key_index parameter is one-indexed */
  secur_nwk_key_switch(key_index - 1);

  ZB_SCHEDULE_APP_CALLBACK(test_step_next, param);

  TRACE_MSG(TRACE_APP1, "<< test_step_set_use_nwk_key", (FMT__0));
}


static void test_step_switch_nwk_key_4(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_switch_nwk_key_4, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);

  /* Switch back to the third nwk key to send the fourth key,
   * replace third key with the second key */
  /* test_step_set_use_nwk_key(ZB_BUF_INVALID, 3); */

  g_is_transport_key_aps_encryption_was_enabled = zb_is_transport_key_aps_encryption_enabled();
  zb_disable_transport_key_aps_encryption();

  ZB_NIB().active_key_seq_number = 2;
  test_unicast_transport_next_key(param);
  ZB_NIB().active_key_seq_number = 1;

  ZB_SCHEDULE_APP_ALARM(test_broadcast_switch_next_key, ZB_BUF_INVALID, RTP_ZCL_22_DELAY_BETWEEN_TRANSPORT_AND_SWITCH_KEY);

  TRACE_MSG(TRACE_APP1, "<< test_step_switch_nwk_key_4", (FMT__0));
}


static void test_step_switch_nwk_key_5(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_switch_nwk_key_5, param %hd", (FMT__H, param));

  ZB_ASSERT(param != ZB_BUF_INVALID);

  /* Switch back to the third nwk key to send the fourth key,
   * replace third key with the second key */
  /* test_step_set_use_nwk_key(ZB_BUF_INVALID, 3); */

  g_is_transport_key_aps_encryption_was_enabled = zb_is_transport_key_aps_encryption_enabled();
  zb_enable_transport_key_aps_encryption();

  ZB_NIB().active_key_seq_number = 3;
  test_unicast_transport_next_key(param);
  ZB_NIB().active_key_seq_number = 1;

  ZB_SCHEDULE_APP_ALARM(test_broadcast_switch_next_key, ZB_BUF_INVALID, RTP_ZCL_22_DELAY_BETWEEN_TRANSPORT_AND_SWITCH_KEY);

  TRACE_MSG(TRACE_APP1, "<< test_step_switch_nwk_key_5", (FMT__0));
}


static void test_step_dispatch(zb_uint8_t param)
{
  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(test_step_dispatch);
    return;
  }

  TRACE_MSG(TRACE_APP1, ">> test_step_dispatch, param %hd, current_step %d", (FMT__H_D, param, g_test_step));

  switch (g_test_step)
  {
    case REG_TEST_STEP_FIND_WWAH_SERVER:
      ZB_SCHEDULE_APP_CALLBACK(test_step_send_wwah_match_desc, param);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SWITCH_NWK_KEY_2:
      ZB_SCHEDULE_APP_CALLBACK(test_step_switch_nwk_key_2, param);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_2:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_ENABLE_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION:
      ZB_SCHEDULE_APP_CALLBACK(reg_test_step_enable_wwah_tc_security_on_nwk_key_rotation, param);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_3:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SWITCH_NWK_KEY_3:
      ZB_SCHEDULE_APP_CALLBACK(test_step_switch_nwk_key_3, param);
      break;

    /* Check that the third key was not applied */
    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_4:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SET_USE_NETWORK_KEY_2_1:
      ZB_SCHEDULE_APP_CALLBACK2(test_step_set_use_nwk_key, param, 2);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_5:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SWITCH_NWK_KEY_4:
      ZB_SCHEDULE_APP_CALLBACK(test_step_switch_nwk_key_4, param);
      break;

    /* Check that the fourth key was not applied */
    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_6:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SET_USE_NETWORK_KEY_2_2:
      ZB_SCHEDULE_APP_CALLBACK2(test_step_set_use_nwk_key, param, 2);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_7:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    case REG_TEST_STEP_SWITCH_NWK_KEY_5:
      ZB_SCHEDULE_APP_CALLBACK(test_step_switch_nwk_key_5, param);
      break;

    case REG_TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_8:
      ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_tc_security_on_nwk_key_rotation_enabled, param);
      break;

    default:
      TRACE_MSG(TRACE_APP1, "test procedure is completed", (FMT__0));
      break;
  }

  TRACE_MSG(TRACE_APP1, "<< test_step_dispatch", (FMT__0));
}


static void test_step_next(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_dispatch", (FMT__0));

  g_test_step++;

  ZB_SCHEDULE_APP_ALARM(test_step_dispatch, param, RTP_ZCL_22_STEP_TIME_ZC);

  TRACE_MSG(TRACE_APP1, "<< test_step_dispatch", (FMT__0));
}


static void test_step_next_extended(zb_uint8_t param, zb_uint16_t delay)
{
  TRACE_MSG(TRACE_APP1, ">> test_step_next_extended", (FMT__0));

  g_test_step++;

  ZB_SCHEDULE_APP_ALARM(test_step_dispatch, param, delay);

  TRACE_MSG(TRACE_APP1, "<< test_step_next_extended", (FMT__0));
}


static void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
    (buffer), cmd_ptr, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, g_dut_ep, ZC_HA_EP,
    ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

#endif /* NCP_MODE_HOST */