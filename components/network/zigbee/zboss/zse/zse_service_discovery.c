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
/*  PURPOSE: SE Service Discovery.
*/

#define ZB_TRACE_FILE_ID 39
#include "zb_common.h"

#ifdef ZB_ENABLE_SE

#if defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING)

#include "zb_se.h"
#include "zcl/zb_zcl_price.h"
#include "zcl/zb_zcl_mdu_pairing.h"

/* Send unicast Match Desc to 0x0000 even in non-MDU mode */
/* #define ZSE_NON_MDU_UNICAST_MATCH_DESC */

void zb_se_service_discovery_process_stored_devs(zb_uint8_t param);

void zb_se_service_discovery_match_desc_req(zb_uint8_t param);

void zb_se_service_discovery_match_desc_req_delayed(zb_uint8_t param);

void zb_se_service_discovery_get_mdu_short_addresses(zb_uint8_t param);

static void se_bind_complete_cb(zb_uint8_t param);

/* FIXME: Unclear points (according to spec):
   - MDU Pairing cluster - if it exists, should use unicast service discovery to specified devices
   -- This cluster is not implemented in stack
   - Apply service discovery only to clusters which support "asynchronous event commands"
   -- Unclear which clusters supports it, now discovery servers for all our client clusters
   - Discover devices on the network "that have services that match with the device's"
   -- Unclear description, now discovery servers for all our client clusters
   - "If device is not an ESI and the ESI are the matching device(s), the device should send binding
      requests to all ESI with matching services."
   -- Unclear what should ESI do - nothing? Looks like only non-ESI devices should start service
      discovery.
 */

/* TODO: For all cases - handle bdb states correctly, embed into SE commissioning machine. */

/* TODO: Repeat discovery phase periodically (3-24 hrs), repeat after R&R. */

/* FIXME: Is it enough timeout for cluster discovery?
   In the worst case - match descr req-resp, read attr-read attr resp, ieee addr req-resp, bind
   req-resp.
*/


/* TRICKY: Do not additionally store client cluster list - in simple descriptor client clusters
 * are stored after server clusters. */
zb_zcl_cluster_desc_t *zb_se_service_discovery_get_current_cluster()
{
    zb_af_endpoint_desc_t *ep_desc = zb_af_get_endpoint_desc(ZSE_CTXC().service_disc.source_ep);
    if (ep_desc
            && ZSE_CTXC().service_disc.current_cluster < ep_desc->simple_desc->app_output_cluster_count)
    {
        TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_get_current_cluster: descr_n %d", (FMT__D, ep_desc->simple_desc->app_input_cluster_count + ZSE_CTXC().service_disc.current_cluster));
        {
            zb_zcl_cluster_desc_t *cl_desc = &ep_desc->cluster_desc_list[ep_desc->simple_desc->app_input_cluster_count + ZSE_CTXC().service_disc.current_cluster];
            TRACE_MSG(TRACE_ZCL1, "cluster_id = %d role = %hd", (FMT__D_H, cl_desc->cluster_id, cl_desc->role_mask ));
        }
        return &ep_desc->cluster_desc_list[ep_desc->simple_desc->app_input_cluster_count + ZSE_CTXC().service_disc.current_cluster];
    }
    return NULL;
}

zb_zcl_cluster_desc_t *zb_se_service_discovery_get_mdu_cluster()
{
    zb_af_endpoint_desc_t *ep_desc = zb_af_get_endpoint_desc(ZSE_CTXC().service_disc.source_ep);

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_get_mdu_cluster", (FMT__0));

    return get_cluster_desc(ep_desc,
                            ZB_ZCL_CLUSTER_ID_MDU_PAIRING,
                            ZB_ZCL_CLUSTER_CLIENT_ROLE);
}


void zb_se_service_discovery_reset_dev()
{
    zb_uint8_t i = 0;

    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        ZSE_CTXC().service_disc.devices[i].step = ZSE_SERVICE_DISC_INVALID;
        ++i;
    }
}

zb_ret_t zb_se_service_discovery_store_dev(zb_uint16_t short_addr, zb_uint8_t ep, zb_uint16_t cluster_id)
{
    zb_uint8_t i = 0;
    zb_ret_t ret = RET_NO_MEMORY;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_store_dev: short_addr %d ep %hd cluster %d", (FMT__D_H_D, short_addr, ep, cluster_id));

    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        if (ZSE_CTXC().service_disc.devices[i].step == ZSE_SERVICE_DISC_INVALID)
        {
            ret = RET_OK;
            ZSE_CTXC().service_disc.devices[i].ep = ep;
            ZSE_CTXC().service_disc.devices[i].short_addr = short_addr;
            ZSE_CTXC().service_disc.devices[i].cluster_id = cluster_id;
            ZSE_CTXC().service_disc.devices[i].step = ZSE_SERVICE_DISC_FIRST_STEP;
            ZSE_CTXC().service_disc.devices[i].op_in_progress = 0;
            break;
        }
        ++i;
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_store_dev ret %hd", (FMT__H, ret));
    return ret;
}


zb_uint8_t zb_se_service_discovery_get_next_dev()
{
    zb_uint8_t i = 0;
    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        if (ZSE_CTXC().service_disc.devices[i].step != ZSE_SERVICE_DISC_INVALID
                && !ZSE_CTXC().service_disc.devices[i].op_in_progress)
        {
            TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_get_next_dev: dev %hd step %hd", (FMT__H_H, i, ZSE_CTXC().service_disc.devices[i].step));
            return i;
        }
        ++i;
    }
    return 0xFF;
}


zb_uint8_t zb_se_service_discovery_get_dev_idx(zb_uint16_t short_addr, zb_uint8_t ep)
{
    zb_uint8_t i = 0;
    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        if (ZSE_CTXC().service_disc.devices[i].step != ZSE_SERVICE_DISC_INVALID
                && ZSE_CTXC().service_disc.devices[i].short_addr == short_addr
                && ZSE_CTXC().service_disc.devices[i].ep == ep)
        {
            TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_get_dev_idx_by_short_addr: dev %hd step %hd", (FMT__H_H, i, ZSE_CTXC().service_disc.devices[i].step));
            return i;
        }
        ++i;
    }
    return 0xFF;
}


void zb_se_service_discovery_op_completed(zb_uint16_t short_addr, zb_uint8_t ep, zb_uint16_t cluster_id, zse_service_disc_step_t step)
{
    zb_uint8_t i = 0;

    TRACE_MSG(TRACE_ZDO1, "zb_se_service_discovery_op_completed addr 0x%x ep %hd cluster_id %d step %d ", (FMT__D_H_D_D, short_addr, ep, cluster_id, step));
    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        if (ZSE_CTXC().service_disc.devices[i].ep != 0xFF)
        {
            TRACE_MSG(TRACE_ZCL3, "devices[%d].short_addr 0x%x ep %hd clusted_id %d step %d op_in_progress %hd",
                      (FMT__D_H_D_H_H, i, ZSE_CTXC().service_disc.devices[i].short_addr,
                       ZSE_CTXC().service_disc.devices[i].ep,
                       ZSE_CTXC().service_disc.devices[i].cluster_id,
                       ZSE_CTXC().service_disc.devices[i].step,
                       ZSE_CTXC().service_disc.devices[i].op_in_progress));
        }

        if (ZSE_CTXC().service_disc.devices[i].short_addr == short_addr
                && ((ZSE_CTXC().service_disc.devices[i].ep == ep
                     && ZSE_CTXC().service_disc.devices[i].cluster_id == cluster_id)
                    || step == ZSE_SERVICE_DISC_DISCOVER_IEEE
                    || step == ZSE_SERVICE_DISC_GEN_KEY
                    || step == ZSE_SERVICE_DISC_BIND)
                && (ZSE_CTXC().service_disc.devices[i].step == step
                    /* Not too funny... We established APS key after bind. Call main logic. */
                    || (ZSE_CTXC().service_disc.devices[i].step == ZSE_SERVICE_DISC_BIND
                        && step == ZSE_SERVICE_DISC_GEN_KEY))
                && ZSE_CTXC().service_disc.devices[i].op_in_progress)
        {
            TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_op_completed: dev %hd step %hd", (FMT__H_H, i, ZSE_CTXC().service_disc.devices[i].step));
            ++ZSE_CTXC().service_disc.devices[i].step;
            ZSE_CTXC().service_disc.devices[i].op_in_progress = 0;
            /* break; */
        }
        ++i;
    }
}

zb_bool_t zb_se_service_discovery_op_in_progress(zb_uint16_t short_addr, zse_service_disc_step_t step)
{
    zb_uint8_t i = 0;

    TRACE_MSG(TRACE_ZDO1, "zb_se_service_discovery_op_in_progress addr 0x%x step %hd", (FMT__D_H, short_addr, step));
    while (i < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices))
    {
        TRACE_MSG(TRACE_ZCL3, "devices[%d].short_addr 0x%x ep %hd clusted_id %d step %d op_in_progress %hd",
                  (FMT__D_H_H_H, i, ZSE_CTXC().service_disc.devices[i].short_addr,
                   ZSE_CTXC().service_disc.devices[i].ep,
                   ZSE_CTXC().service_disc.devices[i].step,
                   ZSE_CTXC().service_disc.devices[i].op_in_progress));

        if (ZSE_CTXC().service_disc.devices[i].short_addr == short_addr &&
                ZSE_CTXC().service_disc.devices[i].step == step &&
                ZSE_CTXC().service_disc.devices[i].op_in_progress)
        {
            return ZB_TRUE;
        }
        ++i;
    }
    return ZB_FALSE;
}

void zb_se_service_discovery_send_start_signal_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(zb_se_service_discovery_send_start_signal);
}

void zb_se_service_discovery_send_start_signal(zb_uint8_t param)
{

    zb_app_signal_pack(param,
                       ZB_SE_SIGNAL_SERVICE_DISCOVERY_START,
                       zb_buf_get_status(param),
                       0);

    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}

static void zb_se_service_discovery_send_bind_signal(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_se_signal_service_discovery_bind_params_t *bind_params;
    zse_service_disc_dev_t *dev;

    ZB_ASSERT(user_param < ZB_ARRAY_SIZE(ZSE_CTXC().service_disc.devices));
    dev = &ZSE_CTXC().service_disc.devices[user_param];

    if (dev->step == ZSE_SERVICE_DISC_BIND)
    {
        bind_params =
            (zb_se_signal_service_discovery_bind_params_t *)zb_app_signal_pack(param,
                    ZB_SE_SIGNAL_SERVICE_DISCOVERY_DO_BIND,
                    RET_OK,
                    sizeof(zb_se_signal_service_discovery_bind_params_t));
        bind_params->endpoint = dev->ep;
        bind_params->cluster_id = dev->cluster_id;
        bind_params->commodity_type = (dev->cluster_id == ZB_ZCL_CLUSTER_ID_METERING ||
                                       dev->cluster_id == ZB_ZCL_CLUSTER_ID_PRICE) ?
                                      dev->commodity_type : 0;
        if (zb_address_ieee_by_short(dev->short_addr, bind_params->device_addr) != RET_OK)
        {
            ZB_ASSERT(0);
        }
        ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
    }
    else
    {
        zb_buf_free(param);
    }
}

void zb_se_service_discovery_bind_req(zb_uint8_t param, zb_ieee_addr_t dst_ieee, zb_uint16_t dst_ep)
{
    zb_zdo_bind_req_param_t *req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    zb_zcl_cluster_desc_t *cluster_desc;
    zb_uint16_t dst_short = zb_address_short_by_ieee(dst_ieee);

    ZB_ASSERT(dst_short != ZB_UNKNOWN_SHORT_ADDR);
    cluster_desc = zb_se_service_discovery_get_current_cluster();

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_bind_req dst_short 0x%x param %hd ep %hd", (FMT__D_H_H, dst_short, param, dst_ep));

    ZB_ASSERT(cluster_desc);
    ZB_MEMCPY(&req->src_address, dst_ieee, sizeof(zb_ieee_addr_t));
    req->src_endp = dst_ep;
    req->cluster_id = cluster_desc->cluster_id;
    ZB_MEMCPY(&req->dst_address.addr_long, ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));
    req->dst_endp = ZSE_CTXC().service_disc.source_ep;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->req_dst_addr = dst_short;

    /* No matter is bind ok or not - server side should care about client notifying if it answers
     * with NOT_SUPPORTED:
     * 5.5.5.2 Service Discovery State
     * Once a device has issued the binding request,  it  does  not  need  to  receive  a  binding
     * response  success.  If  the  device  receives  a NOT_SUPPORTED (or other non-success code)
     * response to a cluster device bind request, it should still send binding requests for any
     * remaining clusters that it has not sent already.
     */
    /* Let's first bind then' if succeed, establish APS key with another device. */
    zb_zdo_bind_req(param, se_bind_complete_cb);

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_bind_req", (FMT__0));
}


static void se_bind_complete_cb(zb_uint8_t param)
{
    zb_apsde_data_indication_t *ind = (zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);
    zb_uint16_t addr = ind->src_addr;

    if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        if (ind->src_addr != 0 && !zb_se_has_valid_key(ind->src_addr))
        {
            TRACE_MSG(TRACE_ZCL3, "Bound ok. Have no APS key for dev 0x%x - try to establish it", (FMT__D, addr));
            /* Note: APS key establishment via TC does not use CBKE and can run in parallel. */
            ZB_SCHEDULE_CALLBACK2(zb_se_start_aps_key_establishment, param, ind->src_addr);
        }
        else if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY)
        {
            TRACE_MSG(TRACE_ZCL3, "bind ok, dev 0x%x - continue discovery", (FMT__D, addr));
            zb_se_service_discovery_op_completed(ind->src_addr, 0, 0, ZSE_SERVICE_DISC_BIND);
            ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_process_stored_devs, param);
        }
        else
        {
            zb_uint16_t *addr_p = (zb_uint16_t *)zb_app_signal_pack(param,
                                  ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_OK,
                                  RET_OK,
                                  2);
            TRACE_MSG(TRACE_ZCL3, "bind ok, dev 0x%x - inform user", (FMT__D, addr));
            *addr_p = addr;
            ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
        }
    }
    else
    {
        zb_uint16_t *addr_p;
        TRACE_MSG(TRACE_ZCL3, "Failed to bind, dev 0x%x status %hd - inform user", (FMT__D_H, addr, bind_resp->status));
        addr_p = (zb_uint16_t *)zb_app_signal_pack(param,
                 ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_FAILED,
                 RET_OK,
                 2);
        *addr_p = addr;
        ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
        if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY)
        {
            /* Continue discovery */
            zb_buf_get_out_delayed(zb_se_service_discovery_process_stored_devs);
        }
    }
}

/*
Note: after bind need to establish APS keys between devices (if not bound to TC itself).
See 5.4.7.4 After Joining:


The originating node would start this process by sending a bind
request command with APS ack 2098 to the Key Establishment cluster of
the destination device. If a bind confirm is received with a 2099
status of success, the initiating device will perform a request key of
the trust center (for an 2100 application link key using the EUI of
the other device in the pair). The trust center will then 2101 send a
link key to each device using the key transport. If the bind confirm
is received with a 2102 status other than success, the request key
should not be sent to the trust center.

I am not sure: is it for Key Establishment cluster only??
 */


void zb_se_service_discovery_ieee_addr_req_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp = (zb_zdo_nwk_addr_resp_head_t *)(zb_buf_begin(param));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_se_service_discovery_op_completed(resp->nwk_addr, 0, 0, ZSE_SERVICE_DISC_DISCOVER_IEEE);
        ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_process_stored_devs, param);
    }
    else
    {
        zb_buf_free(param);
    }
}


zb_bool_t zb_se_service_discovery_block_zcl_cmd(zb_uint16_t short_addr, zb_uint8_t ep, zb_uint8_t zcl_tsn)
{
    zb_uint8_t dev_idx = zb_se_service_discovery_get_dev_idx(short_addr, ep);
    TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_block_zcl_cmd: addr %d ep %hd tsn %hd", (FMT__D_H_H, short_addr, ep, zcl_tsn));
    return (zb_bool_t)(dev_idx != 0xFF && ZSE_CTXC().service_disc.devices[dev_idx].zcl_tsn == zcl_tsn);
}


zb_bool_t zb_se_service_discovery_read_commodity_attr_handle(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_ret_t ret = RET_NOT_FOUND;
    zb_zcl_read_attr_res_t *resp = NULL;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_read_commodity_attr_handle param %hd", (FMT__H, param));

    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);

    if (resp && resp->status == ZB_ZCL_STATUS_SUCCESS)
    {
        if ((cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_METERING &&
                resp->attr_id == ZB_ZCL_ATTR_METERING_METERING_DEVICE_TYPE_ID) ||
                (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_PRICE &&
                 resp->attr_id == ZB_ZCL_ATTR_PRICE_SRV_COMMODITY_TYPE))
        {
            zb_uint8_t dev_idx = zb_se_service_discovery_get_dev_idx(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint);

            TRACE_MSG(TRACE_ZCL1, "commodity: src_addr %d ep %hd value %hd", (FMT__D_H, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint, resp->attr_value[0]));

            if (dev_idx != 0xFF)
            {
                ZSE_CTXC().service_disc.devices[dev_idx].commodity_type = resp->attr_value[0];
                ret = RET_OK;
            }
        }
    }

    if (ret == RET_OK)
    {
        zb_se_service_discovery_op_completed(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint, cmd_info->cluster_id, ZSE_SERVICE_DISC_READ_COMMODITY);
        ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_process_stored_devs, param);
        param = 0;
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_read_commodity_attr_handle", (FMT__0));

    return (zb_bool_t)(!param);
}

#if defined ZB_ENABLE_TIME_SYNC
static void zb_se_time_sync_inform_user_about_time_synch(zb_uint8_t param)
{
    if (!param)
    {
        zb_buf_get_out_delayed(zb_se_time_sync_inform_user_about_time_synch);
    }
    else
    {
        zb_zcl_time_sync_payload_t pl_in;

        TRACE_MSG(TRACE_ZCL1, "The most authoritative time server: ep=%d, addr=0x%x, auth_level=%d, time=%ld(0x%lx)",
                  (FMT__D_D_D_L_L, ZSE_CTXC().time_server.server_ep,
                   ZSE_CTXC().time_server.server_short_addr,
                   ZSE_CTXC().time_server.server_auth_level,
                   ZSE_CTXC().time_server.time,
                   ZSE_CTXC().time_server.time));

        TRACE_MSG(TRACE_ZCL1, "The second authoritative time server: ep=%d, addr=0x%x, auth_level=%d, time=%ld(0x%lx)",
                  (FMT__D_D_D_L_L, ZSE_CTXC().second_time_server.server_ep,
                   ZSE_CTXC().second_time_server.server_short_addr,
                   ZSE_CTXC().second_time_server.server_auth_level,
                   ZSE_CTXC().second_time_server.time,
                   ZSE_CTXC().second_time_server.time));

        if ((ZB_ZCL_TIME_SERVER_NOT_CHOSEN != ZSE_CTXC().time_server.server_auth_level) &&
                (ZB_ZCL_TIME_TIME_INVALID_VALUE != ZSE_CTXC().time_server.time))
        {
            TRACE_MSG(TRACE_ZCL1, "Sync time with 0x%x ep %hd, choose 0x%lx time",
                      (FMT__D_D_L,
                       ZSE_CTXC().time_server.server_short_addr,
                       ZSE_CTXC().time_server.server_ep,
                       ZSE_CTXC().time_server.time));

            /* use rounding to high value */
            pl_in.time = ZSE_CTXC().time_server.time + (ZB_TIMER_GET() - ZSE_CTXC().time_server.receiving_time) / ZB_TIME_ONE_SECOND + 1;
            pl_in.addr = ZSE_CTXC().time_server.server_short_addr;
            pl_in.endpoint = ZSE_CTXC().time_server.server_ep;
            pl_in.level = ZSE_CTXC().time_server.server_auth_level;

            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_TIME_SYNC_CB_ID,
                                              RET_OK, NULL, &pl_in, NULL);
        }
        else if ((ZB_ZCL_TIME_SERVER_NOT_CHOSEN != ZSE_CTXC().second_time_server.server_auth_level) &&
                 (ZB_ZCL_TIME_TIME_INVALID_VALUE != ZSE_CTXC().second_time_server.time))
        {
            TRACE_MSG(TRACE_ZCL1, "Sync time with 0x%x ep %hd, choose 0x%lx time",
                      (FMT__D_D_L,
                       ZSE_CTXC().second_time_server.server_short_addr,
                       ZSE_CTXC().second_time_server.server_ep,
                       ZSE_CTXC().second_time_server.time));

            pl_in.time = ZSE_CTXC().second_time_server.time + (ZB_TIMER_GET() - ZSE_CTXC().second_time_server.receiving_time) / ZB_TIME_ONE_SECOND + 1;
            pl_in.addr = ZSE_CTXC().second_time_server.server_short_addr;
            pl_in.endpoint = ZSE_CTXC().second_time_server.server_ep;
            pl_in.level = ZSE_CTXC().second_time_server.server_auth_level;

            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_TIME_SYNC_CB_ID,
                                              RET_OK, NULL, &pl_in, NULL);
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "Can't synchronize network time: no authoritative servers or invalid time",
                      (FMT__0));

            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_TIME_SYNC_FAILED_CB_ID,
                                              RET_OK, NULL, NULL, NULL);
        }

        if (ZCL_CTX().device_cb)
        {
            ZCL_CTX().device_cb(param);
        }

        zb_buf_free(param);

        ZSE_CTXC().time_server.server_auth_level = ZB_ZCL_TIME_SERVER_NOT_CHOSEN;
        ZSE_CTXC().second_time_server.server_auth_level = ZB_ZCL_TIME_SERVER_NOT_CHOSEN;
    }
}

static void zb_se_service_discovery_read_time_status(zb_uint8_t param, zb_uint16_t short_addr, zb_uint8_t ep)
{
    zb_uint8_t *cmd_ptr = NULL;

    TRACE_MSG(TRACE_ZCL1, "read time attrs from 0x%hx ep %d ", (FMT__D_D, short_addr, ep));
    TRACE_MSG(TRACE_ZCL1, "read time status: attr %d ", (FMT__D, ZB_ZCL_ATTR_TIME_TIME_STATUS_ID));
    TRACE_MSG(TRACE_ZCL1, "read time: attr %d ", (FMT__D, ZB_ZCL_ATTR_TIME_TIME_ID));

    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_STATUS_ID);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_TIME_TIME_ID);

    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(param,
                                      cmd_ptr,
                                      short_addr,
                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      ep,
                                      ZSE_CTXC().service_disc.source_ep,
                                      get_profile_id_by_endpoint(ZSE_CTXC().service_disc.source_ep),
                                      ZB_ZCL_CLUSTER_ID_TIME,
                                      NULL);
}


#define ZB_TIME_COMPARE_AUTH_LEVEL(new_level, new_short_addr, old_level, old_short_addr)  \
        (((new_level) > (old_level)) ||                                                   \
         (((old_level) > ZB_ZCL_TIME_SERVER_NOT_CHOSEN) &&                                \
          ((new_level) == (old_level)) &&                                                 \
          ((new_short_addr) < (old_short_addr))))


zb_uint8_t zb_zcl_time_sync_get_auth_level(zb_uint8_t time_status, zb_uint16_t short_addr)
{
    if (ZB_ZCL_TIME_TIME_STATUS_MASTER_BIT_IS_SET(time_status) && ZB_ZCL_TIME_TIME_STATUS_SUPERSEDING_BIT_IS_SET(time_status))
    {
        return ((0x0000 == short_addr) ?
                ZB_ZCL_TIME_COORDINATOR_WITH_MASTER_AND_SUPERSEDING_BITS :
                ZB_ZCL_TIME_HAS_MASTER_AND_SUPERSEDING_BITS);
    }
    else if (ZB_ZCL_TIME_TIME_STATUS_MASTER_BIT_IS_SET(time_status))
    {
        return ZB_ZCL_TIME_HAS_MASTER_BIT;
    }
    else if (ZB_ZCL_TIME_TIME_STATUS_SYNCHRONIZED_BIT_IS_SET(time_status))
    {
        return ZB_ZCL_TIME_HAS_SYNCHRONIZED_BIT;
    }
    else
    {
        return ZB_ZCL_TIME_SERVER_NOT_CHOSEN;
    }
}


/* Compare new time server with already stored time servers */
static void zb_zcl_time_sync_choose_time_server(zb_uint32_t auth_level, zb_uint16_t short_addr,
        zb_uint8_t ep, zb_uint32_t nw_time)
{
    if (ZB_TIME_COMPARE_AUTH_LEVEL(auth_level,
                                   short_addr,
                                   ZSE_CTXC().time_server.server_auth_level,
                                   ZSE_CTXC().time_server.server_short_addr))
    {
        if (ZB_TIME_COMPARE_AUTH_LEVEL(ZSE_CTXC().time_server.server_auth_level,
                                       ZSE_CTXC().time_server.server_short_addr,
                                       ZSE_CTXC().second_time_server.server_auth_level,
                                       ZSE_CTXC().second_time_server.server_short_addr))
        {
            TRACE_MSG(TRACE_ZCL1, "Find the second authoritative time server", (FMT__0));

            ZB_MEMCPY(&ZSE_CTXC().second_time_server, &ZSE_CTXC().time_server, sizeof(ZSE_CTXC().time_server));
        }

        ZSE_CTXC().time_server.server_auth_level = auth_level;
        ZSE_CTXC().time_server.server_ep = ep;
        ZSE_CTXC().time_server.server_short_addr = short_addr;
        ZSE_CTXC().time_server.time = nw_time;
        ZSE_CTXC().time_server.receiving_time = ZB_TIMER_GET();

        TRACE_MSG(TRACE_ZCL1, "Find authoritative time server", (FMT__0));
    }
    else if (ZB_TIME_COMPARE_AUTH_LEVEL(auth_level,
                                        short_addr,
                                        ZSE_CTXC().second_time_server.server_auth_level,
                                        ZSE_CTXC().second_time_server.server_short_addr))
    {
        TRACE_MSG(TRACE_ZCL1, "Find the second authoritative time server", (FMT__0));

        ZSE_CTXC().second_time_server.server_auth_level = auth_level;
        ZSE_CTXC().second_time_server.server_ep = ep;
        ZSE_CTXC().second_time_server.server_short_addr = short_addr;
        ZSE_CTXC().second_time_server.time = nw_time;
        ZSE_CTXC().second_time_server.receiving_time = ZB_TIMER_GET();
    }
}


/* Read TimeStatus attribute and choose the most authoritative time server*/
zb_bool_t zb_se_service_discovery_read_time_attrs_handle(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_bool_t processed = ZB_FALSE;
    zb_zcl_read_attr_res_t *resp = NULL;

    /* Not sure 0 is correct. But what if no ZB_ZCL_ATTR_TIME_TIME_ID attr?  */
    zb_uint8_t time_status = 0;
    zb_uint32_t nw_time = 0;
    zb_uint8_t fails = 0;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_read_time_attrs_handle param %hd", (FMT__H, param));

    if (cmd_info->cluster_id != ZB_ZCL_CLUSTER_ID_TIME)
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid cluster id %ld", (FMT__H, cmd_info->cluster_id));

        return ZB_FALSE;
    }

    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);

    while (resp)
    {
        if (resp->status != ZB_ZCL_STATUS_SUCCESS)
        {
            TRACE_MSG(TRACE_ZCL1, "Attribute read error: attr 0x%x, status %d", (FMT__D_D, resp->attr_id, resp->status));
            ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);
            fails++;

            continue;
        }

        TRACE_MSG(TRACE_ZCL1, "read attr resp: src_addr 0x%x ep %hd",
                  (FMT__D_D, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint));

        if (ZB_ZCL_ATTR_TIME_TIME_STATUS_ID == resp->attr_id)
        {
            time_status = resp->attr_value[0];
            TRACE_MSG(TRACE_ZCL1, "Time status attr value: 0x%hx", (FMT__H, time_status));
        }
        else if (ZB_ZCL_ATTR_TIME_TIME_ID == resp->attr_id)
        {
            nw_time = ZB_ZCL_ATTR_GET32(resp->attr_value);
            TRACE_MSG(TRACE_ZCL1, "Time attr value: %ld", (FMT__L, nw_time));
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "Unknown attribute: attr 0x%x", (FMT__D, resp->attr_id));
            fails++;
        }

        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);
    }


    if (0 == fails)
    {
        zb_uint8_t curr_auth_level;

        curr_auth_level = zb_zcl_time_sync_get_auth_level(time_status,
                          ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr);

        zb_zcl_time_sync_choose_time_server(curr_auth_level,
                                            ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                            ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                            nw_time);

        zb_se_service_discovery_op_completed(ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                             ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                             cmd_info->cluster_id, ZSE_SERVICE_DISC_GET_TIME_SERVER);

        ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_process_stored_devs, param);
        processed = ZB_TRUE;
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "Mallformed Read Time Status attribute response", (FMT__H, param));
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_read_time_attrs_handle", (FMT__0));

    return processed;
}
#endif  /* ZB_ENABLE_TIME_SYNC */


void zb_se_service_discovery_set_multiple_commodity_enabled(zb_uint8_t enabled)
{
    if (enabled)
    {
        ZSE_SERVICE_DISCOVERY_SET_MULTIPLE_COMMODITY();
    }
    else
    {
        ZSE_SERVICE_DISCOVERY_CLR_MULTIPLE_COMMODITY();
    }
    //ZSE_CTXC().service_disc.multiple_commodity_enabled = enabled;
}

zb_bool_t se_need_to_read_commodity(zb_uint16_t cluster_id, zb_uint16_t *attr_id)
{
    zb_bool_t ret = ZB_TRUE;

    //if (ZSE_CTXC().service_disc.multiple_commodity_enabled)
    if (ZSE_SERVICE_DISCOVERY_GET_MULTIPLE_COMMODITY())
    {
        switch (cluster_id)
        {
        case ZB_ZCL_CLUSTER_ID_PRICE:
            *attr_id = ZB_ZCL_ATTR_PRICE_SRV_COMMODITY_TYPE;
            break;
        case ZB_ZCL_CLUSTER_ID_METERING:
            *attr_id = ZB_ZCL_ATTR_METERING_METERING_DEVICE_TYPE_ID;
            break;
        default:
            ret = ZB_FALSE;
            break;
        }
    }
    else
    {
        ret = ZB_FALSE;
    }
    return ret;
}

#ifdef ZB_ENABLE_TIME_SYNC
static void zb_se_service_discovery_set_internal_time_server_by_ep(zb_uint8_t source_ep)
{
    zb_zcl_cluster_desc_t *time_server_desc;

    time_server_desc = get_cluster_desc(zb_af_get_endpoint_desc(source_ep),
                                        ZB_ZCL_CLUSTER_ID_TIME,
                                        ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (time_server_desc)
    {
        zb_zcl_attr_t *time_desc;
        zb_zcl_attr_t *time_status_desc;
        zb_uint8_t auth_level;

        time_desc = zb_zcl_get_attr_desc_a(source_ep,
                                           ZB_ZCL_CLUSTER_ID_TIME,
                                           ZB_ZCL_CLUSTER_SERVER_ROLE,
                                           ZB_ZCL_ATTR_TIME_TIME_ID);

        time_status_desc = zb_zcl_get_attr_desc_a(source_ep,
                           ZB_ZCL_CLUSTER_ID_TIME,
                           ZB_ZCL_CLUSTER_SERVER_ROLE,
                           ZB_ZCL_ATTR_TIME_TIME_STATUS_ID);

        if (time_desc && time_status_desc)
        {
            auth_level = zb_zcl_time_sync_get_auth_level(ZB_ZCL_GET_ATTRIBUTE_VAL_8(time_status_desc),
                         ZSE_CTXC().time_server.server_short_addr);

            zb_zcl_time_sync_choose_time_server(auth_level,
                                                ZB_PIBCACHE_NETWORK_ADDRESS(),
                                                source_ep,
                                                ZB_ZCL_GET_ATTRIBUTE_VAL_32(time_desc));
        }
    }
}


static void zb_se_service_discovery_set_internal_time_server()
{
    zb_int_t i, j;
    for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
    {
        zb_bool_t found = ZB_FALSE;
        for (j = 0; j < ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count && !found; j++)
        {
            if (ZB_ZCL_CLUSTER_ID_TIME == ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_cluster_list[j])
            {
                zb_se_service_discovery_set_internal_time_server_by_ep(ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint);

                found = ZB_TRUE;
            }
        }
    }
}
#endif /* ZB_ENABLE_TIME_SYNC */

static void zb_se_service_discovery_read_commodity(zb_uint8_t param, zb_uint16_t short_addr, zb_uint8_t ep, zb_uint16_t cluster_id, zb_uint16_t attr_id)
{
    zb_uint8_t *cmd_ptr = NULL;

    TRACE_MSG(TRACE_ZCL1, "read commodity: cluster %d attr %d ", (FMT__D, cluster_id, attr_id));

    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, attr_id);

    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(param,
                                      cmd_ptr,
                                      short_addr,
                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      ep,
                                      ZSE_CTXC().service_disc.source_ep,
                                      get_profile_id_by_endpoint(ZSE_CTXC().service_disc.source_ep),
                                      cluster_id,
                                      NULL);
}


void zb_se_service_discovery_process_stored_devs(zb_uint8_t param)
{
    zb_uint8_t next_dev_idx;
    zse_service_disc_dev_t *next_dev = NULL;
    zb_ieee_addr_t dst_ieee_addr;

    TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_process_stored_devs param %hd", (FMT__H, param));

    /* Note: we are trying to run service discovery in parallel for different devices.
       Only key establishment is serialized. */
    /* Get next device for which operation is not in progress */
    next_dev_idx = zb_se_service_discovery_get_next_dev();
    if (next_dev_idx != 0xFF)
    {
        next_dev = &ZSE_CTXC().service_disc.devices[next_dev_idx];
        TRACE_MSG(TRACE_ZCL3, "next dev addr 0x%x step %d", (FMT__D_D, next_dev->short_addr, next_dev->step));

        switch (next_dev->step)
        {
        /* Need long address for bind and key establishment, so get it now. */
        case ZSE_SERVICE_DISC_DISCOVER_IEEE:
            if (zb_address_ieee_by_short(next_dev->short_addr, dst_ieee_addr) != RET_OK)
            {
                /* Discover IEEE if it is unknown */
                if (zb_zdo_initiate_ieee_addr_req_with_cb(param, next_dev->short_addr, zb_se_service_discovery_ieee_addr_req_cb) != ZB_ZDO_INVALID_TSN)
                {
                    next_dev->op_in_progress = 1;
                    TRACE_MSG(TRACE_ZCL3, "Initiating ieee req", (FMT__0));
                    param = 0;
                    break;
                }
                else
                {
                    TRACE_MSG(TRACE_ERROR, "Can't initiate ieee req", (FMT__0));
                    zb_buf_free(param);
                }
            }
            next_dev->step++;

        /* FALLTHROUGH */
        case ZSE_SERVICE_DISC_GEN_KEY:
        {
            zb_uint16_t attr_id;

            /* For Price/Metering - check commodity before binding. For Time - synchronize time */
            if ((se_need_to_read_commodity(next_dev->cluster_id, &attr_id) || (ZB_ZCL_CLUSTER_ID_TIME == next_dev->cluster_id))
                    &&
                    !zb_se_has_valid_key(next_dev->short_addr) && next_dev->short_addr != 0)
            {
                /* Note: APS key establishment via TC does not use CBKE and can run in parallel. */
                if (!zb_se_service_discovery_op_in_progress(next_dev->short_addr,
                        ZSE_SERVICE_DISC_GEN_KEY))
                {
                    TRACE_MSG(TRACE_ZCL3, "Need to read commodity info or synchronize time, but no aps key - initiate its recv", (FMT__0));
                    ZB_SCHEDULE_CALLBACK2(zb_se_start_aps_key_establishment, param, next_dev->short_addr);
                    param = 0;
                }
                else
                {
                    TRACE_MSG(TRACE_ZCL3, "aps key exchange is already initiated", (FMT__0));
                }
                next_dev->op_in_progress = 1;
                break;
            }
            next_dev->step++;
            TRACE_MSG(TRACE_ZCL3, "Fall thru next discovery step - read commodity", (FMT__0));
        }
#ifdef ZB_ENABLE_TIME_SYNC
        /* FALLTHROUGH */
        case ZSE_SERVICE_DISC_GET_TIME_SERVER:
        {
            if (ZB_ZCL_CLUSTER_ID_TIME == next_dev->cluster_id)
            {
                /* Sure: we have aps key here */
                zb_se_service_discovery_read_time_status(param, next_dev->short_addr, next_dev->ep);
                next_dev->zcl_tsn = ZCL_CTX().seq_number - 1;
                TRACE_MSG(TRACE_ZCL3, "Start read time status tsn %hd", (FMT__H, next_dev->zcl_tsn));
                next_dev->op_in_progress = 1;
                param = 0;
                /*FIXME: check it for time cluster logic
                  dev->step is updated in in zb_se_service_discovery_op_completed().
                */
                break;
            }
            next_dev->step++;
            TRACE_MSG(TRACE_ZCL3, "Fall thru next discovery step - read commodity", (FMT__0));
        }

#endif /* ZB_ENABLE_TIME_SYNC */
        /* FALLTHROUGH */
        case ZSE_SERVICE_DISC_READ_COMMODITY:
        {
            zb_uint16_t attr_id;

            /* For Price/Metering - check commodity before binding. */
            if (se_need_to_read_commodity(next_dev->cluster_id, &attr_id))
            {
                /* Sure we have aps key here */
                zb_se_service_discovery_read_commodity(param, next_dev->short_addr, next_dev->ep, next_dev->cluster_id, attr_id);
                /* Store ZCL seq number for last prepared command (read attr for commodity). */
                next_dev->zcl_tsn = ZCL_CTX().seq_number - 1;
                TRACE_MSG(TRACE_ZCL3, "Start read commodity tsn %hd", (FMT__H, next_dev->zcl_tsn));
                next_dev->op_in_progress = 1;
                param = 0;
                /*
                  dev->step is updated in in zb_se_service_discovery_op_completed().
                */
                break;
            }
            next_dev->step++;
            TRACE_MSG(TRACE_ZCL3, "Fall thru next discovery step - bind", (FMT__0));
        }
        /* FALLTHROUGH */
        case ZSE_SERVICE_DISC_BIND:
            /* Send bind - cluster, endpoint and IEEE are known */
            if (zb_address_ieee_by_short(next_dev->short_addr, dst_ieee_addr) != RET_OK)
            {
                ZB_ASSERT(0);
            }
            /* zb_se_service_discovery_bind_req(param, dst_ieee_addr, next_dev->ep); */
            /* Notify app that we are ready to bind device */
            TRACE_MSG(TRACE_ZCL3, "Send bind signal to app", (FMT__0));
            zb_se_service_discovery_send_bind_signal(param, next_dev_idx);
            /* Note: we may not have a key for that device. If application
             * decide to bind with it, it is up to application to setup
             * aps keys via TC */

            /* Keep 'in progress' so will not touch that entry any more. */
            next_dev->op_in_progress = 1;
            param = 0;          /* Buf is busy */
            break;

        case ZSE_SERVICE_DISC_BOUND_OK:
        {
            zb_uint16_t *addr_p = (zb_uint16_t *)zb_app_signal_pack(param,
                                  ZB_SE_SIGNAL_SERVICE_DISCOVERY_BIND_OK,
                                  RET_OK,
                                  2);
            TRACE_MSG(TRACE_ZCL3, "bind ok, dev 0x%x - inform user", (FMT__D, next_dev->short_addr));
            *addr_p = next_dev->short_addr;
            ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
            next_dev->op_in_progress = 1;
            param = 0;
            break;
        }
        default:
            ZB_ASSERT(0);           /* DEBUG */
            break;
        }

        /* If key establishment is running, sospend other actions until it completed. */
        if (zb_se_service_discovery_get_next_dev() != 0xFF)
        {
            TRACE_MSG(TRACE_ZCL3, "Reschedule zb_se_service_discovery_process_stored_devs", (FMT__0));
            zb_buf_get_out_delayed(zb_se_service_discovery_process_stored_devs);
        }
    }

    if (param)
    {
        zb_buf_free(param);
    }
}

void zb_se_service_discovery_get_short_address_cb(zb_uint8_t param)
{
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_ieee_addr_t ieee_addr;
    zb_uint16_t nwk_addr;
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_get_short_address_cb", (FMT__0));
    TRACE_MSG(TRACE_ZDO2, "zb_se_service_discovery_get_short_address_cb param %hd", (FMT__H, param));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(param);

    if (resp->status == ZB_ZDP_STATUS_TIMEOUT)
    {
        TRACE_MSG(TRACE_ZDO2, "resp status ZB_ZDP_STATUS_TIMEOUT (tsn = %hd), nwk addr [%hd] " TRACE_FORMAT_64 " unresolved, mark it failed",
                  (FMT__H_H_A, ((zb_zdo_callback_info_t *)resp)->tsn, ZSE_CTXC().mdu_pairing.short_address_scan_pos,
                   TRACE_ARG_64(ZSE_CTXC().mdu_pairing.virtual_han_table[ZSE_CTXC().mdu_pairing.short_address_scan_pos])));
    }
    else if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZDO2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));
        ZB_DUMP_IEEE_ADDR(resp->ieee_addr);

        ZB_LETOH64(ieee_addr, resp->ieee_addr);
        ZB_LETOH16(&nwk_addr, &resp->nwk_addr);
        zb_address_update(ieee_addr, nwk_addr, ZB_TRUE, &addr_ref);
        ZSE_CTXC().mdu_pairing.virtual_han_table_short[ZSE_CTXC().mdu_pairing.short_address_scan_pos] = nwk_addr;
    }
    else
    {
        TRACE_MSG(TRACE_ZDO2, "(tsn %hd) resp status %hd - UNKNOWN, nwk addr %d", (FMT__H_H_D, ((zb_zdo_callback_info_t *)resp)->tsn, resp->status, resp->nwk_addr));
        ZB_DUMP_IEEE_ADDR(resp->ieee_addr);
    }
    //  TRACE_MSG(TRACE_ZDO2, "schedule cb %p param %hd",
    //            (FMT__P_H, ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb, ZDO_CTX().zdo_ctx.get_short_addr_ctx.param));
    //  if (ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb)
    // {
    //   TRACE_MSG(TRACE_NWK3, "zb_se_service_discovery_get_short_address_cb: call callback", (FMT__0));
    //   ZB_SCHEDULE_CALLBACK(ZDO_CTX().zdo_ctx.get_short_addr_ctx.cb, ZDO_CTX().zdo_ctx.get_short_addr_ctx.param);
    //  }
    ZSE_CTXC().mdu_pairing.short_address_scan_pos++;
    ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_get_mdu_short_addresses, 0);

    zb_buf_free(param);

}

void zb_se_service_discovery_get_short_address(zb_uint8_t param, zb_uint16_t param2)
{
    //zb_start_get_peer_short_addr(ref_p, start_packet_send, param);
    zb_zdo_nwk_addr_req_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_nwk_addr_req_param_t);

    TRACE_MSG(TRACE_ZDO2, ">> zb_se_service_discovery_get_short_address for %d", (FMT__D, param2));

    req_param->dst_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    ZB_IEEE_ADDR_COPY(req_param->ieee_addr, &ZSE_CTXC().mdu_pairing.virtual_han_table[param2]);

    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    req_param->start_index = 0;
    if (zb_zdo_nwk_addr_req(param, zb_se_service_discovery_get_short_address_cb) == ZB_ZDO_INVALID_TSN)
    {
        zb_zdo_nwk_addr_resp_head_t *resp;

        resp = zb_buf_initial_alloc(param, sizeof(zb_zdo_nwk_addr_resp_head_t));
        resp->tsn = ZB_ZDO_INVALID_TSN;
        resp->status = ZB_ZDP_STATUS_INSUFFICIENT_SPACE;
        TRACE_MSG(TRACE_ERROR, "no mem space for zb_zdo_nwk_addr_req", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_get_short_address_cb, param);
    }
    //zb_zdo_nwk_addr_req(param, zb_se_service_discovery_get_short_address_cb);
}


void zb_se_service_discovery_get_mdu_short_addresses(zb_uint8_t param)
{
    zb_uint8_t i = 0;
    zb_uint16_t short_addr = 0;
    zb_ieee_addr_t *eui;
    //ZVUNUSED(param);
    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_get_mdu_short_addresses [%hd]", (FMT__H, param));
    TRACE_MSG(TRACE_APS3, "ZB_APS_ADDR_MODE_64_ENDP_PRESENT", (FMT__0));
    //ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);

    i = ZSE_CTXC().mdu_pairing.short_address_scan_pos;

    eui = &ZSE_CTXC().mdu_pairing.virtual_han_table[i];

    if (i < ZSE_CTXC().mdu_pairing.virtual_han_size)
    {
        /* convert long (64) to short (16) address, then unicast */
        short_addr = zb_address_short_by_ieee(*eui);
        if (short_addr != ZB_UNKNOWN_SHORT_ADDR)
        {
            /* Address found - continue */
            ZSE_CTXC().mdu_pairing.virtual_han_table_short[i] = short_addr;
            ZSE_CTXC().mdu_pairing.short_address_scan_pos++;
            ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_get_mdu_short_addresses, i + 1);
        }
        else
        {
            ZSE_CTXC().mdu_pairing.virtual_han_table_short[i] = ZB_UNKNOWN_SHORT_ADDR;

            if (zb_buf_get_out_delayed_ext(zb_se_service_discovery_get_short_address, i, 0) != RET_OK)
            {
                TRACE_MSG(TRACE_APS3, "error sending addr request", (FMT__0));
                ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_get_mdu_short_addresses, i);
            }
            else
            {
                TRACE_MSG(TRACE_APS1, "try to search nwk addr by IEEE" TRACE_FORMAT_64 " param %hd, waiting callback", (FMT__A_H, TRACE_ARG_64(*eui), i));
            }
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "!! finished traversing mdu list, short addresses collected", (FMT__0));
        //ZB_SCHEDULE_ALARM(zb_se_service_discovery_match_desc_req_delayed, 0, ZB_SE_SERVICE_DISCOVERY_CLUSTER_TIME);
        zb_se_service_discovery_match_desc_req_delayed(0);
    }
    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_get_mdu_short_addresses", (FMT__0));
}


void zb_se_service_discovery_mdu_pairing_request_cb(zb_uint8_t table_len)
{
    TRACE_MSG(TRACE_ZCL1, ">> mdu_pairing_request_cb list received: %hd devices w/o myself", (FMT__H, table_len));
    ZSE_CTXC().mdu_pairing.virtual_han_size = table_len;
    ZSE_SERVICE_DISCOVERY_SET_MDU_PAIRING_MODE();
    ZB_SCHEDULE_ALARM_CANCEL(zb_se_service_discovery_match_desc_req_delayed, ZB_ALARM_ANY_PARAM);
    ZSE_CTXC().mdu_pairing.short_address_scan_pos = 0;
    ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_get_mdu_short_addresses, 0);
}

void zb_se_service_discovery_mdu_pairing_request(zb_uint8_t param)
{
    //zb_uint8_t next_dev_idx;
    //zse_service_disc_dev_t* next_dev = NULL;
    //zb_ieee_addr_t dst_ieee_addr;
    zb_zcl_mdu_pairing_request_t pl;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_mdu_pairing_request param %hd", (FMT__H, param));

    pl.lpi_version = 0;
    ZB_IEEE_ADDR_COPY(&pl.eui64, ZB_PIBCACHE_EXTENDED_ADDRESS());

    zb_zcl_mdu_pairing_send_cmd_pairing_request( param,
            &ZSE_CTXC().mdu_pairing.mdu_parent,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            ZSE_CTXC().mdu_pairing.mdu_parent_ep,
            ZSE_CTXC().service_disc.source_ep,
            &pl,
            ZSE_CTXC().mdu_pairing.virtual_han_table,
            ZB_ZCL_MDU_PAIRING_HAN_TABLE_SIZE,
            zb_se_service_discovery_mdu_pairing_request_cb);
    param = 0;
    /* Note: we are trying to run service discovery in parallel for different devices.
       Only key establishment is serialized. */
    TRACE_MSG(TRACE_ZCL1, "send mdu pairing to %d", (FMT__D, ZSE_CTXC().mdu_pairing.mdu_parent.addr_short));

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_mdu_pairing_request", (FMT__0));
}

void zb_se_service_discovery_send_unicast_match(zb_uint8_t param);

zb_bool_t zb_se_service_discovery_match_desc_resp_handle(zb_uint8_t param)
{
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(param);
    zb_uint8_t *match_ep;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
    zb_zcl_cluster_desc_t *cluster_desc;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_match_desc_resp_handle param %hd", (FMT__H, param));

    /*
      We are here when some device respond by match desc resp.
      Store address/cluster and proceed it.
      Note that we may need to read attribute at remote, which requires existence of APS key.
      So initiate key request procedure if required.
      Note that only one key establishment procedure can run at once.
     */

    if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY_MDU)
    {
        if (resp->status == ZB_ZDP_STATUS_SUCCESS
                && resp->tsn == ZSE_CTXC().service_disc.match_desc_tsn
                && resp->match_len > 0)
        {
            TRACE_MSG(TRACE_ZCL1, ">> Received reply from %d to MDU server cluster: ep: %hd", (FMT__D_H, ind->src_addr, *(zb_uint8_t *)(resp + 1)));

            ZB_BZERO(&ZSE_CTXC().mdu_pairing, sizeof(ZSE_CTXC().mdu_pairing));
            ZSE_CTXC().mdu_pairing.mdu_parent.addr_short = ind->src_addr;
            ZSE_CTXC().mdu_pairing.mdu_parent_ep = *(zb_uint8_t *)(resp + 1);

            ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_mdu_pairing_request, param);
            param = 0;
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, ">> Broken reply for discovery MDU server cluster: status: %hd, tsn: %hd (%hd), match_len: %d",
                      (FMT__H_H_H_D, resp->status, resp->tsn, ZSE_CTXC().service_disc.match_desc_tsn, resp->match_len));
        }
        //ZSE_CTXC().commissioning.state = SE_STATE_SERVICE_DISCOVERY;
    }
    else if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY)
    {
        cluster_desc = zb_se_service_discovery_get_current_cluster();
        ZB_ASSERT(cluster_desc);

        if (resp->status == ZB_ZDP_STATUS_SUCCESS
                && resp->tsn == ZSE_CTXC().service_disc.match_desc_tsn
                && resp->match_len > 0)
        {
            zb_uint8_t i = 0;

            /* Match EP list follows right after response header
               Cache endpoints */

            match_ep = (zb_uint8_t *)(resp + 1);
            while (i < resp->match_len)
            {
                if (zb_se_service_discovery_store_dev(ind->src_addr,
                                                      *match_ep,
                                                      cluster_desc->cluster_id) != RET_OK)
                {
                    break;
                }
                ++i;
                ++match_ep;
            }

            ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_process_stored_devs, param);
            param = 0;
        }
        if ( resp->tsn == ZSE_CTXC().service_disc.match_desc_tsn
                && ZSE_SERVICE_DISCOVERY_GET_MDU_PAIRING_MODE())
        {
            TRACE_MSG(TRACE_ZCL1, "MDU mode", (FMT__0));
            zb_buf_get_out_delayed(zb_se_service_discovery_send_unicast_match);
        }
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_match_desc_resp_handle", (FMT__0));
    return (zb_bool_t)(!param);
}


void zb_se_service_discovery_send_unicast_match_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZCL1, "### zb_se_service_discovery_send_unicast_match_cb: param: %hd", (FMT__H, param));
    //zb_buf_get_out_delayed(zb_se_service_discovery_send_unicast_match);
}

void zb_se_service_discovery_send_unicast_match(zb_uint8_t param)
{
    zb_uint8_t i = 0;
    //  zb_nlde_data_req_t nldereq;
    zb_uint16_t short_addr = 0;
    //  zb_bool_t sent_status = ZB_FALSE;
    zb_zdo_match_desc_param_t *req;
    //  zb_ieee_addr_t *eui;
    //ZVUNUSED(param);
    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_send_unicast_match param %hd", (FMT__H, param));
    i = ZSE_CTXC().mdu_pairing.short_address_scan_pos;
    TRACE_MSG(TRACE_APS3, "ZB_APS_ADDR_MODE_64_ENDP_PRESENT: i=%hd", (FMT__H, i));
    //ZB_APS_FC_SET_DELIVERY_MODE(fc, ZB_APS_DELIVERY_UNICAST);


    while (i < ZSE_CTXC().mdu_pairing.virtual_han_size &&
            ZSE_CTXC().mdu_pairing.virtual_han_table_short[i] == ZB_UNKNOWN_SHORT_ADDR)
    {
        TRACE_MSG(TRACE_APS3, "i=%hd, addr_short=%04X", (FMT__H, i, ZSE_CTXC().mdu_pairing.virtual_han_table_short[i]));
        i++;
    }

    ZSE_CTXC().mdu_pairing.short_address_scan_pos = i + 1;

    if (i < ZSE_CTXC().mdu_pairing.virtual_han_size)
    {
        TRACE_MSG(TRACE_APS3, "selected i=%hd, addr_short=%04X", (FMT__H, i, ZSE_CTXC().mdu_pairing.virtual_han_table_short[i]));
        short_addr = ZSE_CTXC().mdu_pairing.virtual_han_table_short[i];

        req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t));

        req->nwk_addr = short_addr;
        req->addr_of_interest = short_addr;
        req->profile_id = get_profile_id_by_endpoint(ZSE_CTXC().service_disc.source_ep);
        req->num_in_clusters = 1;
        req->num_out_clusters = 0;
        req->cluster_list[0] = ZSE_CTXC().mdu_pairing.cluster_id;
        TRACE_MSG(TRACE_ZCL1, "send match descr for device %04X cluster %d", (FMT__D_D, req->addr_of_interest, req->cluster_list[0]));

        //ZSE_CTXC().service_disc.match_desc_tsn = zb_zdo_match_desc_req(param, zb_se_service_discovery_send_unicast_match_cb);
        ZSE_CTXC().service_disc.match_desc_tsn = zb_zdo_match_desc_req(param, NULL);

        if (ZSE_CTXC().service_disc.match_desc_tsn != ZB_ZDO_INVALID_TSN)
        {
            TRACE_MSG(TRACE_ZCL1, "### tsn = %hd", (FMT__H, ZSE_CTXC().service_disc.match_desc_tsn));
            //ZB_SCHEDULE_ALARM(zb_se_service_discovery_match_desc_req_delayed, 0, ZB_SE_SERVICE_DISCOVERY_CLUSTER_TIME);
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "service discovery finished: can not send match desc", (FMT__0));
            se_commissioning_signal(SE_COMM_SIGNAL_SERVICE_DISCOVERY_FAILED, param);
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "match descriptor unicast sending for cluster_id :%04X finished", (FMT__D, ZSE_CTXC().mdu_pairing.cluster_id));
        zb_buf_free(param);
        ZB_SCHEDULE_ALARM(zb_se_service_discovery_match_desc_req_delayed, 0, ZB_SE_SERVICE_DISCOVERY_CLUSTER_TIME);
        //    zb_se_service_discovery_match_desc_req_delayed(0);
    }

}


void zb_se_service_discovery_match_desc_req_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(zb_se_service_discovery_match_desc_req);
}


void zb_se_service_discovery_match_desc_req(zb_uint8_t param)
{
    zb_zdo_match_desc_param_t *req;
    zb_zcl_cluster_desc_t *cluster_desc;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_match_desc_req %hd", (FMT__H, param));

    /* In any case reset current device list - we start new cluster discovery or finish whole service
     * discovery. */
    zb_se_service_discovery_reset_dev();

    /* Search MDU Pairing Client Cluster on current device */
    if (ZSE_CTXC().commissioning.state == SE_STATE_SERVICE_DISCOVERY_MDU)
    {
        if (ZSE_SERVICE_DISCOVERY_GET_MDU_PAIRING_MODE())
        {
            TRACE_MSG(TRACE_ZCL1, "found MDU server, requesting clusters list", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "MDU server not found, continue broadcast service discovery", (FMT__0));
        }
        ZSE_CTXC().commissioning.state = SE_STATE_SERVICE_DISCOVERY;
    }
    /*cluster_desc = zb_se_service_discovery_get_mdu_cluster();
    if(cluster_desc)
    {
      TRACE_MSG(TRACE_ZCL1, "found MDU client, try MDU pairing mode", (FMT__0));
      ZSE_SERVICE_DISCOVERY_SET_MDU_PAIRING_MODE();
    }
    */

#ifdef ZB_ENABLE_TIME_SYNC

    /* Additional checks to avoid one more callback invocation */
    if (ZSE_CTXC().time_server.server_auth_level != ZB_ZCL_TIME_SERVER_NOT_CHOSEN)
    {
        zb_buf_get_out_delayed(zb_se_time_sync_inform_user_about_time_synch);
    }
#endif  /* ZB_ENABLE_TIME_SYNC */

    ++ZSE_CTXC().service_disc.current_cluster;

    cluster_desc = zb_se_service_discovery_get_current_cluster();

    TRACE_MSG(TRACE_ZCL3, "current_cluster %d cluster_desc %p",
              (FMT__D_P, ZSE_CTXC().service_disc.current_cluster, cluster_desc));
    /* FIXME: Discover only server roles for which we are client, is it correct? */
    while (cluster_desc &&
            (cluster_desc->role_mask != ZB_ZCL_CLUSTER_CLIENT_ROLE ||
             /* Do not need to bind internal clusters or clusters which does not support "asynchronous
              * event commands". */
             cluster_desc->cluster_id == ZB_ZCL_CLUSTER_ID_KEEP_ALIVE ||
             cluster_desc->cluster_id == ZB_ZCL_CLUSTER_ID_SUB_GHZ ||
             cluster_desc->cluster_id == ZB_ZCL_CLUSTER_ID_MDU_PAIRING ||
             cluster_desc->cluster_id == ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT))
    {
        ++ZSE_CTXC().service_disc.current_cluster;
        cluster_desc = zb_se_service_discovery_get_current_cluster();
        TRACE_MSG(TRACE_ZCL3, "Cluster skip. current_cluster %d cluster_desc %p",
                  (FMT__D_P, ZSE_CTXC().service_disc.current_cluster, cluster_desc));
    }

    if (cluster_desc)
    {
#ifdef ZB_ENABLE_TIME_SYNC
        if (ZB_ZCL_CLUSTER_ID_TIME == cluster_desc->cluster_id)
        {
            zb_se_service_discovery_set_internal_time_server();
        }
#endif /* ZB_ENABLE_TIME_SYNC */
        if (ZSE_SERVICE_DISCOVERY_GET_MDU_PAIRING_MODE())
        {
            TRACE_MSG(TRACE_ZCL1, "MDU mode, requesting cluster %04X by unicasts", (FMT__D, cluster_desc->cluster_id));
            ZSE_CTXC().mdu_pairing.short_address_scan_pos = 0;
            ZSE_CTXC().mdu_pairing.cluster_id = cluster_desc->cluster_id;
            ZB_SCHEDULE_CALLBACK(zb_se_service_discovery_send_unicast_match, param);
        }
        else
        {
            req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t));
            req->nwk_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
#ifdef ZSE_NON_MDU_UNICAST_MATCH_DESC
            req->addr_of_interest = 0;
#else
            req->addr_of_interest = ZB_NWK_BROADCAST_ALL_DEVICES;
#endif
            req->profile_id = get_profile_id_by_endpoint(ZSE_CTXC().service_disc.source_ep);
            req->num_in_clusters = 1;
            req->num_out_clusters = 0;
            req->cluster_list[0] = cluster_desc->cluster_id;
            TRACE_MSG(TRACE_ZCL1, "send match descr for cluster 0x%x", (FMT__D, req->cluster_list[0]));

            /* Do not use callback for broadcast packet - need to handle all answers (from multiple
             * devices). */
            ZSE_CTXC().service_disc.match_desc_tsn = zb_zdo_match_desc_req(param, NULL);

            if (ZSE_CTXC().service_disc.match_desc_tsn != ZB_ZDO_INVALID_TSN)
            {
                ZB_SCHEDULE_ALARM(zb_se_service_discovery_match_desc_req_delayed, 0, ZB_SE_SERVICE_DISCOVERY_CLUSTER_TIME);
            }
            else
            {
                TRACE_MSG(TRACE_ZCL1, "service discovery finished: can not send match desc", (FMT__0));
                se_commissioning_signal(SE_COMM_SIGNAL_SERVICE_DISCOVERY_FAILED, param);
            }
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "service discovery finished: all clusters are discovered", (FMT__0));
        ZSE_CTXC().service_disc.match_desc_tsn = ZB_ZDO_INVALID_TSN;
        se_commissioning_signal(SE_COMM_SIGNAL_SERVICE_DISCOVERY_OK, param);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_match_desc_req", (FMT__0));
}

void zb_se_service_discovery_match_mdu_desc_req(zb_uint8_t param)
{
    zb_zdo_match_desc_param_t *req;
    //  zb_zcl_cluster_desc_t* cluster_desc;

    TRACE_MSG(TRACE_ZCL1, ">> zb_se_service_discovery_match_mdu_desc_req %hd", (FMT__H, param));

    /* In any case reset current device list - we start new cluster discovery or finish whole service
     * discovery. */
    zb_se_service_discovery_reset_dev();

    /* Search MDU Pairing Client Cluster on current device */
    //cluster_desc = zb_se_service_discovery_get_mdu_cluster();

    //if (cluster_desc)
    //{
    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t));

    req->nwk_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req->profile_id = get_profile_id_by_endpoint(ZSE_CTXC().service_disc.source_ep);
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_MDU_PAIRING;
    TRACE_MSG(TRACE_ZCL1, "send match descr for MDU cluster %d", (FMT__D, req->cluster_list[0]));

    /* Do not use callback for broadcast packet - need to handle all answers (from multiple
     * devices). */
    ZSE_CTXC().service_disc.match_desc_tsn = zb_zdo_match_desc_req(param, NULL);

    if (ZSE_CTXC().service_disc.match_desc_tsn != ZB_ZDO_INVALID_TSN)
    {
        ZB_SCHEDULE_ALARM(zb_se_service_discovery_match_desc_req_delayed, 0, ZB_SE_SERVICE_DISCOVERY_CLUSTER_TIME);
        /* schedule broadcast match descr req with 1st server pair. */
        //zb_buf_get_out_delayed(zb_se_service_discovery_match_desc_req);
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "service discovery finished: can not send match desc", (FMT__0));
        se_commissioning_signal(SE_COMM_SIGNAL_SERVICE_DISCOVERY_FAILED, param);
    }
    //}
    // else
    // {
    //   TRACE_MSG(TRACE_ZCL1, "service discovery FAILED: MDU cluster search failed", (FMT__0));
    //   ZB_ASSERT(0);
    // }

    TRACE_MSG(TRACE_ZCL1, "<< zb_se_service_discovery_match_mdu_desc_req", (FMT__0));
}

void zb_se_service_discovery_stop()
{
    TRACE_MSG(TRACE_ZCL1, "zb_se_service_discovery_stop", (FMT__0));

    ZSE_CTXC().service_disc.match_desc_tsn = ZB_ZDO_INVALID_TSN;
    ZSE_CTXC().service_disc.source_ep = 0;
    ZSE_CTXC().service_disc.current_cluster = (zb_uint8_t) -1;
    zb_se_service_discovery_reset_dev();

    ZB_SCHEDULE_ALARM_CANCEL(zb_se_service_discovery_match_desc_req_delayed, ZB_ALARM_ANY_PARAM);
}

zb_ret_t zb_se_service_discovery_start(zb_uint8_t endpoint)
{
    zb_ret_t ret = RET_NOT_FOUND;
    zb_af_endpoint_desc_t *ep_desc = NULL;

    TRACE_MSG(TRACE_ZCL1, "> zb_se_service_discovery_start ep %hd", (FMT__H, endpoint));

    ZSE_CTXC().commissioning.state = SE_STATE_SERVICE_DISCOVERY;
    /* Check if service discovery is configured - zero endpoint is invalid, do not start discovery in
     * that case. */
    ep_desc = zb_af_get_endpoint_desc(endpoint);

    ret = ( (ep_desc              /* have ep descriptor */
#ifndef ZB_SE_BDB_MIXED
             && ep_desc->profile_id == ZB_AF_SE_PROFILE_ID /* with correct profile */
#endif
             && ep_desc->simple_desc->app_output_cluster_count) /* and at least one client cluster */
            ? RET_OK : RET_INVALID_PARAMETER_1 );

    if (ret == RET_OK)
    {
        ZSE_CTXC().service_disc.source_ep = endpoint;
        ZSE_CTXC().service_disc.current_cluster = (zb_uint8_t) -1;

        /* TODO: Work with broadcast commands only (MDU is currently not supported). */

        /* Search MDU Pairing Client Cluster on current device */
        //cluster_desc = zb_se_service_discovery_get_mdu_cluster();
        if (zb_se_service_discovery_get_mdu_cluster())
        {
            TRACE_MSG(TRACE_ZCL1, "found MDU client, try MDU pairing mode", (FMT__0));
            ZSE_CTXC().commissioning.state = SE_STATE_SERVICE_DISCOVERY_MDU;
            //ZSE_SERVICE_DISCOVERY_SET_MDU_PAIRING_MODE();
            /* schedule broadcast match descr req with MDU server pair. */
            zb_buf_get_out_delayed(zb_se_service_discovery_match_mdu_desc_req);
        }
        else
        {
            /* schedule broadcast match descr req with 1st server pair. */
            zb_buf_get_out_delayed(zb_se_service_discovery_match_desc_req);
        }

        /* TODO: Implement some countdown etc - alarm for 3 hrs is not the best option) */
        ZB_SCHEDULE_ALARM_CANCEL(zb_se_service_discovery_send_start_signal_delayed,
                                 ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_ALARM(zb_se_service_discovery_send_start_signal_delayed, 0,
                          ZB_SE_SERVICE_DISCOVERY_PERIODIC_RESTART_TIME);
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_se_service_discovery_start ret %hd", (FMT__H, ret));

    return ret;
}

#endif /* defined(ZB_SE_ENABLE_SERVICE_DISCOVERY_PROCESSING) */

#endif /* ZB_ENABLE_SE */
