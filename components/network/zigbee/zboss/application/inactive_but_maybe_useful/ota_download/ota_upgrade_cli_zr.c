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

#define ZB_TRACE_FILE_ID 40185
#include "zboss_api.h"

#include "ota_upgrade_client.h"
#include "ota_upgrade_test_common.h"

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif

/* Handler for specific zcl commands */
void test_device_cb(zb_uint8_t param);

zb_ieee_addr_t g_zr_addr = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

#define ENDPOINT  10

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* OTA Upgrade client cluster attributes data */
zb_ieee_addr_t upgrade_server = ZB_ZCL_OTA_UPGRADE_SERVER_DEF_VALUE;
zb_uint32_t file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;


#define TEST_CUR_FW_VERSION 0x01010101
zb_uint32_t file_version = TEST_CUR_FW_VERSION;

zb_uint16_t stack_version = ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO;
zb_uint32_t downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
zb_uint16_t downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
zb_uint8_t image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;

zb_uint16_t manufacturer = OTA_UPGRADE_TEST_MANUFACTURER;   // custom data
zb_uint16_t image_type = OTA_UPGRADE_TEST_IMAGE_TYPE;       // custom data

zb_uint16_t min_block_reque = 10;
zb_uint16_t image_stamp = ZB_ZCL_OTA_UPGRADE_IMAGE_STAMP_MIN_VALUE;
zb_uint16_t server_addr;
zb_uint8_t server_ep;

#define OTA_UPGRADE_TEST_HW_VERSION 123

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
                                       &upgrade_server, &file_offset, &file_version, &stack_version, &downloaded_file_ver,
                                       &downloaded_stack_ver, &image_status, &manufacturer, &image_type, &min_block_reque, &image_stamp,
                                       &server_addr, &server_ep, OTA_UPGRADE_TEST_HW_VERSION, OTA_UPGRADE_TEST_DATA_SIZE, ZB_ZCL_OTA_UPGRADE_QUERY_TIMER_COUNT_DEF);

/********************* Declare device **************************/

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CLUSTER_LIST( ota_upgrade_client_clusters,
        basic_attr_list, ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_EP(ota_upgrade_client_ep, ENDPOINT, ota_upgrade_client_clusters);

ZB_HA_DECLARE_OTA_UPGRADE_CLIENT_CTX(ota_upgrade_client_ctx, ota_upgrade_client_ep);


/******************************************************************/

typedef struct zb_ota_upgrade_ctx_s
{
    zb_uint32_t image_size;
    zb_uint32_t fw_version;
    void        *flash_dev;
    zb_uint32_t reboot_counter;
    zb_uint8_t param;
} zb_ota_upgrade_ctx_t;

zb_ota_upgrade_ctx_t ota_ctx;

typedef struct ncs_device_nvram_dataset_s
{
    zb_uint32_t reboot_counter;
} ncs_device_nvram_dataset_t;


static void zcl_device_cb(zb_uint8_t param);
static void process_ota_upgrade(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_OFF();

    ZB_INIT("zr");

    zb_set_long_address(g_zr_addr);
    zb_set_network_router_role(ZB_DEFAULT_APS_CHANNEL_MASK);
    zb_set_nvram_erase_at_start(ZB_FALSE);

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&ota_upgrade_client_ctx);
    ZB_ZCL_CLUSTER_ID_OTA_UPGRADE_CLIENT_ROLE_INIT();
    zb_register_zboss_callback(ZB_ZCL_DEVICE_CB, SET_ZBOSS_CB(zcl_device_cb));

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
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Rebooted-rejoined. Stop.", (FMT__0));
            break;
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
            param = 0;
            break;

        case ZB_ZDO_SIGNAL_LEAVE:
            TRACE_MSG(TRACE_APP1, "Left the network - stop OTA!", (FMT__0));
            zb_zcl_ota_upgrade_stop_client();
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}


/**
   Callback handling device-specific actions (lighting LEDs etc).

   Here handle OTA only.
 */
static void zcl_device_cb(zb_uint8_t param)
{
    zb_buf_t *buffer = ZB_BUF_FROM_REF(param);
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_GET_BUF_PARAM(buffer, zb_zcl_device_callback_param_t);

    /* TRACE_MSG(TRACE_APP1, "> zcl_device_cb param %hd id %hd", (FMT__H_H, */
    /*     param, device_cb_param->device_cb_id)); */

    device_cb_param->status = RET_OK;
    switch (device_cb_param->device_cb_id)
    {
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
        process_ota_upgrade(param);
        break;

    default:
        TRACE_MSG(TRACE_APP1, "Unsupported cb id", (FMT__0));
        device_cb_param->status = RET_ERROR;
        break;
    }
    /* TRACE_MSG(TRACE_APP1, "< zcl_device_cb %hd", (FMT__H, device_cb_param->status)); */
}


static void process_ota_upgrade(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zcl_device_callback_param_t);
    zb_zcl_ota_upgrade_value_param_t *ota_upgrade_value = &(device_cb_param->cb_param.ota_value_param);

    /* TRACE_MSG(TRACE_APP1, "OTA new state %d", (FMT__H, ota_upgrade_value->upgrade_status)); */

    switch (ota_upgrade_value->upgrade_status)
    {
    /* OTA goes thru number of states calling us to make solutions */


    case ZB_ZCL_OTA_UPGRADE_STATUS_START:
    {
        struct zb_zcl_ota_upgrade_start_s *start = &ota_upgrade_value->upgrade.start;
        /* Start upgrade process. Now have file size, version etc. */
        if (ota_ctx.flash_dev)
        {
            TRACE_MSG(TRACE_OTA1, "OTA is already in progress (is it ever possible?)", (FMT__0));
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        }
        else if (!zb_osif_ota_fw_size_ok(start->file_length))
        {
            TRACE_MSG(TRACE_OTA1, "bad file length %d", (FMT__D, start->file_length));
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        }
        else
        {
            ota_ctx.image_size = start->file_length;
            ota_ctx.flash_dev = zb_osif_ota_open_storage();
            ota_ctx.fw_version = start->file_version;
            zb_osif_ota_mark_fw_absent();
            zb_osif_ota_erase_fw(ota_ctx.flash_dev, 0, ota_ctx.image_size);
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        }
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_OTA_UPGRADE_STATUS_START status %hd", (FMT__H, ota_upgrade_value->upgrade_status));
        break;
    }

    case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
    {
        zb_uint_t len = ota_upgrade_value->upgrade.receive.data_length;
        zb_uint32_t off = ota_upgrade_value->upgrade.receive.file_offset;
        zb_uint8_t *ptr = ota_upgrade_value->upgrade.receive.block_data;
        /* Just received an image portion. */
        /* TRACE_MSG(TRACE_APP3, "ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE off %d len %d", */
        /*           (FMT__D_D, ota_upgrade_value->upgrade.receive.file_offset, */
        /*            ota_upgrade_value->upgrade.receive.data_length)); */
        zb_osif_ota_write(ota_ctx.flash_dev, ptr, off, len, ota_ctx.image_size);
        ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;
    }

    case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
        /* Entire image loaded. Application can now start FW verify
         * process. */
        if (zb_osif_ota_verify_integrity_async(ota_ctx.flash_dev, ota_ctx.image_size))
        {
            ota_ctx.param = param;
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
        }
        else
        {
            ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        }
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_APPLAY:
        /* Upgrade End Resp is ok, ZCL checks for manufacturer, image type etc are ok.
           Last step before actual upgrade. */
        TRACE_MSG(TRACE_APP1, "Finishing upgrade.. UPGRADE_STATUS_APPLAY", (FMT__0));
        zb_osif_ota_mark_fw_ready(ota_ctx.flash_dev, ota_ctx.image_size, ota_ctx.fw_version);
        /* Inform ZCL that we do not need more image. After return ok ZCL starts countdown, then calls
         * us with FINISH. */
        ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
        /* Image received and verified, ZCL finished countdown before
           reboot. Now can reset. */
        TRACE_MSG(TRACE_APP1, "ZB_ZCL_OTA_UPGRADE_STATUS_FINISH - reset", (FMT__0));
        /* Actually may skip zb_zcl_ota_upgrade_file_upgraded - we are
         * resetting anyway. But some application may decide to not reset, so
         * illustrate ZBOSS API usage. */
        zb_zcl_ota_upgrade_file_upgraded(ENDPOINT);
        zb_osif_ota_close_storage(ota_ctx.flash_dev);
        ota_ctx.flash_dev = NULL;
        /* Do not reset immediately - lets finish ZCL pkts exchange etc */
        ZB_SCHEDULE_APP_ALARM(zb_reset, 0, ZB_TIME_ONE_SECOND * 15);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
        /* we are here in case of internal error or iw we sent error status to
           ZCL before */
        TRACE_MSG(TRACE_APP1, "Abort upgrade", (FMT__0));
        zb_osif_ota_close_storage(ota_ctx.flash_dev);
        ota_ctx.flash_dev = NULL;
        ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        break;

    default:
        TRACE_MSG(TRACE_APP1, "Strange state... Abort upgrade", (FMT__0));
        zb_osif_ota_close_storage(ota_ctx.flash_dev);
        ota_ctx.flash_dev = NULL;
        ota_upgrade_value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        break;
    }
}

void zb_osif_ota_verify_integrity_done(zb_uint8_t integrity_is_ok)
{
    zb_zcl_ota_upgrade_send_upgrade_end_req(ota_ctx.param,
                                            (integrity_is_ok == ZB_TRUE) ? ZB_ZCL_OTA_UPGRADE_STATUS_OK : ZB_ZCL_OTA_UPGRADE_STATUS_ERROR);
}
