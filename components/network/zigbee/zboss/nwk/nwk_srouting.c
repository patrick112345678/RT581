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
/* PURPOSE: Network source routing
*/

#define ZB_TRACE_FILE_ID 2238
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "nwk_internal.h"
#if defined NCP_MODE && !defined NCP_MODE_HOST
#include "zb_ncp.h"
#endif

#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
/*
 * Generate and send a route record command frame.
 * Add route request entry into the rreq list to be able to track this request.
 */

static zb_nwk_rrec_t *new_source_routing_table_ent(void)
{
    zb_uindex_t i;
    zb_uint8_t expired_entry_num = 0;
    zb_nwk_rrec_t *ent = NULL;

    for (i = 0; i < ZB_NWK_MAX_SRC_ROUTES; i++)
    {
        if (!ZB_U2B(ZB_NIB().nwk_src_route_tbl[i].used))
        {
            TRACE_MSG(TRACE_NWK2, "new_source_routing_table_ent: create new entry %d", (FMT__D, i));
            ent = &ZB_NIB().nwk_src_route_tbl[i];
            ZB_NIB().nwk_src_route_cnt++;
            ZB_NIB().nwk_src_route_tbl[i].used = ZB_TRUE_U;
            break;
        }
        else
        {
            if (ZB_NIB().nwk_src_route_tbl[i].expiry == 0U)
            {
                expired_entry_num++;
            }
        }
    }

    if (ent != NULL && expired_entry_num > 0U)
    {
        /* If there aren't free entries available let's reuse random expired entry */
        zb_uint32_t random_entry_num = ZB_RANDOM_VALUE((zb_uint32_t)expired_entry_num - 1U);
        ZB_ASSERT(random_entry_num < ZB_NWK_MAX_SRC_ROUTES);

        for (i = 0; i < ZB_NWK_MAX_SRC_ROUTES; i++)
        {
            if (ZB_NIB().nwk_src_route_tbl[i].expiry == 0U)
            {
                if (random_entry_num > 0U)
                {
                    random_entry_num--;
                }
                else
                {
                    TRACE_MSG(TRACE_NWK2, "new_source_routing_table_ent: reuse expired entry %d", (FMT__D, i));
                    ent = &ZB_NIB().nwk_src_route_tbl[i];
                    break;
                }
            }
        }
    }

    return ent;
}

static zb_ret_t zb_nwk_send_rrec_int(zb_bufid_t cbuf, zb_uint16_t src_addr, zb_uint16_t dst_addr, zb_uint16_t mac_dst, zb_bool_t is_prior)
{
    zb_ret_t ret = RET_ERROR;
    zb_nwk_hdr_t *nwhdr;
    zb_nwk_cmd_rrec_t *rrec;
    zb_bool_t secure;

    secure = ZG->aps.authenticated && (ZB_NIB_SECURITY_LEVEL() > 0U);

    TRACE_MSG(TRACE_NWK2, ">> zb_nwk_send_rrec_int cbuf %hd src_addr 0x%x dst_addr 0x%x is_prior %d", (FMT__H_D_D_D, cbuf, src_addr, dst_addr, is_prior));

    /* Allocate a buffer for a new route record request */
    if (cbuf == 0U)
    {
        if (!zb_buf_is_oom_state())
        {
            cbuf = zb_buf_get_out();
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Oops - out of memory, can't initiate route rec", (FMT__0));
        }
    }

    if (cbuf != 0U)
    {
        nwhdr = nwk_alloc_and_fill_hdr(cbuf, src_addr, dst_addr, ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);

        if (src_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
        {
            /* AD: when originating command we should put 0 in relay counts, so, no relays */
            /* There're no direction on this in spec, but pass verdicts should be same */
            rrec = (zb_nwk_cmd_rrec_t *)nwk_alloc_and_fill_cmd(cbuf, ZB_NWK_CMD_ROUTE_RECORD, (zb_uint8_t)sizeof(zb_uint8_t) );
            /* AD: I don't remember why I've done this "opt" here */
            /*rrec->opt = 0;*/
            rrec->relay_cnt = 0;
            nwhdr->radius = ZB_NWK_MAX_PATH_LENGTH;
        }
        else /* it's from our end device's child */
        {
            rrec = (zb_nwk_cmd_rrec_t *)nwk_alloc_and_fill_cmd(cbuf, ZB_NWK_CMD_ROUTE_RECORD, (zb_uint8_t)sizeof(zb_uint8_t) + (zb_uint8_t)sizeof(zb_uint16_t));
            rrec->relay_cnt = 1;
            rrec->relay_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
            nwhdr->radius = ZB_NWK_MAX_PATH_LENGTH - 1U;
            ZB_NWK_ADDR_TO_LE16(rrec->relay_addr);
        }

        (void)zb_nwk_init_apsde_data_ind_params(cbuf, ZB_NWK_INTERNAL_NSDU_HANDLE);

        if (is_prior)
        {
            /* Build MCPS Data request here to be able to send RREC prior to send a packet.
               All checks should be done earlier.
             */
            zb_mcps_build_data_request(cbuf, ZB_PIBCACHE_NETWORK_ADDRESS(), mac_dst,
                                       MAC_TX_OPTION_ACKNOWLEDGED_BIT,
                                       ZB_NWK_INTERNAL_NSDU_HANDLE);

            ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, cbuf);
            /* 10/21/2019 EE CR:MINOR No big difference, but better change endian
             * before scheduling data req. It is more linear that knowing that
             * data req will not be executed until return from that function. */
            /* Convert addresses to LE order */
            ZB_NWK_ADDR_TO_LE16(nwhdr->dst_addr);
            ZB_NWK_ADDR_TO_LE16(nwhdr->src_addr);
        }
        else
        {
            /* Use the general NWK mechanism */
            ZB_SCHEDULE_CALLBACK(zb_nwk_forward, cbuf);
        }
#if defined NCP_MODE && !defined NCP_MODE_HOST && defined ZB_APSDE_REQ_ROUTING_FEATURES && defined ZB_NCP_ENABLE_ROUTING_IND
        ncp_nwk_route_send_rrec_ind(rrec);
#endif
        ret = RET_OK;
    }

    TRACE_MSG(TRACE_NWK2, "<< zb_nwk_send_rrec_int %d", (FMT__D, ret));
    return ret;
}

zb_ret_t zb_nwk_send_rrec(zb_bufid_t cbuf, zb_uint16_t src_addr, zb_uint16_t dst_addr)
{
    return zb_nwk_send_rrec_int(cbuf, src_addr, dst_addr, ZB_MAC_SHORT_ADDR_NO_VALUE, ZB_FALSE);
}

zb_ret_t zb_nwk_send_rrec_prior(zb_bufid_t cbuf, zb_uint16_t src_addr, zb_uint16_t dst_addr, zb_uint16_t mac_dst)
{
    return zb_nwk_send_rrec_int(cbuf, src_addr, dst_addr, mac_dst, ZB_TRUE);
}

void zb_nwk_rrec_handler(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_uint8_t hdr_size)
{
    TRACE_MSG(TRACE_NWK2, ">>zb_nwk_rrec_handler: dst 0x%x", (FMT__D, nwk_hdr->dst_addr));

    if (nwk_hdr->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        /* AD: fixme, need some workaround to avoid source routing table overflow blocking */
        if (ZB_NIB_SRCRT_CNT() <= ZB_NWK_MAX_SRC_ROUTES)
        {
            /* adding source record to the table */
            zb_nwk_rrec_t *rrec;
            zb_uint8_t i;

            NWK_ARRAY_FIND_ENT(ZB_NIB().nwk_src_route_tbl, ZB_NWK_MAX_SRC_ROUTES, rrec, rrec->addr == nwk_hdr->src_addr);
            if (rrec == NULL)
            {
                TRACE_MSG(TRACE_NWK2, "no entry found, creating new one", (FMT__0));
                rrec = new_source_routing_table_ent();
                if (rrec == NULL)
                {
                    TRACE_MSG(TRACE_NWK2, "no free route table entries are left", (FMT__0));
                }
            }

            if (rrec != NULL)
            {
                TRACE_MSG(TRACE_NWK2, "processing route record: src_addr 0x%x, count %hd", (FMT__D_H, nwk_hdr->src_addr, rrec->count));
                rrec->expiry =
#if ZB_NWK_MAX_SRC_ROUTES > 1U
                    ZB_NWK_SRC_ROUTE_TABLE_EXPIRY;
#else
                    0U;
#endif
                rrec->addr = nwk_hdr->src_addr;
                rrec->count = *((zb_uint8_t *)nwk_hdr + hdr_size + 1);
                for (i = 0; i < rrec->count; i++)
                {
                    /* first +1 is to skip relay counts, second is shift to the start of relays block */
                    ZB_MEMCPY(&rrec->path[i], ((zb_uint8_t *)nwk_hdr + hdr_size + 1 + (zb_uint8_t)sizeof(zb_uint16_t)*i + 1), 2);
                    ZB_NWK_ADDR_TO_LE16(rrec->path[i]);
                    TRACE_MSG(TRACE_NWK2, "processing route record: add node 0x%x", (FMT__D, rrec->path[i]));
                }
                TRACE_MSG(TRACE_ATM1, "Z< processing route record: src_addr 0x%x, count %hd last node 0x%x", (FMT__D_H_H, nwk_hdr->src_addr, rrec->count, (rrec->count > 0) ? rrec->path[(rrec->count - 1)] : 0xFFFF));
            }
        }
        else
        {
            TRACE_MSG(TRACE_NWK2, "Source routing table not established or no entries left", (FMT__0));
        }
        zb_buf_free(buf);
    }
    else
    {
        zb_uint8_t *relay;
        /* forwarding route record */
        TRACE_MSG(TRACE_NWK2, "forwarding route record ", (FMT__0));
        TRACE_MSG(TRACE_ATM1, "Z< forwarding route record:  src_addr 0x%x count %hd  add node 0x%x", (FMT__H_D_H, nwk_hdr->src_addr, (*(zb_uint8_t *)((zb_uint8_t *)nwk_hdr + hdr_size + 1)), ZB_PIBCACHE_NETWORK_ADDRESS()));
        (*(zb_uint8_t *)((zb_uint8_t *)nwk_hdr + hdr_size + 1))++; /* AD: inc relay count */
        relay = zb_buf_alloc_right(buf, sizeof (zb_uint16_t));
        ZB_HTOLE16(relay, &ZB_PIBCACHE_NETWORK_ADDRESS());
        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, buf);
    }
}

void zb_send_nwk_status_source_route_fail(zb_uint8_t param, zb_uint16_t src_addr)
{
    zb_nlme_send_status_t *req;
    TRACE_MSG(TRACE_APS3, "zb_send_nwk_status_source_route_fail: send network status = 0x0B;", (FMT__0));
    req = ZB_BUF_GET_PARAM(param, zb_nlme_send_status_t);
    /*
    3.4.3.2 NWK Header Fields

    When sent in response to a routing error, the destination address field in the
    NWK header shall be set to the same value as the source address field of
    the data frame that encountered a forwarding failure
     */
    req->dest_addr = src_addr;
    req->status.status = ZB_NWK_COMMAND_STATUS_SOURCE_ROUTE_FAILURE;
    /* FIXME: races? */
    req->status.network_addr = ZG->nwk.handle.status_addr;
    req->ndsu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
    ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, param);
}

static zb_ret_t zb_nwk_source_route_get_random_neighbor(zb_uint16_t *random_addr, zb_uint16_t excluded_addr1, zb_uint16_t excluded_addr2)
{
    zb_neighbor_tbl_ent_t *nbt;
    zb_uint8_t nbt_size = 0;
    zb_uint16_t excluded_addr;
    zb_uint8_t excluded_addr_count = 0;
    zb_uint8_t i;

    TRACE_MSG(TRACE_NWK2, ">> zb_nwk_source_route_get_random_neighbor excluded_addr1 0x%x excluded_addr2 0x%x", (FMT__D_D, excluded_addr1, excluded_addr2));

    ZB_ASSERT(random_addr);

    /* Check if devices that should be excluded are our neighbors */
    for (i = 0; i < 2U; i++)
    {
        excluded_addr = (i == 0U ? excluded_addr1 : excluded_addr2);

        if (zb_nwk_neighbor_get_by_short(excluded_addr, &nbt) == RET_OK &&
                (nbt->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR ||
                 nbt->device_type == ZB_NWK_DEVICE_TYPE_ROUTER))
        {
            excluded_addr_count++;
        }
    }

    /* Get ZR/ZC neighbor count (minus excluded routers) */
    if (zb_nwk_neighbor_get_zc_zr_cnt() > excluded_addr_count)
    {
        nbt_size = zb_nwk_neighbor_get_zc_zr_cnt() - excluded_addr_count;
    }

    if (nbt_size != 0U)
    {
        /* Calculate random neighbor entry number */
        zb_uint32_t random_nbt_num = ZB_RANDOM_VALUE((zb_uint32_t)nbt_size - 1U);
        ZB_ASSERT(random_nbt_num < ZB_NEIGHBOR_TABLE_SIZE);

        /* Searching for the desired random entry */
        for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
        {
            if (zb_nwk_neighbor_get_by_idx(i, &nbt) == RET_OK &&
                    (nbt->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR ||
                     nbt->device_type == ZB_NWK_DEVICE_TYPE_ROUTER))
            {
                zb_address_short_by_ref(random_addr, nbt->u.base.addr_ref);

                if (*random_addr != excluded_addr1 &&
                        *random_addr != excluded_addr2)
                {
                    if (random_nbt_num != 0U)
                    {
                        /* Skip this router */
                        random_nbt_num--;
                    }
                    else
                    {
                        /* Found the desired random entry */
                        TRACE_MSG(TRACE_NWK2, "<< zb_nwk_source_route_get_random_neighbor random_addr 0x%x", (FMT__D, *random_addr));
                        return RET_OK;
                    }
                }
            }
        }
    }

    TRACE_MSG(TRACE_NWK2, "<< zb_nwk_source_route_get_random_neighbor can't find neighbor", (FMT__0));

    return RET_NOT_FOUND;
}

void zb_nwk_many_to_one_route_failure(zb_bufid_t buf)
{
    zb_mcps_data_confirm_params_t *confirm = ZB_BUF_GET_PARAM(buf, zb_mcps_data_confirm_params_t);
    zb_uint16_t mac_dst = confirm->dst_addr.addr_short;
    zb_nlme_status_indication_t *status_cmd;
    zb_nwk_hdr_t *nwk_hdr;
    zb_uint16_t nwk_src_addr, nwk_dst_addr;
    zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
    zb_ret_t ret;

    TRACE_MSG(TRACE_NWK1, "zb_nwk_many_to_one_route_failure %hd", (FMT__H, buf));

    /* Not routed so no MAC header for sure. NWK header at buf begin. */
    nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(buf);
    nwk_src_addr = nwk_hdr->src_addr;
    nwk_dst_addr = nwk_hdr->dst_addr;

    if (ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND
            /* can't use zb_nwk_cmd_frame_get_cmd_id: not buf begin */

            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be
             * a false positive. There are no side effect to 'zb_get_nwk_header_size()'. This
             * violation seems to be caused by the fact that 'zb_get_nwk_header_size()' is an
             * external function, which cannot be analyzed by C-STAT. */
            && ((zb_uint8_t *)nwk_hdr)[zb_get_nwk_header_size(nwk_hdr)] == ZB_NWK_CMD_NETWORK_STATUS)
    {
        status_cmd = (zb_nlme_status_indication_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(buf, zb_get_nwk_header_size(nwk_hdr));
        if (status_cmd->status == ZB_NWK_COMMAND_STATUS_MANY_TO_ONE_ROUTE_FAILURE)
        {
            /* Upon receipt of the network status command frame, if no routing table entry for
               the destination is present, or if delivery of the network  status command frame
               to the next hop in the routing table entry fails, the network status command
               frame shall again be unicast to a random router neighbor.
            */
            if (nwk_hdr->radius > 1U)
            {
                nwk_hdr->radius--;
            }
            else
            {
                /* Stop processing Network status command */
                zb_buf_free(buf);
                return;
            }
        }
        else
        {
            /* No failure should be reported in case of Network Status command */
            zb_buf_free(buf);
            return;
        }
    }
    else
    {
        /* Build new Network status command */
        nwk_hdr = nwk_alloc_and_fill_hdr(buf, ZB_PIBCACHE_NETWORK_ADDRESS(), nwk_dst_addr, ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);
        nwk_hdr->radius = ZB_NIB_MAX_DEPTH() * 2U;

        if (secure)
        {
            nwk_mark_nwk_encr(buf);
        }

        status_cmd = (zb_nlme_status_indication_t *)nwk_alloc_and_fill_cmd(buf, ZB_NWK_CMD_NETWORK_STATUS, (zb_uint8_t)sizeof(zb_nlme_status_indication_t) - 1U);
        status_cmd->status = ZB_NWK_COMMAND_STATUS_MANY_TO_ONE_ROUTE_FAILURE;
        status_cmd->network_addr = nwk_src_addr;
        ZB_NWK_ADDR_TO_LE16(status_cmd->network_addr);

        (void)zb_nwk_init_apsde_data_ind_params(buf, ZB_NWK_INTERNAL_NSDU_HANDLE);
    }

    /* Get random neighbor address except the packet originator device and the failed router */
    ret = zb_nwk_source_route_get_random_neighbor(&mac_dst, nwk_src_addr, mac_dst);
    if (ret != RET_OK)
    {
        /* There are no other neighbours. Skip sending Network status command */
        zb_buf_free(buf);
        return;
    }

    TRACE_MSG(TRACE_NWK1, "Send Many-to-one route failure status to 0x%x", (FMT__D, mac_dst));
    TRACE_MSG(TRACE_ATM1, "Send Many-to-one route failure status to 0x%x", (FMT__D, mac_dst));

    /* Build MCPS Data request here to be able to send
       Network status command to a random neighbor.
     */
    zb_mcps_build_data_request(buf, ZB_PIBCACHE_NETWORK_ADDRESS(), mac_dst,
                               MAC_TX_OPTION_ACKNOWLEDGED_BIT,
                               ZB_NWK_INTERNAL_NSDU_HANDLE);

    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, buf);

    /* Convert addresses to LE order */
    ZB_NWK_ADDR_TO_LE16(nwk_hdr->dst_addr);
    ZB_NWK_ADDR_TO_LE16(nwk_hdr->src_addr);
}

void zb_nwk_source_route_table_expiry(void)
{
    if (ZB_NIB_SRCRT_CNT() <= ZB_NWK_MAX_SRC_ROUTES)
    {
        zb_ushort_t i;

        for (i = 0; i < ZB_NWK_MAX_SRC_ROUTES; i++)
        {
            if (ZB_U2B(ZB_NIB().nwk_src_route_tbl[i].used))
            {
                TRACE_MSG(TRACE_NWK3, "source_route_table ent %d addr 0x%x exp %hd",
                          (FMT__D_D_H, i, ZB_NIB().nwk_src_route_tbl[i].addr, ZB_NIB().nwk_src_route_tbl[i].expiry));

                if (ZB_NIB().nwk_src_route_tbl[i].expiry != 0U)
                {
                    --(ZB_NIB().nwk_src_route_tbl[i].expiry);
                }
            }
        }
    }
}

void zb_nwk_source_routing_clear_rrec_req(zb_uint16_t src_addr)
{
    zb_nwk_routing_t *routing_ent;

    NWK_ARRAY_FIND_ENT(ZB_NIB().routing_table, ZB_NWK_ROUTING_TABLE_SIZE, routing_ent, (routing_ent->dest_addr == src_addr));
    if (routing_ent != NULL &&
            ZB_U2B(routing_ent->many_to_one) &&
            !ZB_U2B(routing_ent->no_route_cache))
    {
        routing_ent->route_record_required = ZB_FALSE;
    }
}

static void zb_nwk_source_route_discovery(zb_bufid_t buf)
{
    zb_nlme_route_discovery_request_t *rreq;

    TRACE_MSG(TRACE_NWK1, "zb_nwk_start_source_route_discovery %hd", (FMT__H, buf));

    /* 10/21/2019 EE CR:MINOR ASSERT would be enough */
    if (ZB_NIB_GET_IS_CONCENTRATOR())
    {
        rreq = ZB_BUF_GET_PARAM(buf, zb_nlme_route_discovery_request_t);
        rreq->address_mode = ZB_ADDR_NO_ADDR;
        rreq->network_addr = ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
        rreq->radius = ZB_NIB_GET_CONCENTRATOR_RADIUS();
        rreq->no_route_cache = (ZB_NWK_MAX_SRC_ROUTES <= 1U);

        /* Start Source route discovery */
        zb_nlme_route_discovery_request(buf);
    }
    else
    {
        /* Concentrator mode is disabled */
        zb_buf_free(buf);
    }
}

static void zb_nwk_start_route_discovery_loop(zb_bufid_t buf)
{
    ZVUNUSED(buf);

    TRACE_MSG(TRACE_NWK2, "zb_nwk_start_source_route_discovery_loop: is_concentrator %hd radius %hd disc_time %d",
              (FMT__H_H_D, ZB_NIB_GET_IS_CONCENTRATOR(), ZB_NIB_GET_CONCENTRATOR_RADIUS(), ZB_NIB_GET_CONCENTRATOR_DISC_TIME()));

    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_start_route_discovery_loop, ZB_ALARM_ANY_PARAM);

    if (ZB_NIB_GET_IS_CONCENTRATOR())
    {
        zb_ret_t ret = zb_buf_get_out_delayed(zb_nwk_source_route_discovery);
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }

        /* Schedule next route discovery request */
        if (ZB_NIB_GET_CONCENTRATOR_DISC_TIME() != 0U)
        {
            ZB_SCHEDULE_ALARM(zb_nwk_start_route_discovery_loop, 0, ZB_TIME_ONE_SECOND * ZB_NIB_GET_CONCENTRATOR_DISC_TIME());
        }
    }
}

void zb_nwk_concentrator_start(void)
{
    if (ZB_NIB_GET_IS_CONCENTRATOR())
    {
        /* Start Route discovery loop after delay */
        ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_start_route_discovery_loop, ZB_ALARM_ANY_PARAM);
        /* 10/21/2019 EE CR:MINOR Is that 1s time specified somewhere? Why do not start immediately? */
        ZB_SCHEDULE_ALARM(zb_nwk_start_route_discovery_loop, 0, ZB_TIME_ONE_SECOND);
    }
}

void zb_nwk_concentrator_stop(void)
{
    /* Stop Route discovery loop */
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_start_route_discovery_loop, ZB_ALARM_ANY_PARAM);
}

#endif  /* pro stack & source routing */
