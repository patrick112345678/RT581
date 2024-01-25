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
/* PURPOSE: ZB Simple output device
*/

#define ZB_TRACE_FILE_ID 40238
#include "zboss_api.h"
#include "zb_wwah.h"
#include "zb_common.h"
#ifdef TEST_INTERPAN
/* Possible only in internal build */
#include "zb_aps_interpan.h"
#endif

#include "../nwk/nwk_internal.h"

#define PERMIT_JOINING_DURATION_TIME 240

enum test_step_e
{
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY1, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION, /** Send Enable APS Link Key Authorization command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY2, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY3, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION, /** Send Disable APS Link Key Authorization command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY4, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY5, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY6, /** Send APS Link Key Authorization Query command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_MGMT_LEAVE_WITHOUT_REJOIN_ENABLED, /** Read MGMTLeaveWithoutRejoinEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN1, /** Send Disable MGMT Leave Without Rejoin command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN2, /** Read MGMTLeaveWithoutRejoinEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN3, /** Send Leave Without Rejoin */
  TEST_STEP_READ_WWAH_NWK_RETRY_COUNT, /** Read NWKRetryCount attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_REQUEST_NEW_APS_LINK_KEY, /** Send Request New APS Link Key command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_MAC_RETRY_COUNT, /** Read MACRetryCount attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_ENABLE_WWAH_APP_EVENT_RETRY_ALGORITHM, /** Send Enable WWAH App Event Retry Algorithm command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_WWAH_APP_EVENT_RETRY_ALGORITHM, /** Send Disable WWAH App Event Retry Algorithm command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_ROUTER_CHECK_IN_ENABLED, /** Read RouterCheckInEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_TOUCHLINK_INTERPAN_ENABLED, /** Read TouchlinkInterpanEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_REQUEST_TIME1, /** Send Request Time command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_REQUEST_TIME2, /** Wait for Response */
  TEST_STEP_READ_WWAH_WWAH_PARENT_CLASSIFICATION_ENABLED, /** Read WWAHParentClassificationEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_ENABLE_WWAH_REJOIN_ALGORITHM, /** Send Enable WWAH Rejoin Algorithm command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_WWAH_APP_EVENT_RETRY_ENABLED, /** Read WWAHAppEventRetryEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_WWAH_REJOIN_ALGORITHM, /** Send Disable WWAH Rejoin Algorithm command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_WWAH_APP_EVENT_RETRY_QUEUE_SIZE, /** Read WWAHAppEventRetryQueueSize attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_SET_IAS_ZONE_ENROLLMENT_METHOD, /** Send Set IAS Zone Enrollment Method command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_WWAH_REJOIN_ENABLED, /** Read WWAHRejoinEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE1_ADD, /** Send Bind Request */
  TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE2_CHECK, /** Send Binding Table Request */
  TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE3_CLEAR, /** Send Clear Binding Table command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE4_CHECK, /** Send Binding Table Request */
  TEST_STEP_WWAH_SEND_ENABLE_PERIODIC_ROUTER_CHECK_INS, /** Send Enable Periodic Router Check Ins command to Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_PERIODIC_ROUTER_CHECK_INS, /** Send Enable Periodic Router Check Ins command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_CONFIGURATION_MODE, /** Send Disable Configuration Mode command to Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_CONFIGURATION_MODE, /** Check Configuration Disablement to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_CONFIGURATION_MODE_ENABLED, /** Read ConfigurationModeEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_ENABLE_CONFIGURATION_MODE, /** Send Enable Configuration Mode command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_PERIODIC_ROUTER_CHECK_INS, /** Send Disable Periodic Router Check Ins command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_CURRENT_DEBUG_REPORT_ID, /** Read CurrentDebugReportID attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DEBUG_REPORT_QUERY, /** Send Debug Report Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_SET_MAC_POLL_CCA_WAIT_TIME, /** Send Set MAC Poll CCA Wait Time command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_MAC_POLL_FAILURE_WAIT_TIME, /** Read MACPollFailureWaitTime attribute from Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_WWAH_BAD_PARENT_RECOVERY_ENABLED, /** Read WWAHBadParentRecoveryEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS, /** Send Require APS ACKs on Unicasts command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY1, /** Send APS ACK Requirement Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_REMOVE_APS_ACKS_ON_UNICASTS_REQUIREMENT, /** Send Remove APS ACKs on Unicasts Requirement command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY2, /** Send APS ACK Requirement Query command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_SET_PENDING_NETWORK_UPDATE, /** Send Set Pending Network Update command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_CHANNEL, /** Read PendingNetworkUpdateChannel attribute from Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_PANID, /** Read PendingNetworkUpdatePANID attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_PENDING_NETWORK_UPDATE1, /** Check Set Pending Network Update command to Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_PENDING_NETWORK_UPDATE2, /** Check Set Pending Network Update command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_OTA_MAX_OFFLINE_DURATION, /** Read OTAMaxOfflineDuration attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_SURVEY_BEACONS, /** Send Survey Beacons command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_DISABLE_OTA_DOWNGRADES, /** Read DisableOTADowngrades attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_OTA_DOWNGRADES1, /** Send Disable OTA Downgrades command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_OTA_DOWNGRADES2, /** Read DisableOTADowngrades attribute from Works with All Hubs cluster */

#ifdef TEST_INTERPAN
  TEST_STEP_WWAH_CHECK_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT1, /** Send Disable Touchlink
                                                                     * Interpan Message Support
  command to Works with All Hubs cluster */
#endif
  TEST_STEP_WWAH_SEND_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT, /** Send Disable Touchlink
                                                                   * Interpan Message Support
  command to Works with All Hubs cluster */
#ifdef TEST_INTERPAN
  TEST_STEP_WWAH_CHECK_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT2, /** Send Disable Touchlink
                                                                     * Interpan Message Support
  command to Works with All Hubs cluster */
#endif
  TEST_STEP_WWAH_SEND_ENABLE_WWAH_PARENT_CLASSIFICATION, /** Send Enable WWAH Parent Classification command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_WWAH_PARENT_CLASSIFICATION, /** Send Disable WWAH Parent Classification command to Works with All Hubs cluster */

  TEST_STEP_WWAH_SEND_ENABLE_WWAH_BAD_PARENT_RECOVERY, /** Send Enable WWAH Bad Parent Recovery command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_DISABLE_WWAH_BAD_PARENT_RECOVERY, /** Send Disable WWAH Bad Parent Recovery command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_USE_TRUST_CENTER_FOR_CLUSTER, /** Send Use Trust Center for Cluster command to Works with All Hubs cluster */
  TEST_STEP_WWAH_SEND_TRUST_CENTER_FOR_CLUSTER_SERVER_QUERY, /** Send Trust Center for Cluster Server Query command to Works with All Hubs cluster */

  TEST_STEP_WWAH_SEND_ENABLE_TC_SECURITY_ON_NWK_KEY_ROTATION, /** Send Enable TC Security On Nwk Key Rotation command to Works with All Hubs cluster */
  TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED, /** Read TCSecurityOnNwkKeyRotationEnabled attribute from Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_TC_SECURITY_ON_NWK_KEY_ROTATION1, /** Check Enable TC Security On Nwk Key Rotation command to Works with All Hubs cluster */
  TEST_STEP_WWAH_CHECK_TC_SECURITY_ON_NWK_KEY_ROTATION2, /** Check Enable TC Security On Nwk Key Rotation command to Works with All Hubs cluster */

  TEST_STEP_FINISHED    /**< Test finished pseudo-step. */
};


/** [DECLARE_CLUSTERS] */
/*********************  Clusters' attributes  **************************/

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
/* Time cluster attributes data */
/** @brief Minimum value for Time attribute */
#define ZB_ZCL_TIME_TIME_MIN_VALUE ((zb_time_t)0x0)

/** @brief Default value for StandardTime attribute */
#define ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE ((zb_uint32_t)0xFFFFFFFF)

/** @brief Default value for LocalTime attribute */
#define ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE ((zb_uint32_t)0xFFFFFFFF)

zb_time_t g_attr_time_time = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_uint8_t g_attr_time_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
zb_int32_t g_attr_time_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
zb_uint32_t g_attr_time_dst_start = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_uint32_t g_attr_time_dst_end = ZB_ZCL_TIME_TIME_MIN_VALUE;
zb_int32_t g_attr_time_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
zb_uint32_t g_attr_time_standard_time = ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE;
zb_uint32_t g_attr_time_local_time = ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE;
zb_time_t g_attr_time_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
zb_time_t g_attr_time_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_keep_alive_base = 1;
zb_uint16_t g_attr_keep_alive_jitter = 15;

/* Keep-Alive cluster attributes */
/* TODO: Move out from ZB_ENABLE_SE */
ZB_ZCL_DECLARE_KEEP_ALIVE_ATTR_LIST_FULL(keep_alive_attr_list, &g_attr_keep_alive_base, &g_attr_keep_alive_jitter);
/*
#else
zb_zcl_attr_t keep_alive_attr_list [] = {0};
#define ZB_ZCL_CLUSTER_ID_KEEP_ALIVE_SERVER_ROLE_INIT NULL
#define ZB_ZCL_CLUSTER_ID_KEEP_ALIVE_CLIENT_ROLE_INIT NULL
#endif*/
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(time_attr_list, &g_attr_time_time, &g_attr_time_time_status, &g_attr_time_time_zone, &g_attr_time_dst_start, &g_attr_time_dst_end, &g_attr_time_dst_shift, &g_attr_time_standard_time, &g_attr_time_local_time, &g_attr_time_last_set_time, &g_attr_time_valid_until_time);

/* Poll Control cluster attributes data */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST_CLIENT(poll_control_attr_list);

/* OTA Upgrade server cluster attributes data */
zb_uint8_t query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_MAX_VALUE;
zb_uint32_t current_time = 0x12345678;//OTA_UPGRADE_TEST_CURRENT_TIME;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(ota_upgrade_attr_list, &query_jitter, &current_time, 1);
ZB_ZCL_DECLARE_WWAH_CLIENT_ATTRIB_LIST(wwah_client_attr_list);
/********************* Declare device **************************/
#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
void zb_zcl_wwah_init_client();
#endif

ZB_HA_DECLARE_WWAH_CLUSTER_LIST_ZC(wwah_ha_clusters, identify_attr_list, time_attr_list, keep_alive_attr_list, poll_control_attr_list, ota_upgrade_attr_list, wwah_client_attr_list);
ZB_HA_DECLARE_WWAH_EP_ZC(wwah_ha_ep, ZC_HA_EP, wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(device_ctx, wwah_ha_ep);
/*********************  Device-specific functions  **************************/
zb_uint16_t g_wwah_srv_addr = 0;
zb_uint16_t g_wwah_srv_ep = 0;
#define DST_ADDR g_wwah_srv_addr
#define DST_ENDPOINT g_wwah_srv_ep
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
zb_uint32_t g_test_step = TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY1;

zb_uint8_t g_error_cnt = 0;

/*********************  Device-specific functions  **************************/

#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
void zb_zcl_wwah_init_client()
{
}
#endif

void test_next_step(zb_uint8_t param);
void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP,
       ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

void send_write_attr(zb_bufid_t buffer,zb_uint16_t clusterID, zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal)
{
  zb_uint8_t *cmd_ptr;
  ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, (attributeID), (attrType), (attrVal));
  ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ((buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE,
    DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

static void send_mgmt_bind_req(zb_uint8_t param, zb_uint16_t index)
{
  zb_bufid_t buf;
  zb_zdo_mgmt_bind_param_t *req_params;

  TRACE_MSG(TRACE_ZDO3, ">>send_mgmt_bind_req: buf_param = %i, start_at = %d",
            (FMT__D_D, param, index));

  buf = param;
  req_params = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_bind_param_t);
  req_params->start_index = index;
  req_params->dst_addr = DST_ADDR;
  zb_zdo_mgmt_bind_req(param, NULL);

  TRACE_MSG(TRACE_ZDO3, "<<send_mgmt_bind_req", (FMT__0));
}

static void test_bind_fsm(zb_uint8_t param)
{
  zb_zdo_bind_req_param_t *bind_req_param;
  zb_bufid_t buf;

  TRACE_MSG(TRACE_ZDO2, ">>test_bind_fsm: param = %hd", (FMT__H, param));

  buf = param;
  bind_req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);

  ZB_IEEE_ADDR_COPY(bind_req_param->src_address, g_ed_addr);
  ZB_IEEE_ADDR_COPY(bind_req_param->dst_address.addr_long, g_zc_addr);
  bind_req_param->src_endp = DST_ENDPOINT;
  bind_req_param->req_dst_addr = DST_ADDR;
  bind_req_param->cluster_id = 0x0000;
  bind_req_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
  bind_req_param->dst_endp = ZC_HA_EP;
  zb_zdo_bind_req(param, NULL);
  TRACE_MSG(TRACE_ZDO2, "<<test_bind_fsm", (FMT__0));
}

void mgmt_nwk_update(zb_uint8_t param, zb_uint32_t channel_mask, zb_bool_t is_broadcast)
{
  zb_bufid_t buf;
  zb_zdo_mgmt_nwk_update_req_t *req;

  ZVUNUSED(param);

  buf = zb_buf_get_out();
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
  }
  else
  {
    req = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_nwk_update_req_t);
    req->hdr.scan_channels = channel_mask;
    req->hdr.scan_duration = ZB_ZDO_NEW_ACTIVE_CHANNEL;
    req->scan_count = 1;
    req->dst_addr = is_broadcast ? ZB_NWK_BROADCAST_ALL_DEVICES : zb_address_short_by_ieee(g_ed_addr);
    zb_zdo_mgmt_nwk_update_req(buf, NULL);
  }
}

static void send_leave_cmd(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_mgmt_leave_param_t *req = NULL;
  zb_ret_t ret;
  zb_address_ieee_ref_t ieee_ref;

  ret = zb_address_by_ieee(g_ed_addr, ZB_FALSE, ZB_FALSE, &ieee_ref);
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_APS1, "Leave (buffer = %d) was sent out", (FMT__D, param));

    req = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
    req->remove_children = ZB_FALSE;
    req->rejoin = ZB_TRUE;
    ZB_MEMCPY(req->device_address, g_ed_addr, sizeof(zb_ieee_addr_t));
    zb_address_short_by_ref((zb_uint16_t*) &req->dst_addr, ieee_ref);
    zdo_mgmt_leave_req(param, NULL);
  }
  else
  {
    zb_buf_free(buf);
    TRACE_MSG(TRACE_ERROR, "Error: can not sent buffer %d - unknown destination, ret code = %d.",
              (FMT__D_D, param, ret));
  }
}

#ifdef TEST_INTERPAN
void send_interp_data(zb_uint8_t param)
{
  zb_uint8_t status;
  TRACE_MSG(TRACE_APS1, "> send_interp_data param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_RESET_TO_FN_REQ(param, g_ed2_addr, NULL, status);

  TRACE_MSG(TRACE_APS1, "< send_interp_data", (FMT__0));
}
#endif

MAIN()
{

  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("sample_zc");

  zb_set_long_address(g_zc_addr);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);

  zb_set_network_coordinator_role(DEV_CHANNEL_MASK);

#ifdef ZB_ZCL_ENABLE_WWAH_CLIENT
  zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_CLIENT);
  /* zb_zcl_set_cluster_encryption(ZC_HA_EP, ZB_ZCL_CLUSTER_ID_WWAH, 1); */
#endif

  /* zb_zcl_set_cluster_encryption(ZC_HA_EP, ZB_ZCL_CLUSTER_ID_TIME, 1); */

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

static void send_key_switch(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zb_secur_nwk_key_switch_procedure);
}

void button_press_handler(zb_uint8_t param)
{
  if (!param)
  {
    /* Button is pressed, get buffer for outgoing command */
    zb_buf_get_out_delayed(button_press_handler);
  }
  else
  {
    if (g_test_step == TEST_STEP_FINISHED /*|| g_error_cnt*/)
    {
      if (g_error_cnt)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR Test failed with %hd errors", (FMT__H, g_error_cnt));
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Test finished. Status: OK", (FMT__0));
      }
      zb_buf_free(param);
    }
    else
    {
#ifndef ZB_USE_BUTTONS
      /* Do not have buttons in simulator - just start periodic on/off sending */
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(400));
#endif
      test_next_step(param);
    }
  }
}
void test_next_step(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  TRACE_MSG(TRACE_APP3, "> test_next_step param %hd step %ld", (FMT__H_L, param, g_test_step));

  /* 2018-12-28: CR:MINOR This is our old-style test application coding - one big switch with
   * g_test_step. If it is preferrable by some specific reason (for instance, it is comfortable for
   * auto-generating etc) please specify it, but currently we are not using it in 99% of
   * cases. Usually it is ok when you are writing simple "request-response" test, but it becomes
   * EXTREMELY unclear when test steps are not so straightforward (skip response, retry sending,
   * switch to some step not in direct order, etc etc etc). */
  switch (g_test_step )
  {
  case TEST_STEP_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t cluster;
      TRACE_MSG(TRACE_APP1, "Send Enable APS Link Key Authorization command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION_START(buffer, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 3, cmd_ptr);
      cluster = ZB_ZCL_CLUSTER_ID_BASIC;
      ZB_ZCL_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      cluster = ZB_ZCL_CLUSTER_ID_GROUPS;
      ZB_ZCL_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      cluster = ZB_ZCL_CLUSTER_ID_SCENES;
      ZB_ZCL_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      ZB_ZCL_WWAH_SEND_ENABLE_APS_LINK_KEY_AUTHORIZATION_END(cmd_ptr, buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t cluster;
      TRACE_MSG(TRACE_APP1, "Send Disable APS Link Key Authorization command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION_START(buffer, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 3, cmd_ptr);
      cluster = ZB_ZCL_CLUSTER_ID_ON_OFF;
      ZB_ZCL_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      cluster = ZB_ZCL_CLUSTER_ID_BASIC;
      ZB_ZCL_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      cluster = ZB_ZCL_CLUSTER_ID_SCENES;
      ZB_ZCL_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION_ADD(cmd_ptr, &cluster);
      ZB_ZCL_WWAH_SEND_DISABLE_APS_LINK_KEY_AUTHORIZATION_END(cmd_ptr, buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_MGMT_LEAVE_WITHOUT_REJOIN_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read MGMTLeaveWithoutRejoinEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_MGMT_LEAVE_WITHOUT_REJOIN_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN1:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable MGMT Leave Without Rejoin command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN2:
    TRACE_MSG(TRACE_APP1, "Read MGMTLeaveWithoutRejoinEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_MGMT_LEAVE_WITHOUT_REJOIN_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_MGMT_LEAVE_WITHOUT_REJOIN3:
    TRACE_MSG(TRACE_APP1, "Sending Leave without rejoin", (FMT__0));
    g_test_step++;
    ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10500));
    send_leave_cmd(param);
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY1:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_IDENTIFY;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY2:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_BASIC;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY3:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_TIME;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY4:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_BASIC;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY5:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_TIME;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY6:
    {
      zb_uint16_t cluster = ZB_ZCL_CLUSTER_ID_ON_OFF;
      TRACE_MSG(TRACE_APP1, "Send APS Link Key Authorization Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_LINK_KEY_AUTHORIZATION_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &cluster);
    }
    break;
  case TEST_STEP_READ_WWAH_NWK_RETRY_COUNT:
    TRACE_MSG(TRACE_APP1, "Read NWKRetryCount attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_NWK_RETRY_COUNT_ID);
    break;
  case TEST_STEP_WWAH_SEND_REQUEST_NEW_APS_LINK_KEY:
    {
      TRACE_MSG(TRACE_APP1, "Send Request New APS Link Key command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10500));
      ZB_ZCL_WWAH_SEND_REQUEST_NEW_APS_LINK_KEY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_MAC_RETRY_COUNT:
    TRACE_MSG(TRACE_APP1, "Read MACRetryCount attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_MAC_RETRY_COUNT_ID);
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_WWAH_APP_EVENT_RETRY_ALGORITHM:
    {
      zb_uint32_t max_backoff_time_in_seconds = 86400;
      TRACE_MSG(TRACE_APP1, "Send Enable WWAH App Event Retry Algorithm command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_WWAH_APP_EVENT_RETRY_ALGORITHM(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, 1, 53, &max_backoff_time_in_seconds, 10);
    }
    break;
  case TEST_STEP_READ_WWAH_ROUTER_CHECK_IN_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read RouterCheckInEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_ROUTER_CHECK_IN_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_WWAH_APP_EVENT_RETRY_ALGORITHM:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable WWAH App Event Retry Algorithm command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_WWAH_APP_EVENT_RETRY_ALGORITHM(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_TOUCHLINK_INTERPAN_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read TouchlinkInterpanEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_TOUCHLINK_INTERPAN_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_REQUEST_TIME1:
    {
      TRACE_MSG(TRACE_APP1, "Send Request Time command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_REQUEST_TIME(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
    }
    break;
  case TEST_STEP_WWAH_SEND_REQUEST_TIME2:
    {
      TRACE_MSG(TRACE_APP1, "Wait for Response", (FMT__0));
      g_test_step++;
    }
    break;
  case TEST_STEP_READ_WWAH_WWAH_PARENT_CLASSIFICATION_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read WWAHParentClassificationEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_PARENT_CLASSIFICATION_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_WWAH_REJOIN_ALGORITHM:
    {
      zb_uint16_t fast_rejoin_timeout_in_seconds = 1000;
      zb_uint16_t duration_between_each_rejoin_in_seconds = 1500;
      zb_uint16_t fast_rejoin_first_backoff_in_seconds = 1100;
      zb_uint16_t max_backoff_time_in_seconds = 1700;
      zb_uint16_t max_backoff_iterations = 1000;
      TRACE_MSG(TRACE_APP1, "Send Enable WWAH Rejoin Algorithm command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_WWAH_REJOIN_ALGORITHM(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &fast_rejoin_timeout_in_seconds, &duration_between_each_rejoin_in_seconds, &fast_rejoin_first_backoff_in_seconds, &max_backoff_time_in_seconds, &max_backoff_iterations);
    }
    break;
  case TEST_STEP_READ_WWAH_WWAH_APP_EVENT_RETRY_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read WWAHAppEventRetryEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_APP_EVENT_RETRY_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_WWAH_REJOIN_ALGORITHM:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable WWAH Rejoin Algorithm command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_WWAH_REJOIN_ALGORITHM(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_WWAH_APP_EVENT_RETRY_QUEUE_SIZE:
    TRACE_MSG(TRACE_APP1, "Read WWAHAppEventRetryQueueSize attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_APP_EVENT_RETRY_QUEUE_SIZE_ID);
    break;
  case TEST_STEP_WWAH_SEND_SET_IAS_ZONE_ENROLLMENT_METHOD:
    {
      TRACE_MSG(TRACE_APP1, "Send Set IAS Zone Enrollment Method command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_SET_IAS_ZONE_ENROLLMENT_METHOD(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_WWAH_ENROLLMENT_MODE_AUTO_ENROLL_REQUEST + 5/*That is a error for test purpose*/);
    }
    break;
  case TEST_STEP_READ_WWAH_WWAH_REJOIN_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read WWAHRejoinEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_REJOIN_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE1_ADD:
    {
      TRACE_MSG(TRACE_APP1, "Send Bind Request", (FMT__0));
      g_test_step++;
      test_bind_fsm(param);
    }
    break;
  case TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE2_CHECK:
    {
      TRACE_MSG(TRACE_APP1, "Send Binding Table Request", (FMT__0));
      g_test_step++;
      send_mgmt_bind_req(param, 0);
    }
    break;
  case TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE3_CLEAR:
    {
      TRACE_MSG(TRACE_APP1, "Send Clear Binding Table command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_CLEAR_BINDING_TABLE(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_CLEAR_BINDING_TABLE4_CHECK:
    {
      TRACE_MSG(TRACE_APP1, "Send Binding Table Request", (FMT__0));
      g_test_step++;
      send_mgmt_bind_req(param, 0);
    }
    break;
  case TEST_STEP_READ_WWAH_MAC_POLL_FAILURE_WAIT_TIME:
    TRACE_MSG(TRACE_APP1, "Read MACPollFailureWaitTime attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_MAC_POLL_FAILURE_WAIT_TIME_ID);
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_PERIODIC_ROUTER_CHECK_INS:
    {
      zb_uint16_t check_in_interval = 20;
      TRACE_MSG(TRACE_APP1, "Send Enable Periodic Router Check Ins command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_PERIODIC_ROUTER_CHECK_INS(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &check_in_interval);
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
    }
    break;
  case TEST_STEP_WWAH_CHECK_PERIODIC_ROUTER_CHECK_INS:
    {
      zb_uint16_t check_in_interval = 5;
      g_test_step++;

      TRACE_MSG(TRACE_APP1, "Send Enable Periodic Router Check Ins command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_PERIODIC_ROUTER_CHECK_INS(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, &check_in_interval);
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(120000));
    }
    break;

  case TEST_STEP_READ_WWAH_CONFIGURATION_MODE_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read ConfigurationModeEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_CONFIGURATION_MODE_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_PERIODIC_ROUTER_CHECK_INS:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable Periodic Router Check Ins command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_PERIODIC_ROUTER_CHECK_INS(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_CURRENT_DEBUG_REPORT_ID:
    TRACE_MSG(TRACE_APP1, "Read CurrentDebugReportID attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_CURRENT_DEBUG_REPORT_ID_ID);
    break;
  case TEST_STEP_WWAH_SEND_SET_MAC_POLL_CCA_WAIT_TIME:
    {
      TRACE_MSG(TRACE_APP1, "Send Set MAC Poll CCA Wait Time command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_SET_MAC_POLL_CCA_WAIT_TIME(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, 16);
    }
    break;
  case TEST_STEP_READ_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read TCSecurityOnNwkKeyRotationEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_TC_SECURITY_ON_NWK_KEY_ROTATION_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_SET_PENDING_NETWORK_UPDATE:
    {
      zb_uint16_t pan_id = 0x1234;
      TRACE_MSG(TRACE_APP1, "Send Set Pending Network Update command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_SET_PENDING_NETWORK_UPDATE(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, 17, &pan_id);
    }
    break;
  case TEST_STEP_READ_WWAH_WWAH_BAD_PARENT_RECOVERY_ENABLED:
    TRACE_MSG(TRACE_APP1, "Read WWAHBadParentRecoveryEnabled attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_BAD_PARENT_RECOVERY_ENABLED_ID);
    break;
  case TEST_STEP_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t cluster_id;
      TRACE_MSG(TRACE_APP1, "Send Require APS ACKs on Unicasts command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS_START(buffer, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 3, cmd_ptr);
      cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
      ZB_ZCL_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS_ADD(cmd_ptr, &cluster_id);
      cluster_id = ZB_ZCL_WWAH_CLUSTER_ID_FREE_RECORD;
      ZB_ZCL_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS_ADD(cmd_ptr, &cluster_id);
      cluster_id = ZB_ZCL_CLUSTER_ID_WWAH;
      ZB_ZCL_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS_ADD(cmd_ptr, &cluster_id);
      ZB_ZCL_WWAH_SEND_REQUIRE_APS_ACKS_ON_UNICASTS_END(cmd_ptr, buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_CHANNEL:
    TRACE_MSG(TRACE_APP1, "Read PendingNetworkUpdateChannel attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_PENDING_NETWORK_UPDATE_CHANNEL_ID);
    break;
  case TEST_STEP_WWAH_SEND_REMOVE_APS_ACKS_ON_UNICASTS_REQUIREMENT:
    {
      TRACE_MSG(TRACE_APP1, "Send Remove APS ACKs on Unicasts Requirement command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_REMOVE_APS_ACKS_ON_UNICASTS_REQUIREMENT(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_PANID:
    TRACE_MSG(TRACE_APP1, "Read PendingNetworkUpdatePANID attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_PENDING_NETWORK_UPDATE_PANID_ID);
    break;
  case TEST_STEP_WWAH_CHECK_PENDING_NETWORK_UPDATE1:
    TRACE_MSG(TRACE_APP1, "Read PendingNetworkUpdatePANID attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    mgmt_nwk_update(param,1l<<18, ZB_FALSE);
    break;
  case TEST_STEP_WWAH_CHECK_PENDING_NETWORK_UPDATE2:
    TRACE_MSG(TRACE_APP1, "Read PendingNetworkUpdatePANID attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    zb_panid_conflict_send_nwk_update(param);
    break;
  case TEST_STEP_WWAH_CHECK_TC_SECURITY_ON_NWK_KEY_ROTATION2:
    TRACE_MSG(TRACE_APP1, "Read PendingNetworkUpdatePANID attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    mgmt_nwk_update(param, 1<<17, ZB_TRUE);
    break;
  case TEST_STEP_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY1:
    {
      TRACE_MSG(TRACE_APP1, "Send APS ACK Requirement Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY2:
    {
      TRACE_MSG(TRACE_APP1, "Send APS ACK Requirement Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_APS_ACK_REQUIREMENT_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_READ_WWAH_OTA_MAX_OFFLINE_DURATION:
    TRACE_MSG(TRACE_APP1, "Read OTAMaxOfflineDuration attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_OTA_MAX_OFFLINE_DURATION_ID);
    break;
  case TEST_STEP_WWAH_SEND_DEBUG_REPORT_QUERY:
    {
      TRACE_MSG(TRACE_APP1, "Send Debug Report Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DEBUG_REPORT_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, 4);
    }
    break;
  case TEST_STEP_WWAH_SEND_SURVEY_BEACONS:
    {
      static zb_uint8_t beacon_type_flag = 0;
      TRACE_MSG(TRACE_APP1, "Send Survey Beacons command to Works with All Hubs cluster", (FMT__0));
      if (beacon_type_flag < 2)
      {
        ZB_ZCL_WWAH_SEND_SURVEY_BEACONS(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, (zb_bool_t)beacon_type_flag);
        ++beacon_type_flag;
      }
      else
      {
        g_test_step++;
      }
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10500));
    }
    break;
  case TEST_STEP_READ_WWAH_DISABLE_OTA_DOWNGRADES:
    TRACE_MSG(TRACE_APP1, "Read DisableOTADowngrades attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_DISABLE_OTA_DOWNGRADES_ID);
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_OTA_DOWNGRADES1:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable OTA Downgrades command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_OTA_DOWNGRADES(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_OTA_DOWNGRADES2:
    TRACE_MSG(TRACE_APP1, "Read DisableOTADowngrades attribute from Works with All Hubs cluster", (FMT__0));
    g_test_step++;
    send_read_attr(buffer, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_DISABLE_OTA_DOWNGRADES_ID);
    break;
#ifdef TEST_INTERPAN
  case TEST_STEP_WWAH_CHECK_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT1:
    send_interp_data(param);
    g_test_step++;
    break;
#endif
  case TEST_STEP_WWAH_SEND_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable Touchlink Interpan Message Support command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
#ifdef TEST_INTERPAN
  case TEST_STEP_WWAH_CHECK_DISABLE_TOUCHLINK_INTERPAN_MESSAGE_SUPPORT2:
    send_interp_data(param);
    g_test_step++;
    break;
#endif
  case TEST_STEP_WWAH_SEND_ENABLE_WWAH_PARENT_CLASSIFICATION:
    {
      TRACE_MSG(TRACE_APP1, "Send Enable WWAH Parent Classification command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_WWAH_PARENT_CLASSIFICATION(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_WWAH_PARENT_CLASSIFICATION:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable WWAH Parent Classification command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_WWAH_PARENT_CLASSIFICATION(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_TC_SECURITY_ON_NWK_KEY_ROTATION:
    {
      TRACE_MSG(TRACE_APP1, "Send Enable TC Security On Nwk Key Rotation command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_TC_SECURITY_ON_NWK_KEY_ROTATION(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_CHECK_TC_SECURITY_ON_NWK_KEY_ROTATION1:
    {
      TRACE_MSG(TRACE_APP1, "Check Enable TC Security On Nwk Key Rotation command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
      ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 2 * ZB_TIME_ONE_SECOND);
      send_key_switch(param);
    }
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_WWAH_BAD_PARENT_RECOVERY:
    {
      TRACE_MSG(TRACE_APP1, "Send Enable WWAH Bad Parent Recovery command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_WWAH_BAD_PARENT_RECOVERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_WWAH_BAD_PARENT_RECOVERY:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable WWAH Bad Parent Recovery command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_WWAH_BAD_PARENT_RECOVERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_ENABLE_CONFIGURATION_MODE:
    {
      TRACE_MSG(TRACE_APP1, "Send Enable Configuration Mode command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_ENABLE_CONFIGURATION_MODE(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_SEND_DISABLE_CONFIGURATION_MODE:
    {
      TRACE_MSG(TRACE_APP1, "Send Disable Configuration Mode command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_DISABLE_CONFIGURATION_MODE(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  case TEST_STEP_WWAH_CHECK_CONFIGURATION_MODE:
    {
      TRACE_MSG(TRACE_APP1, "Check Configuration Mode command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      test_bind_fsm(param);
    }
    break;
  case TEST_STEP_WWAH_SEND_USE_TRUST_CENTER_FOR_CLUSTER:
    {
      zb_uint8_t *cmd_ptr;
      zb_uint16_t cluster_id;
      TRACE_MSG(TRACE_APP1, "Send Use Trust Center for Cluster command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_USE_TRUST_CENTER_FOR_CLUSTER_START(buffer, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, 1, cmd_ptr);
      cluster_id = ZB_ZCL_CLUSTER_ID_OTA_UPGRADE;
      ZB_ZCL_WWAH_SEND_USE_TRUST_CENTER_FOR_CLUSTER_ADD(cmd_ptr, &cluster_id);
      ZB_ZCL_WWAH_SEND_USE_TRUST_CENTER_FOR_CLUSTER_END(cmd_ptr, buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, NULL);
      ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
    }
    break;
  case TEST_STEP_WWAH_SEND_TRUST_CENTER_FOR_CLUSTER_SERVER_QUERY:
    {
      TRACE_MSG(TRACE_APP1, "Send Trust Center for Cluster Server Query command to Works with All Hubs cluster", (FMT__0));
      g_test_step++;
      ZB_ZCL_WWAH_SEND_TRUST_CENTER_FOR_CLUSTER_SERVER_QUERY(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);
    }
    break;
  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    ++g_error_cnt;
    break;
  }

  TRACE_MSG(TRACE_APP3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */

/**
   Callback called when match desc req seeking for WWAH cluster is completed.
*/
static void wwah_match_desc_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_uint8_t *zdp_cmd = zb_buf_begin(buf);
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zdp_cmd;

  if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len == 1)
  {
    g_wwah_srv_addr = resp->nwk_addr;
    g_wwah_srv_ep = *((zb_uint8_t*)(resp + 1));
    ZB_SCHEDULE_APP_ALARM_CANCEL(button_press_handler, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_APP_ALARM(button_press_handler, 0, 1 * ZB_TIME_ONE_SECOND);
  }
  zb_buf_free(buf);
}

/**
   Seek for WWAH cluster at the network.
*/
static void send_wwah_match_desc(zb_uint8_t param)
{
  if (!param)
  {
    zb_buf_get_out_delayed(send_wwah_match_desc);
  }
  else
  {
    zb_bufid_t buf = param;
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_SECUR3, "send_wwah_match_desc", (FMT__0));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

    req->nwk_addr = zb_address_short_by_ieee(g_ed_addr);
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_WWAH;

    zb_zdo_match_desc_req(param, wwah_match_desc_cb);

  }
}

/** Application signal handler. Used for informing application about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler %hd sig %hd status %hd",
            (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch (sig)
    {
      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        ZB_SCHEDULE_APP_ALARM(send_wwah_match_desc, 0, 5 * ZB_TIME_ONE_SECOND);
      break;
/** [SIGNAL_HANDLER_GET_SIGNAL] */
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifdef TEST_INSTALLCODES
        zb_secur_ic_str_add(g_ed_addr, ed1_installcode, NULL);
        zb_secur_ic_str_add(g_ed2_addr, ed2_installcode, NULL);
#endif
        zboss_start_continue();
        break;

      case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
      {
        zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
        TRACE_MSG(TRACE_APP1, "leave indication, device " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(leave_ind_params->device_addr)));
      }
      break;
      default:
        TRACE_MSG(TRACE_ERROR, "skip signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}
