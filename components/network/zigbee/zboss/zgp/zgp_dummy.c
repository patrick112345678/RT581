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
/* PURPOSE: ZGP stubs. Don't implement any code here.
*/

#define ZB_TRACE_FILE_ID 2107

#include "zb_common.h"

/**
   This file contains ZGP function stubs that need for the stack library if ZGP is not used.
   @attention This file is legacy and should not be compiled! Use it for reference only.
   Now conditional compilation is used to remove ZGP usage from the stack
*/

void zb_zgp_init()
{

}

void zb_zgp_set_skip_gpdf(zb_uint8_t skip)
{
    ZVUNUSED(skip);
}

zb_uint8_t zb_zgp_get_skip_gpdf(void)
{
    return 1;
}

void zb_zgp_sync_pib(zb_uint8_t param)
{
    ZVUNUSED(param);
}

zb_uint16_t zb_zgp_ctx_size()
{
    return 0;
}

void zb_cgp_data_cfm(zb_uint8_t param)
{
    zb_buf_free(param);
}

void zgp_init_by_scheduler(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void zgp_handle_dev_annce(zb_zdo_device_annce_t *da)
{
    ZVUNUSED(da);
}

void zb_gp_mcps_data_indication(zb_uint8_t param)
{
    zb_buf_free(param);
}

#ifdef ZB_USE_NVRAM

zb_ret_t zb_nvram_write_zgp_proxy_table_dataset(zb_uint8_t page, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);

    return RET_OK;
}

void zb_nvram_read_zgp_proxy_table_dataset(
    zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);
    ZVUNUSED(length);
    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);
}

zb_uint16_t zb_nvram_zgp_proxy_table_length()
{
    return 0;
}

void zb_nvram_update_zgp_proxy_tbl_offset(zb_uint8_t page, zb_uint32_t dataset_pos, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(dataset_pos);
    ZVUNUSED(pos);
}

void zb_nvram_read_zgp_sink_table_dataset(
    zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);
    ZVUNUSED(length);
    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);
}

zb_ret_t zb_nvram_write_zgp_sink_table_dataset(zb_uint8_t page, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);

    return RET_OK;
}

zb_uint16_t zb_nvram_zgp_sink_table_length()
{
    return 0;
}

void zb_nvram_update_zgp_sink_tbl_offset(zb_uint8_t page, zb_uint32_t dataset_pos, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(dataset_pos);
    ZVUNUSED(pos);
}

void zb_nvram_read_zgp_cluster_dataset(
    zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);
    ZVUNUSED(length);
    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);
}

zb_ret_t zb_nvram_write_zgp_cluster_dataset(zb_uint8_t page, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);

    return RET_OK;
}

zb_uint16_t zb_nvram_zgp_cluster_length()
{
    return 0;
}

zb_ret_t zb_zgp_nvram_write_app_tbl_dataset(zb_uint8_t page, zb_uint32_t pos)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);

    return RET_OK;
}

void zb_zgp_nvram_read_app_tbl_dataset(zb_uint8_t page, zb_uint32_t pos,
                                       zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
    ZVUNUSED(page);
    ZVUNUSED(pos);
    ZVUNUSED(length);
    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);
}

zb_uint16_t zb_zgp_nvram_app_tbl_length(void)
{
    return 0;
}

#endif /* ZB_USE_NVRAM */
