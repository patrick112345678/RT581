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
/*  PURPOSE: Installcodes low level routines
*/

#define ZB_TRACE_FILE_ID 471
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_zdo_globals.h"
#include "zboss_api.h"

#ifdef ZB_SECURITY_INSTALLCODES

const zb_uint8_t zb_ic_size_by_type[ZB_IC_TYPE_MAX] = {6, 8, 12, 16};

#ifndef NCP_MODE_HOST

zb_ret_t zb_secur_ic_check_crc(zb_uint8_t ic_type, zb_uint8_t *ic)
{
    zb_uint16_t calculated_crc = ~zb_crc16(ic, 0xffff, ZB_IC_SIZE_BY_TYPE(ic_type));
    zb_uint16_t ic_crc;
    TRACE_MSG(TRACE_SECUR3, "> zb_secur_ic_check_crc: ic_type: %hd", (FMT__H, ic_type));

    ZB_LETOH16(&ic_crc, ic + ZB_IC_SIZE_BY_TYPE(ic_type));
    if (calculated_crc != ic_crc)
    {
        printf("please use the correct IC CRC value:%04x\r\n", calculated_crc);
        TRACE_MSG(TRACE_SECUR3, "ERROR in installation code:FMT:%hd CRC error:0x%04X != 0x%04X", (FMT__H_D_D, (ZB_IC_SIZE_BY_TYPE(ic_type) * 8), calculated_crc, ic_crc));
        return RET_ERROR;
    }
    return RET_OK;
}


static void zb_secur_ic_generate_key(zb_uint8_t ic_type, zb_uint8_t *ic, zb_uint8_t *key)
{
    ZB_ASSERT(key);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().enable_alldoors_key)
    {
        /*
         * Single key for everything and everybody.
         * Nothing similar in the spec, but we already have it and it might be
         * useful for debug purposes. */
        ZB_MEMCPY(key, ZB_AIB().tc_standard_key, ZB_IC_SIZE_BY_TYPE(ic_type)/*ZB_CCM_KEY_SIZE*/);
    }
    else
#endif
    {
        zb_uint8_t tmp[ZB_CCM_KEY_SIZE * 2U];
        (void)zb_sec_b6_hash(ic, (zb_uint32_t)ZB_IC_SIZE_BY_TYPE(ic_type) + 2U, tmp);
        ZB_MEMCPY(key, tmp, ZB_CCM_KEY_SIZE);

        TRACE_MSG( TRACE_ERROR, "APS key from installcode: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(key)));
    }
}


zb_ret_t zb_secur_ic_get_key_by_address(zb_ieee_addr_t address, zb_uint8_t *key)
{
    zb_uint8_t *ic = NULL;
    zb_uint8_t ic_type = 0;
#if defined ZB_COORDINATOR_ROLE && defined ZB_USE_NVRAM
    zb_uint8_t ic_buf[ZB_IC_TYPE_MAX_SIZE + 2];
#endif
    zb_ret_t ret = RET_NOT_FOUND;


    /*cstat -MISRAC2012-Rule-2.1_b */
    if (ZB_IS_TC())
    {
        /*cstat +MISRAC2012-Rule-2.1_b */
        /** @mdr{00012,8} */
#if defined ZB_COORDINATOR_ROLE && defined ZB_USE_NVRAM
        if (zb_secur_ic_get_from_tc_storage(address, &ic_type, ic_buf) != -1)
        {
            ic = ic_buf;
        }
#endif
    }
#ifndef ZB_COORDINATOR_ONLY
    else if (ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, address)
             || ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address))
    {
        /* At client side IC can be used for communication with TC only */
        ic = zb_secur_ic_get_from_client_storage(&ic_type);
    }
    else
    {
        /* MISRA rule 15.7 requires empty 'else' branch. */
    }
#endif

    if (ic != NULL)
    {
        if (zb_secur_ic_check_crc(ic_type, ic) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "zb_secur_ic_get_key_by_address: bad CRC in installcode for address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(address)));
            ret = RET_PROTOCOL_ERROR;
        }
        else
        {
            zb_secur_ic_generate_key(ic_type, ic, key);
            TRACE_MSG(TRACE_SECUR3, "zb_secur_ic_get_key_by_address: derived key " TRACE_FORMAT_128 " from installcode for address " TRACE_FORMAT_64,
                      (FMT__B_A, TRACE_ARG_128(key), TRACE_ARG_64(address)));
            ret = RET_OK;
            uint8_t i = 0;
            printf("IC KEY: ");
            for (i = 0; i < 16; i++)
            {
                printf("%02x ", key[i]);
            }
            printf("\r\n");
        }
    }
    else
    {
        TRACE_MSG(TRACE_SECUR3, "zb_secur_ic_get_key_by_address: NO installcode for address " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(address)));
    }
    return ret;
}


zb_ret_t zb_secur_ic_set(zb_uint8_t ic_type, zb_uint8_t *ic)
{
    if (zb_secur_ic_check_crc(ic_type, ic) == RET_OK)
    {
        ZB_MEMCPY(ZB_AIB().installcode, ic, (zb_uint32_t)ZB_IC_SIZE_BY_TYPE(ic_type) + 2U);
        ZB_AIB().installcode_type = ic_type;
        TRACE_MSG(TRACE_SECUR1, "zb_secur_ic_set: setup my installcode " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));
        return RET_OK;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "zb_secur_ic_set: bad crc", (FMT__0));
        return RET_CONVERSION_ERROR;
    }
}


#ifndef ZB_COORDINATOR_ONLY
zb_uint8_t *zb_secur_ic_get_from_client_storage(zb_uint8_t *ic_type)
{
    zb_uint_t i;
    zb_uint8_t *p = ZB_AIB().installcode;
    zb_uint32_t zcnt = 0;

    *ic_type = ZB_AIB().installcode_type;

    TRACE_MSG(TRACE_SECUR3, "zb_secur_ic_get_from_client_storage: ret installcode %p " TRACE_FORMAT_128,
              (FMT__P_A_A, ZB_AIB().installcode, TRACE_ARG_128(ZB_AIB().installcode)));
    for (i = 0 ; i < sizeof(ZB_AIB().installcode) ; ++i)
    {
        zcnt += ZB_B2U(p[i] == 0U);
    }
    if (zcnt == sizeof(ZB_AIB().installcode))
    {
        TRACE_MSG(TRACE_SECUR3, "No client installcode", (FMT__0));
        p = NULL;
    }
    return p;
}
#endif  /* #ifndef ZB_COORDINATOR_ONLY */


#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
#if defined(ZB_USE_NVRAM) && !defined(APP_ONLY_NVRAM)


/**
   Read installcode from the storage.

   Use hash to find installcode position in the nvram. Actual installcode data
   is in nvram only.
 */
zb_int8_t zb_secur_ic_get_from_tc_storage(zb_ieee_addr_t address, zb_uint8_t *ic_type, zb_uint8_t *buf)
{
    zb_uint_t n = 0;
    zb_int_t idx = zb_64bit_hash(address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;

    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
        if (ZB_AIB().installcodes_table[idx].nvram_offset != 0)
        {
            zb_aps_installcode_nvram_t rec;
            zb_ret_t ret;
            TRACE_MSG(TRACE_SECUR1, "zb_secur_ic_get_from_tc_storage: nonzero offset found %d", (FMT__D, idx));
            ret = zb_osif_nvram_read(ZB_AIB().installcodes_table[idx].nvram_page, ZB_AIB().installcodes_table[idx].nvram_offset, (zb_uint8_t *)&rec, sizeof(rec));
            if (ret == RET_OK
                    && ZB_IEEE_ADDR_CMP(rec.device_address, address))
            {
                ZB_MEMCPY(buf, rec.installcode, sizeof(rec.installcode));
                *ic_type = ZB_IC_GET_TYPE_FROM_REC(&rec);
                TRACE_MSG(TRACE_SECUR1, "zb_secur_ic_get_from_tc_storage: got installcode by hash idx %d " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64, (FMT__D_B_A, idx, TRACE_ARG_128(rec.installcode), TRACE_ARG_64(address)));
                return idx;
            }
        }
        idx = (idx + 1) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
    }
    TRACE_MSG(TRACE_SECUR1, "zb_secur_ic_get_from_tc_storage: no installcode for address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(address)));
    return -1;
}


void zb_secur_ic_add(zb_ieee_addr_t address, zb_uint8_t ic_type, zb_uint8_t *ic, zb_secur_ic_add_cb_t cb)
{
    zb_ret_t status = RET_OK;

    if (zb_secur_ic_check_crc(ic_type, ic) == RET_OK)
    {
        zb_uint8_t buf[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE];
        zb_uint8_t *existing_ic;
        zb_secur_ic_add_t ic_add;
        zb_uint8_t rd_ic_type;

        existing_ic = (zb_secur_ic_get_from_tc_storage(address, &rd_ic_type, buf) == -1) ? NULL : buf;
        if (!existing_ic
                || ZB_MEMCMP(existing_ic, ic, ZB_IC_SIZE_BY_TYPE(ic_type) + 2))
        {
            ic_add.address = address;
            ic_add.ic = ic;
            ic_add.type = ic_type;
            ic_add.do_update = !!existing_ic;
            ZB_AIB().installcode_to_add = &ic_add;
            TRACE_MSG(TRACE_SECUR1, "zb_secur_ic_add: adding (do_update %hd) installcode " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64,
                      (FMT__H_B_A, ic_add.do_update, TRACE_ARG_128(ic), TRACE_ARG_64(address)));
            /* Note: NVRAM write is synchronous, so it is safe to use ptr to the local variable. */
            zb_nvram_write_dataset(ZB_NVRAM_INSTALLCODES);
            ZB_AIB().installcode_to_add = NULL;
        }

        status = RET_OK;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "zb_secur_ic_add: bad CRC in installcode for address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(address)));

        status = RET_CONVERSION_ERROR;
    }

    if (cb != NULL)
    {
        cb(status);
    }
}


void zb_secur_ic_remove_req(zb_uint8_t param)
{
    zb_int8_t idx;
    zb_uint8_t buf[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE];
    zb_aps_installcode_nvram_t rec;
    zb_uint8_t ic_type;
    zb_secur_ic_remove_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_req_t);
    zb_secur_ic_remove_resp_t *resp_param = NULL;
    zb_ieee_addr_t address;
    zb_callback_t response_cb;
    zb_ret_t ret = RET_OK;

    ZB_IEEE_ADDR_COPY(address, req_param->device_address);
    response_cb = req_param->response_cb;

    idx = zb_secur_ic_get_from_tc_storage(address, &ic_type, buf);
    if (idx != -1)
    {
        TRACE_MSG(TRACE_SECUR1,
                  "zb_secur_ic_remove: remove installcode " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64,
                  (FMT__B_A, TRACE_ARG_128(buf), TRACE_ARG_64(address)));

        ZB_BZERO(&rec, sizeof(zb_aps_installcode_nvram_t));

        ZB_AIB().installcodes_table[idx].nvram_page = 0;
        ZB_AIB().installcodes_table[idx].nvram_offset = 0;

        zb_nvram_write_dataset(ZB_NVRAM_INSTALLCODES);

        ret = RET_OK;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR,
                  "zb_secur_ic_remove: could not find installcode for address " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(address)));
        ret = RET_NO_MATCH;
    }

    if (response_cb != NULL)
    {
        resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_resp_t);
        resp_param->status = ret;

        ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
}

void zb_secur_ic_remove_all_req(zb_uint8_t param)
{
    zb_uint_t n = 0;
    zb_secur_ic_remove_all_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_req_t);
    zb_secur_ic_remove_all_resp_t *resp_param = NULL;
    zb_ret_t ret = RET_OK;
    zb_callback_t response_cb;

    response_cb = req_param->response_cb;

    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE && ret == RET_OK)
    {
        if (ZB_AIB().installcodes_table[n].nvram_offset != 0)
        {
            zb_aps_installcode_nvram_t rec;
            ret = zb_osif_nvram_read(ZB_AIB().installcodes_table[n].nvram_page,
                                     ZB_AIB().installcodes_table[n].nvram_offset,
                                     (zb_uint8_t *)&rec, sizeof(rec));

            /*cstat !MISRAC2012-Rule-14.3_a */
            /** @mdr{00007,13} */
            if (ret == RET_OK)
            {
                TRACE_MSG(TRACE_SECUR1, "got installcode index %d " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64, (FMT__D_B_A, n, TRACE_ARG_128(rec.installcode), TRACE_ARG_64(rec.device_address)));

                ZB_AIB().installcodes_table[n].nvram_page = 0;
                ZB_AIB().installcodes_table[n].nvram_offset = 0;
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "can't read nvram, page %d, offset %d", (FMT__D_D, ZB_AIB().installcodes_table[n].nvram_page, ZB_AIB().installcodes_table[n].nvram_offset));
            }
        }
        n++;
    }

    zb_nvram_write_dataset(ZB_NVRAM_INSTALLCODES);

    if (response_cb != NULL)
    {
        resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_resp_t);
        resp_param->status = ret;

        ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
}

void zb_secur_ic_get_list_req(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_secur_ic_get_list_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_req_t);
    zb_secur_ic_get_list_resp_t *resp_param = NULL;

    zb_uindex_t start_index = req_param->start_index;
    zb_uindex_t ic_entry_index = 0;
    zb_callback_t response_cb = req_param->response_cb;

    zb_uindex_t ic_table_list_max_count = (ZB_APS_PAYLOAD_MAX_LEN) / sizeof(zb_aps_installcode_nvram_t);
    zb_uindex_t ic_table_list_count = 0;
    zb_uindex_t ic_table_entries = 0;

    zb_aps_installcode_nvram_t *ic_table = NULL;
    zb_aps_installcode_nvram_t ic_entry;

    TRACE_MSG(TRACE_SECUR1, ">> zb_secur_ic_get_list_req, param %hd, start_index %hd, max_entries %hd",
              (FMT__H_H_H, param, start_index, ic_table_list_max_count));

    ic_table = (zb_aps_installcode_nvram_t *)zb_buf_initial_alloc(param,
               sizeof(zb_aps_installcode_nvram_t) * ic_table_list_max_count);

    for (ic_entry_index = 0; ic_entry_index < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ic_entry_index++)
    {
        if (ZB_AIB().installcodes_table[ic_entry_index].nvram_offset != 0)
        {
            ret = zb_osif_nvram_read(ZB_AIB().installcodes_table[ic_entry_index].nvram_page,
                                     ZB_AIB().installcodes_table[ic_entry_index].nvram_offset,
                                     (zb_uint8_t *)&ic_entry,
                                     sizeof(zb_aps_installcode_nvram_t));
            /*cstat !MISRAC2012-Rule-14.3_a */
            /** @mdr{00007,14} */
            if (ret == RET_OK)
            {
                TRACE_MSG(TRACE_SECUR1, "got installcode index %d " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64,
                          (FMT__D_B_A, ic_entry_index, TRACE_ARG_128(ic_entry.installcode), TRACE_ARG_64(ic_entry.device_address)));

                if (ic_table_entries >= start_index && ic_table_list_count < ic_table_list_max_count)
                {
                    ZB_MEMCPY(&ic_table[ic_table_list_count], &ic_entry, sizeof(ic_entry));
                    ic_table_list_count++;
                }

                ic_table_entries++;
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "can't read nvram, page %d, offset %d",
                          (FMT__D_D, ZB_AIB().installcodes_table[ic_entry_index].nvram_page, ZB_AIB().installcodes_table[ic_entry_index].nvram_offset));
                break;
            }
        }
    }

    if (response_cb != NULL)
    {
        resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_resp_t);
        resp_param->status = ret;
        resp_param->ic_table_entries = ic_table_entries;
        resp_param->start_index = start_index;
        resp_param->ic_table_list_count = ic_table_list_count;

        ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_SECUR1, "<< zb_secur_ic_get_list_req: %d", (FMT__D, ret));
}


void zb_secur_ic_get_by_idx_req(zb_uint8_t param)
{
    zb_ret_t  ret = RET_ERROR;
    zb_aps_installcode_nvram_t ic_entry;
    zb_secur_ic_get_by_idx_req_t *req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_req_t);
    zb_secur_ic_get_by_idx_resp_t *resp_param = NULL;

    zb_uindex_t ic_entry_index = 0;
    zb_uindex_t ic_table_entries = 0;
    zb_uindex_t ic_index = req_param->ic_index;
    zb_callback_t response_cb = req_param->response_cb;

    TRACE_MSG(TRACE_SECUR1, ">> zb_secur_ic_get_by_idx_req, param %hd, index %hd", (FMT__H_H, param, ic_index));

    for (ic_entry_index = 0; ic_entry_index < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ic_entry_index++)
    {
        if (ZB_AIB().installcodes_table[ic_entry_index].nvram_offset != 0)
        {
            ret = zb_osif_nvram_read(ZB_AIB().installcodes_table[ic_entry_index].nvram_page,
                                     ZB_AIB().installcodes_table[ic_entry_index].nvram_offset,
                                     (zb_uint8_t *)&ic_entry,
                                     sizeof(zb_aps_installcode_nvram_t));

            /*cstat !MISRAC2012-Rule-14.3_a */
            /** @mdr{00007,15} */
            if (ret == RET_OK)
            {
                TRACE_MSG(TRACE_SECUR1, "got installcode index %d " TRACE_FORMAT_128 " for address " TRACE_FORMAT_64,
                          (FMT__D_B_A, ic_entry_index, TRACE_ARG_128(ic_entry.installcode), TRACE_ARG_64(ic_entry.device_address)));

                if (ic_table_entries == ic_index)
                {
                    break;
                }

                ic_table_entries++;
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "can't read nvram, page %d, offset %d",
                          (FMT__D_D, ZB_AIB().installcodes_table[ic_entry_index].nvram_page, ZB_AIB().installcodes_table[ic_entry_index].nvram_offset));
                break;
            }
        }
    }

    if (response_cb != NULL)
    {
        resp_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_resp_t);
        resp_param->status = ret;

        if (ret == RET_OK)
        {
            ZB_IEEE_ADDR_COPY(resp_param->device_address, ic_entry.device_address);
            resp_param->ic_type = ic_entry.options;

            ZB_MEMCPY(resp_param->installcode, ic_entry.installcode, ZB_IC_SIZE_BY_TYPE(resp_param->ic_type));
        }

        ZB_SCHEDULE_CALLBACK(response_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_SECUR1, "<< zb_secur_ic_get_by_idx_req, result %d", (FMT__D, ret));
}


void zb_secur_ic_str_add(zb_ieee_addr_t address, char *ic_str, zb_secur_ic_add_cb_t cb)
{
    zb_uint8_t ic_type;
    zb_uint8_t ic[ZB_IC_TYPE_MAX_SIZE + 2];

    TRACE_MSG(TRACE_SECUR3, ">zb_secur_ic_str_add", (FMT__0));

    if ( RET_OK != zb_secur_ic_from_string( ic_str, &ic_type, ic ) )
    {
        if (cb != NULL)
        {
            cb(RET_ERROR);
        }
    }
    else
    {
        zb_secur_ic_add(address, ic_type, ic, cb);
    }
}

#endif /* ZB_USE_NVRAM && !APP_ONLY_NVRAM */

#endif /* ZB_COORDINATOR_ROLE && ZB_SECURITY_INSTALLCODES */


#endif /* NCP_MODE_HOST */


static zb_uint8_t c2b(char c)
{
    zb_uint8_t uc = (zb_uint8_t)c;

    ZB_ASSERT(c > '\0');
    if ((c <= '9') && (c >= '0'))
    {
        return uc - 48u;
    }
    else if ((c <= 'F') && (c >= 'A'))
    {
        return uc - 55u;
    }
    else if ((c <= 'f') && (c >= 'a'))
    {
        return uc - 87u;
    }
    else
    {
        return 16u;
    };
}


zb_ret_t zb_secur_ic_from_string(char *ic_str, zb_uint8_t *ic_type, zb_uint8_t *ic)
{
    zb_ret_t ret = RET_OK;
    const zb_uint8_t jshift[4] = {4, 0, 12, 8};
    zb_uint8_t ic_len, i, j, n, l, ic_type_tmp;
    char c;
    zb_uint16_t ic16[9], o, b;

    i = 0;
    j = 0;
    o = 0;
    n = 0, l = 0;
    while ((l <= 36u) && (i < 128u))
    {
        if (j > 3u)
        {
            j = 0;
            ic16[n++] = o;
            o = 0;
            TRACE_MSG(TRACE_SECUR3, "Installation code[%hd]=0x%04X", (FMT__H_D, n - 1, ic16[n - 1]));
        }
        c = ic_str[i++];
        if ('\0' == c)
        {
            break;
        }
        if ((b = c2b(c)) < 16u)
        {
            o |= b << jshift[j++];
            l++;
        }
        else if (b != 16u)
        {
            /* TODO: This line is never executed, because c2b() returns values <= 16. Find out if it is a bug, ZOI-500 */
            TRACE_MSG(TRACE_SECUR3, "ERROR in installation code: ic string contains wrong char at pos:%hd", (FMT__H, i));
            n = 0;
            ret = RET_ERROR;
        }
        else
        {
            /* MISRA rule 15.7 requires empty 'else' branch. */
        }
    };

    switch (n)
    {
    case 4:
        ic_type_tmp = ZB_IC_TYPE_48;
        break;
    case 5:
        ic_type_tmp = ZB_IC_TYPE_64;
        break;
    case 7:
        ic_type_tmp = ZB_IC_TYPE_96;
        break;
    case 9:
        ic_type_tmp = ZB_IC_TYPE_128;
        break;
    default:
        TRACE_MSG(TRACE_SECUR3, "ERROR in installation code: ic string len is:%hd words", (FMT__H, n));
        ret = RET_ERROR;
        break; /* MISRA Rule 16.3 requires switch cases to end with 'break' */
    }

    if (ret == RET_OK)
    {
        ic_len = ZB_IC_SIZE_BY_TYPE(ic_type_tmp);

        ret = zb_secur_ic_check_crc(ic_type_tmp, (zb_uint8_t *)ic16);
        if (ret == RET_OK)
        {
            ZB_MEMSET(ic + ic_len + 2U, 0, (zb_uint32_t)ZB_IC_SIZE_BY_TYPE(ZB_IC_TYPE_MAX - 1U) - (zb_uint32_t)ic_len);
            ZB_MEMCPY(ic, ic16, (zb_uint32_t)ic_len + 2U);
            *ic_type = ic_type_tmp;
        }
    }

    return ret;
}

zb_ret_t zb_secur_ic_str_set(char *ic_str)
{
    zb_uint8_t ic_type;
    zb_uint8_t ic[ZB_IC_TYPE_MAX_SIZE + 2U];

    TRACE_MSG(TRACE_SECUR3, ">zb_secur_ic_str_set", (FMT__0));

    if ( RET_OK != zb_secur_ic_from_string( ic_str, &ic_type, ic ) )
    {
        return RET_ERROR;
    }
    return zb_secur_ic_set(ic_type, ic);
}

#endif  /* ZB_SECURITY_INSTALLCODES */
