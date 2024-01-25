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
/* PURPOSE: Security support in ZDO
*/


#define ZB_TRACE_FILE_ID 2468
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_zdo_globals.h"
#include "zb_bdb_internal.h"

/*! \addtogroup ZB_SECUR */
/*! @{ */

#if defined ZB_COORDINATOR_ROLE && !defined ZB_LITE_NO_APS_DATA_ENCRYPTION
static void tc_send_2_app_link_keys(zb_uint8_t param, zb_uint16_t param2);
#endif

#ifdef ZB_COORDINATOR_ROLE
/**
   Check that device auth exclude by its BDA

   Special feature introduced for test 14.29  TP/SEC/BV-29-I Security NWK
   Remove II (No Pre-configured Key)-ZR:

   "gZC causes DUT ZED2 to be removed from the network via APS Remove by
   including ZED2 in the exclusion list of the trust center".


 */
static zb_bool_t zdo_secur_is_device_auth_excluded(zb_ieee_addr_t long_addr)
{
#ifdef ZB_CERTIFICATION_HACKS
    return ZB_IEEE_ADDR_CMP(ZB_CERT_HACKS().auth_excluded_dev_addr, long_addr);
#else
    ZVUNUSED(long_addr);
    return ZB_FALSE;
#endif
}
#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_JOIN_CLIENT
void zb_apsme_transport_key_indication(zb_uint8_t param)
{

    zb_apsme_transport_key_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_indication_t);

    TRACE_MSG(TRACE_SECUR1, ">zb_apsme_transport_key_indication %hd", (FMT__H, param));

    switch (ind->key_type)
    {
    case ZB_STANDARD_NETWORK_KEY:
    {
        zb_uint8_t i = 0;

        if (ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address)
                || ZB_TCPOL().tc_swapped
#ifdef ZB_DISTRIBUTED_SECURITY_ON
                ||
                zb_tc_is_distributed()
#endif
           )
        {
            ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, ind->src_address);
            TRACE_MSG(TRACE_SECUR1, "TC is " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_AIB().trust_center_address)));

#ifdef ZB_DISTRIBUTED_SECURITY_ON
            if (zb_tc_is_distributed())
            {
                if (!zb_distributed_security_enabled())
                {
                    TRACE_MSG(TRACE_SECUR1, "Peer authenticate us in distributed security while we do not accept distributed!", (FMT__0));
                    ZB_SCHEDULE_CALLBACK(zdo_commissioning_authentication_failed, param);
                    break;
                }
                else
                {
                    TRACE_MSG(TRACE_SECUR1, "Authenticated in distributed security", (FMT__0));
                }
            }
            else
            {
                /* just in case */
                TRACE_MSG(TRACE_SECUR1, "Authenticated in central security", (FMT__0));
            }
            zb_sync_distributed();
#endif

            /*
              Have a chance to remember long address of TC.
              But, short address is not known.
              Currently, it is hard-coded to 0


              I try to solve following scenario:
              - ZR1 joins ZC. ZC transport key to ZR1
              - ZR2 joins ZR1. ZR1 transports key to ZR2.
              ZR2 doesn't know ZC address now.
              - ZC broadcasts key transport, APS encrypt. ZR2 can't decrypt it
              because TC long address is unknown (zero)

              BTW, in BDB TC is always at ZC.
            */
#ifdef ZB_TC_AT_ZC
            /*cstat -MISRAC2012-Rule-14.3_a */
            if (!IS_DISTRIBUTED_SECURITY())
            {
                /*cstat +MISRAC2012-Rule-14.3_a */
                /** @mdr{00023,0} */
                zb_address_ieee_ref_t addr_ref;

                if (RET_ALREADY_EXISTS == zb_address_update(ZB_AIB().trust_center_address, 0,
                        ZB_FALSE, /* don't lock now! */
                        &addr_ref))
                {
                    /* We can't update TC addr after Swap-out with zb_address_update()*/
                    zb_long_address_update_by_ref(ZB_AIB().trust_center_address, addr_ref);
                }
                /* Lock ZC address only if it just created. We can be here multiple
                * times after TC rejoin during SE Rejoin recovery, including TC
                * swapout. Do not add more locks than it is necessary. */
                if (!zb_address_is_locked(addr_ref))
                {
                    /* Lock address - it should be stored in NVRAM. */
                    (void)zb_address_lock(addr_ref);
                    ZB_AIB().tc_address_locked = ZB_TRUE_U;
                }
            }
#else
            /*
              Can try to additionally check that ths packet is from device which we
              know (both long and short) and device is TC. It can be device other
              then ZC (short 0).
            */
            ZB_ASSERT(0);
#endif
        }

        /* check for all-zero key */
        if (secur_nwk_key_is_empty(ind->key.nwk.key))
        {
            /* In pre-r21 TC sends all-zero nwk key in case of pe-configured NWK
             * key and (not sure here) secured rejoin. */
            /* r21 devices does not send us empty key! */
            if (!ZG->aps.authenticated)
            {
                TRACE_MSG(TRACE_SECUR1, "got zero nwk key when not authenticated.", (FMT__0));
                ZB_SCHEDULE_CALLBACK(zdo_commissioning_authentication_failed, param);
                break;
            }
        }
        else
        {
            /* this calculation automatically takes key_seq_number overflow into account */
            i = (ZB_NIB().active_secur_material_i +
                 (zb_uint8_t)(ind->key.nwk.key_seq_number - ZB_NIB().active_key_seq_number))
                % ZB_SECUR_N_SECUR_MATERIAL;
            /* Strict condition for prevention array borders violation */
            if (i < ZB_ARRAY_SIZE(ZB_NIB().secur_material_set))
            {
                ZB_MEMCPY(ZB_NIB().secur_material_set[i].key, ind->key.nwk.key, ZB_CCM_KEY_SIZE);
                ZB_NIB().secur_material_set[i].key_seq_number = ind->key.nwk.key_seq_number;
            }
            else
            {
                ZB_ASSERT(0);
            }
#ifdef DEBUG_EXPOSE_ALL_KEYS
            zb_debug_bcast_key(NULL, ind->key.nwk.key);
#endif
            TRACE_MSG(TRACE_INFO1, "NWK key " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ind->key.nwk.key)));
            TRACE_MSG(TRACE_SECUR1, "update key i %hd seq %hd ",
                      (FMT__H_H, i, ind->key.nwk.key_seq_number));
            if (i == ZB_NIB().active_secur_material_i
                    && ind->key.nwk.key_seq_number != ZB_NIB().active_key_seq_number)
            {
                /* Probably, too old key. Recover by switching current key */
                ZB_NIB().active_key_seq_number = ind->key.nwk.key_seq_number;
                ZB_NIB().outgoing_frame_counter = 0;
#if defined ZB_USE_NVRAM
                /* Will store 0 + interval */
                /* If we fail, trace is given and assertion is triggered */
                (void)zb_nvram_write_dataset(ZB_IB_COUNTERS);
#endif
                TRACE_MSG(TRACE_SECUR1, "switch current key", (FMT__0));
            }
        }

        if (!ZG->aps.authenticated)
        {
            zb_address_ieee_ref_t addr_ref = ZG->nwk.handle.parent;
            zb_uint16_t parent_addr;

            zb_address_short_by_ref(&parent_addr, addr_ref);

            ZG->aps.authenticated = ZB_TRUE;

            /* Device successfully authenticated - extneighbors now not needed */
            zb_nwk_exneighbor_stop(parent_addr);

            if (ZB_NIB().active_key_seq_number != ind->key.nwk.key_seq_number)
            {
                ZB_NIB().active_key_seq_number = ind->key.nwk.key_seq_number;
                ZB_NIB().outgoing_frame_counter = 0;
            }
            ZB_NIB().active_secur_material_i = i;

#if defined ZB_USE_NVRAM
            /* Will store 0 + interval */
            /* If we fail, trace is given and assertion is triggered */
            (void)zb_nvram_write_dataset(ZB_IB_COUNTERS);
#endif
            TRACE_MSG(TRACE_SECUR1, "authenticated; curr key #%hd; secur_material_i %hd",
                      (FMT__H_H, ind->key.nwk.key_seq_number, ZB_NIB().active_secur_material_i));

            zdo_commissioning_authenticated(param);
        }
        else
        {
            zb_buf_free(param);
        }
        break;
    }

#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
    case ZB_APP_LINK_KEY:
        if (!ZB_IS_DEVICE_ZC()
                && !IS_DISTRIBUTED_SECURITY()
                /* TODO: in SE add here test for binding of Key
                 * establishment cluster at receiving side and flag if
                 * initiates APS key establishment at sending side.
                 */
                && ZB_TCPOL().accept_new_unsolicited_application_link_key
           )
        {
            if (ZDO_SELECTOR().app_link_key_ind_handler != NULL)
            {
                ZDO_SELECTOR().app_link_key_ind_handler(param);
            }
            else
            {
                TRACE_MSG(TRACE_SECUR2, "update app link key with " TRACE_FORMAT_64,
                          (FMT__A, TRACE_ARG_64(ind->key.app.partner_address)));
                /* It looks like verified/uverified is for TCLK only, so set it for
                   verified for app lk */
                (void)zb_secur_update_key_pair(ind->key.app.partner_address,
                                               ind->key.app.key,
                                               ZB_SECUR_UNIQUE_KEY,
                                               ZB_SECUR_VERIFIED_KEY,
                                               ZB_SECUR_KEY_SRC_UNKNOWN);
#ifdef ZB_USE_NVRAM
                /* If we fail, trace is given and assertion is triggered */
                (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif

                zb_buf_free(param);
            }
        }
        else
        {
            TRACE_MSG(TRACE_SECUR2, "Distributed - drop app key", (FMT__0));
            zb_buf_free(param);
        }
        break;
#endif  /* ZB_LITE_NO_APS_DATA_ENCRYPTION */
    case ZB_TC_LINK_KEY:
#ifdef ZB_DISTRIBUTED_SECURITY_ON
        if (IS_DISTRIBUTED_SECURITY())
        {
            TRACE_MSG(TRACE_SECUR2, "Distributed or SE - drop TCLK", (FMT__0));
            zb_buf_free(param);
        }
        else
#endif
            if (bdb_verify_tclk_in_progress() ||
                    !ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, ind->src_address))
            {
                TRACE_MSG(TRACE_SECUR1, "OOPS: got TCLK from " TRACE_FORMAT_64 " which is not TC " TRACE_FORMAT_64 " or Verify is already in progress - drop it",
                          (FMT__A_A, TRACE_ARG_64(ind->src_address), TRACE_ARG_64(ZB_AIB().trust_center_address)));
                zb_buf_free(param);
            }
            else
            {
                TRACE_MSG(TRACE_SECUR2, "update TCLK: " TRACE_FORMAT_128 " and initiate verify-key", (FMT__B, TRACE_ARG_128(ind->key.tc.key)));

                (void)zb_secur_update_key_pair(ind->src_address,
                                               ind->key.tc.key,
                                               ZB_SECUR_UNIQUE_KEY,
                                               ZB_SECUR_UNVERIFIED_KEY,
                                               ZB_SECUR_KEY_SRC_UNKNOWN);

#ifdef ZB_USE_NVRAM
                /* If we fail, trace is given and assertion is triggered */
                (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
                /* TCLK recv over APS command is not related to SE. Keep it as is. */
                /* BDB procedure of verify key in contrast with pyre r21 has timeout
                 * and retries. Seems no harm in using it in certification tests as
                 * well. So remove IS_BDB check and simplify. */
                ZB_SCHEDULE_CALLBACK(bdb_initiate_key_verify, param);
            }
        break;

    default:
        TRACE_MSG(TRACE_SECUR1, "unsupported key type %hd", (FMT__H, ind->key_type));
        zb_buf_free(param);
        break;
    }

    TRACE_MSG(TRACE_SECUR1, "<zb_apsme_transport_key_indication", (FMT__0));
}


void zb_zdo_update_tclk(zb_uint8_t param)
{
    zb_apsme_request_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_request_key_req_t);
#ifdef ZB_CERTIFICATION_HACKS
    zb_bool_t drop_req_key = ZB_FALSE;
#endif

    TRACE_MSG(TRACE_SECUR1, "request update of TCLK", (FMT__0));

    ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
    req->key_type = ZB_REQUEST_TC_LINK_KEY;
    ZB_IEEE_ADDR_COPY(req->partner_address, ZB_AIB().trust_center_address);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().req_key_call_cb)
    {
        drop_req_key = ZB_CERT_HACKS().req_key_call_cb(param);
        TRACE_MSG(TRACE_SECUR1, "req_key_call_cb %p drop_req_key %d", (FMT__P_D, ZB_CERT_HACKS().req_key_call_cb, drop_req_key));
    }

    if (drop_req_key == ZB_TRUE)
    {
        zb_buf_free(param);
    }
    else
#endif
    {
        ZB_SCHEDULE_CALLBACK(zb_secur_apsme_request_key, param);
    }
}

void zb_zdo_verify_tclk(zb_uint8_t param)
{
    zb_apsme_verify_key_req_t *req;
#ifdef ZB_CERTIFICATION_HACKS
    zb_bool_t drop_cmd = ZB_FALSE;
#endif

    TRACE_MSG(TRACE_SECUR2, "zb_zdo_verify_tclk param %hd", (FMT__H, param));
    /* Initiate key verification */
    req = ZB_BUF_GET_PARAM(param, zb_apsme_verify_key_req_t);
    ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
    req->key_type = ZB_TC_LINK_KEY;

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().verify_key_call_cb)
    {
        drop_cmd = ZB_CERT_HACKS().verify_key_call_cb(param);
    }
    if (drop_cmd == ZB_TRUE)
    {
        zb_buf_free(param);
    }
    else
#endif
    {
        ZB_SCHEDULE_CALLBACK(zb_apsme_verify_key_req, param);
    }
}
#endif /* ZB_JOIN_CLIENT */


#ifdef ZB_ROUTER_ROLE

zb_uint8_t zb_secur_gen_upd_dev_status(zb_ushort_t rejoin_network, zb_ushort_t secure_rejoin)
{
    /*
    NWK Rejoin (0x02)
    TRUE
    0x00
    Standard security device se-cured rejoin

    MAC Association (0x00)
    FALSE
    0x01
    Standard security device unse-cured join
    098
    NWK Rejoin (0x02)
    FALSE
    0x03
    Standard security device unse-cured rejoin
     */
    if (rejoin_network == ZB_NLME_REJOIN_METHOD_ASSOCIATION)
    {
        return ZB_STD_SEQ_UNSECURED_JOIN;
    }
    else if (rejoin_network == ZB_NLME_REJOIN_METHOD_REJOIN)
    {
        return secure_rejoin != 0U ? ZB_STD_SEQ_SECURED_REJOIN : ZB_STD_SEQ_UNSECURED_REJOIN;
    }
    else
    {
        TRACE_MSG(TRACE_SECUR1, "bad rejoin/secur combination %hd %hd", (FMT__H_H, rejoin_network, secure_rejoin));
        return (zb_uint8_t)~0U;
    }
}


/**
   Authnticate child - see 4.6.2.2

   Called from nlme-join.indication at ZC or ZR
 */
void secur_authenticate_child(zb_uint8_t param)
{
    zb_nlme_join_indication_t ind = *ZB_BUF_GET_PARAM(param, zb_nlme_join_indication_t);

    TRACE_MSG(TRACE_SECUR3, "authenticate child 0x%x, devt %hd is_tc %hd",
              (FMT__D_H_H, ind.network_address, zb_get_device_type(), ZB_IS_TC()));
#ifdef ZB_FORMATION
    if ((ZB_IS_DEVICE_ZC()
            && ZB_IS_TC())
            || (ZB_IS_DEVICE_ZR()
                && IS_DISTRIBUTED_SECURITY())
       )
    {
        ZB_ASSERT(APS_SELECTOR().authhenticate_child_directly);
        /* Call secur_authenticate_child_directly indirectly to link it to ZR only if distributed security is enabled. */
        ZB_SCHEDULE_CALLBACK(APS_SELECTOR().authhenticate_child_directly, param);
    }
    else
#endif  /* ZB_FORMATION */
    {
        /*  */
#ifndef ZB_COORDINATOR_ONLY
        /* 4.6.3.2.1  Router Operation */
        /* send UPDATE-DEVICE to TC */
        zb_apsme_update_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_update_device_req_t);
        req->status = zb_secur_gen_upd_dev_status(ind.rejoin_network, ind.secure_rejoin);
        ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
        ZB_IEEE_ADDR_COPY(req->device_address, ind.extended_address);
        req->device_short_address = ind.network_address;
        TRACE_MSG(TRACE_SECUR3, "update device 0x%x status %hd; send cmd to " TRACE_FORMAT_64, (FMT__D_H_A, ind.network_address, req->status, TRACE_ARG_64(req->dest_address)));
        ZB_SCHEDULE_CALLBACK(zb_apsme_update_device_request, param);
#else
        /* is it ever possible? */
        zb_buf_free(param);
#endif  /* ZB_COORDINATOR_ONLY */
    }
}

#endif  /* ZB_ROUTER_ROLE */

#ifdef ZB_COORDINATOR_ROLE
static void secur_send_key_sw_next(zb_uint8_t param);


void zb_secur_send_nwk_key_update_br(zb_uint8_t param)
{
    zb_uint16_t dest_br = *ZB_BUF_GET_PARAM(param, zb_uint16_t);
    zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

    TRACE_MSG(TRACE_SECUR3, "secur_send_nwk_key_update %hd", (FMT__H, param));

    req->addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    req->dest_address.addr_short = dest_br; /* broadcast - see 4.4.9.2.3.2    Network Key Descriptor Field */
    req->key.nwk.use_parent = 0; /* send key directly: we are parent! */
    req->key_type = ZB_STANDARD_NETWORK_KEY;
    req->key.nwk.key_seq_number = ZB_NIB().active_key_seq_number + 1;
    {
        zb_uint8_t *key = secur_nwk_key_by_seq(req->key.nwk.key_seq_number);
        if (!key)
        {
            TRACE_MSG(TRACE_ERROR, "No nwk key # %hd", (FMT__H, req->key.nwk.key_seq_number));
            zb_buf_free(param);
            return;
        }
        ZB_MEMCPY(req->key.nwk.key, key, ZB_CCM_KEY_SIZE);
    }
    TRACE_MSG(TRACE_SECUR3, "Send nwk key #%hd update to all, dest_br 0x%x",
              (FMT__H_D, req->key.nwk.key_seq_number, dest_br));
    ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param);
}


void zb_secur_send_nwk_key_switch(zb_uint8_t param)
{
    zb_uint16_t dest_br = *ZB_BUF_GET_PARAM(param, zb_uint16_t);
    zb_apsme_switch_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_switch_key_req_t);

    TRACE_MSG(TRACE_SECUR1, "zb_secur_send_nwk_key_switch param %hd dest_br %d", (FMT__H_D, param, dest_br));
    if (dest_br == 0)
    {
        /* Switch key, then unicast to all rx-on-when-idle */
        secur_nwk_key_switch(ZB_NIB().active_key_seq_number + 1);
        TRACE_MSG(TRACE_SECUR3, "send key switch to #%hd unicast to all devices from n.t.", (FMT__D, ZB_NIB().active_key_seq_number));
        ZG->aps.tmp.neighbor_table_iterator = zb_nwk_neighbor_next_rx_on_i(0);
        if (ZG->aps.tmp.neighbor_table_iterator != (zb_uint8_t)~0)
        {
            secur_send_key_sw_next(param);
        }
        else
        {
            TRACE_MSG(TRACE_SECUR3, "have nobody to send to", (FMT__0));
            zb_buf_free(param);
        }
    }
    else
    {
        ZB_MEMSET(req->dest_address, -1, sizeof(zb_ieee_addr_t));
        req->key_seq_number = ZB_NIB().active_key_seq_number + 1;
        /* Broadcast, then switch key: remember this buffer */
        ZG->zdo.handle.key_sw = param;
        TRACE_MSG(TRACE_SECUR3, "broadcast key switch, remember key_sw %hd", (FMT__H, ZG->zdo.handle.key_sw));
        ZB_SCHEDULE_CALLBACK(zb_apsme_switch_key_request, param);
    }
}


/**
   Send switch-key.request to the next rx-on-when-idle device from the neighbor table

   Iteration thru neighbor table made using aps.tmp.neighbor_table_iterator
   variable.
   Send command, iterate next device, if it exist, allocate buffer and call
   myself with it.

   @param param - buffer to use.
 */
static void secur_send_key_sw_next(zb_uint8_t param)
{
    zb_apsme_switch_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_switch_key_req_t);

    req->key_seq_number = ZB_NIB().active_key_seq_number;
    zb_address_ieee_by_ref(req->dest_address,
                           ZG->nwk.neighbor.neighbor[ZG->aps.tmp.neighbor_table_iterator].u.base.addr_ref);
    ZB_SCHEDULE_CALLBACK(zb_apsme_switch_key_request, param);

    ZG->aps.tmp.neighbor_table_iterator = zb_nwk_neighbor_next_rx_on_i(ZG->aps.tmp.neighbor_table_iterator + 1);
    if (ZG->aps.tmp.neighbor_table_iterator != (zb_uint8_t)~0)
    {
        zb_buf_get_out_delayed(secur_send_key_sw_next);
    }
}
#endif /* ZB_COORDINATOR_ROLE */

void secur_nwk_key_switch(zb_uint8_t key_number)
{
    zb_uint8_t i;
    for (i = 0 ;
            i < ZB_SECUR_N_SECUR_MATERIAL
            && ZB_NIB().secur_material_set[i].key_seq_number != key_number ;
            ++i)
    {
    }

    if (i == ZB_SECUR_N_SECUR_MATERIAL)
    {
#ifdef ZB_COORDINATOR_ROLE
        if (ZB_IS_TC())
        {
            i = (ZB_NIB().active_secur_material_i + 1) % ZB_SECUR_N_SECUR_MATERIAL;
#ifdef ZB_TC_GENERATES_KEYS
            /* We are here if no key with such key number found. Generate new one. */
            secur_nwk_generate_key(i, key_number);
#endif
        }
        else
#endif
        {
            TRACE_MSG(TRACE_SECUR1, "Could not find nwk key #%hd !", (FMT__H, key_number));
        }
    }

    if (i != ZB_SECUR_N_SECUR_MATERIAL)
    {
        ZB_NIB().active_secur_material_i = i;
        ZB_NIB().active_key_seq_number = key_number;
        ZB_NIB().prev_outgoing_frame_counter = ZB_NIB().outgoing_frame_counter;
        /*The only time the outgoing frame counter is reset to zero is when the device
          is al-ready on a network, it receives an APSME-SWITCH-KEY and its outgoing
          frame counter is greater than 0x80000000.*/
        if (ZB_NIB().outgoing_frame_counter > 0x8000000U)
        {
            ZB_NIB().outgoing_frame_counter = 0;
        }
#ifdef ZB_USE_NVRAM
        zb_nvram_transaction_start();
        /* Will store 0 + interval */
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_IB_COUNTERS);
        (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
        zb_nvram_transaction_commit();
#endif
        TRACE_MSG(TRACE_SECUR1, "switched to nwk key #%hd, i %hd secur_material_i %hd", (FMT__H_H_H, key_number, i, ZB_NIB().active_secur_material_i));
    }
#if !defined ZB_COORDINATOR_ONLY && !defined ZB_DISABLE_REJOIN_AFTER_KEY_SWITCH_FAIL
    else
    {
        /*cstat !MISRAC2012-Rule-14.3_a */
        if (!ZB_IS_TC())
        {
            /** @mdr{00012,24} */
            zb_ret_t ret = zb_buf_get_out_delayed(zb_secur_rejoin_after_security_failure);
            TRACE_MSG(TRACE_SECUR1, "Key switch failed. Try to rejoin", (FMT__0));
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
            }
        }
    }
#endif  /* !defined ZB_COORDINATOR_ONLY && !defined ZB_DISABLE_REJOIN_AFTER_KEY_SWITCH_FAIL */
}

#ifdef ZB_COORDINATOR_ROLE
void zb_secur_switch_nwk_key_br(zb_uint8_t param)
{
    if (!param)
    {
        zb_buf_get_out_delayed(zb_secur_switch_nwk_key_br);
    }
    else
    {
        zb_uint16_t *dest_br_p = ZB_BUF_GET_PARAM(param, zb_uint16_t);
        *dest_br_p = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
        TRACE_MSG(TRACE_SECUR1, "zb_secur_switch_nwk_key_br", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_secur_send_nwk_key_switch, param);
    }
}


/**
   Authenticate device by sending NWK key either after its direct join or after receiving Device Update - common part.
 */
zb_ret_t zb_authenticate_dev(zb_bufid_t param, zb_apsme_update_device_ind_t *ind)
{
    zb_ret_t ret;
    zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
    zb_bool_t use_parent = !ZB_IEEE_ADDR_CMP(ind->src_address, ZB_PIBCACHE_EXTENDED_ADDRESS());


    TRACE_MSG(TRACE_SECUR1, "zb_authenticate_dev: use_parent %d status %d permit_joining %d joiner " TRACE_FORMAT_64 " parent " TRACE_FORMAT_64,
              (FMT__D_D_D_A_A, use_parent, ind->status, ZG->nwk.handle.permit_join,
               TRACE_ARG_64(ind->device_address), TRACE_ARG_64(ind->src_address)));

    if (! zdo_secur_is_device_auth_excluded(ind->device_address))
    {
        ret = RET_OK;
    }
    else
    {
        ret = RET_UNAUTHORIZED;
    }

    if (ret == RET_OK)
    {
        /*
          see 4.6.3.2.2.1    Standard Security Mode.

          Trust Center shall send
          the device the active network key.

          Unicast TRANSPORT-KEY.request to the originator of UPDATE-DEVICE.indication,
          secure APS command.
        */
        req->key_type = ZB_STANDARD_NETWORK_KEY;
        /* secur_authenticate_child_directly() fills src_address by ZC's own address */
        req->key.nwk.use_parent = use_parent;
        ZB_IEEE_ADDR_COPY(req->key.nwk.parent_address, ind->src_address);
        ZB_IEEE_ADDR_COPY(req->dest_address.addr_long, ind->device_address);
        req->addr_mode = ZB_ADDR_64BIT_DEV;
        ZB_MEMCPY(req->key.nwk.key, ZB_NIB().secur_material_set[ZB_NIB().active_secur_material_i].key, ZB_CCM_KEY_SIZE);
        req->key.nwk.key_seq_number = ZB_NIB().active_key_seq_number;

        /* see table 4.40 */
        if (ind->status == ZB_STD_SEQ_SECURED_REJOIN)
        {
            ZB_SCHEDULE_CALLBACK(zdo_commissioning_secure_rejoin_setup_lk_alarm, param);
        }
        /* Comment out UNSECURED_REJOIN: BDB 10.3.2:
           2. The Trust Center SHALL determine if the Status parameter is equal to 0x01 (Unsecured
           join).
           a. If this is not true, the Trust Center SHALL continue from step 12.
        */
        else
        {
            zb_bool_t have_tclk;

            ZB_ASSERT(ind->status == ZB_STD_SEQ_UNSECURED_JOIN || ind->status == ZB_STD_SEQ_UNSECURED_REJOIN);
            /* According to BDB Figure 15 Trust Center link key exchange
             * procedure forget TCLK at child join */
            /* There was check for ZB_IN_BDB(). Seems classic r21 commissioning can work like BDB.
               Not 100% sure about SE, so try explicit check for SE. */
            if (!ZB_SE_MODE()
                    && ind->status == ZB_STD_SEQ_UNSECURED_JOIN
#if defined ZB_CERTIFICATION_HACKS
                    && ZB_CERT_HACKS().use_preconfigured_aps_link_key == 0U
#endif /* ZB_CERTIFICATION_HACKS */
               )
            {
                zb_secur_delete_link_keys_by_long_addr(ind->device_address);
            }

            /* That call checks that we either have valid Verified TCLK (used CBKE in
             * case of SE) or Provisional key create by hashing TCLK in the previous
             * life of TC.  There is one problem: what if we for some reason have
             * Provisional keypair which created elsewhere - for example, using an
             * installcode? Is is a problem to accept that device TC Rejoin if network
             * is already closed?
             */
            have_tclk = (zb_secur_get_verified_or_provisional_link_key(ind->device_address) != NULL);
            TRACE_MSG(TRACE_SECUR4, "have_tclk %hd", (FMT__H, have_tclk));

            if (ind->status == ZB_STD_SEQ_UNSECURED_REJOIN)
            {
                /* Unsecured rejoin (TC rejoin) may be disallowed or ignored by
                 * application - check it first */
                if (ZB_TCPOL().ignore_unsecure_tc_rejoins)
                {
                    /* TC rejoin ignored, this allows to get a secure join next
                     * When Coordinator works through hop sending a Remove Device
                     * calls leave without rejoin */

                    TRACE_MSG(TRACE_SECUR3, "zb_authenticate_dev: ignore unsecure rejoin, drop packet", (FMT__0));
                    ret = RET_IGNORE;
                    zb_buf_free(param);
                }
                else if (!ZB_TCPOL().allow_tc_rejoins)
                {
                    TRACE_MSG(TRACE_SECUR3, "zb_authenticate_dev: disallow unsecure rejoin", (FMT__0));
                    ret = RET_UNAUTHORIZED;
                }
                /* If permit joining is off, allow unsecure rejoins only if device already has TCLK.
                   Prevent unsecure rejoin attacks.
                */
                else if (!ZG->nwk.handle.permit_join
                         && !have_tclk)
                {
                    TRACE_MSG(TRACE_SECUR3, "zb_authenticate_dev: permit joining is off, TC rejoin, no TCLK - reject", (FMT__0));
                    ret = RET_UNAUTHORIZED;
                }
            }
            /* Removed ZDO_CTX().handle.allow_auth. Instead, when device joined by association,
               do not authenticate it if permit joining is off. */
            if (ind->status == ZB_STD_SEQ_UNSECURED_JOIN
                    && (!(ZG->nwk.handle.permit_join && ZB_TCPOL().allow_joins) && !ZB_TCPOL().authenticate_always)
               )
            {
                TRACE_MSG(TRACE_SECUR3, "zb_authenticate_dev: permit_join %d allow_joins %d - disallow association",
                          (FMT__D_D, ZG->nwk.handle.permit_join, ZB_TCPOL().allow_joins));
                ret = RET_UNAUTHORIZED;
            }
#ifdef ZB_SECURITY_INSTALLCODES
            if (ret == RET_OK)
            {
                /* BDB specification 1.0
                   10.3.2 Adding a new node into the network

                   Trust Center link key exchange procedure
                   ...
                   4. If bdbJoinUsesInstallCodeKey is equal to TRUE
                   and bdbJoiningNodeEui64 does not correspond to an entry in apsDeviceKeyPairSet,
                   the Trust Center SHALL continue from step 12.
                   ...
                   12. The Trust Center SHALL do the following before terminating the procedure
                   for adding a new node into the network:
                   a. Expire the bdbTrustCenterNodeJoinTimeout timer.
                   b. Set the value of the bdbJoiningNodeNewTCLinkKey to zero.
                   c. Set the value of the bdbJoiningNodeEui64 to zero
                */
                zb_uint8_t key[ZB_CCM_KEY_SIZE];
                if ((ZB_IS_TC() && ZB_TCPOL().require_installcodes)
                        && !have_tclk
                        && zb_secur_ic_get_key_by_address(ind->device_address, key) != RET_OK)
                {
                    TRACE_MSG(TRACE_SECUR3, "zb_authenticate_dev: ignore this update device, can't find an install code for " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ind->device_address)));
                    ret = RET_IGNORE;
                    zb_buf_free(param);
                }
            }
#endif
            if (ret == RET_OK)
            {
                /* Send transport key primitive */
                TRACE_MSG(TRACE_SECUR1, "zb_authenticate_dev: AUTH ALLOWED apsme_transport_key", (FMT__0));

                if (have_tclk
                        || zb_secur_create_best_suitable_link_key_pair_set(ind->device_address))
                {
                    TRACE_MSG(TRACE_SECUR3, "Indirect %d send nwk key #%hd to " TRACE_FORMAT_64,
                              (FMT__D_H_A, use_parent, req->key.nwk.key_seq_number, TRACE_ARG_64(ind->device_address)));

                    ZB_SCHEDULE_CALLBACK2(zdo_commissioning_send_nwk_key_to_joined_dev, param, 0);
                }
                else
                {
                    ret = RET_UNAUTHORIZED;
                }
            }
        }
    }

    zb_prepare_and_send_device_update_signal(ind->device_address, ind->status);

    if (ret != RET_OK)
    {
        zb_prepare_and_send_device_authorized_signal(ind->device_address,
                ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK,
                ZB_ZDO_TCLK_AUTHORIZATION_FAILED);
    }

    return ret;
}

/**
   Calculate 'status' field for 'update device' operation.

   see "Table 4.32 Mapping of NLME-JOIN.indication Parameters to Update Device Status" in r21
*/

/**
   UPDATE-DEVICE.indication primitive
*/
void zb_apsme_update_device_indication(zb_uint8_t param)
{
    zb_apsme_update_device_ind_t ind;

    TRACE_MSG(TRACE_SECUR1, ">> zb_apsme_update_device_indication param %hd", (FMT__H, param));

    /* We are TC if we got this packet. */

    ind = *ZB_BUF_GET_PARAM(param, zb_apsme_update_device_ind_t);

#ifdef ZB_DISTRIBUTED_SECURITY_ON
    if (IS_DISTRIBUTED_SECURITY())
    {
        TRACE_MSG(TRACE_SECUR1, "Distributed security - drop update-device", (FMT__0));
        zb_buf_free(param);
        param = 0;
    }
#endif
    if (param)
    {
        if (ind.status == ZB_DEVICE_LEFT)
        {
            zb_address_ieee_ref_t addr_ref;
            /* remove device from the neighbor table and address translation table */
            TRACE_MSG(TRACE_SECUR3, "zb_apsme_update_device_indication: ZB_DEVICE_LEFT remove dev address", (FMT__0));
            TRACE_MSG(TRACE_SECUR3, "Device %d left - forget it", (FMT__D, ind.device_short_address));

            if (zb_address_by_short(ind.device_short_address, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK
                    || zb_address_by_ieee(ind.device_address, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
            {
                bdb_cancel_link_key_refresh_alarm(bdb_link_key_refresh_alarm, addr_ref);
                zdo_commissioning_device_left(addr_ref);
                zb_nwk_forget_device(addr_ref);
                zb_buf_free(param);
            }

            zb_prepare_and_send_device_update_signal(ind.device_address, ind.status);
        }
        else
        {
            /* Device joined/rejoined */
            if (zb_authenticate_dev(param, &ind) == RET_UNAUTHORIZED)
            {
                /* auth is disallowed, send remove device request */
                zb_apsme_remove_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_req_t);
                ZB_IEEE_ADDR_COPY(req->parent_address, ind.src_address);
                ZB_IEEE_ADDR_COPY(req->child_address, ind.device_address);
                TRACE_MSG(TRACE_SECUR3, "zb_apsme_update_device_indication: AUTH DISALLOWED apsme_remove_device", (FMT__0));
                ZB_SCHEDULE_CALLBACK(zb_secur_apsme_remove_device, param);
            }
        }
    }

    TRACE_MSG(TRACE_SECUR1, "<< zb_apsme_update_device_indication", (FMT__0));
}
#endif  /* ZB_COORDINATOR_ROLE */


#ifdef ZB_ROUTER_SECURITY
/**
   TC asked us to remove that device.
   Issue LEAVE request.
 */
void zb_apsme_remove_device_indication(zb_uint8_t param)
{
    zb_apsme_remove_device_ind_t ind;
    zb_nlme_leave_request_t *lr;

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_ind_t), sizeof(ind));
    lr = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
    TRACE_MSG(TRACE_SECUR2, "zb_apsme_remove_device_ind from " TRACE_FORMAT_64 " child " TRACE_FORMAT_64,
              (FMT__A_A, TRACE_ARG_64(ind.src_address), TRACE_ARG_64(ind.child_address)));

    if (ZB_IEEE_ADDR_CMP(ind.src_address, ZB_AIB().trust_center_address))
    {
        /* see 4.6.3.6  Network Leave, 4.6.3.6.2  Router Operation */
        ZB_IEEE_ADDR_COPY(lr->device_address, ind.child_address);
        lr->remove_children = 0;      /* not sure */
        lr->rejoin = 0;               /* not sure */
        ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
    }
    else
    {
        TRACE_MSG(TRACE_SECUR1, "zb_apsme_remove_device_ind from non-TC - ignore it", (FMT__0));
        zb_buf_free(param);
    }
}
#endif /* ZB_ROUTER_SECURITY */

#ifdef ZB_JOIN_CLIENT
/**
   switch-key.indication primitive
 */
void zb_apsme_switch_key_indication(zb_uint8_t param)
{
    zb_apsme_switch_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_switch_key_ind_t);

    TRACE_MSG(TRACE_SECUR3, "zb_apsme_switch_key_ind from " TRACE_FORMAT_64 " key # %hd",
              (FMT__A_H, TRACE_ARG_64(ind->src_address), ind->key_seq_number));

    /*cstat -MISRAC2012-Rule-14.3_a */
    if (!IS_DISTRIBUTED_SECURITY())
    {
        /*cstat +MISRAC2012-Rule-14.3_a */
        /** @mdr{00023,1} */
        secur_nwk_key_switch(ind->key_seq_number);
    }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
    else
    {
        TRACE_MSG(TRACE_SECUR1, "Distributed - drop switch key", (FMT__0));
    }
#endif
    zb_buf_free(param);
}


void zb_apsme_confirm_key_indication(zb_uint8_t param)
{
    zb_apsme_confirm_key_ind_t *ind = ZB_BUF_GET_PARAM(param, zb_apsme_confirm_key_ind_t);

    TRACE_MSG(TRACE_SECUR3, "zb_apsme_confirm_key_indication %hd", (FMT__H, param));
    /* See 4.4.8.2 APSME-CONFIRM-KEY.indication, 4.4.8.2.3 Effect on Receipt */

    /*cstat -MISRAC2012-Rule-2.1_b */
    if (ZB_IS_TC())
    {
        /*cstat +MISRAC2012-Rule-2.1_b */
        /** @mdr{00012,9} */
        TRACE_MSG(TRACE_SECUR1, "I am TC - drop confirm key", (FMT__0));
    }
#ifdef ZB_DISTRIBUTED_SECURITY_ON
    else if (IS_DISTRIBUTED_SECURITY())
    {
        TRACE_MSG(TRACE_SECUR1, "Distributed - drop confirm key", (FMT__0));
    }
#endif
    else if (ind->key_type != ZB_TC_LINK_KEY)
    {
        TRACE_MSG(TRACE_SECUR1, "zb_apsme_confirm_key_indication : invalid key type %hd drop confirm key", (FMT__H, ind->key_type));
    }
    else if (ind->status != ZB_APS_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_SECUR1, "zb_apsme_confirm_key_indication : bad status %hd", (FMT__H, ind->status));
        zdo_commissioning_secur_failed(param);
        param = 0;
    }
    else if (!ZB_IEEE_ADDR_CMP(ind->src_address, ZB_AIB().trust_center_address))
    {
        TRACE_MSG(TRACE_SECUR1, "zb_apsme_confirm_key_indication : not from TC - drop", (FMT__0));
    }
    else
    {
        zb_uint16_t aps_key_idx = zb_aps_keypair_get_index_by_addr(ind->src_address, ZB_SECUR_UNVERIFIED_KEY);

        if (aps_key_idx != (zb_uint16_t) -1)
        {
            zb_aps_device_key_pair_set_t *aps_key_update_p = zb_aps_keypair_get_ent_by_idx(aps_key_idx);
            zb_aps_device_key_pair_set_t aps_key_update;

            ZB_MEMCPY(&aps_key_update, aps_key_update_p, sizeof(aps_key_update));
            aps_key_update.key_attributes = ZB_SECUR_VERIFIED_KEY;

            /* delete verified key if exists */
            {
                zb_uint16_t keypair_i = zb_aps_keypair_get_index_by_addr(ind->src_address, ZB_SECUR_VERIFIED_KEY);
                if (keypair_i != (zb_uint16_t) -1)
                {
                    TRACE_MSG(TRACE_SECUR2, "Delete verified key, keypair_i: %d", (FMT__D, keypair_i));
                    zb_secur_delete_link_key_by_idx(keypair_i);
                }
            }

#ifndef ZB_NO_CHECK_INCOMING_SECURE_APS_FRAME_COUNTERS
            ZB_AIB().aps_device_key_pair_storage.key_pair_set[aps_key_idx].incoming_frame_counter = 0;
#endif
            (void)zb_aps_keypair_write(&aps_key_update, aps_key_idx);
        }
        zdo_secur_update_tclk_done(param);
        param = 0;
    }
    if (param != 0U)
    {
        zb_buf_free(param);
    }
}
#endif /* ZB_JOIN_CLIENT */


#ifdef ZB_COORDINATOR_ROLE

void zb_apsme_request_key_indication(zb_uint8_t param)
{
    zb_apsme_request_key_ind_t ind;

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsme_request_key_ind_t), sizeof(ind));

    TRACE_MSG(TRACE_SECUR3, "zb_apsme_request_key_indication ind.keypair_i %d", (FMT__D, ind.keypair_i));

    if (!ZB_IS_TC() || IS_DISTRIBUTED_SECURITY())
    {
        /* In distributed never set is_tc and never answer on request key */
        TRACE_MSG(TRACE_SECUR1, "Drop request-key.indication of TCLK on non-TC device", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
        switch (ind.key_type)
        {
#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
        case ZB_REQUEST_APP_LINK_KEY:
        {
            zb_bool_t drop_req = ZB_FALSE;

            /* r21: If the RequestKeyType is 0x02, Application Link Key, then follow the procedure in section 4.7.3.8. */
            if (
#ifndef ZB_COORDINATOR_ONLY
                /* The Trust Center shall ignore any requests made to establish application link keys with itself. */
                !ZB_IEEE_ADDR_CMP(ind.partner_address, ZB_AIB().trust_center_address)
                &&
#endif
                /*
                  If the Trust Center policy of allowApplicationLinkKeyRequests is 0x01,
                  then the Trust Center shall do the following.

                  a. Run the procedure in section 4.7.3.8.1 using SrcAddress
                  from the primitive as the Initia-torAddress in the procedure, and
                  PartnerAddress from the primitive as the Re-sponderAddress in the
                  procedure.
                */
                (ZB_AIB().tcpolicy.allow_application_link_key_requests == 1
                 || (
                     /*
                       If the Trust Center policy of
                       allowApplicationLinkKeyRequests is 0x02, then the
                       following shall be performed.

                       a. Find an entry in the allowApplicationKeyRequestList where the
                       SrcAddress of the primitive matches the Initiator Address
                          of the entry, and the PartnerAddress of the primitive
                          matches the Responder Address of the entry */
                     /* TODO: implement */
                     ZB_AIB().tcpolicy.allow_application_link_key_requests == 2 && 0)
                ))
            {
                if (ZB_SE_MODE())
                {
                    /* SE spec 1.4, subclause 5.4.7.4
                     *
                     * The Trust Center would not send a link key to either node if one of the nodes
                     * has not authenticated using key establishment.
                     */
                    zb_aps_device_key_pair_set_t *initiator_key = zb_secur_get_link_key_by_address(ind.src_address, ZB_SECUR_VERIFIED_KEY);
                    zb_aps_device_key_pair_set_t *responder_key = zb_secur_get_link_key_by_address(ind.partner_address, ZB_SECUR_VERIFIED_KEY);

                    TRACE_MSG(TRACE_SECUR1, "SE partner link key establishment request", (FMT__0));

                    if (!initiator_key || initiator_key->key_source != ZB_SECUR_KEY_SRC_CBKE)
                    {
                        TRACE_MSG(TRACE_SECUR1, "PLKE initiator wasn't authenticated using CBKE!", (FMT__0));
                        drop_req = ZB_TRUE;
                    }

                    if (!responder_key || responder_key->key_source != ZB_SECUR_KEY_SRC_CBKE)
                    {
                        TRACE_MSG(TRACE_SECUR1, "PLKE responder wasn't authenticated using CBKE!", (FMT__0));
                        drop_req = ZB_TRUE;
                    }

                    TRACE_MSG(TRACE_SECUR1, "PLKE drop_req: %d", (FMT__D, drop_req));
                }
            }
            else
            {
                drop_req = ZB_TRUE;
            }
            if (!drop_req)
            {
                /* Simplify our life: instead of old
                 * zb_secur_send_transport_key_second use second buf */
                zb_buf_get_out_delayed_ext(tc_send_2_app_link_keys, param, 0);
            }
            else
            {
                TRACE_MSG(TRACE_SECUR1, "Drop request-key.indication of app link key", (FMT__0));
                zb_buf_free(param);
            }
        }
        break;
#endif  /* #ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION */
        case ZB_REQUEST_TC_LINK_KEY:
        {
            /*
              2. The device shall verify the key used to encrypt the APS command. If the
              SrcAddress of the APSME-REQUEST-KEY.indication primitive does not equal the
              value of the DeviceAddress of the corresponding apsDeviceKeyPairSet entry
              used to decrypt the message, the message shall be dropped and no further
              processing shall take place.
            */

            zb_bool_t drop = ZB_FALSE;
            zb_aps_device_key_pair_set_t *keypair = NULL;

            zb_legacy_device_auth_signal_cancel(ind.src_address);

            if (ind.keypair_i != (zb_uint16_t) -1)
            {
                keypair = zb_aps_keypair_get_ent_by_idx(ind.keypair_i);
            }

            if ((ind.keypair_i == (zb_uint16_t) -1) &&
                    (zb_secur_get_link_key_by_address(ind.src_address, ZB_SECUR_ANY_KEY_ATTR) != NULL))
            {
                TRACE_MSG(TRACE_SECUR1, "request-key.indication encrypted by keypair not corresponding to src_address - drop", (FMT__0));
                drop = ZB_TRUE;
            }
            else if ((ind.keypair_i != (zb_uint16_t) -1)
                     && (keypair != NULL)
                     && (!ZB_IEEE_ADDR_CMP(keypair->device_address, ind.src_address)))
            {
                TRACE_MSG(TRACE_SECUR1, "request-key.indication encrypted by keypair not corresponding to src_address - drop", (FMT__0));
                drop = ZB_TRUE;
            }
            else if (!(ZB_AIB().tcpolicy.allow_tc_link_key_requests == 1
                       /* r21: If the RequestKeyType is 0x04, Trust Center Link Key, then follow the procedure in section 4.7.3.6. */
                       ||
                       /*
                         If allowTrustCenterLinkKeyRequests is 2, do the following. 11738
                         i. Find an entry in the apsDeviceKeyPairSet of the AIB where the DeviceAddress of 11739 the entry matches the PartnerAddress of the APSME-REQUEST-KEY.indication 11740 primitive, and the KeyAttributes has a value of PROVISIONAL_KEY (0x00). If 11741 no entry can be found matching those criteria, then the request shall be silently 11742 dropped and no more processing shall be done.

                         TODO: implement (if need it - it is O).
                       */
                       (ZB_AIB().tcpolicy.allow_tc_link_key_requests == 2 && 0)))
            {
                TRACE_MSG(TRACE_SECUR1, "request-key.indication drop due to TC policy", (FMT__0));
                drop = ZB_TRUE;
            }

            if (drop)
            {
                zb_buf_free(param);
            }
            else
            {
                zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);

                req->key_type = ZB_TC_LINK_KEY;

                secur_generate_key(req->key.app.key);
                /* TC has no Application Link keys. It has only TCLK - either Global or
                 * Unique. We are generating Unique TCLK now. So, sore it in the key pair set. */
                zb_secur_update_key_pair(ind.src_address,
                                         req->key.app.key,
                                         ZB_SECUR_UNIQUE_KEY,
                                         ZB_SECUR_UNVERIFIED_KEY,
                                         ZB_SECUR_KEY_SRC_UNKNOWN);

                ZB_IEEE_ADDR_COPY(req->dest_address.addr_long, ind.src_address);
                req->addr_mode = ZB_ADDR_64BIT_DEV;
                /* send TC link key. */
                TRACE_MSG(TRACE_SECUR3, "send unique TC link key to " TRACE_FORMAT_64,
                          (FMT__A, TRACE_ARG_64(ind.src_address)));
                ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param);
            }
        }
        break;
        default:
            TRACE_MSG(TRACE_ERROR, "Unexpected request-key.indication. Key type = %d", (FMT__H, ind.key_type));
            zb_buf_free(param);
        }
    }
}


void zb_apsme_verify_key_indication(zb_uint8_t param)
{
    zb_ret_t status = ZB_APS_STATUS_SUCCESS;
    zb_apsme_verify_key_ind_t ind;
    zb_apsme_confirm_key_req_t *rsp;
    zb_aps_device_key_pair_set_t *aps_key = NULL;

    TRACE_MSG(TRACE_SECUR1, "zb_apsme_verify_key_indication", (FMT__0));

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsme_verify_key_ind_t), sizeof(ind));
    /* See 4.4.7.2.3 Effect on Receipt */

    /*
    2. If the device is not the Trust Center, this is not a valid request. The device shall follow the pro-10779 cedure in section 4.4.7.2.3.1 setting the Status value to 0xa3 (ILLEGAL_REQUEST). No further 10780 processing shall be done. 10781
    3. If the StandardKeyType parameter is not equal to 0x04 (Trust Center Link Key), the request is in-10782 valid. The device shall follow the procedure in section 4.4.7.2.3.1 setting the Status value to 10783 0xaa (NOT_SUPPORTED). No further processing shall be done. 10784
    4. If the apsTrustCenterAddress of the AIB is set to 0xFFFFFFFFFFFFFFFF, the device is operating 10785 in distributed Trust Center mode and this is not a valid request. The device shall follow the pro-10786 cedure in section 4.4.7.2.3.1 setting the Status value to 0xaa (NOT_SUPPORTED). No further 10787 processing shall be done. 10788
    5. The device shall find the corresponding entry in the apsDeviceKeyPairSet attribute of the AIB 10789 where the DeviceAddress matches the SrcAddress of this primitive and the KeyAttributes is UN-10790 VERIFIED_KEY (0x01) or VERIFIED_KEY (0x02). If no entry matching those criteria is 10791 found, the following shall be performed. 10792
    a. The Security Manager shall follow the procedure in section 4.4.7.2.3.1
    setting the Status 10793 value to 0xad (SECURITY_FAILURE).
    */
    if (!ZB_IS_TC() || IS_DISTRIBUTED_SECURITY())
    {
        TRACE_MSG(TRACE_SECUR1, "Not TC - verify key failed", (FMT__0));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_ILLEGAL_REQUEST);
    }
    else if (ind.key_type != ZB_TC_LINK_KEY)
    {
        TRACE_MSG(TRACE_SECUR1, "key_type %hd not supported - verify key failed", (FMT__H, ind.key_type));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NOT_SUPPORTED);
    }
    else
    {
        aps_key = zb_secur_get_link_key_by_address(ind.src_address, ZB_SECUR_UNVERIFIED_KEY);
        if (!aps_key)
        {
            aps_key = zb_secur_get_link_key_by_address(ind.src_address, ZB_SECUR_VERIFIED_KEY);
        }
        if (!aps_key)
        {
            TRACE_MSG(TRACE_SECUR1, "aps_key %p not found or wrong key attribute - verify key failed", (FMT__P, aps_key));
            status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_SECURITY_FAIL);
        }
    }
    if (status == ZB_APS_STATUS_SUCCESS)
    {
        zb_uint8_t key[ZB_CCM_KEY_SIZE];

        zb_cmm_key_hash(aps_key->link_key, 3, key);
        if (ZB_MEMCMP(key, ind.key_hash, ZB_CCM_KEY_SIZE))
        {
            TRACE_MSG(TRACE_SECUR1, "key hash does not match - verify key failed", (FMT__0));
            status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_SECURITY_FAIL);
        }
    }
    if (status == ZB_APS_STATUS_SUCCESS)
    {
        zb_aps_device_key_pair_set_t aps_key_update;
        TRACE_MSG(TRACE_SECUR1, "verify key ok", (FMT__0));

        ZB_MEMCPY(&aps_key_update, aps_key, sizeof(aps_key_update));

        /* delete verified key if exists */
        {
            zb_uint16_t keypair_i = zb_aps_keypair_get_index_by_addr(ind.src_address, ZB_SECUR_VERIFIED_KEY);
            if (keypair_i != (zb_uint16_t) -1)
            {
                TRACE_MSG(TRACE_SECUR2, "Delete verified key, keypair_i: %d", (FMT__D, keypair_i));
                zb_secur_delete_link_key_by_idx(keypair_i);
            }
        }

        zb_secur_update_key_pair(aps_key_update.device_address,
                                 aps_key_update.link_key,
#ifndef ZB_LITE_NO_GLOBAL_VS_UNIQUE_KEYS
                                 aps_key_update.aps_link_key_type,
#else
                                 ZB_SECUR_UNIQUE_KEY,
#endif
                                 ZB_SECUR_VERIFIED_KEY,
                                 ZB_SECUR_KEY_SRC_UNKNOWN);

        /* delete provisioning key if exists */
        {
            zb_uint16_t keypair_i = zb_aps_keypair_get_index_by_addr(ind.src_address, ZB_SECUR_PROVISIONAL_KEY);
            if (keypair_i != (zb_uint16_t) -1)
            {
                TRACE_MSG(TRACE_SECUR2, "Delete provisioning key %p", (FMT__P, aps_key));
                zb_secur_delete_link_key_by_idx(keypair_i);
            }
        }
#if defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE
        /* Indicate to the app that new verified key is create - time to backup */
        zb_tcsw_key_added();
#endif
    }

    rsp = ZB_BUF_GET_PARAM(param, zb_apsme_confirm_key_req_t);
    ZB_IEEE_ADDR_COPY(rsp->dest_address, ind.src_address);
    rsp->status = status;
    rsp->key_type = ind.key_type;
    TRACE_MSG(TRACE_SECUR3, "confirm_key_resp param %hd status %hd key_type %hd", (FMT__H_H_H, param, rsp->status, rsp->key_type));

    zb_prepare_and_send_device_authorized_signal(ind.src_address,
            ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK,
            (status == ZB_APS_STATUS_SUCCESS)
            ? ZB_ZDO_TCLK_AUTHORIZATION_SUCCESS
            : ZB_ZDO_TCLK_AUTHORIZATION_FAILED);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_CERT_HACKS().deliver_conf_key_cb)
    {
        zb_uint16_t keypair_i = 0xffff;

        keypair_i = zb_aps_keypair_get_index_by_addr(ind.src_address, ZB_SECUR_VERIFIED_KEY);

        ZB_CERT_HACKS().deliver_conf_key_cb(param, keypair_i);
    }
#endif

    ZB_SCHEDULE_CALLBACK(zb_apsme_confirm_key_request, param);

#ifndef ZB_LITE_NO_TRUST_CENTER_REQUIRE_KEY_EXCHANGE
    /* Cancel the alarm unconditionally. */
    /* if (ZB_TCPOL().update_trust_center_link_keys_required) */
    {
        zb_address_ieee_ref_t ref;
        if (zb_address_by_ieee(ind.src_address, ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
        {
            zb_uint8_t param = bdb_cancel_link_key_refresh_alarm(bdb_link_key_refresh_alarm, ref);
            if (param)
            {
                zb_buf_free(param);
            }
            if (status == ZB_APS_STATUS_SUCCESS)
            {
                zdo_commissioning_tclk_verified_remote(ref);
            }
        }
        else
        {
            TRACE_MSG(TRACE_SECUR1, "Oops: can't get address by long " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ind.src_address)));
        }
    }
#endif  /* #ifndef ZB_LITE_NO_TRUST_CENTER_REQUIRE_KEY_EXCHANGE */
}

#ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION
static void tc_send_2_app_link_keys(zb_uint8_t param2, zb_uint16_t param)
{
    /*
     * 4.7.3.8.1 Procedure for Generating and Sending Application Link Keys
     *
     * This procedure takes two IEEE addresses, InitiatorAddress and ResponderAddress.
     * 1. The Trust Center shall generate a random 128-bit key KeyValue for the application link key.
     * 2. It shall issue an APSME-TRANSPORT-KEY.request with the StandardKeyType
     *    set to 0x03, Application Link Key, the TransportKeyData set to KeyValue,
     *    and the DestAddress set to InitiatorAddress.
     * 3. It shall issue a second APSME-TRANSPORT-KEY.request with the StandardKeyType
     *    set to 0x03, Application Link Key, the TransportKeyData set to KeyValue,
     *    and the DestAddress set to ResponderAddress.
    */
    zb_apsme_request_key_ind_t ind;
    zb_apsme_transport_key_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_transport_key_req_t);
    zb_apsme_transport_key_req_t *req2 = ZB_BUF_GET_PARAM(param2, zb_apsme_transport_key_req_t);

    ZB_MEMCPY(&ind, ZB_BUF_GET_PARAM(param, zb_apsme_request_key_ind_t), sizeof(ind));

    /* #1 */
    secur_generate_key(req->key.app.key);

    /* #2 - prepare 1st request */
    req->key_type = ZB_APP_LINK_KEY;
    req->addr_mode = ZB_ADDR_64BIT_DEV;
    ZB_IEEE_ADDR_COPY(req->dest_address.addr_long, ind.src_address);
    ZB_IEEE_ADDR_COPY(req->key.app.partner_address, ind.partner_address);
    req->key.app.initiator = 1;

    /* #3  - prepare 2nd request */
    ZB_MEMCPY(req2, req, sizeof(*req));
    ZB_IEEE_ADDR_COPY(req2->dest_address.addr_long, ind.partner_address);
    ZB_IEEE_ADDR_COPY(req2->key.app.partner_address, ind.src_address);
    req2->key.app.initiator = 0;

    TRACE_MSG(TRACE_SECUR3, "send app link key to " TRACE_FORMAT_64  " and  " TRACE_FORMAT_64,
              (FMT__A_A, TRACE_ARG_64(ind.src_address), TRACE_ARG_64(ind.partner_address)));

    /* #2,3 - schedule both APSME-TRANSPORT-KEY.requests */
    ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param);
    ZB_SCHEDULE_CALLBACK(zb_apsme_transport_key_request, param2);
}
#endif  /* #ifndef ZB_LITE_NO_APS_DATA_ENCRYPTION */

#endif /* ZB_COORDINATOR_ROLE */

void zb_secur_setup_nwk_key(zb_uint8_t *key, zb_uint8_t i)
{
    if (i >= ZB_SECUR_N_SECUR_MATERIAL)
    {
        TRACE_MSG(TRACE_ERROR, "Too big nwk key # %hd", (FMT__H, i));
    }
    else
    {
        ZB_MEMCPY(&ZB_NIB().secur_material_set[i].key[0], key, ZB_CCM_KEY_SIZE);
        ZB_NIB().secur_material_set[i].key_seq_number = i;
    }
}

zb_bool_t secur_nwk_key_is_empty(zb_uint8_t *key)
{
    zb_ushort_t i;

    /* After introducing of secur_nwk_key_by_seq() key can be NULL */
    if (key == NULL)
    {
        return ZB_TRUE;
    }
    TRACE_MSG(TRACE_SECUR1, "key " TRACE_FORMAT_128, (FMT__AA, TRACE_ARG_128(key)));

    for (i = 0 ; i < ZB_CCM_KEY_SIZE && key[i] == 0U ; ++i)
    {
    }
    return (zb_bool_t)(i == ZB_CCM_KEY_SIZE);
}


zb_uint8_t *secur_nwk_key_by_seq(zb_ushort_t key_seq_number)
{
    zb_ushort_t i = ZB_NIB().active_secur_material_i;
    zb_ushort_t cnt = 0;

    while (cnt++ < ZB_SECUR_N_SECUR_MATERIAL
            && ZB_NIB().secur_material_set[i].key_seq_number != key_seq_number)
    {
        i = (i + 1U) % ZB_SECUR_N_SECUR_MATERIAL;
    }

    if (cnt > ZB_SECUR_N_SECUR_MATERIAL)
    {
        TRACE_MSG(TRACE_SECUR1, "No nwk key # %hd", (FMT__H, key_seq_number));
        return NULL;
    }
    TRACE_MSG(TRACE_SECUR3, "cnt %hd seq %hd - i %hd, ret %p", (FMT__H_H_H_P, cnt, key_seq_number, i, ZB_NIB().secur_material_set[i].key));
    return ZB_NIB().secur_material_set[i].key;
}


zb_aps_device_key_pair_set_t *zb_secur_update_key_pair(zb_ieee_addr_t address,
        zb_uint8_t *key,
        zb_uint8_t key_type,
        zb_uint8_t key_attr,
        zb_uint8_t key_source
                                                      )
{
    zb_uint_t idx = zb_64bit_hash(address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
    zb_uint_t n = 0;
    zb_aps_device_key_pair_set_t key_pair;
    zb_ret_t ret = RET_NOT_FOUND;

    TRACE_MSG(TRACE_SECUR3, ">>zb_secur_update_key_pair key_attr %hd", (FMT__H, key_attr));

#ifdef DEBUG_EXPOSE_ALL_KEYS
    zb_debug_bcast_key(address, key);
#endif

    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
        if (zb_aps_keypair_read_by_idx(idx, &key_pair) == RET_OK)
        {
            if (ZB_IEEE_ADDR_CMP(key_pair.device_address, address)
                    /* When adding verified/unverified key, keep existing provisional and verified key */
                    /* DL: This logic just overwrites the provisional and verified key.
                           And the above is written back. How is it correct? */
                    && !((key_pair.key_attributes == ZB_SECUR_PROVISIONAL_KEY ||
                          key_pair.key_attributes == ZB_SECUR_VERIFIED_KEY)
                         && key_pair.key_attributes != key_attr))
            {
                TRACE_MSG(TRACE_SECUR1, "AIB Key-Pair #%d update, old key_attr %hu new key_attr %hu",
                          (FMT__D_H_H, idx, key_pair.key_attributes, key_attr));
                ZB_MEMCPY(key_pair.link_key, key, ZB_CCM_KEY_SIZE);
                key_pair.key_attributes = key_attr;
                (void)zb_aps_keypair_write(&key_pair, idx);
                ret = RET_OK;
                break;
            }
        }

        idx = (idx + 1U) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
    }

    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_SECUR1, "keypair not found, create new", (FMT__0));

        ZB_BZERO(&key_pair, sizeof(zb_aps_device_key_pair_set_t));

        ZB_MEMCPY(key_pair.device_address, address, sizeof(zb_ieee_addr_t));
        ZB_MEMCPY(key_pair.link_key, key, ZB_CCM_KEY_SIZE);
#ifndef ZB_LITE_NO_GLOBAL_VS_UNIQUE_KEYS
        key_pair.aps_link_key_type = key_type;
#endif
        key_pair.key_source = key_source;
        key_pair.key_attributes = key_attr;

        if (zb_aps_keypair_write(&key_pair, (zb_uint16_t) -1) == RET_OK)
        {
            TRACE_MSG(TRACE_SECUR3, "Added link key pair #%d device " TRACE_FORMAT_64 " key " TRACE_FORMAT_128 " key_type %hd key_attr %hd",
                      (FMT__D_A_B_H_H, ZB_AIB().aps_device_key_pair_storage.cached_i, TRACE_ARG_64(address), TRACE_ARG_128(key), key_type, key_attr));
            ret = RET_OK;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "ERROR no free space to add", (FMT__0));
        }
    }
    TRACE_MSG(TRACE_SECUR3, "<<zb_secur_update_key_pair", (FMT__0));
    return (ret == RET_OK) ? &ZB_AIB().aps_device_key_pair_storage.cached : NULL;
}

zb_bool_t zb_secur_has_verified_key_by_short(zb_uint16_t addr_short)
{
    zb_bool_t ret = ZB_FALSE;
    zb_ieee_addr_t ieee_address;
    zb_aps_device_key_pair_set_t *aps_key;

    if (zb_address_ieee_by_short(addr_short, ieee_address) == RET_OK)
    {
        aps_key = zb_secur_get_link_key_by_address(ieee_address, ZB_SECUR_VERIFIED_KEY);
        if (aps_key != NULL && aps_key->key_source == ZB_SECUR_KEY_SRC_CBKE)
        {
            ret = ZB_TRUE;
        }
    }

    return ret;
}

zb_aps_device_key_pair_set_t *zb_secur_get_link_key_pair_set(zb_ieee_addr_t address, zb_bool_t valid_only)
{
    zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_by_address(address, ZB_SECUR_VERIFIED_KEY);

    TRACE_MSG(TRACE_SECUR4, ">> zb_secur_get_link_key_pair_set address " TRACE_FORMAT_64 " valid_only %hd", (FMT__A_H, TRACE_ARG_64(address), valid_only));
    TRACE_MSG(TRACE_SECUR4, "  verified result %p", (FMT__P, aps_key));
#ifndef ZB_SE_BDB_MIXED
    /*cstat -MISRAC2012-Rule-2.1_b */
    if (ZB_SE_MODE())
    {
        /*cstat +MISRAC2012-Rule-2.1_b */
        /** @mdr{00010,1} */

        /* TODO: Modify - check here only key_attr, check key_source outside */
        if (aps_key != NULL && aps_key->key_source != ZB_SECUR_KEY_SRC_CBKE)
        {
            TRACE_MSG(TRACE_SECUR4, "  SE: aps_key %p aps_key->key_source %hd",
                      (FMT__P_H, aps_key, aps_key != NULL ? aps_key->key_source : (zb_uint16_t) -1));
            return NULL;
        }
    }
    else
#endif
    {
        if (aps_key == NULL)
        {
            aps_key = zb_secur_get_link_key_by_address(address, valid_only == ZB_TRUE ? ZB_SECUR_PROVISIONAL_KEY : ZB_SECUR_ANY_KEY_ATTR);
            TRACE_MSG(TRACE_SECUR4, "  valid_only %hu aps_key %p aps_key->key_source %hd",
                      (FMT__H_P_H, valid_only, aps_key, aps_key != NULL ? aps_key->key_source : (zb_uint16_t) -1));
        }
    }
    TRACE_MSG(TRACE_SECUR4, "<< zb_secur_get_link_key_pair_set ret %p", (FMT__P, aps_key));

    return aps_key;
}

zb_aps_device_key_pair_set_t *zb_secur_get_verified_or_provisional_link_key(zb_ieee_addr_t address)
{
    zb_aps_device_key_pair_set_t *aps_key = zb_secur_get_link_key_by_address(address, ZB_SECUR_VERIFIED_KEY);

#ifndef ZB_SE_BDB_MIXED
    /*cstat -MISRAC2012-Rule-2.1_b */
    if (ZB_SE_MODE())
    {
        /*cstat +MISRAC2012-Rule-2.1_b */
        /** @mdr{00010,2} */
        if (aps_key != NULL && aps_key->key_source != ZB_SECUR_KEY_SRC_CBKE)
        {
            aps_key = NULL;
        }
    }
#endif

    if (aps_key == NULL)
    {
        aps_key = zb_secur_get_link_key_by_address(address, ZB_SECUR_PROVISIONAL_KEY);
    }

    TRACE_MSG(TRACE_SECUR4, "zb_secur_get_verified_or_provisional_link_key aps_key %p", (FMT__P, aps_key));

    if (aps_key != NULL)
    {
        TRACE_MSG(TRACE_SECUR4, "  key_source %hd key_attributes %hd", (FMT__H_H, aps_key->key_source, aps_key->key_attributes));
    }

    return aps_key;
}


zb_aps_device_key_pair_set_t *zb_secur_create_best_suitable_link_key_pair_set(zb_ieee_addr_t address)
{
    zb_aps_device_key_pair_set_t *aps_key = NULL;
#ifdef ZB_DISTRIBUTED_SECURITY_ON
    if (IS_DISTRIBUTED_SECURITY())
    {
        /* maybe we already created keypair for distributed key */
        aps_key = zb_secur_get_link_key_pair_set((zb_uint8_t *)g_unknown_ieee_addr, ZB_FALSE);
        if (!aps_key)
        {
            /* if in distributed mode, use distributed global key */
            aps_key = zb_secur_update_key_pair(address, ZB_AIB().tc_standard_distributed_key, ZB_SECUR_GLOBAL_KEY, ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
            TRACE_MSG(TRACE_SECUR1, "Created aps_key %p from default distributed security key", (FMT__P, aps_key));
        }
    }
    else if (ZB_IEEE_ADDR_CMP(address, g_unknown_ieee_addr))
    {
        aps_key = zb_secur_update_key_pair(address, ZB_AIB().tc_standard_distributed_key, ZB_SECUR_GLOBAL_KEY, ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
        TRACE_MSG(TRACE_SECUR1, "Created aps_key %p from default distributed security key", (FMT__P, aps_key));
    }
    else
#endif  /* ZB_DISTRIBUTED_SECURITY_ON */
    {
        /* if not in distributed mode, use installcode if it exists, else use
         * default TCLK */
        zb_uint8_t key[ZB_CCM_KEY_SIZE];
        zb_uint8_t key_type = ZB_SECUR_UNIQUE_KEY;
        zb_ret_t ret;

#ifdef ZB_SECURITY_INSTALLCODES
        ret = zb_secur_ic_get_key_by_address(address, key);
#else
        ret = RET_NOT_FOUND;
#endif
        if (ret != RET_OK)
        {
            /*cstat -MISRAC2012-Rule-2.1_b -MISRAC2012-Rule-14.3_b */
            if (!ZB_JOIN_USES_INSTALL_CODE_KEY(ZB_FALSE))
            {
                /*cstat +MISRAC2012-Rule-2.1_b +MISRAC2012-Rule-14.3_b */
                /** @mdr{00014,1} */
                ret = RET_OK;
                TRACE_MSG(TRACE_SECUR1, "Use standard ZB key", (FMT__0));
                ZB_MEMCPY(key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
                key_type = ZB_SECUR_GLOBAL_KEY;
            }
            else
            {
                TRACE_MSG(TRACE_SECUR1, "Only installcode can be used, so fail authentication via standard key", (FMT__0));
            }
        }
        else
        {
            TRACE_MSG(TRACE_SECUR1, "Derive ZB key from installcode", (FMT__0));
        }
        if (ret == RET_OK)
        {
            aps_key = zb_secur_update_key_pair(address,
                                               key,
                                               key_type,
                                               ZB_SECUR_PROVISIONAL_KEY,
                                               ZB_SECUR_KEY_SRC_UNKNOWN);
            TRACE_MSG(TRACE_SECUR1, "Created provisional aps_key %p : key " TRACE_FORMAT_128 " key_type %hd key_attr %hd",
                      (FMT__P_B_D_D, aps_key, TRACE_ARG_128(key), ZB_SECUR_UNIQUE_KEY, ZB_SECUR_PROVISIONAL_KEY));
        }
    }

    return aps_key;
}


zb_aps_device_key_pair_set_t *zb_secur_get_link_key_by_address(zb_ieee_addr_t address,
        zb_secur_key_attributes_t attr)
{
    zb_aps_device_key_pair_set_t key_pair;
    zb_uint_t idx = zb_64bit_hash(address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
    zb_uint_t n = 0;

    TRACE_MSG(TRACE_SECUR1, "zb_secur_get_link_key_by_address: start by index %hd", (FMT__H, idx));

    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
        /* Load keypair by idx. */
        if (zb_aps_keypair_read_by_idx(idx, &key_pair) == RET_OK)
        {
            TRACE_MSG(TRACE_SECUR1, "searching key: try i %hd attr %hd " TRACE_FORMAT_64,
                      (FMT__H_H_A, idx,
                       key_pair.key_attributes, TRACE_ARG_64(key_pair.device_address)));
            TRACE_MSG(TRACE_SECUR1, "searching key: attr %hd source %hd",
                      (FMT__H_H, key_pair.key_attributes, key_pair.key_source));
            if ((ZB_IEEE_ADDR_CMP(key_pair.device_address, address)
                    || ZB_IS_64BIT_ADDR_ZERO(address)) /* zero means "delete any" */
                    && (attr == ZB_SECUR_ANY_KEY_ATTR || attr == key_pair.key_attributes))
            {
#ifdef ZB_CERTIFICATION_HACKS
                if (ZB_CERT_HACKS().enable_alldoors_key)
                {
                    /* Do not want to rewrite at every key
                     * get. Compare key first and write only if changed. */
                    if (ZB_MEMCMP(key_pair.link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE))
                    {
                        /* All aps keys will be the same as default including pre-configured */
                        ZB_MEMCPY(key_pair.link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
                        zb_aps_keypair_write(&key_pair, idx);
                    }
                }
#endif
                TRACE_MSG(TRACE_SECUR3, "Found key #%d attr %hd for " TRACE_FORMAT_64, (FMT__D_H_A, idx, key_pair.key_attributes, TRACE_ARG_64(address)));
                return &ZB_AIB().aps_device_key_pair_storage.cached;
            }
        }

        idx = (idx + 1U) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
    }

    TRACE_MSG(TRACE_SECUR1, "Can't find key for " TRACE_FORMAT_64 " attr %d", (FMT__A_D, TRACE_ARG_64(address), (int)attr));
    return NULL;
}

zb_uint16_t zb_aps_keypair_get_index_by_addr(zb_ieee_addr_t dev_addr,
        zb_secur_key_attributes_t attr)
{
    zb_aps_device_key_pair_set_t *key_pair = zb_secur_get_link_key_by_address(dev_addr, attr);
    if (key_pair != NULL)
    {
        return (zb_uint16_t)ZB_AIB().aps_device_key_pair_storage.cached_i;
    }
    return (zb_uint16_t) -1;
}

void zb_secur_delete_link_key_by_idx(zb_uint16_t idx)
{
    if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
    {
        TRACE_MSG(TRACE_SECUR3, "zb_secur_delete_link_key_by_idx idx %hd", (FMT__D, idx));
        ZB_ASSERT(idx == ZB_AIB().aps_device_key_pair_storage.cached_i);
        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset = 0;
        ZB_BZERO(&ZB_AIB().aps_device_key_pair_storage.cached.device_address, sizeof(zb_ieee_addr_t));
        ZB_AIB().aps_device_key_pair_storage.cached_i = (zb_uint8_t) -1;
#ifndef ZB_NO_CHECK_INCOMING_SECURE_APS_FRAME_COUNTERS
        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter = 0;
#endif
        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].outgoing_frame_counter = 0;
#ifdef ZB_USE_NVRAM
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
    }
    else
    {
        TRACE_MSG(TRACE_SECUR3, "zb_secur_delete_link_key_by_idx: not found", (FMT__0));
    }
}


void zb_secur_delete_link_keys_by_addr_ref(zb_address_ieee_ref_t addr_ref)
{
    zb_ieee_addr_t ieee_address;

    zb_address_ieee_by_ref(ieee_address, addr_ref);
    zb_secur_delete_link_keys_by_long_addr(ieee_address);
}


void zb_secur_delete_link_keys_by_long_addr(zb_ieee_addr_t ieee_address)
{
    zb_uint16_t idx;

    TRACE_MSG(TRACE_SECUR1, "zb_secur_delete_link_keys_by_long_addr addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ieee_address)));

    for (idx = 0; idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ++idx)
    {
        if (zb_aps_keypair_load_by_idx(idx) == RET_OK &&
                ZB_IEEE_ADDR_CMP(ZB_AIB().aps_device_key_pair_storage.cached.device_address, ieee_address))
        {
#if defined ZB_ENABLE_SE
#ifndef ZB_TC_AT_ZC
#error Dont know what to do here!
#endif
            /* SE spec, 5.4.3 Devices  Leaving the Network
               Upon  receipt  of  an  APS  update  device  command  indicating  a  device  has  left  the
               network the trust center shall not remove the trust center link key assigned to that
               device. This is to prevent a device on the network performing a denial of service attack by
               spoofing the MAC address of another node and issuing a  false ZigBee Network Leave command.
               Devices should be removed from Trust Center authorization and trust center link key lists
               via out of band methods, i.e. secure meter back haul or secure IP interface.

               Now do not remove CBKE keys from TC at all - use manual call zb_se_delete_cbke_link_key()
               to remove it separately.
            */
            if (!(ZB_IS_DEVICE_ZC() &&
                    ZB_AIB().aps_device_key_pair_storage.cached.key_attributes == ZB_SECUR_VERIFIED_KEY &&
                    ZB_AIB().aps_device_key_pair_storage.cached.key_source == ZB_SECUR_KEY_SRC_CBKE))
#endif
            {
                TRACE_MSG(
                    TRACE_SECUR2, "Delete key for idx %d attr %hd source %hd",
                    (FMT__D_H_H,
                     idx,
                     ZB_AIB().aps_device_key_pair_storage.cached.key_attributes,
                     ZB_AIB().aps_device_key_pair_storage.cached.key_source));
                TRACE_MSG(TRACE_SECUR2, "key " TRACE_FORMAT_128,
                          (FMT__B, TRACE_ARG_128(ZB_AIB().aps_device_key_pair_storage.cached.link_key)));
                zb_secur_delete_link_key_by_idx(idx);
            }
        }
    }
}


zb_ret_t zb_secur_changing_tc_policy_check()
{
    zb_ret_t status;

    if (!ZB_TCPOL().allow_remote_policy_change)
    {
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_ILLEGAL_REQUEST);
    }
    else if (ZB_TCPOL().use_white_list)
    {
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_NOT_SUPPORTED);
    }
    else
    {
        status = (zb_ret_t)ZB_APS_STATUS_SUCCESS;
    }

    TRACE_MSG(TRACE_SECUR1, "zb_secur_changing_tc_policy_check status %d remote_policy_chng %hd white_list %hd",
              (FMT__D_H_H, status, ZB_AIB().tcpolicy.allow_remote_policy_change, ZB_AIB().tcpolicy.use_white_list));

    return status;
}


zb_bool_t zb_secur_aps_cmd_is_encrypted_by_good_key(zb_uint8_t cmd_id, zb_uint16_t src_addr, zb_uint16_t keypair_i)
{
    zb_bool_t ret = ZB_TRUE;
    zb_ieee_addr_t src_ieee_addr;

    TRACE_MSG(TRACE_SECUR1,
              "zb_secur_aps_cmd_is_encrypted_by_good_key: cmd_id %hd src_addr 0x%x keypair_i %d",
              (FMT__H_D_D, cmd_id, src_addr, keypair_i));

    if (ZG->aps.authenticated)
    {
        if (zb_address_ieee_by_short(src_addr, src_ieee_addr) == RET_OK)
        {
            zb_uint16_t keypair_addr
                = zb_aps_keypair_get_index_by_addr(src_ieee_addr, ZB_SECUR_VERIFIED_KEY);
            if ((keypair_addr != (zb_uint16_t) -1)
                    && (keypair_i == (zb_uint16_t) -1 || keypair_addr != keypair_i))
            {
                /* WWAH: Accept Confirm Key cmd - to obtain new veirified key. */
                /* EE: required even without WWAH to decrypt Confirm key after secured
                 * Rejoin and TCLK update, or any other TCLK update. */
                if (cmd_id == APS_CMD_CONFIRM_KEY)
                {
                    keypair_addr = zb_aps_keypair_get_index_by_addr(src_ieee_addr, ZB_SECUR_UNVERIFIED_KEY);

                    TRACE_MSG(TRACE_SECUR1, "CONFIRM_KEY: additional check for WWAH, ret %hd", (FMT__H, ret));
                    if ((keypair_addr != (zb_uint16_t) -1)
                            && (keypair_i == (zb_uint16_t) -1 || keypair_addr != keypair_i))
                    {
                        ret = ZB_FALSE;
                    }
                }
                else
                {
                    ret = ZB_FALSE;
                }
            }
        }
    }

    TRACE_MSG(TRACE_SECUR1, "cmd allowed %hd", (FMT__H, ret));

    return ret;
}


#ifdef ZB_COORDINATOR_ROLE


static void send_key_switch_alarm(zb_uint8_t param)
{
    if (!param)
    {
        zb_buf_get_out_delayed(send_key_switch_alarm);
    }
    else
    {
        *ZB_BUF_GET_PARAM(param, zb_uint16_t) = ZB_NWK_BROADCAST_ALL_DEVICES;
        ZB_SCHEDULE_CALLBACK(zb_secur_send_nwk_key_switch, param);
    }
}


void zb_secur_nwk_key_switch_procedure(zb_uint8_t param)
{
    zb_uint_t i;

    TRACE_MSG(TRACE_SECUR3, "zb_secur_nwk_key_switch_procedure %hd", (FMT__H, param));

    if (!ZB_IS_TC())
    {
        if (param)
        {
            zb_buf_free(param);
        }
        return;
    }

    if (!param)
    {
        zb_buf_get_out_delayed(zb_secur_nwk_key_switch_procedure);
        return;
    }

    /* Check: maybe, we have next key already? If no, geneate a new key.
     * zb_secur_send_nwk_key_update_br wants key to be alive.
     */
    i = (ZB_NIB().active_secur_material_i + 1) % ZB_SECUR_N_SECUR_MATERIAL;
    if (ZB_NIB().secur_material_set[i].key_seq_number != ZB_NIB().active_key_seq_number + 1
            || secur_nwk_key_is_empty(ZB_NIB().secur_material_set[i].key))
    {
        TRACE_MSG(TRACE_SECUR3, "generating key i %d key# %d", (FMT__D_D, i, ZB_NIB().active_key_seq_number + 1));
        secur_nwk_generate_key(i, ZB_NIB().active_key_seq_number + 1);
    }

    *ZB_BUF_GET_PARAM(param, zb_uint16_t) = ZB_NWK_BROADCAST_ALL_DEVICES;
    ZB_SCHEDULE_CALLBACK(zb_secur_send_nwk_key_update_br, param);
    //zb_schedule_alarm(send_key_switch_alarm, 0, ZB_NWK_OCTETS_TO_BI(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS));
    ZB_SCHEDULE_ALARM(send_key_switch_alarm, 0, ZB_NWK_OCTETS_TO_BI(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS));
}


#endif /* #ifdef ZB_COORDINATOR_ROLE */


#ifndef ZB_COORDINATOR_ONLY
void zdo_initiate_tclk_gen_over_aps(zb_uint8_t param)
{
    TRACE_MSG(TRACE_SECUR1, "zdo_initiate_tclk_gen_over_aps %d", (FMT__H, param));
    /* See BDB Figure 13  Trust Center link key exchange procedure
     * sequence chart and BDB Figure 14 */
    /* Set it to 1 here to prevent situation when bdb_commissioning_machine
       called before bdb_initiate_key_exchange */
    ZB_TCPOL().waiting_for_tclk_exchange = ZB_TRUE;
    /* Wait a bit: we must first send Device_annce according to BDB */
    ZB_SCHEDULE_ALARM(bdb_initiate_key_exchange, param, 30);
}

#endif  /* #ifndef ZB_COORDINATOR_ONLY */

void zb_secur_trace_all_key_pairs(zb_uint8_t param)
{
#if TRACE_ENABLED(TRACE_SECUR4)
    zb_uindex_t n = 0;
    zb_aps_device_key_pair_set_t key_pair;

    TRACE_MSG(TRACE_SECUR4, ">> zb_secur_trace_all_key_pairs", (FMT__0));
    TRACE_MSG(TRACE_SECUR4, "  key attrib: ZB_SECUR_PROVISIONAL_KEY = 0, ZB_SECUR_UNVERIFIED_KEY = 1", (FMT__0));
    TRACE_MSG(TRACE_SECUR4, "              ZB_SECUR_VERIFIED_KEY = 2, ZB_SECUR_APPLICATION_KEY = 3", (FMT__0));
    TRACE_MSG(TRACE_SECUR4, "  key types : ZB_SECUR_UNIQUE_KEY = 0, ZB_SECUR_GLOBAL_KEY = 1", (FMT__0));
    TRACE_MSG(TRACE_SECUR4, "  key source: ZB_SECUR_KEY_SRC_UNKNOWN = 0, ZB_SECUR_KEY_SRC_CBKE = 1", (FMT__0));

    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
        if (zb_aps_keypair_read_by_idx(n, &key_pair) == RET_OK)
        {
            TRACE_MSG(TRACE_SECUR4, "  #%u attrib %u type & src 0x%x address " TRACE_FORMAT_64 " key " TRACE_FORMAT_128,
                      (FMT__D_D_D_A_B, (zb_uint_t)n, (zb_uint_t)key_pair.key_attributes,
                       (zb_uint_t)((key_pair.aps_link_key_type << 8) | key_pair.key_source),
                       TRACE_ARG_64(key_pair.device_address), TRACE_ARG_128(key_pair.link_key)));
        }
        n++;
    }
#endif
    ZVUNUSED(param);
}

/*! @} */
