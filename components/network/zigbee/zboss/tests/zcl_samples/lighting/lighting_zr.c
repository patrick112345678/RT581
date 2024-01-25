/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: Test sample with cluster Color Control */

#define ZB_TRACE_FILE_ID 51361
#include "zboss_api.h"
#include "lighting_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5

/**
 * Global variables definitions
 */
zb_ieee_addr_t g_zc_addr = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}; /* IEEE address of the
                                                                              * device */
/* Used endpoint */
#define ZB_SERVER_ENDPOINT          5
#define ZB_CLIENT_ENDPOINT          6

void test_loop(zb_bufid_t param);

/**
 * Declaring attributes for each cluster
 */

/* Color Control cluster attributes */
zb_uint8_t g_attr_current_hue = ZB_ZCL_COLOR_CONTROL_CURRENT_HUE_DEFAULT_VALUE;
zb_uint8_t g_attr_current_saturation = ZB_ZCL_COLOR_CONTROL_CURRENT_SATURATION_DEFAULT_VALUE;
zb_uint16_t g_attr_remaining_time = ZB_ZCL_COLOR_CONTROL_REMAINING_TIME_DEFAULT_VALUE;
zb_uint16_t g_attr_current_X = ZB_ZCL_COLOR_CONTROL_CURRENT_X_DEF_VALUE;
zb_uint16_t g_attr_current_Y = ZB_ZCL_COLOR_CONTROL_CURRENT_Y_DEF_VALUE;
zb_uint16_t g_attr_color_temperature = ZB_ZCL_COLOR_CONTROL_COLOR_TEMPERATURE_DEF_VALUE;
zb_uint8_t g_attr_color_mode = ZB_ZCL_COLOR_CONTROL_COLOR_MODE_DEFAULT_VALUE;
zb_uint8_t g_attr_options = ZB_ZCL_COLOR_CONTROL_OPTIONS_DEFAULT_VALUE;
zb_uint16_t g_attr_enhanced_current_hue = ZB_ZCL_COLOR_CONTROL_ENHANCED_CURRENT_HUE_DEFAULT_VALUE;
zb_uint8_t g_attr_enhanced_color_mode = ZB_ZCL_COLOR_CONTROL_ENHANCED_COLOR_MODE_DEFAULT_VALUE;
zb_uint16_t g_attr_color_loop_active = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_ACTIVE_DEFAULT_VALUE;
zb_uint16_t g_attr_color_loop_direction = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_DIRECTION_DEFAULT_VALUE;
zb_uint16_t g_attr_color_loop_time = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_TIME_DEF_VALUE;
zb_uint16_t g_attr_color_loop_start = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_START_DEF_VALUE;
zb_uint16_t g_attr_color_loop_stored = ZB_ZCL_COLOR_CONTROL_COLOR_LOOP_STORED_ENHANCED_HUE_DEFAULT_VALUE;
zb_uint16_t g_attr_color_capabilities = ZB_ZCL_COLOR_CONTROL_COLOR_CAPABILITIES_DEFAULT_VALUE;
zb_uint16_t g_attr_color_temp_physical_min = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MIN_MIREDS_DEFAULT_VALUE;
zb_uint16_t g_attr_color_temp_physical_max = ZB_ZCL_COLOR_CONTROL_COLOR_TEMP_PHYSICAL_MAX_MIREDS_DEFAULT_VALUE;
zb_uint16_t g_attr_couple_color_temp_to_level_min = 0; /* Missing default value */
zb_uint16_t g_attr_start_up_color_temp = 0; /* Missing default value */
/* Primaries Set */
zb_uint8_t g_attr_number_primaries = 0; /* Missing default value */
zb_uint16_t g_attr_primary_1_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_1_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_1_intensity = 0; /* Missing default value */
zb_uint16_t g_attr_primary_2_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_2_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_2_intensity = 0; /* Missing default value */
zb_uint16_t g_attr_primary_3_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_3_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_3_intensity = 0; /* Missing default value */
zb_uint16_t g_attr_primary_4_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_4_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_4_intensity = 0; /* Missing default value */
zb_uint16_t g_attr_primary_5_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_5_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_5_intensity = 0; /* Missing default value */
zb_uint16_t g_attr_primary_6_X = 0; /* Missing default value */
zb_uint16_t g_attr_primary_6_Y = 0; /* Missing default value */
zb_uint8_t g_attr_primary_6_intensity = 0; /* Missing default value */

ZB_ZCL_DECLARE_COLOR_CONTROL_ATTRIB_LIST_EXT(color_control_attr_list, &g_attr_current_hue, &g_attr_current_saturation,
        &g_attr_remaining_time, &g_attr_current_X, &g_attr_current_Y, &g_attr_color_temperature,
        &g_attr_color_mode, &g_attr_options, &g_attr_number_primaries, &g_attr_primary_1_X,
        &g_attr_primary_1_Y, &g_attr_primary_1_intensity, &g_attr_primary_2_X,
        &g_attr_primary_2_Y, &g_attr_primary_2_intensity, &g_attr_primary_3_X, &g_attr_primary_3_Y,
        &g_attr_primary_3_intensity, &g_attr_primary_4_X, &g_attr_primary_4_Y, &g_attr_primary_4_intensity,
        &g_attr_primary_5_X, &g_attr_primary_5_Y, &g_attr_primary_5_intensity, &g_attr_primary_6_X,
        &g_attr_primary_6_Y, &g_attr_primary_6_intensity, &g_attr_enhanced_current_hue,
        &g_attr_enhanced_color_mode, &g_attr_color_loop_active, &g_attr_color_loop_direction,
        &g_attr_color_loop_time, &g_attr_color_loop_start, &g_attr_color_loop_stored,
        &g_attr_color_capabilities, &g_attr_color_temp_physical_min, &g_attr_color_temp_physical_max,
        &g_attr_couple_color_temp_to_level_min, &g_attr_start_up_color_temp);


/* Declare cluster list for the device */
ZB_DECLARE_LIGHTING_SERVER_CLUSTER_LIST(lighting_server_clusters, color_control_attr_list);

/* Declare server endpoint */
ZB_DECLARE_LIGHTING_SERVER_EP(lighting_server_ep, ZB_SERVER_ENDPOINT, lighting_server_clusters);

ZB_DECLARE_LIGHTING_CLIENT_CLUSTER_LIST(lighting_client_clusters);
/* Declare client endpoint */
ZB_DECLARE_LIGHTING_CLIENT_EP(lighting_client_ep, ZB_CLIENT_ENDPOINT, lighting_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_LIGHTING_CTX(lighting_output_ctx, lighting_server_ep, lighting_client_ep);


MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("lighting_zr");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_router_role(1l << 22);

    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_set_max_children(1);

    /* [af_register_device_context] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&lighting_output_ctx);

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

void test_loop(zb_bufid_t param)
{
    static zb_uint8_t test_step = 0;

    TRACE_MSG(TRACE_APP1, ">> test_loop test_step=%hd", (FMT__H, test_step));

    if (param == 0)
    {
        zb_buf_get_out_delayed(test_loop);
    }
    else
    {
        switch (test_step)
        {
        case 0:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                   ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                   ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0);
            break;

        case 1:
            ZB_ZCL_COLOR_CONTROL_SEND_STEP_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                   ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                   ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0, 0);
            break;

        case 2:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_SATURATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0);
            break;

        case 3:
            ZB_ZCL_COLOR_CONTROL_SEND_STEP_SATURATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0, 0);
            break;

        case 4:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
            break;

        case 5:
            ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0, 0);
            break;

        case 6:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0,
                    ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_UP, 0);
            break;

        case 7:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_SATURATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
            break;

        case 8:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0, 0);
            break;

        case 9:
            ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP, 0);
            break;

        case 10:
            ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_STEP_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0, 0);
            break;

        case 11:
            ZB_ZCL_COLOR_CONTROL_SEND_COLOR_LOOP_SET_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_LOOP_UPDATE_ACTION,
                    ZB_ZCL_CMD_COLOR_CONTROL_LOOP_ACTION_DEACTIVATE,
                    ZB_ZCL_CMD_COLOR_CONTROL_LOOP_DIRECTION_INCREMENT, 0, 0);
            break;

        case 12:
            ZB_ZCL_COLOR_CONTROL_SEND_STOP_MOVE_STEP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 13:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_COLOR_TEMP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_STOP,
                    0, 0, 0);
            break;

        case 14:
            ZB_ZCL_COLOR_CONTROL_SEND_STEP_COLOR_TEMP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_CMD_COLOR_CONTROL_STEP_UP, 0, 0, 0, 0);
            break;

        case 15:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_HUE_SATURATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0, 0);
            break;

        case 16:
            ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_TEMPERATURE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0);
            break;

        case 17:
            ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, ZB_ZCL_CMD_COLOR_CONTROL_MOVE_TO_HUE_UP,
                    0);
            break;

        case 18:
            ZB_ZCL_COLOR_CONTROL_SEND_ENHANCED_MOVE_TO_HUE_SATURATION_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0, 0);
            break;
        }
        test_step++;

        if (test_step <= 18)
        {
            ZB_SCHEDULE_APP_ALARM(test_loop, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
        }
    }

    TRACE_MSG(TRACE_APP1, "<< test_loop", (FMT__0));
}

/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_SKIP_STARTUP:
            zboss_start_continue();
            break;

        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(test_loop, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(50));
            /* Avoid freeing param */
            param = 0;
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
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d sig %d", (FMT__D_D, ZB_GET_APP_SIGNAL_STATUS(param), sig));
    }

    /* Free the buffer if it is not used */
    if (param)
    {
        zb_buf_free(param);
    }
}
