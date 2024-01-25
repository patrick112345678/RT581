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
/* PURPOSE: Permit join API
*/

#define ZB_TRACE_FILE_ID 320
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"

/*! \addtogroup ZB_NWK */
/*! @{ */

#if defined ZB_ROUTER_ROLE

void nwk_permit_timeout(zb_uint8_t param);

#if defined ZB_JOINING_LIST_SUPPORT
static void joining_lists_enable_join_cb(zb_uint8_t param);
static void joining_lists_enable_join(zb_uint8_t param);
static void joining_lists_disable_join(zb_uint8_t param);
#endif /* ZB_JOINING_LIST_SUPPORT */

void nwk_permit_timeout(zb_uint8_t param)
{
    zb_ret_t ret;

    ZVUNUSED(param);
    TRACE_MSG(TRACE_NWK1, ">>nwk_permit_timeout %hd", (FMT__H, param));

    ZG->nwk.handle.permit_join = ZB_FALSE;
    ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_FALSE_U;
#ifdef ZB_RAF_PERMIT_JOIN_ZDO_SIGNAL
    if (ZB_IS_DEVICE_ZC())
    {
        zb_send_permit_join_signal(0x00);
    }
#endif
    TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
    ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
#if defined ZB_JOINING_LIST_SUPPORT
    (void)zb_buf_get_out_delayed(joining_lists_disable_join);
#endif /* ZB_JOINING_LIST_SUPPORT */

    TRACE_MSG(TRACE_NWK1, "<<nwk_permit_timeout", (FMT__0));
}

//! [zb_nlme_permit_joining_confirm]
static void zb_nlme_permit_joining_confirm_ok(zb_uint8_t param)
{
    zb_callback_t cb = zb_nlme_permit_joining_confirm;

    TRACE_MSG(TRACE_NWK1, ">> zb_nlme_permit_joining_confirm_ok param %hd", (FMT__H, param));

    zb_buf_set_status(param, ZB_NWK_STATUS_SUCCESS);
    ZB_SCHEDULE_CALLBACK(cb, param);

    TRACE_MSG(TRACE_NWK1, "<< zb_nlme_permit_joining_confirm_ok", (FMT__0));
}
//! [zb_nlme_permit_joining_confirm]

void zb_nlme_permit_joining_request(zb_uint8_t param)
{
    zb_nlme_permit_joining_request_t *request;
    zb_uint8_t permit_duration;

    request = ZB_BUF_GET_PARAM(param, zb_nlme_permit_joining_request_t);
    permit_duration = request->permit_duration;

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_permit_joining_request %hd", (FMT__H, param));
    CHECK_PARAM_RET_ON_ERROR(request);

    /* zb_th_push_packet(ZB_TH_NLME_NETWORK_PERMIT_JOINING, ZB_TH_PRIMITIVE_REQUEST, param); */
    TRACE_MSG(TRACE_NWK1, "permit join was %hd my device type %hd new duration %hd",
              (FMT__H_H_H, ZG->nwk.handle.permit_join, ZB_NIB_DEVICE_TYPE(), request->permit_duration));

    /*
     * If a second Mgmt_Permit_Joining_req is received while the previous one is still counting down,
     * it will supersede the previous request.

     * Cancel any other permit timeout.
     */
    if ( ZG->nwk.handle.permit_join )
    {
        ZB_SCHEDULE_ALARM_CANCEL(nwk_permit_timeout, ZB_ALARM_ANY_PARAM);
        ZG->nwk.handle.permit_join = ZB_FALSE;
    }

    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        TRACE_MSG(TRACE_NWK1, "permit duration %d", (FMT__D, (int)request->permit_duration));

        switch ( request->permit_duration )
        {
        case 0x00:
            ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_FALSE_U;
#ifdef ZB_RAF_PERMIT_JOIN_ZDO_SIGNAL
            if (ZB_IS_DEVICE_ZC())
            {
                zb_send_permit_join_signal(request->permit_duration);
            }
#endif
            break;
        case 0xff:
            permit_duration = 0xfe;
            /*
             * Versions of this specification prior to revision 21 allowed a value of
             * 0xFF to be interpreted as 'forever'. Version 21 and later do not
             * allow this. All devices conforming to this specification shall
             * interpret 0xFF as 0xFE. Devices that wish to extend the PermitDuration
             * beyond 0xFE seconds shall periodically re-send the
             * Mgmt_Permit_Joining_req.
             */
            break;
        default:
            break;
        }

        if (request->permit_duration > 0U)
        {
            if (ZB_NIB().max_children > 0U
#ifdef ZB_CERTIFICATION_HACKS
                    && ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num
#endif
               )
            {
                ZG->nwk.handle.permit_join = ZB_TRUE;
                ZB_PIBCACHE_ASSOCIATION_PERMIT() = ZB_TRUE_U;
#ifdef ZB_RAF_PERMIT_JOIN_ZDO_SIGNAL
                if (ZB_IS_DEVICE_ZC())
                {
                    zb_send_permit_join_signal(request->permit_duration);
                }
#endif
                ZB_SCHEDULE_ALARM(nwk_permit_timeout, 0, permit_duration * ZB_TIME_ONE_SECOND);
#if defined ZB_JOINING_LIST_SUPPORT
                (void)zb_buf_get_out_delayed(joining_lists_enable_join);
#endif /* ZB_JOINING_LIST_SUPPORT */
            }
        }

        TRACE_MSG(TRACE_NWK1, "ZB_PIBCACHE_ASSOCIATION_PERMIT is set to %d", (FMT__D, ZB_PIBCACHE_ASSOCIATION_PERMIT()));
        /*
         * zb_nwk_update_beacon_payload updates ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT
         * in MAC
         * FIXME: are there races available between zb_nwk_update_beacon_payload
         * complete and application doing something after
         * zb_nlme_permit_joining_confirm call?
         */
        ZG->nwk.handle.run_after_update_beacon_payload = zb_nlme_permit_joining_confirm_ok;
        TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_nwk_update_beacon_payload, param);
    }
    else
    {
        zb_buf_set_status(param, ZB_NWK_STATUS_INVALID_REQUEST);
        ZB_SCHEDULE_CALLBACK(zb_nlme_permit_joining_confirm, param);
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_permit_joining_request", (FMT__0));
}

#if defined ZB_JOINING_LIST_SUPPORT

static void joining_lists_enable_join_cb(zb_uint8_t param)
{
    zb_mlme_get_confirm_t *cfm;
    zb_uint8_t new_status;

    TRACE_MSG(TRACE_ZDO3, ">> joining_lists_enable_join_cb %hd", (FMT__H, param));

    cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
    ZB_ASSERT(zb_buf_len(param) >= sizeof(*cfm));

    if (cfm->status == MAC_SUCCESS)
    {
        ZB_ASSERT(zb_buf_len(param) > sizeof(*cfm));

        /* ieee joining list entries exist => set the corresponding policy */
        if (*(zb_uint8_t *)(cfm + 1) > 0U)
        {
            new_status = ZB_MAC_JOINING_POLICY_IEEELIST_JOIN;
        }
        else
        {
            new_status = ZB_MAC_JOINING_POLICY_ALL_JOIN;
        }

        zb_nwk_pib_set(param,
                       ZB_PIB_ATTRIBUTE_JOINING_POLICY,
                       &new_status,
                       sizeof(new_status),
                       NULL); /* free the buf */
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "Radio doesn't support the Joining lists", (FMT__0));
        zb_buf_free(param);
    }
}

static void joining_lists_enable_join(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO3, ">> joining_lists_enable_join %hd", (FMT__H, param));

    zb_nwk_pib_get(param,
                   ZB_PIB_ATTRIBUTE_JOINING_IEEE_LIST_LENGTH,
                   joining_lists_enable_join_cb);
}

static void joining_lists_disable_join(zb_uint8_t param)
{
    zb_uint8_t new_status;
    TRACE_MSG(TRACE_ZDO3, ">> joining_lists_enable_join %hd", (FMT__H, param));

    new_status = ZB_MAC_JOINING_POLICY_NO_JOIN;
    zb_nwk_pib_set(param,
                   ZB_PIB_ATTRIBUTE_JOINING_POLICY,
                   &new_status,
                   sizeof(new_status),
                   NULL); /* free the buf */
}

#endif /* ZB_JOINING_LIST_SUPPORT */

#endif  /* ZB_ROUTER_ROLE */

/*! @} */
