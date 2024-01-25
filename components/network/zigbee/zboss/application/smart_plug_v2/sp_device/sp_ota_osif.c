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

#define ZB_TRACE_FILE_ID 40251
#include "sp_device.h"

#ifdef SP_WWAH_COMPATIBLE
#include "zdo_wwah_stubs.h"
#endif
#ifdef SP_OTA
#ifdef ZB_USE_OSIF_OTA_ROUTINES

extern sp_device_ctx_t g_dev_ctx;

zb_uint8_t sp_ota_upgrade_init(zb_uint32_t image_size,
                               zb_uint32_t image_version)
{
  zb_uint8_t ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;

  if (g_dev_ctx.ota_ctx.flash_dev)
  {
    TRACE_MSG(TRACE_OTA1, "OTA is already in progress (is it ever possible?)", (FMT__0));
    ret = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
  }
#ifdef SP_WWAH_COMPATIBLE
  else if (zb_zcl_wwah_check_if_downgrade_disabled() && g_dev_ctx.ota_attr.file_version > image_version)
  {
    TRACE_MSG(TRACE_OTA1, "OTA Downgrages disabled by WWAH", (FMT__0));
    ret = ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
  }
#endif
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
       Alternetively can erase by portions in sp_ota_upgrade_write_next_portion().
     */
    zb_osif_ota_erase_fw(g_dev_ctx.ota_ctx.flash_dev, 0, g_dev_ctx.ota_ctx.total_image_size);
    ret = ZB_ZCL_OTA_UPGRADE_STATUS_OK;
  }
  return ret;
}


zb_ret_t sp_ota_upgrade_write_next_portion(zb_uint8_t *ptr, zb_uint32_t off, zb_uint8_t len)
{
  zb_osif_ota_write(g_dev_ctx.ota_ctx.flash_dev, ptr, off, len, g_dev_ctx.ota_ctx.total_image_size);

  return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

zb_uint8_t sp_ota_upgrade_check_fw(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "> sp_ota_upgrade_check_fw", (FMT__0));

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

void sp_ota_upgrade_mark_fw_ok()
{
  zb_osif_ota_mark_fw_ready(g_dev_ctx.ota_ctx.flash_dev, g_dev_ctx.ota_ctx.total_image_size, g_dev_ctx.ota_ctx.fw_version);
  zb_osif_ota_close_storage(g_dev_ctx.ota_ctx.flash_dev);
  g_dev_ctx.ota_ctx.flash_dev = NULL;
}


void sp_ota_upgrade_abort()
{
  zb_osif_ota_close_storage(g_dev_ctx.ota_ctx.flash_dev);
  g_dev_ctx.ota_ctx.flash_dev = NULL;
}


#endif  /* ZB_USE_OSIF_OTA_ROUTINES */
#endif  /* #ifdef SP_OTA */
