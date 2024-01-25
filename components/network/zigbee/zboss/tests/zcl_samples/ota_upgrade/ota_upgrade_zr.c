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
/* PURPOSE: Test sample with clusters OTA Upgrade */

#define ZB_TRACE_FILE_ID 51369
#include "zboss_api.h"
#include "ota_upgrade_zr.h"

/* Insert that include before any code or declaration. */
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif

zb_uint16_t g_dst_addr = 0x00;

#define DST_ADDR g_dst_addr
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT
#define DST_EP 5
#define TUNNEL_DATA_SIZE 3

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

/* OTA Server cluster attributes */
zb_uint8_t g_attr_query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_DEF_VALUE;
zb_uint32_t g_attr_current_time = ZB_ZCL_OTA_UPGRADE_CURRENT_TIME_DEF_VALUE;
ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(ota_server_attr_list, &g_attr_query_jitter,
        &g_attr_current_time, 1);

/* OTA Client cluster attributes */
zb_ieee_addr_t g_attr_ota_upgrade_server = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
zb_uint32_t g_attr_ota_file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
zb_uint32_t g_attr_ota_file_version = 0;
zb_uint16_t g_attr_ota_stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
zb_uint32_t g_attr_ota_downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
zb_uint16_t g_attr_ota_downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
zb_uint8_t g_attr_ota_image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
zb_uint16_t g_attr_ota_manufacturer = 0;
zb_uint16_t g_attr_ota_image_type = 0;
zb_uint16_t g_attr_ota_min_block_reque = 0;
zb_uint16_t g_attr_ota_image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
zb_uint16_t g_attr_ota_server_addr;
zb_uint8_t g_attr_ota_server_ep = ZB_SERVER_ENDPOINT;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_client_attr_list, &g_attr_ota_upgrade_server,
                                       &g_attr_ota_file_offset, &g_attr_ota_file_version, &g_attr_ota_stack_version,
                                       &g_attr_ota_downloaded_file_ver, &g_attr_ota_downloaded_stack_ver, &g_attr_ota_image_status,
                                       &g_attr_ota_manufacturer, &g_attr_ota_image_type, &g_attr_ota_min_block_reque,
                                       &g_attr_ota_image_stamp, &g_attr_ota_server_addr, &g_attr_ota_server_ep, 0,
                                       0, ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);


/* Declare cluster list for the device */
ZB_DECLARE_OTA_UPGRADE_SERVER_CLUSTER_LIST(ota_upgrade_server_clusters, ota_server_attr_list);

/* Declare server endpoint */
ZB_DECLARE_OTA_UPGRADE_SERVER_EP(ota_upgrade_server_ep, ZB_SERVER_ENDPOINT, ota_upgrade_server_clusters);

ZB_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST(ota_upgrade_client_clusters, ota_client_attr_list);

/* Declare client endpoint */
ZB_DECLARE_OTA_UPGRADE_CLIENT_EP(ota_upgrade_client_ep, ZB_CLIENT_ENDPOINT, ota_upgrade_client_clusters);

/* Declare application's device context for single-endpoint device */
ZB_DECLARE_OTA_UPGRADE_CTX(ota_upgrade_output_ctx, ota_upgrade_server_ep, ota_upgrade_client_ep);


MAIN()
{
    ARGV_UNUSED;

    /* Trace enable */
    ZB_SET_TRACE_ON();
    /* Traffic dump enable*/
    ZB_SET_TRAF_DUMP_ON();

    /* Global ZBOSS initialization */
    ZB_INIT("ota_upgrade_zr");

    /* Set up defaults for the commissioning */
    zb_set_long_address(g_zc_addr);
    zb_set_network_router_role(1l << 22);

    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_set_max_children(1);

    /* [af_register_device_context] */
    /* Register device ZCL context */
    ZB_AF_REGISTER_DEVICE_CTX(&ota_upgrade_output_ctx);

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
            ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_BLOCK_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                    ZB_ZCL_OTA_UPGRADE_QUERY_IMAGE_BLOCK_IEEE_PRESENT, 0, 0,
                                                    0, 0, 0, 0, 0, 0);
            break;

        case 1:
            ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_PAGE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                   ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                   ZB_ZCL_OTA_UPGRADE_QUERY_IMAGE_BLOCK_IEEE_PRESENT, 0, 0, 0,
                                                   0, 0, 0, 0, 0);
            break;

        case 2:
            ZB_ZCL_OTA_UPGRADE_SEND_UPGRADE_END_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                                                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                                                    ZB_ZCL_STATUS_SUCCESS, 0, 0, 0);
            break;

        case 3:
            ZB_ZCL_OTA_UPGRADE_SEND_IMAGE_NOTIFY_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_OTA_UPGRADE_IMAGE_NOTIFY_PAYLOAD_JITTER_CODE_IMAGE_VER,
                    0, 0, 0, 0);
            break;

        case 4:
            ZB_ZCL_OTA_UPGRADE_SEND_QUERY_NEXT_IMAGE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL,
                    ZB_ZCL_OTA_UPGRADE_QUERY_IMAGE_BLOCK_IEEE_PRESENT, 0, 0, 0, 0,
                    0);
            break;

        case 5:
            ZB_ZCL_OTA_UPGRADE_SEND_QUERY_SPECIFIC_FILE_REQ(param, DST_ADDR, DST_ADDR_MODE, DST_EP, ZB_CLIENT_ENDPOINT,
                    ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, 0, 0,
                    0, 0, 0);
            break;

        }

        test_step++;

        if (test_step <= 5)
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
