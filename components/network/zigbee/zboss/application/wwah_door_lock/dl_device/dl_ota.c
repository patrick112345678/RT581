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

#define ZB_TRACE_FILE_ID 40129
#include "zboss_api.h"
#include "zb_wwah_door_lock.h"
#include "zcl/zb_zcl_wwah.h"
#include "zb_osif_ota_stub.h"

zb_uint8_t dl_ota_upgrade_init(zb_uint32_t image_size, zb_uint32_t image_version);
zb_ret_t dl_ota_upgrade_write_next_portion(zb_uint8_t *ptr, zb_uint32_t off, zb_uint8_t len);
zb_uint8_t dl_ota_upgrade_check_fw(zb_uint8_t param);
void dl_ota_upgrade_mark_fw_ok(void);
void dl_ota_upgrade_abort(void);
void dl_ota_upgrade_server_not_found(void);
void dl_device_reset_after_upgrade(zb_uint8_t param);

void dl_ota_start_upgrade(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "> dl_ota_init_upgrade", (FMT__0));

    ZB_SCHEDULE_APP_ALARM(zb_zcl_ota_upgrade_init_client, param, 15 * ZB_TIME_ONE_SECOND);

    TRACE_MSG(TRACE_APP1, "< dl_ota_init_upgrade", (FMT__0));
}

void dl_ota_upgrade_server_not_found(void)
{
    TRACE_MSG(TRACE_APP1, "dl_ota_upgrade_server_not_found", (FMT__0));
}

void dl_process_ota_upgrade_cb(zb_uint8_t param)
{
    zb_zcl_device_callback_param_t *device_cb_param =
        ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
    zb_zcl_ota_upgrade_value_param_t *value = &(device_cb_param->cb_param.ota_value_param);

    TRACE_MSG(TRACE_APP1, ">> dl_process_ota_upgrade_cb param %hd",
              (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "status %hd", (FMT__H, value->upgrade_status));

    switch (value->upgrade_status)
    {
    case ZB_ZCL_OTA_UPGRADE_STATUS_START:
        value->upgrade_status = dl_ota_upgrade_init(value->upgrade.start.file_length,
                                value->upgrade.start.file_version);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_RECEIVE:
        value->upgrade_status = dl_ota_upgrade_write_next_portion(value->upgrade.receive.block_data,
                                value->upgrade.receive.file_offset,
                                value->upgrade.receive.data_length);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_CHECK:
        value->upgrade_status = dl_ota_upgrade_check_fw(param);
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_APPLY:
        dl_ota_upgrade_mark_fw_ok();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_FINISH:
        zb_zcl_ota_upgrade_file_upgraded(WWAH_DOOR_LOCK_EP);
        /* Do not reset immediately - lets finish ZCL pkts exchange etc */
        ZB_SCHEDULE_APP_ALARM(dl_device_reset_after_upgrade, 0, ZB_TIME_ONE_SECOND * 15);
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_ABORT:
        dl_ota_upgrade_abort();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    case ZB_ZCL_OTA_UPGRADE_STATUS_SERVER_NOT_FOUND:
        dl_ota_upgrade_server_not_found();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
        break;

    default:
        dl_ota_upgrade_abort();
        value->upgrade_status = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< dl_process_ota_upgrade_cb result_status %hd",
              (FMT__H, value->upgrade_status));
}

void dl_device_reset_after_upgrade(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> dl_ota_upgrade_check_fw", (FMT__0));

    ZVUNUSED(param);

    zb_reset(0);

    TRACE_MSG(TRACE_APP1, "<< dl_ota_upgrade_check_fw", (FMT__0));
}


#ifdef ZB_USE_OSIF_OTA_ROUTINES
zb_uint8_t dl_ota_upgrade_init(zb_uint32_t image_size,
                               zb_uint32_t image_version)
{
    zb_uint8_t ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;

    if (g_dev_ctx.ota_ctx.flash_dev)
    {
        TRACE_MSG(TRACE_OTA1, "OTA is already in progress (is it ever possible?)", (FMT__0));
        ret = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
    }
    else if (zb_zcl_wwah_check_if_downgrade_disabled() && g_dev_ctx.ota_attr.file_version > image_version)
    {
        TRACE_MSG(TRACE_OTA1, "OTA Downgrages disabled by WWAH", (FMT__0));
        ret = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
    }
    else if (!zb_osif_ota_fw_size_ok(image_size))
    {
        TRACE_MSG(TRACE_OTA1, "bad file length %d", (FMT__D, image_size));
        ret = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
    }
    else
    {
        /*
         * OTA server sends OTA file with OTA header at begin.
         * We support here trivial OTA file with only single image. We are not
         * interested on OTA header - will skip it.
         */
        g_dev_ctx.ota_ctx.total_image_size = image_size;
        g_dev_ctx.ota_ctx.flash_dev = zb_osif_ota_open_storage();
        g_dev_ctx.ota_ctx.fw_version = image_version;
        zb_osif_ota_mark_fw_absent();
        /* Simplify our life: sync erase space for entire FW.
           Alternetively can erase by portions in dl_ota_upgrade_write_next_portion().
         */
        zb_osif_ota_erase_fw(g_dev_ctx.ota_ctx.flash_dev, 0, g_dev_ctx.ota_ctx.total_image_size);
        ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
    }
    return ret;
}


zb_ret_t dl_ota_upgrade_write_next_portion(zb_uint8_t *ptr, zb_uint32_t off, zb_uint8_t len)
{
    zb_osif_ota_write(g_dev_ctx.ota_ctx.flash_dev, ptr, off, len, g_dev_ctx.ota_ctx.total_image_size);

    return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

zb_uint8_t dl_ota_upgrade_check_fw(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "> dl_ota_upgrade_check_fw", (FMT__0));

    if (zb_osif_ota_verify_integrity_async(g_dev_ctx.ota_ctx.flash_dev, g_dev_ctx.ota_ctx.total_image_size))
    {
        g_dev_ctx.ota_ctx.param = param;
        return ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
    }

    return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
}

void zb_osif_ota_verify_integrity_done(zb_uint8_t integrity_is_ok)
{
    zb_zcl_ota_upgrade_send_upgrade_end_req(g_dev_ctx.ota_ctx.param,
                                            (integrity_is_ok == ZB_TRUE) ? ZB_ZCL_OTA_UPGRADE_STATUS_OK : ZB_ZCL_OTA_UPGRADE_STATUS_ERROR);
}

void dl_ota_upgrade_mark_fw_ok(void)
{
    zb_osif_ota_mark_fw_ready(g_dev_ctx.ota_ctx.flash_dev, g_dev_ctx.ota_ctx.total_image_size, g_dev_ctx.ota_ctx.fw_version);
    zb_osif_ota_close_storage(g_dev_ctx.ota_ctx.flash_dev);
    g_dev_ctx.ota_ctx.flash_dev = NULL;
}

void dl_ota_upgrade_abort(void)
{
    zb_osif_ota_close_storage(g_dev_ctx.ota_ctx.flash_dev);
    g_dev_ctx.ota_ctx.flash_dev = NULL;
}

#endif  /* ZB_USE_OSIF_OTA_ROUTINES */
