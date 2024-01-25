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
/* PURPOSE: Simple output for HA profile
*/

#define ZB_TRACE_FILE_ID 40136
#include "zboss_api.h"

#include "test_output_dev.h"
#include "scenes_test.h"


void test_device_cb(zb_uint8_t param);
zb_uint8_t test_device_scenes_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id);
void test_device_scenes_remove_entries_by_group(zb_uint16_t group_id);
void test_device_scenes_table_init(void);

#define TEST_DEVICE_SCENES_TABLE_SIZE 3

/* Define structure for saving scene state */
typedef struct test_device_scenes_table_entry_s
{
  zb_zcl_scene_table_record_fixed_t common;
  zb_uint8_t onoff_state;
}
test_device_scenes_table_entry_t;

/* Define table for saving scene state */
test_device_scenes_table_entry_t scenes_table[TEST_DEVICE_SCENES_TABLE_SIZE];

typedef struct resp_info_s
{
  zb_zcl_parsed_hdr_t cmd_info;
  zb_zcl_scenes_view_scene_req_t view_scene_req;
  zb_zcl_scenes_get_scene_membership_req_t get_scene_membership_req;
} resp_info_t;

resp_info_t resp_info;

static void add_to_test_group(zb_uint8_t param);

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}; /* IEEE address of the
                                                                              * device */

zb_uint16_t g_dst_addr;
zb_uint8_t g_addr_mode;
zb_uint8_t g_endpoint;

zb_uint8_t g_on_value = ZB_TRUE;
zb_uint8_t g_off_value = ZB_FALSE;

#ifdef ZB_USE_NVRAM
typedef struct nvram_app_dataset_s
{
  zb_uint8_t start_up_on_off;
  zb_uint8_t align[3];
} ZB_PACKED_STRUCT nvram_app_dataset_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(nvram_app_dataset_t);



#endif

/**
 * Declaring attributes for each cluster
 */

/* On/Off cluster attributes */
zb_uint8_t g_attr_on_off = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;

zb_bool_t g_attr_global_scene_ctrl = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
zb_uint16_t g_attr_on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
zb_uint16_t g_attr_off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_start_up_on_off;

zb_bool_t g_on_off_cluster_delayed_off_state = ZB_FALSE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT_WITH_START_UP_ON_OFF(on_off_attr_list,
                                                           &g_attr_on_off,
                                                           &g_attr_global_scene_ctrl,
                                                           &g_attr_on_time,
                                                           &g_attr_off_wait_time,
                                                           &g_attr_start_up_on_off);

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list,
                                 &g_attr_zcl_version,
                                 &g_attr_power_source);

/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attributes */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene =
    ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group =
    ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support =
    ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* Declare cluster list for a device */
ZB_HA_DECLARE_ON_OFF_OUTPUT_CLUSTER_LIST(
    on_off_output_clusters,
    on_off_attr_list,
    basic_attr_list,
    identify_attr_list,
    groups_attr_list,
    scenes_attr_list);

/* Declare endpoint */
ZB_HA_DECLARE_ON_OFF_OUTPUT_EP(
    on_off_output_ep,
    HA_OUTPUT_ENDPOINT,
    on_off_output_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_ON_OFF_OUTPUT_CTX(on_off_output_ctx, on_off_output_ep);

void apply_start_up_on_off(void)
{
  switch(g_attr_start_up_on_off)
  {
    case ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_OFF:
    {
      g_attr_on_off = ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_OFF;
      break;
    }
    case ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_ON:
    {
      g_attr_on_off = ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_ON;
      break;
    }
    case ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_TOGGLE:
    {
      g_attr_on_off = (g_attr_on_off == ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_OFF) ? ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_ON : ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_OFF;
      break;
    }
    case ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_PREVIOUS:
    {
      break;
    }
    default:
    {
      break;
    }
  }
}

MAIN()
{
  ARGV_UNUSED;

  /* Global ZBOSS initialization */
  ZB_INIT("scenes_zc");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(1l<<21);
  zb_set_nvram_erase_at_start(ZB_FALSE);

  /* Set Device user application callback */
  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);
  /* Register device ZCL context */
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_output_ctx);
  /* Register cluster commands handler for a specific endpoint */
  ZB_AF_SET_ENDPOINT_HANDLER(HA_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);

  test_device_scenes_table_init();

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

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_uint8_t ret = ZB_FALSE;
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Unsupported \"from server\" command direction",
        (FMT__0));
  }

  return ret;
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));
        add_to_test_group(param);
        param = 0;
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APP1, "Finding&binding done", (FMT__0));
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
    TRACE_MSG(TRACE_ERROR,
              "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<<zboss_signal_handler", (FMT__0));
}

static void add_to_test_group(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_apsme_add_group_req_t *aps_req;

  TRACE_MSG(TRACE_APP1, "> add_to_test_group buf %p", (FMT__P, buf));

  zb_buf_reuse(buf);
  aps_req = ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_req_t);
  ZB_BZERO(aps_req, sizeof(*aps_req));
  aps_req->group_address = TEST_GROUP_ID;
  aps_req->endpoint = HA_OUTPUT_ENDPOINT;

  zb_apsme_add_group_request(buf);

  TRACE_MSG(TRACE_APP1, "< add_to_test_group", (FMT__0));
}

void send_view_scene_resp(zb_uint8_t param, zb_uint16_t idx)
{
  zb_bufid_t buf = param;
  zb_uint8_t *payload_ptr;
  zb_uint8_t view_scene_status = ZB_ZCL_STATUS_NOT_FOUND;

  TRACE_MSG(TRACE_APP1, ">> send_view_scene_resp param %hd idx %d", (FMT__H_D, param, idx));

  if (idx != 0xFF &&
      scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
  {
    /* Scene found */
    view_scene_status = ZB_ZCL_STATUS_SUCCESS;
  }
  else if (!zb_aps_is_endpoint_in_group(
             resp_info.view_scene_req.group_id,
             ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint))
  {
    /* Not in the group */
    view_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
  }

  ZB_ZCL_SCENES_INIT_VIEW_SCENE_RES(
    buf,
    payload_ptr,
    resp_info.cmd_info.seq_number,
    view_scene_status,
    resp_info.view_scene_req.group_id,
    resp_info.view_scene_req.scene_id);

  if (view_scene_status == ZB_ZCL_STATUS_SUCCESS)
  {
    ZB_ZCL_SCENES_ADD_TRANSITION_TIME_VIEW_SCENE_RES(
      payload_ptr,
      scenes_table[idx].common.transition_time);

    ZB_ZCL_SCENES_ADD_SCENE_NAME_VIEW_SCENE_RES(
      payload_ptr,
      scenes_table[idx].common.scene_name);

    /* Extention set: Cluster ID = On/Off */
    ZB_ZCL_PACKET_PUT_DATA16_VAL(payload_ptr, ZB_ZCL_CLUSTER_ID_ON_OFF);

    /* Extention set: Fieldset length = 1 */
    ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, 1);

    /* Extention set: On/Off state */
    ZB_ZCL_PACKET_PUT_DATA8(payload_ptr, scenes_table[idx].onoff_state);
  }

  ZB_ZCL_SCENES_SEND_VIEW_SCENE_RES(
    buf,
    payload_ptr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
    resp_info.cmd_info.profile_id,
    NULL);

  TRACE_MSG(TRACE_APP1, "<< send_view_scene_resp", (FMT__0));
}

void send_get_scene_membership_resp(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_uint8_t *payload_ptr;
  zb_uint8_t *capacity_ptr;
  zb_uint8_t *scene_count_ptr;

  TRACE_MSG(TRACE_APP1, ">> send_get_scene_membership_resp param %hd", (FMT__H, param));

  if (!zb_aps_is_endpoint_in_group(
        resp_info.get_scene_membership_req.group_id,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint))
  {
    /* Not in the group */
    ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
      buf,
      payload_ptr,
      resp_info.cmd_info.seq_number,
      capacity_ptr,
      ZB_ZCL_STATUS_INVALID_FIELD,
      ZB_ZCL_SCENES_CAPACITY_UNKNOWN,
      resp_info.get_scene_membership_req.group_id);
  }
  else
  {
    zb_uint8_t i = 0;

    ZB_ZCL_SCENES_INIT_GET_SCENE_MEMBERSHIP_RES(
      buf,
      payload_ptr,
      resp_info.cmd_info.seq_number,
      capacity_ptr,
      ZB_ZCL_STATUS_SUCCESS,
      0,
      resp_info.get_scene_membership_req.group_id);

    scene_count_ptr = payload_ptr;
    ZB_ZCL_SCENES_ADD_SCENE_COUNT_GET_SCENE_MEMBERSHIP_RES(payload_ptr, 0);

    while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
    {
      if (scenes_table[i].common.group_id == resp_info.get_scene_membership_req.group_id)
      {
        /* Add to payload */
        TRACE_MSG(TRACE_APP1, "add scene_id %hd", (FMT__H, scenes_table[i].common.scene_id));
        ++(*scene_count_ptr);
        ZB_ZCL_SCENES_ADD_SCENE_ID_GET_SCENE_MEMBERSHIP_RES(
          payload_ptr,
          scenes_table[i].common.scene_id);
      }
      else if (scenes_table[i].common.group_id == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        TRACE_MSG(TRACE_APP1, "add capacity num", (FMT__0));
        ++(*capacity_ptr);
      }
      ++i;
    }
  }

  ZB_ZCL_SCENES_SEND_GET_SCENE_MEMBERSHIP_RES(
    buf,
    payload_ptr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
    ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
    resp_info.cmd_info.profile_id,
    NULL);

  TRACE_MSG(TRACE_APP1, "<< send_get_scene_membership_resp", (FMT__0));
}

void test_device_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);
  TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H,
      param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;
  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
      TRACE_MSG(TRACE_APP1, "on/off setting to %hd", (FMT__H, device_cb_param->cb_param.set_attr_value_param.values.data8));
      break;

      /* >>>> Scenes */
    case ZB_ZCL_SCENES_ADD_SCENE_CB_ID:
    {
      const zb_zcl_scenes_add_scene_req_t *add_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_add_scene_req_t);
      zb_bufid_t buf = param;
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *add_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_ADD_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd transition_time %d", (FMT__D_H_D, add_scene_req->group_id, add_scene_req->scene_id, add_scene_req->transition_time));

      *add_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
      idx = test_device_scenes_get_entry(add_scene_req->group_id, add_scene_req->scene_id);

      if (idx != 0xFF)
      {
        zb_zcl_scenes_fieldset_common_t *fieldset;
        zb_uint8_t fs_content_length;
        zb_uint8_t *fs_data_ptr;

        if (scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
        {
          /* Indicate that we overwriting existing record */
          device_cb_param->status = RET_ALREADY_EXISTS;
        }

        ZB_ZCL_SCENES_GET_ADD_SCENE_REQ_NEXT_FIELDSET_DESC(buf,
                                                           fieldset,
                                                           fs_content_length);
        if (fieldset &&
            fieldset->cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
            fieldset->fieldset_length == 1)
        {
          /* Store this scene */
          scenes_table[idx].common.group_id = add_scene_req->group_id;
          scenes_table[idx].common.scene_id = add_scene_req->scene_id;
          scenes_table[idx].common.transition_time = add_scene_req->transition_time;
          fs_data_ptr = (zb_uint8_t*)fieldset + sizeof(zb_zcl_scenes_fieldset_common_t);
          scenes_table[idx].onoff_state = *fs_data_ptr;
          TRACE_MSG(TRACE_APP1, "onoff_state %hd", (FMT__H, scenes_table[idx].onoff_state));
          *add_scene_status = ZB_ZCL_STATUS_SUCCESS;
        }
      }
      else
      {
        *add_scene_status = ZB_ZCL_STATUS_INSUFF_SPACE;
      }
    }
    break;
    case ZB_ZCL_SCENES_VIEW_SCENE_CB_ID:
    {
      const zb_zcl_scenes_view_scene_req_t *view_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_view_scene_req_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);
      zb_uint8_t idx = 0xFF;

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_VIEW_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, view_scene_req->group_id, view_scene_req->scene_id));

      idx = test_device_scenes_get_entry(view_scene_req->group_id, view_scene_req->scene_id);

      /* Send View Scene Response */
      ZB_MEMCPY(&resp_info.cmd_info, in_cmd_info, sizeof(zb_zcl_parsed_hdr_t));
      ZB_MEMCPY(&resp_info.view_scene_req, view_scene_req, sizeof(zb_zcl_scenes_view_scene_req_t));
      zb_buf_get_out_delayed_ext(send_view_scene_resp, idx, 0);
    }
    break;
    case ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID:
    {
      const zb_zcl_scenes_remove_scene_req_t *remove_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_remove_scene_req_t);
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *remove_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, remove_scene_req->group_id, remove_scene_req->scene_id));

      *remove_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
      idx = test_device_scenes_get_entry(remove_scene_req->group_id, remove_scene_req->scene_id);

      if (idx != 0xFF &&
          scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        /* Remove this entry */
        scenes_table[idx].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
        TRACE_MSG(TRACE_APP1, "removing scene: entry idx %hd", (FMT__H, idx));
        *remove_scene_status = ZB_ZCL_STATUS_SUCCESS;
      }
      else if (!zb_aps_is_endpoint_in_group(
                 remove_scene_req->group_id,
                 ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint))
      {
        *remove_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
      }
    }
    break;
    case ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID:
    {
      const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_remove_all_scenes_req_t);
      zb_uint8_t *remove_all_scenes_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_REMOVE_ALL_SCENES_CB_ID: group_id 0x%x", (FMT__D, remove_all_scenes_req->group_id));

      if (!zb_aps_is_endpoint_in_group(
                 remove_all_scenes_req->group_id,
                 ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint))
      {
        *remove_all_scenes_status = ZB_ZCL_STATUS_INVALID_FIELD;
      }
      else
      {
        test_device_scenes_remove_entries_by_group(remove_all_scenes_req->group_id);
        *remove_all_scenes_status = ZB_ZCL_STATUS_SUCCESS;
      }
    }
    break;
    case ZB_ZCL_SCENES_STORE_SCENE_CB_ID:
    {
      const zb_zcl_scenes_store_scene_req_t *store_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_store_scene_req_t);
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *store_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_STORE_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, store_scene_req->group_id, store_scene_req->scene_id));

      if (!zb_aps_is_endpoint_in_group(
                 store_scene_req->group_id,
                 ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).dst_endpoint))
      {
        *store_scene_status = ZB_ZCL_STATUS_INVALID_FIELD;
      }
      else
      {
        idx = test_device_scenes_get_entry(store_scene_req->group_id, store_scene_req->scene_id);

        if (idx != 0xFF)
        {
          if (scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
          {
            /* Update existing entry with current On/Off state */
            device_cb_param->status = RET_ALREADY_EXISTS;
            scenes_table[idx].onoff_state = g_attr_on_off;
            TRACE_MSG(TRACE_APP1, "update scene: entry idx %hd onoff_state %hd", (FMT__H_H, idx, scenes_table[idx].onoff_state));
          }
          else
          {
            /* Create new entry with empty name and 0 transition time */
            scenes_table[idx].common.group_id = store_scene_req->group_id;
            scenes_table[idx].common.scene_id = store_scene_req->scene_id;
            scenes_table[idx].common.transition_time = 0;
            scenes_table[idx].onoff_state = g_attr_on_off;
            TRACE_MSG(TRACE_APP1, "store new scene: entry idx %hd onoff_state %hd", (FMT__H_H, idx, scenes_table[idx].onoff_state));
          }

          *store_scene_status = ZB_ZCL_STATUS_SUCCESS;
        }
        else
        {
          *store_scene_status = ZB_ZCL_STATUS_INSUFF_SPACE;
        }
      }
    }
    break;
    case ZB_ZCL_SCENES_RECALL_SCENE_CB_ID:
    {
      const zb_zcl_scenes_recall_scene_req_t *recall_scene_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_recall_scene_req_t);
      zb_uint8_t idx = 0xFF;
      zb_uint8_t *recall_scene_status = ZB_ZCL_DEVICE_CMD_PARAM_OUT_GET(param, zb_uint8_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_RECALL_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, recall_scene_req->group_id, recall_scene_req->scene_id));

      idx = test_device_scenes_get_entry(recall_scene_req->group_id, recall_scene_req->scene_id);

      if (idx != 0xFF &&
          scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
      {
        /* Recall this entry */
        TRACE_MSG(TRACE_APP1, "recall scene: entry idx %hd onoff_state %hd transition_time 0x%x", (FMT__H_H_D, idx, scenes_table[idx].onoff_state, scenes_table[idx].common.transition_time));
        ZB_ZCL_SET_ATTRIBUTE(
          HA_OUTPUT_ENDPOINT,
          ZB_ZCL_CLUSTER_ID_ON_OFF,
          ZB_ZCL_CLUSTER_SERVER_ROLE,
          ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
          &scenes_table[idx].onoff_state,
          ZB_FALSE);
        *recall_scene_status = ZB_ZCL_STATUS_SUCCESS;
      }
      else
      {
        *recall_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
      }
    }
    break;
    case ZB_ZCL_SCENES_GET_SCENE_MEMBERSHIP_CB_ID:
    {
      const zb_zcl_scenes_get_scene_membership_req_t *get_scene_membership_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_get_scene_membership_req_t);
      const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_GET_SCENE_MEMBERSHIP_CB_ID: group_id 0x%xd", (FMT__D, get_scene_membership_req->group_id));

      /* Send View Scene Response */
      ZB_MEMCPY(&resp_info.cmd_info, in_cmd_info, sizeof(zb_zcl_parsed_hdr_t));
      ZB_MEMCPY(&resp_info.get_scene_membership_req, get_scene_membership_req, sizeof(zb_zcl_scenes_get_scene_membership_req_t));
      zb_buf_get_out_delayed(send_get_scene_membership_resp);
    }
    break;

    case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID:
    {
      const zb_zcl_scenes_remove_all_scenes_req_t *remove_all_scenes_req = ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_scenes_remove_all_scenes_req_t);

      TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_CB_ID: group_id 0x%x", (FMT__D, remove_all_scenes_req->group_id));

      /* Have only one endpoint */
      test_device_scenes_remove_entries_by_group(remove_all_scenes_req->group_id);
    }
    break;
    case ZB_ZCL_SCENES_INTERNAL_REMOVE_ALL_SCENES_ALL_ENDPOINTS_ALL_GROUPS_CB_ID:
    {
      test_device_scenes_table_init();
    }
    break;
      /* <<<< Scenes */
    default:
      device_cb_param->status = RET_ERROR;
      break;
  }
  TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}

void test_device_scenes_table_init(void)
{
  zb_uint8_t i = 0;
  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
    ++i;
  }
}

zb_uint8_t test_device_scenes_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id)
{
  zb_uint8_t i = 0;
  zb_uint8_t idx = 0xFF, free_idx = 0xFF;

  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    if (scenes_table[i].common.group_id == group_id &&
        scenes_table[i].common.group_id == scene_id)
    {
      idx = i;
      break;
    }
    else if (free_idx == 0xFF &&
             scenes_table[i].common.group_id == ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
    {
      /* Remember free index */
      free_idx = i;
    }
    ++i;
  }

  return ((idx != 0xFF) ? idx : free_idx);
}

void test_device_scenes_remove_entries_by_group(zb_uint16_t group_id)
{
  zb_uint8_t i = 0;

  TRACE_MSG(TRACE_APP1, ">> test_device_scenes_remove_entries_by_group: group_id 0x%x", (FMT__D, group_id));
  while (i < TEST_DEVICE_SCENES_TABLE_SIZE)
  {
    if (scenes_table[i].common.group_id == group_id)
    {
      TRACE_MSG(TRACE_APP1, "removing scene: entry idx %hd", (FMT__H, i));
      scenes_table[i].common.group_id = ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD;
    }
    ++i;
  }
  TRACE_MSG(TRACE_APP1, "<< test_device_scenes_remove_entries_by_group", (FMT__0));
}

#ifdef ZB_USE_NVRAM
void nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  nvram_app_dataset_t ds;
  zb_ret_t ret;

  ZB_ASSERT(payload_length == sizeof(ds));
  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));
  if (ret == RET_OK)
  {
    g_attr_start_up_on_off = ds.start_up_on_off;
    apply_start_up_on_off();
  }
}

zb_ret_t nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  nvram_app_dataset_t ds;

  ds.start_up_on_off = g_attr_start_up_on_off;
  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, sizeof(ds));
  return ret;
}

zb_uint16_t nvram_get_app_data_size(void)
{
 return sizeof(nvram_app_dataset_t);
}
#endif
