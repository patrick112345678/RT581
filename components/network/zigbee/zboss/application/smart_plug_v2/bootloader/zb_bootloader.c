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
/* PURPOSE: OTA bootloader
*/

#define ZB_TRACE_FILE_ID 63260

#include "zboss_api.h"

#define GP_COMPONENT_ID GP_COMPONENT_ID_APP

#define OTA_FW_IMG_TAG_ADDRESS                                      \
  (OTA_FW_START_ADDRESS + sizeof(zb_zcl_ota_upgrade_file_header_t))
#define OTA_FW_IMG_TAG_LENGTH 2

#define OTA_FW_IMG_LEN_ADDRESS (OTA_FW_IMG_TAG_ADDRESS + OTA_FW_IMG_TAG_LENGTH)
#define OTA_FW_IMG_LEN_LENGTH 4

#define OTA_FW_IMG_DATA_ADDRESS (OTA_FW_IMG_LEN_ADDRESS + OTA_FW_IMG_LEN_LENGTH)

int main_bl(void)
{
    char buf[ZB_NVRAM_FLASH_PAGE_SIZE] = {0};
    zb_int32_t fw_length = 0;
    zb_uint32_t read_address = OTA_FW_IMG_DATA_ADDRESS;
    zb_uint32_t write_address = GP691_FW_APPLICATION_ADDRESS;
    zb_uint16_t length = ZB_NVRAM_FLASH_PAGE_SIZE;
    zb_uint32_t status = 0;
    zb_ret_t ret = RET_OK;

    zb_m25p20_spi_init();

    /* Check flash for actual FW */
    zb_m25p20_spi_read_block(OTA_FW_CTRL_ADDRESS, sizeof(zb_uint32_t), (zb_uint8_t *)&status);

    if (status == OTA_FW_DOWNLOADED)
    {
        zb_uint16_t offset = write_address % ZB_NVRAM_FLASH_PAGE_SIZE;

        /* FW is present. Get its length */
        zb_m25p20_spi_read_block(OTA_FW_IMG_LEN_ADDRESS, sizeof(zb_int32_t), (zb_uint8_t *)&fw_length);

        ret = zb_osif_bl_flash_erase(fw_length);

        if (fw_length > 0 && offset != 0)
        {
            /* NK: Get rest of the page */
            zb_uint16_t page_rest = ZB_NVRAM_FLASH_PAGE_SIZE - offset;

            zb_m25p20_spi_read_block(read_address, page_rest, (zb_uint8_t *)buf);
            ret = zb_osif_bl_flash_write(write_address, (zb_uint8_t *)buf, page_rest);
            fw_length -= page_rest;
            read_address += page_rest;
            write_address += page_rest;
        }

        while ((fw_length > 0) && (ret == RET_OK))
        {
            /* NK: Last block case */
            if (fw_length <= ZB_NVRAM_FLASH_PAGE_SIZE)
            {
                length = fw_length;
            }
            zb_m25p20_spi_read_block(read_address, length, (zb_uint8_t *)buf);
            ret = zb_osif_bl_flash_write(write_address, (zb_uint8_t *)buf, length);
            fw_length -= length;
            read_address += length;
            write_address += length;
        }

        zb_m25p20_spi_erase_sector(OTA_FW_CTRL_ADDRESS);
    }

    zb_m25p20_spi_deinit();

    return 0;
}
