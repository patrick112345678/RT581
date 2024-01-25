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
/* PURPOSE: Simple switch for HA profile
*/

#define ZB_TRACE_FILE_ID 40137
#include "zboss_api.h"

#include "test_switch_dev.h"
#include "scenes_test.h"

/* #define DST_ADDR 0 */
zb_uint16_t dst_addr = 0;
#define DST_ENDPOINT 5
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT

zb_ieee_addr_t g_ed_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void test_process_default_response(
  zb_bufid_t zcl_cmd_buf,
  zb_zcl_parsed_hdr_t* cmd_info);

static void add_to_test_group(zb_bufid_t zcl_cmd_buf);

static void check_add_scene(zb_bufid_t zcl_cmd_buf);

static void check_remove_scene(zb_bufid_t zcl_cmd_buf);

static void check_remove_all_scenes(zb_bufid_t zcl_cmd_buf);

static void check_store_scene(zb_bufid_t zcl_cmd_buf);

static void check_scene_membership(zb_bufid_t zcl_cmd_buf);

/**
 * Declaring attributes for each cluster
 */

/* Switch config cluster attributes */
zb_uint8_t attr_switch_type =
    ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_TOGGLE;
zb_uint8_t attr_switch_actions =
    ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(
    switch_cfg_attr_list,
    &attr_switch_type, &attr_switch_actions);

/* Basic cluster attributes */
zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
    basic_attr_list,
    &attr_zcl_version,
    &attr_power_source);

/* Identify cluster attributes */
zb_uint16_t attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

/* Declare cluster list for the device */
ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST(
    on_off_switch_clusters,
    switch_cfg_attr_list,
    basic_attr_list,
    identify_attr_list);

/* Declare endpoint */
ZB_HA_DECLARE_ON_OFF_SWITCH_EP(
    on_off_switch_ep,
    HA_SWITCH_ENDPOINT,
    on_off_switch_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_ON_OFF_SWITCH_CTX(on_off_switch_ctx, on_off_switch_ep);


/*! Program states according to test scenario */
enum test_states_e
{
  /*! Initial test state */
  SCENES_TEST_STATE_INITIAL,
  SCENES_TEST_STATE_ADD_GROUP,
  /*! Add Group command sent (test step 3 started) */
  SCENES_TEST_STATE_SENT_ADD_SCENE,
  /*! View Scene command sent (test step 4 started) */
  SCENES_TEST_STATE_SENT_VIEW_SCENE,
  /*! Remove Scene command sent (test step 5 started) */
  SCENES_TEST_STATE_SENT_REMOVE_SCENE,
  /*! Remove All Scenes command sent (test step 6 started) */
  SCENES_TEST_STATE_SENT_REMOVE_ALL_SCENES,
  /*! Store Scene command sent (test step 7 started) */
  SCENES_TEST_STATE_SENT_STORE_SCENE,
  /*! Recall Scene command sent (test step 9 started) */
  SCENES_TEST_STATE_SENT_RECALL_SCENE,
  /*! Get Scene Membership command sent (test step 11 started) */
  SCENES_TEST_STATE_SENT_GET_SCENE_MEMBERSHIP,
  /*! Test finished */
  SCENES_TEST_STATE_STATE_FINISHED
};

/*! @brief Test harness state
    Takes values of @ref test_states_e
*/
zb_uint8_t g_test_state = SCENES_TEST_STATE_INITIAL;

zb_short_t g_error_cnt = 0;

MAIN()
{
  ARGV_UNUSED;

  /* Global ZBOSS initialization */
  ZB_INIT("scenes_zed");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_ed_addr);
  zb_set_network_ed_role(1l<<21);
  zb_set_nvram_erase_at_start(ZB_FALSE);

  /* Configure end-device parameters */
  zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_switch_ctx);
  /* Register cluster commands handler for a specific endpoint */
  ZB_AF_SET_ENDPOINT_HANDLER(HA_SWITCH_ENDPOINT, zcl_specific_cluster_cmd_handler);

  /* Initiate the stack start with starting the commissioning */
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/*! Test step */
void next_step(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = 0;
  zb_uint8_t *cmd_ptr;

  TRACE_MSG(TRACE_APP1, "> next_step: param: %hd, state %d",
            (FMT__H_D, param, g_test_state));

  if (!param)
  {
    zb_buf_get_out_delayed(next_step);
    return;
  }

  zcl_cmd_buf = param;

  if (g_test_state == SCENES_TEST_STATE_INITIAL)
  {
    TRACE_MSG(
        TRACE_APP3,
        "test step 1, SCENES_TEST_STATE_INITIAL",
        (FMT__0));

    TRACE_MSG(TRACE_APP3, "Add DUT to group 1", (FMT__0));
    add_to_test_group(zcl_cmd_buf);
    g_test_state = SCENES_TEST_STATE_ADD_GROUP;
  }
  else if (g_test_state == SCENES_TEST_STATE_ADD_GROUP)
  {
    TRACE_MSG(TRACE_APP3, "Send Add Scene to DUT", (FMT__0));
    ZB_ZCL_SCENES_INIT_ADD_SCENE_REQ(
        zcl_cmd_buf,
        cmd_ptr,
        ZB_FALSE,
        TEST_GROUP_ID,
        TEST_SCENE_ID_1,
        TEST_TRANSITION_TIME_1);
    ZB_ZCL_SCENES_INIT_FIELDSET(cmd_ptr, ZB_ZCL_CLUSTER_ID_ON_OFF, sizeof(zb_uint8_t));
    ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, (zb_uint8_t)ZB_TRUE); /* OnOff fieldset data */
    ZB_ZCL_SCENES_SEND_ADD_SCENE_REQ(
        zcl_cmd_buf,
        cmd_ptr,
        dst_addr,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        NULL);

    g_test_state = SCENES_TEST_STATE_SENT_ADD_SCENE;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_ADD_SCENE)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 3, SCENES_TEST_STATE_SENT_ADD_SCENE, parse reponse",
        (FMT__0));
    check_add_scene(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP3, "Send View Scene cmd", (FMT__0));
    ZB_ZCL_SCENES_SEND_VIEW_SCENE_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
        NULL,
        TEST_GROUP_ID,
        TEST_SCENE_ID_1);

    g_test_state = SCENES_TEST_STATE_SENT_VIEW_SCENE;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_VIEW_SCENE)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 4, SCENES_TEST_STATE_SENT_VIEW_SCENE, do not parse reponse",
        (FMT__0));

    TRACE_MSG(TRACE_APP3, "Send Remove Scene command", (FMT__0));
    /** [Send Remove Scene command] */
    ZB_ZCL_SCENES_SEND_REMOVE_SCENE_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ADDR_MODE,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
        NULL,
        TEST_GROUP_ID,
        TEST_SCENE_ID_1);
    /** [Send Remove Scene command] */

    g_test_state = SCENES_TEST_STATE_SENT_REMOVE_SCENE;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_REMOVE_SCENE)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 5, SCENES_TEST_STATE_SENT_REMOVE_SCENE, parse response",
        (FMT__0));
    check_remove_scene(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP3, "Send Remove all scenes command", (FMT__0));
    /** [Send Remove All Scene command] */
    ZB_ZCL_SCENES_SEND_REMOVE_ALL_SCENES_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ADDR_MODE,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
        NULL,
        TEST_GROUP_ID);
    /** [Send Remove All Scene command] */

    g_test_state = SCENES_TEST_STATE_SENT_REMOVE_ALL_SCENES;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_REMOVE_ALL_SCENES)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 6, SCENES_TEST_STATE_SENT_REMOVE_ALL_SCENES, parse reponse",
        (FMT__0));
    check_remove_all_scenes(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP3, "Send Store scene command", (FMT__0));
    /** [Send Store Scene command] */
    ZB_ZCL_SCENES_SEND_STORE_SCENE_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ADDR_MODE,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
        NULL,
        TEST_GROUP_ID,
        TEST_SCENE_ID_1);
    /** [Send Store Scene command] */

    g_test_state = SCENES_TEST_STATE_SENT_STORE_SCENE;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_STORE_SCENE)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 7, SCENES_TEST_STATE_SENT_STORE_SCENE, parse reponse",
        (FMT__0));
    check_store_scene(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP3, "Send Recall Scene cmd", (FMT__0));
    /** [Send Recall Scene command] */
    ZB_ZCL_SCENES_SEND_RECALL_SCENE_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ADDR_MODE,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
        NULL,
        TEST_GROUP_ID,
        TEST_SCENE_ID_1);
    /** [Send Recall Scene command] */
    ZB_SCHEDULE_APP_ALARM(next_step, 0, ZB_TIME_ONE_SECOND * 3);
    g_test_state = SCENES_TEST_STATE_SENT_RECALL_SCENE;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_RECALL_SCENE)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 9, SCENES_TEST_STATE_SENT_RECALL_SCENE, no reponse",
        (FMT__0));

    TRACE_MSG(TRACE_APP3, "Send Get Scene Membership cmd", (FMT__0));

    /** [Send Get Scene Membership command] */
    ZB_ZCL_SCENES_SEND_GET_SCENE_MEMBERSHIP_REQ(
        zcl_cmd_buf,
        dst_addr,
        DST_ADDR_MODE,
        DST_ENDPOINT,
        HA_SWITCH_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_FALSE,
        NULL,
        TEST_GROUP_ID);
    /** [Send Get Scene Membership command] */

    g_test_state = SCENES_TEST_STATE_SENT_GET_SCENE_MEMBERSHIP;
  }
  else if (g_test_state == SCENES_TEST_STATE_SENT_GET_SCENE_MEMBERSHIP)
  {
    TRACE_MSG(
        TRACE_APP3,
        "finishing test step 11, SCENES_TEST_STATE_SENT_GET_SCENE_MEMBERSHIP, parse reponse",
        (FMT__0));

    check_scene_membership(zcl_cmd_buf);

    g_test_state = SCENES_TEST_STATE_STATE_FINISHED;
  }
  else
  {
    g_test_state = SCENES_TEST_STATE_STATE_FINISHED;
    g_error_cnt++;
    TRACE_MSG(TRACE_ERROR, "ERROR unknown state", (FMT__0));
  }

  if (g_test_state == SCENES_TEST_STATE_STATE_FINISHED)
  {
    zb_buf_free(zcl_cmd_buf);
    if (!g_error_cnt)
    {
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
    }
    else
    {
      TRACE_MSG(
          TRACE_APP1,
          "Test finished. Status: FAILED, error number %hd",
          (FMT__H, g_error_cnt));
    }
  }

  TRACE_MSG(
      TRACE_APP3,
      "< next_step: g_test_state: %hd",
      (FMT__H, g_test_state));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info =
      ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed = ZB_FALSE;


  TRACE_MSG(
      TRACE_APP1,
      "> zcl_specific_cluster_cmd_handler %hd",
      (FMT__H, param));

  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %hd", (FMT__H, zb_buf_len(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    TRACE_MSG(
        TRACE_APP1,
        "Skip command, Unsupported direction \"to server\"",
        (FMT__0));
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_SCENES:
      {
        if (cmd_info->is_common_command)
        {
          TRACE_MSG(TRACE_APP1, "Scenes cluster, process general command %hd",
                    (FMT__H, cmd_info->cmd_id));
          switch (cmd_info->cmd_id)
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              test_process_default_response(zcl_cmd_buf, cmd_info);
              cmd_processed = ZB_TRUE;
              break;
            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              next_step(param);
              cmd_processed = ZB_TRUE;
              break;
            default:
              TRACE_MSG(
                  TRACE_ERROR,
                  "ERROR unsupported common command",
                  (FMT__0));
              break;
          }
        }
        else
        {
          TRACE_MSG(
              TRACE_APP3,
              "Scenes cluster, process specific command %hd",
              (FMT__H, cmd_info->cmd_id));
          next_step(param);
          cmd_processed = ZB_TRUE;
        }
      }
        break;

      default:
        TRACE_MSG(
            TRACE_ERROR,
            "ERROR cluster 0x%x is not supported in the test",
            (FMT__D, cmd_info->cluster_id));
        break;
    }
  }

  TRACE_MSG(
      TRACE_APP1,
      "< zcl_specific_cluster_cmd_handler cmd_processed %hd",
      (FMT__H, cmd_processed));
  return cmd_processed;
}

void test_process_default_response(
  zb_bufid_t zcl_cmd_buf,
  zb_zcl_parsed_hdr_t* cmd_info)
{
  zb_zcl_default_resp_payload_t* payload;

  TRACE_MSG(
      TRACE_APP1,
      "> test_process_default_response zcl_cmd_buf %p cmd_info %p",
      (FMT__P_P, zcl_cmd_buf, cmd_info));

  if (ZB_ZCL_CLUSTER_ID_SCENES != cmd_info->cluster_id)
  {
    payload = ZB_ZCL_READ_DEFAULT_RESP(zcl_cmd_buf);
    TRACE_MSG(TRACE_APP3, "COMMAND ID: 0x%hx, is successfull: 0x%hx",
              (FMT__H_H, payload->command_id, payload->status));
  }

  next_step(zcl_cmd_buf);
  TRACE_MSG(TRACE_APP1, "< test_process_default_response", (FMT__0));
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_APP_ALARM(next_step, param, ZB_TIME_ONE_SECOND * 3);
        param = 0;
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

static void check_scene_membership(zb_bufid_t zcl_cmd_buf)
{
  zb_zcl_scenes_get_scene_membership_res_t* response;
  zb_uint16_t tmp;

  TRACE_MSG(
      TRACE_APP1,
      "> check_scene_membership zcl_cmd_buf %p",
      (FMT__P, zcl_cmd_buf));

  /** [Parse Get Scene Membership response] */
  ZB_ZCL_SCENES_GET_GET_SCENE_MEMBERSHIP_RES(zcl_cmd_buf, response);
  /** [Parse Get Scene Membership response] */
  if (! response)
  {
    ++g_error_cnt;
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Malformed Scenes.GetSceneMembershipResponse",
        (FMT__0));
    goto check_scene_membership_finals;
  }
  ZB_ASSIGN_UINT16(&tmp, &(response->mandatory.group_id));
  if (! (   response->mandatory.status == ZB_ZCL_STATUS_SUCCESS
         && tmp == TEST_GROUP_ID)) /* Don't mention capacity there */
  {
    ++g_error_cnt;
    TRACE_MSG(TRACE_APP3, "Scenes.ViewSceneResponse failed:", (FMT__0));
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR status %hd group_id 0x%04x capacity %hd",
        (
          FMT__H_D_H,
          response->mandatory.status,
          response->mandatory.group_id,
          tmp));
    goto check_scene_membership_finals;
  }
  if (1 != response->optional.scene_count)
  {
    ++g_error_cnt;
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Reported SceneCount %hd (should be 1)",
        (FMT__H, response->optional.scene_count));
    goto check_scene_membership_finals;
  }
  if (TEST_SCENE_ID_1 != response->optional.scene_list[0])
  {
    ++g_error_cnt;
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Reported scene_id %hd (should be %hd)",
        ( FMT__H_H,
          response->optional.scene_list[0],
          (zb_uint8_t)TEST_SCENE_ID_1));
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "Scenes.GetSceneMembership ok", (FMT__0));
  }

check_scene_membership_finals:
  TRACE_MSG( TRACE_APP1, "< check_scene_membership", (FMT__0));
}

static void add_to_test_group(zb_bufid_t buf)
{
/* [apsme_add_group_req] */
  zb_apsme_add_group_req_t *aps_req;

  TRACE_MSG(TRACE_APP1, "> add_to_test_group buf %p", (FMT__P, buf));

  zb_buf_reuse(buf);
  aps_req = ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_req_t);
  ZB_BZERO(aps_req, sizeof(*aps_req));
  aps_req->group_address = TEST_GROUP_ID;
  aps_req->endpoint = HA_SWITCH_ENDPOINT;

  zb_apsme_add_group_request(buf);
/* [apsme_add_group_req] */

  zb_buf_get_out_delayed(next_step);
  TRACE_MSG(TRACE_APP1, "< add_to_test_group", (FMT__0));
}

static void check_add_scene(zb_bufid_t zcl_cmd_buf)
{
  zb_zcl_scenes_add_scene_res_t* response;
  zb_uint16_t group_id;

  TRACE_MSG(TRACE_APP1, "> check_add_scene zcl_cmd_buf %p", (FMT__P, zcl_cmd_buf));

  ZB_ZCL_SCENES_GET_ADD_SCENE_RES(zcl_cmd_buf, response);
  if (! response)
  {
    ++g_error_cnt;
    TRACE_MSG(TRACE_ERROR, "ERROR Malformed Scenes.AddSceneResponse", (FMT__0));
    goto check_add_scene_finals;
  }
  ZB_ASSIGN_UINT16(&group_id, &(response->group_id));
  if (! (   response->status == ZB_ZCL_STATUS_SUCCESS
         && group_id == TEST_GROUP_ID
         && response->scene_id == TEST_SCENE_ID_1))
  {
    ++g_error_cnt;
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Errouneous Scenes.AddSceneResponse status 0x%hx group_id 0x%04x scene_id %hd",
        (FMT__H_D_H, response->status, group_id, response->scene_id));
  }

check_add_scene_finals:
  TRACE_MSG(TRACE_APP1, "< check_add_scene", (FMT__0));
}

static void check_remove_scene(zb_bufid_t zcl_cmd_buf)
{
  zb_zcl_scenes_remove_scene_res_t* response;
  zb_uint16_t group_id;

  TRACE_MSG(TRACE_APP1, "> check_remove_scene zcl_cmd_buf %p", (FMT__P, zcl_cmd_buf));

  /** [Parse Remove Scene response] */
  ZB_ZCL_SCENES_GET_REMOVE_SCENE_RES(zcl_cmd_buf, response);
  if (! response)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Malformed Scenes.RemoveSceneResponse", (FMT__0));
    goto check_remove_scene_error;
  }
  ZB_ASSIGN_UINT16(&group_id, &(response->group_id));
  if(   response->status == ZB_ZCL_STATUS_SUCCESS
     && group_id == TEST_GROUP_ID
     && response->scene_id == TEST_SCENE_ID_1)
  {
    TRACE_MSG(TRACE_APP3, "Scenes.RemoveSceneResponse ok", (FMT__0));
    goto check_remove_scene_finals;
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR erroneous response status 0x%02hx group_id 0x%04x scene_id %hd",
        (FMT__H_D_H, response->status, group_id, response->scene_id));
  }
  /** [Parse Remove Scene response] */

check_remove_scene_error:
  ++g_error_cnt;
check_remove_scene_finals:
  TRACE_MSG(TRACE_APP1, "< check_remove_scene", (FMT__0));
}/* static void check_remove_scene(zb_bufid_t zcl_... */

static void check_remove_all_scenes(zb_bufid_t zcl_cmd_buf)
{
  zb_zcl_scenes_remove_all_scenes_res_t* response;
  zb_uint16_t group_id;

  TRACE_MSG(TRACE_APP1, "> check_remove_all_scenes zcl_cmd_buf %p", (FMT__P, zcl_cmd_buf));

  /** [Parse Remove All Scene response] */
  ZB_ZCL_SCENES_GET_REMOVE_ALL_SCENES_RES(zcl_cmd_buf, response);
  if (! response)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Malformed Scenes.RemoveAllScenesResponse", (FMT__0));
    ++g_error_cnt;
  }
  else
  {
    ZB_ASSIGN_UINT16(&group_id, &(response->group_id));
    if (response->status == ZB_ZCL_STATUS_SUCCESS && group_id == TEST_GROUP_ID)
    {
      TRACE_MSG(TRACE_APP1, "Scenes.RemoveAllScenesResponse ok", (FMT__0));
    }
    else
    {
      ++g_error_cnt;
      TRACE_MSG(
          TRACE_ERROR,
          "ERROR status 0x%02hx group_id 0x%04x",
          (FMT__H_D, response->status, group_id));
    }
  }
  /** [Parse Remove All Scene response] */

  TRACE_MSG(TRACE_APP1, "< check_remove_all_scenes", (FMT__0));
}/* static void check_remove_all_scenes(zb_bufid_t... */

static void check_store_scene(zb_bufid_t zcl_cmd_buf)
{
  zb_zcl_scenes_store_scene_res_t* response;
  zb_uint16_t group_id;

  TRACE_MSG(TRACE_APP1, "> check_store_scene zcl_cmd_buf %p", (FMT__P, zcl_cmd_buf));

  /** [Parse Store Scene response] */
  ZB_ZCL_SCENES_GET_STORE_SCENE_RES(zcl_cmd_buf, response);
  /** [Parse Store Scene response] */
  if (! response)
  {
    ++g_error_cnt;
    TRACE_MSG(TRACE_ERROR, "ERROR Malformed Scenes.StoreSceneResponse", (FMT__0));
  }
  else
  {
    ZB_ASSIGN_UINT16(&group_id, &(response->group_id));
    if (    response->status == ZB_ZCL_STATUS_SUCCESS
        &&  group_id == TEST_GROUP_ID
        &&  response->scene_id == TEST_SCENE_ID_1)
    {
      TRACE_MSG(TRACE_APP3, "Scenes.StoreSceneResponse ok", (FMT__0));
    }
    else
    {
      ++g_error_cnt;
      TRACE_MSG(
          TRACE_ERROR,
          "ERROR status 0x%02hx group_id 0x%04x scene_id %hd",
          (FMT__H_D_H, response->status, group_id, response->scene_id));
    }
  }

  TRACE_MSG(TRACE_APP1, "< check_store_scene", (FMT__0));
}/* static void check_store_scene(zb_bufid_t zcl_c... */
