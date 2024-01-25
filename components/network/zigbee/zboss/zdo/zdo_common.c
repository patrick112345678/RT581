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
/* PURPOSE: ZDO common functions, both client and server side
*/

#define ZB_TRACE_FILE_ID 2095
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_hash.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"

/*! \addtogroup ZB_ZDO */
/*! @{ */

static zb_uint8_t zdo_send_req(zb_uint8_t param, zb_callback_t cb, zb_uint8_t resp_counter);

/*
 * Why do we pass resp_counter set to ZB_ZDO_CB_DEFAULT_COUNTER to the function
 * zdo_send_req_by_short()? Indeed, we should know, how many ZDO
 * responses we are waiting. Can this be done by analysis of
 * short_address field / consulting with specification to ZDO commands?
 * Yes, at least at part of even broadcast requests we need just 1 answer.
 */

zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_callback_t cb, zb_uint16_t addr,
                                 zb_uint8_t resp_counter)
{
    zb_uint8_t           tsn;
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO2, ">> zdo_send_req_by_short param %hd", (FMT__H, param));
    ZB_BZERO(dreq, sizeof(*dreq));
    dreq->dst_addr.addr_short = addr;
    if (!ZB_NWK_IS_ADDRESS_BROADCAST(addr))
    {
        dreq->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    }
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    dreq->clusterid = command_id;
    /* resp_counter usage:
       - together with ZB_ZDO_CB_UNICAST_COUNTER and ZB_ZDO_CB_BROADCAST_COUNTER define ZB_ZDO_CB_DEFAULT_COUNTER 0xFF
       - for all the calls where 1 is specified, set ZB_ZDO_CB_DEFAULT_COUNTER
       - in this func check, if ZB_ZDO_CB_DEFAULT_COUNTER then apply the logic below */
    if (resp_counter == ZB_ZDO_CB_DEFAULT_COUNTER)
    {
        resp_counter = ZB_NWK_IS_ADDRESS_BROADCAST(addr) ? ZB_ZDO_CB_BROADCAST_COUNTER : ZB_ZDO_CB_UNICAST_COUNTER;
    }
    tsn = zdo_send_req(param, cb, resp_counter);
    TRACE_MSG(TRACE_ZDO2, "<< zdo_send_req_by_short ", (FMT__0));
    return tsn;
}


zb_uint8_t zdo_send_req_by_long(zb_uint8_t command_id, zb_uint8_t param, zb_callback_t cb,
                                zb_ieee_addr_t addr)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    ZB_BZERO(dreq, sizeof(*dreq));
    ZB_IEEE_ADDR_COPY(dreq->dst_addr.addr_long, addr);
    dreq->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    dreq->clusterid = command_id;
    dreq->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    return zdo_send_req(param, cb, 1);
}

static zb_uint8_t zdo_send_req(zb_uint8_t param, zb_callback_t cb, zb_uint8_t resp_counter)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
    zb_uint8_t           cb_tsn;
    zb_uint8_t rx_on_when_idle;
    zb_bool_t success = ZB_TRUE;

    ZDO_TSN_INC();

    TRACE_MSG(TRACE_ZDO2, ">> zdo_send_req, tsn %hd", (FMT__H, ZDO_CTX().tsn));

    {
        zb_uint8_t *tsn_p;
        tsn_p = zb_buf_alloc_left(param, 1);
        *tsn_p = ZDO_CTX().tsn;
    }
#ifndef ZB_LITE_NO_ZDO_SYSTEM_SERVER_DISCOVERY
    if (dreq->clusterid == ZDO_SYSTEM_SERVER_DISCOVERY_REQ_CLID)
    {
        ZDO_CTX().system_server_discovery_tsn = ZDO_CTX().tsn;
    }
#endif
    if (dreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
        rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_by_ieee(dreq->dst_addr.addr_long);
        /*
          If rx_on_when_idle wasn't successfully retrieved,
          lets assume that req is sent to sleepy dev
        */
        if (rx_on_when_idle == ZB_NWK_NEIGHBOR_ERROR_VALUE)
        {
            rx_on_when_idle = ZB_FALSE;
        }
    }
    else
    {
        rx_on_when_idle = zb_nwk_get_nbr_rx_on_idle_short_or_false(dreq->dst_addr.addr_short);
    }

    if (cb != NULL)
    {
        success = register_zdo_cb(ZDO_CTX().tsn, cb, resp_counter, CB_TYPE_TSN,
                                  (zb_bool_t)rx_on_when_idle);
    }

    if (success)
    {
        cb_tsn = ZDO_CTX().tsn;
        ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);

        /* suppose 1 resp */
        TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
        zb_zdo_pim_start_turbo_poll_packets(1);
    }
    else
    {
        ZDO_TSN_DEC();
        cb_tsn = 0xFF;
    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_send_req frame tsn: %hd", (FMT__H, cb_tsn));
    return cb_tsn;
}

void zdo_send_resp_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t addr)
{
    zb_apsde_data_req_t *dreq = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);

    TRACE_MSG(TRACE_ZDO3, "zdo_send_resp_by_short, command_id %d, param %hd, addr %d", (FMT__D_H_D, command_id, param, addr));
    ZB_BZERO(dreq, sizeof(*dreq));
    dreq->dst_addr.addr_short = addr;
    if ( !ZB_NWK_IS_ADDRESS_BROADCAST(addr) && (command_id != ZDO_MGMT_LEAVE_RESP_CLID) )
    {
        dreq->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    }
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    dreq->clusterid = command_id;

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}

static void zdo_kill_cb_by_index(zb_uint8_t buf_ref, zb_uint16_t index)
{
    zdo_cb_hash_ent_t *ent = &ZDO_CTX().zdo_cb[ZB_GET_LOW_BYTE(index)];

    TRACE_MSG(TRACE_ZDO1, "> zdo_kill_cb_by_index, tsn 0x%hx, index %hd",
              (FMT__H_H_H, ZB_GET_HI_BYTE(index), ZB_GET_LOW_BYTE(index)));

    if (ent->func != NULL)
    {
        zb_zdo_callback_info_t *zdo_cb_info;

        zdo_cb_info = zb_buf_initial_alloc(buf_ref, sizeof(zb_zdo_callback_info_t));
        zdo_cb_info->tsn = (zb_uint8_t)ZB_GET_HI_BYTE(index);
        zdo_cb_info->status = ZB_ZDP_STATUS_TIMEOUT;

        ZB_SCHEDULE_CALLBACK(ent->func, buf_ref);

        /* mark entry as free */
        ent->func = NULL;
        ent->tsn = ZB_ZDO_INVALID_TSN;
        ent->type = CB_TYPE_DEFAULT;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "zdo_kill_cb_by_index func is NULL!", (FMT__0));
        zb_buf_free(buf_ref);
    }

    TRACE_MSG(TRACE_ZDO1, "< zdo_kill_cb_by_index", (FMT__0));
}

static void zdo_cb_killer(zb_uint8_t param)
{
    zb_uint16_t i;
    zb_bool_t   is_found = ZB_FALSE;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, "> zdo_cb_killer", (FMT__0));

    for (i = 0; i < ZDO_TRAN_TABLE_SIZE; i++)
    {
        zdo_cb_hash_ent_t *ent = &ZDO_CTX().zdo_cb[i];

        if (((ent->type == CB_TYPE_TSN && ent->tsn != ZB_ZDO_INVALID_TSN) ||
                (ent->type != CB_TYPE_TSN)) && ent->func != NULL)
        {
            ZDO_CTX().zdo_cb[i].clock_counter--;

            if (ZDO_CTX().zdo_cb[i].clock_counter == 0U &&
                    ((ent->type != CB_TYPE_TSN) ||
                     (ent->type == CB_TYPE_TSN && ent->tsn != ZB_ZDO_INVALID_TSN)))
            {
                zb_uint16_t buf_ref = i;

                ZB_SET_HI_BYTE(buf_ref, (zb_uint16_t)ent->tsn);

                if (ent->type == CB_TYPE_TSN)
                {
                    ent->tsn = ZB_ZDO_INVALID_TSN;
                }

                if (zb_buf_get_out_delayed_ext(zdo_kill_cb_by_index, buf_ref, 0) != RET_OK)
                {
                    //Unfortunatelly, no free slot for register callback
                    //Potentially memory leak!!!! in case when some buf wait us, for instance, zdo nwk addr request
                    //in case when apsde_request schedule it for resolve short addr
                    TRACE_MSG(TRACE_ERROR, "zdo_cb_killer ZB_GET_OUT_BUF_DELAYED2 failed", (FMT__0));
                    ent->func = NULL;
                    ent->type = CB_TYPE_DEFAULT;
                }
            }
            else
            {
                is_found = ZB_TRUE;
            }
        }
    }

    if (is_found == ZB_TRUE)
    {
        ZB_SCHEDULE_ALARM(zdo_cb_killer, 0, ZB_ZDO_CB_KILLER_QUANT);
    }

    TRACE_MSG(TRACE_ZDO1, "< zdo_cb_killer", (FMT__0));
}

void zdo_cb_reset()
{
    zb_ushort_t i;
    TRACE_MSG(TRACE_ZDO1, "> zdo_cb_reset", (FMT__0));

    for (i = 0; i < ZDO_TRAN_TABLE_SIZE; i++)
    {
        /* mark as free */
        ZDO_CTX().zdo_cb[i].func = NULL;
        ZDO_CTX().zdo_cb[i].tsn = ZB_ZDO_INVALID_TSN;
    }

    TRACE_MSG(TRACE_ZDO1, "< zdo_cb_reset", (FMT__0));
}

zb_bool_t register_zdo_cb(zb_uint8_t tsn, zb_callback_t cb,
                          zb_uint8_t resp_counter, zb_uint8_t cb_type,
                          zb_bool_t rx_on_when_idle)
{
    zb_ushort_t h_i = ZB_TSN_HASH(tsn);
    zb_ushort_t i = h_i;

    TRACE_MSG(TRACE_ZDO1, "register_zdo_cb tsn 0x%hx resp_counter %hd cb_type %hd", (FMT__H_H_H, tsn, resp_counter, cb_type));

    if (cb != NULL)
    {
        zb_time_t dummy_time;

        if (zb_schedule_get_alarm_time(zdo_cb_killer, 0, &dummy_time) == RET_NOT_FOUND)
        {
            TRACE_MSG(TRACE_ZDO1, "set alarm for zdo_cb_killer", (FMT__0));
            ZB_SCHEDULE_ALARM(zdo_cb_killer, 0, ZB_ZDO_CB_KILLER_QUANT);
        }

        do
        {
            if (((cb_type == CB_TYPE_TSN && ZDO_CTX().zdo_cb[i].tsn == ZB_ZDO_INVALID_TSN) ||
                    (cb_type != CB_TYPE_TSN)) && ZDO_CTX().zdo_cb[i].func == NULL)
            {
                /* found free slot */
                ZDO_CTX().zdo_cb[i].tsn = tsn;
                ZDO_CTX().zdo_cb[i].func = cb;
                ZDO_CTX().zdo_cb[i].resp_counter = resp_counter;
                ZDO_CTX().zdo_cb[i].type = cb_type;
                ZDO_CTX().zdo_cb[i].clock_counter = (zb_uint8_t)ZB_ZDO_CB_CLOCK_COUNTER(rx_on_when_idle);
                TRACE_MSG(TRACE_ZDO1, "register_zdo_cb i %d type %hd tsn 0x%hx", (FMT__D_H_H, i, cb_type, tsn));
                return ZB_TRUE;
            }
            i = (i + 1U) % ZDO_TRAN_TABLE_SIZE;
        } while ( i != h_i );
        TRACE_MSG(TRACE_ERROR, "No free space for ZDO cb, tsn 0x%hx", (FMT__H, tsn));
    }
    return ZB_FALSE;
}


/**
   Call user's routine on ZDO response

   This routine called when AF send packet to ZDO, ZDO parse it and detect rsp
   (cluster id starts from 0x8000).
 */
zb_ret_t zdo_af_resp(zb_uint8_t param)
{
    zb_uint8_t  tsn = *(zb_uint8_t *)zb_buf_begin(param);
    zb_ushort_t h_i = ZB_TSN_HASH(tsn);
    zb_ushort_t i = h_i;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_af_resp 0x%hx", (FMT__H, tsn));

    do
    {
        if (ZDO_CTX().zdo_cb[i].tsn == tsn
                && ZDO_CTX().zdo_cb[i].func != NULL
                && ZDO_CTX().zdo_cb[i].type == CB_TYPE_TSN)
        {
            /* found - schedule it to execution */
            ZB_SCHEDULE_CALLBACK(ZDO_CTX().zdo_cb[i].func, param);

            ZDO_CTX().zdo_cb[i].resp_counter--;
            TRACE_MSG(TRACE_ZDO2, "<< zdo_af_resp func %p param %hd resp_counter %d RET_OK", (FMT__P_H_D, ZDO_CTX().zdo_cb[i].func, param, ZDO_CTX().zdo_cb[i].resp_counter));

            if (ZDO_CTX().zdo_cb[i].resp_counter == 0U)
            {
                /* mark as free */
                ZDO_CTX().zdo_cb[i].func = NULL;
                ZDO_CTX().zdo_cb[i].tsn = ZB_ZDO_INVALID_TSN;
                ZDO_CTX().zdo_cb[i].type = CB_TYPE_DEFAULT;
            }
            TRACE_MSG(TRACE_ZDO2, "<< zdo_af_resp", (FMT__0));
            return RET_OK;
        }
        i = (i + 1U) % ZDO_TRAN_TABLE_SIZE;
    } while ( i != h_i );
    TRACE_MSG(TRACE_ZDO1, "zdo tsn 0x%hx not found!", (FMT__H, tsn));
    /* zb_buf_free(param); free buffer on caller level */
    TRACE_MSG(TRACE_ZDO2, "<< zdo_af_resp NOT FOUND", (FMT__0));
    return RET_NOT_FOUND;
}

zb_ret_t zdo_run_cb_by_index(zb_uint8_t param)
{
    zb_ushort_t h_i = ZB_TSN_HASH(param);
    zb_ushort_t i = h_i;

    TRACE_MSG(TRACE_ZDO2, ">>zdo_run_cb_by_index %hd", (FMT__H, param));
    do
    {
        if (ZDO_CTX().zdo_cb[i].tsn == param
                && ZDO_CTX().zdo_cb[i].func != NULL
                && ZDO_CTX().zdo_cb[i].type == CB_TYPE_INDEX)
        {
            TRACE_MSG(TRACE_ZDO3, ">>zdo_run_cb_by_index: run callback with param %hd", (FMT__H, param));
            ZB_SCHEDULE_CALLBACK(ZDO_CTX().zdo_cb[i].func, param);

            ZDO_CTX().zdo_cb[i].resp_counter--;
            if (ZDO_CTX().zdo_cb[i].resp_counter == 0U)
            {
                /* mark as free */
                ZDO_CTX().zdo_cb[i].func = NULL;
            }
            TRACE_MSG(TRACE_ZDO2, "<<zdo_run_cb_by_index RET_OK", (FMT__0));
            return RET_OK;
        }
        i = (i + 1U) % ZDO_TRAN_TABLE_SIZE;
    } while ( i != h_i );
    TRACE_MSG(TRACE_ZDO1, "zdo cb by index 0x%hx not found!", (FMT__H, param));
    TRACE_MSG(TRACE_ZDO2, "<<zdo_run_cb_by_index NOT FOUND", (FMT__0));
    return RET_NOT_FOUND;
}


void zb_zdo_register_leave_indication_cb(zb_callback_t cb)
{
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_register_leave_indication_cb cb %p", (FMT__P, cb));
    ZG->zdo.leave_ind_cb = cb;
}

void zb_zdo_get_diag_data(zb_uint16_t short_address, zb_uint8_t *lqi, zb_int8_t *rssi)
{
    TRACE_MSG(TRACE_ZDO1, ">> zb_zdo_get_diag_data short_address 0x%hx",
              (FMT__D, short_address));
#ifdef ZB_MAC_SPECIFIC_GET_LQI_RSSI
    zb_mac_diag_data_get(short_address, lqi, rssi);
#else
    {
        zb_neighbor_tbl_ent_t *nbt;

        if (RET_OK == zb_nwk_neighbor_get_by_short(short_address, &nbt))
        {
            *lqi = nbt->lqi;
#ifndef ZB_LITE_DONT_STORE_RSSI
            *rssi = nbt->rssi;
#else
            /* For platform which have LQI only create RSSI from LQI (not sure how to do it..) */
            *rssi = (-(*lqi) / 2);
#endif
        }
        else
        {
            *lqi = ZB_MAC_LQI_UNDEFINED;
            *rssi = (zb_int8_t)ZB_MAC_RSSI_UNDEFINED;
        }
    }
#endif

    TRACE_MSG(TRACE_ZDO1, "<< zb_zdo_get_diag_data short_address 0x%hx lqi %u rssi %d",
              (FMT__D_H_H, short_address, *lqi, *rssi));
}

/*! @} */
