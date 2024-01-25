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
/* PURPOSE: WWAH Beacon Survey routines
*/

#define ZB_TRACE_FILE_ID 12085

#include "zb_common.h"
#include "zb_nwk.h"
#include "zdo_wwah_survey_beacons.h"
#include "zdo_wwah_parent_classification.h"

#if defined ZB_BEACON_SURVEY && defined ZB_ZCL_ENABLE_WWAH_SERVER

static zb_ret_t zdo_wwah_fill_survey_beacon(
    zb_mac_beacon_notify_indication_t *beacon_noti_ind_ptr,
    zb_mac_beacon_payload_t *beacon_payload_ptr,
    zb_zcl_wwah_beacon_survey_t *beacon_survey_ptr);

static zb_zcl_wwah_beacon_survey_t *zdo_wwah_find_survey_beacon(
    zb_uint8_t beacons_number, zb_zcl_wwah_beacon_survey_t *first_sb_ptr, zb_bool_t find_best);

static zb_bool_t zdo_wwah_compare_survey_beacons(
    zb_zcl_wwah_beacon_survey_t *first_ptr, zb_zcl_wwah_beacon_survey_t *second_ptr);

/****************************************************************************/

zb_ret_t zdo_wwah_start_survey_beacons(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_bufid_t buf_beacon_survey_resp;
    zb_zdo_beacon_survey_configuration_t *conf =
        ZB_BUF_GET_PARAM(param, zb_zdo_beacon_survey_configuration_t);

    TRACE_MSG(TRACE_ZDO1, ">>zdo_wwah_start_survey_beacons, param %hd", (FMT__H, param));

    if (!ZB_JOINED())
    {
        ret = RET_ERROR;
        TRACE_MSG(TRACE_ERROR, "We are not joined in a network!", (FMT__0));
    }

    if (RET_OK == ret && ZB_NLME_STATE_IDLE != ZG->nwk.handle.state)
    {
        ret = RET_ERROR;
        TRACE_MSG(TRACE_ERROR, "Invalid NWK state!", (FMT__0));
    }

    if (RET_OK == ret &&
            ACTIVE_SCAN != conf->params.scan_type)
    {
        ret = RET_ERROR;
        TRACE_MSG(TRACE_ZDO1, "enhanced beacons aren't supported", (FMT__0));
    }

    if (RET_OK == ret)
    {
        buf_beacon_survey_resp = zb_buf_get_out();
        if (!buf_beacon_survey_resp)
        {
            TRACE_MSG(TRACE_ERROR, "could not get buffer", (FMT__0));
            ret = RET_ERROR;
        }
        else
        {
            HUBS_CTX().survey_beacons_resp_ref = buf_beacon_survey_resp;
            TRACE_MSG(TRACE_ZDO1, "buf by ref %hd used to send resp", (FMT__H, buf_beacon_survey_resp));
        }
    }

    if (RET_OK == ret)
    {
        zb_zdo_beacon_survey_resp_params_t *resp_params;

        TRACE_MSG(
            TRACE_ZDO1,
            "scan params: type 0x%hx, page 0x%hx, channel 0x%lx",
            (FMT__H_H_L, conf->params.scan_type, conf->params.channel_page, conf->params.channel_mask));

        resp_params =
            zb_buf_get_tail(
                buf_beacon_survey_resp,
                (sizeof(zb_zdo_beacon_survey_resp_params_t)
                 + ZDO_WWAH_MAX_BEACON_SURVEY_BYTES()));

        ZB_BZERO(resp_params,
                 sizeof(zb_zdo_beacon_survey_resp_params_t) + ZDO_WWAH_MAX_BEACON_SURVEY_BYTES());

        /* it need to compatibility with r23 */
        resp_params->configuration.params.channel_mask = conf->params.channel_mask;
        resp_params->configuration.params.channel_page = conf->params.channel_page;
        resp_params->configuration.params.scan_type = conf->params.scan_type;

        resp_params->parents.parents_info_ptr = (zb_uint8_t *)(resp_params + 1);

#ifdef ZB_ROUTER_ROLE
        /* Short address of the our parent (0xffff for Routers)
         * see r23 spec, I.3.5 Potential Parents TLV
         */
        resp_params->parents.current_parent = 0xffff;
#else
        {
            zb_uint16_t current_parent;
            zb_address_short_by_ref(&current_parent, ZG->nwk.handle.parent);
            resp_params->parents.current_parent = current_parent;
        }
#endif

        ZG->nwk.handle.state = ZB_NLME_STATE_SURVEY_BEACON;
        ZB_SCHEDULE_CALLBACK(zb_nlme_beacon_survey_scan, param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zdo_wwah_start_survey_beacons, ret 0x%lx", (FMT__L, ret));

    return ret;
}


/* TODO: Looks like it is better to aggregate beacon survery information
 * in zb_mlme_beacon_notify_indication(), not in WWAH-specific code.
 * Rationale:
 * - beacon handling is not a part of ZDO
 * - this may be reused for R23, as only WWAH-specific part of these is sending back ZCL WWAH cmd.
 * Let's discuss it when R23 beacon survey implementation will be started. */
void zdo_wwah_process_beacon_info(
    zb_mac_beacon_notify_indication_t *beacon_noti_ind_ptr,
    zb_mac_beacon_payload_t *beacon_payload_ptr)
{
    zb_zdo_beacon_survey_resp_params_t *resp_params;
    zb_zcl_wwah_beacon_survey_t new_beacon_survey = { 0 };

    TRACE_MSG(TRACE_ZDO1, ">> zdo_wwah_process_beacon_info, resp_ref %hd",
              (FMT__H, HUBS_CTX().survey_beacons_resp_ref));

    ZB_ASSERT(beacon_noti_ind_ptr);
    ZB_ASSERT(beacon_payload_ptr);

    resp_params =
        zb_buf_get_tail(
            HUBS_CTX().survey_beacons_resp_ref,
            (sizeof(zb_zdo_beacon_survey_resp_params_t)
             + ZDO_WWAH_MAX_BEACON_SURVEY_BYTES()));

    if (beacon_payload_ptr->end_device_capacity > 0)
    {
        ++(resp_params->results.num_potential_parents_current_zbn);
    }
    ++(resp_params->results.total_beacons_surveyed);

    /* TODO: to implement this counters need
       R23 - 3.6.1.4 Joining or Rejoining a Network - nwkDiscoveryTable */
    /* ++(resp_params->results.num_non_zbn); */
    /* ++(resp_params->results.num_zbn); */

    if (zdo_wwah_fill_survey_beacon(
                beacon_noti_ind_ptr,
                beacon_payload_ptr,
                &new_beacon_survey) == RET_OK)
    {
        if (resp_params->parents.count_potential_parents
                < ZDO_WWAH_MAX_BEACON_SURVEY())
        {
            ZB_MEMCPY(
                (resp_params->parents.parents_info_ptr
                 + resp_params->parents.count_potential_parents * sizeof(zb_zcl_wwah_beacon_survey_t)),
                &new_beacon_survey,
                sizeof(zb_zcl_wwah_beacon_survey_t));
            ++(resp_params->parents.count_potential_parents);

            TRACE_MSG(TRACE_ZDO1, "new survey_beacon_payload filled successful: beacons_number=%d",
                      (FMT__D, resp_params->parents.count_potential_parents));
        }
        else
        {
            zb_zcl_wwah_beacon_survey_t *worst_beacon_survey =
                zdo_wwah_find_survey_beacon(resp_params->parents.count_potential_parents,
                                            (zb_zcl_wwah_beacon_survey_t *)resp_params->parents.parents_info_ptr, ZB_FALSE);

            if (zdo_wwah_compare_survey_beacons(&new_beacon_survey, worst_beacon_survey))
            {
                ZB_MEMCPY(worst_beacon_survey, &new_beacon_survey, sizeof(zb_zcl_wwah_beacon_survey_t));
            }

            TRACE_MSG(TRACE_ZDO1, "worst survey_beacon_payload updated successful: beacons_number=%d",
                      (FMT__D, resp_params->parents.count_potential_parents));
        }
    }

    TRACE_MSG(TRACE_ZDO1, "<< zdo_wwah_process_beacon_info", (FMT__0));
}

/**
 *  @brief Fill a @zb_zcl_wwah_beacon_survey_t struct using information from a
 *         @zb_mac_beacon_notify_indication_t and @zb_mac_beacon_payload_t
 *
 *  @param beacon_noti_ind_ptr - [in] pointer to the @zb_mac_beacon_notify_indication_t
 *  @param beacon_payload_ptr - [in] pointer to the @zb_mac_beacon_payload_t
 *  @param survey_beacon_ptr - [out] pointer to a @zb_zcl_wwah_beacon_survey_t struct;
 *
 *  @return @RET_OK if @param beacon_survey_ptr filled, @RET_ERROR otherwise
 */
static zb_ret_t zdo_wwah_fill_survey_beacon(
    zb_mac_beacon_notify_indication_t *beacon_noti_ind_ptr,
    zb_mac_beacon_payload_t *beacon_payload_ptr,
    zb_zcl_wwah_beacon_survey_t *beacon_survey_ptr)
{
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_ZDO2, ">> zdo_wwah_fill_survey_beacon", (FMT__0));

    ZB_ASSERT(beacon_noti_ind_ptr);
    ZB_ASSERT(beacon_payload_ptr);
    ZB_ASSERT(beacon_survey_ptr);

    if (ZB_PIBCACHE_PAN_ID() != beacon_noti_ind_ptr->pan_descriptor.coord_pan_id)
    {
        TRACE_MSG(TRACE_ZDO2, "beacon from other network - not used for Beacon Survey", (FMT__0));
        ret = RET_ERROR;
    }

    TRACE_MSG(
        TRACE_ZDO2,
        "our PAN 0x%x; packet PAN 0x%x",
        (FMT__D_D, ZB_PIBCACHE_PAN_ID(), beacon_noti_ind_ptr->pan_descriptor.coord_pan_id));

    if (ret == RET_OK)
    {
        switch (beacon_noti_ind_ptr->pan_descriptor.coord_addr_mode)
        {
        case ZB_ADDR_16BIT_DEV_OR_BROADCAST:
            beacon_survey_ptr->device_short =
                beacon_noti_ind_ptr->pan_descriptor.coord_address.addr_short;
            break;
        case ZB_ADDR_64BIT_DEV:
            beacon_survey_ptr->device_short =
                zb_address_short_by_ieee(beacon_noti_ind_ptr->pan_descriptor.coord_address.addr_long);
            break;
        default:
            TRACE_MSG(TRACE_ERROR, "invalid addr mode", (FMT__0));
            ret = RET_ERROR;
            break;
        }
    }

    if (ret == RET_OK && ZB_UNKNOWN_SHORT_ADDR == beacon_survey_ptr->device_short)
    {
        TRACE_MSG(TRACE_ERROR, "invalid short addr", (FMT__0));
        ret = RET_ERROR;
    }

    TRACE_MSG(TRACE_ZDO2, "beacon short addr 0x%x", (FMT__D, beacon_survey_ptr->device_short));

    if (ret == RET_OK)
    {
        beacon_survey_ptr->rssi = beacon_noti_ind_ptr->rssi;
        beacon_survey_ptr->classification_mask = 0;

        ZB_ZDO_SET_TC_CONNECTIVITY_BIT_VALUE(
            &beacon_survey_ptr->classification_mask, beacon_payload_ptr->tc_connectivity);
        ZB_ZDO_SET_LONG_UPTIME_BIT_VALUE(
            &beacon_survey_ptr->classification_mask, beacon_payload_ptr->long_uptime);

        TRACE_MSG(
            TRACE_ZDO2,
            "rssi %hd, tc_connectivity %hd, long_uptime %hd",
            (FMT__H_H_H,
             beacon_survey_ptr->rssi,
             beacon_payload_ptr->tc_connectivity,
             beacon_payload_ptr->long_uptime));
    }

    TRACE_MSG(TRACE_ZDO2, "<< zdo_wwah_fill_survey_beacon, ret %ld", (FMT__L, ret));

    return ret;
}

/**
 *  @brief Find the best/worst Survey Beacon in an array
 *
 *         See Zigbee WWAH Requirements spec,
 *         6.2 Commissioning Requirements, C-20
 *
 *  @param beacons_number - number of @zb_zcl_wwah_beacon_survey_t
 *  @param first_survey_beacons_ptr - pointer to the first @zb_zcl_wwah_beacon_survey_t
 *  @param find_best - finding type flag
 *         ZB_TRUE to find the best beacon;
 *         ZB_FALSE to find the worst beacon;
 *
 *  @return pointer to the best/worst @zb_zcl_wwah_beacon_survey_t
 *          NULL if no beacons found;
 */
static zb_zcl_wwah_beacon_survey_t *zdo_wwah_find_survey_beacon(
    zb_uint8_t beacons_number, zb_zcl_wwah_beacon_survey_t *first_sb_ptr, zb_bool_t find_best)
{
    zb_zcl_wwah_beacon_survey_t *found_beacon = NULL;
    zb_zcl_wwah_beacon_survey_t *current_beacon = NULL;
    zb_zcl_wwah_beacon_survey_t *beacon_from_good_group = NULL;
    zb_zcl_wwah_beacon_survey_t *beacon_from_marginal_group = NULL;
    zb_uindex_t i;

    ZB_ASSERT(first_sb_ptr);
    TRACE_MSG(TRACE_ZDO2, ">>zdo_wwah_find_survey_beacon", (FMT__0));

    for (i = 0; i < beacons_number; ++i)
    {
        current_beacon = first_sb_ptr + i;
        if (ZDO_WWAH_GET_LINK_QUALITY_GROUP_BY_RSSI(current_beacon->rssi))
        {
            if (!beacon_from_good_group
                    || (beacon_from_good_group
                        && (find_best
                            == zdo_wwah_compare_survey_beacons(current_beacon, beacon_from_good_group))))
            {
                beacon_from_good_group = current_beacon;
            }
        }
        else
        {
            if (!beacon_from_marginal_group
                    || (beacon_from_marginal_group
                        && (find_best
                            == zdo_wwah_compare_survey_beacons(current_beacon, beacon_from_marginal_group))))
            {
                beacon_from_marginal_group = current_beacon;
            }
        }
    }

    if (find_best)
    {
        found_beacon = beacon_from_good_group ? beacon_from_good_group : beacon_from_marginal_group;
    }
    else
    {
        found_beacon = beacon_from_marginal_group ? beacon_from_marginal_group : beacon_from_good_group;
    }

    TRACE_MSG(TRACE_ZDO2, "<<zdo_wwah_find_survey_beacon", (FMT__0));

    return found_beacon;
}

/**
 *  @brief Compare @zb_zcl_wwah_beacon_survey_t by
 *         @zb_zcl_wwah_classification_mask_t
 *
 *  @param first_ptr - pointer to the first @zb_zcl_wwah_beacon_survey_t
 *  @param second_ptr - pointer to the second @zb_zcl_wwah_beacon_survey_t
 *
 *  @return ZB_TRUE if the first_ptr is better than the second_ptr;
 *          ZB_FALSE otherwise
 */
zb_bool_t zdo_wwah_compare_survey_beacons(
    zb_zcl_wwah_beacon_survey_t *first_ptr,
    zb_zcl_wwah_beacon_survey_t *second_ptr)
{
    zb_zcl_wwah_parent_priority_t first_priority;
    zb_zcl_wwah_parent_priority_t second_priority;

    TRACE_MSG(TRACE_ZDO2, "<<zdo_wwah_compare_survey_beacons", (FMT__0));

    ZB_ASSERT(first_ptr);
    ZB_ASSERT(second_ptr);

    first_priority =
        zdo_wwah_get_parent_priority_by_classification_mask(
            first_ptr->classification_mask);

    second_priority =
        zdo_wwah_get_parent_priority_by_classification_mask(
            second_ptr->classification_mask);

    TRACE_MSG(TRACE_ZDO2, ">>zdo_wwah_compare_survey_beacons", (FMT__0));

    return (zb_bool_t)(first_priority > second_priority);
}


void zb_nlme_beacon_survey_scan_confirm(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);

    TRACE_MSG(
        TRACE_NWK1,
        ">>zb_nlme_beacon_survey_scan_confirm, param %hd, status %hd",
        (FMT__H_H, param, confirm->status));

    zb_buf_free(param);
    ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;

    ZDO_SEND_SURVEY_BEACONS_RESPONSE_FUNC(HUBS_CTX().survey_beacons_resp_ref);

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_beacon_survey_scan_confirm", (FMT__0));
}


/* TODO: need to implement in r23 */
void zdo_send_survey_beacons_response(void)
{
    TRACE_MSG(TRACE_ZDO1, ">>zdo_send_survey_beacons_response", (FMT__0));
    TRACE_MSG(TRACE_ZDO1, "<<zdo_send_survey_beacons_response", (FMT__0));
}

#endif /* ZB_BEACON_SURVEY */
