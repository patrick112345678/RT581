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
/* PURPOSE: Trace functions for MAC primitives.
*/
#define ZB_TRACE_FILE_ID 56

#include "zb_common.h"
#include "zb_mac.h"
#if !defined ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_mac_transport.h"
#include "mac_internal.h"
#include "zb_secur.h"


#if defined ZB_MAC_API_TRACE_PRIMITIVES && !defined ZB_MACSPLIT_HOST

void mac_api_trace_dump(zb_uint8_t *buf, zb_ushort_t len)
{
    zb_ushort_t i;
    ZVUNUSED(buf);

    TRACE_MSG(TRACE_MAC_API1, "len %hd", (FMT__H, len));

#define HEX_ARG(n) buf[i+n]

    for (i = 0 ; i < len ; )
    {
        if (len - i >= 8)
        {
            TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx %hx %hx %hx %hx %hx",
                      (FMT__H_H_H_H_H_H_H_H,
                       HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                       HEX_ARG(4), HEX_ARG(5), HEX_ARG(6), HEX_ARG(7)));
            i += 8;
        }
        else
        {
            switch (len - i)
            {
            case 7:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5), HEX_ARG(6)));
                break;
            case 6:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4), HEX_ARG(5)));
                break;
            case 5:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx %hx %hx",
                          (FMT__H_H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3),
                           HEX_ARG(4)));
                break;
            case 4:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx %hx",
                          (FMT__H_H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2), HEX_ARG(3)));
                break;
            case 3:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx %hx",
                          (FMT__H_H_H,
                           HEX_ARG(0), HEX_ARG(1), HEX_ARG(2)));
                break;
            case 2:
                TRACE_MSG(TRACE_MAC_API1, "%hx %hx",
                          (FMT__H_H,
                           HEX_ARG(0), HEX_ARG(1)));
                break;
            case 1:
                TRACE_MSG(TRACE_MAC_API1, "%hx",
                          (FMT__H,
                           HEX_ARG(0)));
                break;
            }
            i = len;
        }
    }
}


void zb_mac_api_trace_association_request(zb_uint8_t param)
{
    zb_mlme_associate_params_t *assoc_req = NULL;
    assoc_req = ZB_BUF_GET_PARAM(param, zb_mlme_associate_params_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ASSOCIATION.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "ChannelNumber=0x%hx", (FMT__H, assoc_req->logical_channel));
    TRACE_MSG(TRACE_MAC_API1, "ChannelPage=0x%hx", (FMT__H, assoc_req->channel_page));
    TRACE_MSG(TRACE_MAC_API1, "CoordAddrMode=0x%hx", (FMT__H, assoc_req->coord_addr_mode));
    TRACE_MSG(TRACE_MAC_API1, "CoordPANId=0x%x", (FMT__D, assoc_req->pan_id));

    if (assoc_req->coord_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "CoordAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(assoc_req->coord_addr.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "CoordAddr(short)=0x%x",
                  (FMT__D, assoc_req->coord_addr.addr_short));
    }

    TRACE_MSG(TRACE_MAC_API1, "CapabilityInformation=0x%x",
              (FMT__H, assoc_req->capability));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

void zb_mac_api_trace_association_response(zb_uint8_t param)
{
    zb_mlme_associate_response_t *assoc_resp = NULL;

    assoc_resp = ZB_BUF_GET_PARAM(param, zb_mlme_associate_response_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ASSOCIATION.response()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "DeviceAddress=" TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(assoc_resp->device_address)));
    TRACE_MSG(TRACE_MAC_API1, "AssocShortAddress=0x%x", (FMT__D, assoc_resp->short_address));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H, assoc_resp->status));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_association_confirm(zb_uint8_t param)
{
    zb_mlme_associate_confirm_t *assoc_confirm = NULL;

    assoc_confirm = ZB_BUF_GET_PARAM(param, zb_mlme_associate_confirm_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ASSOCIATE.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "AssocShortAddress=0x%x",
              (FMT__D, assoc_confirm->assoc_short_address));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H, assoc_confirm->status));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_association_indication(zb_uint8_t param)
{
    zb_mlme_associate_indication_t *assoc_ind = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ASSOCIATE.indication()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "DeviceAddress=" TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(assoc_ind->device_address)));
    TRACE_MSG(TRACE_MAC_API1, "CapabilityInformation=0x%x",
              (FMT__H, assoc_ind->capability));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

/* MLME-RESET.request() */
void zb_mac_api_trace_reset_request(zb_uint8_t param)
{
    zb_mlme_reset_request_t *reset_req = NULL;

    reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-RESET.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "SetDefaultPIB=%hd", (FMT__H, reset_req->set_default_pib));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

/* MLME-RESET.confirm() */
void zb_mac_api_trace_reset_confirm(zb_uint8_t param)
{
    zb_mlme_reset_confirm_t *reset_conf = NULL;

    reset_conf = ZB_BUF_GET_PARAM(param, zb_mlme_reset_confirm_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-RESET.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%hx", (FMT__H, reset_conf->status));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

/* MLME-BEACON-NOTIFY.indication() */
static void zb_mac_api_trace_super_frame_spec(zb_super_frame_spec_t *super_frame_spec)
{
    /* Make compiler happy */
    super_frame_spec = super_frame_spec;

    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec=", (FMT__0));
    mac_api_trace_dump((zb_uint8_t *)super_frame_spec, 2);

    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: BeaconOrder=%hd", (FMT__H, super_frame_spec->beacon_order));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: SuperframeOrder=%hd", (FMT__H, super_frame_spec->superframe_order));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: FinalCAPSlot=%hd", (FMT__H, super_frame_spec->final_cap_slot));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: BatteryLifeExtension=%hd", (FMT__H, super_frame_spec->battery_life_extension));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: PANCoordinator=%hd", (FMT__H, super_frame_spec->pan_coordinator));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeSpec: AssociationPermit=%hd", (FMT__H, super_frame_spec->associate_permit));
}

static void zb_mac_api_trace_pan_descriptor(zb_pan_descriptor_t *pan_descriptor)
{
    /* Make compiler happy */
    pan_descriptor = pan_descriptor;

    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: CoordAddrMode=0x%hx", (FMT__H, pan_descriptor->coord_addr_mode));
    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: CoordPANId=0x%x", (FMT__D, pan_descriptor->coord_pan_id));

    if (pan_descriptor->coord_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: CoordAddress(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(pan_descriptor->coord_address.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: CoordAddress(short)=0x%x",
                  (FMT__D, pan_descriptor->coord_address.addr_short));
    }

    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: ChannelNumber=0x%hx", (FMT__H, pan_descriptor->logical_channel));
    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: ChannelPage=0x%hx", (FMT__H, pan_descriptor->channel_page));

    zb_mac_api_trace_super_frame_spec(&pan_descriptor->super_frame_spec);

    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: GTSPermit=%hd", (FMT__H, pan_descriptor->gts_permit));
    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: LinkQuality=0x%hx", (FMT__H, pan_descriptor->link_quality));

    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: TimeStamp=0x%lx", (FMT__L, pan_descriptor->timestamp));
    TRACE_MSG(TRACE_MAC_API1, "PANDescriptor: SecurityLevel=0", (FMT__0));
}

void zb_mac_api_trace_beacon_notify_indication(zb_uint8_t param)
{
    zb_mac_beacon_notify_indication_t *ind = (zb_mac_beacon_notify_indication_t *)zb_buf_begin(param);
    zb_mac_beacon_payload_t *beacon_payload = (zb_mac_beacon_payload_t *)ind->sdu;
    zb_uint8_t i;

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-BEACON-NOTIFY.indication()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "BeaconType=0x%hx", (FMT__H, ind->beacon_type));
    TRACE_MSG(TRACE_MAC_API1, "BSN=0x%x", (FMT__H, ind->bsn));

    zb_mac_api_trace_pan_descriptor(&ind->pan_descriptor);

    TRACE_MSG(TRACE_MAC_API1, "PendAddrSpec: numShortPending=%hd", (FMT__H, ind->pend_addr_spec.num_of_short_addr));
    TRACE_MSG(TRACE_MAC_API1, "PendAddrSpec: numExtendedPending=%hd", (FMT__H, ind->pend_addr_spec.num_of_long_addr));

    for (i = 0; i < ind->pend_addr_spec.num_of_short_addr; i++)
    {
        TRACE_MSG(TRACE_MAC_API1, "AddrList: ShortPending[%hd]=0x%x",
                  (FMT__H_D, i, ind->addr_list[i].addr_short));
    }

    for (; i < ind->pend_addr_spec.num_of_short_addr + ind->pend_addr_spec.num_of_long_addr; i++)
    {
        TRACE_MSG(TRACE_MAC_API1, "AddrList: ExtendedPending[%hd]=" TRACE_FORMAT_64,
                  (FMT__H_A, i, TRACE_ARG_64(ind->addr_list[i].addr_long)));
    }

    TRACE_MSG(TRACE_MAC_API1, "SDU =", (FMT__0));
    mac_api_trace_dump((zb_uint8_t *) beacon_payload, ind->sdu_length);

    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_comm_status_indication(zb_uint8_t param)
{
    zb_mlme_comm_status_indication_t *comm_status_ind = NULL;

    comm_status_ind = ZB_BUF_GET_PARAM(param, zb_mlme_comm_status_indication_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-COMM-STATUS.indication()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "PANId=0x%x", (FMT__D, comm_status_ind->pan_id));

    TRACE_MSG(TRACE_MAC_API1, "SrcAddrMode=0x%hx", (FMT__H, comm_status_ind->src_addr_mode));
    if (comm_status_ind->src_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(comm_status_ind->src_addr.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(short)=0x%x",
                  (FMT__D, comm_status_ind->src_addr.addr_short));
    }

    TRACE_MSG(TRACE_MAC_API1, "DstAddrMode=0x%hx", (FMT__H, comm_status_ind->dst_addr_mode));
    if (comm_status_ind->dst_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(comm_status_ind->dst_addr.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(short)=0x%x",
                  (FMT__D, comm_status_ind->dst_addr.addr_short));
    }

    TRACE_MSG(TRACE_MAC_API1, "Status=0x%hx", (FMT__H, comm_status_ind->status));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_orphan_indication(zb_uint8_t param)
{
    zb_mac_orphan_ind_t *orphan_ind;

    orphan_ind = ZB_BUF_GET_PARAM(param, zb_mac_orphan_ind_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ORPHAN.indication()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "OrphanAddr=" TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(orphan_ind->orphan_addr)));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_orphan_response(zb_uint8_t param)
{
    zb_mac_orphan_response_t *orphan_resp = NULL;

    orphan_resp = ZB_BUF_GET_PARAM(param, zb_mac_orphan_response_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-ORPHAN.response()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "OrphanAddr=" TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(orphan_resp->orphan_addr)));
    TRACE_MSG(TRACE_MAC_API1, "ShortAddr=0x%x",
              (FMT__D, orphan_resp->short_addr));
    TRACE_MSG(TRACE_MAC_API1, "AssociatedMember=%hd",
              (FMT__H, orphan_resp->associated));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

void zb_mac_api_trace_scan_request(zb_uint8_t param)
{
    zb_mlme_scan_params_t *scan_params;

    scan_params = ZB_BUF_GET_PARAM(param, zb_mlme_scan_params_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-SCAN.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "ScanType=%hd", (FMT__H, scan_params->scan_type));
    TRACE_MSG(TRACE_MAC_API1, "ScanChannels=0x%lx", (FMT__L, scan_params->channels));
    TRACE_MSG(TRACE_MAC_API1, "ScanDuration=%hd", (FMT__H, scan_params->scan_duration));
    TRACE_MSG(TRACE_MAC_API1, "ChannelPage=0x%hx", (FMT__H, scan_params->channel_page));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_scan_confirm(zb_uint8_t param)
{
    zb_mac_scan_confirm_t *scan_confirm = NULL;
    zb_ushort_t i;

    scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-SCAN.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%hx", (FMT__H, scan_confirm->status));
    TRACE_MSG(TRACE_MAC_API1, "ScanType=%hd", (FMT__H, scan_confirm->scan_type));
    TRACE_MSG(TRACE_MAC_API1, "ChannelPage=0x%hx", (FMT__H, scan_confirm->channel_page));
    TRACE_MSG(TRACE_MAC_API1, "UnscannedChannels=0x%lx", (FMT__L, scan_confirm->unscanned_channels));

    if (scan_confirm->scan_type == ORPHAN_SCAN)
    {
        TRACE_MSG(TRACE_MAC_API1, "ResultListSize=0", (FMT__0));
    }
    else if (scan_confirm->scan_type == ED_SCAN)
    {
        TRACE_MSG(TRACE_MAC_API1, "ResultListSize=%hd", (FMT__H, scan_confirm->result_list_size));

        for (i = 0; i < scan_confirm->result_list_size; i++)
        {
            TRACE_MSG(TRACE_MAC_API1, "EnergyDetectList[%hd]=%hd",
                      (FMT__H_H, i, scan_confirm->list.energy_detect[i]));
        }
    }
    else if (scan_confirm->scan_type == ACTIVE_SCAN ||
             scan_confirm->scan_type == PASSIVE_SCAN)
    {
        if (MAC_PIB().mac_auto_request)
        {
            zb_pan_descriptor_t *pan_desc;

            TRACE_MSG(TRACE_MAC_API1, "ResultListSize=%hd", (FMT__H, scan_confirm->result_list_size));

            pan_desc = (zb_pan_descriptor_t *)zb_buf_begin(param);

            for (i = 0; i < scan_confirm->result_list_size; i++)
            {
                TRACE_MSG(TRACE_MAC_API1, "#######################", (FMT__0));
                TRACE_MSG(TRACE_MAC_API1, "PANDescriptor[%hd]:", (FMT__H, i));
                zb_mac_api_trace_pan_descriptor(pan_desc);
                TRACE_MSG(TRACE_MAC_API1, "#######################", (FMT__0));
                pan_desc++;
            }
        }
        else
        {
            TRACE_MSG(TRACE_MAC_API1, "ResultListSize=0", (FMT__0));
        }
    }
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

void zb_mac_api_trace_poll_request(zb_uint8_t param)
{
    zb_mlme_poll_request_t *poll_req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);


    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-POLL.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "CoordAddrMode=0x%hx", (FMT__H, poll_req->coord_addr_mode));
    TRACE_MSG(TRACE_MAC_API1, "CoordPANId=0x%x", (FMT__D, poll_req->coord_pan_id));
    if (poll_req->coord_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "CoordAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(poll_req->coord_addr.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "CoordAddr(short)=0x%x",
                  (FMT__D, poll_req->coord_addr.addr_short));
    }
    TRACE_MSG(TRACE_MAC_API1, "PollRate=%ld", (FMT__L, poll_req->poll_rate));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_poll_confirm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-POLL.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H, zb_buf_get_status(param)));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_start_request(zb_uint8_t param)
{
    zb_mlme_start_req_t *start_req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

    start_req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-START.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "PANId=0x%x", (FMT__D, start_req->pan_id));
    TRACE_MSG(TRACE_MAC_API1, "LogicalChannel=0x%x", (FMT__D, start_req->logical_channel));
    TRACE_MSG(TRACE_MAC_API1, "BeaconOrder=0x%x", (FMT__D, start_req->beacon_order));
    TRACE_MSG(TRACE_MAC_API1, "SuperframeOrder=0x%x", (FMT__D, start_req->superframe_order));
    TRACE_MSG(TRACE_MAC_API1, "PANCoordinator=0x%x", (FMT__D, start_req->pan_coordinator));
    TRACE_MSG(TRACE_MAC_API1, "BatteryLifeExtension=0x%x", (FMT__D, start_req->battery_life_extension));
    TRACE_MSG(TRACE_MAC_API1, "CoordinatorRealignment=0x%x", (FMT__D, start_req->coord_realignment));
    TRACE_MSG(TRACE_MAC_API1, "CoordinatorRealignmentSecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "BeaconSecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_start_confirm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-START.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H,   zb_buf_get_status(param)));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

void zb_mac_api_trace_set_request(zb_uint8_t param)
{
    zb_mlme_set_request_t *set_req = (zb_mlme_set_request_t *) zb_buf_begin(param);
    zb_uint8_t *ptr = (zb_uint8_t *)zb_buf_begin(param) + sizeof(zb_mlme_set_request_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-SET.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "PIBAttribute=0x%x", (FMT__D, set_req->pib_attr));
    TRACE_MSG(TRACE_MAC_API1, "PIBIndex=0x%x", (FMT__D, set_req->pib_index));
    TRACE_MSG(TRACE_MAC_API1, "PIBLength=0x%x", (FMT__D, set_req->pib_length));

    switch (set_req->pib_attr)
    {
    /*8-bit attributes*/
    case ZB_PHY_PIB_CURRENT_CHANNEL:
    case ZB_PHY_PIB_CURRENT_PAGE:
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_RETRIES:
    case ZB_PIB_ATTRIBUTE_RESPONSE_WAIT_TIME:
    case ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT:
    case ZB_PIB_ATTRIBUTE_BATT_LIFE_EXT:
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH:
    case ZB_PIB_ATTRIBUTE_BEACON_ORDER:
    case ZB_PIB_ATTRIBUTE_BSN:
    case ZB_PIB_ATTRIBUTE_DSN:
    case ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE:
    case ZB_PIB_ATTRIBUTE_AUTO_REQUEST:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *ptr));
        break;
    /*16-bit attributes*/
    case ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION:
    case ZB_PIB_ATTRIBUTE_TRANSACTION_PERSISTENCE_TIME:
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_TOTAL_WAIT_TIME:
    case ZB_PIB_ATTRIBUTE_PANID:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *((zb_uint16_t *)ptr)));
        break;
    /*Beacon payload attributes*/
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=", (FMT__0));
        mac_api_trace_dump(ptr, set_req->pib_length);
        break;
    /*16-bit address attributes*/
    case ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS:
    case ZB_PIB_ATTRIBUTE_SHORT_ADDRESS:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *((zb_uint16_t *)ptr)));
        break;
    /*64-bit address attributes*/
    case ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS:
    case ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=" TRACE_FORMAT_64, (FMT__A,
                  TRACE_ARG_64((zb_uint8_t *)ptr)));
        break;
    default:
        break;
    }
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_set_confirm(zb_uint8_t param)
{
    zb_mlme_set_confirm_t *set_conf = (zb_mlme_set_confirm_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-SET.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H, set_conf->status));
    TRACE_MSG(TRACE_MAC_API1, "PIBAttribute=0x%x", (FMT__D, set_conf->pib_attr));
    TRACE_MSG(TRACE_MAC_API1, "PIBIndex=0x%x", (FMT__D, set_conf->pib_index));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}

void zb_mac_api_trace_purge_request(zb_uint8_t param)
{
    zb_mlme_purge_request_t *purge_req = ZB_BUF_GET_PARAM(param, zb_mlme_purge_request_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MCPS-PURGE.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "msduHandle=0x%x", (FMT__D, purge_req->msdu_handle));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_purge_confirm(zb_uint8_t param)
{
    zb_mlme_purge_confirm_t *purge_conf = ZB_BUF_GET_PARAM(param, zb_mlme_purge_confirm_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MCPS-PURGE.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "msduHandle=0x%x", (FMT__D, purge_conf->msdu_handle));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H,   zb_buf_get_status(param)));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_data_request(zb_uint8_t param)
{
    zb_mcps_data_req_params_t  *data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
    zb_uint8_t *msdu = zb_buf_begin(param);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MCPS-DATA.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "SrcAddrMode=0x%hx", (FMT__H, data_req->src_addr_mode));
    if (data_req->src_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(data_req->src_addr.addr_long)));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(short)=0x%x",
                  (FMT__D, data_req->src_addr.addr_short));
    }

    TRACE_MSG(TRACE_MAC_API1, "DstAddrMode=0x%hx", (FMT__H, data_req->dst_addr_mode));
    if (data_req->dst_addr_mode == ZB_ADDR_64BIT_DEV)
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(data_req->dst_addr.addr_long)));
    }
    else if (data_req->dst_addr_mode == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(short)=0x%x", (FMT__D, data_req->dst_addr.addr_short));
    }
    TRACE_MSG(TRACE_MAC_API1, "DstPANId=0x%x", (FMT__D, data_req->dst_pan_id));
    TRACE_MSG(TRACE_MAC_API1, "TxOptions=0x%hd", (FMT__H, data_req->tx_options));
    TRACE_MSG(TRACE_MAC_API1, "msduHandle=0x%hx", (FMT__H, data_req->msdu_handle));
    TRACE_MSG(TRACE_MAC_API1, "msduLength=%hd", (FMT__H, zb_buf_len(param)));
    TRACE_MSG(TRACE_MAC_API1, "msdu = ", (FMT__0));
    mac_api_trace_dump(msdu, zb_buf_len(param));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_data_confirm(zb_uint8_t param)
{
    zb_mcps_data_confirm_params_t *data_conf = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MCPS-DATA.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "msduHandle=0x%x", (FMT__D, data_conf->msdu_handle));
    TRACE_MSG(TRACE_MAC_API1, "Status=0x%x", (FMT__H,   zb_buf_get_status(param)));
    TRACE_MSG(TRACE_MAC_API1, "Timestamp=0x%lx", (FMT__L, data_conf->timestamp));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_data_indication(zb_uint8_t param)
{
    zb_mac_mhr_t mhr;
    zb_uint8_t mhr_size = zb_parse_mhr(&mhr, param);
    zb_uint8_t rssi = 0;
    zb_uint8_t lqi = 0;
    zb_uint8_t *msdu = (zb_uint8_t *)zb_buf_begin(param) + mhr_size;
    zb_uint8_t msdu_length = 0;
    zb_time_t timestamp = *ZB_BUF_GET_PARAM(param, zb_time_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MCPS-DATA.indication()", (FMT__0));

    TRACE_MSG(TRACE_MAC_API1, "SrcAddrMode=0x%hx", (FMT__H, ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control)));
    TRACE_MSG(TRACE_MAC_API1, "SrcPANId=0x%x", (FMT__D, mhr.src_pan_id));
    if (ZB_FCF_GET_SRC_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(short)=0x%x", (FMT__D, mhr.src_addr.addr_short));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "SrcAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(mhr.src_addr.addr_long)));
    }

    TRACE_MSG(TRACE_MAC_API1, "DstAddrMode=0x%hx", (FMT__H, ZB_FCF_GET_DST_ADDRESSING_MODE(mhr.frame_control)));
    TRACE_MSG(TRACE_MAC_API1, "DstPANId=0x%x", (FMT__D, mhr.dst_pan_id));
    if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr.frame_control) == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(short)=0x%x", (FMT__D, mhr.dst_addr.addr_short));
    }
    else
    {
        TRACE_MSG(TRACE_MAC_API1, "DstAddr(IEEE)=" TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(mhr.dst_addr.addr_long)));
    }

    msdu_length = zb_buf_len(param) - mhr_size - ZB_TAIL_SIZE_FOR_RECEIVED_MAC_FRAME;
    TRACE_MSG(TRACE_MAC_API1, "msduLength=%hd", (FMT__H, msdu_length));
    TRACE_MSG(TRACE_MAC_API1, "msdu = ", (FMT__0));
    mac_api_trace_dump(msdu, msdu_length);

    rssi = ZB_MAC_GET_RSSI(param);
    lqi = ZB_MAC_GET_LQI(param);

    TRACE_MSG(TRACE_MAC_API1, "RSSI=%hd", (FMT__H, rssi));
    TRACE_MSG(TRACE_MAC_API1, "LQI=%hd", (FMT__H, lqi));
    TRACE_MSG(TRACE_MAC_API1, "DSN=%hd", (FMT__H, mhr.seq_number));

    TRACE_MSG(TRACE_MAC_API1, "Timestamp=0x%lx", (FMT__L, timestamp));
    TRACE_MSG(TRACE_MAC_API1, "SecurityLevel=0", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}



void zb_mac_api_trace_get_request(zb_uint8_t param)
{
    zb_mlme_get_request_t *get_req = (zb_mlme_get_request_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-GET.request()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "PIBAttribute=0x%x", (FMT__D, get_req->pib_attr));
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


void zb_mac_api_trace_get_confirm(zb_uint8_t param)
{
    zb_mlme_get_confirm_t *get_conf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
    zb_uint8_t *ptr = (zb_uint8_t *)zb_buf_begin(param) + sizeof(zb_mlme_get_confirm_t);

    TRACE_MSG(TRACE_MAC_API1, ">>>>>>>>>>>>>>>>>>>>", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "MLME-GET.confirm()", (FMT__0));
    TRACE_MSG(TRACE_MAC_API1, "PIBAttribute=0x%x", (FMT__D, get_conf->pib_attr));
    TRACE_MSG(TRACE_MAC_API1, "PIBLength=0x%x", (FMT__D, get_conf->pib_length));
    TRACE_MSG(TRACE_MAC_API1, "PIBIndex=0x%x", (FMT__D, get_conf->pib_index));
    switch (get_conf->pib_attr)
    {
    /*8-bit attributes*/
    case ZB_PHY_PIB_CURRENT_CHANNEL:
    case ZB_PHY_PIB_CURRENT_PAGE:
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_RETRIES:
    case ZB_PIB_ATTRIBUTE_RESPONSE_WAIT_TIME:
    case ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT:
    case ZB_PIB_ATTRIBUTE_BATT_LIFE_EXT:
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH:
    case ZB_PIB_ATTRIBUTE_BEACON_ORDER:
    case ZB_PIB_ATTRIBUTE_BSN:
    case ZB_PIB_ATTRIBUTE_DSN:
    case ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE:
    case ZB_PIB_ATTRIBUTE_AUTO_REQUEST:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *ptr));
        break;
    /*16-bit attributes*/
    case ZB_PIB_ATTRIBUTE_ACK_WAIT_DURATION:
    case ZB_PIB_ATTRIBUTE_TRANSACTION_PERSISTENCE_TIME:
    case ZB_PIB_ATTRIBUTE_MAX_FRAME_TOTAL_WAIT_TIME:
    case ZB_PIB_ATTRIBUTE_PANID:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *((zb_uint16_t *)ptr)));
        break;
    /*Beacon payload attributes*/
    case ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD:
        TRACE_MSG(TRACE_MAC_API1, "Beacon payload = ", (FMT__0));
        mac_api_trace_dump(ptr, MAC_PIB().mac_beacon_payload_length);
        break;
    /*16-bit address attributes*/
    case ZB_PIB_ATTRIBUTE_COORD_SHORT_ADDRESS:
    case ZB_PIB_ATTRIBUTE_SHORT_ADDRESS:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=0x%x", (FMT__D, *((zb_uint16_t *)ptr)));
        break;
    /*64-bit address attributes*/
    case ZB_PIB_ATTRIBUTE_COORD_EXTEND_ADDRESS:
    case ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS:
        TRACE_MSG(TRACE_MAC_API1, "PIBAttributeValue=" TRACE_FORMAT_64, (FMT__A,
                  TRACE_ARG_64((zb_uint8_t *)ptr)));
        break;
    default:
        break;

    }
    TRACE_MSG(TRACE_MAC_API1, "<<<<<<<<<<<<<<<<<<<<", (FMT__0));
}


#endif  /* ZB_MAC_API_TRACE_PRIMITIVES && !ZB_MACSPLIT_HOST */
