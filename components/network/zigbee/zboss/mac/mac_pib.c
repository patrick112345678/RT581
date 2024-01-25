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
/* PURPOSE: MAC layer main module
*/

#define ZB_TRACE_FILE_ID 293
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

/*! \addtogroup ZB_MAC */
/*! @{ */


#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_mac_globals.h"


void zb_mac_reinit_pib()
{
    /* Q: Do we really need all that parameters to be
       configurable at runtime? Maybe, use constants instead?

       A: Yes, in general we need all PIB
       attributes configurable, because they could be changed via
       MLME-SET.request. This is a part of mac_layer API. For Zigbee, we
       don't need all these attributes configurable. In Zigbee2007 only
       the following PIB attributes could be changed:
       1. macShortAddress
       2. macAssociationPermit
       3. macAutoRequest
       4. macPANID
       5. macBeaconPayload
       6. macBeaconPayloadLength

       Replacing an attribute with a constant could save ~5-7 bytes of code for
       each occurrance. Currently, ZB_CONFIGURABLE_MAC_PIB in zb_config.h is implemented to
       switch between configurable and const PIB mode
    */


#ifdef ZB_CONFIGURABLE_MAC_PIB
    MAC_PIB().mac_ack_wait_duration = ZB_MAC_PIB_ACK_WAIT_DURATION_CONST;

    /*
      macAckWaitDuration =  aUnitBackoffPeriod + aTurnaroundTime + phySHRDuration + (6 * phySymbolsPerOctet)
    */
    /* this time value should be calculated according to the described
     * formula, but it is too complex, so use estimated predefined value */
    MAC_PIB().mac_max_frame_retries = ZB_MAC_MAX_FRAME_RETRIES;


    /*
      aBaseSlotDuration = 60
      aNumSuperframeSlots = 16
      aBaseSuperframeDuration (symbols) = aBaseSlotDuration * aNumSuperframeSlots
    */
    MAC_PIB().mac_base_superframe_duration = ZB_MAC_BASE_SUPERFRAME_DURATION;

    /* The maximum time (in unit periods) that a transaction is stored by
     * a coordinator and indicated in its beacon. range 0x0000 -
     * 0xffff. unit period = aBaseSuperframeDuration (== beacon interval)*/
    MAC_PIB().mac_transaction_persistence_time = ZB_MAC_TRANSACTION_PERSISTENCE_TIME;

    MAC_PIB().mac_response_wait_time = ZB_MAC_PIB_RESPONSE_WAIT_TIME_CONST;

#endif  /* ZB_CONFIGURABLE_MAC_PIB */

    /*
      macMaxFrameTotalWaitTime = ( SUM (0..m-1) (2 ^ (macMinBE +k)) + (2 ^ macMaxBE - 1)*(macMaxCSMABackoffs - m) ) * aUnitBackoffPeriod + phyMaxFrameDuration

      m is min(macMaxBE-macMinBE, macMaxCSMABackoffs).
      macMaxBE = 3-8
      macMinBE = 0 - macMaxBE
      macMaxCSMABackoffs = 0 - 5
      aUnitBackoffPeriod = 20
      phyMaxFrameDuration = 55, 212, 266, 1064
    */
    /* this time value should be calculated according to the described
     * formula, but it is too complex, so use estimated predefined value */
    MAC_PIB().mac_max_frame_total_wait_time = ZB_MAC_PIB_MAX_FRAME_TOTAL_WAIT_TIME_CONST_2_4_GHZ;


    /* TODO: Should be disabled by IEEE MAC 802.15.4. Before editing, please take care of different
     * cases when it may break join/rejoin logic! */
    ZB_PIB_RX_ON_WHEN_IDLE() = 1;
    MAC_PIB().mac_dsn = ZB_RANDOM_U8();
    MAC_PIB().mac_bsn = ZB_RANDOM_U8();

    /* To receive beacons on UZ set broadcast panid */
    MAC_PIB().mac_pan_id = ZB_BROADCAST_PAN_ID;
    MAC_PIB().mac_short_address = ZB_MAC_SHORT_ADDR_NOT_ALLOCATED;

#if defined ZB_MAC_TESTING_MODE
    MAC_PIB().mac_auto_request = 1;
    MAC_PIB().mac_association_permit = 0;
    MAC_PIB().mac_beacon_payload_length = 0;
    ZB_BZERO(&MAC_PIB().mac_beacon_payload, sizeof(zb_mac_beacon_payload_t));
#endif

#if defined ZB_ENHANCED_BEACON_SUPPORT
    MAC_PIB().mac_ebsn = ZB_RANDOM_U8();
#endif /* ZB_ENHANCED_BEACON_SUPPORT */

#if defined ZB_JOINING_LIST_SUPPORT
    MAC_PIB().mac_ieee_joining_list.length = 0;
    MAC_PIB().mac_joining_policy = ZB_MAC_JOINING_POLICY_ALL_JOIN;
    MAC_PIB().mac_ieee_expiry_interval = 5;
#endif /* ZB_JOINING_LIST_SUPPORT */

#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    zb_mac_duty_cycle_mib_init();
#endif  /* ZB_MAC_DUTY_CYCLE_MONITORING */

    MAC_PIB().phy_current_channel = 0xFF;
    MAC_PIB().phy_current_page = 0xFF;
    MAC_PIB().phy_ephemeral_page = 0xFF;

#ifdef ZB_ENABLE_ZGP_DIRECT
#ifndef ZB_ZGPD_ROLE
    MAC_PIB().zgp_skip_all_packets = ZB_TRUE;
#else
    MAC_PIB().zgp_skip_all_packets = ZB_FALSE;
#endif /* ZB_ZGPD_ROLE */
#endif /* ZB_ENABLE_ZGP_DIRECT */

    TRACE_MSG(TRACE_MAC3,
              "zb_mac_reinit_pib ack_wait_duration %d response_wait_time %d transaction_persistence_time %d",
              (FMT__D_D_D, ZB_MAC_PIB_ACK_WAIT_DURATION, ZB_MAC_PIB_RESPONSE_WAIT_TIME, ZB_MAC_TRANSACTION_PERSISTENCE_TIME));
}


static void mlme_get_8bit(zb_uint8_t param, zb_uint8_t v)
{
    zb_mlme_get_confirm_t *conf;
    conf = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_confirm_t) + sizeof(zb_uint8_t));
    conf->pib_length = (zb_uint8_t)sizeof(zb_uint8_t);
    *(((zb_uint8_t *)conf) + sizeof(zb_mlme_get_confirm_t)) = v;
    conf->pib_index = 0;
}


static void mlme_get_16bit(zb_uint8_t param, zb_uint16_t v)
{
    zb_mlme_get_confirm_t *conf;
    conf = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_confirm_t) + sizeof(zb_uint16_t));
    conf->pib_length = (zb_uint8_t)sizeof(zb_uint16_t);
    ZB_ASSIGN_UINT16(conf + 1, &v);
    conf->pib_index = 0;
}

#if defined ZB_JOINING_LIST_SUPPORT
static void mlme_get_32bit(zb_uint8_t param, zb_uint32_t v)
{
    zb_mlme_get_confirm_t *conf;
    conf = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_confirm_t) + sizeof(zb_uint32_t));
    conf->pib_length = (zb_uint8_t)sizeof(zb_uint32_t);
    ZB_ASSIGN_UINT32(conf + 1, &v);
    conf->pib_index = 0;
}
#endif /* ZB_JOINING_LIST_SUPPORT */

#if defined ZB_MAC_DIAGNOSTICS
static void mlme_get_mac_diag_info(zb_uint8_t param)
{
    zb_mlme_get_confirm_t *conf;
    zb_mac_diagnostic_ex_info_t *diag_info;

    conf = zb_buf_initial_alloc(param,
                                sizeof(zb_mlme_get_confirm_t) + sizeof(zb_mac_diagnostic_info_t));
    conf->pib_length = (zb_uint8_t)sizeof(zb_uint32_t);
    conf->pib_index = 0;

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,1} */
    diag_info = (zb_mac_diagnostic_ex_info_t *)(conf + 1);
    zb_mac_diagnostics_get_info(diag_info);
}
#endif

#if defined ZB_JOINING_LIST_SUPPORT

static void mlme_get_handle_joining_list_req(zb_bufid_t buf,
        zb_mlme_get_request_t *mlme_get_req,
        zb_mac_status_t *status)
{
    /* This attribute requires zb_mlme_get_ieee_joining_list_req_t to be stored
     * right after zb_mlme_get_request_t
     */
    zb_mlme_get_ieee_joining_list_req_t req_params;
    zb_mlme_get_ieee_joining_list_res_t *res_params;
    zb_mlme_get_confirm_t *conf;
    zb_uint8_t fetch_length;
    void *p;

    *status = MAC_SUCCESS;
    ZB_MEMCPY(&req_params, (zb_uint8_t *)zb_buf_begin(buf) + sizeof(*mlme_get_req), sizeof(req_params));

    /* calculate number of elements to be copied */
    fetch_length = 0;
    if (req_params.start_index < MAC_PIB().mac_ieee_joining_list.length)
    {
        fetch_length = MAC_PIB().mac_ieee_joining_list.length - req_params.start_index;
        if (fetch_length > req_params.count)
        {
            fetch_length = req_params.count;
        }
    }
    else if (req_params.start_index != 0U)
    {
        *status = MAC_INVALID_INDEX;
    }
    else
    {
        /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    TRACE_MSG(TRACE_MAC3,
              "mlme_get_handle_joining_list_req start %hd fetch %hd status %hd",
              (FMT__H_H_H, req_params.start_index, fetch_length, *status));

    if (*status == MAC_SUCCESS)
    {
        zb_uint8_t *ptr;

        /* allocate space for mlme-req.confirm and zb_mlme_get_ieee_joining_list_res_t */
        p = zb_buf_initial_alloc(buf, sizeof(*conf) + sizeof(*res_params) + fetch_length * sizeof(zb_ieee_addr_t));
        ptr = (zb_uint8_t *)p;
        conf = (zb_mlme_get_confirm_t *)p;
        conf->pib_length = (zb_uint8_t)zb_buf_len(buf) - (zb_uint8_t)sizeof(*conf);

        res_params = (zb_mlme_get_ieee_joining_list_res_t *)p;
        res_params += (zb_uint8_t)sizeof(*conf);
        res_params->start_index = req_params.start_index;
        res_params->total_length = MAC_PIB().mac_ieee_joining_list.length;
        res_params->addr_count = fetch_length;
        res_params->joining_policy = MAC_PIB().mac_joining_policy;

        if (fetch_length > 0U)
        {
            ZB_MEMCPY(ptr + sizeof(*conf) + sizeof(*res_params),
                      MAC_PIB().mac_ieee_joining_list.items + req_params.start_index,
                      fetch_length * sizeof(zb_ieee_addr_t)
                     );
        }
    }
    else
    {
        /* in case of error we need space for a confirm header only */
        p = zb_buf_initial_alloc(buf, sizeof(*conf));
        conf = (zb_mlme_get_confirm_t *)p;
        conf->pib_length = 0;
    }
}

static zb_mac_status_t mlme_set_joining_list_insert_element(zb_mlme_set_ieee_joining_list_req_t *req_params)
{
    zb_mac_status_t status = MAC_SUCCESS;
    zb_bool_t found = ZB_FALSE;
    zb_uint8_t i;

    if ((MAC_PIB().mac_ieee_joining_list.length + 1U) >= MAC_JOINING_LIST_SIZE_LIMIT)
    {
        status = MAC_INVALID_PARAMETER;
    }
    else
    {
        for (i = 0; i < MAC_PIB().mac_ieee_joining_list.length && !found; i++)
        {
            if (ZB_64BIT_ADDR_CMP(&req_params->param.ieee_value, &MAC_PIB().mac_ieee_joining_list.items[i]))
            {
                found = ZB_TRUE;
            }
        }

        if (!found)
        {
            ZB_MEMCPY(&MAC_PIB().mac_ieee_joining_list.items[MAC_PIB().mac_ieee_joining_list.length],
                      &req_params->param.ieee_value,
                      sizeof(zb_ieee_addr_t));
            MAC_PIB().mac_ieee_joining_list.length++;
        }

        TRACE_MSG(TRACE_MAC3,
                  "mlme_set_joining_list_insert_element found %hd new len %hd",
                  (FMT__H_H, found, MAC_PIB().mac_ieee_joining_list.length));
    }

    return status;
}


static void mlme_set_joining_list_erase_element(zb_mlme_set_ieee_joining_list_req_t *req_params)
{
    zb_uint8_t i;

    for (i = 0; i < MAC_PIB().mac_ieee_joining_list.length; i++)
    {
        if (!ZB_64BIT_ADDR_CMP(&req_params->param.ieee_value, &MAC_PIB().mac_ieee_joining_list.items[i]))
        {
            continue;
        }

        if (i != (MAC_PIB().mac_ieee_joining_list.length - 1U))
        {
            zb_uint8_t move_len = (MAC_PIB().mac_ieee_joining_list.length - i - 1U) * (zb_uint8_t)sizeof(zb_ieee_addr_t);
            ZB_MEMMOVE(&MAC_PIB().mac_ieee_joining_list.items[i],
                       &MAC_PIB().mac_ieee_joining_list.items[i + 1U],
                       move_len);
        }

        MAC_PIB().mac_ieee_joining_list.length--;
        break;
    }
}

static zb_mac_status_t mlme_set_joining_list_range(zb_mlme_set_ieee_joining_list_range_t *req_params,
        zb_uint8_t *ptr)
{
    zb_mac_status_t status = MAC_SUCCESS;
    zb_uint8_t end_length = req_params->start_index + req_params->items_count;

    if (end_length > MAC_JOINING_LIST_SIZE_LIMIT
            || req_params->list_size > MAC_JOINING_LIST_SIZE_LIMIT)
    {
        status = MAC_INVALID_PARAMETER;
    }

    if (status == MAC_SUCCESS && end_length > req_params->list_size)
    {
        status = MAC_INVALID_PARAMETER;
    }

    if (status == MAC_SUCCESS)
    {
        if (req_params->list_size > MAC_PIB().mac_ieee_joining_list.length)
        {
            zb_size_t list_len = ((zb_size_t)req_params->list_size - (zb_size_t)MAC_PIB().mac_ieee_joining_list.length);
            list_len *= sizeof(zb_ieee_addr_t);
            /* enlarged list size => clearing it first in case of holes */
            ZB_MEMSET(MAC_PIB().mac_ieee_joining_list.items + MAC_PIB().mac_ieee_joining_list.length,
                      0xFF,
                      list_len);
        }

        ZB_MEMCPY(MAC_PIB().mac_ieee_joining_list.items + req_params->start_index,
                  ptr,
                  sizeof(zb_ieee_addr_t) * req_params->items_count);

        MAC_PIB().mac_ieee_joining_list.length = req_params->list_size;
        MAC_PIB().mac_joining_policy = req_params->joining_policy;
    }
    return status;
}

static zb_mac_status_t mlme_set_handle_joining_list_req(zb_bufid_t buf, void *ptr)
{
    zb_mac_status_t status = MAC_SUCCESS;
    zb_mlme_set_ieee_joining_list_req_t *req_params = (zb_mlme_set_ieee_joining_list_req_t *)ptr;

    /* zb_mlme_set_ieee_joining_list_req_t header is expected */
    if (zb_buf_len(buf) < sizeof(zb_mlme_set_ieee_joining_list_req_t))
    {
        status = MAC_INVALID_PARAMETER;
    }
    else
    {
        switch ((zb_mlme_set_ieee_joining_list_req_type_t)req_params->op_type)
        {
        case ZB_MLME_SET_IEEE_JL_REQ_RANGE:
        {
            zb_uint8_t *p = (zb_uint8_t *)ptr;
            p += sizeof(req_params);
            ZB_ASSERT(req_params->param.range_params.items_count * sizeof(zb_ieee_addr_t)
                      <= ((zb_uint32_t)(zb_uint8_t *)zb_buf_begin(buf) + zb_buf_len(buf) - (zb_uint32_t)p));
            status = mlme_set_joining_list_range(&req_params->param.range_params, p);
            break;
        }

        case ZB_MLME_SET_IEEE_JL_REQ_INSERT:
            status = mlme_set_joining_list_insert_element(req_params);
            break;

        case ZB_MLME_SET_IEEE_JL_REQ_ERASE:
            mlme_set_joining_list_erase_element(req_params);
            break;

        case ZB_MLME_SET_IEEE_JL_REQ_CLEAR:
            MAC_PIB().mac_ieee_joining_list.length = 0U;
            MAC_PIB().mac_joining_policy = req_params->param.clear_params.joining_policy;
            break;

        default:
            ZB_ASSERT(0);
            break;
        }
    }

    return status;
}
#endif /* ZB_JOINING_LIST_SUPPORT */

void zb_mlme_get_request(zb_uint8_t param)
{
    zb_mac_status_t status = MAC_SUCCESS;
    zb_mlme_get_request_t req;
    zb_mlme_get_confirm_t *conf = NULL;

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_get_request(param);
#endif

    TRACE_MSG(TRACE_MAC2, ">>zb_mlme_get_req %d", (FMT__D, param));
    ZB_MEMCPY(&req, (zb_mlme_get_request_t *) zb_buf_begin(param),
              sizeof(req));

    TRACE_MSG(TRACE_MAC2, "zb_mlme_get_req: pib attr 0x%x, pib index 0x%x, cb %p", (FMT__H_H_P, req.pib_attr, req.pib_index, req.confirm_cb_u));

    /* Historically that code is in form if-else... to bypass Keil glitch
     * with big switch statement at 8051. Seems, now it is not actuall, so rewrite
     * it to use switch! */
    switch (req.pib_attr)
    {
    case ZB_PHY_PIB_CURRENT_CHANNEL:
        mlme_get_8bit(param, MAC_PIB().phy_current_channel);
        break;
#ifndef ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PHY_PIB_CURRENT_PAGE:
        mlme_get_8bit(param, MAC_PIB().phy_current_page);
        break;
    case ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION:
        /* Note that ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION impacts ONLY nsng and ZB_AUTO_ACK_TX case */
        ZB_ASSERT(ZB_MAC_PIB_ACK_WAIT_DURATION <= ZB_UINT16_MAX);
        mlme_get_16bit(param, (zb_uint16_t)ZB_MAC_PIB_ACK_WAIT_DURATION);
        break;
#endif  /* ZB_LITE_LIMIT_PIB_ACCESS */
    case ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT:
        mlme_get_8bit(param, MAC_PIB().mac_association_permit);
        break;
#ifndef ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PIB_ATTRIBUTE_BATT_LIFE_EXT:
        mlme_get_8bit(param, MAC_PIB().mac_batt_life_ext);
        break;
#endif
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD:
        conf = zb_buf_initial_alloc(param,
                                    sizeof(zb_mlme_get_confirm_t) +
                                    sizeof(ZB_PIB_BEACON_PAYLOAD()));
        conf->pib_length = MAC_PIB().mac_beacon_payload_length;
        ZB_MEMCPY((((zb_uint8_t *) conf) + sizeof(zb_mlme_get_confirm_t)),
                  &ZB_PIB_BEACON_PAYLOAD (), sizeof(zb_mac_beacon_payload_t));
        conf->pib_index = 0;
        TRACE_MSG(TRACE_MAC3, "get payld  " TRACE_FORMAT_64, (FMT__A,
                  TRACE_ARG_64
                  (ZB_PIB_BEACON_PAYLOAD
                   ().
                   extended_panid)));
        break;
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH:
        mlme_get_8bit(param, MAC_PIB().mac_beacon_payload_length);
        break;
#ifndef ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PIB_ATTRIBUTE_BEACON_ORDER:
        mlme_get_8bit(param, MAC_PIB().mac_beacon_order);
        break;
#endif
    case ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS:
        conf = zb_buf_initial_alloc(param,
                                    sizeof(zb_mlme_get_confirm_t) +
                                    sizeof(zb_ieee_addr_t));
        ZB_IEEE_ADDR_COPY (((zb_uint8_t *) (conf + 1)),
                           ZB_PIB_EXTENDED_ADDRESS ());
        TRACE_MSG(TRACE_MAC2, "addr get   " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64 (ZB_PIB_EXTENDED_ADDRESS ())));
        conf->pib_index = 0;
        conf->pib_length = (zb_uint8_t)sizeof(zb_ieee_addr_t);
        break;
#if !defined ZB_LITE_LIMIT_PIB_ACCESS || defined ZB_MAC_TESTING_MODE
    case ZB_PIB_ATTRIBUTE_BSN:
        mlme_get_8bit(param, MAC_PIB().mac_bsn);
        break;
    case ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS:
        mlme_get_16bit(param, MAC_PIB().mac_coord_short_address);
        break;
    case ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS:
        conf = zb_buf_initial_alloc(param,
                                    sizeof(zb_mlme_get_confirm_t) +
                                    sizeof(zb_ieee_addr_t));
        ZB_IEEE_ADDR_COPY (((zb_uint8_t *) (conf + 1)),
                           MAC_PIB().mac_coord_extended_address);
        TRACE_MSG(TRACE_MAC3, "coord addr get   " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64 (MAC_PIB().mac_coord_extended_address)));
        conf->pib_index = 0;
        conf->pib_length = (zb_uint8_t)sizeof(zb_ieee_addr_t);
        break;
    case ZB_PIB_ATTRIBUTE_DSN:
        mlme_get_8bit(param, MAC_PIB().mac_dsn);
        break;
#endif  /* !defined ZB_LITE_LIMIT_PIB_ACCESS || defined ZB_MAC_TESTING_MODE*/
    case ZB_PIB_ATTRIBUTE_PANID:
        mlme_get_16bit(param, MAC_PIB().mac_pan_id);
        break;
#ifdef ZB_PROMISCUOUS_MODE
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE:
        mlme_get_8bit(param, MAC_PIB().mac_promiscuous_mode);
        break;
#endif
    case ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE:
        mlme_get_8bit(param, MAC_PIB().mac_rx_on_when_idle);
        break;
    case ZB_PIB_ATTRIBUTE_SHORT_ADDRESS:
        mlme_get_16bit(param, MAC_PIB().mac_short_address);
        break;
    case ZB_PIB_ATTRIBUTE_SUPER_FRAME_ORDER:
        mlme_get_16bit(param, MAC_PIB().mac_superframe_order);
        break;
#ifndef ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PIB_ATTRIBUTE_TRANSACTION_PERSISTENCE_TIME:
        mlme_get_16bit(param, ZB_MAC_PIB_TRANSACTION_PERSISTENCE_TIME);
        break;
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_TOTAL_WAIT_TIME:
        ZB_ASSERT(MAC_PIB().mac_max_frame_total_wait_time <= ZB_UINT16_MAX);
        mlme_get_16bit(param, (zb_uint16_t)MAC_PIB().mac_max_frame_total_wait_time);
        break;
#ifdef ZB_CONFIGURABLE_MAC_PIB
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_RETRIES:
        mlme_get_8bit(param, MAC_PIB().mac_max_frame_retries);
        break;
#endif
    case ZB_PIB_ATTRIBUTE_RESPONSE_WAIT_TIME:
        mlme_get_8bit(param, ZB_MAC_PIB_RESPONSE_WAIT_TIME);
        break;
#endif  /* #ifndef ZB_LITE_LIMIT_PIB_ACCESS */
#ifdef ZB_MAC_TESTING_MODE
    case ZB_PIB_ATTRIBUTE_NO_AUTO_ACK:
        mlme_get_8bit(param, MAC_PIB().mac_no_auto_ack);
        break;
#endif
#if defined ZB_JOINING_LIST_SUPPORT
    case ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST:
        mlme_get_handle_joining_list_req(param, &req, &status);
        break;
    case ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST_LENGTH:
        mlme_get_8bit(param, MAC_PIB().mac_ieee_joining_list.length);
        break;
    case ZB_PIB_ATTRIBUTE_JOINING_POLICY:
        mlme_get_8bit(param, (zb_uint8_t) MAC_PIB().mac_joining_policy);
        break;
    case ZB_PIB_ATTRIBUTE_IEEE_EXPIRY_INTERVAL:
        mlme_get_32bit(param, MAC_PIB().mac_ieee_expiry_interval);
        break;
#endif /* ZB_JOINING_LIST_SUPPORT */
#if defined ZB_MAC_DIAGNOSTICS
    case ZB_PIB_ATTRIBUTE_IEEE_DIAGNOSTIC_INFO:
        mlme_get_mac_diag_info(param);
        break;
    case ZB_PIB_ATTRIBUTE_GET_AND_CLEANUP_DIAG_INFO:
        mlme_get_mac_diag_info(param);
        zb_mac_diagnostics_cleanup_info();
        break;
#endif /* ZB_MAC_DIAGNOSTICS */
    default:
        TRACE_MSG(TRACE_MAC2, "zb_mlme_get_req: unsupported attr", (FMT__0));
        conf = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_confirm_t));
        status = MAC_UNSUPPORTED_ATTRIBUTE;
        conf->pib_length = 0;
        break;
    }

    if (conf == NULL)
    {
        conf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
    }
    conf->pib_attr = req.pib_attr;
    conf->status = status;
#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_get_confirm(param);
#endif

#ifndef MAC_DIRECT_PIB_ACCESS
    if (req.confirm_cb_u.cb != NULL)
    {
        ZB_SCHEDULE_CALLBACK(req.confirm_cb_u.cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
#else
    ZB_SCHEDULE_CALLBACK(zb_mlme_get_confirm, param);
#endif /* ifndef MAC_DIRECT_PIB_ACCESS */
    TRACE_MSG(TRACE_MAC2, "<<zb_mlme_get_req %d", (FMT__D, param));
}


#ifdef MAC_DIRECT_PIB_ACCESS
/* zb_mlme_get_confirm must be somewhere in the upper layer. */
void zb_mlme_get_confirm(zb_uint8_t param)
{
    zb_mlme_get_confirm_t *conf = NULL;

    TRACE_MSG(TRACE_MAC2, ">>zb_mlme_get_confirm %d", (FMT__D, param));
    conf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

    if ((conf->pib_attr == ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH)
            || (conf->pib_attr == ZB_PIB_ATTRIBUTE_BEACON_ORDER)
            || (conf->pib_attr == ZB_PIB_ATTRIBUTE_BSN))
    {
        ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_mlme_get_confirm_t) + sizeof(zb_uint8_t), conf);
        conf->pib_length = sizeof(zb_uint8_t);
        *(((zb_uint8_t *)conf) + sizeof(zb_mlme_get_confirm_t)) = MAC_PIB().mac_beacon_payload_length;
        conf->pib_index = 0;
        TRACE_MSG(TRACE_NWK2, "attr %hd get param %hd", (FMT__H_H, conf->pib_attr, *(((zb_uint8_t *)conf) + sizeof(zb_mlme_get_confirm_t))));
    }

    TRACE_MSG(TRACE_MAC2, "<<zb_mlme_get_confirm status %hd", (FMT__H, conf->status));
    zb_buf_free(param);
}
#endif

void zb_mlme_set_request_sync(zb_uint8_t param);

void zb_mlme_set_request(zb_uint8_t param)
{
    zb_mlme_set_request_t *req = (zb_mlme_set_request_t *)zb_buf_begin(param);

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_set_request(param);
#endif

    switch (req->pib_attr)
    {
    case ZB_PHY_PIB_CURRENT_CHANNEL:
    case ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS:
    case ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS:
    case ZB_PIB_ATTRIBUTE_PANID:
    case ZB_PIB_ATTRIBUTE_SHORT_ADDRESS:
    case ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS:
    case ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE:
#ifdef ZB_PROMISCUOUS_MODE
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE:
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON:
#endif
        TRACE_MSG(TRACE_MAC1, "zb_mlme_set_request via tx cb %hd attr %d", (FMT__H_D, param, req->pib_attr));
        if (ZB_SCHEDULE_TX_CB(zb_mlme_set_request_sync, param) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Oops! No place for zb_mlme_set_request in tx queue", (FMT__0));
            zb_buf_free(param);
        }
        break;
    default:
        zb_mlme_set_request_sync(param);
        break;
    }
}

void zb_mlme_set_request_sync(zb_uint8_t param)
{
    zb_mac_status_t status = MAC_SUCCESS;
    zb_mlme_set_request_t *req = zb_buf_begin(param);
    zb_uint8_t *ptr = (zb_uint8_t *)req;
    zb_mlme_set_confirm_t conf;
    zb_mlme_set_confirm_t *conf_p;
#ifndef MAC_DIRECT_PIB_ACCESS
    zb_callback_t confirm_cb;
#endif

    TRACE_MSG(TRACE_MAC1, ">>zb_mlme_set_request_sync %hd", (FMT__H, param));

    ZB_BZERO(&conf, sizeof(conf));
    conf.status = status;
    conf.pib_attr = req->pib_attr;
    conf.pib_index = 0;
    ptr += sizeof(zb_mlme_set_request_t);
    switch (req->pib_attr)
    {
    case ZB_PHY_PIB_CURRENT_CHANNEL:
        /* NOTE: on sub-gig radio must set current page first! */
        // comment it out and set in nwk    ZB_PIBCACHE_CURRENT_CHANNEL() = *ptr;

        ZB_ASSERT(MAC_PIB().phy_ephemeral_page != ZB_MAC_INVALID_LOGICAL_PAGE);
        zb_mac_change_channel(MAC_PIB().phy_ephemeral_page, *ptr);
        break;
    case ZB_PHY_PIB_CURRENT_PAGE:
        MAC_PIB().phy_ephemeral_page = *ptr;
        TRACE_MSG(TRACE_MAC1, "set page %hd", (FMT__H, MAC_PIB().phy_ephemeral_page));
        break;
#if defined ZB_CONFIGURABLE_MAC_PIB && !defined ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_TOTAL_WAIT_TIME:
        ZB_ASSIGN_UINT16(&MAC_PIB().mac_max_frame_total_wait_time, ptr);
        break;
    case ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION:
        /* Note that ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION impacts ONLY nsng and ZB_AUTO_ACK_TX case */
        ZB_ASSIGN_UINT16(&MAC_PIB().mac_ack_wait_duration, ptr);
        break;
    case ZB_PIB_ATTRIBUTE_TRANSACTION_PERSISTENCE_TIME:
        ZB_ASSIGN_UINT16(&MAC_PIB().mac_transaction_persistence_time, ptr);
        break;
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_RETRIES:
        MAC_PIB().mac_max_frame_retries = *ptr;
        /* not necessary to update. max_frame_retries will be used during packet send */
        break;
    case ZB_PIB_ATTRIBUTE_RESPONSE_WAIT_TIME:
        MAC_PIB().mac_response_wait_time = *ptr;
        break;
#endif  /* ZB_CONFIGURABLE_MAC_PIB && !ZB_LITE_LIMIT_PIB_ACCESS*/
    case ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT:
        MAC_PIB().mac_association_permit = *ptr;
        break;
#ifndef ZB_LITE_LIMIT_PIB_ACCESS
    case ZB_PIB_ATTRIBUTE_BATT_LIFE_EXT:
        MAC_PIB().mac_batt_life_ext = *ptr;
        break;
#endif
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD:
        ZB_MEMCPY(&ZB_PIB_BEACON_PAYLOAD (), ptr,
                  sizeof(zb_mac_beacon_payload_t));
        TRACE_MSG(TRACE_MAC3, "set payld  " TRACE_FORMAT_64,
                  (FMT__A,
                   TRACE_ARG_64 (ZB_PIB_BEACON_PAYLOAD ().extended_panid)));
        break;
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH:
        MAC_PIB().mac_beacon_payload_length = *ptr;
        break;
    case ZB_PIB_ATTRIBUTE_BEACON_ORDER:
        MAC_PIB().mac_beacon_order = *ptr;
        break;
    case ZB_PIB_ATTRIBUTE_BSN:
        ZB_MAC_BSN() = *ptr;
        break;
#if !defined ZB_LITE_LIMIT_PIB_ACCESS || defined ZB_MAC_TESTING_MODE
    case ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS:
        ZB_ASSIGN_UINT16(&ZB_PIB_COORD_SHORT_ADDRESS(), ptr);
        ZB_TRANSCEIVER_SET_COORD_SHORT_ADDR(ZB_PIB_COORD_SHORT_ADDRESS());
        break;
    case ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS :
        ZB_IEEE_ADDR_COPY(MAC_PIB().mac_coord_extended_address, (zb_uint8_t *)ptr);
        ZB_TRANSCEIVER_SET_COORD_EXT_ADDR(MAC_PIB().mac_coord_extended_address);
        break;
    case ZB_PIB_ATTRIBUTE_DSN:
        ZB_MAC_DSN() = *ptr;
        break;
#endif  /* !defined ZB_LITE_LIMIT_PIB_ACCESS || defined ZB_MAC_TESTING_MODE */
    case ZB_PIB_ATTRIBUTE_PANID:
        ZB_ASSIGN_UINT16(&ZB_PIB_SHORT_PAN_ID (), ptr);
        ZB_TRANSCEIVER_UPDATE_PAN_ID ();
        break;
#ifdef ZB_PROMISCUOUS_MODE
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE:
        if (MAC_PIB().mac_promiscuous_mode != *ptr)
        {
            MAC_PIB().mac_promiscuous_mode = *ptr;
            ZB_TRANSCEIVER_SET_PROMISCUOUS(MAC_PIB().mac_promiscuous_mode);
        }
        break;
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON:
        ZB_TRANSCEIVER_SET_RX_ON_OFF(*ptr);
        break;
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE_CB:
        ZB_MEMCPY(&MAC_PIB().mac_promiscuous_mode_cb, ptr, sizeof(zb_callback_t));
        break;
#endif
    case ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE:
        ZB_PIB_RX_ON_WHEN_IDLE() = *ptr;
        if (MAC_PIB().phy_current_page != ZB_MAC_INVALID_LOGICAL_PAGE)
        {
            ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
        }
        break;
    case ZB_PIB_ATTRIBUTE_SHORT_ADDRESS:
        ZB_ASSIGN_UINT16(&ZB_PIB_SHORT_ADDRESS(), ptr);
        TRACE_MSG(TRACE_MAC3, "set SHORT_ADDRESS %x", (FMT__D, ZB_PIB_SHORT_ADDRESS()));
        ZB_TRANSCEIVER_UPDATE_SHORT_ADDR();
        break;
    case ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS:
        ZB_IEEE_ADDR_COPY(MAC_PIB().mac_extended_address, (zb_uint8_t *)ptr);
        TRACE_MSG(TRACE_MAC3, "addr set   " TRACE_FORMAT_64, (FMT__A,
                  TRACE_ARG_64(MAC_PIB().mac_extended_address)));
        ZB_TRANSCEIVER_UPDATE_LONGMAC();
        break;
#ifdef ZB_MAC_TESTING_MODE
    case ZB_PIB_ATTRIBUTE_AUTO_REQUEST:
        MAC_PIB().mac_auto_request = *ptr;
        TRACE_MSG(TRACE_MAC3, "set Auto Request %hd", (FMT__H, MAC_PIB().mac_auto_request));
        break;
    case ZB_PIB_ATTRIBUTE_NO_AUTO_ACK:
        MAC_PIB().mac_no_auto_ack = *ptr;
        TRACE_MSG(TRACE_MAC3, "set No Auto ack %hd", (FMT__H, MAC_PIB().mac_no_auto_ack));
        break;
#endif
#if defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
    case ZB_PIB_ATTRIBUTE_SRC_MATCH:
        TRACE_MSG(TRACE_MAC3, "src match command %hd",
                  (FMT__H, ((zb_mac_src_match_params_t *)ptr)->cmd));
        /*cstat !MISRAC2012-Rule-11.3 */
        /** @mdr{00002,3} */
        status = (zb_mac_status_t)zb_mac_src_match_process_cmd((zb_mac_src_match_params_t *)ptr);
        break;
#endif  /* defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING */
#if defined ZB_JOINING_LIST_SUPPORT
    case ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST:
        status = mlme_set_handle_joining_list_req(param, ptr);
        break;
    case ZB_PIB_ATTRIBUTE_JOINING_POLICY:
        MAC_PIB().mac_joining_policy = *ptr;
        TRACE_MSG(TRACE_MAC3, "set joining policy %hd", (FMT__H, MAC_PIB().mac_joining_policy));
        break;
    case ZB_PIB_ATTRIBUTE_IEEE_EXPIRY_INTERVAL:
        ZB_ASSIGN_UINT32(&MAC_PIB().mac_ieee_expiry_interval, ptr);
        MAC_PIB().mac_joining_policy = (zb_mac_joining_policy_t)(*ptr);
        TRACE_MSG(TRACE_MAC3, "set ieee expiry interval %d", (FMT__D, MAC_PIB().mac_ieee_expiry_interval));
        break;
#endif /* ZB_JOINING_LIST_SUPPORT */
#ifdef ZB_ENABLE_ZGP_DIRECT
    case ZB_PIB_ATTRIBUTE_SKIP_ALL_GPF:
        MAC_PIB().zgp_skip_all_packets = *ptr;
        TRACE_MSG(TRACE_MAC3, "set zgp_skip_all_packets %d", (FMT__H, *ptr));
        break;
#endif
    default:
        status = MAC_UNSUPPORTED_ATTRIBUTE;
        break;
    }
    conf.status = status;
#ifndef MAC_DIRECT_PIB_ACCESS
    confirm_cb = req->confirm_cb_u.cb;
#endif
    conf_p = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_confirm_t));
    ZB_MEMCPY(conf_p, &conf, sizeof(*conf_p));

#if defined ZB_MAC_API_TRACE_PRIMITIVES
    zb_mac_api_trace_set_confirm(param);
#endif

#ifndef MAC_DIRECT_PIB_ACCESS
    if (confirm_cb != NULL)
    {
        ZB_SCHEDULE_CALLBACK(confirm_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
#else
    ZB_SCHEDULE_CALLBACK(zb_mlme_set_confirm, param);
#endif /* ifndef MAC_DIRECT_PIB_ACCESS */
    TRACE_MSG(TRACE_MAC2, "<<zb_mlme_set_req %d %d", (FMT__H, param, status));
}

/*! @} */

static void zb_set_mac_transaction_persistence_time_delayed2(zb_uint8_t param, zb_uint16_t new_value)
{
    zb_bufid_t buf = param;

    TRACE_MSG(TRACE_MAC3, ">>zb_set_mac_transaction_persistence_time_delayed2", (FMT__0));
    TRACE_MSG(TRACE_MAC3, "buf %p, param %hd, new_value %d ms", (FMT__P_H_D, buf, param, new_value));

    if (buf != 0U)
    {
        zb_mlme_set_request_t *req = (zb_mlme_set_request_t *)zb_buf_begin(buf);
        zb_uint8_t *pib_attr_value = (zb_uint8_t *)req + sizeof(zb_mlme_set_request_t);

        req->pib_attr = ZB_PIB_ATTRIBUTE_TRANSACTION_PERSISTENCE_TIME;
        req->pib_index = 0;
        req->pib_length = (zb_uint8_t)sizeof(zb_uint16_t);
        req->confirm_cb_u.cb = NULL;

        ZB_ASSERT(ZB_MILLISECONDS_TO_BEACON_INTERVAL(new_value) <= ZB_UINT16_MAX);
        new_value = (zb_uint16_t)ZB_MILLISECONDS_TO_BEACON_INTERVAL(new_value);
        ZB_MEMCPY(pib_attr_value, &new_value, sizeof(zb_uint16_t));

        ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Invalid buf ptr!", (FMT__0));
    }

    TRACE_MSG(TRACE_MAC3, "<<zb_set_mac_transaction_persistence_time_delayed2", (FMT__0));
}

void zb_set_mac_transaction_persistence_time(zb_uint16_t ms)
{
    zb_ret_t ret;

    ret = zb_buf_get_in_delayed_ext(zb_set_mac_transaction_persistence_time_delayed2, ms, 0);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_in_delayed_ext [%d]", (FMT__D, ret));
    }
}

#endif /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST*/
