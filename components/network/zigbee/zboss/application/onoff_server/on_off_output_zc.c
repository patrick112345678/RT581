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
/* ![mem_config_max] */
#define ZB_TRACE_FILE_ID 40248
#include "zboss_api.h"
#include "zb_led_button.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* Next define clusters, attributes etc. */
/* ![mem_config_max] */

/*
#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif
*/
#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to build led bulb demo
#endif

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa}; /* IEEE address of the
                                                                              * device */
zb_uint16_t g_dst_addr;
zb_uint8_t g_addr_mode;
zb_uint8_t g_endpoint;

/* Uncomment to enable installcode usage */
/* #define TEST_USE_INSTALLCODE */

#ifdef TEST_USE_INSTALLCODE
/* IEEE address of the end device */
zb_ieee_addr_t g_ed_addr = {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};
char g_installcode[] = "966b9f3ef98ae605 9708";
#endif

/* Used endpoint */
#define ZB_OUTPUT_ENDPOINT          5
#define ZB_OUTPUT_MAX_CMD_PAYLOAD_SIZE 2

/* Handler for specific ZCL commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

void test_device_interface_cb(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);

/**
 * Declaring attributes for each cluster
 */
/* On/Off cluster attributes */
zb_uint8_t g_attr_on_off = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
#ifdef ZB_ENABLE_ZLL
/* On/Off cluster attributes additions */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_attr_on_off,
                                      &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);
#else
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_attr_on_off);
#endif

/* Basic cluster attributes */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

#ifdef ZB_ZCL_SUPPORT_CLUSTER_SCENES
/* Scenes cluster attributes */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_name_support = ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
                                  &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
                                  &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);
#else
zb_zcl_attr_t scenes_attr_list[] = { ZB_ZCL_NULL_ID, 0, 0, NULL };
#endif

/* Declare cluster list for the device */
ZB_HA_DECLARE_ON_OFF_OUTPUT_CLUSTER_LIST(on_off_output_clusters,
        on_off_attr_list,
        basic_attr_list,
        identify_attr_list,
        groups_attr_list,
        scenes_attr_list);

/* Declare endpoint */
ZB_HA_DECLARE_ON_OFF_OUTPUT_EP(on_off_output_ep, ZB_OUTPUT_ENDPOINT, on_off_output_clusters);

/* Declare application's device context for single-endpoint device */
ZB_HA_DECLARE_ON_OFF_OUTPUT_CTX(on_off_output_ctx, on_off_output_ep);



MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("on_off_output_zc");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
#ifndef ZB_APP_ENABLE_SUBGHZ_MODE
    {
        zb_set_network_coordinator_role(1l << 21);
    }
#else
    {
        zb_channel_list_t channel_list;
        zb_channel_list_init(channel_list);
        zb_channel_page_list_set_mask(channel_list, (ZB_CHANNEL_LIST_PAGE28_IDX), (1l << 24));
        zb_set_network_coordinator_role_ext(channel_list);
    }
#endif
    zb_set_nvram_erase_at_start(ZB_FALSE);

    zb_set_max_children(1);

    /* [af_register_device_context] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&on_off_output_ctx);
    /* [af_register_device_context] */

    /* Register cluster commands handler for a specific endpoint */
    ZB_AF_SET_ENDPOINT_HANDLER(ZB_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);

    /* [zcl_register_device_cb] */
    /* Set Device user application callback */
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_interface_cb);
    /* [zcl_register_device_cb] */

#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(0, 0, button_press_handler);
#endif

    /* Initiate the stack start without starting the commissioning */
    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        /* Call the main loop */
        zboss_main_loop();
    }

    /* Deinitialize trace */
    TRACE_DEINIT();

    MAIN_RETURN(0);
}

/* [zcl_register_device_cb_example_cb] */
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
        device_cb_param->status = RET_ERROR;
        break;
    }

    TRACE_MSG(TRACE_APP1, "< test_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
/* [zcl_register_device_cb_example_cb] */

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_uint8_t lqi = ZB_MAC_LQI_UNDEFINED;
    zb_int8_t rssi = ZB_MAC_RSSI_UNDEFINED;

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;
    g_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint;
    g_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    ZB_ZCL_DEBUG_DUMP_HEADER(&cmd_info);
    TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, zb_buf_len(param)));

    zb_zdo_get_diag_data(g_dst_addr, &lqi, &rssi);
    TRACE_MSG(TRACE_ZCL3, "lqi %hd rssi %d", (FMT__H_H, lqi, rssi));

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Unsupported \"from server\" command direction",
            (FMT__0));
    }

    TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
    return ZB_FALSE;
}

void button_press_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "button is pressed, do nothing", (FMT__0));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
#ifndef ZB_MACSPLIT_HOST
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#else
        case ZB_MACSPLIT_DEVICE_BOOT:
#endif /* ZB_MACSPLIT_HOST */

#ifdef TEST_USE_INSTALLCODE
            zb_secur_ic_str_add(g_ed_addr, g_installcode, NULL);
#endif
            zboss_start_continue();
            break;

        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        /* [zb_bdb_finding_binding_target_usage] */
        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APP1, "Successfull steering, start f&b target", (FMT__0));
            zb_bdb_finding_binding_target(ZB_OUTPUT_ENDPOINT);
            break;
        /* [zb_bdb_finding_binding_target_usage] */
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
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d sig %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), sig));
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}
