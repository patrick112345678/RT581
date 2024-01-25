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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 40186
#include "zboss_api.h"

#include "ota_upgrade_server.h"
#include "ota_upgrade_test_common.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
zb_uint8_t g_key[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

#define ENDPOINT  5

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* OTA Upgrade server cluster attributes data */
//zb_uint8_t query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_MAX_VALUE;
/* Speedup upoad for debug */
zb_uint8_t query_jitter = 1;
zb_uint32_t current_time = OTA_UPGRADE_TEST_CURRENT_TIME;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(ota_upgrade_attr_list, &query_jitter, &current_time, 1);

/********************* Declare device **************************/

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_CLUSTER_LIST(ota_upgrade_server_clusters,
        basic_attr_list, ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_EP(ota_upgrade_server_ep, ENDPOINT, ota_upgrade_server_clusters);

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_CTX(ota_upgrade_server_ctx, ota_upgrade_server_ep);

/******************* Declare test data & constants *************/

void *flashdev;

void insert_ota_file(zb_uint8_t param);

/* This cb is called on next image block request
   Parameters:
   - index - file indexes
   - offset - current offset for the file
   - size - block size
*/
zb_uint8_t *next_data_ota_data_ask_cb(zb_uint8_t index, zb_uint32_t offset, zb_uint8_t size)
{
    TRACE_MSG(TRACE_ZCL1, "next_data_ota_data_ask_cb %hd %d %hd",
              (FMT__H_D_H, index, offset, size));
    ZVUNUSED(index);

    return zb_osif_ota_srv_get_image(flashdev, offset, size);
}

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("zc");

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(ZB_DEFAULT_APS_CHANNEL_MASK);
    zb_set_nvram_erase_at_start(ZB_FALSE);
    zb_secur_setup_nwk_key(g_key, 0);

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&ota_upgrade_server_ctx);
    ZB_ZCL_CLUSTER_ID_OTA_UPGRADE_SERVER_ROLE_INIT();
    zb_zcl_ota_upgrade_init_server(ENDPOINT, next_data_ota_data_ask_cb);
    /* switch ON Flash B */
    flashdev = zb_osif_ota_open_storage();

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

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %h", (FMT__H, param));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            ZB_SCHEDULE_APP_ALARM(insert_ota_file, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(20 * 1000));
            param = 0;
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
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
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}


void insert_ota_file(zb_uint8_t param)
{
    zb_buf_t *buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    /* zb_uint8_t *ota_file = zb_osif_ota_srv_get_image(0); */

    TRACE_MSG(TRACE_ZCL1, "> insert_ota_file %hd", (FMT__H, param));
    /*
      According to 6.3.2.7 File Version in OTA Cluster specification

       file version consists of (high to low):
       - Application release 1b
       - Application build 1b
       - stack release 1b - (ZBOSS_MAJOR << 4) | ZBOSS_MINOR
       - stack build 1b - ZBOSS_SDK_REVISION

       File version is set in OTA image header (on image generation).
       Note that a binary-coded decimal convention (BCD) concept is used here for version number.

       Note: Wireshark uses wrong bytes order there.

    */
    /* We have single file, #0 only. */
    {
        zb_ret_t ret;

        ZB_ZCL_OTA_UPGRADE_INSERT_FILE(buf, ENDPOINT, 0, zb_osif_ota_srv_get_image_header(flashdev), OTA_UPGRADE_TEST_UPGRADE_TIME, ZB_TRUE, ret);
        ZB_ASSERT(ret == RET_OK);
    }
    TRACE_MSG(TRACE_ZCL1, "< insert_ota_file", (FMT__0));
}
