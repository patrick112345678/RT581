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

/* NOTE: This sample may be used for ZTT Color Control Server testing. */

/* ![mem_config_max] */
#define ZB_TRACE_FILE_ID 63914
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_custom_dim.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */
/* ![mem_config_max] */

#include "../common/zcl_basic_attr_list.h"
#include "ha/custom/zb_ha_custom_dimmable_light.h"

/*
#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif
*/
#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to build led bulb demo
#endif

#define ZB_OUTPUT_ENDPOINT          5
#define ZB_OUTPUT_MAX_CMD_PAYLOAD_SIZE 2

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
void test_device_interface_cb(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);
#define ENDPOINT_C 5
#define ENDPOINT_ED 10

#define TEST_DEVICE_SCENES_TABLE_SIZE 3

typedef struct test_device_scenes_table_entry_s
{
    zb_zcl_scene_table_record_fixed_t common;
    zb_uint8_t onoff_state;
    zb_uint16_t current_x;
    zb_uint16_t current_y;
    zb_uint16_t enchanced_current_hue;
    zb_uint8_t current_saturation;
    zb_uint8_t color_loop_active;
    zb_uint8_t color_loop_direction;
    zb_uint16_t color_loop_time;
    zb_uint16_t color_temperature_mireds;
}
test_device_scenes_table_entry_t;

test_device_scenes_table_entry_t scenes_table[TEST_DEVICE_SCENES_TABLE_SIZE];

typedef struct resp_info_s
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_zcl_scenes_view_scene_req_t view_scene_req;
    zb_zcl_scenes_get_scene_membership_req_t get_scene_membership_req;
} resp_info_t;

resp_info_t resp_info;

zb_uint8_t test_device_scenes_get_entry(zb_uint16_t group_id, zb_uint8_t scene_id);
void test_device_scenes_remove_entries_by_group(zb_uint16_t group_id);
void test_device_scenes_table_init();

#ifdef ZB_USE_NVRAM
typedef struct application_dataset_s
{
    zb_uint16_t start_up_color_temp_mireds;
    zb_uint8_t align[2];
} ZB_PACKED_STRUCT application_dataset_t;

ZB_ASSERT_IF_NOT_ALIGNED_TO_4(application_dataset_t);

application_dataset_t g_dev_ctx;
#endif

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
/* Groups cluster attributes data */
zb_uint8_t g_attr_groups_name_support = 0;
/* Scenes cluster attributes data */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support = 0;
/* On/Off cluster attributes data */
zb_bool_t g_attr_on_off_on_off = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
zb_bool_t g_attr_on_off_global_scene_control = ZB_ZCL_ON_OFF_GLOBAL_SCENE_CONTROL_DEFAULT_VALUE;
zb_uint16_t g_attr_on_off_on_time = ZB_ZCL_ON_OFF_ON_TIME_DEFAULT_VALUE;
zb_uint16_t g_attr_on_off_off_wait_time = ZB_ZCL_ON_OFF_OFF_WAIT_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_on_off_startup_on_off;
/* Level Control cluster attributes data */
zb_uint8_t g_attr_level_control_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_attr_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
zb_uint8_t g_attr_level_control_start_up_current_level;
zb_uint8_t g_attr_level_control_options = 0x00;
/* Color Control cluster attributes data */
zb_uint8_t g_attr_color_control_current_hue = ZB_ZCL_COLOR_CONTROL_CURRENT_HUE_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_current_saturation = ZB_ZCL_COLOR_CONTROL_CURRENT_SATURATION_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_remaining_time = ZB_ZCL_COLOR_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_current_x = ZB_ZCL_COLOR_CONTROL_CURRENT_X_DEF_VALUE;
zb_uint16_t g_attr_color_control_current_y = ZB_ZCL_COLOR_CONTROL_CURRENT_Y_DEF_VALUE;
zb_uint16_t g_attr_color_control_color_temperature = ZB_ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_DEF_VALUE;
zb_uint8_t g_attr_color_control_color_mode = ZB_ZCL_COLOR_CONTROL_COLOR_MODE_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_options = 0x01;//ZB_ZCL_COLOR_CONTROL_OPTIONS_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_number_of_primaries = 0;
zb_uint16_t g_attr_color_control_primary_1_x = 0;
zb_uint16_t g_attr_color_control_primary_1_y = 0;
zb_uint8_t g_attr_color_control_primary_1_intensity = 0;
zb_uint16_t g_attr_color_control_primary_2_x = 0;
zb_uint16_t g_attr_color_control_primary_2_y = 0;
zb_uint8_t g_attr_color_control_primary_2_intensity = 0;
zb_uint16_t g_attr_color_control_primary_3_x = 0;
zb_uint16_t g_attr_color_control_primary_3_y = 0;
zb_uint8_t g_attr_color_control_primary_3_intensity = 0;
zb_uint16_t g_attr_color_control_primary_4_x = 0;
zb_uint16_t g_attr_color_control_primary_4_y = 0;
zb_uint8_t g_attr_color_control_primary_4_intensity = 0;
zb_uint16_t g_attr_color_control_primary_5_x = 0;
zb_uint16_t g_attr_color_control_primary_5_y = 0;
zb_uint8_t g_attr_color_control_primary_5_intensity = 0;
zb_uint16_t g_attr_color_control_primary_6_x = 0;
zb_uint16_t g_attr_color_control_primary_6_y = 0;
zb_uint8_t g_attr_color_control_primary_6_intensity = 0;
zb_uint16_t g_attr_color_control_enhanced_current_hue = ZB_ZCL_COLOR_CONTROL_ENHANCED_CURRENT_HUE_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_enhanced_color_mode = ZB_ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_color_loop_active = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_ACTIVE_DEFAULT_VALUE;
zb_uint8_t g_attr_color_control_color_loop_direction = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_DIRECTION_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_color_loop_time = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_TIME_DEF_VALUE;
zb_uint16_t g_attr_color_control_color_loop_start_enhanced_hue = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_START_DEF_VALUE;
zb_uint16_t g_attr_color_control_color_loop_stored_enhanced_hue = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_STORED_ENHANCED_HUE_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_color_capabilities = (zb_uint16_t)0x001F;//ZB_ZCL_COLOR_CONTROL_COLOR_CAPABILITIES_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_color_temp_physical_min_mireds = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS_DEFAULT_VALUE + 10;
zb_uint16_t g_attr_color_control_color_temp_physical_max_mireds = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS_DEFAULT_VALUE;
zb_uint16_t g_attr_color_control_couple_color_temp_to_level_min_mireds = 0;
zb_uint16_t g_attr_color_control_start_up_color_temperature_mireds = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT_WITH_START_UP_ON_OFF(on_off_attr_list, &g_attr_on_off_on_off, &g_attr_on_off_global_scene_control, &g_attr_on_off_on_time, &g_attr_on_off_off_wait_time, &g_attr_on_off_startup_on_off);
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST_EXT(level_control_attr_list, &g_attr_level_control_current_level, &g_attr_level_control_remaining_time, &g_attr_level_control_start_up_current_level, &g_attr_level_control_options);
ZB_ZCL_DECLARE_COLOR_CONTROL_ATTRIB_LIST_EXT(color_control_attr_list, &g_attr_color_control_current_hue, &g_attr_color_control_current_saturation, &g_attr_color_control_remaining_time,
        &g_attr_color_control_current_x, &g_attr_color_control_current_y, &g_attr_color_control_color_temperature, &g_attr_color_control_color_mode,
        &g_attr_color_control_options, &g_attr_color_control_number_of_primaries, &g_attr_color_control_primary_1_x, &g_attr_color_control_primary_1_y,
        &g_attr_color_control_primary_1_intensity, &g_attr_color_control_primary_2_x, &g_attr_color_control_primary_2_y, &g_attr_color_control_primary_2_intensity,
        &g_attr_color_control_primary_3_x, &g_attr_color_control_primary_3_y, &g_attr_color_control_primary_3_intensity, &g_attr_color_control_primary_4_x,
        &g_attr_color_control_primary_4_y, &g_attr_color_control_primary_4_intensity, &g_attr_color_control_primary_5_x, &g_attr_color_control_primary_5_y,
        &g_attr_color_control_primary_5_intensity, &g_attr_color_control_primary_6_x, &g_attr_color_control_primary_6_y, &g_attr_color_control_primary_6_intensity,
        &g_attr_color_control_enhanced_current_hue, &g_attr_color_control_enhanced_color_mode, &g_attr_color_control_color_loop_active,
        &g_attr_color_control_color_loop_direction, &g_attr_color_control_color_loop_time, &g_attr_color_control_color_loop_start_enhanced_hue,
        &g_attr_color_control_color_loop_stored_enhanced_hue, &g_attr_color_control_color_capabilities, &g_attr_color_control_color_temp_physical_min_mireds,
        &g_attr_color_control_color_temp_physical_max_mireds, &g_attr_color_control_couple_color_temp_to_level_min_mireds, &g_attr_color_control_start_up_color_temperature_mireds);
/********************* Declare device **************************/
ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_CLUSTER_LIST(custom_dimmable_light_clusters, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list, on_off_attr_list, level_control_attr_list, color_control_attr_list);
ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_EP(custom_dimmable_light_ep, ENDPOINT_C, custom_dimmable_light_clusters);
ZB_HA_DECLARE_CUSTOM_DIMMABLE_LIGHT_CTX(device_ctx, custom_dimmable_light_ep);
zb_uint16_t g_dst_addr = 0;

#ifdef ZB_USE_NVRAM

void nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    application_dataset_t ds;
    zb_ret_t ret;
    ZB_ASSERT(payload_length == sizeof(ds));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));
    if (ret == RET_OK)
    {
        g_attr_color_control_color_temperature = ds.start_up_color_temp_mireds;
    }
}

zb_ret_t nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    application_dataset_t ds;
    ZB_MEMCPY(&ds, &g_dev_ctx, sizeof(application_dataset_t));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));
    return ret;
}

zb_uint16_t nvram_get_app_data_size(void)
{
    return sizeof(application_dataset_t);
}
#endif

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("sample_zc");

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(1l << 24);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_max_children(1);
    zb_set_pan_id(0x029a);

#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(nvram_read_app_data);
    zb_nvram_register_app1_write_cb(nvram_write_app_data, nvram_get_app_data_size);
#endif

    /* Register device list */

    ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(ZB_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);

#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(0, 0, button_press_handler);
#endif

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/* TODO: for AN - remove unused Scenes parts */

void send_view_scene_resp(zb_uint8_t param, zb_uint16_t idx)
{
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
        param,
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
        param,
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
            param,
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
            param,
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
        param,
        payload_ptr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).source.u.short_addr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).src_endpoint,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&resp_info.cmd_info).dst_endpoint,
        resp_info.cmd_info.profile_id,
        NULL);

    TRACE_MSG(TRACE_APP1, "<< send_get_scene_membership_resp", (FMT__0));
}

void test_device_interface_cb(zb_uint8_t param)
{

    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);

    TRACE_MSG(TRACE_APP1, "> test_device_interface_cb param %hd id %hd", (FMT__H_H,
              param, device_cb_param->device_cb_id));

    device_cb_param->status = RET_OK;

    switch (device_cb_param->device_cb_id)
    {
    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
        if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF &&
                device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
        {
            if (device_cb_param->cb_param.set_attr_value_param.values.data8)
            {
                TRACE_MSG(TRACE_APP1, "set ON", (FMT__0));
#ifdef ZB_USE_BUTTONS
                zb_osif_led_on(0);
#endif
            }
            else
            {
                TRACE_MSG(TRACE_APP1, "set OFF", (FMT__0));
#ifdef ZB_USE_BUTTONS
                zb_osif_led_off(0);
#endif
            }
        }
        else if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_COLOR_CONTROL &&
                 device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_COLOR_CONTROL_START_UP_COLOR_TEMPERATURE_MIREDS_ID)
        {
#ifdef ZB_USE_NVRAM
            if (device_cb_param->cb_param.set_attr_value_param.values.data16 != ZB_ZCL_COLOR_CONTROL_START_UP_COLOR_TEMPERATURE_USE_PREVIOUS_VALUE)
            {
                g_dev_ctx.start_up_color_temp_mireds = device_cb_param->cb_param.set_attr_value_param.values.data16;
            }
            else
            {
                zb_zcl_attr_t *attr_desc;
                attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                                   ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
                g_dev_ctx.start_up_color_temp_mireds =  ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc);
            }
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
        }
        break;

    /* >>>> Scenes */
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
                    scenes_table[idx].onoff_state = g_attr_on_off_on_off;
                    scenes_table[idx].current_x = g_attr_color_control_current_x;
                    scenes_table[idx].current_y = g_attr_color_control_current_y;
                    scenes_table[idx].enchanced_current_hue = g_attr_color_control_enhanced_current_hue;
                    scenes_table[idx].current_saturation = g_attr_color_control_current_saturation;
                    scenes_table[idx].color_loop_active = g_attr_color_control_color_loop_active;
                    scenes_table[idx].color_loop_direction = g_attr_color_control_color_loop_direction;
                    scenes_table[idx].color_loop_time = g_attr_color_control_color_loop_time;
                    scenes_table[idx].color_temperature_mireds = g_attr_color_control_color_temperature;
                    TRACE_MSG(TRACE_APP1, "update scene: entry idx %hd onoff_state %hd", (FMT__H_H, idx, scenes_table[idx].onoff_state));
                }
                else
                {
                    /* Create new entry with empty name and 0 transition time */
                    scenes_table[idx].common.group_id = store_scene_req->group_id;
                    scenes_table[idx].common.scene_id = store_scene_req->scene_id;
                    scenes_table[idx].common.transition_time = 0;
                    scenes_table[idx].onoff_state = g_attr_on_off_on_off;
                    scenes_table[idx].current_x = g_attr_color_control_current_x;
                    scenes_table[idx].current_y = g_attr_color_control_current_y;
                    scenes_table[idx].enchanced_current_hue = g_attr_color_control_enhanced_current_hue;
                    scenes_table[idx].current_saturation = g_attr_color_control_current_saturation;
                    scenes_table[idx].color_loop_active = g_attr_color_control_color_loop_active;
                    scenes_table[idx].color_loop_direction = g_attr_color_control_color_loop_direction;
                    scenes_table[idx].color_loop_time = g_attr_color_control_color_loop_time;
                    scenes_table[idx].color_temperature_mireds = g_attr_color_control_color_temperature;
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

        TRACE_MSG(TRACE_APP1, "ZB_ZCL_SCENES_REMOVE_SCENE_CB_ID: group_id 0x%x scene_id 0x%hd", (FMT__D_H, recall_scene_req->group_id, recall_scene_req->scene_id));

        idx = test_device_scenes_get_entry(recall_scene_req->group_id, recall_scene_req->scene_id);

        if (idx != 0xFF &&
                scenes_table[idx].common.group_id != ZB_ZCL_SCENES_FREE_SCENE_TABLE_RECORD)
        {
            zb_zcl_attr_t *attr_desc;

            /* Recall this entry */
            TRACE_MSG(TRACE_APP1, "recall scene: entry idx %hd onoff_state %hd transition_time 0x%x", (FMT__H_H_D, idx, scenes_table[idx].onoff_state, scenes_table[idx].common.transition_time));
            ZB_ZCL_SET_ATTRIBUTE(
                ENDPOINT_C,
                ZB_ZCL_CLUSTER_ID_ON_OFF,
                ZB_ZCL_CLUSTER_SERVER_ROLE,
                ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID,
                &scenes_table[idx].onoff_state,
                ZB_FALSE);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].current_x);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].current_y);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_ENHANCED_CURRENT_HUE_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].enchanced_current_hue);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_SATURATION_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, scenes_table[idx].current_saturation);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_ACTIVE_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, scenes_table[idx].color_loop_active);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_DIRECTION_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, scenes_table[idx].color_loop_direction);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_LOOP_TIME_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].color_loop_time);

            attr_desc = zb_zcl_get_attr_desc_a(ENDPOINT_C, ZB_ZCL_CLUSTER_ID_COLOR_CONTROL,
                                               ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_COLOR_CONTROL_COLOR_TEMPERATURE_ID);
            ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, scenes_table[idx].color_temperature_mireds);

            *recall_scene_status = ZB_ZCL_STATUS_SUCCESS;
        }
        else
        {
            *recall_scene_status = ZB_ZCL_STATUS_NOT_FOUND;
        }
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
        device_cb_param->status = RET_OK;
        break;
    }

    TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
    zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

    TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;

    ZB_ZCL_DEBUG_DUMP_HEADER(&cmd_info);
    TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(param)));

    zb_zdo_get_diag_data(g_dst_addr, &lqi, &rssi);
    TRACE_MSG(TRACE_APP3, "lqi %hd rssi %d", (FMT__H_H, lqi, rssi));

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Unsupported \"from server\" command direction",
            (FMT__0));
    }

    TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
    return ZB_FALSE;
}

void button_press_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "button is pressed, do nothing", (FMT__0));
}
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b target", (FMT__0));
            zb_bdb_finding_binding_target(ZB_OUTPUT_ENDPOINT);
            break;
        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal %d", (FMT__D, (zb_uint16_t)sig));
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

void test_device_scenes_table_init()
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
