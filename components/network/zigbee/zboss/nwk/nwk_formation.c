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
/* PURPOSE: Router start. File named nwk_formation.c by historical
reasons. The name is confusing, but keep it to minimize build scripts
change.
*/

#define ZB_TRACE_FILE_ID 324
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_magic_macros.h"
#include "zdo_wwah_stubs.h"

/*! \addtogroup ZB_NWK */
/*! @{ */



#ifdef ZB_ROUTER_ROLE
void nwk_router_start_common(zb_uint8_t param)
{
    /* Do not touch Association Permit - network should be opened separately (on BDB steering or by
     * explicit call).
     * Remain it only under ZB_CERTIFICATION_HACKS - not to break existing certification tests. */
#ifdef ZB_CERTIFICATION_HACKS
    ZB_PIBCACHE_ASSOCIATION_PERMIT() = (ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num);
#endif

    /* [MM]: Hacky hack for ZLL: disallow devices to join by association */
#ifdef ZB_ENABLE_ZLL
    if (ZLL_TRAN_CTX().transaction_task != ZB_ZLL_NO_TASK)
    {
        ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_FALSE;
    }
#endif

    TRACE_MSG(TRACE_INFO1, "max_children %hd, zr# %hd, ze# %hd, mac_association_permit %hd",
              (FMT__H_H_H_H,
               ZB_NIB().max_children, ZB_NIB().router_child_num, ZB_NIB().ed_child_num, ZB_PIBCACHE_ASSOCIATION_PERMIT()));
#ifdef ZB_PRO_STACK

    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_link_status_alarm, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_ALARM(zb_nwk_link_status_alarm, 0, ZB_NWK_JITTER(ZB_SECONDS_TO_BEACON_INTERVAL(ZB_NIB_GET_LINK_STATUS_PERIOD())));

#if defined ZB_MAC_POWER_CONTROL
    /* For all interfaces - schedule power_delta_alarm. Power delta is
     * independent command.  */
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
    {
        zb_uint8_t i;

        for (i = 0; i < ZB_NWK_MAC_IFACE_TBL_SIZE; i++)
        {
            if (ZB_NIB().mac_iface_tbl[i].state && ZB_NIB_GET_POWER_DELTA_PERIOD(i))
            {
                ZB_SCHEDULE_ALARM(zb_nwk_link_power_delta_alarm, i, ZB_NIB_GET_POWER_DELTA_PERIOD(i));
            }
        }
    }
#endif  /* ZB_MAC_POWER_CONTROL */

#endif   /* Zigbee pro */

#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    if (ZB_PIBCACHE_RX_ON_WHEN_IDLE() == 0xFFU)
    {
        ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
    }
#endif

    /* zb_nwk_update_beacon_payload updates macAssociationPermit as well */
    TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
    {
        zb_uint8_t rx_on = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE,
                       &rx_on, 1, zb_nwk_update_beacon_payload);
    }
}
#endif  /* ZB_ROUTER_ROLE */

void zb_mlme_start_confirm(zb_uint8_t param)
{
#ifndef ZB_ROUTER_ROLE
    ZVUNUSED(param);
#else
    TRACE_MSG(TRACE_NWK1, ">>zb_mlme_start_confirm %hd state %hd", (FMT__H_H, param, ZG->nwk.handle.state));

    if (zb_buf_get_status(param) != RET_OK)
    {
        /* CR: MM: Question to EE: Am I right, saying that if the
         * zb_mlme_start_request is unsuccessful, then the
         * zb_zdo_startup_complete will never be called? */
        TRACE_MSG(TRACE_ERROR, "mlme_start FAILED %hd", (FMT__H, zb_buf_get_status(param)));
        nwk_internal_unlock_in();
    }
    else
    {
        zb_ret_t ret;
        zb_address_ieee_ref_t addr_ref;

        /* Ext neighbor was filled during Active scan at Formation time. Free it
         * (will find no devices in our PAN, so put nothing into Neighbor table).
         * Note that it is safe to call zb_nwk_exneighbor_stop() even if ext neighbor
         * was not started before.
         */
        zb_nwk_exneighbor_stop((zb_uint16_t) -1);
        /* short and long addresses already known - update it here */
        ret = zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(),
                                ZB_PIBCACHE_NETWORK_ADDRESS(),
                                ZB_FALSE, &addr_ref);
        ZB_ASSERT(ret == RET_OK);

        /* DD: moved from nwk_cr_parent.c:zb_nlme_start_router_request()
           Rationale: we need to reinit routing for ZC also.
           Possible issue: ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION state,
           but I am not sure route discovery will work well during
           PAN ID Conflict resolution process anyway.
         */
#if defined NCP_MODE && !defined NCP_MODE_HOST
        zb_nwk_mesh_routing_deinit();
        zb_nwk_mesh_routing_init();
#endif

        switch ( ZG->nwk.handle.state )
        {
#ifndef ZB_COORDINATOR_ONLY
        case ZB_NLME_STATE_ROUTER:
            ZG->nwk.handle.run_after_update_beacon_payload = zb_nlme_start_router_confirm;
            ZB_SCHEDULE_CALLBACK(nwk_router_start_common, param);
            ZB_ZDO_SCHEDULE_UPDATE_LONG_UPTIME();
            break;
#endif /* #ifndef ZB_COORDINATOR_ONLY */

#ifdef ZB_FORMATION
        case ZB_NLME_STATE_FORMATION:
#ifdef ZB_COORDINATOR_ROLE
            zb_nwk_formation_force_link();
#endif
            ZB_ASSERT(ZG->nwk.selector.formation_mlme_start_conf);
            (*ZG->nwk.selector.formation_mlme_start_conf)(param);
            ZB_ZDO_SCHEDULE_UPDATE_LONG_UPTIME();
            break;
#endif /* ZB_FORMATION */

        case ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION:
            TRACE_MSG(TRACE_NWK1, "done panid update after panid conflict resolution", (FMT__0));
            zb_buf_free(param);
            break;

        default:
            TRACE_MSG(TRACE_NWK1, "wrong nwk ste %hd", (FMT__H, ZG->nwk.handle.state));
            ZB_ASSERT(0);
            break;
        }
    }
    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;

    TRACE_MSG(TRACE_NWK1, "<<mlme_start_conf", (FMT__0));
#endif  /* ZB_ROUTER_ROLE */
}

/*! @} */
