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
/* PURPOSE: Network layer main module
*/

#define ZB_TRACE_FILE_ID 327
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_secur.h"
#include "zb_watchdog.h"
#include "zb_zdo.h"
#include "zb_bdb_internal.h"
#include "zb_nwk_ed_aging.h"
#include "zb_manuf_prefixes.h"
/* #include "mac_internal.h" */
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE
#include "zb_aps_interpan.h"
#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
#include "zdo_wwah_stubs.h"
#include "zdo_diagnostics.h"

#ifdef ZB_ENABLE_ZGP
void zb_cgp_data_cfm(zb_uint8_t param);
#endif /* ZB_ENABLE_ZGP */
#include "zb_bufpool.h"
/*! \addtogroup ZB_NWK */
/*! @{ */



/**
   NIB database in memory
*/

#ifdef ZB_ENABLE_ZGP_DIRECT
void zb_gp_mcps_data_indication(zb_uint8_t param);
#endif /* ZB_ENABLE_ZGP_DIRECT */

void nwk_frame_indication(zb_uint8_t param);
void zb_nwk_leave_ind_prnt(zb_uint8_t param);
static void zb_nwk_call_leave_ind(zb_uint8_t param, zb_uint8_t rejoin, zb_address_ieee_ref_t addr_ref);
static void zb_nwk_leave_handler(zb_uint8_t param, zb_nwk_hdr_t *nwk_hdr, zb_uint8_t lp);

#ifdef ZB_JOIN_CLIENT
/* Reset nwk short address in MAC PIB upon leave */
static void zb_nwk_reset_nwk_addr_in_mac_pib(zb_uint8_t param);
/* Reset beacon payload length to zero - to prevent  device sending
 * beacons after leaving network */
static void zb_nwk_remove_beacon_payload(zb_uint8_t param);

static void nwk_broadcast_delivery_time_passed_cb(zb_uint8_t param);
#endif


#ifdef ZB_ROUTER_ROLE
static void zb_nwk_network_status_handler(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_nlme_status_indication_t *cmd);
static void nwk_check_relay_frame_status(zb_uint8_t param);
static zb_bool_t is_destination_our_neighbour(const zb_nwk_hdr_t *nwk_header, zb_neighbor_tbl_ent_t **nbt);
#endif
static void nwk_add_into_addr_and_nbt(zb_nwk_hdr_t *nwk_hdr, zb_apsde_data_ind_params_t *ind);

static void call_status_indication(zb_uint8_t param, zb_nwk_command_status_t status, zb_uint16_t dest_addr);

/* Former check for is_rx buf flag. Not need that flag any more.  Now
   always have zb_apsde_data_ind_params_t at buffer tail.  When
   relaying buffer, it has value != -1.  For outgoing buffer it is -1.
   See zb_nwk_init_apsde_data_ind_params().
*/
#define NWK_PACKET_IS_RX(buf) (nwk_get_pkt_mac_source(buf) != ZB_MAC_SHORT_ADDR_NOT_ALLOCATED)

void zb_nwk_init()
{
    TRACE_MSG(TRACE_NWK1, "+nwk_init", (FMT__0));

    /* Initialize internal structures */
    /* There was device type set in MAC, but now it is not necessary: device type
     * usage changed to the beacon payload length which initializer to 0 by
     * default, and set when updating beacon payload */

    zb_nwk_nib_init(ZB_TRUE);

    zb_nwk_neighbor_init();

#ifdef ZB_NWK_TREE_ROUTING
    zb_nwk_tree_routing_init();
#endif

#ifdef ZB_NWK_MESH_ROUTING
    zb_nwk_mesh_routing_init();
#endif

#ifdef ZB_CHECK_OOM_STATUS
    ZG->nwk.oom_presents = ZB_FALSE;
#ifdef ZB_SEND_OOM_STATUS
    /* Buffer for sending oom_status will be obtained later. */
    ZG->nwk.oom_status_buf_ref = 0;
    ZG->nwk.oom_status = ZB_OOM_STATUS_NOT_SENT;
#endif
#endif

    /* ZC/ZR: Start this watchdog here (not sure).
       ZED: Start this watchdog when we really waiting ZB traffic - on dev_annce (assoc/rejoin completed, device authenticated).
       Stop this watchdog when we do not waiting ZB traffic:
       - on leave without rejoin (fall into association)
    */
#if (defined ZB_USE_ZB_TRAFFIC_WATCHDOG)
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        zb_add_watchdog(ZB_WD_ZB_TRAFFIC, ZB_WD_ZB_TRAFFIC_TO);
        ZB_DISABLE_WATCHDOG(ZB_WD_ZB_TRAFFIC);
    }
#endif

#ifdef ZB_JOIN_CLIENT
    /*
     * Required for checking "the device has been powered up and operating on the
     * network for nwkNetworkBroadcastDeliveryTime" condition of CCB2033
     *
     * TODO: do we need to take account of "operation on the network .." part?
     */
    ZB_SCHEDULE_ALARM(nwk_broadcast_delivery_time_passed_cb, 0,
                      ZB_NWK_OCTETS_TO_BI(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS));
#endif  /* ZB_JOIN_CLIENT */

    TRACE_MSG(TRACE_NWK1, "-nwk_init", (FMT__0));
}


void zb_nwk_nib_init(zb_bool_t is_first)
{
    TRACE_MSG(TRACE_NWK1, ">>nib_init: is_first %hd", (FMT__H, is_first));

    ZB_NIB().passive_ack_timeout = ZB_NWK_PASSIVE_ACK_TIMEOUT_OCTETS;
    ZB_NIB().max_broadcast_retries = ZB_NWK_MAX_BROADCAST_RETRIES;

#if defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE
    ZB_NIB().max_routers = ZB_NWK_MAX_ROUTERS;
#endif

#if defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN
    ZB_NIB().uniq_addr   = ZB_FALSE_U;
    ZB_NIB().addr_alloc  = ZB_NWK_ADDRESS_ALLOC_METHOD_STOCHASTIC;
    ZB_NIB_MAX_DEPTH()   = ZB_NWK_STOCH_DEPTH;
#else
    ZB_NIB().uniq_addr   = ZB_TRUE_U;
    ZB_NIB().addr_alloc  = ZB_NWK_ADDRESS_ALLOC_METHOD_DISTRIBUTED;
    ZB_NIB_MAX_DEPTH()   = ZB_NWK_TREE_ROUTING_DEPTH;
#endif

#ifdef ZB_PRO_STACK
    ZB_NIB_SET_USE_TREE_ROUTING(0);
#ifndef ZB_LITE_NO_SOURCE_ROUTING
    /* Disable Source routing by default */
    ZB_NIB_SRCRT_CNT() = 0xFF;
    ZB_NIB_SET_IS_CONCENTRATOR(ZB_FALSE);
    ZB_NIB_SET_CONCENTRATOR_RADIUS(0);
    ZB_NIB_SET_CONCENTRATOR_DISC_TIME(0);
#endif

    /* Since R21 should be turned off by default */
    ZB_NIB_SET_USE_MULTICAST(0);
    /* Enable leave request processing, enabled by default */
    ZB_NIB().leave_req_allowed = ZB_TRUE_U;
#ifndef ZB_LITE_NO_CONFIGURABLE_LINK_STATUS
    ZB_NIB().link_status_period = ZB_NWK_LINK_STATUS_PERIOD;
    TRACE_MSG(TRACE_NWK3, "set link_status_period %d", (FMT__D, ZB_NIB_GET_LINK_STATUS_PERIOD()));
    ZB_NIB().router_age_limit = ZB_NWK_ROUTER_AGE_LIMIT;
#endif
    /* Set of using preconfigured key */
    ZB_NIB().leave_req_without_rejoin_allowed = ZB_TRUE_U;

    ZB_BZERO(&ZB_NIB().secur_material_set[0].key[0], ZB_CCM_KEY_SIZE);
    ZB_NIB().secur_material_set[0].key_seq_number = 0;
#else  /* ZB_PRO_STACK */
    /* Tree routing is mandatory in 2007 */
    ZB_NIB_SET_USE_TREE_ROUTING(1);
#endif

    ZB_NIB().nwk_report_constant_cost = 0;
#if defined ZB_ROUTER_ROLE
    ZG->nwk.handle.send_link_status_index = 0;
#endif /* ZB_ROUTER_ROLE */

#ifndef ZB_PRO_STACK
    /* ZB_NIB().nwk_report_constant_cost = 1; */
#endif /* ZB_PRO_STACK */

#if defined ZB_NWK_MESH_ROUTING && !defined ZB_LITE_NO_NLME_ROUTE_DISCOVERY
    ZB_NIB().aps_rreq_addr = (zb_uint16_t) -1;
#endif

#ifndef ZB_DIRECT_PIB_ACCESS
    /* Initialize PIB cache by ~0 to be able to detect its change. In such case
     * update PIB by this value. */
    ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_NWK_BROADCAST_ALL_DEVICES;
    ZB_PIBCACHE_PAN_ID() = ZB_BROADCAST_PAN_ID;
    ZB_PIBCACHE_CURRENT_CHANNEL() = 0xffU;
    /* do not clear ext addr: its modification is not standard */
    //  ZB_64BIT_ADDR_ZERO(ZB_PIBCACHE_EXTENDED_ADDRESS());
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    if (is_first)
    {
        /* Application can override ZB_PIBCACHE_RX_ON_WHEN_IDLE between ZB_INIT and zdo_start */
        ZB_PIBCACHE_RX_ON_WHEN_IDLE() = 0xff;
    }
#endif
    ZB_PIBCACHE_ASSOCIATION_PERMIT() = 0xff;
#endif

    ZB_NIB().disable_rejoin = ZB_FALSE_U;

    /* We can be here first during ZB_INIT, second after
     * mlme-reset. is_first is TRUE if this is call from ZB_INIT.
     * Next secion is under is_first check to set reasonable defaults
     * which application can owerwrite after ZB_INIT call.
     */
    if (is_first)
    {
        /* We use same var ZB_NIB().nwk_keepalive_modes at FFD and  ZED.
           For FFD it is keepalive mode we report in resp to ED
           timeout req and check in poll indication.
           For ED this is keepalive mode ED got from ZC/ZR.
         */
        /* Here we do not know our device type yet: it can be ZR capable
         * to switch into ED mode from the app or when loading NVRAM -
         * anyway, after after ZB_INIT. So set most reasonable default
         * here.
         * Later after join ZED may discover that its parent is
         * pre-r21. In such case it must switch off keepalives.
         *
         * MAC_DATA_POLL_KEEPALIVE is widely used while
         * ED_TIMEOUT_REQUEST_KEEPALIVE is erally necessary for
         * certification only.
         */
        ZB_SET_KEEPALIVE_MODE(MAC_DATA_POLL_KEEPALIVE);

#ifdef ZB_ED_FUNC
        /* Timeout which we will negotiate with parent */
        zb_set_ed_timeout(NWK_ED_DEVICE_TIMEOUT_DEFAULT);
        /* Interval between keepalive packets tx (polls / ed timeout req sends) */
        zb_set_keepalive_timeout(zb_nwk_get_default_keepalive_timeout());
#endif
    }

    ZB_BZERO(&ZB_NIB().tx_stat, sizeof(zb_tx_stat_window_t));
    /* parent info got from ed deice timeout resp. Just after MAC reset it is unknown. */
    ZB_SET_PARENT_INFO(0);
    /* ZOI-81: According to [Zigbee_pro_core] CCB 2713 - PAN ID Conflict Proposal,
     * we change the default nwkManagerAddr from 0x0000 to 0xffff. */
    ZB_NIB_NWK_MANAGER_ADDR() = 0xffff;
    ZB_NIB_UPDATE_ID() = 0;

#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().override_nwk_protocol_version = ZB_PROTOCOL_VERSION;
#endif

    ZG->nwk.handle.nwk_broadcast_delivery_time_passed = ZB_FALSE;
    /* Auto panid conflict resolution disabled by default, panid
     * conflict resolution code not linked if
     * zb_enable_panid_conflict_resolution() or
     * zb_enable_auto_pan_id_conflict_resolution() not called. */
    ZG->nwk.panid_conflict_auto_resolution = ZB_FALSE;
    ZG->nwk.handle.parent = (zb_address_ieee_ref_t) -1;

    zb_nwk_mm_mac_iface_table_init();

    TRACE_MSG(TRACE_NWK1, "<<nib_init", (FMT__0));
}


zb_nwk_pib_cache_t *zb_nwk_get_pib_cache()
{
    return &ZG->nwk.pib_cache;
}

zb_nib_t *zb_nwk_get_nib()
{
    return &ZG->nwk.nib;
}

#if defined R22_MULTIMAC

zb_int32_t zb_nwk_get_octet_duration_us()
{
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
    {
        return ZB_EU_FSK_SYMBOL_DURATION_USEC * ZB_EU_FSK_PHY_SYMBOLS_PER_OCTET;
    }
    else
    {
        return ZB_2_4_GHZ_OCTET_DURATION_USEC;
    }
}

#endif

#ifdef ZB_JOIN_CLIENT
/* Needed for CCB2033 implementation */
static void nwk_broadcast_delivery_time_passed_cb(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_NWK3, "nwk_broadcast_delivery_time_passed_cb fired", (FMT__0));
    ZG->nwk.handle.nwk_broadcast_delivery_time_passed = ZB_TRUE;
}
#endif  /* ZB_JOIN_CLIENT */

#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
static zb_nwk_rrec_t *nwk_find_src_route_for_packet(zb_nwk_alloc_hdr_req_t *req)
{
    zb_nwk_rrec_t *rrec = NULL;

    /*AD: checking for source routing before filling nwhdr. 3.3.1 NPDU FRAME FORMAT */
    TRACE_MSG(TRACE_NWK1, "ZB_NIB_SRCRT_CNT() %d ZB_NWK_MAX_SRC_ROUTES %d", (FMT__D_D, ZB_NIB_SRCRT_CNT(), ZB_NWK_MAX_SRC_ROUTES));
    if (ZB_NIB_SRCRT_CNT() <= ZB_NWK_MAX_SRC_ROUTES)
    {
        TRACE_MSG(TRACE_NWK2, "trying source routing for addr 0x%x", (FMT__D, req->dst_addr));
        NWK_ARRAY_FIND_ENT(ZB_NIB().nwk_src_route_tbl, ZB_NWK_MAX_SRC_ROUTES, rrec, rrec->addr == req->dst_addr);
        if (rrec != NULL)
        {
            TRACE_MSG(TRACE_NWK2, "found a source route entry", (FMT__0));
        }
    }

    return rrec;
}

static void nwk_fill_src_route_subheader(zb_uint8_t *ptr,
        zb_nwk_rrec_t *rrec)
{
    *ptr++ = rrec->count; /* AD: Counter */
    *ptr++ = rrec->count; /* AD: Current index */
    ZB_MEMCPY(ptr, rrec->path  /* AD: skip destination */, rrec->count * sizeof(zb_uint16_t));
}

void zb_nwk_source_routing_record_delete(zb_uint16_t addr)
{
    zb_nwk_rrec_t  *src_route_record;
    NWK_ARRAY_FIND_ENT(ZB_NIB().nwk_src_route_tbl, ZB_NWK_MAX_SRC_ROUTES, src_route_record, src_route_record->addr == addr);
    if (src_route_record != NULL)
    {
        NWK_ARRAY_PUT_ENT(ZB_NIB().nwk_src_route_tbl, src_route_record, ZB_NIB_SRCRT_CNT());
    }
}

#endif  /* if source routing */

/**
   Internal Allocate and fill nwk header

   @return pointer to just filled header or NULL if no space in the buffer
 */
static zb_nwk_hdr_t *nwk_alloc_and_fill_hdr_int(zb_bufid_t              buf,
        zb_nwk_alloc_hdr_req_t *req)
{
    zb_nwk_hdr_t   *nwhdr;
#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
    zb_nwk_rrec_t  *src_route_record;
    zb_ushort_t     src_route_subhdr_pos = 0;
#endif
    zb_ushort_t     hdr_size;
    zb_ushort_t     ccm_m_size = 0;

    TRACE_MSG(TRACE_NWK3, "is_has_src_ieee = %d, is_has_dst_ieee = %d",
              (FMT__D_D, req->is_has_src_ieee, req->is_has_dst_ieee));

    if (req->is_has_src_ieee)
    {
        TRACE_MSG(TRACE_MAC2, "src_ieee_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(req->src_ieee_addr)));
    }

    if (req->is_has_dst_ieee)
    {
        TRACE_MSG(TRACE_MAC2, "dst_ieee_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(req->dst_ieee_addr)));
    }

    hdr_size = ZB_OFFSETOF(zb_nwk_hdr_t, dst_ieee_addr);
    if ( req->is_has_src_ieee )
    {
        hdr_size += sizeof(zb_ieee_addr_t);
    }
    if ( req->is_has_dst_ieee )
    {
        hdr_size += sizeof(zb_ieee_addr_t);
    }
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
    if ( req->is_multicast )
    {
        /* AD: mc field size is 1byte. DO NOT USE sizeof(struct_t) here, especially when used PACKED and bitfield */
        hdr_size += sizeof(zb_uint8_t);
    }
#endif

#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
    src_route_record = nwk_find_src_route_for_packet(req);
    if ( src_route_record != NULL )
    {
        src_route_subhdr_pos = hdr_size;
        hdr_size += src_route_record->count * sizeof(zb_uint16_t) + 1U /*relay index*/ + 1U /*relay count */;
    }
#endif

    if ( req->is_secured )
    {
        hdr_size += sizeof(zb_nwk_aux_frame_hdr_t);
        /* Message Integrity Check (MIC) Field size */
        ccm_m_size = ZB_CCM_M;
    }

    TRACE_MSG(TRACE_NWK3, "nwk_alloc_and_fill_hdr: header size=%hd", (FMT__H, hdr_size));

    /* fill nwk header */
    if ( req->is_cmd_frame )
    {
        nwhdr = zb_buf_initial_alloc(buf, hdr_size);
    }
    else
    {
        if (zb_buf_len(buf) + MAX_MAC_OVERHEAD_SHORT_ADDRS + hdr_size + ccm_m_size > MAX_PHY_FRM_SIZE)
        {
            /* Last chance to fit: try to skip long addresses */
            if (req->is_has_src_ieee)
            {
                req->is_has_src_ieee = ZB_FALSE;
                hdr_size -= sizeof(zb_ieee_addr_t);
            }
            if (req->is_has_dst_ieee)
            {
                req->is_has_dst_ieee = ZB_FALSE;
                hdr_size -= sizeof(zb_ieee_addr_t);
            }
        }
        if (zb_buf_len(buf) + MAX_MAC_OVERHEAD_SHORT_ADDRS + hdr_size + ccm_m_size > MAX_PHY_FRM_SIZE)
        {
            TRACE_MSG(TRACE_NWK3, "Need %d butes - not enough free space - will drop that frame!",
                      (FMT__D, zb_buf_len(buf) + 15U/*proposed mac overhead*/ + hdr_size));
            return NULL;
        }

        nwhdr = zb_buf_alloc_left(buf, hdr_size);
    }

    /* fill frame control fields */
    ZB_BZERO2(nwhdr->frame_control);
    if ( req->is_cmd_frame )
    {
        ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control, ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
    }
    else
    {
#ifdef ZB_CERTIFICATION_HACKS
        ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control,
                ZB_NWK_FRAME_TYPE_DATA,
                ZB_CERT_HACKS().override_nwk_protocol_version);
#else
        ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control, ZB_NWK_FRAME_TYPE_DATA, ZB_PROTOCOL_VERSION);
#endif
    }

#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
    if ( src_route_record != NULL )
    {
        zb_uint8_t *ptr = (zb_uint8_t *)nwhdr + src_route_subhdr_pos;
        nwk_fill_src_route_subheader(ptr, src_route_record);

        /* source route subframe is present */
        ZB_NWK_FRAMECTL_SET_SOURCE_ROUTE(nwhdr->frame_control, 1U);
    }
#endif

    ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, (req->is_has_src_ieee) ? 1U : 0U, (req->is_has_dst_ieee) ? 1U : 0U);

    if (ZB_IS_DEVICE_ZED() && (ZB_GET_PARENT_INFO() != 0U))
    {
        ZB_NWK_FRAMECTL_SET_END_DEVICE_INITIATOR(nwhdr->frame_control);
    }

    if ( req->is_secured )
    {
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,27} */
        zb_nwk_aux_frame_hdr_t *aux = (zb_nwk_aux_frame_hdr_t *)((zb_uint8_t *)nwhdr + hdr_size - sizeof(*aux));
        /* to be able to encrypt MAC indirect TX by the old key after key
         * switch while packet is in indirect MAC queue.  See
         * u.hdr.use_same_key assignment in nwk_broadcast_transmition and
         * its usage in zb_nwk_secure_frame.
         */
        aux->key_seq_number = ZB_NIB().active_key_seq_number;
        ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, 1U);
        nwk_mark_nwk_encr(buf);
    }

    nwhdr->src_addr = req->src_addr;      // ZB_PIBCACHE_NETWORK_ADDRESS();
    nwhdr->dst_addr = req->dst_addr;
    nwhdr->radius = 1U;
    nwhdr->seq_num = ZB_NIB_SEQUENCE_NUMBER();

#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
    if (req->is_multicast)
    {
        zb_nwk_multicast_control_field_t *mc_ptr;
        ZB_NWK_FRAMECTL_SET_MULTICAST_FLAG(nwhdr->frame_control, 0x01U);
        mc_ptr = &GET_NWK_MCF(nwhdr);
        mc_ptr->multicast_mode = (zb_ushort_t)(zb_aps_is_in_group(nwhdr->dst_addr)) ? (ZB_NWK_MULTICAST_MODE_MEMBER) : (ZB_NWK_MULTICAST_MODE_NONMEMBER);
        mc_ptr->nonmember_radius = ZB_AIB().aps_nonmember_radius;
        mc_ptr->max_nonmember_radius = ZB_AIB().aps_max_nonmember_radius;
    }
#endif

    ZB_NIB_SEQUENCE_NUMBER_INC();

    if ( req->is_has_src_ieee && req->is_has_dst_ieee )
    {
        ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, req->dst_ieee_addr);
        ZB_IEEE_ADDR_COPY(nwhdr->src_ieee_addr, req->src_ieee_addr);
    }
    else if ( req->is_has_dst_ieee )
    {
        ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, req->dst_ieee_addr);
    }
    else if ( req->is_has_src_ieee )
    {
        /* Yes, here should be dst_ieee_addr, because src_ieee_addr is absent */
        ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, req->src_ieee_addr);
    }
    else
    {
        /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    return nwhdr;
}

/**
   Allocate and fill nwk header for alias

   @return pointer to just filled header or NULL if no space in the buffer
 */
#ifdef ZB_USEALIAS
static zb_nwk_hdr_t *nwk_alloc_and_fill_hdr_for_alias(zb_bufid_t   buf,
        zb_uint16_t  src_addr,
        zb_uint16_t  dst_addr,
        zb_bool_t    is_multicast,
        zb_bool_t    is_secured)
{
    zb_nwk_alloc_hdr_req_t  req;
#ifndef ZB_NO_NWK_MULTICAST
    req.is_multicast = is_multicast;
#else
    ZVUNUSED(is_multicast);
#endif
    req.is_secured = is_secured;
    req.is_cmd_frame = ZB_FALSE;
    req.is_has_dst_ieee = ZB_FALSE;
    req.is_has_src_ieee = ZB_FALSE;
    req.src_addr = src_addr;
    req.dst_addr = dst_addr;

    return nwk_alloc_and_fill_hdr_int(buf, &req);
}
#endif

/*
  Alloc and fill nwk hdr, return pointer to the allocated hdr
  Multicast control field and ieee addresses not mandatory fields in packet nwk header
*/

/**
   Allocate and fill nwk header

   @return pointer to just filled header or NULL if no space in the buffer
 */
zb_nwk_hdr_t *nwk_alloc_and_fill_hdr(zb_bufid_t buf,
                                     zb_uint16_t src_addr, zb_uint16_t dst_addr,
                                     zb_bool_t is_multicast, zb_bool_t is_secured, zb_bool_t is_cmd_frame, zb_bool_t force_long)
{
    zb_nwk_alloc_hdr_req_t  req;

#ifndef ZB_NO_NWK_MULTICAST
    req.is_multicast = is_multicast;
#else
    ZVUNUSED(is_multicast);
#endif
    req.is_secured = is_secured;
    req.is_cmd_frame = is_cmd_frame;
    req.is_has_src_ieee = ZB_FALSE;
    req.is_has_dst_ieee = ZB_FALSE;
    req.src_addr = src_addr;
    req.dst_addr = dst_addr;

    /*
     * See 3.6.1.9.1
     *
     * uniq_addr == ZB_FALSE really means Zigbee PRO (not just 2007).
       In PRO we CAN (not MUST) put long addresses into data frame.
       But that check here looks ugly. Better place check for "pro" and place that check to the call point.

       Best way is to make it configurable, say, at ZCL cluster
       basis. Rationale: in general long addresses helps to detect
       address conflicts (as specification says) and helps in general to
       know long/short address.
       But in some applications we need to minimize extra payload. So let's make it tunable.
     */
    if ((!ZB_U2B(ZB_NIB().uniq_addr) || force_long)
#ifdef ZB_LOW_SECURITY_MODE
            /* NK: Destination IEEE is mandatory for:
               - Route Request (should be added if known)
               - Route Reply
               - Network Status
               - NWK Leave (should be added if known, except the case when send to aged ED)
               - Route Record
               - Rejoin Request
               - Rejoin Response
               - Network Report
               - ED Timeout Request
               - ED Timeout Response
               - Link Power Delta
               Or, in other words, for all NWK commands (where destination address is not
               broadcast). Let's exclude it for other cases (if configured by dst_ieee_policy).
            */
            && (!ZB_NIB().ieee_policy || req.is_cmd_frame)
#endif
       )
    {
        if (src_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
        {
#if defined ZB_STACK_REGRESSION_TESTING_API
            if (!ZB_REGRESSION_TESTS_API().disable_extended_nwk_src)
#endif  /* ZB_STACK_REGRESSION_TESTING_API */
            {
                ZB_64BIT_ADDR_COPY(req.src_ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
                req.is_has_src_ieee = ZB_TRUE;
            }
        }
        else if (zb_address_ieee_by_short(src_addr, req.src_ieee_addr) == RET_OK)
        {
            req.is_has_src_ieee = ZB_TRUE;
        }
        else
        {
            req.is_has_src_ieee = ZB_FALSE;
        }
        if (!ZB_NWK_IS_ADDRESS_BROADCAST(dst_addr)
#ifndef ZB_NO_NWK_MULTICAST
                && !is_multicast
#endif
           )
        {
            if ( zb_address_ieee_by_short(dst_addr, req.dst_ieee_addr) == RET_OK )
            {
                if (!ZB_IEEE_ADDR_IS_UNKNOWN(req.dst_ieee_addr)
                        /* Maybe, not very good solution, but...
                           When we rejoin to another device (another long address) with same
                           short address, it causes address conflict at the destination.
                           Specially for ZLL/BDB Touchlink test bdb-tl-TP-PRE-TC-04

                           The problem takes place when ZED rejoins to another ZR having same
                           short address. Resolved at ZLL level.
                         */
                        /* VP: according to r21 (3.4.6.2 and 3.4.7.2) destination IEEE address bit shall be set
                         * in Rejoin Request and Rejoin Response commands and destination IEEE address
                         * shall be present */
                   )
                {
                    req.is_has_dst_ieee = ZB_TRUE;
                }
            }
        }
    }

    return nwk_alloc_and_fill_hdr_int(buf, &req);
}

zb_ieee_addr_t *zb_nwk_get_src_long_from_hdr(zb_nwk_hdr_t *nwhdr)
{
    if ( ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwhdr->frame_control) != 0U )
    {
        return (zb_ieee_addr_t *)&nwhdr->src_ieee_addr;
    }
    else
    {
        return (zb_ieee_addr_t *)&nwhdr->dst_ieee_addr;
    }
}

void *nwk_alloc_and_fill_cmd(zb_bufid_t buf, zb_uint8_t cmd, zb_uint8_t cmd_size)
{
    zb_uint8_t *ptr = zb_buf_alloc_right(buf, (zb_uint_t)ZB_NWK_COMMAND_SIZE((zb_uint32_t)cmd_size));
    *ptr = cmd;
    return ptr + 1;
}


/**
   Pass nlde-data.confirm up to APS.

   If have nwk header, parse it then cut.
 */
void nwk_call_nlde_data_confirm(zb_bufid_t param, zb_nwk_status_t status, zb_bool_t has_nwk_hdr)
{
    /* reuse aps data conf everywhere */
    zb_apsde_data_confirm_t *st = zb_buf_alloc_tail(param, sizeof(*st));
    st->status = ERROR_CODE(ERROR_CATEGORY_NWK, status);
    /* If no nwk header, addr mode will be 0: no addr. zb_buf_alloc_tail zeroes parameters for us.  */
    if (has_nwk_hdr)
    {
        zb_nwk_hdr_t *nwk_hdr = zb_buf_begin(param);

        /* We can get long address from nwk header and pass it up, but do we need it? Now always use short address. */
        st->dst_addr.addr_short = nwk_hdr->dst_addr;
        ZB_LETOH16_ONPLACE(st->dst_addr.addr_short);
        st->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
        (void)zb_buf_cut_left(param, ZB_NWK_HDR_SIZE(nwk_hdr));
    }
    ZB_SCHEDULE_CALLBACK(zb_nlde_data_confirm, param);
}


void zb_nlde_data_request(zb_uint8_t param)
{
    zb_nlde_data_req_t *nldereq = ZB_BUF_GET_PARAM(param, zb_nlde_data_req_t);
    zb_nwk_hdr_t *nwhdr;
    zb_bool_t secure;
#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
    zb_uint8_t ext_flags_tmp = 0;
#endif

    ZB_TH_PUSH_PACKET(ZB_TH_APSDE_DATA, ZB_TH_PRIMITIVE_REQUEST, param);

    TRACE_MSG(TRACE_NWK1, "+ nlde_data_request(%hd)", (FMT__H, param));

    ZB_ASSERT(param);
    ZB_ASSERT(nldereq);

    /* 14/06/2016 CR [AEV] start */
#ifdef ZB_USEALIAS
    TRACE_MSG(TRACE_NWK3, "nldereq.use_alias %hd", (FMT__H, nldereq->use_alias));
    TRACE_MSG(TRACE_NWK3, "nldereq.alias_src_addr 0x%04hx", (FMT__H, nldereq->alias_src_addr));
    TRACE_MSG(TRACE_NWK3, "nldereq.alias_seq_num %hd", (FMT__H, nldereq->alias_seq_num));
#endif
    /* 14/06/2016 CR [AEV] end */

    /* check that we are associated */
    if (!ZB_JOINED())
    {
        nwk_call_nlde_data_confirm(param, ZB_NWK_STATUS_INVALID_REQUEST, ZB_FALSE);
        return;
    }

    /* if not broadcast or multicast, may send source and destination ieee
     * address.
     * Not sure when really needs to send it.
     * Sure need to send it for some comman frames (it depends on frame). Usually:
     * source - always, destination - depends on command.
     */

    TRACE_MSG(TRACE_NWK1, "security_enable %hd authenticated %hd security_level %hd",
              (FMT__H_H_H, nldereq->security_enable, ZG->aps.authenticated, ZB_NIB_SECURITY_LEVEL()));

    secure = (ZB_U2B(nldereq->security_enable) && ZG->aps.authenticated
              && ZB_NIB_SECURITY_LEVEL() > 0U);
#ifdef ZB_USEALIAS
    if ( (nldereq->use_alias) == ZB_TRUE )
    {
        nwhdr = nwk_alloc_and_fill_hdr_for_alias(param,
                ZB_PIBCACHE_NETWORK_ADDRESS(),
                nldereq->dst_addr,
                (zb_bool_t)(nldereq->addr_mode == ZB_ADDR_16BIT_MULTICAST),
                secure);
    }
    else
#endif
    {
        nwhdr = nwk_alloc_and_fill_hdr(param,
                                       ZB_PIBCACHE_NETWORK_ADDRESS(),
                                       nldereq->dst_addr,
                                       (zb_bool_t)(nldereq->addr_mode == ZB_ADDR_16BIT_MULTICAST), secure,
                                       ZB_FALSE, ZB_FALSE);
    }

    if (nwhdr != NULL)
    {
        TRACE_MSG(TRACE_APS3, "if nwhdr in with secure=%hd", (FMT__H, secure));

#ifdef ZB_USEALIAS
        /* 3.2.1.1.3 Effect on receipt of request */
        if ( (nldereq->use_alias) == ZB_TRUE )
        {
            TRACE_MSG(TRACE_NWK3, "nlde_data_request: usealias=TRUE", (FMT__0));
            {
                TRACE_MSG(TRACE_NWK3, "nlde_data_request: using alias src_addr=0x%04hx, seq_num=%hd", (FMT__D_H, nldereq->alias_src_addr, nldereq->alias_seq_num));
                nwhdr->src_addr = nldereq->alias_src_addr;
                nwhdr->seq_num  = nldereq->alias_seq_num;
            }
        }
#endif /*ZB_USEALIAS*/

#ifdef ZB_CERTIFICATION_HACKS
        if (ZB_CERT_HACKS().nwk_counter_custom_setup)
        {
            nwhdr->seq_num = ZB_CERT_HACKS().nwk_counter_custom_value;
        }
#endif /* ZB_CERTIFICATION_HACKS */

        nwhdr->radius = nldereq->radius > 0U ? nldereq->radius : ZB_NIB_MAX_DEPTH() * 2U;

        if ( ZB_U2B(nldereq->discovery_route)
                && (!ZB_NWK_IS_ADDRESS_BROADCAST(nldereq->dst_addr))
                && (!ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(nwhdr->frame_control))))
        {
            ZB_NWK_FRAMECTL_SET_DISCOVER_ROUTE(nwhdr->frame_control, 1U);
#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
            if (ZB_BIT_IS_SET(nldereq->extension_flags, ZB_NLDE_OPT_FORCE_MESH_ROUTE_DISC))
            {
                zb_nwk_mesh_delete_route(nldereq->dst_addr);
                TRACE_MSG(TRACE_NWK1, "Mesh delete_route", (FMT__0));
            }
            else if (ZB_BIT_IS_SET(nldereq->extension_flags, ZB_NLDE_OPT_FORCE_SEND_ROUTE_REC))
            {
                zb_nwk_routing_t *route_ent;
                NWK_ARRAY_FIND_ENT(ZB_NIB().routing_table, ZB_NWK_ROUTING_TABLE_SIZE, route_ent, (route_ent->dest_addr == nldereq->dst_addr));
                if (route_ent != NULL)
                {
                    if (!ZB_U2B(route_ent->many_to_one))
                    {
                        nldereq->extension_flags |= ZB_NLDE_OPT_TEMPORARY_MARK_ROUTE_AS_MTO;
                        route_ent->route_record_required = ZB_TRUE;
                        ext_flags_tmp = nldereq->extension_flags;
                    }
                    route_ent->route_record_required = ZB_TRUE;
                    TRACE_MSG(TRACE_NWK1, "Set route record command frame buf begin %p nldereq %p buf len %ld", (FMT__P_P_L, zb_buf_begin(param), nldereq, zb_buf_len(param)));
                }
            }
#endif

        }

        {
#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
            zb_uint8_t *extension_flags;
#endif
            (void)zb_nwk_init_apsde_data_ind_params(param, nldereq->ndsu_handle);
#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
            extension_flags = zb_buf_alloc_tail(param, sizeof(zb_uint8_t));
            *extension_flags = ext_flags_tmp;
#endif
            ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Seems too big data frame - do not send it", (FMT__0));
        nwk_call_nlde_data_confirm(param, ZB_NWK_STATUS_FRAME_NOT_BUFFERED, ZB_FALSE);
    }

    TRACE_MSG(TRACE_NWK1, "-nlde_data_request", (FMT__0));
}

/**
   Calculate destination address.

   The calculation the destination address:
   1) if it is broadcast
   2) in neighbor table (the dest is a neighbor of our)
   3) routing table (the route path exists)
   4) route discovery (the route path does not exists but mesh routing is in
   progress)
   5) tree (none of the above and the route discovery flag is disabled)

   @param nsdu - packet to send; NWK header is filled already
   @param handle - ndsu packet handle
   @param mac_dst - output parameter, short MAC address of the next hop if know
   it now, 0 if will wait
   @param indirect - output parameter, true if neighbor does not keep its rx on
   when idle (i.e. sleeping), then we need to send frame indirectly

   @return RET_OK on success, RET_BUSY - when route discovery has been initiated or
   route discovery is in progress, error code otherwise
*/
static zb_ret_t nwk_calc_destination(zb_bufid_t nsdu, zb_uint8_t *handle, zb_uint16_t *mac_dst, zb_bool_t *indirect, zb_nwk_command_status_t *cmd_status)
{
    zb_ret_t ret = RET_OK;
    zb_nwk_hdr_t *nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(nsdu);
#ifdef ZB_ROUTER_ROLE
    zb_neighbor_tbl_ent_t *nbh = NULL;
    zb_address_ieee_ref_t addr_ref;
#endif
#ifdef ZB_NWK_MESH_ROUTING
    zb_nwk_routing_t *route;
#endif

    ZVUNUSED(handle);
    ZVUNUSED(cmd_status);
    TRACE_MSG(TRACE_NWK1, ">>nwk_calc_destination nsdu %p handle %hd mac_dst %p indir %p cmd_status %p", (FMT__P_H_P_P_P, nsdu, *handle, mac_dst, indirect, cmd_status));

    /* set most frequent calculation result */
    *mac_dst = nwhdr->dst_addr;
    *indirect = ZB_FALSE;

    /* Mark packet routed in case if it has been issued by 3rd device, not by ourselves */
    if (nwhdr->src_addr != ZB_PIBCACHE_NETWORK_ADDRESS()
            && !ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr)
            && !ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(nwhdr->frame_control)))
    {
        *handle = ZB_NWK_INTERNAL_RELAYED_UNICAST_FRAME_CONFIRM_HANDLE;
    }

    /* ed sends all to its parent */
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZED())
#endif
    {
        /* but not during rejoin */
        if ( ZG->nwk.handle.state != ZB_NLME_STATE_REJOIN )
        {
            zb_address_short_by_ref(mac_dst, ZG->nwk.handle.parent);
            TRACE_MSG(TRACE_NWK1, "ED - send to parent 0x%x", (FMT__D, *mac_dst));
        }
    }
#ifdef ZB_ROUTER_ROLE
    else
    {
        TRACE_MSG(TRACE_NWK1, "dest addr 0x%x dev type %hd", (FMT__D_H, nwhdr->dst_addr, ZB_NIB().device_type));
        /* is the destination address is broadcast */
        if ( ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr)
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
                || ( (ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwhdr->frame_control)) &&
                     (GET_NWK_MCF(nwhdr).multicast_mode == ZB_NWK_MULTICAST_MODE_MEMBER) )
#endif
           )
        {
            *mac_dst = ZB_NWK_BROADCAST_ALL_DEVICES;
            TRACE_MSG(TRACE_NWK1, "broadcast, mac dst ~0", (FMT__0));
        }
#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING
        else if (ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(nwhdr->frame_control) != 0U)
        {
            zb_uint8_t index;
            zb_uint8_t *ptr_to_srt;

            ptr_to_srt = (zb_uint8_t *)(((zb_uint8_t *) nwhdr) + ZB_NWK_HDR_GET_BASE_SIZE(nwhdr));

            ptr_to_srt++;                 /* skip relay count */
            index = (*ptr_to_srt);
            if (index != 0U)
            {
                (*ptr_to_srt)--;            /* decrement index */
            }
            TRACE_MSG(TRACE_NWK2, "forwarding source route pkt cnt %d idx %d",
                      (FMT__D_D, *(ptr_to_srt - 1), index));
            ptr_to_srt++;                 /* skip index */

            if (index == 0U)
            {
                *mac_dst = nwhdr->dst_addr;
            }
            else
            {
                ZB_MEMCPY(mac_dst, (ptr_to_srt + ((zb_uint32_t)index - 1U) * sizeof(zb_uint16_t)), 2);
            }

            /* check if our dest is a sleepy device */
            if (zb_address_by_short(*mac_dst, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
            {
                if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbh) == RET_OK
                        && nbh->rx_on_when_idle == 0U)
                {
                    *indirect = ZB_TRUE;
                }
            }
        }
#endif  /* if source routing */

        /* is the destination our neighbor */
        else if (is_destination_our_neighbour(nwhdr, &nbh))
        {
            TRACE_MSG(TRACE_NWK1, "dst is neighb, rx_on %hd", (FMT__D, nbh->rx_on_when_idle));
#ifdef ZB_CERTIFICATION_HACKS
            /*
              3.6.3.3 Upon Receipt of a Unicast Frame

              A Zigbee router or Zigbee coordinator may check the neighbor table for
              an entry corresponding to the 8813 routing destination of the frame. If
              there is such an entry, the device may route the frame directly to the
              8814 destination using the MCPS-DATA.request primitive as described in
              section 3.6.2.1

              So, we can ignore routing tables and always route to the neighbor.
              But specially for the test TP_PRO_BV-04 use routing even for neighbor.
              Do not do it in other situations.
             */
            if (ZB_CERT_HACKS().use_route_for_neighbor &&
                    /* Always send directly to ZED child */
                    /* If the receiving device is a Zigbee router or Zigbee coordinator,
                     * and the destination of the frame is a Zigbee end device and
                     * also the child of the receiving device, the frame shall be routed
                     * directly to the destination using the MCPS-DATA.request
                     * primitive, as described in section 3.6.2.1.  */
                    !(nbh->device_type == ZB_NWK_DEVICE_TYPE_ED && nbh->relationship == ZB_NWK_RELATIONSHIP_CHILD))
            {
                route = zb_nwk_mesh_find_route(nwhdr->dst_addr);
                if (route
                        && (route->status == ZB_NWK_ROUTE_STATE_ACTIVE
                            || route->status == ZB_NWK_ROUTE_STATE_VALIDATION_UNDERWAY))
                {
                    TRACE_MSG(TRACE_NWK1, "found route for 0x%x better (?) than sending to neighbor, next hop ref %hd", (FMT__D_H, nwhdr->dst_addr, route->next_hop_addr_ref));
                    zb_address_short_by_ref(mac_dst, route->next_hop_addr_ref);
                    route->status = ZB_NWK_ROUTE_STATE_ACTIVE;
                    TRACE_MSG(TRACE_ATM1, "Z< found route for 0x%04x, next hop 0x%04x", (FMT__D_D, nwhdr->dst_addr, *mac_dst));
                }
            }
            else
#endif  /* ZB_CERTIFICATION_HACKS */
                if (!ZB_U2B(nbh->rx_on_when_idle))
                {
                    *indirect = ZB_TRUE;
                }
        }
#ifdef ZB_NWK_MESH_ROUTING
        /* is the destination in our routing table */
        else
        {
            route = zb_nwk_mesh_find_route(nwhdr->dst_addr);
            TRACE_MSG(TRACE_NWK3, "dst 0x%x send_via_routing %hd got route %p",
                      (FMT__D_H_P, nwhdr->dst_addr, nbh != NULL ? nbh->send_via_routing : 0, route));
            /* [MM]: Disabled the condition of DiscoverRoute, as per spec.
             * Refer to 3.6.3.3. Upon Receipt of Unicast Frame.
             * The reason is that the router ALREADY has active route to the destination device,
             * and there is no need to discover it once more time.
             * As for example, this condition fails for NWK Network Status frames,
             * they are sent w/o Discover Route bit in NWK header.
             * Still not sure about Validation_Underway status, as it's been removed in R23 spec.
             *
             * TODO: Refactor routing due to specification in R23.
             */

            if ( /* ZB_NWK_FRAMECTL_GET_DISCOVER_ROUTE(nwhdr->frame_control) && */
                route != NULL
                && (route->status == ZB_NWK_ROUTE_STATE_ACTIVE
                    || route->status == ZB_NWK_ROUTE_STATE_VALIDATION_UNDERWAY) )
            {
                TRACE_MSG(TRACE_NWK1, "found route for 0x%x, next hop ref %hd", (FMT__D_H, nwhdr->dst_addr, route->next_hop_addr_ref));
                zb_address_short_by_ref(mac_dst, route->next_hop_addr_ref);
                route->status = ZB_NWK_ROUTE_STATE_ACTIVE;
                TRACE_MSG(TRACE_ATM1, "Z< found route for 0x%04x, next hop 0x%04x", (FMT__D_D, nwhdr->dst_addr, *mac_dst));

#ifndef ZB_LITE_NO_SOURCE_ROUTING
                if (ZB_U2B(route->many_to_one) && ZB_U2B(route->route_record_required))
                {
                    /* Check if this packet from us or our ED child */
                    if (nwhdr->src_addr == ZB_PIBCACHE_NETWORK_ADDRESS()
#ifdef ZB_ROUTER_ROLE
                            /*cstat !MISRAC2012-Rule-13.5 */
                            /* After some investigation, the following violation of Rule 13.5 seems to be
                             * a false positive. There are no side effects to 'zb_nwk_ed_is_our_child()'. This
                             * violation seems to be caused by the fact that 'zb_nwk_ed_is_our_child()' is an external
                             * function, which cannot be analyzed by C-STAT. */
                            || zb_nwk_ed_is_our_child(nwhdr->src_addr)
#endif
                       )
                    {
#if defined ZB_ROUTER_ROLE && defined ZB_APSDE_REQ_ROUTING_FEATURES
                        zb_uint8_t *extension_flags = ZB_BUF_GET_PARAM(nsdu, zb_uint8_t);
                        TRACE_MSG(TRACE_NWK3, "ext_flags extension flags %hd", (FMT__H, *extension_flags));
                        if (ZB_BIT_IS_SET(*extension_flags, (ZB_NLDE_OPT_FORCE_SEND_ROUTE_REC | ZB_NLDE_OPT_TEMPORARY_MARK_ROUTE_AS_MTO)))
                        {
                            route->many_to_one = ZB_FALSE;
                            TRACE_MSG(TRACE_NWK1, "reset ZB_NLDE_OPT_TEMPORARY_MARK_ROUTE_AS_MTO flag", (FMT__0));
                        }
#endif
                        TRACE_MSG(TRACE_NWK2, "Need to send RREC prior to send a packet", (FMT__0));

                        /* Need to send RREC prior to send the packet */
                        ret = zb_nwk_send_rrec_prior(ZB_BUF_INVALID, nwhdr->src_addr, nwhdr->dst_addr, *mac_dst);
                        if (ret != RET_OK)
                        {
                            TRACE_MSG(TRACE_NWK1, "Failed to send RREC prior to send a packet!", (FMT__0));
                            /* Send a packet without RREC */
                            ret = RET_OK;
                        }
                    }
                }
#endif /* ZB_LITE_NO_SOURCE_ROUTING */
            }
            /* discovery is in progress */
            else if ( ZB_U2B(ZB_NWK_FRAMECTL_GET_DISCOVER_ROUTE(nwhdr->frame_control))
                      && route != NULL
                      && route->status == ZB_NWK_ROUTE_STATE_DISCOVERY_UNDERWAY )
            {
                TRACE_MSG(TRACE_NWK2, "disc in progress - who is it?", (FMT__0));

                /* add buffer to the pending list */
                ret = zb_nwk_mesh_add_buf_to_pending(nsdu, *handle);
                TRACE_MSG(TRACE_NWK2, "add buf to pending %d", (FMT__D, ret));
                if ( ret == RET_OK )
                {
                    /* check who initiate discovery */
                    zb_nwk_route_discovery_t *disc_entry = zb_nwk_mesh_find_route_discovery_entry(nwhdr->dst_addr);

                    if ( disc_entry == NULL
                            || disc_entry->source_addr != ZB_PIBCACHE_NETWORK_ADDRESS() )
                    {
                        /* initiate route discovery */
                        zb_nwk_mesh_route_discovery(ZB_BUF_INVALID, nwhdr->dst_addr, 0, ZB_NWK_RREQ_TYPE_NOT_MTORR);
                    }
                    else
                    {
                        TRACE_MSG(TRACE_NWK3, "already init r disc to %d", (FMT__D, nwhdr->dst_addr));
                    }

                    /* discovery is in progress */
                    ret = RET_BUSY;
                }
                else
                {
                    *cmd_status = ZB_NWK_COMMAND_STATUS_NO_ROUTE_AVAILABLE;
                }
            }
            /* are we allowed to discover the route? */
            else if ( ZB_U2B(ZB_NWK_FRAMECTL_GET_DISCOVER_ROUTE(nwhdr->frame_control))
                      && route == NULL)
            {
                TRACE_MSG(TRACE_NWK2, "init r disc to 0x%x", (FMT__D, nwhdr->dst_addr));

                /* add buffer to the pending list */
                ret = zb_nwk_mesh_add_buf_to_pending(nsdu, *handle);
                TRACE_MSG(TRACE_NWK3, "add buf to pend.l. %d", (FMT__D, ret));
                if ( ret == RET_OK )
                {
                    /* initiate route discovery */
                    zb_nwk_mesh_route_discovery(ZB_BUF_INVALID, nwhdr->dst_addr, 0, ZB_NWK_RREQ_TYPE_NOT_MTORR);
                    /* discovery is in progress */
                    ret = RET_BUSY;
                }
                else
                {
                    *cmd_status = ZB_NWK_COMMAND_STATUS_NO_ROUTE_AVAILABLE;
                }
            }
#endif  /* ZB_NWK_MESH_ROUTING */
            /* seems like we need to use tree routing, when DISCOVER_ROUTE bit in framectl
             * is 0, but we have no such frames */
#ifdef ZB_NWK_TREE_ROUTING
            else if ( ZB_NWK_FRAMECTL_GET_DISCOVER_ROUTE(nwhdr->frame_control)
                      && ZB_NIB_GET_USE_TREE_ROUTING() )
            {
                /* calc next hop */
                nbh = zb_nwk_tree_routing_route(nwhdr->dst_addr);
                TRACE_MSG(TRACE_NWK2, "route using tree r, to the neighb %p", (FMT__P, nbh));
                if ( nbh )
                {
                    zb_address_short_by_ref(mac_dst, nbh->addr_ref);
                    TRACE_MSG(TRACE_NWK3, "neighb addr %d", (FMT__D, *mac_dst));
                }
                else
                {
                    TRACE_MSG(TRACE_ERROR, "tree route fail, n.t. empty?", (FMT__0));
                    ret = RET_ERROR;
                    *cmd_status = ZB_NWK_COMMAND_STATUS_TREE_LINK_FAILURE;
                }
            }
#endif  /* ZB_NWK_TREE_ROUTING */
            else
            {
                TRACE_MSG(TRACE_ERROR, "Unable to route!, get_discovery_route %hd", (FMT__H, ZB_NWK_FRAMECTL_GET_DISCOVER_ROUTE(nwhdr->frame_control)));
                ret = RET_ERROR;
                *cmd_status = ZB_NWK_COMMAND_STATUS_NO_ROUTING_CAPACITY;
            }
        }
    }
#endif  /* ZB_ROUTER_ROLE */

    TRACE_MSG(TRACE_NWK1, "<<nwk_calc_destination %d", (FMT__D, ret));
    return ret;
}

static zb_uint8_t zb_nwk_cmd_frame_get_cmd_id(zb_bufid_t param)
{
    void *ptr = zb_buf_begin(param);
    zb_uint8_t *nwhdr = ptr;
    return nwhdr[zb_get_nwk_header_size((zb_nwk_hdr_t *)ptr)];
}

/**
   Forward (or send) packet

   Define destination, if it already known - send packet to MAC

   @param param - NWK packet to proceed.
*/

void zb_nwk_forward(zb_uint8_t param)
{
    zb_ret_t ret;
    zb_bool_t indirect;
    zb_uint16_t mac_dst;
    zb_nwk_hdr_t *nwhdr = zb_buf_begin(param);
    zb_uint8_t handle = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t)->handle;
    zb_nwk_command_status_t cmd_status = ZB_NWK_COMMAND_STATUS_NO_ROUTE_AVAILABLE;

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_forward(%hd)", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

#if defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
    /* Put this packet to queue if Inter-Pan procedure is enabled and in progress */
    if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_nwk_packet_to_queue != NULL)
    {
        if (ZB_APS_INTRP_MCHAN_FUNC_SELECTOR().put_nwk_packet_to_queue(param) == RET_OK)
        {
            return;
        }
    }
#endif /* defined ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */

    /* Try to requalify buffer to "out" type in case when "in" buffer is used for packet sending.
     * Such situation could happen on upper layers, for example, to send a ZCL Response as ZCL will use "in" buffer
     * for a request to send the response.
     */
    if (zb_buf_requalify_in_to_out(param) != RET_OK)
    {
        TRACE_MSG(TRACE_NWK1, "buffer requalifiaction is not performed, buf %hd", (FMT__H, param));
    }

    TRACE_MSG(TRACE_NWK3, "forward: pckt multicast flag=%hd ", (FMT__H, ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwhdr->frame_control)));

#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        /* Current frame can be retransmitted end device frame */
        ZB_NWK_FRAMECTL_CLR_END_DEVICE_INITIATOR(nwhdr->frame_control);
    }
#endif

    ret = nwk_calc_destination(param, &handle, &mac_dst, &indirect, &cmd_status);
    TRACE_MSG(TRACE_NWK3, "forward: mac_dst addr=0x%hx", (FMT__H, mac_dst));
#ifdef ZB_PRO_STACK
    if ( ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND )
    {
        zb_uint8_t cmd_id = zb_nwk_cmd_frame_get_cmd_id(param);
        ZVUNUSED(cmd_id);
        TRACE_MSG(TRACE_NWK3, "forwarding cmd id, 0x%hx", (FMT__H, cmd_id));
    }
#endif

    if ( ret == RET_OK )
    {
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
        /* check address is broadcast. */
        if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
                && (ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr)
#ifndef ZB_NO_NWK_MULTICAST
                    || ZB_NWK_IS_MULTICAST_MEMBER_MODE(mac_dst, nwhdr->frame_control)
#endif
                   ))
        {
            zb_nwk_btr_t *btr = zb_nwk_broadcasting_find_btr(nwhdr);
            if (btr == NULL)
            {
                if (!zb_nwk_broadcasting_add_btr(nwhdr))
                {
                    TRACE_MSG(TRACE_NWK3, "btt tbl full, drop pkt", (FMT__0));
                    ret = RET_ERROR;
                    goto done;
                }
            }
        }
#endif

        /* re-assign because data can be moved in the packet */
        nwhdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
#ifdef ZB_ROUTER_ROLE
        TRACE_MSG(TRACE_NWK3, "dst 0x%x is_br %hd dev_t %hd brrt_cnt %hd",
                  (FMT__D_H_H_D, nwhdr->dst_addr,
                   ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr),
                   ZB_NIB_DEVICE_TYPE(), ZG->nwk.handle.brrt_cnt));

        if ((ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr)
#ifndef ZB_NO_NWK_MULTICAST
                || ZB_NWK_IS_MULTICAST_MEMBER_MODE(mac_dst, nwhdr->frame_control)
#endif
            )
                /*cstat !MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of Rule 13.5 seems to be a false
                 * positive. There are no side effects to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This violation seems to
                 * be caused by the fact that this function is an external function, which cannot be
                 * analyzed by C-STAT. */
                && ZB_IS_DEVICE_ZC_OR_ZR())
        {
            ZB_SCHEDULE_CALLBACK(zb_nwk_broadcasting_transmit, param);
        }
        else
#endif /* ifdef ZB_ROUTER_ROLE */
        {
            /* Unlock next IN param processing */
            /* zb_nwk_unlock_in(param); */

            TRACE_MSG(TRACE_NWK1, "fill mcps data req %hd src 0x%hx dst 0x%hx indirect %hd h %hd",
                      (FMT__H_H_H_H_H, param, ZB_PIBCACHE_NETWORK_ADDRESS(), mac_dst, indirect, handle));

            zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(), mac_dst,
                                       ((indirect ? MAC_TX_OPTION_INDIRECT_TRANSMISSION_BIT : 0U) |
                                        /* if not broadcast, wants ack */
                                        ((mac_dst != ZB_NWK_BROADCAST_ALL_DEVICES) ? MAC_TX_OPTION_ACKNOWLEDGED_BIT : 0x0U)),
                                       handle);

#ifdef ZB_ENABLE_NWK_RETRANSMIT
            /* Set NwkRetryCount value to retry counter if buffer parameter.
             * It will be decreased every time nwk param is failed to delivery */
            if (!ZB_NWK_IS_ADDRESS_BROADCAST(nwhdr->dst_addr))
            {
                zb_mcps_data_req_params_t *_p = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
                _p->nwk_retry_cnt = ZB_NWKC_UNICAST_RETRIES + 1U;
            }
#endif /* ZB_ENABLE_NWK_RETRANSMIT */

            ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
        }
        /* Convert addresses to LE order */
        /* It works because zb_nwk_broadcasting_transmit or
         * zb_mcps_data_request is scheduled, but not executed yet, so we
         * patch nwk packet before its execution. Ugly solution. */
        ZB_NWK_ADDR_TO_LE16(nwhdr->dst_addr);
        ZB_NWK_ADDR_TO_LE16(nwhdr->src_addr);
    }
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
done:
#endif
    if ( ret != RET_OK
            && ret != RET_BUSY )
    {
#ifdef ZB_ROUTER_ROLE
        zb_neighbor_tbl_ent_t *nbt = NULL;
#endif

        TRACE_MSG(TRACE_NWK1, "cant deliver param %p err %d cmd_status %hd", (FMT__P_D_H, param, ret, cmd_status));

        /* check if it's our child */
#ifdef ZB_ROUTER_ROLE
        if ( zb_nwk_neighbor_get_by_short(mac_dst, &nbt) == RET_OK
                && nbt != NULL
                && (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD
                    || nbt->relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD) )
        {
            call_status_indication(0, cmd_status, mac_dst);
        }
#endif
        {
            /* NK: pass data_confirm here to done with this param */
            /* EE: add * */
            /* zb_uint8_t handle = *ZB_BUF_GET_PARAM(param, zb_uint8_t); */
            if (handle < ZB_NWK_MINIMAL_INTERNAL_HANDLE)
            {
                nwk_call_nlde_data_confirm(param, ZB_NWK_STATUS_NO_ROUTING_CAPACITY, ZB_TRUE);
            }
            else
            {
                zb_buf_free(param);
            }
        }
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_forward", (FMT__0));
}



/**
  translate MAC status to NWK command status
*/
static zb_nwk_command_status_t mac_status_2_nwk_command_status(zb_mac_status_t mac_status)
{
    zb_nwk_command_status_t ret = (zb_nwk_command_status_t)mac_status;

    switch (mac_status )
    {
    case MAC_NO_ACK:
    case MAC_CHANNEL_ACCESS_FAILURE:
        ret = ZB_NWK_COMMAND_STATUS_PARENT_LINK_FAILURE;
        break;
    case MAC_TRANSACTION_OVERFLOW:
    case MAC_TRANSACTION_EXPIRED:
        ret = ZB_NWK_COMMAND_STATUS_INDIRECT_TRANSACTION_EXPIRY;
        break;
    case MAC_PURGED:
        /* Let's use some nwk retcode. Eliminate using of MAC codes in higher level logic. */
        ret = ZB_NWK_STATUS_FRAME_NOT_BUFFERED;
        break;
    default:
        break;
    }
    return ret;
}


static void call_nlme_status_ind(zb_uint8_t param, zb_uint16_t par2)
{
    zb_nlme_status_indication_t *status =  ZB_BUF_GET_PARAM(param, zb_nlme_status_indication_t);
    zb_uint16_t addr;
    zb_uint8_t mac_status = (zb_uint8_t)ZB_GET_HI_BYTE(par2);

    status->status = mac_status_2_nwk_command_status((zb_mac_status_t)mac_status);
    zb_address_short_by_ref(&addr, (zb_address_ieee_ref_t)ZB_GET_LOW_BYTE(par2));
    status->network_addr = addr;
    TRACE_MSG(TRACE_NWK1, "calling zb_nlme_status_indication param %hd status %hd dest_addr 0x%04x", (FMT__H_H_D, param, status->status, addr));

    ZB_SCHEDULE_CALLBACK(zb_nlme_status_indication, param);
}


static void call_status_indication(zb_uint8_t param, zb_nwk_command_status_t status, zb_uint16_t dest_addr)
{
    zb_ret_t ret;
    zb_address_ieee_ref_t ref;

    ret = zb_address_by_short(dest_addr, ZB_FALSE, ZB_FALSE, &ref);
    if (ret == RET_OK)
    {
        zb_uint16_t par2 = ref;

        ZB_ASSERT_COMPILE_TIME(sizeof(status) <= sizeof(zb_uint16_t));
        ZB_SET_HI_BYTE(par2, status);
        if (param != 0U)
        {
            ZB_SCHEDULE_CALLBACK2(call_nlme_status_ind, param, par2);
        }
        else
        {
            ret = zb_buf_get_out_delayed_ext(call_nlme_status_ind, par2, 0);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
        }
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "Attempt to send nwk status to unknown addr 0x%x", (FMT__D, dest_addr));
    }

    if (ret != RET_OK)
    {
        if (param != 0U)
        {
            zb_buf_free(param);
        }
    }
}

#ifdef ZB_ENABLE_NWK_RETRANSMIT
static void nwk_do_retry(zb_bufid_t param)
{
    zb_mcps_data_confirm_params_t confirm = *ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    zb_mcps_data_req_params_t *req = zb_buf_alloc_tail(param, sizeof(zb_mcps_data_req_params_t));

    /* memcpy because that are different structures. Can ignore difference if not GP tx */
    ZB_MEMCPY(&req->src_addr, &confirm.src_addr, sizeof(confirm.src_addr));
    req->dst_addr = confirm.dst_addr;
    req->dst_pan_id = confirm.dst_pan_id;
    req->tx_options = confirm.tx_options;
    req->src_addr_mode = confirm.src_addr_mode;
    req->dst_addr_mode = confirm.dst_addr_mode;
    req->nwk_retry_cnt = confirm.nwk_retry_cnt;
    req->msdu_handle = confirm.msdu_handle;
    ZB_SCHEDULE_ALARM(zb_mcps_data_request, param, ZB_NWKC_UNICAST_RETRY_DELAY);

    TRACE_MSG(TRACE_NWK1, "Scheduling NWK Retry (buf %hd). Remaining retry count is %hd", (FMT__H_D, param, confirm.nwk_retry_cnt));
}
#endif /* ZB_ENABLE_NWK_RETRANSMIT */

void zb_mcps_data_confirm(zb_uint8_t param)
{
    zb_mcps_data_confirm_params_t *confirm = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    zb_uint16_t dest_addr = confirm->dst_addr.addr_short;
#if defined ZB_ROUTER_ROLE || defined ZB_ENABLE_INTER_PAN_EXCHANGE
    zb_nwk_hdr_t *nwk_hdr = zb_buf_begin(param);
#endif

    /* This is NWK handler for mcps.confirm.  Pass mcps.confirm up, to the APS layer.  */
    TRACE_MSG(TRACE_NWK1, ">> zb_mcps_data_confirm param %hd handle %hd status %hd",
              (FMT__H_H_H, param, confirm->msdu_handle, confirm->status));

#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        /* Check for status from confirm.
         * TX total increased for every transmission.
         */
        nwk_txstat_tx_inc();

        if (confirm->status == MAC_CHANNEL_ACCESS_FAILURE)
        {
            /* Increment nwk tx fail counter only for channel access failure to prevent
             * channel change in case of absent mac ack. */
            nwk_txstat_fail_inc();
        }
    }
#endif

#ifdef ZB_ENABLE_NWK_RETRANSMIT
    if (confirm->status == MAC_CHANNEL_ACCESS_FAILURE
            || confirm->status == MAC_NO_ACK)
    {
        if (confirm->nwk_retry_cnt != 0U)
        {
            confirm->nwk_retry_cnt--;

            if (confirm->nwk_retry_cnt == 0U)
            {
                ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NWK_RETRY_OVERFLOW_ID);
            }
            else if (ZB_JOINED())
            {
                ZB_SCHEDULE_CALLBACK(nwk_do_retry, param);
                return;
            }
            else
            {
                /* MISRA rule 15.7 requires empty 'else' branch. */
            }
        }
    }
#endif /* ZB_ENABLE_NWK_RETRANSMIT */

    if (confirm->status == MAC_SUCCESS)
    {
        /* kick watchdog if successfully sent anything */
        ZB_KICK_WATCHDOG(ZB_WD_ZB_TRAFFIC);
    }

    /* There was complex logic which can be eliminated if we never send to MAC the original buffer.
       Nos changed design to always send a copy to MAC.
       After MAC called confirm, that copy can be safely dropped, or rebroadcasted in case of channel access failure.
       That is simple, so smaller and more robust.
     */

    switch (confirm->msdu_handle)
    {
#ifdef ZB_JOIN_CLIENT
    case ZB_NWK_INTERNAL_REJOIN_CMD_HANDLE:
        if (ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN )
        {
            ZB_SCHEDULE_ALARM_CANCEL(zb_nlme_rejoin_response_timeout, ZB_ALARM_ANY_PARAM);
#ifdef ZB_ED_FUNC
            if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
            {
                TRACE_MSG(TRACE_NWK1, "schedule zb_nlme_rejoin_response_timeout %d",
                          (FMT__D, ZB_NWK_REJOIN_TIMEOUT / ZB_NWK_REJOIN_POLL_ATTEMPTS));
                ZB_SCHEDULE_ALARM(zb_nlme_rejoin_response_timeout, 0,
                                  ZB_NWK_REJOIN_TIMEOUT / ZB_NWK_REJOIN_POLL_ATTEMPTS);
                ZG->nwk.handle.tmp.rejoin.poll_attempts = 0;
                nwk_next_rejoin_poll(param);
            }
            else
#endif  /* ZB_ED_FUNC */
            {
                TRACE_MSG(TRACE_NWK1, "schedule zb_nlme_rejoin_response_timeout %d",
                          (FMT__D, ZB_NWK_REJOIN_TIMEOUT));
                ZB_SCHEDULE_ALARM(zb_nlme_rejoin_response_timeout, 0,
                                  ZB_NWK_REJOIN_TIMEOUT);
                zb_buf_free(param);
            }
        }
        break;

    case ZB_NWK_INTERNAL_LEAVE_IND_AT_DATA_CONFIRM_HANDLE:
        TRACE_MSG(TRACE_NWK3, "schedule leave.indication", (FMT__0));
        zb_nwk_call_leave_ind(param, ZG->nwk.leave_context.rejoin_after_leave, (zb_address_ieee_ref_t)~0U);
        break;
#ifdef ZB_ED_FUNC
#ifndef ZB_LITE_NO_ED_AGING_REQ
    case ZB_NWK_INTERNAL_ED_TIMEOUT_REQ_FRAME_COFIRM_HANDLE:
        zb_nwk_ed_timeout_req_frame_confirm(param);
        break;
#endif
#endif
#endif  /* ZB_JOIN_CLIENT */
#ifdef ZB_ROUTER_ROLE
    case ZB_NWK_INTERNAL_REJOIN_CMD_RESPONSE:
        zb_nlme_rejoin_resp_sent(param);
        break;
#endif
#ifdef ZB_ENABLE_ZGP
    case ZB_MAC_DIRECT_GPDF_MSDU_HANDLE:
        TRACE_MSG(TRACE_NWK3, "schedule cgp-data.confirm", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_cgp_data_cfm, param);
        break;
#endif /* ZB_ENABLE_ZGP */
#ifdef ZB_ROUTER_ROLE
    case ZB_NWK_INTERNAL_RELAYED_UNICAST_FRAME_CONFIRM_HANDLE:
        nwk_check_relay_frame_status(param);
        break;
#endif  /* ZB_ROUTER_ROLE */

#ifdef ZB_SEND_OOM_STATUS
    case ZB_NWK_INTERNAL_OMM_STATUS_CONFIRM_HANDLE:
        TRACE_MSG(TRACE_NWK3, "schedule oom_status.confirm", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_oom_status_confirm, param);
        break;
#endif  /* ZB_SEND_OOM_STATUS */

    case ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE:
    {
        zb_nlme_leave_confirm_t *lc;
        zb_nwk_status_t status = (zb_nwk_status_t)confirm->status;
        zb_uint16_t addr_short = confirm->dst_addr.addr_short;

        /* zb_buf_alloc_tail zeroes parameters */
        lc = zb_buf_alloc_tail(param, sizeof(zb_nlme_leave_confirm_t));
        lc->status = status;
        if (addr_short != ZB_NWK_BROADCAST_ALL_DEVICES)
        {
            /* if destination was not broadcast, packet has 'request' bit set */
            if (zb_address_ieee_by_short(addr_short, lc->device_address) != RET_OK)
            {
                /* Handle injected leave whet we sent LEAVE to forgotten
                 * device which is absent in our address translation table.
                 *
                 * In that case, if keep lc->device_address zero,
                 * zb_nlme_leave_confirm decides that is our device who is
                 * leaving.
                 * To prevent that fill address by -1.
                 */
                ZB_64BIT_ADDR_UNKNOWN(lc->device_address);
            }
        }
        TRACE_MSG(TRACE_NWK3, "schedule leave.confirm status %d dst_addr %d " TRACE_FORMAT_64,
                  (FMT__D_D_A, status, addr_short, TRACE_ARG_64(lc->device_address)));
        ZB_SCHEDULE_CALLBACK(zb_nlme_leave_confirm, param);
        break;
    }

    default:
        if (confirm->msdu_handle >= ZB_NWK_MINIMAL_INTERNAL_HANDLE)
        {
            /* Any internal NWK transfer not covered by previous checks */
            if (confirm->status != MAC_SUCCESS)
            {
                /* send status.indication */
                TRACE_MSG(TRACE_NWK3, "internal nwk transmission - send nwk status ind", (FMT__0));
                call_status_indication(param, (zb_nwk_command_status_t)confirm->status, dest_addr);

#if defined ZB_PRO_STACK && defined ZB_ROUTER_ROLE && !defined ZB_LITE_NO_SOURCE_ROUTING
                /* #AT checking for source routing transmission status of data packets */
                /*
                  There must be NWK destination address:

                  3.4.3.3.2 Destination Address
                  The destination address field is 2 octets in length and shall be present if, and
                  only if, the network status command frame is being sent in response to a routing
                  failure. In this case, it shall contain the destination address from the data
                  frame that encountered the failure.  */
                if (ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(nwk_hdr->frame_control) != 0U)
                {
                    zb_uint16_t src_addr = nwk_hdr->src_addr;

                    ZB_HTOLE16_ONPLACE(src_addr);
                    /* Can we assume both addresses are in our translation table?? If no, can't compress both addresses into 2-bytes second param. */
                    ZG->nwk.handle.status_addr = nwk_hdr->dst_addr;
                    ZB_HTOLE16_ONPLACE(ZG->nwk.handle.status_addr);
                    TRACE_MSG(TRACE_NWK3, "its packet source route data retrans with status = 0x%x;, src 0x%x", (FMT__H_D, confirm->status, src_addr));
                    TRACE_MSG(TRACE_ATM1, "Z< its packet source route data retrans with status = 0x%02x;, src 0x%04x", (FMT__H_D, confirm->status, src_addr));
                    {
                        zb_ret_t ret = zb_buf_get_out_delayed_ext(zb_send_nwk_status_source_route_fail, src_addr, 0);
                        if (ret != RET_OK)
                        {
                            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
                        }
                    }
                }
#endif  /* if source routing */
            }
            else
            {
                TRACE_MSG(TRACE_NWK3, "free confirm to internal nwk transmission", (FMT__0));
                zb_buf_free(param);
            }
        }
        else /* All not internal NSDU handles. */
        {
            /* This is confirm of TX initiated by user's data request. */
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE
            TRACE_MSG(
                TRACE_NWK3,
                "NWK frame type 0x%hx",
                (FMT__H, ZB_NWK_FRAMECTL_GET_FRAME_TYPE((zb_uint8_t *)nwk_hdr)));
            if (ZB_NWK_FRAMECTL_GET_FRAME_TYPE((zb_uint8_t *)nwk_hdr)
                    ==  ZB_NWK_FRAME_TYPE_INTER_PAN)
            {
                TRACE_MSG(TRACE_NWK3, "Passing up to interpan Stub-APS", (FMT__0));
                ZB_SCHEDULE_CALLBACK(zb_intrp_data_frame_confirm, param);
            }
            else
#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
            {
#ifdef ZB_ROUTER_ROLE
                if (ZB_IS_DEVICE_ZC_OR_ZR()
                        && confirm->status != MAC_SUCCESS
                        && confirm->status != MAC_PURGED)
                {
                    zb_uint16_t addr;
                    ZB_LETOH16((zb_uint8_t *)&addr, (zb_uint8_t *)&nwk_hdr->dst_addr);
                    /* get destination from NWK hdr, do 1 step route expire */
                    if (!ZB_NWK_IS_ADDRESS_BROADCAST(addr))
                    {
                        zb_nwk_route_expire(addr);
                    }
                }
#endif
                if ( confirm->status != MAC_SUCCESS
                        && confirm->status != MAC_TRANSACTION_OVERFLOW
                        && confirm->status != MAC_PURGED
                   )
                {
                    TRACE_MSG(TRACE_NWK3, "calling call_status_indication status %d dest_addr 0x%x", (FMT__D_H, confirm->status, dest_addr));
                    call_status_indication(0, (zb_nwk_command_status_t)confirm->status, dest_addr);
                }
#ifdef ZB_ED_FUNC
                else if (ZB_IS_DEVICE_ZED())
                {
                    /* Reset counter on success */
                    ZDO_CTX().parent_link_failure = 0;
                }
#endif
                else
                {
                    /* MISRA rule 15.7 requires empty 'else' branch. */
                }

                /* Pass conf up to APS. */
                TRACE_MSG(TRACE_NWK3, "schedule nlde-data.confirm %hd", (FMT__H, param));
                nwk_call_nlde_data_confirm(param, mac_status_2_nwk_command_status((zb_uint8_t)confirm->status), ZB_TRUE);
            } /* else (!interpan) */
        }
        break;
    } /* switch (handle) */

    TRACE_MSG(TRACE_NWK1, "<< zb_mcps_data_confirm", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE
static void nwk_allocate_n_forward(zb_uint8_t new_param, zb_uint16_t orig_param)
{
    zb_apsde_data_ind_params_t *new_pkt_params;

    TRACE_MSG(TRACE_NWK1, ">>nwk_allocate_n_forward %hd orig %hd",
              (FMT__H_H, new_param, orig_param));

    ZB_ASSERT(new_param);

    /* dup buffer  */
    zb_buf_copy(new_param, (zb_uint8_t)orig_param);
    /*
      Encrypt forwarded by the same nwk key source packet was encrypted by.
      To be used, for instance, to encrypt forwarded switch-key aps command.
    */
    TRACE_MSG(TRACE_SECUR3, "use same key for secure in buf %hd", (FMT__H, new_param));
    zb_buf_flags_or(new_param, ZB_BUF_USE_SAME_KEY);

    /* pass original buffer up. So do not unlock input yet. */
    ZB_SCHEDULE_CALLBACK(nwk_frame_indication, (zb_uint8_t)orig_param);

    /* Call data retransmit. May do not worry about delays: new buffer does not
     * block input. */
    new_pkt_params = ZB_BUF_GET_PARAM(new_param, zb_apsde_data_ind_params_t);
    new_pkt_params->handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, new_param);

    TRACE_MSG(TRACE_NWK1, "<<nwk_allocate_n_forward", (FMT__0));
}

#endif  /* ZB_ROUTER_ROLE */

void mcps_indication_process_in(zb_uint8_t param);

void zb_mcps_data_indication(zb_uint8_t param)
{
    zb_mac_mhr_t mac_hdr;
    zb_uint_t    mhr_size;
    zb_uint8_t   *fc;
    TRACE_MSG(TRACE_NWK1, "> zb_mcps_data_indication param %hd input_blocked_by %hd",
              (FMT__H_H, param, ZG->nwk.handle.input_blocked_by));

    /* Get nwk fc */
    mhr_size = zb_parse_mhr(&mac_hdr, param);
    fc = (zb_uint8_t *) zb_buf_begin(param) + mhr_size;

    /* Check version */
    switch (ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(fc))
    {
    case ZB_PROTOCOL_VERSION:
    {
        /* Note: we stop turbo here after getting any data. It could be not optimal. */
        /* If it is not we are polling for (broadcast/NWK packet, or APS dup), turbo polling will restart. */
        /* Accept packets only if we are joined or performing rejoin */
        /* NK: Also allow BDB touchlink target/initiator to process scan requests/responses.

           TODO: check other possible cases. */
        if (ZB_JOINED() || (ZG->nwk.handle.state == ZB_NLME_STATE_REJOIN)
                || (ZG->nwk.selector.should_accept_frame_before_join != NULL
                    /*cstat !MISRAC2012-Rule-10.1_R2 */
                    /* It seems to be a false positive */
                    && ZG->nwk.selector.should_accept_frame_before_join(param))
           )
        {
            TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_got_data", (FMT__0));
            zb_zdo_pim_got_data(0);
            ZB_SCHEDULE_CALLBACK_PRIOR(mcps_indication_process_in, param);
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "Drop packet - we are not joined and not in rejoin, state %hd", (FMT__H, ZG->nwk.handle.state));
            zb_buf_free(param);
        }
        break;
    }

#ifdef ZB_ENABLE_ZGP
    case ZB_ZGP_PROTOCOL_VERSION:
    {
        ZB_SCHEDULE_CALLBACK(zb_gp_mcps_data_indication, param);
        break;
    }
#endif

    default:
        TRACE_MSG(TRACE_ERROR, "Drop packet with improper protocol version: 0x%x", (FMT__H, ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(fc)));
        zb_buf_free(param);
        break;
    }

    TRACE_MSG(TRACE_NWK1, "< zb_mcps_data_indication", (FMT__0));
}


void zb_nwk_unlock_in(zb_uint8_t param)
{
    if (param != 0U                     /* can be here with 0 from MAC traffic dump */
            && ZG->nwk.handle.input_blocked_by == param)
    {
        TRACE_MSG(TRACE_COMMON2, "unlock in by buf %hd", (FMT__H, param));
        ZG->nwk.handle.input_blocked_by = 0;
        if (!ZB_RING_BUFFER_IS_EMPTY(ZB_NWK_IN_Q))
        {
            param = *ZB_RING_BUFFER_GET(ZB_NWK_IN_Q);
            ZB_BYTE_ARRAY_FLUSH_GET(ZB_NWK_IN_Q, ZB_NWK_IN_Q_SIZE);
            ZG->nwk.handle.input_blocked_by = param;
            TRACE_MSG(TRACE_INFO1, "set input_blocked_by and process %hd from in queue", (FMT__H, param));
            ZB_ASSERT(param);
            ZB_SCHEDULE_CALLBACK(zb_nlde_data_indication, param);
        }
    }
    else
    {
        TRACE_MSG(TRACE_COMMON3, "zb_nwk_unlock_in: not locked by %hd",  (FMT__H, param));
    }
}

/* For "fake" lock of the NWK input use impossible buffer id */
#define NWK_PARAM_RESERVED (ZB_N_BUF_IDS)

void nwk_internal_lock_in(void)
{
    if (ZG->nwk.handle.input_blocked_by  == 0U)
    {
        ZG->nwk.handle.input_blocked_by = NWK_PARAM_RESERVED;
    }
}


void nwk_internal_unlock_in(void)
{
    zb_nwk_unlock_in(NWK_PARAM_RESERVED);
}


void zb_mcps_data_indication_cont(zb_uint8_t param);

void mcps_indication_process_in(zb_uint8_t param)
{
    zb_mac_mhr_t mac_hdr;
    zb_nwk_hdr_t *nwk_hdr;
    zb_bool_t drop_pkt = ZB_FALSE;
    /* Note about data format: indication from MAC has no parameters section.
       MAC packet has only data section holding  MAC header, then data, then LQI and RSSI.
       So it is safe to allocate parameters now.
    */
    /* init packet params here, assign correct handle later */
    zb_apsde_data_ind_params_t *pkt_params = zb_nwk_init_apsde_data_ind_params(param, 0);

    TRACE_MSG(TRACE_NWK2, ">> mcps_indication_process_in param %hd", (FMT__H, param));

    /* get and parse mac header */
    pkt_params->lqi = ZB_MAC_GET_LQI(param);
    pkt_params->rssi = ZB_MAC_GET_RSSI(param);

    /* parse and remove MAC header */
    {
        zb_uint_t mhr_size = zb_parse_mhr(&mac_hdr, param);
        TRACE_MSG(TRACE_NWK1, "MAC src 0x%x dst 0x%x seq %hd len %hd lqi %hd rssi %hd",
                  (FMT__D_D_H_H_H_H, mac_hdr.src_addr.addr_short, mac_hdr.dst_addr.addr_short, mac_hdr.seq_number, zb_buf_len(param), pkt_params->lqi, pkt_params->rssi));
        ZB_MAC_CUT_HDR(param, mhr_size, nwk_hdr);
        TRACE_MSG(TRACE_NWK3, "mhr_size %d nwk pkt size %d", (FMT__D_D, mhr_size, zb_buf_len(param)));
    }

    /* Safety checks */

    /* Check protocol version */
    if (ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(nwk_hdr->frame_control) != ZB_PROTOCOL_VERSION)
    {
        TRACE_MSG(TRACE_NWK1, "drop packet with improper protocol version: required - 0x%x, received - 0x%x",
                  (FMT__H_H, ZB_PROTOCOL_VERSION, ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(nwk_hdr->frame_control)) );
        drop_pkt = ZB_TRUE;
    }
    /*[AM]
      Kill frames with incorrect length. Example: ZGP frame with wrong protocol
      ID. Covered by GPP_GPDF_BASIC_APP_010 gppb test.
    */
    else if ((zb_buf_len(param) - ZB_NWK_HDR_SIZE(nwk_hdr)) <= 0U)
    {
        TRACE_MSG(TRACE_NWK1, "drop packet with incorrect length: received length - 0x%x",
                  (FMT__H, zb_buf_len(param) ) );
        drop_pkt = ZB_TRUE;
    }
    /* Interpan? */
#if defined ZB_ENABLE_INTER_PAN_EXCHANGE
    else if (ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control) == ZB_NWK_FRAME_TYPE_INTER_PAN)
    {
        TRACE_MSG(TRACE_NWK3, "Scheduling inter-PAN frame indication", (FMT__0));
        /* NK:TODO: Fix polling for inter-PAN case. Now will assume that it is not APS pkt. */
        zb_intrp_data_frame_indication(param, &mac_hdr, pkt_params->lqi, pkt_params->rssi);
        zb_zdo_pim_continue_polling_for_pkt();
    }
#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
    /* Continue safety checks */
    else if (!
             (ZB_FCF_GET_SRC_ADDRESSING_MODE(mac_hdr.frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST
              && ZB_FCF_GET_DST_ADDRESSING_MODE(mac_hdr.frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
             || ZB_NWK_IS_ADDRESS_BROADCAST(mac_hdr.src_addr.addr_short))
    {
        TRACE_MSG(TRACE_NWK2, "drop packet with wrong src (%d) or dst (%d) addr mode or broadcast mac src 0x%x",
                  (FMT__D_D_D,
                   ZB_FCF_GET_SRC_ADDRESSING_MODE(mac_hdr.frame_control),
                   ZB_FCF_GET_DST_ADDRESSING_MODE(mac_hdr.frame_control),
                   mac_hdr.src_addr.addr_short));
        drop_pkt = ZB_TRUE;
    }
    else
        /* data packet */
    {
        /* Normal data packet */
        pkt_params->mac_src_addr = mac_hdr.src_addr.addr_short;
        pkt_params->mac_dst_addr = mac_hdr.dst_addr.addr_short;
#if !defined ZB_ED_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING && defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
        {
            zb_neighbor_tbl_ent_t *nbt;
            if (zb_nwk_neighbor_get_by_short(mac_hdr.src_addr.addr_short, &nbt) == RET_OK
                    && nbt->is_subghz)
            {
                nbt_inc_in_pkt_count(nbt);
            }
        }
#endif
        ZB_SCHEDULE_CALLBACK_PRIOR(zb_mcps_data_indication_cont, param);
    }

    if (drop_pkt)
    {
        zb_buf_free(param);
        zb_zdo_pim_continue_polling_for_pkt();
    }

    TRACE_MSG(TRACE_NWK2, "<< mcps_indication_process_in", (FMT__0));
}

#ifdef ZB_ROUTER_ROLE

#ifdef ZB_NWK_CHECK_MULTIHOP_TRANSMISSIONS_FROM_POS
/* [DT] Some devices (i.e. GE bulb) can come into network through router and continue talking
 * to coordinator indirectly even if they are within each other's personal operating space.
 * In this case coordinator erases nbt entry and does Route request.
 * However nbt entry will be created when broadcast receives broadcast without hop (i.e. Link Status).
 * This leads to periodical route requests from coordinator if router continues talking to coordinator indirectly.
 * To fix this we check manufacturer prefix in IEEE address
 * This change can harm workflow with other devices (was not tested) */

/* Prefixes of IEEE addresses for manufacturers that can show such behavior.
 * Mind the little endian when adding new entries */

static const zb_uint8_t zb_sticklers_for_indirect_list[][3] = { ZB_MANUF_PREFIX_QUIRKY_INC };

/* Return ZB_TRUE if neighbor seems to have left POS */
static zb_bool_t zb_nwk_extra_check_for_transmission_from_pos(zb_neighbor_tbl_ent_t *nbt)
{
    zb_uindex_t i;
    zb_ieee_addr_t ieee_address;

    zb_address_ieee_by_ref(ieee_address, nbt->u.base.addr_ref);
    /* To ensure the endian - no harm with local variable and little endian host */
    //ZB_HTOLE64_ONPLACE(ieee_address);

    for (i = 0; i < ZB_ARRAY_SIZE(zb_sticklers_for_indirect_list); i++)
    {
        /* Mind the little endian here */
        if (!ZB_MEMCMP(&(ieee_address[5]), zb_sticklers_for_indirect_list[i], 3))
        {
            /* If nbt entry aged (no link status received for some time), device must have left POS */
            return (nbt->u.base.age >= ZB_NIB().router_age_limit) ? ZB_TRUE : ZB_FALSE;
        }
    }
    return ZB_TRUE;
}
#endif

static void zb_nwk_check_child_presense(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_apsde_data_ind_params_t *mac_addrs = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);
    zb_ret_t ret;
    zb_neighbor_tbl_ent_t *nbt = NULL;

    TRACE_MSG(TRACE_NWK2, "zb_nwk_check_child_presense param %hd", (FMT__H, param));

    ret = RET_NOT_FOUND;
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control)))
    {
        /* Get neighbor by nwk long */
        ret = zb_nwk_neighbor_get_by_ieee(ZB_U2B(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control))
                                          ? nwk_hdr->src_ieee_addr
                                          : nwk_hdr->dst_ieee_addr, &nbt);
    }

    if (ret != RET_OK)
    {
        /* Get neighbor by nwk short if no long address present or if we know short address only */
        ret = zb_nwk_neighbor_get_by_short(nwk_hdr->src_addr, &nbt);
    }

    /* Just in case we compiled as ZR but work as ZED. ZED has the only entry in nbt - its parent */
    if (ZB_IS_DEVICE_ZC_OR_ZR()
            && ret == RET_OK)
    {
        ZB_ASSERT(nbt != NULL);
        /* Found long address of interest */

        /* Several conditions are needed to be sure that device is not out neighbor anymore: */
        if (
            /* Check for parent is now removed: ZR has no parents */
            /* 2. Packet should be sent through hop: compare addresses on MAC and NWK level */
            nwk_hdr->src_addr != mac_addrs->mac_src_addr
            /* 3. Ignore broadcasts - device might have lost original packet and receive rebroadcasted one */
            && ZB_MAC_IS_ADDRESS_BROADCAST(mac_addrs->mac_dst_addr) == 0U
#ifdef ZB_NWK_CHECK_MULTIHOP_TRANSMISSIONS_FROM_POS
            /* 4. Manufacturer specific check (in enabled) - some device might consistenly send packets to us
             *    through hop even if we communicate directly. Check manufacturer by IEEE address. */
            && zb_nwk_extra_check_for_transmission_from_pos(nbt)
#endif
        )
        {
            TRACE_MSG(TRACE_NWK2, "Got indirect rx of packet from our former neighbor 0x%x of type %hd", (FMT__D_H, nwk_hdr->src_addr, nbt->device_type));

            /* Addrs are not equal - it is not our child already!
               Clear neighbor table entry for it.

               There was a dirty hack created in lcgw_lnb_expo branch
               (means - this is fix for GW when prepating for some exibition):
               broadcast NWK address or IEEE request to ensure that device
               is not rejoined with another short address.
               Such no-so-probable problem solution causes tones of broadcasts over net
               if devices always goes to us via router but we see its broadcasts directly.
               It can be caused by an assimetrical link or buggy device.
               Originally that hack was introduced to exclude delays in communication with device which reqoins frequently.

               Anyway, that short address change must be handled at layers upper than NWK.

               Even current solution (remove from the neighbor table) is not standard and may cause questions.
               Better remove device from neigbors not now, but when tx to it failed.
               */

            if (nbt->device_type == ZB_NWK_DEVICE_TYPE_ED )
            {
                /*
                 * Delete from NBT only our ED children (not routers). Routers will be aged later or routing
                 * table will be used to transmit for this neighbors to avoid assimetric links.
                 */
                (void)zb_nwk_neighbor_delete(nbt->u.base.addr_ref);
                /* maybe our  end device child has changed parent - restart aging */
                zb_nwk_restart_aging();
            }
            else
            {
                nbt->send_via_routing = 1;
                TRACE_MSG(TRACE_NWK2, "former neighbor ZR 0x%x - set send_via_routing flag", (FMT__D, nwk_hdr->src_addr));
            }
        }
    }
}
#endif  /* ZB_ROUTER_ROLE */

static void send_unknown_command_status(zb_uint8_t param, zb_nwk_hdr_t *nwk_hdr, zb_uint8_t command)
{
    zb_nlme_send_status_t *request;

    if (nwk_hdr->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        request = ZB_BUF_GET_PARAM(param, zb_nlme_send_status_t);
        request->dest_addr = nwk_hdr->src_addr;
        request->status.status = ZB_NWK_COMMAND_STATUS_UNKNOWN_COMMAND;
        request->status.network_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
        request->status.unknown_command_id = command;
        request->ndsu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
        ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, param);

        TRACE_MSG(TRACE_NWK1, "Frame with unknown command identifier 0x%x, responding", (FMT__D, command));
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "Frame with unknown command identifier 0x%x, ignoring", (FMT__D, command));
        zb_buf_free(param);
    }
}


static zb_bool_t nwk_is_frame_unsecured_aps_tk(const zb_uint8_t *apsdu)
{
    /* Check if it is an APS Tranport Key command frame */
    zb_uint8_t aps_fc = *apsdu;
    zb_bool_t is_tk = ZB_FALSE;

    /* If an APS Command ... */
    if (ZB_APS_FC_GET_FRAME_TYPE(aps_fc) == ZB_APS_FRAME_COMMAND &&
            /* ... and it's secured ...*/
            (ZB_U2B(ZB_APS_FC_GET_SECURITY(aps_fc)) ||
             /* ... or not, but TK command */
             (!ZB_U2B(ZB_APS_FC_GET_SECURITY(aps_fc)) &&
              *(apsdu + sizeof(zb_aps_command_pkt_header_t)) == APS_CMD_TRANSPORT_KEY)))
    {
        is_tk = ZB_TRUE;
    }

    TRACE_MSG(TRACE_NWK3, "aps frame type %hd, secured %hd, is_tk %hd",
              (FMT__H_H_H, ZB_APS_FC_GET_FRAME_TYPE(aps_fc), ZB_APS_FC_GET_SECURITY(aps_fc), is_tk));

    return is_tk;
}


static zb_bool_t nwk_filter_unsecured_frame(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_uint8_t frame_type = ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control);
    zb_ushort_t hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);
    zb_uint8_t command_id = 0;
    /* Default is false, and this is fine */
    zb_bool_t pass;

    TRACE_MSG(TRACE_NWK3, ">> nwk_filter_unsecured_frame param %hd ", (FMT__H, param));

    if (frame_type == ZB_NWK_FRAME_TYPE_COMMAND)
    {
        command_id = zb_nwk_cmd_frame_get_cmd_id(param);
    }

    TRACE_MSG(TRACE_NWK3, "NIB secur lvl %hd, joined %hd, aps.auth %hd, frame_type %hd, cmd_id %hd",
              (FMT__H_H_H_H_H, ZB_NIB_SECURITY_LEVEL(), ZB_JOINED(), ZG->aps.authenticated, frame_type, command_id));

    /*
       ... If the security sub-field is set to 0, the
       8678 nwkSecurityLevel attribute in the NIB is non-zero, the device is currently joined and authenticated, and the
       8679 incoming frame is a NWK data frame, the NLDE shall discard the frame. If the security sub-field is set to
       8680 0, the nwkSecurityLevel attribute in the NIB is non-zero, and the incoming frame is a NWK command
       8681 frame and the command ID is 0x06 (rejoin request), the NLDE shall only accept the frame if it is destined
       8682 to itself, that is, if it does not need to be forwarded to another device. Otherwise the frame shall be
       8683 dropped and no further processing done.

       8684 If the device is not joined and authenticated, or undergoing the Trust Center Rejoin process, it shall perform
       8685 the following checks. If the frame is a NWK command where the security sub-field of the frame is set to
       8686 zero then it shall only accept the frame if the command ID is 0x07 (rejoin response). If the frame is a
       8687 NWK data frame where the security sub-field is set to 0, the device shall further examine the APDU and
       8688 determine if it contains an APS command ID of 0x05 (Transport Key). If the message does not contain an
       8689 APS Command of 0x05 (Transport Key), then the message shall be dropped and no further processing
       8690 done. All other messages where the security sub-field is set to 0 shall be dropped and no further pro-
       8691 cessing shall be done.2
    */

    if (ZB_NIB_SECURITY_LEVEL() != 0U)
    {
        if (ZB_JOINED()
                && ZG->aps.authenticated
                && frame_type == ZB_NWK_FRAME_TYPE_COMMAND
                && command_id == ZB_NWK_CMD_REJOIN_REQUEST
                && nwk_hdr->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
        {
            pass = ZB_TRUE;
        }
        else if (!ZB_JOINED()
                 && !ZG->aps.authenticated
                 && frame_type == ZB_NWK_FRAME_TYPE_COMMAND
                 && command_id == ZB_NWK_CMD_REJOIN_RESPONSE)
        {
            pass = ZB_TRUE;
        }
        else if (ZB_JOINED()
                 && !ZG->aps.authenticated
                 && frame_type == ZB_NWK_FRAME_TYPE_DATA
                 /*cstat !MISRAC2012-Rule-13.5 */
                 /* After some investigation, the following violation of Rule 13.5 seems to be a false
                  * positive. There are no side effects to 'nwk_is_frame_unsecured_aps_tk()', which is a
                  * static function. There are also no side effects to 'zb_buf_data()'. This violation
                  * seems to be caused by the fact that 'zb_buf_data()' is an external function, which
                  * cannot be analyzed by C-STAT. */
                 && nwk_is_frame_unsecured_aps_tk(zb_buf_data(param, hdr_size)))
        {
            pass = ZB_TRUE;
        }
        else
        {
            pass = ZB_FALSE;
        }
    }
    else
    {
        pass = ZB_TRUE;
    }

    TRACE_MSG(TRACE_NWK3, "<< nwk_filter_unsecured_frame pass %hd", (FMT__H, pass));

    return pass;
}


static zb_bool_t zb_nwk_check_packet_is_addresses_valid(zb_uint8_t param)
{
    zb_apsde_data_ind_params_t *mac_addrs;
    zb_nwk_hdr_t *nwk_hdr;
    zb_uint8_t nwk_hdr_size;
    zb_bool_t ret = ZB_TRUE;

    /* get NWK header */
    nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);

    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SECURITY(nwk_hdr->frame_control)) &&
            ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control)))
    {
        /* continue to parse the packet */
        mac_addrs = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);
        nwk_hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);

        if (nwk_hdr->src_addr == mac_addrs->mac_src_addr)
        {
            /* A device sends packets directly. IEEE address from Security header should be
             * equal to IEEE address from NWK header.
             */
            if (!ZB_IEEE_ADDR_CMP((zb_uint8_t *)nwk_hdr + nwk_hdr_size - (zb_uint8_t)sizeof(zb_nwk_aux_frame_hdr_t) + (zb_uint8_t)ZB_OFFSETOF(zb_nwk_aux_frame_hdr_t, source_address),
                                  ZB_U2B(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control)) ?
                                  nwk_hdr->src_ieee_addr : nwk_hdr->dst_ieee_addr))
            {
                ret = ZB_FALSE;
            }
        }
        else
        {
            /* A device sends packets via another router. IEEE address from Security header
             * should NOT be equal to IEEE address from NWK header.
             */
            if (ZB_IEEE_ADDR_CMP((zb_uint8_t *)nwk_hdr + nwk_hdr_size - (zb_uint8_t)sizeof(zb_nwk_aux_frame_hdr_t) + (zb_uint8_t)ZB_OFFSETOF(zb_nwk_aux_frame_hdr_t, source_address),
                                 ZB_U2B(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control)) ?
                                 nwk_hdr->src_ieee_addr : nwk_hdr->dst_ieee_addr))
            {
                ret = ZB_FALSE;
            }
        }
    }

    return ret;
}


void zb_mcps_data_indication_cont(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_uint8_t frame_type = ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control);
    zb_uint8_t command_id = 0;
    zb_apsde_data_ind_params_t mac_addrs;
    zb_bool_t indicate = ZB_FALSE;
    zb_neighbor_tbl_ent_t *nbt = NULL;
#ifdef ZB_ROUTER_ROLE
    zb_bool_t retransmit = ZB_FALSE;
#endif
    zb_bool_t send_leave_req = ZB_FALSE; /* CCB2255 */
    zb_bool_t is_addresses_valid;

    TRACE_MSG(TRACE_NWK2, "> zb_mcps_data_indication_cont param %hd", (FMT__H, param));

    ZB_MEMCPY(&mac_addrs, ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t), sizeof(mac_addrs));

    /* TODO: if frame is from non-joined device, drop it (how to check it??) */

    /* addr is in little endian. Can't convert it before unsecure: can't
     * change nwk header. */

    ZB_LETOH16_ONPLACE(nwk_hdr->dst_addr);
    ZB_LETOH16_ONPLACE(nwk_hdr->src_addr);

    /* check frame consistency */
    TRACE_MSG(TRACE_NWK3, "hdr_size %d radius %hd frame type %hd",
              (FMT__D_H_H, ZB_NWK_HDR_SIZE(nwk_hdr), nwk_hdr->radius, frame_type));
    if ( (nwk_hdr->radius == 0U)
            || (frame_type != ZB_NWK_FRAME_TYPE_COMMAND && frame_type != ZB_NWK_FRAME_TYPE_DATA)
            /* NWK source can not be broadcast */
            || ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->src_addr)
       )
    {
        TRACE_MSG(TRACE_NWK1, "###drop bad packet", (FMT__0));
        zb_buf_free(param);
        goto done;
    }
    ZB_ZDO_UPDATE_TC_CONNECTION(nwk_hdr->src_addr);
    TRACE_MSG(TRACE_NWK1, "dst addr mac 0x%x nwk 0x%x, src addr mac 0x%x nwk 0x%x",
              (FMT__D_D_D_D, mac_addrs.mac_dst_addr, nwk_hdr->dst_addr,
               mac_addrs.mac_src_addr, nwk_hdr->src_addr));

#if defined(ZB_LIMIT_VISIBILITY) && defined(ZB_CERTIFICATION_HACKS)
    /* Extend MAC visibility control for ZC: check that frame has source ieee address and then
     * check that this address is visible for us (filter ZC from other networks by MAC address).
     */
    if (nwk_hdr->src_addr == 0x0000U && ZB_CERT_HACKS().check_zc_long_addr_is_visible)
    {
        zb_uint8_t *zc_ieee_addr_ptr = NULL;

        if (ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control)
                && ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control))
        {
            /* Actually we don't convert 64-bit addresses, so no
             * need to use here ZB_LETOH64. So, no need to copy address. You can set pointer
             * to the address. Check for NULL pointer unstead of has_src_ieee flag. */
            zc_ieee_addr_ptr = (zb_uint8_t *) nwk_hdr->src_ieee_addr;
        }
        else if (ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control))
        {
            /* ZBOSS hack: If frame has only source long address then ieee address is
             * stored into dst_ieee_addr field. */
            zc_ieee_addr_ptr = (zb_uint8_t *) nwk_hdr->dst_ieee_addr;
        }

        if ((zc_ieee_addr_ptr != NULL) && !zb_mac_is_long_addr_visible(zc_ieee_addr_ptr))
        {
            TRACE_MSG(TRACE_NWK1, "##Frame is filtered by NWK: source = " TRACE_FORMAT_64,
                      (FMT__A, TRACE_ARG_64(zc_ieee_addr_ptr)));
            zb_buf_free(param);
            goto done;
        }
    }
#endif

    /*
     * Processing of a broadcast with a NWK source of the local device shall only be done when the
     * device has been powered up and operating on the network for nwkNetworkBroadcastDeliveryTime.
     * This prevents broadcasts from being processed that might have recently originated from
     * the device after a reset. (CCB2033)
     */
    if (ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr)
            && nwk_hdr->src_addr == ZB_PIBCACHE_NETWORK_ADDRESS()
            && !ZG->nwk.handle.nwk_broadcast_delivery_time_passed)
    {
        TRACE_MSG(TRACE_NWK2, "temporarily skipping broadcasts originating from the device %d", (FMT__D, !ZG->nwk.handle.nwk_broadcast_delivery_time_passed));
        zb_buf_free(param);
        goto done;
    }

#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
    TRACE_MSG(TRACE_NWK2, "pckt multicast flag=%hd", (FMT__H,
              ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control)) );
#endif

    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_END_DEVICE_INITIATOR(nwk_hdr->frame_control)))
    {
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            if (zb_nwk_neighbor_get_by_short(nwk_hdr->src_addr, &nbt) != RET_OK
                    || nbt == NULL
                    || nbt->device_type != ZB_NWK_DEVICE_TYPE_ED)
            {
                /* We've recieved a frame with ed initiator=TRUE from a device that is not our child */

                /* The routing device shall issue a NWK Leave command to the sender
                   with the Rejoin pa-rameter set to 1 and the RemoveChildren parameter set to 0.

                   r22 spec, 3.6.2.2  / CCB 2255
                */
                send_leave_req = ZB_TRUE;
            }
            else if (nbt->device_type != ZB_NWK_DEVICE_TYPE_ED)
            {
                TRACE_MSG(TRACE_NWK2, "drop end device initiated frame", (FMT__0));
                zb_buf_free(param);
                goto done;
            }
            else /* for debug purposes */
            {
                TRACE_MSG(TRACE_NWK2, "frame with ed initiator from %d with relationship %d", (FMT__D_D,
                          nwk_hdr->src_addr, nbt->relationship));
            }
        }
        else
#endif
        {
            TRACE_MSG(TRACE_NWK2, "end device does not handle end device initiated frames", (FMT__0));
            zb_buf_free(param);
            goto done;
        }
    }

    /* Special check for devices that operates on the SiLabs ZB stack (e.g. Centralite SP, IKEA bulb, etc.).
     * These devices respond to commands from a coordinator which addressed to their ED child by themselves
     * instead of retransmitting these commands to a child. Moreover, in such case these devices insert
     * in APS ACK frame the short address from their ED child, but IEEE address - its own (in NWK part).
     * After that, we had a problem with the address map table. So, we need to check such invalid frames before
     * the address map table will be updated.
     */
    is_addresses_valid = zb_nwk_check_packet_is_addresses_valid(param);

#ifndef ZB_RAF_MOVE_NWK_TEST_ADDRESS
    if (is_addresses_valid)
    {
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            /* FIXME: must not check for address conflict before successfully
              * decrypting the frame! Else an attacker can cause address conflich
              * without even knowing NWK key. */
            if (zb_nwk_test_addresses(param) != RET_OK)
            {
                TRACE_MSG(TRACE_INFO1, "Address conflict is detected for address 0x%x",
                          (FMT__H, mac_addrs.mac_src_addr));
                zb_buf_free(param);
                goto done;
            }
        }
#endif
    }
    else
    {
        /* Pass a packet up without updating the address map table and NBT */
        TRACE_MSG(TRACE_NWK2, "Received a packet with wrong short/long addresses pair, param %hd", (FMT__H, param));
    }
#endif

    /* DD: place checks for broadcast and not for us packets before decryption.
       Duplicates should be filtered before unsecuring a packet.
       Rationale: if ED receives broadcasted Switch Key command from non-parent device, switches the network key and after that receives the same rebroadcasted Switch Key command from its parent, ED will initiate a rejoin if this rebroadcasted by parent command isn't filtered as a duplicate.
    */
    /* See 3.6.5 Skip already processed broadcast packets and not for us packets */
    if (ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr)
#ifdef ZB_ROUTER_ROLE
#ifndef ZB_NO_NWK_MULTICAST
            || (ZB_IS_DEVICE_ZC_OR_ZR()
                && ZB_NWK_IS_MULTICAST_MEMBER_MODE(mac_addrs.mac_dst_addr, nwk_hdr->frame_control)
               )
#endif
            /* Do not relay route requests as usual frames */
#endif
       )
    {
        if (!(nwk_hdr->dst_addr == ZB_NWK_BROADCAST_ALL_DEVICES
                /*cstat -MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of Rule 13.5 seems to be a false
                 * positive. There are no side effects to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()' or
                 * 'ZB_IS_DEVICE_ZC_OR_ZR()'. This violation seems to be caused by the fact that
                 * these functions are external functions, which cannot be analyzed by C-STAT. */
                || (nwk_hdr->dst_addr == ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE
                    && ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
#ifdef ZB_ROUTER_ROLE
                || (ZB_IS_DEVICE_ZC_OR_ZR()
                    /*cstat +MISRAC2012-Rule-13.5 */
                    && (nwk_hdr->dst_addr == ZB_NWK_BROADCAST_ROUTER_COORDINATOR
                        || nwk_hdr->dst_addr == ZB_NWK_BROADCAST_LOW_POWER_ROUTER
#ifndef ZB_NO_NWK_MULTICAST
                        || ZB_NWK_IS_MULTICAST_MEMBER_MODE(mac_addrs.mac_dst_addr, nwk_hdr->frame_control)
#endif
                       ))
#endif
             ))
        {
            TRACE_MSG(TRACE_NWK1, "unsupported broadc pkt 0x%x - drop", (FMT__D, nwk_hdr->dst_addr));
            zb_buf_free(param);
            goto done;
        }

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
        /* Don't do passive ack and dup rejection for RREQ here */
        if (frame_type != ZB_NWK_FRAME_TYPE_COMMAND)
        {
#ifdef ZB_ROUTER_ROLE
            /* the following routinges could be united somehow */
#ifdef ZB_CERTIFICATION_HACKS
            if (!ZB_CERT_HACKS().nwk_disable_passive_acks)
#endif
            {
                if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
                {
                    zb_nwk_broadcasting_mark_passive_ack(mac_addrs.mac_src_addr, nwk_hdr);
                }
            }
#endif

            /*
              Need to check for dups for rx-on-when-idle devices, even ZED.
              But can skip it for rx-off-when-idle: all dups are already rejected at
              our parent.
            */
            if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
            {
                TRACE_MSG(TRACE_NWK3, "rx-on-when-idle", (FMT__0));
                if (zb_nwk_broadcasting_find_btr(nwk_hdr) == NULL)
                {
                    TRACE_MSG(TRACE_NWK3, "not a dup", (FMT__0));
                    /* packet isn't a dup, creating btt entry */
                    if (!zb_nwk_broadcasting_add_btr(nwk_hdr))
                    {
                        /* can't create entry, drop packet */
                        TRACE_MSG(TRACE_NWK1, "btt tbl is full - drop", (FMT__0));
                        zb_buf_free(param);
                        goto done;
                    }
                }
                else
                {
                    TRACE_MSG(TRACE_NWK1, "duplicate data frame - drop", (FMT__0));
                    zb_buf_free(param);
                    goto done;
                }
            } /* if rx-on-when-idle */
            else
            {
                TRACE_MSG(TRACE_NWK3, "rx-off-when-idle - do not detect dups", (FMT__0));
            }
        } /* if this broadcast must be checked via RTT for dups and passive acks */
#endif
    } /* if nwk_hdr->dst_addr or mac_dst_addr is BC */

    /* decrypt now */
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SECURITY(nwk_hdr->frame_control)))
    {
        if (!ZG->aps.authenticated)
        {
            TRACE_MSG(TRACE_SECUR3, "Not authenticated - drop secured frame", (FMT__0));
            zb_buf_free(param);
            goto done;
        }

        /* revert address back to keep header unchanged before decrypt */
        ZB_LETOH16_ONPLACE(nwk_hdr->dst_addr);
        ZB_LETOH16_ONPLACE(nwk_hdr->src_addr);

        if (zb_nwk_unsecure_frame(param) != RET_OK)
        {
            /* Do not free the buffer: it used to send NWK status indication in zb_nwk_unsecure_frame */
            TRACE_MSG(TRACE_NWK1, "pkt unsecure failed - drop", (FMT__0));
            goto done;
        }
        else
        {
            /* unsecure frame does left alloc and can move data in the
              * frame. Reassign nwk header. */
            nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
            ZB_LETOH16_ONPLACE(nwk_hdr->dst_addr);
            ZB_LETOH16_ONPLACE(nwk_hdr->src_addr);

            /* Set flags to secure this frame in case in will be forwarded */
            zb_buf_flags_or(param, ZB_BUF_SECUR_NWK_ENCR | ZB_BUF_USE_SAME_KEY);

            /* Check: can parameter (not just mac_addrs) be
                overwritten during unsecure?
                Probably, yes. So, must recover it there. */
            /* VP: add copy addrs in buffer tail */
            ZB_MEMCPY(ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t), &mac_addrs, sizeof(mac_addrs));
        }
    }
    else                          /* Not secured */
    {
        if (nwk_filter_unsecured_frame(param))
        {
            /* Ok to accept not secured. Remember that device below. */
            /* (void)nwk_add_into_addr_and_nbt(nwk_hdr, mac_addrs.mac_src_addr); */
        }
        else
        {
            TRACE_MSG(TRACE_NWK1, "Not secured frame - drop", (FMT__0));
            zb_buf_free(param);
            goto done;
        }
    }

#ifdef ZB_RAF_MOVE_NWK_TEST_ADDRESS
    if (is_addresses_valid)
    {
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            if (zb_nwk_test_addresses(param) != RET_OK)
            {
                TRACE_MSG(TRACE_INFO1, "Address conflict is detected for address 0x%x",
                          (FMT__H, mac_addrs.mac_src_addr));
                zb_buf_free(param);
                goto done;
            }
        }
#endif
    }
    else
    {
        /* Pass a packet up without updating the address map table and NBT */
        TRACE_MSG(TRACE_NWK2, "Received a packet with wrong short/long addresses pair, param %hd", (FMT__H, param));
    }
#endif

    if (is_addresses_valid)
    {
        /* If checked and unsecured successfully, remember that device in address
         * and nbt (if not done so already). */
        nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
        nwk_add_into_addr_and_nbt(nwk_hdr, &mac_addrs);
    }
    /*cstat -MISRAC2012-Rule-13.5 */
    if (send_leave_req == ZB_TRUE
            && !(frame_type == ZB_NWK_FRAME_TYPE_COMMAND
                 /* After some investigation, the following violation of MISRA Rule 13.5 seems to be a
                  * false positive. The only way this function could have side effects is if the functions
                  * 'zb_buf_begin()' or 'zb_get_nwk_header_size()' called inside
                  * 'zb_nwk_cmd_frame_get_cmd_id()' (a static function) had side effects, which they
                  * don't. */
                 && zb_nwk_cmd_frame_get_cmd_id(param) == ZB_NWK_CMD_LEAVE))
        /*cstat +MISRAC2012-Rule-13.5 */
    {
        TRACE_MSG(TRACE_NWK1, "leave unknown ED device", (FMT__0));
        ZB_SCHEDULE_CALLBACK2(zb_nwk_send_direct_leave_req, param, nwk_hdr->src_addr);
        goto done;
    }

    /* decrement radius after unsecure */
    nwk_hdr->radius--;
    TRACE_MSG(TRACE_NWK3, "new radius %hd, frame type %hd",
              (FMT__H_H, nwk_hdr->radius, ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control)));

    TRACE_MSG(TRACE_ATM1, "Z< new radius %hd, frame type %hd, cmd id %hd",
              (FMT__H_H_H, nwk_hdr->radius, ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control), zb_nwk_cmd_frame_get_cmd_id(param)));

    /* Unconditional. If frame is not a command, dust here - it is ok. Get
     * command id after unsecure. */
    if ( frame_type == ZB_NWK_FRAME_TYPE_COMMAND)
    {
        command_id = zb_nwk_cmd_frame_get_cmd_id(param);
        /*Drop unknown commands to prevent ED-Scan */
        if ((command_id < ZB_NWK_CMD_ROUTE_REQUEST) || (command_id > ZB_NWK_CMD_LINK_POWER_DELTA))
        {
            /* see 3.6.13 of r22 spec */
            send_unknown_command_status(param, nwk_hdr, command_id);
            goto done;
        }

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
        /* do not check RREQ for dups, it will be checked in routing mechanics */
        if (command_id != ZB_NWK_CMD_ROUTE_REQUEST
                && ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr)
                /*cstat -MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of MISRA Rule 13.5 seems to be a false
                 * positive. There are no side effects to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()' or
                 * 'zb_nwk_broadcasting_find_btr()'. This violations seem to be caused by the fact that
                 * these functions are external functions, which cannot be analyzed by C-STAT. */
                && (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
                    || zb_nwk_broadcasting_find_btr(nwk_hdr) != NULL))
            /*cstat +MISRAC2012-Rule-13.5 */
        {
            TRACE_MSG(TRACE_NWK1, "duplicate cmd frame - drop", (FMT__0));
            zb_buf_free(param);
            goto done;
        }
#endif
    }

    /* Decide what we should do with this frame */
#ifdef ZB_ROUTER_ROLE

    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        if ( ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr) )
        {
            indicate = ZB_TRUE;
            if ( nwk_hdr->radius != 0U                                          /* Radius is enough to retransmit */
                    && (frame_type == ZB_NWK_FRAME_TYPE_DATA                       /* All brd data frames should be retranmited */
                        || (frame_type == ZB_NWK_FRAME_TYPE_COMMAND                /* Some type of commands */
                            && (command_id == ZB_NWK_CMD_NETWORK_REPORT
                                /* || command_id == ZB_NWK_CMD_NETWORK_STATUS */   /* WARNING: NWK status will be forwarded after handling */
                                || command_id == ZB_NWK_CMD_NETWORK_UPDATE))))
            {
                /* Note: we retransmit once even if this is broadcast from the only
                 * device from our neighbours. Can skip retransmit by checking for MAC
                 * broadcast and counting # of devices for passive ack here,
                 * but... maybe, it is still better to retransmit at least once? */
                retransmit = ZB_TRUE;
            }
        } /* if ( ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr)) */
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
        else if ( ZB_MAC_IS_ADDRESS_BROADCAST(mac_addrs.mac_dst_addr))
        {
            if ( (ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control))
                    && (GET_NWK_MCF(nwk_hdr).multicast_mode == ZB_NWK_MULTICAST_MODE_MEMBER)
                    && (frame_type == ZB_NWK_FRAME_TYPE_DATA)
                    && (nwk_hdr->radius)
               )
            {
                TRACE_MSG(TRACE_NWK3, "its mmode mc bcast pckt", (FMT__0));

                /* check belonging to a group and change current non-member hops counter */
                if (zb_aps_is_in_group(nwk_hdr->dst_addr))
                {
                    TRACE_MSG(TRACE_NWK3, "its our group", (FMT__0));
                    indicate = ZB_TRUE;
                    if (GET_NWK_MCF(nwk_hdr).nonmember_radius != ZB_APS_AIB_INFINITY_NONMEMBER_RADIUS)
                    {
                        GET_NWK_MCF(nwk_hdr).nonmember_radius = GET_NWK_MCF(nwk_hdr).max_nonmember_radius;
                    }
                }
                else
                {
                    TRACE_MSG(TRACE_NWK3, "its not our group", (FMT__0));
                    indicate = ZB_FALSE;
                    if (GET_NWK_MCF(nwk_hdr).nonmember_radius != ZB_APS_AIB_INFINITY_NONMEMBER_RADIUS)
                    {
                        --(GET_NWK_MCF(nwk_hdr).nonmember_radius);
                    }
                    if (!GET_NWK_MCF(nwk_hdr).nonmember_radius)
                    {
                        TRACE_MSG(TRACE_NWK1, "non-member hop counter has expired - drop", (FMT__0));
                        goto done;
                    }
                }
                retransmit = ZB_TRUE;
            }
        } /* if ( ZB_NWK_IS_ADDRESS_BROADCAST(mac_dst_addr) ) */
#endif  /* if multicast */
        /* Its own destination address, not multicast frame */
        else if ( ( nwk_hdr->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS() )
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
                  && ( !ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control ) )
#endif
                )
        {
            TRACE_MSG(TRACE_NWK1, "pckt to its own addr, not mc", (FMT__0));
            indicate = ZB_TRUE;
        }
        /* AD: Route record processing */
#ifdef  ZB_PRO_STACK
        else if (zb_nwk_cmd_frame_get_cmd_id(param) == ZB_NWK_CMD_ROUTE_RECORD)
        {
            indicate = ZB_TRUE;
            /* AD: we should not use common retransmitting, because command frame should be modified */
            retransmit = ZB_FALSE;
        }
#endif
        /* Others situations (such as non-member multicast frame, etc)*/
        else if ( nwk_hdr->radius != 0U )
        {
            retransmit = ZB_TRUE;

            /* CCB2673: R22 Errata:
             * On receipt of a network status command frame by a router
             * that is the parent of an end device that is the intended destination,
             * where the status code field of the command frame payload has a value
             * of 0x00, 0x01 or 0x02 indicating a link failure, the NWK layer will
             * remove the routing table entry corresponding to the value of the
             * destination address field of the command frame payload, if one exists.
             * It will then relay the frame as usual to the end device. */
            if (zb_nwk_neighbor_get_by_short(nwk_hdr->dst_addr, &nbt) == RET_OK
                    && nbt != NULL
                    && nbt->device_type == ZB_NWK_DEVICE_TYPE_ED
                    /*cstat !MISRAC2012-Rule-13.5 */
                    /* After some investigation, the following violations of Rule 13.5 and 13.6 seem to be
                     * false positives. There are no side effect to 'zb_nwk_cmd_frame_get_cmd_id()'. This
                     * violation seems to be caused by the fact that 'zb_nwk_cmd_frame_get_cmd_id()' is an
                     * external function, which cannot be analyzed by C-STAT. */
                    && zb_nwk_cmd_frame_get_cmd_id(param) == ZB_NWK_CMD_NETWORK_STATUS)
            {
                indicate = ZB_TRUE;
            }
            /* Get multicast frame in non-member unicast mode,
               we need check belonging to a group, check the status of non-members counters,
               and if necessary retransmit it
            */
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
            if (ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control ))
            {
                TRACE_MSG(TRACE_NWK3, "its nmmode mc unicast pckt", (FMT__0));
                /* Ok - set member-mode increase counter to max value, broadcast packet */
                if (zb_aps_is_in_group(nwk_hdr->dst_addr))
                {
                    indicate = ZB_TRUE;
                    /* Rebroadcast this frame by group address address */
                    GET_NWK_MCF(nwk_hdr).multicast_mode = ZB_NWK_MULTICAST_MODE_MEMBER;
                    if (GET_NWK_MCF(nwk_hdr).nonmember_radius != ZB_APS_AIB_INFINITY_NONMEMBER_RADIUS)
                    {
                        GET_NWK_MCF(nwk_hdr).nonmember_radius = GET_NWK_MCF(nwk_hdr).max_nonmember_radius;
                    }
                    TRACE_MSG(TRACE_NWK3, "set mmode, inc rad to max -> bcast retrasm", (FMT__0));
                }
                /* Receiver non group member - check counter - retransmit or drop frame */
                else
                {
                    indicate = ZB_FALSE;
                    GET_NWK_MCF(nwk_hdr).multicast_mode = ZB_NWK_MULTICAST_MODE_NONMEMBER;
                    if (GET_NWK_MCF(nwk_hdr).nonmember_radius != ZB_APS_AIB_INFINITY_NONMEMBER_RADIUS)
                    {
                        --(GET_NWK_MCF(nwk_hdr).nonmember_radius);
                    }
                    TRACE_MSG(TRACE_NWK3, "nmmode, decr rad", (FMT__0));
                    if (!GET_NWK_MCF(nwk_hdr).nonmember_radius)
                    {
                        TRACE_MSG(TRACE_NWK1, "nmode hop cnt has expired -> drop", (FMT__0));
                        goto done;
                    }
                }
            } /* if (ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control )) */
#endif  /* multicast */
        }
        else
        {
            /* MISRA rule 15.7 requires empty 'else' branch. */
        }

#ifndef ZB_LITE_NO_SOURCE_ROUTING
        if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(nwk_hdr->frame_control)))
        {
            /* When relaying a source routed data frame, the NWK layer of a device shall also
               examine the routing table entry corresponding to the source address of the frame.
               If the no route cache field of the routing table entry has a value of FALSE,
               then the route record required field of the routing table entry shall be set to FALSE.
             */
            zb_nwk_source_routing_clear_rrec_req(nwk_hdr->src_addr);
        }
#endif /* ZB_LITE_NO_SOURCE_ROUTING */
    } /* device type is ZB_NWK_DEVICE_TYPE_COORDINATOR or ZB_NWK_DEVICE_TYPE_ROUTER */
    else
#endif  /* ZB_ROUTER_ROLE */
    {
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
        if ( ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_hdr->frame_control) )
        {
            if (zb_aps_is_in_group(nwk_hdr->dst_addr))
            {
                TRACE_MSG(TRACE_NWK3, "its end device-mc pckt", (FMT__0));
                indicate = ZB_TRUE;
            }
        }
        else
#endif  /* multicast */
            if ( nwk_hdr->dst_addr == ZB_PIBCACHE_NETWORK_ADDRESS()
                    || ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr) )
            {
                indicate = ZB_TRUE;
            }
    }

    /* update neighbor lqi and rssi */
    if ( zb_nwk_neighbor_get_by_short(mac_addrs.mac_src_addr, &nbt) == RET_OK )
    {
#ifdef ZB_CERTIFICATION_HACKS
        /* For TP_PRO_BV_04 need not change in/out costs by LQI */
        if (!ZB_CERT_HACKS().disable_in_out_cost_updating)
#endif
        {
            nbt->lqi = mac_addrs.lqi;
#ifndef ZB_LITE_DONT_STORE_RSSI
            nbt->rssi = mac_addrs.rssi;
#ifdef ZB_ROUTER_ROLE
            zb_nwk_neighbour_rssi_store(nbt, mac_addrs.rssi);
#endif
#endif
            TRACE_MSG(TRACE_NWK3, "0x%x : update lqi to %hd, rssi to %hd", (FMT__D_H_H, mac_addrs.mac_src_addr, mac_addrs.lqi, mac_addrs.rssi));
        }

#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER)
        if (nbt->relationship == ZB_NWK_RELATIONSHIP_PARENT)
        {
            zb_zcl_wwah_bad_parent_recovery_signal(
                ZB_ZCL_WWAH_BAD_PARENT_RECOVERY_RSSI_IS_GOOD(nbt->rssi) ?
                ZB_ZCL_WWAH_BAD_PARENT_RECOVERY_RSSI_WITH_PARENT_OK :
                ZB_ZCL_WWAH_BAD_PARENT_RECOVERY_RSSI_WITH_PARENT_BAD);
        }
#endif
    }

    /* After long power down ZB network may be changed. Previos children
     * may rejoin the PAN to another routers, but still send packets to
     * us. In this case we should remove the device from the neighbor
     * table and update route the sender. Decision we make is based on
     * comparing MAC and NWK short addresses. If they are not equal and
     * the device is our child, then it should be deleted from neighbor
     * table */
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC_OR_ZR()
            && is_addresses_valid)
    {
        zb_nwk_check_child_presense(param);
    }
#endif

#ifdef ZB_ROUTER_ROLE
    TRACE_MSG(TRACE_NWK1, "zb_mcps_data_indication_cont %hd: retransmit = %hd; indicate = %hd", (FMT__H_H_H, param, retransmit, indicate));

    if (retransmit
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'zb_buf_memory_low()'. This violation seems to be
             * caused by the fact that 'zb_buf_memory_low()' is an external function, which cannot be
             * analyzed by C-STAT. */
            && zb_buf_memory_low())
    {
        retransmit = ZB_FALSE;
        TRACE_MSG(TRACE_ERROR, "Oops - memory low. Do not forward this buffer %hd - maybe indicate",
                  (FMT__H, param));
    }

#ifdef ZB_CERTIFICATION_HACKS

    if (ZB_CERT_HACKS().disable_frame_retransmission)
    {
        retransmit = ZB_FALSE;
    }

    if (ZB_CERT_HACKS().disable_frame_retransmission_countdown != 0)
    {
        if (ZB_CERT_HACKS().disable_frame_retransmission_countdown == 1)
        {
            ZB_CERT_HACKS().disable_frame_retransmission_countdown = 0;
            ZB_CERT_HACKS().disable_frame_retransmission = 1;
        }
        else
        {
            ZB_CERT_HACKS().disable_frame_retransmission_countdown--;
        }
    }

    if (ZB_CERT_HACKS().force_frame_indication)
    {
        indicate = ZB_TRUE;
    }

#endif

    if (retransmit && indicate)
    {
        /* schedule buff allocation  */
        if (zb_buf_get_out_delayed_ext(nwk_allocate_n_forward, param, 0) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops - out of memory. Do not retransmit this buffer %hd - just indicate", (FMT__H, param));
            ZB_SCHEDULE_CALLBACK_PRIOR(nwk_frame_indication, param);
        }
    }
    else if ( retransmit )
    {
        zb_apsde_data_ind_params_t *pkt_params = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);

        /* this packet is not for us, but try to forward it */
        pkt_params->handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
        ZB_ASSERT(param);
        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
        /* can handle next input packet */
        /* zb_nwk_unlock_in(param); */
    }
    else
#endif  /* ZB_ROUTER_ROLE */
        if ( indicate )
        {
            TRACE_MSG(TRACE_NWK3, "indicating param %hd", (FMT__H, param));
            ZB_SCHEDULE_CALLBACK_PRIOR(nwk_frame_indication, param);
        }
        else
        {
            /* this frame is not for us, drop it */
            TRACE_MSG(TRACE_NWK1, "drop %hd", (FMT__H, param));
            zb_buf_free(param);
        }

done:
    if (!indicate)
    {
        zb_zdo_pim_continue_polling_for_pkt();
    }
    TRACE_MSG(TRACE_NWK2, "< zb_mcps_data_indication_cont", (FMT__0));
}


/**
   Remember as much as we can about that packet: address translation, neighbor
 */
static void nwk_add_into_addr_and_nbt(zb_nwk_hdr_t *nwk_hdr, zb_apsde_data_ind_params_t *ind)
{
    zb_address_ieee_ref_t addr_ref;
    zb_ret_t ret;

    ZVUNUSED(ind);
    if (!ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->src_addr))
    {
        /* That logic is initially implemented to deal
          with NWK encrypted Device_annce when we have no both addresses yet.
          Now we are collecting long/short/neighbor information from any nwk frames passing.
        */
        if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(nwk_hdr->frame_control)))
        {
            zb_ieee_addr_t *ieee_addr = zb_nwk_get_src_long_from_hdr(nwk_hdr);

            /* have long & short address - can update/add translation table */
            TRACE_MSG(TRACE_NWK1, "%d get/create address by short 0x%x and long " TRACE_FORMAT_64,
                      (FMT__D_D_A, ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control),
                       nwk_hdr->src_addr,
                       TRACE_ARG_64(*ieee_addr)));

            ret = zb_address_update(*ieee_addr, nwk_hdr->src_addr, ZB_FALSE, &addr_ref);
        }
        else
        {
            /* FIXME: is it always necesary? */
            TRACE_MSG(TRACE_NWK1, "get/create address by short 0x%x only ", (FMT__D, nwk_hdr->src_addr));
            ret = zb_address_by_short(nwk_hdr->src_addr, ZB_TRUE, ZB_FALSE, &addr_ref);
        }

        /* remember long and short address of the last hop */
        if (ZB_NWK_FRAMECTL_GET_SECURITY(nwk_hdr->frame_control) != 0U)
        {
            zb_address_ieee_ref_t addr_ieee_ref;
            /*cstat !MISRAC2012-Rule-11.3 */
            /** @mdr{00002,23} */
            zb_nwk_aux_frame_hdr_t *aux = (zb_nwk_aux_frame_hdr_t *)((zb_uint8_t *)nwk_hdr + ZB_NWK_HDR_SIZE(nwk_hdr) - (zb_uint8_t)sizeof(zb_nwk_aux_frame_hdr_t));
            (void)zb_address_update(aux->source_address, ind->mac_src_addr, ZB_FALSE, &addr_ieee_ref);
        }

        /*
          Create an entry in the neighbor only if this is direct transmit.
          If we just received a frame from the device, it is our neighbor.
          And note that ZED can have >1 device in the neighbor if it is rx-on-wnen-idle.
        */
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
        if (ret == RET_OK
                /*cstat !MISRAC2012-Rule-13.5 */
                /* After some investigation, the following violation of Rule 13.5 seems to be a false
                 * positive. There are no side effects to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()'. This violation
                 * seems to be caused by the fact that this function is an external function, which cannot
                 * be analyzed by C-STAT. */
                && ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
                && nwk_hdr->src_addr == ind->mac_src_addr
                /* Do not add broadcasts with bad rssi to the neighbor table to avoid direct
                 * transmissions to devices that may not receive our packet because of
                 * weak signal strength. Also use transmission via routing table for devices that
                 * starts sending broadcasts with bad rssi.
                 */
                && !(ZB_MAC_IS_ADDRESS_BROADCAST(ind->mac_dst_addr)
                     && nwk_is_lq_bad_for_direct(ind->rssi, ind->lqi)))
        {
            zb_neighbor_tbl_ent_t *nbt;

            TRACE_MSG(TRACE_NWK1, "NWK packet from the neighbr - get/create nbt for 0x%x", (FMT__D, ind->mac_src_addr));
            ret = zb_nwk_neighbor_get(addr_ref, ZB_TRUE, &nbt);
            ZVUNUSED(ret);
        }
#endif
    }
}

zb_bool_t nwk_is_lq_bad_for_direct(zb_int8_t rssi, zb_uint8_t lqi)
{
    ZVUNUSED(lqi);
    return (zb_bool_t)(rssi < ZB_NWK_NEIGHBOR_RSSI_FILTER);
}

void nwk_frame_indication(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_uint8_t frame_type = ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control);
    zb_uint8_t hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);

    TRACE_MSG(TRACE_NWK1, ">> nwk_frame_indication param %hd", (FMT__H, param));

    /* handle data/command frames */
    TRACE_MSG(TRACE_NWK1, "frame type %hd; in q empty %hd input_blocked_by %hd",
              (FMT__H_H_H, frame_type, ZB_RING_BUFFER_IS_EMPTY(ZB_NWK_IN_Q), ZG->nwk.handle.input_blocked_by));

    if (frame_type == ZB_NWK_FRAME_TYPE_DATA)
    {
        if (ZG->nwk.handle.input_blocked_by == 0U)
        {
            ZG->nwk.handle.input_blocked_by = param;
            TRACE_MSG(TRACE_NWK2, "set input_blocked_by to %hd and process it immediately", (FMT__H, param));
            ZB_SCHEDULE_CALLBACK(zb_nlde_data_indication, param);
            param = 0;
        }
        else if (!ZB_BYTE_ARRAY_IS_FULL(ZB_NWK_IN_Q, ZB_NWK_IN_Q_SIZE))
        {
            TRACE_MSG(TRACE_INFO1, "Put %hd into input queue (blocked by %hd)", (FMT__H_H, param, ZG->nwk.handle.input_blocked_by));
            ZB_BYTE_ARRAY_PUT(ZB_NWK_IN_Q, param, ZB_NWK_IN_Q_SIZE);
            param = 0;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "No free slots in the input nwk queue - drop pkt %hd", (FMT__H, param));
            zb_zdo_pim_continue_polling_for_pkt();
        }
    }
    else if (frame_type == ZB_NWK_FRAME_TYPE_COMMAND)
    {
        zb_bool_t is_unknown_command = ZB_FALSE;
        zb_uint8_t command_id = zb_nwk_cmd_frame_get_cmd_id(param);

        TRACE_MSG(TRACE_NWK3, "command_id %hd devt %hd", (FMT__H_H, command_id, ZB_NIB().device_type));
        ZB_TH_PUSH_PACKET(ZB_TH_NL_COMMAND, ZB_TH_PRIMITIVE_INDICATION, param);
        if ( (command_id == ZB_NWK_CMD_LEAVE))
        {
            /* leave payload is one byte */
            zb_uint8_t *leave_pl = (zb_uint8_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

            TRACE_MSG(TRACE_NWK3, "got leave cmd", (FMT__0));
            zb_nwk_leave_handler(param, nwk_hdr, *leave_pl);
            param = 0;
        }
#ifdef ZB_JOIN_CLIENT
        else if (command_id == ZB_NWK_CMD_REJOIN_RESPONSE)
        {
            TRACE_MSG(TRACE_NWK3, "got rejoin response cmd", (FMT__0));
            /*cstat !MISRAC2012-Rule-14.3_a */
            /** @mdr{00012,16} */
            if (!ZB_IS_DEVICE_ZC())
            {
                zb_nlme_rejoin_response(param);
                param = 0;
            }
        }
#endif
#ifdef ZB_ED_FUNC
        else if (command_id == ZB_NWK_CMD_ED_TIMEOUT_RESPONSE)
        {
            zb_nwk_ed_timeout_response_t *toresp = (zb_nwk_ed_timeout_response_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

            TRACE_MSG(TRACE_NWK3, "Got ED timeout response cmd", (FMT__0));
            if (ZB_IS_DEVICE_ZED())
            {
                nwk_timeout_resp_handler(param, nwk_hdr, toresp);
                param = 0;
            }
        }
#endif
#if defined ZB_MAC_POWER_CONTROL
        else if (command_id == ZB_NWK_CMD_LINK_POWER_DELTA)
        {
            TRACE_MSG(TRACE_NWK3, "got link power delta cmd, current page %hd",
                      (FMT__H, ZB_PIBCACHE_CURRENT_PAGE()));
            if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
            {
                zb_nwk_handle_link_power_delta_command(param, nwk_hdr, hdr_size);
                param = 0;
            }
        }
#endif  /* ZB_MAC_POWER_CONTROL */
#ifndef ZB_LITE_NO_PANID_CONFLICT_DETECTION
        else if (command_id == ZB_NWK_CMD_NETWORK_UPDATE)
        {
            if (ZG->nwk.selector.panid_conflict_network_update_recv != NULL)
            {
                (*ZG->nwk.selector.panid_conflict_network_update_recv)(param);
                param = 0;
            }
            else
            {
                TRACE_MSG(TRACE_NWK3, "Network Update command is received, but panid_conflict_network_update_recv is not set, so skip",
                          (FMT__0));
            }
        }
#endif  /* ZB_LITE_NO_PANID_CONFLICT_DETECTION */
#ifdef ZB_ROUTER_ROLE
        else if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
#ifdef ZB_NWK_MESH_ROUTING
            if (command_id == ZB_NWK_CMD_ROUTE_REQUEST)
            {
                zb_nwk_cmd_rreq_t *rreq = (zb_nwk_cmd_rreq_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

                TRACE_MSG(TRACE_NWK3, "got r req cmd", (FMT__0));
                ZB_NWK_ADDR_TO_LE16(rreq->dest_addr);
                zb_nwk_mesh_rreq_handler(param, nwk_hdr, rreq);
                param = 0;
            }
            else if (command_id == ZB_NWK_CMD_ROUTE_REPLY)
            {
                zb_nwk_cmd_rrep_t *rrep = (zb_nwk_cmd_rrep_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

                TRACE_MSG(TRACE_ATM1, "Z< get route reply command", (FMT__0));
                TRACE_MSG(TRACE_NWK3, "got r repl cmd", (FMT__0));
                zb_nwk_mesh_rrep_handler(param, nwk_hdr, rrep);
                param = 0;
            }
#ifdef ZB_PRO_STACK
#ifndef ZB_LITE_NO_SOURCE_ROUTING
            else if (command_id == ZB_NWK_CMD_ROUTE_RECORD)
            {
                TRACE_MSG(TRACE_NWK3, "got route record cmd", (FMT__0));
                zb_nwk_rrec_handler(param, nwk_hdr, hdr_size);
                param = 0;
            }
#endif
            else if (command_id == ZB_NWK_CMD_LINK_STATUS)
            {
                TRACE_MSG(TRACE_NWK3, "got link status cmd", (FMT__0));
                zb_nwk_receive_link_status_command(param, nwk_hdr, hdr_size);
                param = 0;
            }
#endif  /* PRO */
#endif /* ZB_NWK_MESH_ROUTING */
            else if (command_id == ZB_NWK_CMD_REJOIN_REQUEST)
            {
                TRACE_MSG(TRACE_NWK3, "got rejoin request cmd", (FMT__0));
                //zb_nlme_rejoin_request_handler(param);
                zb_nlme_rejoin_request_pre_handler(param);
                param = 0;
            }
            else if (command_id == ZB_NWK_CMD_ED_TIMEOUT_REQUEST)
            {
                zb_nwk_ed_timeout_request_t *toreq = (zb_nwk_ed_timeout_request_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

                TRACE_MSG(TRACE_NWK3, "got ED timeout request cmd", (FMT__0));
                zb_nwk_ed_timeout_request_handler(param, nwk_hdr, toreq);
                param = 0;
            }
#ifndef ZB_LITE_NO_PANID_CONFLICT_DETECTION
            else if (command_id == ZB_NWK_CMD_NETWORK_REPORT)
            {
                /* Function pointer is 0 if panid conflicts detect/resolve is disabled. */
                if (ZG->nwk.selector.panid_conflict_got_network_report != NULL)
                {
                    (*ZG->nwk.selector.panid_conflict_got_network_report)(param);
                    param = 0;
                }
            }
#endif  /* ZB_LITE_NO_PANID_CONFLICT_DETECTION */
            else if (command_id == ZB_NWK_CMD_NETWORK_STATUS)
            {
                zb_nlme_status_indication_t *cmd = (zb_nlme_status_indication_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

                zb_nwk_network_status_handler(param, nwk_hdr, cmd);
                param = 0;
            }
            else
            {
                is_unknown_command = ZB_TRUE;
            }
        }  /* if !ed */
#endif /* ZB_ROUTER_ROLE */
        else if (command_id == ZB_NWK_CMD_NETWORK_STATUS)
        {
            /* ignoring this command*/
        }
        else
        {
            is_unknown_command = ZB_TRUE;
        }

        /* see 3.6.13 of r22 spec */
        if (is_unknown_command)
        {
            send_unknown_command_status(param, nwk_hdr, command_id);
            param = 0;
        }

        /* It was not APS packet, so lets poll again. */
        zb_zdo_pim_continue_polling_for_pkt();
    } /* if command */

    if (param != 0U)
    {
        TRACE_MSG(TRACE_ERROR, "unknown or disallowed cmd - drop", (FMT__0));
        zb_buf_free(param);
    }
    TRACE_MSG(TRACE_NWK1, "<< nwk_frame_indication", (FMT__0));
}

#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
void zb_nwk_clear_btr_for_address(zb_address_ieee_ref_t addr_ref)
{
    TRACE_MSG(TRACE_NWK2, "zb_nwk_clear_btr_for_address ref %hd", (FMT__H, addr_ref));

    if (ZG->nwk.handle.btt_cnt != 0U)
    {
        zb_ushort_t i;

        for (i = 0; i < ZB_NWK_BTR_TABLE_SIZE; i++)
        {
            if (ZB_U2B(ZG->nwk.handle.btt[i].used) &&
                    ZG->nwk.handle.btt[i].source_addr == addr_ref)
            {
                NWK_ARRAY_PUT_ENT(ZG->nwk.handle.btt, &ZG->nwk.handle.btt[i], ZG->nwk.handle.btt_cnt);
            }
        }
    }
}
#endif  /* !rx-off-when-idle */

/**
   Handle LEAVE command

   Handle LEAVE packet got from the net.

   This routine always free the packet.

   @param packet - incoming packet
   @param nwk_header - already parsed network header
   @param lp - LEAVE command payload (1 byte)
*/
static void zb_nwk_leave_handler(zb_uint8_t param, zb_nwk_hdr_t *nwk_hdr, zb_uint8_t lp)
{
    zb_ret_t    status;
    zb_address_ieee_ref_t addr_ref = 0;
    zb_neighbor_tbl_ent_t *nbt = NULL;
    zb_bool_t send_leave_frame = ZB_FALSE;
#ifdef ZB_RAF_ADDRESS_MAP_LOCK_CNT_UNSYMMETRICAL
    zb_bufid_t buf = 0;
#endif

    TRACE_MSG(TRACE_NWK3, "> zb_nwk_leave_handler from 0x%x to 0x%x request %hd remove_ch %hd rejoin %hd",
              (FMT__D_D_H_H_H, nwk_hdr->src_addr, nwk_hdr->dst_addr,
               (zb_uint8_t)ZB_LEAVE_PL_GET_REQUEST(lp), (zb_uint8_t)ZB_LEAVE_PL_GET_REMOVE_CHILDREN(lp),
               (zb_uint8_t)ZB_LEAVE_PL_GET_REJOIN(lp)));

    /* VP: accept Leave only in Idle state (joined); in some cases device can receive leave in rejoin state;
     * this causes stack to send DEVICE FAILED signal to application while rejoin is ongoing thus
     * allowing application to leave from network. */
    if (ZB_JOINED())
    {
        /* Check the sender of the command */
        status = zb_address_by_short(nwk_hdr->src_addr, ZB_FALSE, ZB_FALSE, &addr_ref);
    }
    else
    {
        TRACE_MSG(TRACE_NWK2, "Got leave when device not joined", (FMT__0));
        status = RET_ERROR;
    }

    if (status == RET_OK)
    {
        if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt) != RET_OK)
        {
#ifdef ZB_ROUTER_ROLE
            if (ZB_LEAVE_PL_GET_REQUEST(lp) == 0U)
            {
                TRACE_MSG(TRACE_NWK2, "delete address", (FMT__0));
                /* addr_ref is always valid, it was fill by zb_address_by_short */
                (void)zb_address_delete(addr_ref);
            }
#endif
            TRACE_MSG(TRACE_NWK2, "incoming LEAVE to 0x%x not from the neighbor", (FMT__D, nwk_hdr->dst_addr));
            status = RET_ERROR;
        }
    }

#ifdef SNCP_MODE
    if (ZB_LEAVE_PL_GET_REJOIN(lp) == 0U)
    {
        if (!ZB_U2B(ZB_NIB_GET_LEAVE_REQ_WITHOUT_REJOIN_ALLOWED()))
        {
            TRACE_MSG(TRACE_NWK2, "LEAVE without rejoin not allowed ", (FMT__0));
            status = RET_ERROR;
        }
    }
#endif

    if (status == RET_OK)
    {
        if (ZB_LEAVE_PL_GET_REQUEST(lp) != 0U)
        {
#ifdef ZB_JOIN_CLIENT
            status = zb_nwk_validate_leave_req(nwk_hdr->src_addr);


            /* EE: all the poem below are old comments related to dr-tar-tc-03e.
               I do not know why it was decided that frame without a dest must be dropped.
               The test says following (1b):
               "NWK Leave command without the destination
            IEEE address:
            THr1 unicasts to the short address of the DUT a
            correctly protected NWK Leave command, with
            the Destination IEEE Address sub-field of the
            NWK Frame Control field set to 0b0 and the
            destination IEEE address field NOT present, with
            Rejoin = 0b0, Request = 0b1, Remove children =
            0b0."
            "NWK Leave command without the destination
            IEEE address:
            THr1 unicasts to the short address of the DUT a
            correctly protected NWK Leave command, with
            the Destination IEEE Address sub-field of the
            NWK Frame Control field set to 0b0 and the
            destination IEEE address field NOT present, with
            Rejoin = 0b0, Request = 0b1, Remove children =
            0b0."

            So LEAVE frame without dest IEEE MUST be accepted.
             */
            /* Drop leave frame if frame has no dest ieee address in nwk header (according to test dr-tar-tc-03e). */

            /* 06/23/2016: [MM]: Is this thing documented in Zigbee PRO specification? The point
             * is when using ED aging, device can be aged and hence deleted from our
             * neighbor table. If non-neighbor device polls, NWK Leave request
             * command frame should be transmitted, but as far as device is already
             * not a neighbor, Destination IEEE address field will not be
             * present.
             *
             * Thus, according to current implementation, ED will not be able to
             * perform a Leave+Rejoin procedure. Refer to Zigbee PRO Test
             * Specification, test case TP_R21_BV_20
             *
             * Made workaround for BDB DR-TAR-TC-03E test: analyze Rejoin bit as
             * well, make decision based on presense of Destination IEEE and Rejoin flag.
             */
            if (status == RET_OK && !ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr))
                /* Remove it forever, but keep here for a while to understand comments above.
                   (ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_hdr->frame_control) || ZB_LEAVE_PL_GET_REJOIN(lp)))*/
            {
                send_leave_frame = ZB_TRUE;
            }
#endif  /* ZB_JOIN_CLIENT */
        }
        else
        {
            ZG->nwk.leave_context.leave_ind_prnt.addr_ref = addr_ref;
            ZG->nwk.leave_context.leave_ind_prnt.rejoin = ZB_LEAVE_PL_GET_REJOIN(lp);

#ifdef ZB_RAF_ADDRESS_MAP_LOCK_CNT_UNSYMMETRICAL
            buf = zb_buf_get_out();
            if (buf)
            {
                status = RET_OK;
                //printf("Z< (O) zb_nwk_leave_handler get buf \n");
            }
            else
            {
                status = RET_ERROR;
                //printf("Z< (X) zb_nwk_leave_handler get buf \n");
            }

            if (status == RET_OK)
            {
                zb_schedule_callback(zb_nwk_leave_ind_prnt, buf);
                (void)zb_address_lock(ZG->nwk.leave_context.leave_ind_prnt.addr_ref);
            }
#else
            status = zb_buf_get_out_delayed(zb_nwk_leave_ind_prnt);
            if (status == RET_OK)
            {
                /* No need to check return code, as addr_ref will be always valid */
                (void)zb_address_lock(ZG->nwk.leave_context.leave_ind_prnt.addr_ref);
            }
#endif

#if defined ZB_ROUTER_ROLE && defined ZB_JOIN_CLIENT
            if (nbt->relationship == ZB_NWK_RELATIONSHIP_PARENT
                    && ZB_LEAVE_PL_GET_REMOVE_CHILDREN(lp) != 0U
                    /*cstat !MISRAC2012-Rule-13.5 */
                    /* After some investigation, the following violation of Rule 13.5 seems to be
                     * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This
                     * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZC_OR_ZR()' is an
                     * external macro, which cannot be analyzed by C-STAT. */
                    && ZB_IS_DEVICE_ZC_OR_ZR())
            {
                send_leave_frame = ZB_TRUE;
            }
#endif
        }
    }

    if (status == RET_OK)
    {
        if (send_leave_frame)
        {
            /* Fill and send the NWK LEAVE command frame */
            zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
            if (secure && !ZG->aps.authenticated)
            {
                /* Silently leave if not authenticated */
                TRACE_MSG(TRACE_NWK1, "Not authenticated - do not send broadcast LEAVE", (FMT__0));
                zb_nwk_call_leave_ind(param, ZB_LEAVE_PL_GET_REJOIN(lp), (zb_address_ieee_ref_t) -1);
            }
            else
            {
                zb_uint8_t *new_lp;
                zb_nwk_hdr_t *nwhdr;

                nwhdr = nwk_alloc_and_fill_hdr(param,
                                               ZB_PIBCACHE_NETWORK_ADDRESS(),
                                               ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                               ZB_FALSE,
                                               secure,
                                               ZB_TRUE, ZB_TRUE);
                nwhdr->radius = 1;
                if (secure)
                {
                    nwk_mark_nwk_encr(param);
                }
                new_lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, (zb_uint8_t)sizeof(zb_uint8_t));
                *new_lp = 0;
                ZB_LEAVE_PL_SET_REJOIN(*new_lp, ZB_LEAVE_PL_GET_REJOIN(lp));
                ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*new_lp, ZB_LEAVE_PL_GET_REMOVE_CHILDREN(lp));
                /* When will got data.confirm at this request, do actual leave */
                ZG->nwk.leave_context.rejoin_after_leave = ZB_LEAVE_PL_GET_REJOIN(lp);

                (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_LEAVE_IND_AT_DATA_CONFIRM_HANDLE);
                TRACE_MSG(TRACE_NWK2, "send LEAVE with request 0, rejoin %hd, remove_children %hd",
                          (FMT__H_H, ZB_LEAVE_PL_GET_REJOIN(lp), ZB_LEAVE_PL_GET_REMOVE_CHILDREN(lp)));
                ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

#if defined ZB_ROUTER_ROLE && defined ZB_PRO_STACK
                /* it need to cancel sending Link Status command when Leave command
                 * was sent, but Leave Confirm doesn't received
                 */
                if (!ZB_U2B(ZB_LEAVE_PL_GET_REMOVE_CHILDREN(lp)))
                {
                    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_link_status_alarm, ZB_ALARM_ALL_CB);
                    /* fix for tp_r20_bv-13: we should NOT send Link Status after Leave */
                    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_send_link_status_command, ZB_ALARM_ALL_CB);
                }
#endif
            }
        }
        else
        {
            zb_buf_free(param);
        }
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK3, "< zb_nwk_leave_handler", (FMT__0));
}

void zb_nwk_leave_ind_prnt(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK3, ">> zb_nwk_leave_ind_prnt param %hd", (FMT__H, param));
    zb_nwk_call_leave_ind(param, ZG->nwk.leave_context.leave_ind_prnt.rejoin,
                          ZG->nwk.leave_context.leave_ind_prnt.addr_ref);
    zb_address_unlock(ZG->nwk.leave_context.leave_ind_prnt.addr_ref);
    TRACE_MSG(TRACE_NWK3, "<< zb_nwk_leave_ind_prnt", (FMT__0));
}

static void zb_nwk_call_leave_ind(zb_uint8_t param, zb_uint8_t rejoin, zb_address_ieee_ref_t addr_ref)
{
    zb_nlme_leave_indication_t *request;

    TRACE_MSG(TRACE_NWK3, ">>zb_nwk_call_leave_ind", (FMT__0));
    TRACE_MSG(TRACE_NWK3, "zb_nwk_call_leave_ind: param = %hd; rejoin = %hd; addr_ref = %hd;", (FMT__H_H_H, param, rejoin, addr_ref));

    request = ZB_BUF_GET_PARAM(param, zb_nlme_leave_indication_t);

    if (addr_ref == (zb_address_ieee_ref_t) -1)
    {
        ZB_IEEE_ADDR_ZERO(request->device_address);
    }
    else
    {
        zb_address_ieee_by_ref(request->device_address, addr_ref);
    }
    request->rejoin = rejoin;
    ZB_SCHEDULE_CALLBACK(zb_nlme_leave_indication, param);
    TRACE_MSG(TRACE_NWK3, "<<zb_nwk_call_leave_ind", (FMT__0));
}

#ifdef ZB_ROUTER_ROLE
static void zb_nwk_forget_rejoin(zb_uint8_t addr_ref)
{
    zb_uindex_t i;

    for (i = 0 ; i < ZG->nwk.handle.rejoin_req_table_cnt ; ++i)
    {
        if (addr_ref == ZG->nwk.handle.rejoin_req_table[i].addr_ref)
        {
            ZG->nwk.handle.rejoin_req_table_cnt--;
            if (i < ZG->nwk.handle.rejoin_req_table_cnt)
            {
                ZB_MEMMOVE(&ZG->nwk.handle.rejoin_req_table[i], &ZG->nwk.handle.rejoin_req_table[i + 1U],
                           (ZG->nwk.handle.rejoin_req_table_cnt - i) * sizeof(zb_rejoin_context_t));
            }

            break;
        }
    }
}
#endif

#if defined ZB_MAC_POWER_CONTROL
static void lpd_delete_device(zb_uint8_t param)
{
    zb_mlme_set_power_info_tbl_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_set_power_info_tbl_req_t);

    TRACE_MSG(TRACE_NWK1, ">> lpd_delete_device param %hd ieee_addr "TRACE_FORMAT_64,
              (FMT__H_A, param, TRACE_ARG_64(ZG->nwk.handle.lpd_leave_ieee)));

    ZB_BZERO(req, sizeof(zb_mlme_set_power_info_tbl_req_t));
    /* Mark for deleting */
    req->ent.nwk_negotiated = 0;
    ZB_IEEE_ADDR_COPY(req->ent.ieee_addr, ZG->nwk.handle.lpd_leave_ieee);

    ZB_SCHEDULE_CALLBACK(zb_mlme_set_power_information_table_request, param);

    TRACE_MSG(TRACE_NWK1, "<< lpd_delete_device", (FMT__0));
}
#endif  /* ZB_MAC_POWER_CONTROL */

void zb_nwk_forget_device(zb_uint8_t addr_ref)
{
    zb_uint16_t short_addr;

    /*
      Note: there are different implementation of that function in different branches.
      Not sure the result is correct.
      Anyway, leave code is to be rewritten.
      EE.
     */

    zb_address_short_by_ref(&short_addr, addr_ref);

#if defined ZB_MAC_POWER_CONTROL
    zb_address_ieee_by_ref(ZG->nwk.handle.lpd_leave_ieee, addr_ref);
#endif  /* ZB_MAC_POWER_CONTROL */

    TRACE_MSG(TRACE_NWK3, ">> zb_nwk_forget_device addr_ref %hd short_addr 0x%x", (FMT__H_D, addr_ref, short_addr));

    /* Short address can be absent if we are here after unsuccessful send of association comfirm. */
    if (short_addr != ZB_NWK_BROADCAST_ALL_DEVICES)
    {
#ifdef ZB_ROUTER_ROLE
        zb_nwk_forget_rejoin(addr_ref);
#endif

#ifdef ZB_JOIN_CLIENT
        if (short_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
        {
            zb_ret_t ret;

            /* Forget myself. Clear all pending APS retransmissions */
            ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_NWK_BROADCAST_ALL_DEVICES;
            /* Update our address in MAC as well */
            ret = zb_buf_get_out_delayed(zb_nwk_reset_nwk_addr_in_mac_pib);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }

            /* It is not fine to call APS from NWK anyway... But, at least, do it in a single call */
            /* Unbind all entries, remove all groups */
            apsme_forget_device();

            if (ZB_JOINED())
            {
                zb_ret_t ret_addr_del;

                /* the address was unlocked in nwk_clear_not_joined() which was called from zb_nwk_do_leave()
                   zb_nwk_forget_device for this device address is called only in zb_nwk_do_leave() */
                /* Delete address entry */
                ret_addr_del = zb_address_delete(addr_ref);
                ZB_ASSERT(ret_addr_del == RET_OK);
            }
        }
        else
#endif  /* ZB_JOIN_CLIENT */
        {
            zb_ret_t ret;

            zb_secur_delete_link_keys_by_addr_ref(addr_ref);
            /* Unbind all corresponding entries */
            zb_apsme_unbind_by_ref(addr_ref);
            /* Delete neigbhor entry and address entry */
            ret = zb_nwk_neighbor_delete(addr_ref);
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_nwk_neighbor_delete [%d]", (FMT__D, ret));
            }

#if defined ZB_PRO_STACK && !defined ZB_LITE_NO_SOURCE_ROUTING && defined ZB_ROUTER_ROLE
            zb_nwk_source_routing_record_delete(short_addr);
#endif
        }

#ifdef ZB_ROUTER_ROLE
        nwk_clear_pending_table_for_destination(short_addr);
#endif
    }

    /* [DD] move ED aging restart and writing info to NVRAM to zb_nwk_neighbor_complete_deletion
       because the NBT entry might be updated after several scheduler loop iterations */
#if 0
    /* Original code in r21. Let's keep it for a while here. */
#if !defined ZB_LIMIT_VISIBILITY
    zb_address_delete(addr_ref);
#endif
#ifdef ZB_USE_NVRAM
    zb_nvram_transaction_start();
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
    (void)zb_nvram_write_dataset(ZB_NVRAM_NEIGHBOUR_TBL);
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    zb_nvram_transaction_commit();
#endif

#ifdef ZB_ROUTER_ROLE
    /*Restart keepalive timeout if needed*///  if (((zb_uint8_t) ZG->nwk.neighbor.next_aging_end_device->addr_ref) == addr_ref)
    if (short_addr != ZB_PIBCACHE_NETWORK_ADDRESS()
            && ZB_IS_DEVICE_ZC_OR_ZR())
    {
        zb_nwk_restart_aging();
    }
#endif
#endif  /* 0 */

#if defined ZB_MAC_POWER_CONTROL
    if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(ZB_PIBCACHE_CURRENT_PAGE()))
    {
        zb_buf_get_out_delayed(lpd_delete_device);
    }
#endif

    TRACE_MSG(TRACE_NWK3, "<< zb_nwk_forget_device", (FMT__0));
}

#ifdef ZB_JOIN_CLIENT
static void nwk_clear_not_joined(zb_uint8_t param)
{
    zb_address_ieee_ref_t addr_ref;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_NWK3, ">>nwk_clear_not_joined", (FMT__0));

    ZB_SET_PARENT_INFO(0);
    ZB_ZDO_CANCEL_UPDATE_LONG_UPTIME();

#ifdef ZB_ROUTER_ROLE
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_link_status_alarm, ZB_ALARM_ALL_CB);
    /* fix for tp_r20_bv-13: we should NOT send Link Status after Leave */
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_send_link_status_command, ZB_ALARM_ALL_CB);
#endif  /* defined ZB_ROUTER_ROLE && defined ZB_PRO_STACK */

#if defined ZB_MAC_POWER_CONTROL
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_link_power_delta_alarm, ZB_ALARM_ALL_CB);
#endif  /* ZB_MAC_POWER_CONTROL */

#ifdef ZB_ED_FUNC
#ifndef ZB_LITE_NO_ED_AGING_REQ
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_ed_timeout_resp_recv_fail_trig, ZB_ALARM_ALL_CB);
#endif
#endif

    ZG->nwk.handle.router_started = ZB_FALSE;
    ZG->nwk.leave_context.pending_list_bm = 0;
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
    zb_nwk_broadcasting_clear();
#endif
    /* VP: can be here if our parent leave, so check parent presence */
    /* if ((ZB_NIB_DEVICE_TYPE() != ZB_NWK_DEVICE_TYPE_COORDINATOR) && */
    /*      ZB_JOINED() && (ZG->nwk.handle.parent != (zb_address_ieee_ref_t)-1)) */
    /* { */
    /*   zb_address_unlock(ZG->nwk.handle.parent); */
    /* } */
    /* ZB_SET_JOINED_STATUS(ZB_FALSE); */
#ifdef ZB_ROUTER_ROLE
    ZG->nwk.handle.rejoin_req_table_cnt = 0;
#endif
    zb_aps_clear_after_leave(0xffff);

    /* Unlock ourselves for lock/unlock balance */
    /* our address is locked during the association/rejoin process
       and it should be unlocked on leave */
    if (ZB_JOINED()
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of the Rule 13.5 seems to be a false
             * positive. The only way the function 'zb_address_by_ieee()' could have side effects is if it
             * is called with the second and third parameters ('create' and 'lock', respectively) equal to
             * ZB_TRUE, which is not the case. */
            && RET_OK == zb_address_by_ieee(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref))
    {
        zb_address_unlock(addr_ref);
    }
    ZB_NIB().nwk_hub_connectivity = 0;

    TRACE_MSG(TRACE_NWK3, "<<nwk_clear_not_joined", (FMT__0));
}

/**
   Do actual leave, potentially followed by rejoin

   @param param - buffer to be used for rejoin
   @param rejoin - rejoin flag
*/
void zb_nwk_do_leave(zb_uint8_t param, zb_uint8_t rejoin)
{
    TRACE_MSG(TRACE_ERROR, "zb_nwk_do_leave param %hd rejoin %hd", (FMT__H_H, param, rejoin));

    nwk_clear_not_joined(0);

#ifdef ZB_USE_ZB_TRAFFIC_WATCHDOG
    if (!rejoin)
    {
        zb_stop_watchdog(ZB_WD_ZB_TRAFFIC);
    }
#endif

    if (rejoin == 0U)
    {
        ZG->aps.authenticated = ZB_FALSE;
    }

    if (rejoin != 0U)
    {
        /* Upon rejoin we lost the parent and try to find new one (more likely it will be the same device);
         * this remove device from parents */
        if (ZB_JOINED()
                && ZG->nwk.handle.parent != (zb_address_ieee_ref_t) -1)
        {
            /* Delete the parent's neighbor entry, the device will choose new one after rejoin. */
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be a false
             * positive. There are no side effects to 'ZB_IS_DEVICE_ZR()'. This violation
             * seems to be caused by the fact that this function is an external function, which cannot
             * be analyzed by C-STAT. */
            if (ZB_IS_DEVICE_ZED() || ZB_IS_DEVICE_ZR())
            {
                zb_ret_t ret = zb_nwk_neighbor_delete(ZG->nwk.handle.parent);
                ZB_ASSERT(ret == RET_OK);
            }

            ZG->nwk.handle.parent = (zb_address_ieee_ref_t) -1;
        }

        ZB_SET_JOINED_STATUS(ZB_FALSE);

        ZB_SCHEDULE_CALLBACK(zdo_commissioning_leave_with_rejoin, param);
    }
    else                          /* leave w/o rejoin */
    {
        zb_address_ieee_ref_t addr_ref;

        TRACE_MSG(TRACE_NWK1, "LEAVE without rejoin", (FMT__0));

        if (zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
        {
            zb_nwk_forget_device(addr_ref);
        }

        /* We are leaving from the network, clear ext_pan_id-s */
        ZB_EXTPANID_ZERO(ZB_NIB_EXT_PAN_ID());

        if (!ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address)
#ifdef ZB_DISTRIBUTED_SECURITY_ON
                && !zb_tc_is_distributed()
#endif
           )
        {
            if (zb_address_by_ieee(ZB_AIB().trust_center_address, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK &&
                    !ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, ZB_PIBCACHE_EXTENDED_ADDRESS()))
            {
                TRACE_MSG(TRACE_NWK1, "Clear Trust Center lock", (FMT__0));

                if (ZB_U2B(ZB_AIB().tc_address_locked))
                {
                    /* Clear "Trust Center" lock */
                    zb_address_unlock(addr_ref);
                    ZB_AIB().tc_address_locked = ZB_FALSE_U;
                }

                /* Remove APS keys etc */
                if (addr_ref != ZG->nwk.handle.parent)
                {
                    zb_nwk_forget_device(addr_ref);
                }
            }
            ZB_BZERO(ZB_AIB().trust_center_address, sizeof(ZB_AIB().trust_center_address));
        }

        /* Forget the parent address and neighbor */
        if (ZG->nwk.handle.parent != (zb_address_ieee_ref_t) -1)
        {
            zb_nwk_forget_device(ZG->nwk.handle.parent);
        }

        zdo_clear_after_leave(param);
    }

    /* Clear nwk.handle.parent - currently we do not have a parent (even if we are going to rejoin). */
    ZG->nwk.handle.parent = (zb_address_ieee_ref_t) -1;
}


void zb_nwk_do_rejoin_after_leave(zb_uint8_t param)
{
    zb_nlme_join_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_join_request_t);

    ZB_BZERO(req, sizeof(*req)); /* all defaults to 0 */

    /* join to the same PAN */
    ZB_EXTPANID_COPY(req->extended_pan_id, ZB_NIB_EXT_PAN_ID());
#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZR())
    {
        ZB_MAC_CAP_SET_ROUTER_CAPS(req->capability_information); /* join as ZR */
        TRACE_MSG(TRACE_NWK1, "Rejoin to pan " TRACE_FORMAT_64 " as ZR",
                  (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));
    }
    else
#endif
    {
        TRACE_MSG(TRACE_NWK1, "Rejoin to pan " TRACE_FORMAT_64 " as ZE",
                  (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));
    }

    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
        ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(req->capability_information, 1U);
        /* Set power cap */
        ZB_MAC_CAP_SET_POWER_SOURCE(req->capability_information, 1U);
    }

    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(req->capability_information, 1U);
    req->rejoin_network = ZB_NLME_REJOIN_METHOD_REJOIN;
    zb_channel_page_list_copy(req->scan_channels_list, ZB_AIB().aps_channel_mask_list);
    req->scan_duration = ZB_DEFAULT_SCAN_DURATION; /* TODO: configure it somehow? */
    ZG->zdo.handle.rejoin = ZB_TRUE;

    /* All the necessary unlocks are done in nwk_clear_not_joined */
#if 0
    {
        zb_address_ieee_ref_t addr_ref;
        /* Unlock our address and our parent's address: it will be locked at rejoin confirm */
        if (zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
        {
            zb_address_unlock(addr_ref);
        }
    }
#endif
    /* Small delay for the case when a device rejoins after Leave request */
    ZB_SCHEDULE_ALARM(zb_nlme_join_request, param, 10);
}
#endif  /* ZB_JOIN_CLIENT */


zb_ushort_t zb_nwk_hdr_base_size(zb_uint8_t *fctl)
{
    return (
               ZB_NWK_SHORT_HDR_SIZE(ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(fctl)) +
               (zb_uint8_t)(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(fctl) + ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(fctl)) * sizeof(zb_ieee_addr_t)
           );
}


zb_uint8_t zb_get_nwk_header_size(const zb_nwk_hdr_t *hdr)
{
    zb_uint8_t hdr_size = (zb_uint8_t)ZB_OFFSETOF(zb_nwk_hdr_t, dst_ieee_addr);

    //  TRACE_MSG(TRACE_NWK3, "hdr frame control field = 0x%x 0x%x", (FMT__H_H, hdr->frame_control[ZB_PKT_16B_FIRST_BYTE], hdr->frame_control[ZB_PKT_16B_ZERO_BYTE]));

    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(hdr->frame_control)))
    {
        //    TRACE_MSG(TRACE_NWK3, "hdr has SOURCE_IEEE_FLAG", (FMT__0));
        hdr_size += (zb_uint8_t)sizeof(zb_ieee_addr_t);                       /* 8 B */
    }
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(hdr->frame_control)))
    {
        //    TRACE_MSG(TRACE_NWK3, "hdr has DESTINATION_IEEE_FLAG", (FMT__0));
        hdr_size += (zb_uint8_t)sizeof(zb_ieee_addr_t);                       /* 8 B */
    }
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(hdr->frame_control)))
    {
        TRACE_MSG(TRACE_NWK3, "hdr has MULTICAST_FLAG", (FMT__0));
        hdr_size += (zb_uint8_t)ZB_NWK_MULTICAST_CONTROL_FIELD_SIZE;     /* 1 B */
    }
#endif
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_ROUTE(hdr->frame_control)))
    {
        TRACE_MSG(TRACE_NWK3, "hdr has SOURCE_ROUTE_FLAG", (FMT__0));
        /* Relay entries */
        TRACE_MSG(TRACE_NWK3, "number of entries %d, hdr_size %d", (FMT__D_D, *((const zb_uint8_t *)((const zb_uint8_t *)hdr + hdr_size)), hdr_size));
        hdr_size += *((const zb_uint8_t *)((const zb_uint8_t *)hdr + hdr_size)) * (zb_uint8_t)sizeof(zb_uint16_t);

        /* Relay count field + Relay index field */
        hdr_size += (zb_uint8_t)sizeof(zb_uint8_t) + (zb_uint8_t)sizeof(zb_uint8_t);
    }
    if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SECURITY(hdr->frame_control)))
    {
        //    TRACE_MSG(TRACE_NWK3, "hdr has SECURITY_FLAG", (FMT__0));
        hdr_size += (zb_uint8_t)sizeof(zb_nwk_aux_frame_hdr_t);               /* 14 B */
    }

    TRACE_MSG(TRACE_NWK3, "zb_get_nwk_header_size: fcf 0x%hx 0x%hx secur %hd size=%hd",
              (FMT__H_H_H_H, hdr->frame_control[ZB_PKT_16B_FIRST_BYTE], hdr->frame_control[ZB_PKT_16B_ZERO_BYTE],
               ZB_NWK_FRAMECTL_GET_SECURITY(hdr->frame_control), hdr_size));
    return hdr_size;
}

#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
zb_nwk_multicast_control_field_t *zb_get_mc_field_from_header(zb_nwk_hdr_t *hdr)
{
    /* Get size only of base part of header nonincluding source routing fields */
    zb_uint8_t hdr_size = zb_get_nwk_header_size(hdr);
    zb_nwk_multicast_control_field_t *res = NULL;

    if ( ZB_NWK_FRAMECTL_GET_SECURITY(hdr->frame_control) )
    {
        hdr_size -= sizeof(zb_nwk_aux_frame_hdr_t);
    }

    /* Multicast control field must be last in header buffer, so get it by offset (-1) from end of the buffer */
    if (ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(hdr->frame_control))
    {
        res = (zb_nwk_multicast_control_field_t *)((((zb_uint8_t *)hdr) + hdr_size - ZB_NWK_MULTICAST_CONTROL_FIELD_SIZE));
        return res;
    }
    return NULL;
}
#endif  /* multicast */

#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
void zb_nwk_set_address_assignment_cb(zb_addr_assignment_cb_t cb)
{
    TRACE_MSG(TRACE_NWK3, "zb_nwk_set_address_assignment_cb: cb = 0x%x", (FMT__P, cb));
    ZG->nwk.addr_cb = cb;
}

void zb_nwk_set_dev_associate_cb(zb_addr_assignment_cb_t cb)
{
    TRACE_MSG(TRACE_NWK3, "zb_nwk_set_dev_associate_cb: cb = 0x%x", (FMT__P, cb));
    ZG->nwk.dev_associate_cb = cb;
}
#endif

#ifdef ZB_ROUTER_ROLE
/**
   Increment counter of TX.

   Normalize TX success/failure using tx counters window
 */
void nwk_txstat_tx_inc()
{
    if (ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] == 254U)
    {
        ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] = 255U;
        ZB_NIB().tx_stat.tx_ok_i = (ZB_NIB().tx_stat.tx_ok_i + 1U) % ZB_TX_STAT_WINDOW_SIZE;
        ZB_NIB().tx_stat.tx_total++;
        if (ZB_NIB().tx_stat.tx_ok_used < ZB_TX_STAT_WINDOW_SIZE)
        {
            /* switch to the next entry. no reuse */
            /* 255 means "no errors" */
            ZB_NIB().tx_stat.tx_ok_used++;
        }
        else
        {
            /* reuse entry - remove tx/falls from the window */
            ZB_NIB().tx_stat.tx_total -= ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i];
            if (ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] != 255U)
            {
                ZB_NIB().tx_stat.tx_fail--;
            }
        }

        ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] = 0U;
    }
    else
    {
        if (ZB_NIB().tx_stat.tx_ok_used == 0U)
        {
            ZB_NIB().tx_stat.tx_ok_used = 1U;
        }
        ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i]++;
        ZB_NIB().tx_stat.tx_total++;
    }
}

/* FIXME: remove ZDO from NWK! */
#include "zb_zdo.h"
#include "zb_manuf_prefixes.h"

/**
   Increment counter of failed TX. Schedule ZDO failures check routine.

   May normalize TX success/failure using tx counters window
 */
void nwk_txstat_fail_inc()
{
    if (ZB_NIB().tx_stat.tx_ok_used == 0U)
    {
        ZB_NIB().tx_stat.tx_ok_used = 1U;
    }
    ZB_NIB().tx_stat.tx_fail++;
    ZB_NIB().tx_stat.tx_ok_i = (ZB_NIB().tx_stat.tx_ok_i + 1U) % ZB_TX_STAT_WINDOW_SIZE;
    if (ZB_NIB().tx_stat.tx_ok_used < ZB_TX_STAT_WINDOW_SIZE)
    {
        ZB_NIB().tx_stat.tx_ok_used++;
    }
    else
    {
        ZB_NIB().tx_stat.tx_total -= ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i];
        if (ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] != 255U)
        {
            ZB_NIB().tx_stat.tx_fail--;
        }
    }

    ZB_NIB().tx_stat.tx_ok_cnts[ZB_NIB().tx_stat.tx_ok_i] = 0U;

    TRACE_MSG(TRACE_MAC3, "nwk_tx_fail %hd", (FMT__H, ZB_NIB_NWK_TX_FAIL()));
    if (!ZB_U2B(ZB_ZDO_GET_CHECK_FAILS()))
    {
        ZB_SCHEDULE_CALLBACK(zb_zdo_check_fails, 0);
    }
}


/**
   Clear TX statistic counters.

   To be used after channel change.
 */
void nwk_txstat_clear()
{
    ZB_BZERO(&ZB_NIB().tx_stat, sizeof(ZB_NIB().tx_stat));
}
#endif  /* #ifdef ZB_ROUTER_ROLE */

/**
  Fill packet parameters (that will be passed to apsde) with default values and assign handle;
  params here is pointer
 */
zb_apsde_data_ind_params_t *zb_nwk_init_apsde_data_ind_params(zb_bufid_t buf, zb_uint8_t handle)
{
    zb_apsde_data_ind_params_t *params = zb_buf_alloc_tail(buf, sizeof(zb_apsde_data_ind_params_t));
    params->mac_dst_addr = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;
    params->mac_src_addr = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;
    params->handle = handle;
    return params;
}


zb_uint16_t nwk_get_pkt_mac_source(zb_bufid_t b)
{
    zb_apsde_data_ind_params_t *mac_addrs = ZB_BUF_GET_PARAM(b, zb_apsde_data_ind_params_t);
    return mac_addrs->mac_src_addr;
}


#ifdef ZB_JOIN_CLIENT
zb_ret_t zb_nwk_validate_leave_req(zb_uint16_t src_addr)
{
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_NWK1, ">>zb_nwk_validate_leave_req src_addr %x role %hd",
              (FMT__D_H, src_addr, ZB_NIB_DEVICE_TYPE()));

#ifdef ZB_ROUTER_ROLE
    if (ZB_IS_DEVICE_ZC())
        /*cstat !MISRAC2012-Rule-2.1_b */
        /** @mdr{00012,17} */
    {
        /* Coordinator doesn't support leave command */
        ret = RET_ERROR;
    }
    else if (ZB_IS_DEVICE_ZR())
    {
        if (!ZB_U2B(ZB_NIB().leave_req_allowed))
        {
            ret = RET_ERROR;
        }
    }
    else
#endif
    {
#ifdef SNCP_MODE
        /* Allow/ Disallow to deny Nwk leave from parent device in ZED */
        if (!ZB_U2B(ZB_NIB().leave_req_allowed))
        {
            ret = RET_ERROR;
        }
        else
#endif
        {
            zb_uint8_t addr_ref;
            zb_neighbor_tbl_ent_t *nbr;
            zb_ret_t ret2;


            ret2 = zb_address_by_short(src_addr, ZB_FALSE, ZB_FALSE, &addr_ref);
            if (ret2 == RET_OK)
            {
                ret2 = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbr);
            }
            if (ret2 != RET_OK
                    || nbr->relationship != ZB_NWK_RELATIONSHIP_PARENT)
            {
                ret = RET_ERROR;
            }
        }
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_nwk_validate_leave_req ret %d", (FMT__D, ret));

    return ret;
}

static void zb_nwk_reset_nwk_addr_in_mac_pib(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    zb_uint16_t reset_value = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
    req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
    req->pib_index = 0;
    req->pib_length = (zb_uint8_t)sizeof(zb_uint16_t);
    ZB_MEMCPY((req + 1), &reset_value, sizeof(zb_uint16_t));
    req->confirm_cb_u.cb = zb_nwk_remove_beacon_payload;
    ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

static void zb_nwk_remove_beacon_payload(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
    req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH;
    req->pib_index = 0;
    req->pib_length = (zb_uint8_t)sizeof(zb_uint8_t);
    *((zb_uint8_t *) (req + 1)) = 0;
    req->confirm_cb_u.cb = NULL;
    ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

#endif  /* ZB_JOIN_CLIENT */


#if defined ZB_MAC_DUTY_CYCLE_MONITORING
void zb_mlme_duty_cycle_mode_indication(zb_uint8_t param)
{
    zb_mlme_duty_cycle_mode_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_mlme_duty_cycle_mode_indication_t);

    TRACE_MSG(TRACE_MAC1, ">> zb_mlme_duty_cycle_mode_indication param %hd status %hd",
              (FMT__H_H, param, ind->status));

    if (ZDO_CTX().duty_cycle_mode_ind_cb != NULL)
    {
        (ZDO_CTX().duty_cycle_mode_ind_cb)((zb_mac_duty_cycle_status_t)ind->status);
    }

    zb_buf_free(param);

    TRACE_MSG(TRACE_MAC1, "<< zb_mlme_duty_cycle_mode_indication", (FMT__0));
}
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

void zb_nwk_send_direct_leave_req(zb_uint8_t param, zb_uint16_t dst_addr)
{
    zb_bool_t secure;
    zb_nwk_hdr_t *nwhdr;
    zb_uint8_t *lp;

    TRACE_MSG(TRACE_NWK1, ">> zb_nwk_send_direct_leave_req param %hd, dst_addr 0x%x", (FMT__H_D, param, dst_addr));

    secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());
    nwhdr = nwk_alloc_and_fill_hdr(param,
                                   ZB_PIBCACHE_NETWORK_ADDRESS(),
                                   dst_addr,
                                   ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
    if (secure)
    {
        nwk_mark_nwk_encr(param);
    }
    /* Don't want it to be routed - see 3.4.4.2 */
    nwhdr->radius = 1;

    lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, (zb_uint8_t)sizeof(zb_uint8_t));
    *lp = 0;
    ZB_LEAVE_PL_SET_REJOIN(*lp, ZB_TRUE);
    ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_FALSE);
    ZB_LEAVE_PL_SET_REQUEST(*lp);
    zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(), dst_addr,
                               MAC_TX_OPTION_ACKNOWLEDGED_BIT,
                               ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE);
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

    TRACE_MSG(TRACE_NWK1, "<< zb_nwk_send_direct_leave_req", (FMT__0));
}


#ifdef ZB_ROUTER_ROLE
static void zb_nwk_network_status_handler(zb_bufid_t buf, zb_nwk_hdr_t *nwk_hdr, zb_nlme_status_indication_t *cmd)
{
    zb_nwk_command_status_t status = (zb_nwk_command_status_t)cmd->status;
    zb_uint16_t network_addr = cmd->network_addr;
    zb_bool_t processed = ZB_FALSE;

    ZVUNUSED(nwk_hdr);

    TRACE_MSG(TRACE_NWK2, "got network status cmd status %hd network_addr 0x%x", (FMT__H_D, status, network_addr));

    /* Case is not applied for routers acting as ED */
    switch (status)
    {
    /* [Max]: Historical code. Maybe post processing is needed */
    case ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT:
        (void)zb_nwk_address_conflict_resolve(buf, network_addr);
        processed = ZB_TRUE;
        break;

    case ZB_NWK_COMMAND_STATUS_NO_ROUTE_AVAILABLE:
    /* FALLTHROUGH */
    case ZB_NWK_COMMAND_STATUS_TREE_LINK_FAILURE:
    /* FALLTHROUGH */
    case ZB_NWK_COMMAND_STATUS_NONE_TREE_LINK_FAILURE:
        zb_nwk_mesh_delete_route(network_addr);
        break;

    default:
        TRACE_MSG(TRACE_NWK2, "Unhandled NWK status command frame", (FMT__0));
        break;
    }

    if (!processed)
    {
        /* notify higher layers */
        cmd = ZB_BUF_GET_PARAM(buf, zb_nlme_status_indication_t);
        cmd->status = status;
        cmd->network_addr = network_addr;
        /* Do prior schedule to exclude overflow on too many pkts. */
        ZB_SCHEDULE_CALLBACK_PRIOR(zb_nlme_status_indication, buf);
    }
}


/**
   Check send status of unicast NWK frame forwarded by us
 */
static void nwk_check_relay_frame_status(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr;
    zb_neighbor_tbl_ent_t *nbt;
    zb_mcps_data_confirm_params_t *confirm = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
    zb_nlme_send_status_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_send_status_t);
    zb_nwk_routing_t *routing_ent;

    TRACE_MSG(TRACE_NWK1, "nwk_check_relay_frame_status param %hd", (FMT__H, param));

    nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_NWK1, "src 0x%x dst 0x%x status %hd",
              (FMT__D_D_H, nwk_hdr->src_addr, nwk_hdr->dst_addr, confirm->status));

    if (confirm->status != MAC_SUCCESS)
    {
        /* Invalidate route to the failed router. Routers can repair the link by link statuses.
         * Do nothing for ZEDs. ZEDs are deleted by Aging mechanism.
         */
        if (zb_nwk_neighbor_get_by_short(nwk_hdr->dst_addr, &nbt) == RET_OK
                && nbt != NULL
                && (nbt->device_type == ZB_NWK_DEVICE_TYPE_ROUTER
                    || nbt->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR))
        {
            nbt->u.base.outgoing_cost = 0;
        }

#ifndef ZB_LITE_NO_SOURCE_ROUTING
        /* If a failed link is encountered while a device is forwarding a unicast data frame using
           a routing table entry with the many-to-one field set to TRUE, a network status command
           frame with status code of 0x0c indicating many-to-one route failure shall be generated
           and shall be unicast to a random router neighbor
         */
        NWK_ARRAY_FIND_ENT(ZB_NIB().routing_table, ZB_NWK_ROUTING_TABLE_SIZE, routing_ent, (routing_ent->dest_addr == nwk_hdr->dst_addr));
        if (routing_ent != NULL && routing_ent->many_to_one != 0U)
        {
            zb_nwk_many_to_one_route_failure(param);
            param = 0;
        }
        else
#endif /* ZB_LITE_NO_SOURCE_ROUTING */
            /* No failure should be reported in case of Network Status command */
            if (!(ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwk_hdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND
                    /* can't use zb_nwk_cmd_frame_get_cmd_id: not buf begin */

                    /*cstat -MISRAC2012-Rule-13.5 */
                    /* After some investigation, the following violation of Rule 13.5 seems to be
                      * a false positive. There are no side effects to 'zb_get_nwk_header_size()'. This
                      * violation seems to be caused by the fact that 'zb_get_nwk_header_size()' is an
                      * external function, which cannot be analyzed by C-STAT. */
                    && ((zb_uint8_t *)nwk_hdr)[zb_get_nwk_header_size(nwk_hdr)] == ZB_NWK_CMD_NETWORK_STATUS))
            {
                request->dest_addr = nwk_hdr->src_addr;
                request->status.status = ZB_NWK_COMMAND_STATUS_NONE_TREE_LINK_FAILURE;
                request->status.network_addr = nwk_hdr->dst_addr;
                request->ndsu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;

                ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, param);
                param = 0;
            }
            else
            {
                /* MISRA rule 15.7 requires empty 'else' branch. */
            }
    }

    if (param != 0U)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_NWK1, "<< nwk_check_relay_frame_status", (FMT__0));
}
#endif  /* ZB_ROUTER_ROLE */

void zb_mcps_build_data_request(zb_bufid_t buf, zb_uint16_t src_addr_param, zb_uint16_t dst_addr_param, zb_uint8_t tx_options_param, zb_uint8_t msdu_hande_param)
{
    /* zeroing all not initialized explicitly */
    zb_mcps_data_req_params_t *_p = zb_buf_alloc_tail(buf, sizeof(zb_mcps_data_req_params_t));

#ifndef ZB_MAC_EXT_DATA_REQ
    _p->src_addr = (src_addr_param);
    _p->dst_addr = (dst_addr_param);
    _p->tx_options = (tx_options_param);
    _p->msdu_handle = (msdu_hande_param);
#else
    _p->src_addr.addr_short = (src_addr_param);
    _p->dst_addr.addr_short = (dst_addr_param);
    _p->src_addr_mode = _p->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    _p->tx_options = (tx_options_param);
    _p->dst_pan_id = ZB_PIBCACHE_PAN_ID();
    _p->msdu_handle = (msdu_hande_param);
#endif
#ifdef ZB_ENABLE_NWK_RETRANSMIT
    _p->nwk_retry_cnt = 0;
#endif
}

void nwk_mark_nwk_encr1(zb_bufid_t buf, zb_uint16_t file_id, zb_uint16_t line)
{
    TRACE_MSG(TRACE_NWK1, "nwk_mark_nwk_encr1 param %hd file %hu:%hu", (FMT__H_H_H, buf, file_id, line));
    zb_buf_flags_or(buf, ZB_BUF_SECUR_NWK_ENCR);
}

#ifdef ZB_LOW_SECURITY_MODE
void zb_nwk_set_ieee_policy(zb_bool_t put_always)
{
    /* Inverted logic - see ZB_NIB().dst_ieee_policy definition. */
    ZB_NIB().ieee_policy = !put_always;
}
#endif

zb_uint16_t zb_nwk_get_parent(void)
{
    zb_uint16_t parent_addr = ZB_UNKNOWN_SHORT_ADDR;

    if (ZG->nwk.handle.parent < ZB_IEEE_ADDR_TABLE_SIZE)
    {
        zb_address_short_by_ref(&parent_addr, ZG->nwk.handle.parent);
    }

    return parent_addr;
}

#ifdef ZB_ROUTER_ROLE
/**
   Checks if the destination device is our neighbour.

   @param nwk_header (in) - NWK packet header that contains information on the
   destination device's address.
   @param nbt (out) - destination device neighbour table entry

   @return ZB_TRUE if the destination device is our neighbour, ZB_FALSE otherwise.
 */
static zb_bool_t is_destination_our_neighbour(const zb_nwk_hdr_t *nwk_header,
        zb_neighbor_tbl_ent_t **nbt)
{
    zb_address_ieee_ref_t addr_ref;
    zb_ret_t ret;

    ret = zb_address_by_short(nwk_header->dst_addr, ZB_FALSE, ZB_FALSE, &addr_ref);

    if (ret != RET_OK)
    {
        if (ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(nwk_header->frame_control) != 0U)
        {
            ret = zb_address_by_ieee(nwk_header->dst_ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref);
        }
    }

    if (ret == RET_OK)
    {
        ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, nbt);
    }

    if (ret == RET_OK)
    {
        if ((*nbt)->send_via_routing == 0U
#if defined ZB_PRO_STACK && !defined ZB_NO_NWK_MULTICAST
                && ZB_NWK_FRAMECTL_GET_MULTICAST_FLAG(nwk_header->frame_control) == 0U
#endif
           )
        {
            ret = RET_OK;
        }
        else
        {
            ret = RET_ERROR;
        }
    }

    return (ret == RET_OK) ? ZB_TRUE : ZB_FALSE;
}
#endif /* ZB_ROUTER_ROLE */

/*! @} */
