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
/* PURPOSE: PAIND conflict detection and resolution at runtime
3.6.1.13, 3.4.9, 3,4,10
*/

#define ZB_TRACE_FILE_ID 2237
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_secur.h"
#include "zdo_wwah_stubs.h"
#include "zb_bufpool.h"

/*
  Note: by default panid conflict detection and resolution is disabled
  and that file is not linked.  To enable panid conflicts
  detection/resolution application must call
  zb_enable_panid_conflict_resolution() or
  zb_enable_auto_pan_id_conflict_resolution().
  It forces that file link.
 */

/*! \addtogroup ZB_NWK */
/*! @{ */

/**
   Minimum time between PAN ID conflicts resolution.
   It's need to prevent frequently PAN ID changes.
*/
#define ZB_PANID_CONFLICTS_RESOLUTION_DELAY_TIME (2U * 60U * ZB_TIME_ONE_SECOND)

#ifndef ZB_LITE_NO_PANID_CONFLICT_DETECTION

#ifndef ZB_LITE_NO_PANID_CONFLICT_RECEIVING
static void zb_panid_conflict_set_panid_alarm(zb_uint8_t param);
static void zb_panid_conflict_set_panid_conf(zb_uint8_t param);
static void zb_panid_conflict_set_panid(zb_uint8_t param);
static void zb_panid_conflict_network_update_recv(zb_uint8_t param);
#endif  /* ZB_LITE_NO_PANID_CONFLICT_RECEIVING */

#ifdef ZB_ROUTER_ROLE

/**
   zb_panid_conflict_send_status_ind

   @return nothing
 */
static void nwk_panid_conflict_in_beacon(zb_uint8_t param);
static void zb_panid_conflict_send_status_ind(zb_uint8_t param);
static void zb_panid_conflict_send_network_report(zb_uint8_t param);
static void zb_panid_conflict_got_network_report(zb_uint8_t param);
static void zb_panid_conflict_schedule_network_report(zb_uint8_t param);
static void zb_nwk_handle_panid_conflict(zb_uint8_t param);

#endif  /* #ifdef ZB_ROUTER_ROLE */

/* API */
void zb_enable_panid_conflict_resolution(zb_bool_t status)
{
    if (status)
    {
        ZB_NIB_NWK_MANAGER_ADDR() = 0;
        TRACE_MSG(TRACE_NWK1, "zb_enable_panid_conflict_resolution enabled", (FMT__0));
#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            ZG->nwk.selector.panid_conflict_in_beacon = nwk_panid_conflict_in_beacon;
            ZG->nwk.selector.panid_conflict_got_network_report
                = zb_panid_conflict_got_network_report;
        }
#endif
#ifndef ZB_LITE_NO_PANID_CONFLICT_RECEIVING
        ZG->nwk.selector.panid_conflict_network_update_recv
            = zb_panid_conflict_network_update_recv;
#endif
    }
    else
    {
        ZB_NIB_NWK_MANAGER_ADDR() = 0xffff;
        TRACE_MSG(TRACE_NWK1, "zb_enable_panid_conflict_resolution disabled", (FMT__0));
    }
}


#ifdef ZB_ROUTER_ROLE
static void nwk_panid_conflict_in_beacon(zb_uint8_t param)
{
    zb_mac_beacon_notify_indication_t *ind = (zb_mac_beacon_notify_indication_t *)zb_buf_begin(param);
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,28} */
    zb_mac_beacon_payload_t *beacon_payload = (zb_mac_beacon_payload_t *)ind->sdu;
    /* Detect PANID conflict, On HW seems not need to check for short panid, but
     * let's do it for ns */
    if (ind->pan_descriptor.coord_pan_id == ZB_PIBCACHE_PAN_ID()
            && !ZB_EXTPANID_CMP(beacon_payload->extended_panid, ZB_NIB_EXT_PAN_ID())
#ifdef ZB_CERTIFICATION_HACKS
            && !ZB_CERT_HACKS().disable_pan_id_conflict_detection
#endif
       )
    {
        if (ZB_NIB().nwk_manager_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
        {
            TRACE_MSG(TRACE_NWK1, "Got PANID conflict, I am a nwk manager - start panid conflict resolve", (FMT__0));
            if (ZG->nwk.handle.state == ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION)
            {
                TRACE_MSG(TRACE_NWK1, "Already in panid conflict resolution state", (FMT__0));
                zb_buf_free(param);
            }
            else
            {
                zb_pan_id_conflict_info_t *info;

                info = zb_buf_initial_alloc(param, sizeof(zb_pan_id_conflict_info_t));
                info->panid_count = 1;
                info->panids[0] = ZB_PIBCACHE_PAN_ID();

                zb_nwk_handle_panid_conflict(param);
            }
        }
        else
        {
            zb_panid_conflict_schedule_network_report(param);
        }
    }
    else
    {
        zb_buf_free(param);
    }
}


/* API */
/* Under the scope of Security Statement, automatic pan id conflict resolution
 * is not used. Use manual one: see sample in application/pid_conflcit */
void zb_enable_auto_pan_id_conflict_resolution(zb_bool_t status)
{
    zb_enable_panid_conflict_resolution(status);
    ZG->nwk.panid_conflict_auto_resolution = status;
}


/* API */
/* Actual resolution of PAN ID conflict */
void zb_start_pan_id_conflict_resolution(zb_uint8_t param)
{
    zb_pan_id_conflict_info_t *info = (zb_pan_id_conflict_info_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_NWK2, ">> zb_start_pan_id_conflict_resolution param %hd", (FMT__H, param));

    ZB_ASSERT(info->panid_count <= sizeof(info->panids) / sizeof(info->panids[0]));

    ZG->nwk.handle.state = ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION;
    TRACE_MSG(TRACE_NWK3, "sending Pan ID conflict resolution network update", (FMT__0));

    zb_panid_conflict_network_update(param);

    TRACE_MSG(TRACE_NWK2, "<< zb_start_pan_id_conflict_resolution", (FMT__0));
}



/**
  A function responsible for PAN ID conflict processing. Depending on the current settings,
 * it will either start the resolution automatically, or generate the corresponding signal.
 *
 * Buffer denoted by param is expected to contain zb_pan_id_conflict_info_t (in the buffer body)
 */
static void zb_nwk_handle_panid_conflict(zb_uint8_t param)
{
    TRACE_MSG(TRACE_NWK2, ">> zb_nwk_handle_panid_conflict param %hd", (FMT__H, param));

    /* Under the scope of Security Statement, automatic pan id conflict resolution
     * is not used. Use manual one: see sample in application/pid_conflcit */
    if (ZG->nwk.panid_conflict_auto_resolution)
    {
        zb_start_pan_id_conflict_resolution(param);
    }
    else
    {
        TRACE_MSG(TRACE_NWK3, "Sending a ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED signal", (FMT__0));
        zb_app_signal_pack_with_data(param, ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED, RET_OK);
        ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
    }

    TRACE_MSG(TRACE_NWK2, "<< zb_nwk_handle_panid_conflict", (FMT__0));
}


/**
   Reaction on panid conflict report at nwk manager (usually ZC).
 */
static void zb_panid_conflict_got_network_report(zb_uint8_t param)
{
    zb_uint8_t hdr_size = ZB_NWK_HDR_SIZE((zb_nwk_hdr_t *)zb_buf_begin(param));
    zb_nwk_report_cmd_t *report = (zb_nwk_report_cmd_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

    TRACE_MSG(TRACE_NWK3, "got network report cmd param %hd", (FMT__H, param));
    if (!ZB_NWK_REPORT_IS_PANID_CONFLICT(report->command_options)
            || !ZB_EXTPANID_CMP(report->epid, ZB_NIB_EXT_PAN_ID()))
    {
        TRACE_MSG(TRACE_ERROR, "drop report cmd %hd", (FMT__H, ZB_NWK_REPORT_COMMAND_ID(report->command_options)));
        zb_buf_free(param);
    }
    else
    {
        zb_uint8_t n_panids = ZB_NWK_REPORT_INFO_COUNT(report->command_options);
        if (ZG->nwk.handle.state == ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION
                || ZG->nwk.handle.panid_conflict)
        {
            TRACE_MSG(TRACE_NWK3, "panid conflict resolve is already in progress", (FMT__0));
            zb_buf_free(param);
        }
        else
        {
            zb_pan_id_conflict_info_t *info;
            zb_uint8_t i;
            zb_uint_t alloc_right_bytes;

            TRACE_MSG(TRACE_NWK3, "got panid report, n_panids %hd, start conflict resolve", (FMT__H, n_panids));

            /* report points to a location inside the buffer */
            /* if we cut the buffer before the start of pan id array, we can easily construct
               an zb_pan_id_conflict_info_t instance */
            (void)zb_buf_cut_left(param, (zb_uint_t)((zb_uint8_t *)&report->panids - (zb_uint8_t *)zb_buf_begin(param)));

            /* allocate more space in the end, in case the report did not have maximum possible number of PAN ids*/
            alloc_right_bytes = sizeof(zb_uint16_t) * ZB_PAN_ID_CONFLICT_INFO_MAX_PANIDS_COUNT - zb_buf_len(param);
            if (alloc_right_bytes > 0U)
            {
                (void)zb_buf_alloc_right(param, alloc_right_bytes);
            }

            /* allocate space for panids_count field */
            info = zb_buf_alloc_left(param, sizeof(zb_uint16_t));

            info->panid_count = n_panids;
            for (i = 0; i < n_panids; i++)
            {
                ZB_LETOH16_ONPLACE((info->panids[i]));
            }

            zb_nwk_handle_panid_conflict(param);
        }
    }
}


/**
   After active scan complete send network update command to entire net

   Not an official API.

   Called from the test tests/parent_loss/pl_th_zc.c
 */
void zb_panid_conflict_network_update(zb_uint8_t param)
{
    zb_pan_id_conflict_info_t *info = (zb_pan_id_conflict_info_t *)zb_buf_begin(param);

    /* choose new panid */
    zb_ushort_t i;
    do
    {
        ZG->nwk.handle.new_panid = ZB_RANDOM_U16();
        /* filter using buffer param */
        for (i = 0; i < info->panid_count && info->panids[i] != ZG->nwk.handle.new_panid; ++i)
        {
        }
    } while (i < info->panid_count);

    TRACE_MSG(TRACE_NWK3, "zb_panid_conflict_network_update: chosen new_panid 0x%x", (FMT__D, ZG->nwk.handle.new_panid));


    /* According to 3.6.1.13.2 change nwkUpdateId now and does not change it
     * after actual PANID change */
    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
        zb_ret_t ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
        TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
        }
    }
    ZB_SCHEDULE_CALLBACK(zb_panid_conflict_send_nwk_update, param);
    ZB_SCHEDULE_ALARM(zb_panid_conflict_set_panid_alarm,
                      0,
                      ZB_NWK_OCTETS_TO_BI(ZB_NWK_JITTER(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)));
}


/* If we are not in PAN ID conflict state now, enter the state and send a network report.
   Later, hopefully, there would be an MLME-SCAN.req and zb_panid_conflict_send_network_report
   would handle its results. For now, we are not collecting other PAN IDs and
   zb_panid_conflict_send_network_report generates the report with our own PAN ID only.
*/
static void zb_panid_conflict_schedule_network_report(zb_uint8_t param)
{
    if (!ZG->nwk.handle.panid_conflict)
    {
        TRACE_MSG(TRACE_NWK1, "panid conflict detected, schedule panid conflict report", (FMT__0));
        /* TODO: call MLME-SCAN.req */
        ZB_SCHEDULE_ALARM(zb_panid_conflict_send_network_report, param, ZB_NWK_JITTER(ZB_DEFAULT_SCAN_DURATION));
        ZG->nwk.handle.panid_conflict = ZB_TRUE;
    }
    else
    {
        zb_buf_free(param);
    }
}


/**
   Sent network report command after panid conflict detect on ZR
 */
static void zb_panid_conflict_send_network_report(zb_uint8_t param)
{
    zb_uint8_t pan_id_cnt;
    zb_nwk_report_cmd_t *rep;
    zb_nwk_hdr_t *nwhdr;
    zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL() != 0U);
    zb_bool_t status;

    TRACE_MSG(TRACE_NWK1, ">> zb_panid_conflict_send_network_report", (FMT__0));
    TRACE_MSG(TRACE_ATM1, "Z< send network report", (FMT__0));

    if (0U == ZB_NIB_NWK_MANAGER_ADDR())
    {
        /* This function is supposed to generate a Network Report packet with all the information.
           Currently ZBOSS does not collect neighboring PAN IDs, so this function just packs
           ZB_PIBCACHE_PAN_ID() */

        if (ZG->nwk.handle.panid_conflict && ZG->nwk.handle.new_panid != 0U)
        {
            TRACE_MSG(TRACE_NWK1, "Already got new panid - not need to send nwk report", (FMT__0));
            zb_buf_free(param);
            status = ZB_FALSE;
        }
        else
        {
            nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NIB().nwk_manager_addr,
                                           ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
            nwhdr->radius = (zb_uint8_t)(ZB_NIB_MAX_DEPTH() * 2U);

            pan_id_cnt = 1;
            rep = (zb_nwk_report_cmd_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_NETWORK_REPORT,
                    (zb_uint8_t)sizeof(zb_nwk_report_cmd_t)
                    + (pan_id_cnt - 1U) * (zb_uint8_t)sizeof(zb_uint16_t));

            rep->command_options = pan_id_cnt; /* counter */
            ZB_IEEE_ADDR_COPY(rep->epid, ZB_NIB_EXT_PAN_ID());
            rep->panids[0] = ZB_PIBCACHE_PAN_ID();
            ZB_HTOLE16_ONPLACE(rep->panids[0]);

            TRACE_MSG(TRACE_NWK2, "zb_panid_conflict_send_network_report param %hd %hd panids panid[0] = %hd",
                      (FMT__H_H_D, param, pan_id_cnt, rep->panids[0]));

            /* send request */
            (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);
            ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

            /* done with panid conflict info collecting */

            status = ZB_TRUE;
        }
    }
    else
    {
        TRACE_MSG(TRACE_NWK1, "PAN ID conflcit resolution is disabled. No NWK report will be sent.",
                  (FMT__0));
        zb_buf_free(param);
        status = ZB_FALSE;
    }

    ZVUNUSED(status);
    TRACE_MSG(TRACE_NWK1, "<< zb_panid_conflict_send_network_report, report will be sent: %d", (FMT__D, status));
}


/**
  Not an official API. Called form application/HA_samples/wwah/sample_zc.c
*/
void zb_panid_conflict_send_nwk_update(zb_uint8_t param)
{
    /* [DT]: 06/01/2016: CR
     * Replaced zb_nwk_fill_out_command call (which was duplicate
     * and didn't process extended addresses properly */
    zb_ret_t ret;
    zb_nwk_update_cmd_t *upd;
    zb_nwk_hdr_t *nwhdr;
    zb_bool_t secure = (zb_bool_t)ZB_NIB_SECURITY_LEVEL();

    TRACE_MSG(TRACE_NWK1, ">> zb_panid_conflict_send_network_update param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ATM1, "Z< send network update", (FMT__0));

    nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_NWK_BROADCAST_ALL_DEVICES,
                                   ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);

    nwhdr->radius = (zb_uint8_t)(ZB_NIB_MAX_DEPTH() * 2U);

    upd = (zb_nwk_update_cmd_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_NETWORK_UPDATE, (zb_uint8_t)sizeof(zb_nwk_update_cmd_t));

    upd->command_options = 1;     /* counter */
    ZB_IEEE_ADDR_COPY(upd->epid, ZB_NIB_EXT_PAN_ID());
    ++ZB_NIB_UPDATE_ID();
    upd->update_id = ZB_NIB_UPDATE_ID();
    ZB_HTOLE16((zb_uint8_t *)&upd->new_panid, (zb_uint8_t *)&ZG->nwk.handle.new_panid);

    /* send request */
    (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);
    ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);

    ret = zb_buf_get_out_delayed(zb_panid_conflict_send_status_ind);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }

    TRACE_MSG(TRACE_NWK1, "<< zb_panid_conflict_send_network_update", (FMT__0));
}


#endif  /* #ifdef ZB_ROUTER_ROLE */

#ifndef ZB_LITE_NO_PANID_CONFLICT_RECEIVING
static void zb_panid_conflict_send_status_ind(zb_uint8_t param)
{
    zb_nlme_status_indication_t *status =  ZB_BUF_GET_PARAM(param, zb_nlme_status_indication_t);

    TRACE_MSG(TRACE_NWK1, ">> zb_panid_conflict_send_status_ind %hd", (FMT__H, param));

    /* NetworkAddr parameter set to 0 and the
       Status parameter set to PAN Identifier Update */
    status->status = ZB_NWK_COMMAND_STATUS_PAN_IDENTIFIER_UPDATE;
    status->network_addr = ZG->nwk.handle.new_panid;
    /* notify */
    ZB_SCHEDULE_CALLBACK(zb_nlme_status_indication, param);
    ZG->nwk.handle.panid_conflict = ZB_FALSE;
    TRACE_MSG(TRACE_NWK1, "<< zb_panid_conflict_send_status_ind %hd", (FMT__H, param));
}


/* Suppose it is possible for non-sleepy ZED to receive network update. */

/**
   Reaction on nwk update command frame recv - change panid
 */
static void zb_panid_conflict_network_update_recv(zb_uint8_t param)
{
    zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
    zb_uint8_t hdr_size = ZB_NWK_HDR_SIZE(nwk_hdr);
    zb_nwk_update_cmd_t *upd = (zb_nwk_update_cmd_t *)ZB_NWK_CMD_FRAME_GET_CMD_PAYLOAD(param, hdr_size);

    TRACE_MSG(TRACE_NWK3, "got ZB_NWK_CMD_NETWORK_UPDATE", (FMT__0));
    if ( !ZB_NWK_UPDATE_IS_PANID_UPDATE(upd->command_options)
            || !ZB_EXTPANID_CMP(upd->epid, ZB_NIB_EXT_PAN_ID())
            || ZB_NWK_UPDATE_INFO_COUNT(upd->command_options) != 1U
            /* If a NLME Network Update command is received by the device specifying a short PAN ID
             * that does not match PendingNetworkUpdatePANID attribute, then the NLME Network Update
             * message SHALL be ignored by the device. A value of 0xFFFF indicates that any short PAN ID
             * received in a NLME Network Update command SHALL be accepted. */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER)
            || !ZB_ZDO_CHECK_NEW_PANID(upd->new_panid)
#endif
            /* If TCSecurityOnNwkKeyRotationEnabled Attribute is set to TRUE, the node SHALL only
             * process network key rotation commands (APS Transport Key and Network Update commands)
             * which are sent via unicast and are encrypted by the Trust Center Link Key. */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER)
            || (ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr)
                && !ZB_ZDO_CHECK_NWK_KEY_COMMANDS_BROADCAST_ALLOWED())
#endif
       )
    {
        TRACE_MSG(TRACE_ERROR, "drop nwk update cmd %hd", (FMT__H, ZB_NWK_UPDATE_COMMAND_ID(upd->command_options)));
        zb_buf_free(param);
    }
    else
    {
        TRACE_MSG(TRACE_NWK3, "received NWK update cmd", (FMT__0));

#ifdef ZB_ROUTER_ROLE
        if (ZB_IS_DEVICE_ZC_OR_ZR())
        {
            zb_ret_t ret = zb_buf_get_out_delayed(zb_nwk_update_beacon_payload);
            TRACE_MSG(TRACE_ZDO1, "schedule zb_nwk_update_beacon_payload", (FMT__0));
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
                zb_buf_free(param);
                return;
            }
        }
#endif
        ZB_NIB_UPDATE_ID() = upd->update_id;

        ZB_LETOH16((zb_uint8_t *)&ZG->nwk.handle.new_panid, (zb_uint8_t *)&upd->new_panid);
        TRACE_MSG(TRACE_NWK1, "zb_panid_conflict_network_update_recv update_id %hd panid %d",
                  (FMT__H_D, upd->update_id, ZG->nwk.handle.new_panid));
        ZB_SCHEDULE_ALARM(zb_panid_conflict_set_panid_alarm, 0,
                          ZB_NWK_OCTETS_TO_BI(ZB_NWK_JITTER(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)));
        ZB_SCHEDULE_CALLBACK(zb_panid_conflict_send_status_ind, param);
    }
}


/**
   Allocate buffer to set new panid
 */
static void zb_panid_conflict_set_panid_alarm(zb_uint8_t param)
{
    zb_ret_t ret;

    ZVUNUSED(param);
    ret = zb_buf_get_out_delayed(zb_panid_conflict_set_panid);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
}

static void zb_panid_conflict_set_panid(zb_uint8_t param)
{
    /* This if construction was added to solve the problem with two network update broadcasts.
     * When device receive Network Update command, it schedule zb_panid_conflict_set_panid().
     * If we run this function two times, 0 will be written to ZB_PIBCACHE_PAN_ID()
     */
    if (ZG->nwk.handle.new_panid != 0U)
    {
        ZB_PIBCACHE_PAN_ID() = ZG->nwk.handle.new_panid;
        ZG->nwk.handle.new_panid = 0;
        zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2, zb_panid_conflict_set_panid_conf);
    }
    else
    {
        zb_buf_free(param);
    }
}

static void zb_panid_conflict_mark_resolved(zb_uint8_t param)
{
    ZVUNUSED(param);

    if (ZG->nwk.handle.state == ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION)
    {
        /* panid conflict resolved */
        ZG->nwk.handle.state = ZB_NLME_STATE_IDLE;
    }
}


static void zb_panid_conflict_set_panid_conf(zb_uint8_t param)
{
    zb_buf_free(param);
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
    if (ZB_NIB().nwk_manager_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
        /* If we are the NWK manager then marks the conflict resolved
           only after some delay to prevent frequently PAN ID changes
        */
        ZB_SCHEDULE_ALARM(zb_panid_conflict_mark_resolved, 0, ZB_PANID_CONFLICTS_RESOLUTION_DELAY_TIME);
    }
    else
    {
        zb_panid_conflict_mark_resolved(0);
    }
}

#endif  /* #ifndef ZB_LITE_NO_PANID_CONFLICT_RECEIVING */

#endif  /* ZB_LITE_NO_PANID_CONFLICT_DETECTION */
/*! @} */
