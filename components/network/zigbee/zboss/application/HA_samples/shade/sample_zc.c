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

#define ZB_TRACE_FILE_ID 40208
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_shade.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */

#include "../common/zcl_basic_attr_list.h"

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

/* [COMMON_DECLARATION] */
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
/* Level Control cluster attributes data */
zb_uint8_t g_attr_level_control_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_attr_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
/* Shade Configuration cluster attributes data */
zb_uint8_t g_attr_shade_config_status = ZB_ZCL_SHADE_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint16_t g_attr_shade_config_closed_limit = ZB_ZCL_SHADE_CONFIG_CLOSED_LIMIT_DEFAULT_VALUE;
zb_uint8_t g_attr_shade_config_mode = ZB_ZCL_SHADE_CONFIG_MODE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
ZB_ZCL_DECLARE_SHADE_CONFIG_ATTRIB_LIST(shade_config_attr_list, &g_attr_shade_config_status, &g_attr_shade_config_closed_limit, &g_attr_shade_config_mode);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_attr_on_off_on_off, &g_attr_on_off_global_scene_control, &g_attr_on_off_on_time, &g_attr_on_off_off_wait_time);
ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list, &g_attr_level_control_current_level, &g_attr_level_control_remaining_time);
/********************* Declare device **************************/
ZB_HA_DECLARE_SHADE_CLUSTER_LIST(shade_clusters, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list, shade_config_attr_list, on_off_attr_list, level_control_attr_list);
ZB_HA_DECLARE_SHADE_EP(shade_ep, ENDPOINT_C, shade_clusters);
ZB_HA_DECLARE_SHADE_CTX(device_ctx, shade_ep);
/* [COMMON_DECLARATION] */
zb_uint16_t g_dst_addr = 0;
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

    /* [REGISTER] */
    /* Register device list */
    ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(ZB_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);
    /* [REGISTER] */

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

/* [ZCL_COMMAND_HANDLER] */
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
        break;

    default:
        device_cb_param->status = RET_OK;
        break;
    }

    TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
/* [ZCL_COMMAND_HANDLER] */

/* [COMMAND_HANDLER] */
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
/* [COMMAND_HANDLER] */

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
