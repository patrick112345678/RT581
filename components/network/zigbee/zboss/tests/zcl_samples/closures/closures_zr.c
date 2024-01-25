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
/* PURPOSE: Test sample with clusters Shade Configuration, Door Lock and
    Window Covering
 */

#define ZB_TRACE_FILE_ID 51362
#include "zboss_api.h"
#include "closures_zr.h"

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
void test_device_cb(zb_uint8_t param);

/**
 * Declaring attributes for each cluster
 */

zb_uint8_t g_attr_shade_config_status = ZB_ZCL_SHADE_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint16_t g_attr_shade_config_closed_limit = ZB_ZCL_SHADE_CONFIG_CLOSED_LIMIT_DEFAULT_VALUE;
zb_uint8_t g_attr_shade_config_mode = ZB_ZCL_SHADE_CONFIG_MODE_DEFAULT_VALUE;
/* Shade Configuration cluster attributes */
ZB_ZCL_DECLARE_SHADE_CONFIG_ATTRIB_LIST(shade_config_attr_list, &g_attr_shade_config_status, &g_attr_shade_config_closed_limit,
                                        &g_attr_shade_config_mode);

/* Door Lock cluster attributes */
zb_uint8_t g_attr_lock_state = 0;
zb_uint8_t g_attr_lock_type = 0;
zb_uint8_t g_attr_actuator_enabled = 0;
ZB_ZCL_DECLARE_DOOR_LOCK_ATTRIB_LIST(door_lock_attr_list, &g_attr_lock_state, &g_attr_lock_type, &g_attr_actuator_enabled);

/* Window Covering cluster attributes */
zb_uint8_t g_attr_window_covering_type = ZB_ZCL_WINDOW_COVERING_WINDOW_COVERING_TYPE_DEFAULT_VALUE;
zb_uint8_t g_attr_config_status = ZB_ZCL_WINDOW_COVERING_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint16_t g_attr_current_position_lift_percentage = ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_DEFAULT_VALUE;
zb_uint16_t g_attr_current_position_tilt_percentage = ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_open_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_closed_limit_lift = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_open_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_TILT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_closed_limit_tilt = ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_TILT_DEFAULT_VALUE;
zb_uint8_t g_attr_mode = ZB_ZCL_WINDOW_COVERING_MODE_DEFAULT_VALUE;
ZB_ZCL_DECLARE_WINDOW_COVERING_CLUSTER_ATTRIB_LIST(window_covering_attr_list, &g_attr_window_covering_type,
        &g_attr_config_status, &g_attr_current_position_lift_percentage,
        &g_attr_current_position_tilt_percentage, &g_attr_installed_open_limit_lift,
        &g_attr_installed_closed_limit_lift, &g_attr_installed_open_limit_tilt,
        &g_attr_installed_closed_limit_tilt, &g_attr_mode);

/* Declare cluster list for the device */
ZB_DECLARE_CLOSURES_SERVER_CLUSTER_LIST(closures_server_clusters, shade_config_attr_list, door_lock_attr_list,
                                        window_covering_attr_list);

/* Declare server endpoint */
ZB_DECLARE_CLOSURES_SERVER_EP(closures_server_ep, ZB_SERVER_ENDPOINT, closures_server_clusters);

ZB_DECLARE_CLOSURES_CLIENT_CLUSTER_LIST(closures_client_clusters);
/* Declare client endpoint */
ZB_DECLARE_CLOSURES_CLIENT_EP(closures_client_ep, ZB_CLIENT_ENDPOINT, closures_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_CLOSURES_CTX(closures_output_ctx, closures_server_ep, closures_client_ep);


MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("closures_zr");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_router_role(1l << 22);

    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_set_max_children(1);

    /* [af_register_device_context] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&closures_output_ctx);
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

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
            ZB_ZCL_DOOR_LOCK_SEND_LOCK_DOOR_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 1:
            ZB_ZCL_DOOR_LOCK_SEND_UNLOCK_DOOR_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                  ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 2:
            ZB_ZCL_WINDOW_COVERING_SEND_STOP_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                 ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 3:
            ZB_ZCL_WINDOW_COVERING_SEND_UP_OPEN_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 4:
            ZB_ZCL_WINDOW_COVERING_SEND_DOWN_CLOSE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
            break;

        case 5:
            ZB_ZCL_WINDOW_COVERING_SEND_GO_TO_LIFT_PERCENTAGE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                    ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    NULL, 0);
            break;

        case 6:
            ZB_ZCL_WINDOW_COVERING_SEND_GO_TO_TILT_PERCENTAGE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP,
                    ZB_CLIENT_ENDPOINT, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    NULL, 0);
            break;
        }

        test_step++;

        if (test_step <= 6)
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

void test_device_cb(zb_uint8_t param)
{
    zb_uint8_t lock_state;
    zb_bufid_t buffer = param;
    zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);

    TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H, param, device_cb_param->device_cb_id));

    switch (device_cb_param->device_cb_id)
    {
    /* Set Door Lock status to Locked */
    case ZB_ZCL_DOOR_LOCK_LOCK_DOOR_CB_ID:
        lock_state = 0x01; /* Locked */
        ZB_ZCL_SET_ATTRIBUTE(
            ZB_SERVER_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
            (zb_uint8_t *)&lock_state,
            ZB_FALSE /* do not check access */
        );
        device_cb_param->status = RET_OK;
        break;

    /* Set Door Lock status to Unlocked */
    case ZB_ZCL_DOOR_LOCK_UNLOCK_DOOR_CB_ID:
        lock_state = 0x02; /* Unlocked */
        ZB_ZCL_SET_ATTRIBUTE(
            ZB_SERVER_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
            (zb_uint8_t *)&lock_state,
            ZB_FALSE /* do not check access */
        );
        device_cb_param->status = RET_OK;
        break;

    default:
        TRACE_MSG(TRACE_APP1, "nothing to do, skip", (FMT__0));
        break;
    }
    TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}
