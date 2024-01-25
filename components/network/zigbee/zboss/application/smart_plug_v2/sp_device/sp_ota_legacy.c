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
/*  PURPOSE: Legacy implementation for OTA
*/

#define ZB_TRACE_FILE_ID 40252
#include "sp_device.h"

#ifdef SP_OTA
#ifndef ZB_USE_OSIF_OTA_ROUTINES

extern sp_device_ctx_t g_dev_ctx;

void sp_ota_upgrade_mark_downloaded_fw(zb_uint32_t status)
{
    TRACE_MSG(TRACE_APP2, ">> sp_ota_upgrade_mark_downloaded_fw status %ld", (FMT__L, status));

    if (!status)
    {
        /* No firmware - erase */
        TRACE_MSG(TRACE_OTA3, "erase fw at addr %lx sector size %ld", (FMT__L_L, SP_OTA_FW_CTRL_ADDRESS, OTA_ERASE_PORTION_SIZE));
        zb_erase_fw(SP_OTA_FW_CTRL_ADDRESS, OTA_ERASE_PORTION_SIZE);
        g_dev_ctx.ota_ctx.addr_to_erase = SP_OTA_FW_CTRL_ADDRESS + OTA_ERASE_PORTION_SIZE;
    }
    else
    {
        zb_uint32_t val = SP_OTA_FW_DOWNLOADED;
        /* Firmware downloaded ok - write flag (which is also a tag) */
        zb_write_fw(SP_OTA_FW_CTRL_ADDRESS, (void *)&val, sizeof(zb_uint32_t));
    }

    TRACE_MSG(TRACE_APP2, "<< sp_ota_upgrade_mark_downloaded_fw", (FMT__0));
}

zb_uint8_t sp_ota_upgrade_init(zb_uint32_t image_size,
                               zb_uint32_t image_version)
{
    zb_uint8_t hash[SP_OTA_UPGARDE_HASH_LENGTH] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                                                   0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
                                                  };

    TRACE_MSG(TRACE_APP1, ">> sp_ota_upgrade_init image_size %ld", (FMT__L, image_size));

    (void)image_version;
    g_dev_ctx.ota_ctx.total_image_size = image_size;
    g_dev_ctx.ota_ctx.address = SP_OTA_FW_START_ADDRESS;
    g_dev_ctx.ota_ctx.addr_to_erase = SP_OTA_FW_ERASE_ADDRESS;
    g_dev_ctx.ota_ctx.hash_addr = g_dev_ctx.ota_ctx.address + (sizeof(zb_zcl_ota_upgrade_file_header_t) + 6);
    g_dev_ctx.ota_ctx.fw_image_portion_size = 0;

    ZB_MEMCPY(g_dev_ctx.ota_ctx.hash, hash, sizeof(hash));

    sp_ota_upgrade_mark_downloaded_fw(0);

    TRACE_MSG(TRACE_APP1, "<< sp_ota_upgrade_init", (FMT__0));
    return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

zb_ret_t sp_ota_upgrade_write_next_portion(zb_uint8_t *buf, zb_uint32_t offset, zb_uint8_t len)
{
    TRACE_MSG(TRACE_APP1, ">> sp_ota_upgrade_write_next_portion buf %p offset %ld len %hd",
              (FMT__P_L_H, buf, offset, len));
    /* Suppose ZCL gives us continuous data, so do not check for holes */

    /* To overcome possible fragmentation or not alligned to 4 data use
     * temporary buffer: copy data, glue it if needed and write by
     * 32 or 64 bytes blocks */
    ZB_MEMCPY(g_dev_ctx.ota_ctx.fw_image_portion + g_dev_ctx.ota_ctx.fw_image_portion_size,
              buf, len);
    /* do not keep our offset. Trust ZCL code. */
    offset -= g_dev_ctx.ota_ctx.fw_image_portion_size;
    g_dev_ctx.ota_ctx.fw_image_portion_size += len;

    /* TRACE_MSG(TRACE_APP1, "total_image_size %ld fw_image_portion_size %ld offset %ld",
              (FMT__L_L_L, g_dev_ctx.ota_ctx.total_image_size, g_dev_ctx.ota_ctx.fw_image_portion_size, offset)); */

    len = 0;
    if (g_dev_ctx.ota_ctx.fw_image_portion_size + offset >= g_dev_ctx.ota_ctx.total_image_size)
    {
        /* Check for 4-byte alignement. Should be zero for all image blocks
         *  except the last one */
        /* if FW end, align to 4 */
        len = (g_dev_ctx.ota_ctx.fw_image_portion_size + 3) / 4 * 4;
    }
    else if (g_dev_ctx.ota_ctx.fw_image_portion_size >= SP_OTA_IMAGE_BLOCK_DATA_SIZE_MAX)
    {
        /* in the middle align to block size */
        len = g_dev_ctx.ota_ctx.fw_image_portion_size / SP_OTA_IMAGE_BLOCK_DATA_SIZE_MAX * SP_OTA_IMAGE_BLOCK_DATA_SIZE_MAX;
    }

    if (len)
    {
        if ((g_dev_ctx.ota_ctx.address + offset + len) > g_dev_ctx.ota_ctx.addr_to_erase)
        {
            /* TRACE_MSG(TRACE_APP1, "erase sector addr %ld", (FMT__L, g_dev_ctx.ota_ctx.addr_to_erase)); */
            zb_erase_fw(g_dev_ctx.ota_ctx.addr_to_erase, OTA_ERASE_PORTION_SIZE);
            g_dev_ctx.ota_ctx.addr_to_erase += (1 * OTA_ERASE_PORTION_SIZE);
        }

        /* TRACE_MSG(TRACE_APP1, "write: addr %ld len %hd", (FMT__L_H, g_dev_ctx.ota_ctx.address + offset, len)); */
        zb_write_fw(g_dev_ctx.ota_ctx.address + offset, g_dev_ctx.ota_ctx.fw_image_portion, len);

        if (offset + len >= g_dev_ctx.ota_ctx.total_image_size)
        {
            g_dev_ctx.ota_ctx.fw_image_portion_size = 0;
        }
        else
        {
            g_dev_ctx.ota_ctx.fw_image_portion_size -= len;
            if (g_dev_ctx.ota_ctx.fw_image_portion_size)
            {
                ZB_MEMMOVE(g_dev_ctx.ota_ctx.fw_image_portion, g_dev_ctx.ota_ctx.fw_image_portion + len,
                           g_dev_ctx.ota_ctx.fw_image_portion_size);
            }
        }
    } /* if len */
    TRACE_MSG(TRACE_APP1, "<< sp_ota_upgrade_write_next_portion", (FMT__0));
    return ZB_ZCL_OTA_UPGRADE_STATUS_OK;
}

/* Asynchronous OTA image CRC calculation. It is useful only if use slow
 * external flash. Else better use synchronous one. */

zb_uint8_t sp_ota_upgrade_check_fw(zb_uint8_t param)
{
    zb_uint32_t BufferLength = g_dev_ctx.ota_ctx.total_image_size -
                               (2 /* tag Id */ + 4 /* length */ + 2 * SP_OTA_UPGARDE_HASH_LENGTH /* Full hash and Last Block Hash*/);
    zb_uint32_t pBuffer = (zb_uint32_t)g_dev_ctx.ota_ctx.address;

    TRACE_MSG(TRACE_APP1, "> sp_ota_upgrade_check_fw", (FMT__0));

    if (g_dev_ctx.ota_ctx.hash16_calc_ongoing)
    {
        // strange case : this should not happen since the ZBOSS stack prevents this
        // in any case : do not trigger the ZBOSS stack - this will eventually be done when the ongoing hash calculation finishes

        TRACE_MSG(TRACE_APP1, "< sp_ota_upgrade_check_fw ret %hd", (FMT__H, ZB_ZCL_OTA_UPGRADE_STATUS_ERROR));
        return ZB_ZCL_OTA_UPGRADE_STATUS_ERROR;
    }

    g_dev_ctx.ota_ctx.hash16_calc_ongoing = ZB_TRUE;
    g_dev_ctx.ota_ctx.param = param;
    ZB_MEMSET(g_dev_ctx.ota_ctx.hash, 0, sizeof(g_dev_ctx.ota_ctx.hash));
#ifndef SP_OTA_SKIP_HASH_CHECK
    Hash16_Calc(pBuffer, BufferLength, g_dev_ctx.ota_ctx.hash);
#else
    ZB_SCHEDULE_APP_CALLBACK(ota_checking_fw_finished, 0);
#endif

    TRACE_MSG(TRACE_APP1, "< sp_ota_upgrade_check_fw ret %hd", (FMT__H, ZB_ZCL_OTA_UPGRADE_STATUS_BUSY));

    return ZB_ZCL_OTA_UPGRADE_STATUS_BUSY;
}


void ota_checking_fw_finished(zb_uint8_t param)
{
    zb_uint8_t ota_hash[16];
    zb_uint32_t ota_hash_address = (zb_uint32_t)(g_dev_ctx.ota_ctx.address + (g_dev_ctx.ota_ctx.total_image_size - 2 * SP_OTA_UPGARDE_HASH_LENGTH));

    ZVUNUSED(param);

    g_dev_ctx.ota_ctx.hash16_calc_ongoing = ZB_FALSE;

    TRACE_MSG(TRACE_APP1, ">> ota_checking_fw_finished", (FMT__0));

    ZB_BZERO(ota_hash, sizeof(ota_hash));
    zb_read_fw(ota_hash_address, ota_hash, sizeof(ota_hash));

    if (
#ifndef SP_OTA_SKIP_HASH_CHECK
        !ZB_MEMCMP(g_dev_ctx.ota_ctx.hash, ota_hash, sizeof(g_dev_ctx.ota_ctx.hash))
#else
        1
#endif
    )
    {
        /* set mark "new fw" so bootloader will try to upgrade */
#ifndef SP_OTA_SKIP_APP_UPGRADE
        sp_ota_upgrade_mark_downloaded_fw(1);
#endif
        zb_zcl_ota_upgrade_send_upgrade_end_req(g_dev_ctx.ota_ctx.param, ZB_ZCL_OTA_UPGRADE_STATUS_OK);
    }
    else
    {
        /* Abort will be called from zb_zcl_ota_upgrade_send_upgrade_end_req(). */
        zb_zcl_ota_upgrade_send_upgrade_end_req(g_dev_ctx.ota_ctx.param, ZB_ZCL_OTA_UPGRADE_STATUS_ERROR);
    }

    TRACE_MSG(TRACE_APP1, "<< ota_checking_fw_finished", (FMT__0));
}


void sp_ota_upgrade_mark_fw_ok()
{
    /* TODO: move there     sp_ota_upgrade_mark_downloaded_fw(1); */
}


void sp_ota_upgrade_abort()
{
    sp_ota_upgrade_mark_downloaded_fw(0);
}

#endif  /* #ifndef ZB_USE_OSIF_OTA_ROUTINES */
#endif  /* #ifdef SP_OTA */
