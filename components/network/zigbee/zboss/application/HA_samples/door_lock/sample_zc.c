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

#define ZB_TRACE_FILE_ID 40214
#include "zboss_api.h"
#include "zb_led_button.h"
#include "zb_door_lock.h"

#ifdef ZTT_USER_INTERACTION
/* nordic dependent headers removed */
#endif

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
#define BUTTON_1 ZB_BOARD_BUTTON_1

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
/* Door Lock cluster attributes data */
zb_uint8_t g_attr_door_lock_lock_state = 0;
zb_uint8_t g_attr_door_lock_lock_type = 0;
zb_bool_t g_attr_door_lock_actuator_enabled = ZB_FALSE;
ZB_ZCL_DECLARE_DOOR_LOCK_ATTRIB_LIST(door_lock_attr_list, &g_attr_door_lock_lock_state, &g_attr_door_lock_lock_type, &g_attr_door_lock_actuator_enabled);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_groups_name_support);
ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count, &g_attr_scenes_current_scene, &g_attr_scenes_current_group, &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
/********************* Declare device **************************/
ZB_HA_DECLARE_DOOR_LOCK_CLUSTER_LIST(door_lock_clusters, door_lock_attr_list, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list);
ZB_HA_DECLARE_DOOR_LOCK_EP(door_lock_ep, ENDPOINT_C, door_lock_clusters);
ZB_HA_DECLARE_DOOR_LOCK_CTX(device_ctx, door_lock_ep);
/* [COMMON_DECLARATION] */
zb_uint16_t g_dst_addr = 0;

#ifdef ZTT_USER_INTERACTION
void button_2_handler(zb_uint8_t param)
{
    ZVUNUSED(param);

    g_attr_door_lock_lock_state ^= 0x01;
    zb_uint8_t lock_state = g_attr_door_lock_lock_state;
    ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                 ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                 ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
                                 &lock_state,
                                 ZB_FALSE));
}

static void gpio_init(void)
{
    zb_osif_led_button_init();
    //APP_ERROR_CHECK(err_code);
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(BUTTON_1, 0, button_2_handler);
#endif
    //APP_ERROR_CHECK(err_code);
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

    /* [REGISTER] */
    /* Register device list */
    ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);
    /* [REGISTER] */

#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(0, 0, button_press_handler);
#endif

#ifdef ZTT_USER_INTERACTION
    gpio_init();
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
    case ZB_ZCL_DOOR_LOCK_LOCK_DOOR_CB_ID:
    {
        zb_uint8_t lock_state = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED;

        TRACE_MSG(TRACE_APP1, "Lock the door", (FMT__0));

        ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                     ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
                                     &lock_state,
                                     ZB_FALSE));
    }
    break;

    case ZB_ZCL_DOOR_LOCK_UNLOCK_DOOR_CB_ID:
    {
        zb_uint8_t lock_state = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED;

        TRACE_MSG(TRACE_APP1, "Unlock the door", (FMT__0));

        ZVUNUSED(zb_zcl_set_attr_val(ENDPOINT_C,
                                     ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE,
                                     ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
                                     &lock_state,
                                     ZB_FALSE));
    }
    break;

    default:
        device_cb_param->status = RET_OK;
        break;
    }

    TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
/* [ZCL_COMMAND_HANDLER] */

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
