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
/* PURPOSE: Route discovery requested by layer upper than NWK. Is it ever used??
*/

#define ZB_TRACE_FILE_ID 2234
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_magic_macros.h"

/*! \addtogroup ZB_NWK */
/*! @{ */

#ifndef ZB_LITE_NO_NLME_ROUTE_DISCOVERY

#if defined ZB_ROUTER_ROLE

void nwk_route_discovery_confirm(zb_bufid_t buf, zb_uint8_t cfm_status)
{
    zb_nlme_route_discovery_confirm_t *ptr;
    ptr = zb_buf_initial_alloc(buf, sizeof(zb_nlme_route_discovery_confirm_t));
    ZB_ASSERT(ptr);
    ptr->status = cfm_status;
    ZB_SCHEDULE_CALLBACK(zb_nlme_route_discovery_confirm, buf);
}

void zb_nlme_route_discovery_request(zb_uint8_t param)
{
    zb_nlme_route_discovery_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_route_discovery_request_t);

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_route_discovery_request %hd", (FMT__H, param));
    CHECK_PARAM_RET_ON_ERROR(request);

    /* check that we are router or coordinator */
    if (ZB_IS_DEVICE_ZED())
    {
        nwk_route_discovery_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
        TRACE_MSG(TRACE_NWK1, "no routing capacity available, ret error", (FMT__0));
        goto done;
    }

    /* check for broadcast address */
    /* We don't need a many-to-one route discovery, because it's optional, but we */
    /* need to process nlme_route_discovery with dst_addr = 0x00. Just to establish */
    /* Source Route Table */
    if (ZB_NWK_IS_ADDRESS_BROADCAST(request->network_addr))
    {
#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING
        /* save dest address, to call confirm on error or success */
        ZB_NIB().aps_rreq_addr = request->network_addr;

        /* Establishing Source Route Table. Table 3.34 */
        if (ZB_NIB().nwk_src_route_cnt == 0xFFU) /*AD: check if srt table already established */
        {
            ZB_NIB().nwk_src_route_cnt = 0;
        }
        if (request->no_route_cache)
        {
            /* We still have to establish a source route table to be able to handle Route Record
               commands from devices. In the case of Low RAM Concentrator, we should have only one
               place for a route in the table. Let's use the same logic to simplify.
            */
            TRACE_MSG(TRACE_NWK1, "Send MTORR without source routing", (FMT__0));
            zb_nwk_mesh_route_discovery(param, request->network_addr, request->radius, ZB_NWK_RREQ_TYPE_MTORR_RREC_TABLE_UNSUPPORTED);
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "Send MTORR with source routing", (FMT__0));
            zb_nwk_mesh_route_discovery(param, request->network_addr, request->radius, ZB_NWK_RREQ_TYPE_MTORR_RREC_TABLE_SUPPORTED);
        }
#else
        /* Source routing is not supported */
        nwk_route_discovery_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
#endif

        goto done;
    }

    /* check it's not our own address */
    if ( request->address_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST
            && request->network_addr == ZB_PIBCACHE_NETWORK_ADDRESS() )
    {
        nwk_route_discovery_confirm(param, ZB_NWK_STATUS_SUCCESS);
        TRACE_MSG(TRACE_NWK1, "truing to discover our own address, send success", (FMT__0));
        goto done;
    }

    /* check it's not our neighbour */
    {
        zb_neighbor_tbl_ent_t *nbt;

        if (zb_nwk_neighbor_get_by_short(request->network_addr, &nbt) == RET_OK )
        {
            nwk_route_discovery_confirm(param, ZB_NWK_STATUS_SUCCESS);
            TRACE_MSG(TRACE_NWK1, "discovery device found in neighbour table, ret success", (FMT__0));
            goto done;
        }
    }

    if ( request->address_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST )
    {
        /* save dest address, to call confirm on error or success */
        ZB_NIB().aps_rreq_addr = request->network_addr;
        zb_nwk_mesh_route_discovery(param, request->network_addr, request->radius, ZB_NWK_RREQ_TYPE_NOT_MTORR);
    }
    else
    {
        nwk_route_discovery_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST);
    }

done:
    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_route_discovery_request", (FMT__0));
}

#endif /* #ifdef ZB_ROUTER_ROLE */
#endif  /* ZB_LITE_NO_NLME_ROUTE_DISCOVERY */
/*! @} */
