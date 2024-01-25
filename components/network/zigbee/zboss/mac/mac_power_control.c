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
/*  PURPOSE: MAC Power control implementation for Zigbee R22 specification
*/

#define ZB_TRACE_FILE_ID 58

#include "zb_config.h"
#include "zb_common.h"

#if defined ZB_MAC_POWER_CONTROL && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_mac_globals.h"

static void mac_power_ctrl_expire_alarm(zb_uint8_t index);

static void mac_power_ctrl_tbl_init_ent(zb_mac_power_ctrl_info_tbl_ent_t **ent)
{
    ZB_ASSERT(ent);

    ZB_IEEE_ADDR_COPY((*ent)->ieee_addr, g_unknown_ieee_addr);
    (*ent)->short_addr = 0xFFFF;
    (*ent)->nwk_negotiated = 0;
    (*ent)->last_rssi = ZB_MAC_POWER_CONTROL_INVALID_POWER_VALUE;
    (*ent)->tx_power = ZB_MAC_POWER_CONTROL_INVALID_POWER_VALUE;
}

static zb_ret_t mac_power_ctrl_tbl_create_ent(zb_mac_power_ctrl_info_tbl_ent_t **ent)
{
    zb_uint8_t i;
    zb_ret_t ret = RET_NO_MEMORY;

    TRACE_MSG(TRACE_MAC3, ">> mac_power_ctrl_tbl_create_ent", (FMT__0));

    /* Firstly try to find free 'gap' slot */
    for (i = 0; i < MAC_PIB().power_info_table.n_used; i++)
    {
        if (MAC_PIB().power_info_table.tbl[i].short_addr == 0xFFFF &&
                ZB_IEEE_ADDR_IS_UNKNOWN(MAC_PIB().power_info_table.tbl[i].ieee_addr))
        {
            break;
        }
    }

    if (i < ZB_MAC_POWER_CONTROL_INFO_TABLE_SIZE)
    {
        *ent = &MAC_PIB().power_info_table.tbl[i];
        mac_power_ctrl_tbl_init_ent(ent);

        if (i == MAC_PIB().power_info_table.n_used)
        {
            MAC_PIB().power_info_table.n_used++;
        }

        ret = RET_OK;
    }

    TRACE_MSG(TRACE_MAC3, "<< mac_power_ctrl_tbl_create_ent ret %d used %hd",
              (FMT__D_H, ret, MAC_PIB().power_info_table.n_used));

    return ret;
}


static zb_ret_t mac_power_ctrl_tbl_get_ent_by_ieee(zb_ieee_addr_t ieee_addr,
        zb_mac_power_ctrl_info_tbl_ent_t **ent)
{
    zb_ushort_t i;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_MAC3, ">> mac_power_ctrl_tbl_get_ent_by_ieee ieee "TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ieee_addr)));

    *ent = NULL;
    for (i = 0; i < MAC_PIB().power_info_table.n_used; i++)
    {
        if (ZB_IEEE_ADDR_CMP(MAC_PIB().power_info_table.tbl[i].ieee_addr, ieee_addr))
        {
            *ent = &MAC_PIB().power_info_table.tbl[i];
            ret = RET_OK;
            break;
        }
    }

    TRACE_MSG(TRACE_MAC3, "<< mac_power_ctrl_tbl_get_ent_by_ieee ret %d, idx %hd",
              (FMT__D_H, ret, i));

    return ret;
}


static zb_ret_t mac_power_ctrl_tbl_get_ent_by_short(zb_uint16_t short_addr,
        zb_mac_power_ctrl_info_tbl_ent_t **ent)
{
    zb_ushort_t i;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_MAC3, ">> mac_power_ctrl_tbl_get_ent_by_short short_addr 0x%x",
              (FMT__H, short_addr));

    *ent = NULL;
    if (short_addr != 0xFFFF)
    {
        for (i = 0; i < MAC_PIB().power_info_table.n_used; i++)
        {
            if (MAC_PIB().power_info_table.tbl[i].short_addr == short_addr)
            {
                *ent = &MAC_PIB().power_info_table.tbl[i];
                ret = RET_OK;
                break;
            }
        }
    }

    TRACE_MSG(TRACE_MAC3, "<< mac_power_ctrl_tbl_get_ent_by_short ret %d, idx %hd",
              (FMT__D_H, ret, i));

    return ret;
}


static void mac_power_ctrl_tbl_clear_ent_by_idx(zb_uint8_t idx)
{
    zb_mac_power_ctrl_info_tbl_ent_t *ent;
    ent = &MAC_PIB().power_info_table.tbl[idx];
    mac_power_ctrl_tbl_init_ent(&ent);
}


static zb_ret_t mac_power_ctrl_tbl_delete_ent_by_idx(zb_uint8_t idx)
{
    zb_ret_t ret = RET_ERROR;

    TRACE_MSG(TRACE_MAC3, ">> mac_power_ctrl_tbl_delete_ent_by_idx idx %hd used %hd",
              (FMT__H_H, idx, MAC_PIB().power_info_table.n_used));

    if (idx < MAC_PIB().power_info_table.n_used)
    {
        zb_time_t dummy_tmo;
        /* Clear the entry */
        mac_power_ctrl_tbl_clear_ent_by_idx(idx);

        ret = RET_OK;

        if (ZB_SCHEDULE_GET_ALARM_TIME(mac_power_ctrl_expire_alarm, ZB_ALARM_ANY_PARAM, &dummy_tmo) == RET_NOT_FOUND)
        {
            zb_uint8_t i;

            for (i = 0; i < MAC_PIB().power_info_table.n_used; i++)
            {
                if (MAC_PIB().power_info_table.tbl[i].short_addr == 0xFFFF &&
                        ZB_IEEE_ADDR_IS_UNKNOWN(MAC_PIB().power_info_table.tbl[i].ieee_addr))
                {
                    MAC_PIB().power_info_table.n_used--;
                    if (i < MAC_PIB().power_info_table.n_used)
                    {
                        ZB_MEMMOVE(&MAC_PIB().power_info_table.tbl[i], &MAC_PIB().power_info_table.tbl[i + 1],
                                   (MAC_PIB().power_info_table.n_used - i) * sizeof(zb_mac_power_ctrl_info_tbl_ent_t));
                    }
                }
            }
        }
    }

    TRACE_MSG(TRACE_MAC3, "<< mac_power_ctrl_tbl_delete_ent_by_idx ret %d",
              (FMT__D, ret));

    return ret;
}


static zb_ret_t mac_power_ctrl_tbl_get_idx(zb_mac_power_ctrl_info_tbl_ent_t **ent,
        zb_uint8_t *idx)
{
    zb_ret_t ret = RET_ERROR;
    zb_uint8_t tbl_index;

    tbl_index = (*ent - MAC_PIB().power_info_table.tbl) /
                sizeof(zb_mac_power_ctrl_info_tbl_ent_t);

    if (tbl_index < MAC_PIB().power_info_table.n_used)
    {
        *idx = tbl_index;
        ret = RET_OK;
    }

    return ret;
}


static zb_ret_t mac_power_ctrl_tbl_delete_ent(zb_mac_power_ctrl_info_tbl_ent_t **ent)
{
    zb_ret_t ret;
    zb_uint8_t idx;

    TRACE_MSG(TRACE_MAC3, ">> mac_power_ctrl_tbl_delete_ent ent %p *ent %p",
              (FMT__P_P, ent, *ent));

    ret = mac_power_ctrl_tbl_get_idx(ent, &idx);
    if (ret == RET_OK)
    {
        ret = mac_power_ctrl_tbl_delete_ent_by_idx(idx);
    }

    TRACE_MSG(TRACE_MAC2, "<< mac_power_ctrl_tbl_delete_ent ret %d",
              (FMT__D, ret));

    return ret;
}


static void mac_power_ctrl_expire_alarm(zb_uint8_t index)
{
    TRACE_MSG(TRACE_MAC1, "mac_power_ctrl_expire_alarm index %hd",
              (FMT__H, index));
    mac_power_ctrl_tbl_delete_ent_by_idx(index);
}


static zb_int8_t mac_power_ctrl_calc_tx_power(zb_int8_t rx_power, zb_int8_t rx_rssi)
{
    zb_int8_t tx_power;
    zb_int8_t pathloss;

    pathloss = rx_power - rx_rssi;

    tx_power = ZB_MAC_POWER_CONTROL_OPT_SIGNAL_LEVEL + pathloss;

    if (tx_power > ZB_TRANS_MAX_TX_POWER_SUB_GHZ)
    {
        tx_power = ZB_TRANS_MAX_TX_POWER_SUB_GHZ;
    }
    else if (tx_power < ZB_TRANS_MIN_TX_POWER_SUB_GHZ)
    {
        tx_power = ZB_TRANS_MIN_TX_POWER_SUB_GHZ;
    }

    TRACE_MSG(TRACE_MAC3, "mac_power_calc_tx_power rx_power %hd, rx_rssi %hd, tx_power %hd",
              (FMT__H_H_H, rx_power, rx_rssi, tx_power));

    return tx_power;
}


zb_ret_t zb_mac_power_ctrl_update_ent_by_ieee(zb_ieee_addr_t ieee_addr,
        zb_int8_t rx_power,
        zb_int8_t rssi,
        zb_uint8_t create)
{
    zb_mac_power_ctrl_info_tbl_ent_t *ent;
    zb_bool_t created_new = ZB_FALSE;
    zb_uint8_t idx = 0;
    zb_ret_t ret;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_update_ent_by_ieee ieee_addr "TRACE_FORMAT_64
              " rx_power %hd, rssi %hd, create %hd",
              (FMT__A_H_H_H, TRACE_ARG_64(ieee_addr), rx_power, rssi, create));

    ret = mac_power_ctrl_tbl_get_ent_by_ieee(ieee_addr, &ent);
    if (ret != RET_OK && create)
    {
        ret = mac_power_ctrl_tbl_create_ent(&ent);
        if (ret == RET_OK)
        {
            created_new = ZB_TRUE;
            mac_power_ctrl_tbl_get_idx(&ent, &idx);
            ZB_SCHEDULE_ALARM(mac_power_ctrl_expire_alarm, idx, ZB_MAC_POWER_CONTROL_EXPIRATION_TIMEOUT);
        }
    }

    if (ret == RET_OK)
    {
        if (created_new)
        {
            ZB_IEEE_ADDR_COPY(ent->ieee_addr, ieee_addr);
        }
        ent->last_rssi = rssi;
        ent->tx_power = mac_power_ctrl_calc_tx_power(rx_power, rssi);
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_update_ent_by_ieee ret %d",
              (FMT__D, ret));

    return ret;
}


zb_ret_t zb_mac_power_ctrl_update_ent(zb_ieee_addr_t ieee_addr,
                                      zb_uint16_t short_addr,
                                      zb_int8_t rx_power,
                                      zb_int8_t rssi,
                                      zb_uint8_t create)
{
    zb_mac_power_ctrl_info_tbl_ent_t *ent;
    zb_uint8_t idx = 0;
    zb_ret_t ret;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_update_ent ieee_addr "TRACE_FORMAT_64
              " short_addr 0x%x, rx_power %hd, rssi %hd, create %hd",
              (FMT__A_D_H_H_H, TRACE_ARG_64(ieee_addr), short_addr, rx_power, rssi, create));

    /* Firstly search by IEEE */
    ret = mac_power_ctrl_tbl_get_ent_by_ieee(ieee_addr, &ent);

    /* Search by short address if not found */
    if (ret != RET_OK)
    {
        ret = mac_power_ctrl_tbl_get_ent_by_short(short_addr, &ent);
    }

    if (ret != RET_OK && create)
    {
        ret = mac_power_ctrl_tbl_create_ent(&ent);
        if (ret == RET_OK)
        {
            mac_power_ctrl_tbl_get_idx(&ent, &idx);
            ZB_SCHEDULE_ALARM(mac_power_ctrl_expire_alarm, idx, ZB_MAC_POWER_CONTROL_EXPIRATION_TIMEOUT);
        }
    }

    if (ret == RET_OK)
    {
        ZB_IEEE_ADDR_COPY(ent->ieee_addr, ieee_addr);
        ent->short_addr = short_addr;
        ent->last_rssi = rssi;
        ent->tx_power = mac_power_ctrl_calc_tx_power(rx_power, rssi);
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_update_ent ret %d",
              (FMT__D, ret));

    return ret;
}


zb_ret_t zb_mac_power_ctrl_tbl_update_rssi_by_short(zb_uint16_t short_addr,
        zb_int8_t rssi)
{
    zb_ret_t ret;
    zb_mac_power_ctrl_info_tbl_ent_t *ent;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_tbl_update_rssi_by_short short_addr 0x%x, rssi %hd",
              (FMT__D_H, short_addr, rssi));

    ret = mac_power_ctrl_tbl_get_ent_by_short(short_addr, &ent);
    if (ret == RET_OK)
    {
        ent->last_rssi = rssi;
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_tbl_update_rssi_by_short ret %d",
              (FMT__D, ret));

    return ret;
}


zb_ret_t zb_mac_power_ctrl_tbl_update_rssi_by_ieee(zb_ieee_addr_t ieee_addr,
        zb_int8_t rssi)
{
    zb_ret_t ret;
    zb_mac_power_ctrl_info_tbl_ent_t *ent;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_tbl_update_rssi_by_ieee ieee "TRACE_FORMAT_64
              " rssi %hd", (FMT__A_H, TRACE_ARG_64(ieee_addr), rssi));

    ret = mac_power_ctrl_tbl_get_ent_by_ieee(ieee_addr, &ent);
    if (ret == RET_OK)
    {
        ent->last_rssi = rssi;
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_tbl_update_rssi_by_ieee ret %hd",
              (FMT__D, ret));

    return ret;
}


zb_ret_t zb_mac_power_ctrl_apply_tx_power_by_short(zb_uint16_t short_addr)
{
    zb_ret_t ret = RET_NOT_FOUND;
    zb_mac_power_ctrl_info_tbl_ent_t *ent;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_get_tx_power_by_short short_addr 0x%x",
              (FMT__D, short_addr));

    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(MAC_PIB().phy_current_page))
    {
        ret = mac_power_ctrl_tbl_get_ent_by_short(short_addr, &ent);

        if (ret == RET_OK)
        {
            zb_mac_set_tx_power(ent->tx_power);
        }
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_get_tx_power_by_short ret 0x%x",
              (FMT__D, ret));

    return ret;
}


zb_ret_t zb_mac_power_ctrl_apply_tx_power_by_ieee(zb_ieee_addr_t ieee_addr)
{
    zb_ret_t ret = RET_NOT_FOUND;

    zb_mac_power_ctrl_info_tbl_ent_t *ent;

    TRACE_MSG(TRACE_MAC2, ">> zb_mac_power_ctrl_apply_tx_power_by_ieee ieee_addr "TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ieee_addr)));

    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(MAC_PIB().phy_current_page))
    {
        ret = mac_power_ctrl_tbl_get_ent_by_ieee(ieee_addr, &ent);

        if (ret == RET_OK)
        {
            zb_mac_set_tx_power(ent->tx_power);
        }
    }

    TRACE_MSG(TRACE_MAC2, "<< zb_mac_power_ctrl_apply_tx_power_by_ieee ret 0x%x",
              (FMT__D, ret));

    return ret;
}


/* Do we really need this function - yes! */
void zb_mac_power_ctrl_restore_tx_power()
{
    TRACE_MSG(TRACE_MAC2, "zb_mac_power_ctrl_restore_tx_power", (FMT__0));

    zb_mac_set_tx_power(MAC_CTX().default_tx_power);
}

/************* MLME primitives *************/
void zb_mlme_get_power_information_table_request(zb_uint8_t param)
{
    zb_mlme_get_power_info_tbl_req_t *req;
    zb_mlme_get_power_info_tbl_conf_t *conf;
    zb_mac_power_ctrl_info_tbl_ent_t *ent;
    zb_uint16_t short_addr;
    zb_ieee_addr_t ieee_addr;
    zb_uint8_t status = MAC_SUCCESS;

    TRACE_MSG(TRACE_MAC1, ">> zb_mlme_get_power_information_table_request param %hd",
              (FMT__H, param));

    req = ZB_BUF_GET_PARAM(param, zb_mlme_get_power_info_tbl_req_t);
    short_addr = req->short_addr;
    ZB_IEEE_ADDR_COPY(ieee_addr, req->ieee_addr);

    TRACE_MSG(TRACE_MAC2, "short_addr 0x%x, ieee_addr "TRACE_FORMAT_64,
              (FMT__D_A, short_addr, TRACE_ARG_64(ieee_addr)));

    conf = ZB_BUF_GET_PARAM(param, zb_mlme_get_power_info_tbl_conf_t);

    /* Check in-params: */
    if (short_addr > 0xFFF7 &&
            (ZB_IEEE_ADDR_IS_ZERO(ieee_addr) ||
             ZB_IEEE_ADDR_IS_UNKNOWN(ieee_addr)))
    {
        /* FAIL return code should be used, but it's not determined in the spec */
        status = MAC_INVALID_PARAMETER;
    }

    if (status == MAC_SUCCESS)
    {
        zb_ret_t ret;

        if (!(ZB_IEEE_ADDR_IS_ZERO(ieee_addr) ||
                ZB_IEEE_ADDR_IS_UNKNOWN(ieee_addr)))
        {
            ret = mac_power_ctrl_tbl_get_ent_by_ieee(ieee_addr, &ent);
        }

        if (ret == RET_OK)
        {
            /* Entry found, check additionally for short address. If they don't
             * match, force an error. Or update with new address??? */
            if (short_addr != 0xFFFF &&
                    short_addr != ent->short_addr)
            {
                ret = RET_ERROR;
            }
        }
        else
        {
            ret = mac_power_ctrl_tbl_get_ent_by_short(short_addr, &ent);
        }

        TRACE_MSG(TRACE_MAC2, "Found %hd, entry %p", (FMT__H_P, (ret == RET_OK), ent));

        if (ret == RET_OK)
        {
            ZB_ASSERT(ent != NULL);

            ZB_MEMCPY(&conf->ent, ent, sizeof(zb_mac_power_ctrl_info_tbl_ent_t));
        }
        else
        {
            status = MAC_INVALID_ADDRESS;
        }
    }

    conf->status = status;

    ZB_SCHEDULE_CALLBACK(zb_mlme_get_power_information_table_confirm, param);

    TRACE_MSG(TRACE_MAC1, "<< zb_mlme_get_power_information_table_request status %hd",
              (FMT__H, status));
}


void zb_mlme_set_power_information_table_request(zb_uint8_t param)
{
    zb_mlme_set_power_info_tbl_req_t *req;
    zb_mlme_set_power_info_tbl_conf_t *conf;
    zb_mac_power_ctrl_info_tbl_ent_t *ent;
    zb_uint8_t status = MAC_SUCCESS;
    zb_ret_t ret;
    zb_int8_t tx_power;

    TRACE_MSG(TRACE_MAC1, ">> zb_mlme_set_power_information_table_request param %hd",
              (FMT__H, param));

    req = ZB_BUF_GET_PARAM(param, zb_mlme_set_power_info_tbl_req_t);

    TRACE_MSG(TRACE_MAC2, "short_addr 0x%x, ieee_addr "TRACE_FORMAT_64,
              (FMT__D_A, req->ent.short_addr, TRACE_ARG_64(req->ent.ieee_addr)));

    /* Search the same IEEE address */
    ret = mac_power_ctrl_tbl_get_ent_by_ieee(req->ent.ieee_addr, &ent);
    if (ret == RET_OK)
    {
        if (req->ent.nwk_negotiated)
        {
            zb_uint8_t idx;

            if (ent->nwk_negotiated == 0 &&
                    mac_power_ctrl_tbl_get_idx(&ent, &idx) == RET_OK)
            {
                ZB_SCHEDULE_ALARM_CANCEL(mac_power_ctrl_expire_alarm, idx);
            }

            /* Update the whole entry, but TX power */
            tx_power = ent->tx_power + req->ent.tx_power;
            ZB_MEMCPY(ent, &req->ent, sizeof(zb_mac_power_ctrl_info_tbl_ent_t));

            if (tx_power > ZB_TRANS_MAX_TX_POWER_SUB_GHZ)
            {
                tx_power = ZB_TRANS_MAX_TX_POWER_SUB_GHZ;
            }
            else if (tx_power < ZB_TRANS_MIN_TX_POWER_SUB_GHZ)
            {
                tx_power = ZB_TRANS_MIN_TX_POWER_SUB_GHZ;
            }

            ent->tx_power = tx_power;
        }
        else
        {
            /* Delete entry */
            ret = mac_power_ctrl_tbl_delete_ent(&ent);
        }
    }
    else
    {
        /* Create new entry only if NWK negotiated */
        if (req->ent.nwk_negotiated)
        {
            ret = mac_power_ctrl_tbl_create_ent(&ent);
            if (ret == RET_OK)
            {
                tx_power = MAC_CTX().default_tx_power + req->ent.tx_power;
                ZB_MEMCPY(ent, &req->ent, sizeof(zb_mac_power_ctrl_info_tbl_ent_t));
                req->ent.tx_power = tx_power;
            }
        }
    }

    if (ret != RET_OK)
    {
        status = MAC_INVALID_PARAMETER;
    }

    conf = ZB_BUF_GET_PARAM(param, zb_mlme_set_power_info_tbl_conf_t);
    conf->status = status;
    ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_confirm, param);

    TRACE_MSG(TRACE_MAC1, "<< zb_mlme_set_power_information_table_request status %hd",
              (FMT__H, status));
}
#endif  /* ZB_MAC_POWER_CONTROL && !ZB_MACSPLIT_HOST*/
