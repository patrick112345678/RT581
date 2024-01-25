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

#define ZB_TRACE_FILE_ID 40135

#include "zboss_api.h"
#include "zb_osif_ota_stub.h"

void *zb_osif_ota_open_storage(void)
{
  void *dev = NULL;

  TRACE_MSG(TRACE_APP1, " >> zb_osif_ota_open_storage", (FMT__0));
  TRACE_MSG(TRACE_APP1, " << zb_osif_ota_open_storage %p", (FMT__P, dev));

  return dev;
}

zb_bool_t zb_osif_ota_fw_size_ok(zb_uint32_t image_size)
{
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">> zb_osif_ota_fw_size_ok image_size %ld", (FMT__L, image_size));
  TRACE_MSG(TRACE_APP1, "<< zb_osif_ota_fw_size_ok ret %hd", (FMT__H, ret));

  return ret;
}

zb_uint32_t zb_osif_ota_get_erase_portion(void)
{
  zb_uint32_t erase_portion = 0;

  TRACE_MSG(TRACE_APP1, ">> zb_osif_ota_get_erase_portion", (FMT__0));
  TRACE_MSG(TRACE_APP1, "<< zb_osif_ota_get_erase_portion ret %ld", (FMT__L, erase_portion));

  return erase_portion;
}

void zb_osif_ota_erase_fw(void *dev, zb_uint_t offset, zb_uint32_t size)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_erase_fw dev %p offset %d size %ld", (FMT__P_L_D, dev, offset, size));
}

void zb_osif_ota_write(void *dev, zb_uint8_t *data, zb_uint_t off, zb_uint_t size, zb_uint32_t image_size)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_write dev %p data %p offset %d size %d image size %ld",
            (FMT__P_P_D_D_L, dev, data, off, size, image_size));
}

void zb_osif_ota_mark_fw_ready(void *dev, zb_uint32_t size, zb_uint32_t revision)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_mark_fw_ready dev %p size %ld revision %ld",
            (FMT__P_L_L, dev, size, revision));
}

void zb_osif_ota_mark_fw_absent(void)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_mark_fw_absent", (FMT__0));
}


void zb_osif_ota_mark_fw_updated(void)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_mark_fw_updated", (FMT__0));
}

void zb_osif_ota_close_storage(void *dev)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_close_storage %p", (FMT__P, dev));
}

zb_bool_t zb_osif_ota_verify_integrity(void *dev, zb_uint32_t raw_len)
{
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">> zb_osif_ota_verify_integrity dev %p len %ld", (FMT__P_L, dev, raw_len));
  TRACE_MSG(TRACE_APP1, "<< zb_osif_ota_verify_integrity ret %hd", (FMT__H, ret));

  return ret;
}

/* WARNING: Works with absolute address! */
void zb_osif_ota_read(void *dev, zb_uint8_t *data, zb_uint32_t addr, zb_uint32_t size)
{
  TRACE_MSG(TRACE_APP1, "zb_osif_ota_read dev %p data %p addr %ld size %ld",
            (FMT__P_P_L_L, dev, data, addr, size));
}

zb_bool_t zb_osif_ota_verify_integrity_async(void *dev, zb_uint32_t raw_len)
{
  zb_bool_t ret = ZB_TRUE;

  TRACE_MSG(TRACE_APP1, ">> zb_osif_ota_verify_integrity_async dev %p len %ld", (FMT__P_L, dev, raw_len));
  TRACE_MSG(TRACE_APP1, "<< zb_osif_ota_verify_integrity_async ret %hd", (FMT__H, ret));
  return ret;
}


zb_uint8_t zb_erase_fw(zb_uint32_t address, zb_uint32_t pages_count)
{
  ZVUNUSED(address);
  ZVUNUSED(pages_count);
  TRACE_MSG(TRACE_APP2, "TODO: zb_erase_fw emulation", (FMT__0));
  return 0;
}

zb_uint8_t zb_write_fw(zb_uint32_t address, zb_uint8_t *buf, zb_uint16_t len)
{
  ZVUNUSED(address);
  ZVUNUSED(buf);
  ZVUNUSED(len);
  TRACE_MSG(TRACE_APP2, "TODO: zb_write_fw emulation", (FMT__0));
  return 0;
}


void Hash16_Calc(zb_uint32_t pBuffer, zb_uint32_t BufferLength, zb_uint8_t *hash16)
{
  ZVUNUSED(pBuffer);
  ZVUNUSED(BufferLength);
  ZVUNUSED(hash16);
  TRACE_MSG(TRACE_APP1, "zb_nvram_app_write_dataset", (FMT__0));
}
