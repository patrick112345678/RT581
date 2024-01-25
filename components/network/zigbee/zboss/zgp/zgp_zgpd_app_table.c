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
/*  PURPOSE: Implementation of Application description table
*/

#define ZB_TRACE_FILE_ID 2105

#include "zb_common.h"

#if defined ZB_ENABLE_ZGP

#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"

#ifdef ZB_ENABLE_ZGP_SINK

#define ZB_GET_APP_TBL()               ZGP_CTXC().app_table
#define ZB_GET_APP_TBL_ENT_BY_IDX(idx) ZGP_CTXC().app_table[idx]

static zb_bool_t zb_zgp_addrs_are_eq(zb_zgpd_id_t *zgpd_id_p, zgp_runtime_app_tbl_ent_t *ent)
{
    if ((zgpd_id_p->app_id == ZB_ZGP_APP_ID_0000) &&
            !ent->base.info.options.ieee_addr_present &&
            (zgpd_id_p->addr.src_id == ent->base.info.addr.src_id))
    {
        return ZB_TRUE;
    }

    if ((zgpd_id_p->app_id != ZB_ZGP_APP_ID_0000) &&
            ent->base.info.options.ieee_addr_present &&
            (ZB_MEMCMP(ent->base.info.addr.ieee_addr, zgpd_id_p->addr.ieee_addr, sizeof(zb_ieee_addr_t)) == 0))
    {
        return ZB_TRUE;
    }

    return ZB_FALSE;
}

static zgp_runtime_app_tbl_ent_t *zb_zgp_create_app_tbl_ent_by_id(zb_zgpd_id_t *zgpd_id_p)
{
    zb_uint8_t i = 0;

    while ((i < ZB_ZGP_APP_TBL_SIZE) && ZB_GET_APP_TBL_ENT_BY_IDX(i).status != ZGP_APP_TBL_ENT_STATUS_FREE)
    {
        i++;
    }

    if (i >= ZB_ZGP_APP_TBL_SIZE)
    {
        return NULL;
    }

    ZB_GET_APP_TBL_ENT_BY_IDX(i).status = ZGP_APP_TBL_ENT_STATUS_INIT;
    ZB_GET_APP_TBL_ENT_BY_IDX(i).receive_reports_num = 0;
    ZB_GET_APP_TBL_ENT_BY_IDX(i).reply_buf = 0;
    ZB_GET_APP_TBL_ENT_BY_IDX(i).need_reply = ZB_FALSE;
    ZB_GET_APP_TBL_ENT_BY_IDX(i).base.info.options.ieee_addr_present = zgpd_id_p->app_id != ZB_ZGP_APP_ID_0000;
    ZB_MEMCPY(&ZB_GET_APP_TBL_ENT_BY_IDX(i).base.info.addr, &zgpd_id_p->addr, sizeof(zgpd_id_p->addr));

    return &ZB_GET_APP_TBL_ENT_BY_IDX(i);
}

zgp_runtime_app_tbl_ent_t *zb_zgp_get_app_tbl_ent_by_id(zb_zgpd_id_t *zgpd_id_p)
{
    zb_uint8_t i;
    for (i = 0; i < ZB_ZGP_APP_TBL_SIZE; i++)
    {
        if (ZB_GET_APP_TBL_ENT_BY_IDX(i).status != ZGP_APP_TBL_ENT_STATUS_FREE)
        {
            if (zb_zgp_addrs_are_eq(zgpd_id_p, &ZB_GET_APP_TBL_ENT_BY_IDX(i)))
            {
                return &ZB_GET_APP_TBL_ENT_BY_IDX(i);
            }
        }
    }

    return NULL;
}

zgp_runtime_app_tbl_ent_t *zb_zgp_alloc_app_tbl_ent_by_id(zb_zgpd_id_t *zgpd_id_p)
{
    zgp_runtime_app_tbl_ent_t *ent;

    ent = zb_zgp_get_app_tbl_ent_by_id(zgpd_id_p);

    if (!ent)
    {
        ent = zb_zgp_create_app_tbl_ent_by_id(zgpd_id_p);
    }

    return ent;
}

void zb_zgp_erase_app_table_ent_by_id(zb_zgpd_id_t *zgpd_id_p)
{
    zgp_runtime_app_tbl_ent_t *ent;

    ent = zb_zgp_get_app_tbl_ent_by_id(zgpd_id_p);

    if (ent)
    {
        if (ent->reply_buf)
        {
            zb_buf_free(ent->reply_buf);
        }
        ZB_BZERO(ent, sizeof(zgp_runtime_app_tbl_ent_t));
    }
}

void zb_zgp_erase_app_tbl_ent(zgp_runtime_app_tbl_ent_t *ent)
{
    if (ent)
    {
        ZB_BZERO(ent, sizeof(zgp_runtime_app_tbl_ent_t));
    }
}

zgp_report_desc_t *zb_zgp_get_report_desc_from_app_tbl(zb_zgpd_id_t *zgpd_id_p, zb_uint8_t report_idx)
{
    zgp_runtime_app_tbl_ent_t *ent;

    ent = zb_zgp_get_app_tbl_ent_by_id(zgpd_id_p);

    if (!ent)
    {
        TRACE_MSG(TRACE_ERROR, "zb_zgp_get_report_desc_from_app_tbl: ent not found", (FMT__0));
    }
    else if (ent->status != ZGP_APP_TBL_ENT_STATUS_COMPLETE)
    {
        TRACE_MSG(TRACE_ERROR, "zb_zgp_get_report_desc_from_app_tbl: invalid status %hd, total_rep %hd, rec_rep %hd",
                  (FMT__H_H_H, ent->status, ent->base.info.total_reports_num, ent->receive_reports_num));
    }
    else if (report_idx >= ZB_APP_DESCR_REPORTS_NUM)
    {
        TRACE_MSG(TRACE_ERROR, "zb_zgp_get_report_desc_from_app_tbl: invalid report_idx %hd, max_idx %hd",
                  (FMT__H_H, report_idx, ZB_APP_DESCR_REPORTS_NUM - 1));
    }
    else
    {
        return &ent->base.reports[report_idx];
    }

    return NULL;
}
#endif /* ZB_ENABLE_ZGP_SINK */


#ifdef ZB_USE_NVRAM

#ifdef ZB_ENABLE_ZGP_SINK

static zb_uint16_t zb_zgp_nvram_calc_app_tbl_ent_size (zgp_app_tbl_ent_t *ent)
{
    ZB_ASSERT(ent);
    return (sizeof(ent->info) + ent->info.total_reports_num * sizeof(zgp_report_desc_t));
}

static zb_ret_t zb_zgp_nvram_read_app_tbl_ent(zgp_app_tbl_ent_t *ent, zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    ZB_ASSERT(ent);

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&ent->info, sizeof(ent->info));

    if ((ret == RET_OK) && (ent->info.total_reports_num > 0))
    {
        ret = zb_osif_nvram_read(page, pos + sizeof(ent->info), (zb_uint8_t *)ent->reports, sizeof(zgp_report_desc_t) * ent->info.total_reports_num);
    }

    return ret;
}

static zb_ret_t zb_zgp_nvram_write_app_tbl_ent(zgp_app_tbl_ent_t *ent, zb_uint8_t page, zb_uint32_t *pos)
{
    zb_ret_t ret;

    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, *pos, (zb_uint8_t *)&ent->info, sizeof(ent->info));
    *pos += sizeof(ent->info);

    if ((ret == RET_OK) && (ent->info.total_reports_num))
    {
        /* If we fail, trace is given and assertion is triggered */
        ret = zb_nvram_write_data(page, *pos, (zb_uint8_t *)ent->reports, sizeof(zgp_report_desc_t) * ent->info.total_reports_num);
        *pos += sizeof(zgp_report_desc_t) * ent->info.total_reports_num;
    }

    return ret;
}

#endif /* ZB_ENABLE_ZGP_SINK */


zb_uint16_t zb_zgp_nvram_app_tbl_length(void)
{
    zb_uint16_t ret = 0;
#ifdef ZB_ENABLE_ZGP_SINK
    zb_uint8_t i = 0;

    for (i = 0; i < ZB_ZGP_APP_TBL_SIZE; i++)
    {
        if ((ZB_GET_APP_TBL_ENT_BY_IDX(i).status == ZGP_APP_TBL_ENT_STATUS_COMPLETE) ||
                (ZB_GET_APP_TBL_ENT_BY_IDX(i).status == ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO))
        {
            ret += zb_zgp_nvram_calc_app_tbl_ent_size(&ZB_GET_APP_TBL_ENT_BY_IDX(i).base);
        }
    }
#endif /* ZB_ENABLE_ZGP_SINK */
    return ret;
}

zb_ret_t zb_zgp_nvram_write_app_tbl_dataset(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret = RET_OK;
#ifdef ZB_ENABLE_ZGP_SINK
    zb_uint8_t i;

    for (i = 0; i < ZB_ZGP_APP_TBL_SIZE && ret == RET_OK ; i++)
    {
        if ((ZB_GET_APP_TBL_ENT_BY_IDX(i).status == ZGP_APP_TBL_ENT_STATUS_COMPLETE) ||
                (ZB_GET_APP_TBL_ENT_BY_IDX(i).status == ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO))
        {
            ret = zb_zgp_nvram_write_app_tbl_ent(&ZB_GET_APP_TBL_ENT_BY_IDX(i).base, page, &pos);
        }
    }
#else
    ZVUNUSED(page);
    ZVUNUSED(pos);
#endif /* ZB_ENABLE_ZGP_SINK */
    return ret;
}

void zb_zgp_nvram_read_app_tbl_dataset(
    zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
#ifdef ZB_ENABLE_ZGP_SINK
    zb_ret_t ret = RET_OK;

    zb_uint8_t i;

    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);

    ZB_BZERO(ZB_GET_APP_TBL(), sizeof(ZB_GET_APP_TBL())); /* clear app_tbl */

    for (i = 0; i < ZB_ZGP_APP_TBL_SIZE && ret == RET_OK && length ; i++)
    {
        zb_uint16_t ent_size;

        ret = zb_zgp_nvram_read_app_tbl_ent(&ZB_GET_APP_TBL_ENT_BY_IDX(i).base, page, pos);

        if (ret == RET_OK)
        {
            ent_size = zb_zgp_nvram_calc_app_tbl_ent_size(&ZB_GET_APP_TBL_ENT_BY_IDX(i).base);
            pos += ent_size;

            ZB_GET_APP_TBL_ENT_BY_IDX(i).status = ZB_GET_APP_TBL_ENT_BY_IDX(i).base.info.total_reports_num > 0 ?
                                                  ZGP_APP_TBL_ENT_STATUS_COMPLETE
                                                  : ZGP_APP_TBL_ENT_STATUS_INIT_WITH_SW_INFO;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "zb_osif_nvram_read return error (idx %hd)", (FMT__H, i));
        }

        if (ret == RET_OK && ent_size <= length)
        {
            length -= ent_size;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "invalid length", (FMT__H, i));
            ret = RET_ERROR;
        }
    }

    if (ret == RET_OK)
    {
        TRACE_MSG(TRACE_ZGP4, "%hd records readed", (FMT__H, i));
    }
#else
    ZVUNUSED(page);
    ZVUNUSED(pos);
    ZVUNUSED(length);
    ZVUNUSED(nvram_ver);
    ZVUNUSED(ds_ver);
#endif
}

#endif /* ZB_USE_NVRAM */

#endif /* defined ZB_ENABLE_ZGP */

