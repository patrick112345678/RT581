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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_OTA_CLI_03_DUT_ZR

#define ZB_TRACE_FILE_ID 40462
//#include "zboss_api.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "../common/zb_reg_test_globals.h"
#include "rtp_ota_cli_03_dut_zr.h"
#include "rtp_ota_cli_03_common.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error Define ZB_STACK_REGRESSION_TESTING_API
#endif

/* Handler for specific zcl commands */
static void test_device_cb(zb_uint8_t param);
static void test_ota_resume(zb_uint8_t param);

static zb_ieee_addr_t g_zr_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_ota_cli_03_dut_zr_basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* OTA Upgrade client cluster attributes data */
static zb_ieee_addr_t upgrade_server = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
static zb_uint32_t file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;

static zb_uint32_t file_version = OTA_UPGRADE_TEST_FILE_VERSION;   // custom data

static zb_uint16_t stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
static zb_uint32_t downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
static zb_uint16_t downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
static zb_uint8_t image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;

static zb_uint16_t manufacturer = OTA_UPGRADE_TEST_MANUFACTURER;   // custom data
static zb_uint16_t image_type = OTA_UPGRADE_TEST_IMAGE_TYPE;       // custom data

static zb_uint16_t min_block_reque = 0;
static zb_uint16_t image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
static zb_uint16_t server_addr;
static zb_uint8_t server_ep;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(rtp_ota_cli_03_dut_zr_ota_upgrade_attr_list,
                                       &upgrade_server, &file_offset, &file_version, &stack_version, &downloaded_file_ver,
                                       &downloaded_stack_ver, &image_status, &manufacturer, &image_type, &min_block_reque, &image_stamp,
                                       &server_addr, &server_ep, 0x0101, OTA_UPGRADE_TEST_DATA_SIZE, ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

/********************* Declare device **************************/

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST(rtp_ota_cli_03_dut_zr_ota_upgrade_client_clusters,
        rtp_ota_cli_03_dut_zr_basic_attr_list, rtp_ota_cli_03_dut_zr_ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_EP(rtp_ota_cli_03_dut_zr_ota_upgrade_client_ep, TEST_ENDPOINT_DUT, rtp_ota_cli_03_dut_zr_ota_upgrade_client_clusters);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CTX(rtp_ota_cli_03_dut_zr_ota_upgrade_client_ctx, rtp_ota_cli_03_dut_zr_ota_upgrade_client_ep);


/******************************************************************/

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zr");

    zb_set_long_address(g_zr_addr);
    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* need to generate jitter value equal to value from Image Notify Command */
    ZB_REGRESSION_TESTS_API().zcl_ota_custom_query_jitter = 1;

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&rtp_ota_cli_03_dut_zr_ota_upgrade_client_ctx);
    ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);

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

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %h", (FMT__H, param));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(2 * 1000));
            param = 0;
            break;

        case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
            TRACE_MSG(TRACE_APP1, "Production configuration block is ready", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}


static void test_device_cb(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
    TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd", (FMT__H_H,
              param, device_cb_param->device_cb_id));

    device_cb_param->status = RET_OK;
    switch (device_cb_param->device_cb_id)
    {
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
    {
        zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value = &(device_cb_param->cb_param.ota_value_param);

        switch (ota_upgrade_value->upgrade_status)
        {
        case ZB_ZCL_OTA_UPGRADE_STATUS_START:
            TRACE_MSG(TRACE_APP1, "test_device_cb(): ZB_ZCL_OTA_UPGRADE_STATUS_START", (FMT__0));
            /* Start OTA upgrade. */
            if (image_status == ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_NORMAL)
            {
                /* Accept image */
                ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
            }
            else
            {
                /* Another download is in progress, deny new image */
                ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
            }
            break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
            TRACE_MSG(TRACE_APP1, "test_device_cb(): ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE", (FMT__0));
            /* Process image block, wait for a while and resume ota process after some time */
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
            ZB_SCHEDULE_ALARM(test_ota_resume, param, TEST_DUT_WAIT_TIME);
            break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
            TRACE_MSG(TRACE_APP1, "test_device_cb(): ZB_ZCL_OTA_UPGRADE_STATUS_CHECK", (FMT__0));
            /* Downloading is finished, do additional checks if needed etc before Upgrade End Request. */
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
            break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
            TRACE_MSG(TRACE_APP1, "test_device_cb(): ZB_ZCL_OTA_UPGRADE_STATUS_APPLY", (FMT__0));
            /* Upgrade End Resp is ok, ZCL checks for manufacturer, image type etc are ok.
               Last step before actual upgrade. */
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
            break;
        case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
            TRACE_MSG(TRACE_APP1, "test_device_cb(): ZB_ZCL_OTA_UPGRADE_STATUS_FINISH", (FMT__0));
            /* It is time to upgrade FW. */
            break;
        }
    }
    break;

    default:
        device_cb_param->status = RET_ERROR;
        break;
    }
    TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}

static void test_ota_resume(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">>test_ota_resume %h", (FMT__H, param));

    zb_zcl_ota_upgrade_resume_client(param, ZB_ZCL_OTA_UPGRADE_STATUS_OK);

    TRACE_MSG(TRACE_APP1, "<<test_ota_resume", (FMT__0));
}
