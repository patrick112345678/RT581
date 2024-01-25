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
/*  PURPOSE: OTA routines based on OTA low level routines in osif
*/

#define ZB_TRACE_FILE_ID 63355
#include "izs_device.h"

#ifdef IZS_OTA
#ifdef ZB_USE_OSIF_OTA_ROUTINES

extern izs_device_ctx_t g_device_ctx;

zb_uint8_t izs_ota_upgrade_start(zb_uint32_t image_size,
                                 zb_uint32_t image_version)
{
  zb_uint8_t ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;

  if (g_device_ctx.ota_ctx.flash_dev)
  {
    TRACE_MSG(TRACE_OTA1, "OTA is already in progress (is it ever possible?)", (FMT__0));
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
    g_device_ctx.ota_ctx.total_image_size = image_size;
    g_device_ctx.ota_ctx.flash_dev = zb_osif_ota_open_storage();
    g_device_ctx.ota_ctx.fw_version = image_version;
    zb_osif_ota_mark_fw_absent();
    /* Simplify our life: sync erase space for entire FW.
       Alternetively can erase by portions in izs_ota_upgrade_write_next_portion().
     */
    zb_osif_ota_erase_fw(g_device_ctx.ota_ctx.flash_dev, 0, g_device_ctx.ota_ctx.total_image_size);
    ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
  }
  return ret;
}


zb_ret_t izs_ota_upgrade_write_next_portion(zb_uint8_t *ptr, zb_uint32_t off, zb_uint8_t len)
{
  zb_osif_ota_write(g_device_ctx.ota_ctx.flash_dev, ptr, off, len, g_device_ctx.ota_ctx.total_image_size);

  return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}


zb_uint8_t izs_ota_upgrade_check_fw(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "> izs_ota_upgrade_check_fw", (FMT__0));

  if (zb_osif_ota_verify_integrity_async(g_device_ctx.ota_ctx.flash_dev, g_device_ctx.ota_ctx.total_image_size))
  {
    g_device_ctx.ota_ctx.param = param;
    return ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
  }
  return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
}

void zb_osif_ota_verify_integrity_done(zb_uint8_t integrity_is_ok)
{
  zb_zcl_ota_upgrade_send_upgrade_end_req(g_device_ctx.ota_ctx.param,
    (integrity_is_ok == ZB_TRUE) ? ZB_ZCL_OTA_UPGRADE_STATUS_OK : ZB_ZCL_OTA_UPGRADE_STATUS_ERROR);
}

void izs_ota_upgrade_mark_fw_ok(void)
{
  zb_osif_ota_mark_fw_ready(g_device_ctx.ota_ctx.flash_dev, g_device_ctx.ota_ctx.total_image_size, g_device_ctx.ota_ctx.fw_version);
  zb_osif_ota_close_storage(g_device_ctx.ota_ctx.flash_dev);
  g_device_ctx.ota_ctx.flash_dev = NULL;
}


void izs_ota_upgrade_abort(void)
{
  zb_osif_ota_close_storage(g_device_ctx.ota_ctx.flash_dev);
  g_device_ctx.ota_ctx.flash_dev = NULL;
}

#endif  /* ZB_USE_OSIF_OTA_ROUTINES */
#endif  /* #ifdef IZS_OTA */
