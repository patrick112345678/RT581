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
/* PURPOSE: Network address assign
*/
#define ZB_TRACE_FILE_ID 326
#include <stdlib.h>

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_secur.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_zdo.h"
#include "zb_watchdog.h"
#include "zb_bufpool.h"


/* See 3.4.8 Link Status Command */

#if defined ZB_PRO_STACK && defined ZB_ROUTER_ROLE    /* zigbee pro and router */

/**
   Calculates the maximum statuses for one NWK command
   Moved from zb_nwk_send_link_status_command() with comments
 */
static zb_uint8_t zb_nwk_get_link_status_max_count(void)
{
    /* 6/5/2017 EE CR:MINOR Seems we assume here src IEEE address
     * presence. Can we have a situation (certification?) when we must
     * remove 8 bytes of src ieee and put more entries there? */
    /* NK: Looks like we do not have such test, but can add certification define here if needed. */
    return (ZB_NWK_MAX_BROADCAST_PAYLOAD_SIZE - 2U) /* link status cmd id (1b) +
                                                   * first/last/count (1b)*/
           / ZB_LINK_STATUS_SIZE;
}

/*! \addtogroup ZB_NWK */
/*! @{ */

/*
 * Link Status command
 * Add age for all neighbor
 * If coordinator or router then send Link Status requests broadcast
 */
void zb_nwk_link_status_alarm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_link_status_alarm param %d", (FMT__H, param));
    ZVUNUSED(param);
    if (ZB_JOINED())
    {
        zb_nwk_add_age_neighbor();
        if (zb_buf_get_out_delayed(zb_nwk_send_link_status_command) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops - out of memory - skip send_link_status_command", (FMT__0));
            //ZB_SCHEDULE_ALARM(zb_nwk_link_status_alarm, 0, ZB_NWK_JITTER(ZB_SECONDS_TO_BEACON_INTERVAL(ZB_NIB_GET_LINK_STATUS_PERIOD())));
        }
    }
    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_link_status_alarm", (FMT__0));
}

/*
 * Add age all neighbor
 * see spec 3.6.3.4.2
 */
void zb_nwk_add_age_neighbor()
{
    zb_uindex_t i;
#ifndef ZB_COORDINATOR_ONLY
    zb_bool_t all_routers_expired = ZB_TRUE;
#endif

    for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
    {
        if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used) && !ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor))
        {

#ifdef ZB_RAF_NEIGHBOR_AUTO_DELETETION
            /* neighbor offline over 120s, it should be delete from neighbor table */
            if (ZG->nwk.neighbor.neighbor[i].u.base.age >= ZB_RAF_NWK_NEIGHBOR_ENTRY_TIMEOUT_AGE)
            {
                if ((ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR ||
                        ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ROUTER) &&
                        ZG->nwk.neighbor.neighbor[i].u.base.outgoing_cost == 0U
                   )
                {

                    /* Delete entry from the NBT */
                    zb_ret_t ret = zb_nwk_neighbor_delete(ZG->nwk.neighbor.neighbor[i].u.base.addr_ref);
                    zb_uint16_t short_addr;

                    zb_address_short_by_ref(&short_addr, ZG->nwk.neighbor.neighbor[i].u.base.addr_ref);
                    TRACE_MSG(TRACE_ERROR, "neighbor entry is offline,  ref %hd short_adr 0x%04x",
                              (FMT__H_H, ZG->nwk.neighbor.neighbor[i].u.base.addr_ref, short_addr));

                    /* Delete related routing table */
                    if (ret == RET_OK)
                    {
                        zb_nwk_mesh_delete_route(short_addr);
                    }

                    if (ret != RET_OK)
                    {
                        TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_delete address reference not found [%d]", (FMT__D, ret));
                    }
                }
            }
            else
#endif
                if (ZG->nwk.neighbor.neighbor[i].u.base.age >= ZB_NIB_GET_ROUTER_AGE_LIMIT())
                {
                    ZG->nwk.neighbor.neighbor[i].u.base.outgoing_cost = 0;
                    /*        ZG->nwk.neighbor.neighbor[i].used = ZB_FALSE;*/
                    TRACE_MSG(TRACE_NWK3, "neighbor[%hd] addr_ref %hd set outgoing_cost 0 - expired",
                              (FMT__H_H, i, ZG->nwk.neighbor.neighbor[i].u.base.addr_ref));
#ifdef ZB_PARENT_CLASSIFICATION
                    if (ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR
                            && ZB_NIB().nwk_hub_connectivity)
                    {
                        TRACE_MSG(TRACE_NWK3, "no link statuses from ZC - clear nwk_hub_connectivity", (FMT__0));
                        nwk_set_tc_connectivity(0);
                    }
#endif
#ifdef ZB_RAF_NEIGHBOR_AUTO_DELETETION
                    ZG->nwk.neighbor.neighbor[i].u.base.age++;
#endif
                }
                else
                {
                    ZG->nwk.neighbor.neighbor[i].u.base.age++;
                }
#ifndef ZB_COORDINATOR_ONLY
            if ((ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR ||
                    ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ROUTER) &&
                    ZG->nwk.neighbor.neighbor[i].u.base.outgoing_cost != 0U
               )
            {
                all_routers_expired = ZB_FALSE;
            }
#endif

        }
    }
#ifndef ZB_COORDINATOR_ONLY
    {
        if (ZB_IS_DEVICE_ZR() && all_routers_expired
                && ZG->nwk.selector.no_active_links_left_cb != NULL)
        {
            /* All routers expired - looks like link is down, send ZB_NWK_SIGNAL_NO_ACTIVE_LINKS_LEFT signal*/
            zb_ret_t ret = zb_buf_get_out_delayed(ZG->nwk.selector.no_active_links_left_cb);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
        }
    }
#endif

}


/*
 * Link Status handler
 * Process received Link Status request
 * see 3.6.3.4.2
 */
void zb_nwk_receive_link_status_command(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_uint8_t hdr_size)
{
    zb_uint8_t *status_cmd = (zb_uint8_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(buf, hdr_size);
    zb_uint8_t *dt = (zb_uint8_t *)(status_cmd + sizeof(zb_uint8_t));
    zb_uint16_t startAddr;// = dt[0].addr;
    zb_uint16_t endAddr;// = dt[status_cmd->count-1].addr;
    zb_uint8_t count;
    zb_neighbor_tbl_ent_t *nbt = NULL;
    zb_ret_t status = RET_OK;

    count = ZB_NWK_LS_GET_COUNT(*status_cmd);

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_receive_link_status_command count %hu", (FMT__H, count));

#ifdef ZB_RAF_NEIGHBOR_AUTO_DELETETION
    if (count > zb_nwk_get_link_status_max_count())   //neighbor table could be deleted, count value could be zero
#else
    if (count == 0U || count > zb_nwk_get_link_status_max_count())
#endif
    {
        TRACE_MSG(TRACE_ERROR, "Oops! link status count is incorrect (%hu)", (FMT__H, count));
        status = RET_ERROR;
    }

    /*
      3.6.3.4.2 Upon Receipt of a Link Status Command Frame

      Upon receipt of a link status command frame by a Zigbee router or coordinator,
      the age field of the neighbor table entry corresponding to the transmitting device
      is reset to 0.
     */
    if (status == RET_OK)
    {
        status = zb_nwk_neighbor_get_by_short(nwk_hdr->src_addr, &nbt);
    }

    if (status == RET_OK)
    {
        ZB_LETOH16(&startAddr, dt);
        ZB_LETOH16(&endAddr, (dt + (count - 1U)*ZB_LINK_STATUS_SIZE));

        nbt->u.base.age = 0;
        /* Decide that routers are always sleepless devices */
        nbt->rx_on_when_idle = 1;
        /* If device type is unknown, update it to COORDINATOR/ROUTER (depending on src_addr) */
        if (nbt->device_type == ZB_NWK_DEVICE_TYPE_NONE)
        {
            zb_bool_t neighbor_is_zc = (nwk_hdr->src_addr == 0U);

            /*cstat !MISRAC2012-Rule-14.3_b */
            /** @mdr{00012,14} */
            if (neighbor_is_zc && ZB_IS_DEVICE_ZC())
                /*cstat !MISRAC2012-Rule-2.1_b */
                /** @mdr{00012,15} */
            {
                TRACE_MSG(TRACE_ERROR, "ERROR: We are ZC and we received Link Status from another ZC", (FMT__0));
            }
            else
            {
                nbt->device_type = (neighbor_is_zc) ?
                                   ZB_NWK_DEVICE_TYPE_COORDINATOR : ZB_NWK_DEVICE_TYPE_ROUTER;
            }
        }
#ifdef ZB_PARENT_CLASSIFICATION
        if (nbt->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR
                && !ZB_NIB().nwk_hub_connectivity)
        {
            TRACE_MSG(TRACE_NWK3, "link status from ZC - set nwk_hub_connectivity", (FMT__0));
            nwk_set_tc_connectivity(1);
        }
#endif
        /* test-spec, tp_r21_bv-26, expected outcome for DUT_ZR (criteria 3 - fail verdict)
         *
         * relationship 'sibling' does not described in spec, but it should
         * be set for routers, otherwise the test is considered failed
         */
        if (ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE == nbt->relationship)
        {
            nbt->relationship = ZB_NWK_RELATIONSHIP_SIBLING;
        }
    }

    if (status == RET_OK
            && nbt != NULL
            && startAddr <= ZB_PIBCACHE_NETWORK_ADDRESS() && ZB_PIBCACHE_NETWORK_ADDRESS() <= endAddr)
    {
        zb_ushort_t i;

        /*
          If the receiver's address is not found, the
          outgoing cost field is set to 0.
        */
        /*
          It really means: we are not in his neighbor table
        */
        nbt->u.base.outgoing_cost = 0;

        for (i = 0; i < count; i++)
        {
            zb_uint16_t addr;
            ZB_LETOH16(&addr, (dt + i * ZB_LINK_STATUS_SIZE));
            if (addr == ZB_PIBCACHE_NETWORK_ADDRESS())
            {
                /*
                  If the receiver's address is found, the outgoing cost
                  field of the neighbor table entry corresponding to the sender is set to the incoming
                  cost value of the link status entry.

                  The outgoing cost field contains the cost of the link as measured by
                  the neighbor.
                  The value is obtained from the most recent link status command frame received
                  from the neighbor. A value of 0 indicates that no link status command listing this
                  device has been received.
                */
                nbt->u.base.outgoing_cost = ZB_GET_INCOMING_COST(dt + i * ZB_LINK_STATUS_SIZE);
                /* Once we listed in neighbor's Link Status, sure can go there directly */
                nbt->send_via_routing = 0;
                TRACE_MSG(TRACE_NWK3, "0x%x set outgoing_cost %hd; clear send_via_routing", (FMT__D_H, nwk_hdr->src_addr, nbt->u.base.outgoing_cost));
                break;
            }
        }
    }

    zb_buf_free(buf);

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_receive_link_status_command", (FMT__0));
}

/*
 * Prepare one Link Status request
 * Fill Link Status List Field by next NWK_NEIGHBORS_PER_MESSAGE neighbors
 * if neighbors are done then set send_link_status_index to invalid index (-1)
 */
zb_uint8_t zb_nwk_prepare_link_status_command(zb_uint8_t *dt, zb_uint8_t max_count)
{
    zb_uindex_t i;
    zb_neighbor_tbl_ent_t *nbt;
    zb_uint8_t tmp;
    zb_uint8_t count = 0;

    /*
    3.4.8 Link Status Command
     */

    for (i = 0; i < max_count; i++)
    {
        if (zb_nwk_get_sorted_neighbor(&(ZG->nwk.handle.send_link_status_index), &nbt) != RET_OK)
        {
            break;
        }

        if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ROUTER
                || nbt->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR)
        {
            zb_uint16_t addr;
            zb_uint8_t in_cost;

            zb_address_short_by_ref(&addr, nbt->u.base.addr_ref);
            ZB_HTOLE16(dt, &addr);
            dt += sizeof(zb_uint16_t); /*address shift */

            /* The incoming cost field contains the device's estimate of the link cost for the neighbor,
             * which is a value between 1 and 7. */
            in_cost = ZB_NWK_NEIGHBOUR_GET_PATH_COST(nbt);
            ZB_LS_SET_INCOMING_COST(dt, in_cost);

            /*The outgoing cost field contains the value of the outgoing cost field from the neighbor
             * table. */
            ZB_LS_SET_OUTGOING_COST(dt, nbt->u.base.outgoing_cost);
            TRACE_MSG(TRACE_NWK2, "Link Status addr 0x%x inc cost %hd oug cost %hd",
                      (FMT__D_H_H, addr, in_cost, nbt->u.base.outgoing_cost));
            dt++;
            count++;
        }

        ZG->nwk.handle.send_link_status_index++;
    }

    /* Last address previous request = first address next request - see spec 3.4.8.3.2 */
    tmp = ZG->nwk.handle.send_link_status_index;
    if ((zb_nwk_get_sorted_neighbor(&tmp, &nbt) == RET_OK)
            && (ZG->nwk.handle.send_link_status_index > 0U))
    {
        ZG->nwk.handle.send_link_status_index--;
    }
    else
    {
        ZG->nwk.handle.send_link_status_index = (zb_address_ieee_ref_t)(-1);
    }

    return count;
}


/*
 * Send one Link Status request
 * if send_link_status_index is invalid index (-1) then
 *   make schedule alarm for zb_nwk_link_status_handle with ZB_NIB().link_status_period second
 * Its means after ZB_NIB().link_status_period second will start send Link Status requests
 */
void zb_nwk_send_link_status_command(zb_uint8_t param)
{
    zb_address_ieee_ref_t ref_p;
    zb_uint8_t count;
    zb_uint8_t max_count;
    //zb_nlme_link_status_t *dt;
    zb_uint8_t *dt;
    //zb_nlme_link_status_indication_t *status_cmd;
    zb_uint8_t *status_cmd;
    zb_uint8_t status_cmd_pos;

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_send_link_status_command param %hd curr index %hd",
              (FMT__H_H, param, ZG->nwk.handle.send_link_status_index));
    TRACE_MSG(TRACE_ATM1, "Z< send link status", (FMT__0));

    if (ZB_JOINED())
    {
        zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
        zb_nwk_hdr_t *nwhdr;

        nwhdr = nwk_alloc_and_fill_hdr(param,
                                       ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NWK_BROADCAST_ROUTER_COORDINATOR,
                                       ZB_FALSE, secure, ZB_TRUE, ZB_FALSE);

        nwhdr->radius = 1;

        TRACE_MSG(TRACE_NWK2, "link_status_index before %d", (FMT__H, ZG->nwk.handle.send_link_status_index));
        /* Sanity check */
        if (zb_address_by_sorted_table_index(ZG->nwk.handle.send_link_status_index, &ref_p) != RET_OK)
        {
            /* This is a situation, where link_status_index referes to index of
             * already removed device */
            TRACE_MSG(TRACE_NWK2, "idx is not found, reset", (FMT__0));
            ZG->nwk.handle.send_link_status_index = (zb_address_ieee_ref_t)(0);
        }

        status_cmd = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LINK_STATUS, (zb_uint8_t)sizeof(zb_uint8_t));
        *status_cmd = 0;
        /* Remember the position, because the buffer's content can be moved to left */
        status_cmd_pos = (zb_uint8_t)(status_cmd - (zb_uint8_t *)zb_buf_begin(param));

        ZB_NWK_LS_SET_FIRST_FRAME(status_cmd, (ZG->nwk.handle.send_link_status_index == (zb_address_ieee_ref_t)(0)) ? 1U : 0U);

        max_count = zb_nwk_get_link_status_max_count();

        dt = zb_buf_alloc_right(param, (zb_uint_t)max_count * ZB_LINK_STATUS_SIZE);

        count = zb_nwk_prepare_link_status_command(dt, max_count);

        TRACE_MSG(TRACE_NWK2, "max_count %d count %d link_status_index %d",
                  (FMT__H_H_H, max_count, count, ZG->nwk.handle.send_link_status_index));

        /*
          See:

          3.6.3.4.1 Initiation of a Link Status Command Frame

          When joined to a network, a ZigBee router or coordinator shall
          periodically send a link status command every
          nwkLinkStatusPeriod seconds, as a one-hop broadcast without
          retries.
         */

        status_cmd = (zb_uint8_t *)zb_buf_data(param, status_cmd_pos);
        ZB_NWK_LS_SET_COUNT(status_cmd, count);

        zb_buf_cut_right(param, ((zb_uint_t)max_count - (zb_uint_t)count)*ZB_LINK_STATUS_SIZE);

        if (ZG->nwk.handle.send_link_status_index == (zb_address_ieee_ref_t)(-1))
        {
            ZB_NWK_LS_SET_LAST_FRAME(status_cmd, 1U);
            ZG->nwk.handle.send_link_status_index = (zb_address_ieee_ref_t)(0);
        }

        (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);

        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

        ZB_ASSERT(ZB_NIB_GET_LINK_STATUS_PERIOD() > 0U);
        TRACE_MSG(TRACE_NWK3, "link_status_period %d", (FMT__D, ZB_NIB_GET_LINK_STATUS_PERIOD()));
        ZB_SCHEDULE_ALARM(zb_nwk_link_status_alarm, 0, ZB_NWK_JITTER(ZB_SECONDS_TO_BEACON_INTERVAL(ZB_NIB_GET_LINK_STATUS_PERIOD())));
    }
    else
    {
        /* prevent reboot if nobody joined so no any ZB traffic */
        ZB_KICK_WATCHDOG(ZB_WD_ZB_TRAFFIC);
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_send_link_status_command", (FMT__0));
}


void nwk_maybe_force_send_via_routing(zb_uint16_t addr)
{
    zb_neighbor_tbl_ent_t *nbt = NULL;
    zb_ret_t ret = RET_ERROR;

    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        ret = zb_nwk_neighbor_get_by_short(addr, &nbt);
    }

    if (ret == RET_OK
            && nbt->relationship != ZB_NWK_RELATIONSHIP_CHILD
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'nwk_is_lq_bad_for_direct()'. This violation seems
             * to be caused by the fact that this function is an external function, which cannot be
             * analyzed by C-STAT. */
            && nwk_is_lq_bad_for_direct(nbt->rssi, nbt->lqi))
    {
        TRACE_MSG(TRACE_NWK3,
                  "nwk_maybe_force_send_via_routing: addr 0x%x nbt %p - set send_via_routing flag",
                  (FMT__D_P, addr, nbt));
        nbt->send_via_routing = 1;
    }
    else
    {
        TRACE_MSG(TRACE_NWK3, "do not touch send_via_routing flag for 0x%x - nbt %p rssi %d",
                  (FMT__D_P_D, addr, nbt, nbt ? nbt->rssi : -1));
    }
}

#endif /* Zigbee pro */  /* ZB_ROUTER_ROLE */

/*! @} */
