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
/* PURPOSE: SERVER: ZCL Tunneling cluster, purpose: general data tunneling.
*/

#define ZB_TRACE_FILE_ID 72

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_TUNNELING) || defined DOXYGEN

#include "zcl/zb_zcl_tunneling.h"

/* WARNING: Assume one tunnel on client and multiple tunnels on server.
   It is not clearly described how much tunnels may be on the client and on the server, but
   according to the ZCL spec - 10.6.1 Overview:

   Client: Requests a tunnel from the server and closes the tunnel if it is no longer needed.
   Server: Provides and manages tunnels to the clients.

   looks like client supports only one tunnel to one server.
 */

/* TODO: Not implemented:
   - MaxIncomingTransferSize handling (for example, what to do if it is too small?)
   - APS fragmentation support
 */

zb_uint8_t gs_tunneling_server_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNELING_SERVER_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_tunneling_server_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNELING_SERVER_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_tunneling_server_cmd_list =
{
    sizeof(gs_tunneling_server_received_commands), gs_tunneling_server_received_commands,
    sizeof(gs_tunneling_server_generated_commands), gs_tunneling_server_generated_commands
};

typedef struct zb_zcl_tunneling_ctx_s
{
    zb_zcl_tunneling_srv_rec_t srv_table[ZB_ZCL_TUNNELING_SRV_TABLE_SIZE];
    zb_bool_t timer_running;
} zb_zcl_tunneling_ctx_t;

static zb_zcl_tunneling_ctx_t zcl_tun_ctx;

void zb_zcl_tunneling_timer_start(void);

zb_uint16_t zb_zcl_tunneling_srv_get_record_by_addr(zb_uint16_t dst_addr, zb_uint8_t dst_ep)
{
    zb_uint8_t i = 0;

    while (i < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table))
    {
        if (zcl_tun_ctx.srv_table[i].close_tunnel_timeout &&
                zcl_tun_ctx.srv_table[i].dst_addr == dst_addr &&
                zcl_tun_ctx.srv_table[i].dst_ep == dst_ep)
        {
            return i;
        }
        ++i;
    }
    return ZB_ZCL_TUNNELING_TUNNEL_ID_INVALID_VALUE;
}

zb_uint16_t zb_zcl_tunneling_srv_get_free_record()
{
    zb_uint8_t i = 0;

    while (i < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table))
    {
        if (!zcl_tun_ctx.srv_table[i].close_tunnel_timeout)
        {
            return i;
        }
        ++i;
    }
    return ZB_ZCL_TUNNELING_TUNNEL_ID_INVALID_VALUE;
}

zb_bool_t zb_zcl_tunneling_server_request_tunnel_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_request_tunnel_t req;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_uint16_t tunnel_id = ZB_ZCL_TUNNELING_TUNNEL_ID_INVALID_VALUE;
    zb_zcl_tunnel_request_params_out_t tunnel_request_params_out = { 0 };
    zb_ret_t ret = RET_ERROR;

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_tunneling_server_request_tunnel_handle", (FMT__0));

    ZB_ZCL_TUNNELING_GET_REQUEST_TUNNEL(&req, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_ZCL1, "protocol_id[%hd], flow[%hd], manuf_code[0x%04X], max_incoming_to_client_transfer[%d]",
                  (FMT__H_H_D_D, req.protocol_id, req.flow_control_support, req.manufacturer_code, req.max_incoming_transfer_size));

        tunnel_request_params_out.tunnel_status = ZB_ZCL_TUNNELING_STATUS_NO_MORE_IDS;

        /* Check by addr if we already have tunnel */
        if (zb_zcl_tunneling_srv_get_record_by_addr(
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint) !=
                ZB_ZCL_TUNNELING_TUNNEL_ID_INVALID_VALUE)
        {
            /* One tunnels for one client is allowed. */
            tunnel_request_params_out.tunnel_status = ZB_ZCL_TUNNELING_STATUS_NO_MORE_IDS;
        }
        /* Check if we have free tunneling record */
        else if ((tunnel_id = zb_zcl_tunneling_srv_get_free_record())
                 != ZB_ZCL_TUNNELING_TUNNEL_ID_INVALID_VALUE)
        {
            if (ZCL_CTX().device_cb)
            {
                ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                                  ZB_ZCL_TUNNELING_REQUEST_TUNNEL_CB_ID, RET_OK, cmd_info, &req, &tunnel_request_params_out);

                ZCL_CTX().device_cb(param);

                TRACE_MSG(TRACE_ZCL1,
                          "cb status %hd cmd status %hd",
                          (FMT__H_H, ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param), tunnel_request_params_out.tunnel_status));

                if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) == RET_OK
                        && tunnel_request_params_out.tunnel_status == ZB_ZCL_TUNNELING_STATUS_SUCCESS)
                {
                    /* Free record found - fill the info. */
                    zb_zcl_attr_t *attr_desc_tmo = zb_zcl_get_attr_desc_a(
                                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                                       ZB_ZCL_CLUSTER_ID_TUNNELING,
                                                       ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                       ZB_ZCL_ATTR_TUNNELING_CLOSE_TUNNEL_TIMEOUT_ID);

                    ZB_ASSERT(attr_desc_tmo);
                    zcl_tun_ctx.srv_table[tunnel_id].close_tunnel_timeout = ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc_tmo);
                    zcl_tun_ctx.srv_table[tunnel_id].tunnel_options = req;
                    zcl_tun_ctx.srv_table[tunnel_id].dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
                    zcl_tun_ctx.srv_table[tunnel_id].dst_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
                    zcl_tun_ctx.srv_table[tunnel_id].src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;
                    zcl_tun_ctx.srv_table[tunnel_id].max_incoming_to_srv_transfer_size = tunnel_request_params_out.max_incoming_to_srv_transfer_size;
                    TRACE_MSG(TRACE_ZCL1, "srv: remember tunnel_id %d addr %d ep %hd", (FMT__D_D_H, tunnel_id, zcl_tun_ctx.srv_table[tunnel_id].dst_addr, zcl_tun_ctx.srv_table[tunnel_id].dst_ep));
                    TRACE_MSG(TRACE_ZCL1, "srv: tmo %d", (FMT__D, zcl_tun_ctx.srv_table[tunnel_id].close_tunnel_timeout));
                    zb_zcl_tunneling_timer_start();
                }
            }
        }

        if (tunnel_request_params_out.max_incoming_to_srv_transfer_size <= ZB_ZCL_TUNNELING_MAX_INCOMING_TRANSFER_SIZE)
        {
            ZB_ZCL_TUNNELING_SEND_REQUEST_TUNNEL_RESPONSE(param,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                    cmd_info->profile_id,
                    ZB_ZCL_DISABLE_DEFAULT_RESPONSE,
                    cmd_info->seq_number,
                    NULL,                     /* no callback */
                    tunnel_id,
                    tunnel_request_params_out.tunnel_status,
                    tunnel_request_params_out.max_incoming_to_srv_transfer_size);

            param = 0;
            ret = RET_OK;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "ERROR: Application want to set up max_incoming_to_srv_transfer_size[%d] more than supported maximum [%d]",
                      (FMT__D_D, tunnel_request_params_out.max_incoming_to_srv_transfer_size, ZB_ZCL_TUNNELING_MAX_INCOMING_TRANSFER_SIZE));
        }
    }

    if (param)
    {
        zb_zcl_send_default_handler(param, cmd_info,
                                    ret == RET_OK ? ZB_ZCL_STATUS_SUCCESS : ZB_ZCL_STATUS_FAIL);
    }

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_tunneling_server_request_tunnel_handle", (FMT__0));

    return ZB_TRUE;              /* cmd processed */
}

zb_bool_t zb_zcl_tunneling_server_transfer_data_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_transfer_data_payload_t tr_data;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_uint8_t transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_NO_SUCH_TUNNEL;

    ZB_ZCL_TUNNELING_GET_TRANSFER_DATA(&tr_data, param, parse_status);


    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* SRV: Try to get record by tunnel_id */
        if (tr_data.hdr.tunnel_id < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table)
                && zcl_tun_ctx.srv_table[tr_data.hdr.tunnel_id].close_tunnel_timeout)
        {
            transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK;

            if (ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr != zcl_tun_ctx.srv_table[tr_data.hdr.tunnel_id].dst_addr
                    || ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint != zcl_tun_ctx.srv_table[tr_data.hdr.tunnel_id].dst_ep)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_WRONG_DEVICE;
            }
            else if (tr_data.data_size > zcl_tun_ctx.srv_table[tr_data.hdr.tunnel_id].max_incoming_to_srv_transfer_size)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_DATA_OVERFLOW;
            }
        }

        if (transfer_data_status == ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK && ZCL_CTX().device_cb)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                param,
                ZB_ZCL_TUNNELING_TRANSFER_DATA_CLI_CB_ID,
                RET_OK,
                cmd_info,
                &tr_data,
                NULL);

            ZCL_CTX().device_cb(param);

            if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) != RET_OK)
            {
                transfer_data_status = ZB_ZCL_TUNNELING_TRANSFER_DATA_INTERNAL_ERROR;
            }
            else
            {
                /* CLI->SRV command: reset timer - tunnel is active */
                zb_zcl_attr_t *attr_desc_tmo = zb_zcl_get_attr_desc_a(
                                                   ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                                   ZB_ZCL_CLUSTER_ID_TUNNELING,
                                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                                   ZB_ZCL_ATTR_TUNNELING_CLOSE_TUNNEL_TIMEOUT_ID);

                zcl_tun_ctx.srv_table[tr_data.hdr.tunnel_id].close_tunnel_timeout =
                    ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc_tmo);
            }
        }

        if (transfer_data_status != ZB_ZCL_TUNNELING_TRANSFER_DATA_STATUS_OK)
        {
            ZB_ZCL_TUNNELING_SEND_TRANSFER_DATA_ERROR(param,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                    ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                    cmd_info->profile_id,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    NULL,                     /* no callback */
                    cmd_info->seq_number,
                    tr_data.hdr.tunnel_id,
                    transfer_data_status,
                    ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA_ERROR,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI);
            param = 0;
        }
    }

    /* Do not send Default Response on Transfer Data - have optional Ack cmd. */

    if (param)
    {
        zb_buf_free(param);
    }

    return ZB_TRUE;
}

zb_bool_t zb_zcl_tunneling_server_close_tunnel_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_close_tunnel_t close_tunnel;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;

    ZB_ZCL_TUNNELING_GET_CLOSE_TUNNEL(&close_tunnel, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        if (close_tunnel.tunnel_id < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table)
                && zcl_tun_ctx.srv_table[close_tunnel.tunnel_id].close_tunnel_timeout
                && zcl_tun_ctx.srv_table[close_tunnel.tunnel_id].dst_addr ==
                ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr
                && zcl_tun_ctx.srv_table[close_tunnel.tunnel_id].dst_ep ==
                ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint)
        {
            zcl_tun_ctx.srv_table[close_tunnel.tunnel_id].close_tunnel_timeout = 0;

            if (ZCL_CTX().device_cb)
            {
                ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                    param,
                    ZB_ZCL_TUNNELING_CLOSE_TUNNEL_CB_ID,
                    RET_ERROR,
                    cmd_info,
                    &close_tunnel,
                    NULL);

                ZCL_CTX().device_cb(param);
            }

            switch (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param))
            {
            case RET_OK:
                zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_SUCCESS);
                break;
            case RET_ERROR:
                zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_FAIL);
                break;
            default:
                zb_buf_free(param);
                break;
            }

            param = 0;
        }
    }

    if (param)
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    return ZB_TRUE;
}

zb_bool_t zb_zcl_tunneling_server_transfer_data_error_handle(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_tunneling_transfer_data_error_t tr_error;
    zb_uint8_t parse_status = ZB_ZCL_PARSE_STATUS_FAILURE;
    zb_ret_t ret = RET_ERROR;

    ZB_ZCL_TUNNELING_GET_TRANSFER_DATA_ERROR(&tr_error, param, parse_status);

    if (parse_status == ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        /* SRV: Try to get record by tunnel_id */
        if (tr_error.tunnel_id < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table)
                && zcl_tun_ctx.srv_table[tr_error.tunnel_id].close_tunnel_timeout
                && zcl_tun_ctx.srv_table[tr_error.tunnel_id].dst_addr ==
                ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr
                && zcl_tun_ctx.srv_table[tr_error.tunnel_id].dst_ep ==
                ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint)
        {
            ret = RET_OK;
        }

        if (ret == RET_OK && ZCL_CTX().device_cb)
        {
            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                param,
                ZB_ZCL_TUNNELING_TRANSFER_DATA_ERROR_CLI_CB_ID,
                RET_OK,
                cmd_info,
                &tr_error,
                NULL);

            ZCL_CTX().device_cb(param);
        }
    }

    /* Do not send Default Response on Transfer Data Error - client may send Close Tunnel. */

    zb_buf_free(param);

    return ZB_TRUE;
}

zb_bool_t zb_zcl_process_s_tunneling_specific_commands(zb_uint8_t param)
{
    zb_bool_t processed = ZB_FALSE;
    zb_zcl_parsed_hdr_t cmd_info;

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_tunneling_server_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG( TRACE_ZCL1,
               "> zb_zcl_process_s_tunneling_specific_commands: param %hd, cmd %hd",
               (FMT__H_H, param, cmd_info.cmd_id));

    ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_TUNNELING);

    if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        switch (cmd_info.cmd_id)
        {
        case ZB_ZCL_TUNNELING_CLI_CMD_REQUEST_TUNNEL:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_CLI_CMD_REQUEST_TUNNEL", (FMT__0));
            processed = zb_zcl_tunneling_server_request_tunnel_handle(param, &cmd_info);
            break;

        case ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA", (FMT__0));
            processed = zb_zcl_tunneling_server_transfer_data_handle(param, &cmd_info);
            break;

        case ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA_ERROR:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_CLI_CMD_TRANSFER_DATA_ERROR", (FMT__0));
            processed = zb_zcl_tunneling_server_transfer_data_error_handle(param, &cmd_info);
            break;

        case ZB_ZCL_TUNNELING_CLI_CMD_CLOSE_TUNNEL:
            TRACE_MSG(TRACE_ZCL1, "ZB_ZCL_TUNNELING_CLI_CMD_CLOSE_TUNNEL", (FMT__0));
            processed = zb_zcl_tunneling_server_close_tunnel_handle(param, &cmd_info);
            break;

        /* TODO: Handle other commands. */

        default:
            break;
        }
    }

    TRACE_MSG( TRACE_ZCL1,
               "< zb_zcl_process_s_tunneling_specific_commands: processed %d",
               (FMT__D, processed));
    return processed;
}

zb_ret_t zb_zcl_tunneling_server_send_transfer_data(zb_uint8_t param, zb_uint8_t ep, zb_uint16_t prfl_id,
        zb_uint8_t def_resp, zb_callback_t cb, zb_uint16_t tunnel_id,
        zb_uint16_t data_size, zb_uint8_t *image_data)
{
    zb_ret_t ret = RET_ERROR;
    zb_bufid_t buffer = param;
    zb_uint16_t dst_addr = ZB_UNKNOWN_SHORT_ADDR;
    zb_uint8_t dst_ep = 0;

    TRACE_MSG(TRACE_ZCL1, "zb_zcl_tunneling_send_transfer_data param %hd tunnel_id %d", (FMT__H_D, param, tunnel_id));

    if (tunnel_id < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table) &&
            zcl_tun_ctx.srv_table[tunnel_id].close_tunnel_timeout)
    {
        if (data_size <= zcl_tun_ctx.srv_table[tunnel_id].tunnel_options.max_incoming_transfer_size)
        {
            dst_addr = zcl_tun_ctx.srv_table[tunnel_id].dst_addr;
            dst_ep = zcl_tun_ctx.srv_table[tunnel_id].dst_ep;

            if (dst_addr != ZB_UNKNOWN_SHORT_ADDR && dst_ep)
            {
                /* Both Transfer Data (from cli and from srv) are requests, right? */
                zb_uint8_t *ptr = zb_zcl_start_command_header(buffer,
                                  ZB_ZCL_CONSTRUCT_FRAME_CONTROL(ZB_ZCL_FRAME_TYPE_CLUSTER_SPECIFIC,
                                          ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
                                          ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                                          def_resp),
                                  0, ZB_ZCL_TUNNELING_SRV_CMD_TRANSFER_DATA, NULL);
                ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, tunnel_id);
                if (data_size > 0)
                {
                    ZB_ZCL_PACKET_PUT_DATA_N(ptr, image_data, data_size);
                }
                ZB_ZCL_FINISH_N_SEND_PACKET(buffer, ptr,
                                            dst_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, dst_ep, ep, prfl_id,
                                            ZB_ZCL_CLUSTER_ID_TUNNELING, cb);
                ret = RET_OK;
            }
            else
            {
                TRACE_MSG(TRACE_ERROR,
                          "transfer data failed - incorrect dst_addr=0x%x or dst_ep=%d",
                          (FMT__H_D, dst_addr, dst_ep));
            }
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "transfer data failed - too much data_size=%d", (FMT__D, data_size));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "transfer data failed - no tunnel with %d id", (FMT__D, tunnel_id));
    }

    return ret;
}


static void tunneling_device_callback_close(zb_uint8_t param, zb_uint16_t tunnel_id)
{
    if (!param)
    {
        zb_buf_get_out_delayed_ext(tunneling_device_callback_close, tunnel_id, 0);
    }
    else
    {
        if (ZCL_CTX().device_cb)
        {

            zb_zcl_parsed_hdr_t cmd_info;
            zb_zcl_tunneling_close_tunnel_t close_tunnel;

            close_tunnel.tunnel_id = tunnel_id;
            ZB_BZERO(&cmd_info, sizeof(zb_zcl_parsed_hdr_t));

            cmd_info.addr_data.common_data.source.u.short_addr = zcl_tun_ctx.srv_table[tunnel_id].dst_addr;
            cmd_info.addr_data.common_data.src_endpoint = zcl_tun_ctx.srv_table[tunnel_id].dst_ep;
            cmd_info.addr_data.common_data.dst_endpoint = zcl_tun_ctx.srv_table[tunnel_id].src_ep;

            ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(
                param,
                ZB_ZCL_TUNNELING_CLOSE_TUNNEL_CB_ID,
                RET_ERROR,
                &cmd_info,
                &close_tunnel,
                NULL);

            ZCL_CTX().device_cb(param);
        }
        else
        {
            zb_buf_free(param);
        }
    }
}


static zb_bool_t zb_zcl_tunneling_age_tunnels()
{
    zb_uint16_t i;
    zb_int_t non_empty = 0;

    for (i = 0; i < ZB_ARRAY_SIZE(zcl_tun_ctx.srv_table); i++)
    {
        zb_zcl_tunneling_srv_rec_t *ent = &zcl_tun_ctx.srv_table[i];

        if (ent->close_tunnel_timeout)
        {
            ent->close_tunnel_timeout--;
            non_empty |= (ent->close_tunnel_timeout != 0);
            if (!ent->close_tunnel_timeout)
            {
                TRACE_MSG(TRACE_ZCL1, "zb_zcl_tunneling_age_tunnels: close tunnel %d", (FMT__D, i));

                zb_buf_get_out_delayed_ext(tunneling_device_callback_close, i, 0);
            }
        }
    }
    return (zb_bool_t)non_empty;
}

/* TODO: May use some unified aging scheme (one alarm, different counters)?
   Already have many independent aging systems (route, route discovery, ed aging, dups, etc) which
   brings additional load for alarms subsystem. */
void zb_zcl_tunneling_timer_cb(zb_uint8_t param)
{
    ZVUNUSED(param);

    zcl_tun_ctx.timer_running = zb_zcl_tunneling_age_tunnels();

    /* schedule myself again */
    if (zcl_tun_ctx.timer_running)
    {
        ZB_SCHEDULE_ALARM(zb_zcl_tunneling_timer_cb, 0, ZB_TIME_ONE_SECOND);
    }
}

void zb_zcl_tunneling_timer_start(void)
{
    if (!zcl_tun_ctx.timer_running)
    {
        zcl_tun_ctx.timer_running = ZB_TRUE;
        ZB_SCHEDULE_ALARM(zb_zcl_tunneling_timer_cb, 0, ZB_TIME_ONE_SECOND);
    }
}

void zb_zcl_tunneling_s_init()
{
    ZB_SCHEDULE_ALARM_CANCEL(zb_zcl_tunneling_timer_cb, ZB_ALARM_ANY_PARAM);
    ZB_BZERO(&zcl_tun_ctx, sizeof(zb_zcl_tunneling_ctx_t));
}

void zb_zcl_tunneling_init_server()
{
    if (zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_TUNNELING,
                                    ZB_ZCL_CLUSTER_SERVER_ROLE,
                                    (zb_zcl_cluster_check_value_t)NULL,
                                    (zb_zcl_cluster_write_attr_hook_t)NULL,
                                    zb_zcl_process_s_tunneling_specific_commands) == RET_OK)
    {
        zb_zcl_tunneling_s_init();
    }
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_TUNNELING || defined DOXYGEN */
