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
/* PURPOSE: NVRAM custom ds handlers
*/

#define ZB_TRACE_FILE_ID 121
#include "zboss_api_core.h"

#include "zb_common.h"
#if defined ZB_USE_NVRAM

#include "zb_nvram.h"

#define DS_CTX() ZB_NVRAM().custom_ds_ctx

zb_ret_t zb_nvram_custom_ds_register(zb_nvram_ds_filter_cb_t filter,
                                     zb_nvram_ds_get_length_cb_t get_length,
                                     zb_nvram_ds_get_version_cb_t get_version,
                                     zb_nvram_ds_read_cb_t read,
                                     zb_nvram_ds_write_cb_t write)
{
    zb_size_t curr_idx;
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_COMMON1,
              ">> zb_nvram_custom_ds_register, filter %p, get_length %p, get_version %p"
              "read %p, write %p",
              (FMT__P_P_P_P_P, filter, get_length, get_version, read, write));

    ZB_ASSERT(filter != NULL);
    ZB_ASSERT(get_length != NULL);
    ZB_ASSERT(read != NULL);
    ZB_ASSERT(write != NULL);

    curr_idx = DS_CTX().ds_cnt;

    if (DS_CTX().ds_cnt < ZB_NVRAM_CUSTOM_DS_MAX_NUMBER)
    {
        DS_CTX().handlers[curr_idx].filter = filter;
        DS_CTX().handlers[curr_idx].get_length = get_length;
        DS_CTX().handlers[curr_idx].get_version = get_version;
        DS_CTX().handlers[curr_idx].read = read;
        DS_CTX().handlers[curr_idx].write = write;

        DS_CTX().ds_cnt++;
    }
    else
    {
        ret = RET_NO_MEMORY;
    }

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_custom_ds_register, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t zb_nvram_custom_ds_try_read(zb_uint16_t ds_type,
                                     zb_uint8_t page,
                                     zb_uint32_t pos,
                                     zb_uint16_t len,
                                     zb_nvram_ver_t nvram_ver,
                                     zb_uint16_t ds_ver)
{
    zb_size_t curr_idx;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_COMMON1,
              ">> zb_nvram_custom_ds_try_read, ds_type %d, page %d, pos %d, len %d, nvram_ver %d, ds_ver %d",
              (FMT__D_D_D_D_D, ds_type, page, pos, len, nvram_ver, ds_ver));

    for (curr_idx = 0; curr_idx < DS_CTX().ds_cnt; curr_idx++)
    {
        if (DS_CTX().handlers[curr_idx].filter(ds_type))
        {
            DS_CTX().handlers[curr_idx].read(ds_type, page, pos, len, nvram_ver, ds_ver);
            ret = RET_OK;
            break;
        }
    }

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_custom_ds_try_read, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t zb_nvram_custom_ds_try_write(zb_uint16_t ds_type,
                                      zb_uint8_t page,
                                      zb_uint32_t pos)
{
    zb_size_t curr_idx;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_custom_ds_try_write, ds_type %d, page %d, pos %d",
              (FMT__D_D_D, ds_type, page, pos));

    for (curr_idx = 0; curr_idx < DS_CTX().ds_cnt; curr_idx++)
    {
        if (DS_CTX().handlers[curr_idx].filter(ds_type))
        {
            ret = DS_CTX().handlers[curr_idx].write(ds_type, page, pos);
            break;
        }
    }

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_custom_ds_try_write, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t zb_nvram_custom_ds_try_get_length(zb_uint16_t ds_type,
        zb_size_t *len)
{
    zb_size_t curr_idx;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_custom_ds_try_get_length, ds_type %d, len %p",
              (FMT__D_P, ds_type, len));

    ZB_ASSERT(len != NULL);

    for (curr_idx = 0; curr_idx < DS_CTX().ds_cnt; curr_idx++)
    {
        if (DS_CTX().handlers[curr_idx].filter(ds_type))
        {
            *len = DS_CTX().handlers[curr_idx].get_length(ds_type);
            ret = RET_OK;
            break;
        }
    }

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_custom_ds_try_get_length, ret %d", (FMT__D, ret));

    return ret;
}


zb_bool_t zb_nvram_custom_ds_is_supported(zb_uint16_t ds_type)
{
    zb_size_t curr_idx;
    zb_bool_t ret = ZB_FALSE;

    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_custom_ds_is_supported, ds_type %d", (FMT__D, ds_type));

    for (curr_idx = 0; curr_idx < DS_CTX().ds_cnt; curr_idx++)
    {
        if (DS_CTX().handlers[curr_idx].filter(ds_type))
        {
            ret = ZB_TRUE;
            break;
        }
    }

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_custom_ds_is_supported, ret %d", (FMT__D, ret));

    return ret;
}

#endif /* ZB_USE_NVRAM */
