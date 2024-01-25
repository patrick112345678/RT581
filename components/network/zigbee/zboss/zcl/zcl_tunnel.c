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
/* PURPOSE: ZBOSS specific Tunnel cluster, purpose: general data tunneling.
*/


#define ZB_TRACE_FILE_ID 2086

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_TUNNEL)

#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zcl/zb_zcl_tunnel.h"
#include "zb_zdo.h"
#include "zb_aps.h"

zb_zcl_tunnel_context_t tunnel_ctx;

zb_uint8_t gs_tunnel_client_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNEL_FC00_CLIENT_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_tunnel_server_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_TUNNEL_FC00_SERVER_ROLE_RECEIVED_CMD_LIST
};

zb_discover_cmd_list_t gs_tunnel_client_cmd_list =
{
    sizeof(gs_tunnel_client_received_commands), gs_tunnel_client_received_commands,
    sizeof(gs_tunnel_server_received_commands), gs_tunnel_server_received_commands
};

zb_discover_cmd_list_t gs_tunnel_server_cmd_list =
{
    sizeof(gs_tunnel_server_received_commands), gs_tunnel_server_received_commands,
    sizeof(gs_tunnel_client_received_commands), gs_tunnel_client_received_commands
};

static zb_bool_t tunnel_copy_received_data(zb_bufid_t io_buf, zb_uint8_t param,
        zb_uint8_t *received_data, zb_uint8_t data_size);
static zb_zcl_tunnel_io_slot_t *tunnel_get_io_slot(zb_uint8_t index);
static zb_uint8_t tunnel_find_io_slot(zb_zcl_tunnel_io_param_t *io_param);
static zb_uint8_t tunnel_get_new_io_slot(zb_uint8_t buf_ref);
static zb_ret_t tunnel_tx_data_portion(zb_uint8_t param, zb_uint8_t io_buf_param);
static void tunnel_invoke_user_app(zb_uint8_t index);
void zb_zcl_tunnel_call_tx_data(zb_uint8_t param, zb_uint16_t index16);
static void tunnel_free_slot_tail(zb_uint8_t param);

zb_bool_t zb_zcl_process_tunnel_specific_commands_srv(zb_uint8_t param);
zb_bool_t zb_zcl_process_tunnel_specific_commands_cli(zb_uint8_t param);

void zb_zcl_tunnel_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_TUNNEL,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_tunnel_specific_commands_srv);
}

void zb_zcl_tunnel_init_client()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_TUNNEL,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_tunnel_specific_commands_cli);
}


void zb_zcl_tunnel_init(zb_uint16_t manuf_id)
{
    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_init manuf_id %d", (FMT__D, manuf_id));

    tunnel_ctx.tunnel_cb = NULL;
    tunnel_ctx.manufacturer_id = manuf_id;
    ZB_MEMSET(tunnel_ctx.tunnel_io_slot, 0, sizeof(tunnel_ctx.tunnel_io_slot));

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_init", (FMT__0));
}

void zb_zcl_tunnel_register_cb(zb_zcl_tunnel_cb_t tunnel_cb)
{
    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_register_cb tunnel_cb %p", (FMT__P, tunnel_cb));

    tunnel_ctx.tunnel_cb = tunnel_cb;

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_register_cb", (FMT__0));
}

/* Return RET_OK if command is sent,
 *        RET_BUSY if we are sending to the same peer now
 *        RET_ERROR otherwise */
zb_ret_t zb_zcl_tunnel_transmit_data(zb_uint8_t buf_param)
{
    zb_zcl_tunnel_io_param_t *io_param;
    zb_ret_t ret = RET_OK;
    zb_uint8_t index;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_transmit_data param %hd", (FMT__H, buf_param));

    io_param = ZB_BUF_GET_PARAM(buf_param, zb_zcl_tunnel_io_param_t);
    /* modify io_param, set op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_TX */
    io_param->op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_TX;

    /* check: if io_slot found, return RET_BUSY - we
     * are sending to the same peer now */
    index = tunnel_find_io_slot(io_param);
    if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
    {
        /* On error caller should free input buffer himself */
        zb_zcl_tunnel_io_slot_t *io_slot = ZB_BUF_GET_PARAM(buf_param, zb_zcl_tunnel_io_slot_t);
        TRACE_MSG( TRACE_ZCL2, "Channel to peer (0x%x:%hd) for transfer data is busy", (FMT__D_H,
                   io_slot->io_param.peer_addr, io_slot->io_param.peer_ep));
        ret = RET_BUSY;
    }
    else
    {
        /* get new slot */
        zb_uint8_t index = tunnel_get_new_io_slot(buf_param);
        if ( index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
        {
            zb_zcl_tunnel_io_slot_t *io_slot = ZB_BUF_GET_PARAM(buf_param, zb_zcl_tunnel_io_slot_t);
            zb_zcl_tunnel_io_param_t *io_param = ZB_BUF_GET_PARAM(buf_param, zb_zcl_tunnel_io_param_t);
            // move transmit parameters
            ZB_MEMMOVE(&(io_slot->io_param), io_param, sizeof(zb_zcl_tunnel_io_param_t));
            // fill other slot parameter
            io_slot->io_param.op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_TX;
            io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_OK;
            io_slot->offset = 0;

            zb_buf_get_out_delayed_ext(zb_zcl_tunnel_send_fist_block, index, 0);
        }
        else
        {
            TRACE_MSG( TRACE_ZCL2, "No free Slot for add new command", (FMT__0));
            ret = RET_ERROR;
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_transmit_data %hd", (FMT__H, ret));

    return ret;
}

/**
 * Default User App
 * 1 Show command result - for sample
 * 2 Free buffer chain
 */
void zb_zcl_tunnel_default_user_app(zb_uint8_t param)
{
    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_default_user_app %hd", (FMT__H, param));

    // 1 Show command result - for sample
    if (param != 0)
    {
        zb_zcl_tunnel_io_param_t *io_param = ZB_BUF_GET_PARAM(param, zb_zcl_tunnel_io_param_t);
        TRACE_MSG( TRACE_ZCL2, "EP %hd Command %hd, command result %hd", (FMT__H_H_H,
                   io_param->src_ep, io_param->op_status.op_code, io_param->op_status.status));

        // 2 Free buffer chain
        tunnel_free_slot_tail(io_param->next_buf);

        zb_buf_free(param);
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_default_user_app", (FMT__0));
}

/* Generate ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_REQ */
static zb_ret_t tunnel_tx_data_portion(zb_uint8_t param, zb_uint8_t io_buf_param)
{
    zb_zcl_tunnel_io_slot_t *io_slot;
    zb_uint8_t data_size;

    TRACE_MSG( TRACE_ZCL1, "> tunnel_tx_data_portion param %hd, io_buf_param %hd",
               (FMT__H_H, param, io_buf_param));

    io_slot = ZB_BUF_GET_PARAM(io_buf_param, zb_zcl_tunnel_io_slot_t);

    ZB_ASSERT(io_slot);

    data_size = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - sizeof(zb_zcl_tunnel_transfer_data_req_t);
    if (data_size > io_slot->io_param.length - io_slot->offset)
    {
        data_size = io_slot->io_param.length - io_slot->offset;
    }

    // TODO Make select data from multi-buffer chain

    ZB_ZCL_TUNNEL_SEND_TRANSFER_REQ(param, io_slot->io_param.peer_addr,
                                    ZB_APS_ADDR_MODE_16_ENDP_PRESENT, io_slot->io_param.peer_ep, io_slot->io_param.src_ep,
                                    ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL,
                                    tunnel_ctx.manufacturer_id,
                                    (io_slot->offset == 0 ? ZB_ZCL_TUNNEL_TX_START : 0),
                                    (io_slot->offset == 0 ? io_slot->io_param.length : io_slot->offset),
                                    data_size,
                                    ((zb_uint8_t *)zb_buf_begin(io_buf_param) + io_slot->offset));

    io_slot->offset += data_size;

    TRACE_MSG( TRACE_ZCL1, "< tunnel_tx_data_portion", (FMT__0));
    return RET_OK;
}

/* return - if input buffer has be used */
static zb_bool_t tunnel_copy_received_data(zb_bufid_t io_buf, zb_uint8_t param,
        zb_uint8_t *received_data, zb_uint8_t data_size)
{
    zb_zcl_tunnel_io_slot_t *io_slot;
    zb_bool_t ret = ZB_FALSE;
    ZVUNUSED(param);

    TRACE_MSG( TRACE_ZCL1, "> tunnel_copy_received_data data_size %hd", (FMT__H, data_size));

    ZB_ASSERT(io_buf);

    // TODO Make copy data to multi-buffer chain

    io_slot = ZB_BUF_GET_PARAM(io_buf, zb_zcl_tunnel_io_slot_t);
    TRACE_MSG( TRACE_ZCL2, "data_size before %d", (FMT__D, io_slot->offset));

    if (zb_buf_len(io_buf) + data_size + sizeof(zb_zcl_tunnel_io_slot_t) <= ZB_IO_BUF_SIZE)
    {
        zb_uint8_t *ptr;
        ptr = zb_buf_alloc_right(io_buf, data_size);
        /* copy block */
        ZB_MEMMOVE(ptr, received_data, data_size);
        io_slot->offset += data_size;
        io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_OK;
    }
    else
    {
        // TODO Make copy data to multi-buffer chain

        TRACE_MSG( TRACE_ZCL2, "Need use multi-buffer chain code", (FMT__0));
        io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_ERROR_NO_MEMORY;
    }

    TRACE_MSG( TRACE_ZCL2, "data_size after %d", (FMT__D, io_slot->offset));

    TRACE_MSG( TRACE_ZCL1, "> tunnel_copy_received_data %hd", (FMT__H, ret));

    return ret;
}

zb_ret_t zb_zcl_tunnel_transfer_data_req_handler(zb_uint8_t buf_param)
{
    zb_ret_t ret = RET_OK;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_zcl_tunnel_transfer_data_req_data_internal_t req_data;
    zb_zcl_parse_status_t status;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_transfer_data_req_handler param %hd", (FMT__H, buf_param));

    ZB_ZCL_COPY_PARSED_HEADER(buf_param, &cmd_info);

    ZB_ZCL_TUNNEL_GET_TRANSFER_REQ(&req_data, buf_param, status);
    if (status != ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        TRACE_MSG( TRACE_ZCL2, "Invalid paylaod", (FMT__0));
        ret = RET_INVALID_PARAMETER;
    }
    else
    {
        zb_uint8_t index;
        zb_bool_t is_buffer_reused = ZB_FALSE;
        zb_zcl_tunnel_io_param_t io_param_tmp;

        /* fill in io_param_tmp for received data:
           - peer_addr
           - peer_ep
           - op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_RX
           - src_ep
        */
        io_param_tmp.peer_addr = cmd_info.addr_data.common_data.source.u.short_addr;
        io_param_tmp.peer_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint;
        io_param_tmp.op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_RX;
        io_param_tmp.src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint;

        /* implement without code doubling:

           find io_slot

           - (*) if tx_flag == ZB_ZCL_TUNNEL_TX_START
           -- if slot found then discard all the data
           -- create new io_slot
           - (**) if tx_flag == ZB_ZCL_TUNNEL_TX_CONTINUE,
           -- do nothing

           common part: copy data portion
           -- if slot is not found/create for previous, return Error
           -- if slot is found, copy data portion ...
        */

        // get slot and stop timeout timer
        index = tunnel_find_io_slot(&io_param_tmp);
        if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
        {
            ZB_SCHEDULE_ALARM_CANCEL(zb_zcl_tunnel_timeout, index);
        }

        TRACE_MSG( TRACE_ZCL2, "Index %hd tx_flag %hd", (FMT__H_H, index, req_data.req_header.tx_flag));

        // correct slot by tx_flag
        if (req_data.req_header.tx_flag == ZB_ZCL_TUNNEL_TX_START)
        {
            /* check if io_slot exist, discard all the data, restart data receiving */
            if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
            {
                zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);
                tunnel_free_slot_tail(io_slot->io_param.next_buf);
                zb_buf_free(io_slot->io_param.next_buf);
            }

            /* create new io_slot */
            is_buffer_reused = ZB_TRUE;

            /* get new io_slot */
            index = tunnel_get_new_io_slot(buf_param);

            /* fill slot structs */
            if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
            {
                zb_zcl_tunnel_io_slot_t *io_slot = ZB_BUF_GET_PARAM(buf_param, zb_zcl_tunnel_io_slot_t);
                ZB_MEMMOVE(&(io_slot->io_param), &io_param_tmp, sizeof(zb_zcl_tunnel_io_param_t));
                io_slot->io_param.length = req_data.req_header.byte_num;
                io_slot->io_param.next_buf = 0;
                io_slot->offset = 0;
                io_slot->seq = cmd_info.seq_number;
                io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_OK;

                zb_buf_reuse(buf_param);
                TRACE_MSG( TRACE_ZCL2, "new save data", (FMT__0));
            }
        }
        else
        {
            zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);
            if (io_slot != NULL)
            {
                io_slot->seq = cmd_info.seq_number;
                TRACE_MSG( TRACE_ZCL2, "continue save data", (FMT__0));
                if (req_data.req_header.byte_num != io_slot->offset)
                {
                    io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_ERROR;

                    ZB_SCHEDULE_CALLBACK2(zb_zcl_tunnel_transfer_data_resp_send, buf_param, index);
                    ret = RET_BUSY;      // input buffer reuse into this function
                }
            }
            else
            {
                TRACE_MSG( TRACE_ZCL2, "Slot is wrong.", (FMT__0));
            }
        }

        /* copy data and schedule response */
        if (ret == RET_OK)
        {
            if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
            {
                zb_bufid_t io_buf = tunnel_ctx.tunnel_io_slot[index];
                is_buffer_reused |= tunnel_copy_received_data(io_buf, buf_param, req_data.tun_data, req_data.req_header.data_size);

                if (is_buffer_reused)
                {
                    /* alloc new buffer to send response */
                    zb_buf_get_out_delayed_ext(zb_zcl_tunnel_transfer_data_resp_send, index, 0);
                }
                else
                {
                    /* re-use existing buffer to send response */
                    ZB_SCHEDULE_CALLBACK2(zb_zcl_tunnel_transfer_data_resp_send, buf_param, index);
                }

                ret = RET_BUSY;      // input buffer reuse into this function
            }
            else
            {
                ret = RET_TABLE_FULL;
            }
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_transfer_data_req_handler ret %hd", (FMT__H, ret));

    return ret;
}

/* Function sends ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_RESP and returns
 * data to a user if all the data is received */
void zb_zcl_tunnel_transfer_data_resp_send(zb_uint8_t param, zb_uint16_t index16)
{
    zb_uint8_t index = (zb_uint8_t)index16;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_send_response param %hd index %d", (FMT__H_D, param, index16));

    TRACE_MSG( TRACE_ZCL2, "index %d", (FMT__D, index16));

    if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN && tunnel_ctx.tunnel_io_slot[index] != 0)
    {
        zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);

        // TODO Calc offset for buffer-chain

        ZB_ZCL_TUNNEL_SEND_TRANSFER_RESP(
            param, io_slot->io_param.peer_addr,
            ZB_APS_ADDR_MODE_16_ENDP_PRESENT, io_slot->io_param.peer_ep, io_slot->io_param.src_ep,
            ZB_AF_HA_PROFILE_ID, io_slot->seq, NULL,
            tunnel_ctx.manufacturer_id,
            io_slot->io_param.op_status.status);

        /* will call user callback if error appeared or all the data is received */
        TRACE_MSG( TRACE_ZCL2, "current result status %hd, received %d all %d", (FMT__H_D_D,
                   io_slot->io_param.op_status.status, io_slot->offset, io_slot->io_param.length));

        if (io_slot->io_param.op_status.status != ZB_ZCL_TUNNEL_STATUS_OK ||
                io_slot->offset >= io_slot->io_param.length)
        {
            if (io_slot->io_param.op_status.status != ZB_ZCL_TUNNEL_STATUS_OK)
            {
                tunnel_free_slot_tail(io_slot->io_param.next_buf);
                io_slot->io_param.next_buf = 0;
            }

            tunnel_invoke_user_app(index);
        }
        else
        {
            /* start timer to handle receive timeout */
            ZB_SCHEDULE_ALARM(zb_zcl_tunnel_timeout, index, ZB_ZCL_TUNNEL_TIMEOUT);
        }
    }
    else
    {
        TRACE_MSG( TRACE_ZCL2, "wrong slot index or slot buffer", (FMT__0));
        zb_buf_free(param);
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_send_response", (FMT__0));
}

zb_ret_t zb_zcl_tunnel_transfer_data_resp_handler(zb_uint8_t buf_param)
{
    /*
      check if need to call user tunnel_cb:
      - if tun_status contains error status
      - if all the data is sent (with any status)
    */
    zb_zcl_tunnel_io_param_t io_param_tmp;
    zb_ret_t ret = RET_OK;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_zcl_tunnel_transfer_data_resp_t resp_data;
    zb_zcl_parse_status_t status;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_transfer_data_resp_handler param %hd", (FMT__H, buf_param));

    ZB_ZCL_COPY_PARSED_HEADER(buf_param, &cmd_info);

    ZB_ZCL_TUNNEL_GET_TRANSFER_RESP(&resp_data, buf_param, status);

    if (status != ZB_ZCL_PARSE_STATUS_SUCCESS)
    {
        ret = RET_INVALID_PARAMETER;
    }
    else
    {
        zb_uint8_t index;

        /* fill in io_param_tmp for received data:
           - peer_addr
           - peer_ep
           - op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_TX
        */
        io_param_tmp.peer_addr = cmd_info.addr_data.common_data.source.u.short_addr;
        io_param_tmp.peer_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint;
        io_param_tmp.op_status.op_code = ZB_ZCL_TUNNEL_OPERATION_TX;
        io_param_tmp.src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint;

        index = tunnel_find_io_slot(&io_param_tmp);

        /* found existing i/o slot */
        if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
        {
            zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);
            zb_bool_t no_error = (zb_bool_t)(resp_data.tun_status == ZB_ZCL_TUNNEL_STATUS_OK);

            ZB_SCHEDULE_ALARM_CANCEL(zb_zcl_tunnel_timeout, index);

            TRACE_MSG( TRACE_ZCL2, "no err %hd byte_num %d offset %d", (FMT__H_D_D,
                       no_error, io_slot->io_param.length, io_slot->offset));

            if ( no_error && (io_slot->io_param.length > io_slot->offset))
            {
                // send next block
                if (ZB_ZCL_CHECK_IF_SEND_DEFAULT_RESP(cmd_info, ZB_ZCL_STATUS_SUCCESS))
                {
                    /* Default response should be sent => alloc new buf for the next data portion transffer */
                    zb_buf_get_out_delayed_ext(zb_zcl_tunnel_call_tx_data, tunnel_ctx.tunnel_io_slot[index], 0);
                }
                else
                {
                    /* Reuse buffer to send the next data portion  */
                    tunnel_tx_data_portion(buf_param, tunnel_ctx.tunnel_io_slot[index]);
                    ret = RET_BUSY;
                }

                ZB_SCHEDULE_ALARM(zb_zcl_tunnel_timeout, index, ZB_ZCL_TUNNEL_TIMEOUT);
            }
            else // invoke User App
            {
                io_slot->io_param.op_status.status = (no_error && (io_slot->io_param.length == io_slot->offset))
                                                     ? ZB_ZCL_TUNNEL_STATUS_OK : resp_data.tun_status;

                tunnel_invoke_user_app(index);

                /* bo NOT release buffer here - it will be release or Default response
                 * will be sent from the caller func zb_zcl_process_tunnel_specific_commands */
            }
        }
        else
        {
            TRACE_MSG( TRACE_ZCL1, "TX slot not found", (FMT__0));
            ret = RET_INVALID_PARAMETER_1;
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_transfer_data_resp_handler ret %hd", (FMT__H, ret));
    return ret;
}

/** @brief Default Response command */
zb_ret_t zb_zcl_process_tunnel_default_response_commands(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_zcl_default_resp_payload_t *default_res;

    TRACE_MSG(TRACE_ZCL1, "> zb_zcl_tunnel_default_response_commands %hx", (FMT__H, param));

    default_res = ZB_ZCL_READ_DEFAULT_RESP(param);
    TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_DEFAULT_RESP: command_id 0x%hx, status: 0x%hx",
              (FMT__H_H, default_res->command_id, default_res->status));

    if ( (default_res->command_id == ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_REQ ||
            default_res->command_id == ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_RESP) &&
            (default_res->status != ZB_ZCL_STATUS_SUCCESS) )
    {
        zb_uint8_t index;
        zb_zcl_tunnel_io_param_t io_param_tmp;

        io_param_tmp.peer_addr = cmd_info->addr_data.common_data.source.u.short_addr;
        io_param_tmp.peer_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
        io_param_tmp.op_status.op_code =
            default_res->command_id == ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_REQ ?
            ZB_ZCL_TUNNEL_OPERATION_TX : ZB_ZCL_TUNNEL_OPERATION_RX;
        io_param_tmp.src_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint;

        index = tunnel_find_io_slot(&io_param_tmp);

        if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
        {
            zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);

            ZB_ASSERT(io_slot);

            // remove timeout handler for this slot
            ZB_SCHEDULE_ALARM_CANCEL(zb_zcl_tunnel_timeout, index);

            // ask UserApp about error
            io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_ERROR;
            // use first buffer from buffer-chain
            tunnel_free_slot_tail(io_slot->io_param.next_buf);
            io_slot->io_param.next_buf = 0;

            tunnel_invoke_user_app(index);
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_zcl_tunnel_default_response_commands %hd", (FMT__H, ret));

    return ret;
}

zb_bool_t zb_zcl_process_tunnel_specific_commands(zb_uint8_t param)
{
    zb_bool_t processed = ZB_TRUE;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_ret_t status = RET_OK;

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);

    TRACE_MSG( TRACE_ZCL1,
               "> zb_zcl_process_tunnel_specific_commands: param %d, cmd %d",
               (FMT__H_H, param, cmd_info.cmd_id));

    if (!cmd_info.is_common_command)
    {
        if (ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction)
        {
            switch ( cmd_info.cmd_id )
            {
            case ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_REQ:
                status = zb_zcl_tunnel_transfer_data_req_handler(param);
                break;

            default:
                processed = ZB_FALSE;
                break;
            }
        }
        else  // ZB_ZCL_FRAME_DIRECTION_TO_CLI
        {
            switch ( cmd_info.cmd_id )
            {
            case ZB_ZCL_CMD_TUNNEL_TRANSFER_DATA_RESP:
                status = zb_zcl_tunnel_transfer_data_resp_handler(param);
                break;

            default:
                processed = ZB_FALSE;
                break;
            }
        }
    }

    if ( processed )
    {
        if (!ZB_ZCL_CHECK_IF_SEND_DEFAULT_RESP(cmd_info, status))
        {
            TRACE_MSG( TRACE_ZCL3,
                       "Default response disabled",
                       (FMT__0));
            zb_buf_free(param);
        }
        else if (status != RET_BUSY)
        {
            zb_zcl_send_default_resp_ext(param,
                                         &cmd_info,
                                         ((status == RET_OK) ?
                                          ZB_ZCL_STATUS_SUCCESS :
                                          ZB_ZCL_STATUS_INVALID_FIELD));
        }
    }

    TRACE_MSG( TRACE_ZCL1,
               "< zb_zcl_process_tunnel_specific_commands: processed %d",
               (FMT__D, processed));
    return processed;
}


zb_bool_t zb_zcl_process_tunnel_specific_commands_srv(zb_uint8_t param)
{
    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_tunnel_server_cmd_list;
        return ZB_TRUE;
    }
    return zb_zcl_process_tunnel_specific_commands(param);
}


zb_bool_t zb_zcl_process_tunnel_specific_commands_cli(zb_uint8_t param)
{
    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_tunnel_client_cmd_list;
        return ZB_TRUE;
    }
    return zb_zcl_process_tunnel_specific_commands(param);
}


static zb_zcl_tunnel_io_slot_t *tunnel_get_io_slot(zb_uint8_t index)
{
    zb_zcl_tunnel_io_slot_t *io_slot = NULL;

    TRACE_MSG( TRACE_ZCL1, "> tunnel_get_io_slot index %hd %hd", (FMT__H_H, index, tunnel_ctx.tunnel_io_slot[index]));

    if (index < ZB_ZCL_TUNNEL_MAX_IO_SLOT_NUMBER)
    {
        if (tunnel_ctx.tunnel_io_slot[index] != 0 )
        {
            zb_bufid_t buf = tunnel_ctx.tunnel_io_slot[index];
            io_slot = ZB_BUF_GET_PARAM(buf, zb_zcl_tunnel_io_slot_t);
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< tunnel_get_io_slot %p", (FMT__P, io_slot));

    return io_slot;
}

static zb_uint8_t tunnel_find_io_slot(zb_zcl_tunnel_io_param_t *io_param)
{
    zb_uint8_t i;

    TRACE_MSG( TRACE_ZCL1, "> tunnel_find_io_slot addr %d ep %hd src_ep %hd op_code %hd", (FMT__D_H_H_H,
               io_param->peer_addr, io_param->peer_ep, io_param->src_ep, io_param->op_status.op_code));

    for (i = 0; i < ZB_ZCL_TUNNEL_MAX_IO_SLOT_NUMBER; i++)
    {
        if (tunnel_ctx.tunnel_io_slot[i] != 0)
        {
            zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(i);
            TRACE_MSG( TRACE_ZCL2, "i %hd addr %d ep %hd src_ep %hd op_code %hd", (FMT__H_D_H_H_H,
                       i, io_slot->io_param.peer_addr, io_slot->io_param.peer_ep, io_slot->io_param.src_ep,
                       io_slot->io_param.op_status.op_code));

            if (io_param->peer_addr == io_slot->io_param.peer_addr &&
                    io_param->peer_ep == io_slot->io_param.peer_ep &&
                    io_param->src_ep == io_slot->io_param.src_ep &&
                    io_param->op_status.op_code == io_slot->io_param.op_status.op_code)
            {
                break;
            }
        }
    }

    if (i == ZB_ZCL_TUNNEL_MAX_IO_SLOT_NUMBER)
    {
        /* slot not found */
        i = ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN;
    }

    TRACE_MSG( TRACE_ZCL1, "< tunnel_find_io_slot index %hd", (FMT__H, i));
    return i;
}

static zb_uint8_t tunnel_get_new_io_slot(zb_uint8_t buf_ref)
{
    zb_uint8_t i;

    TRACE_MSG( TRACE_ZCL1, "> tunnel_get_new_io_slot buf_ref %hd", (FMT__H, buf_ref));

    for (i = 0; i < ZB_ZCL_TUNNEL_MAX_IO_SLOT_NUMBER; i++)
    {
        if (tunnel_ctx.tunnel_io_slot[i] == 0)
        {
            tunnel_ctx.tunnel_io_slot[i] = buf_ref;
            break;
        }
    }

    if (i == ZB_ZCL_TUNNEL_MAX_IO_SLOT_NUMBER)
    {
        /* slot not found */
        i = ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN;
    }


    TRACE_MSG( TRACE_ZCL1, "< tunnel_get_new_io_slot ret index %hd", (FMT__H, i));

    return i;
}

static void tunnel_invoke_user_app(zb_uint8_t index)
{
    TRACE_MSG( TRACE_ZCL1, "> tunnel_invoke_user_app index %hd", (FMT__H, index));

    if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN && tunnel_ctx.tunnel_io_slot[index] != 0)
    {
        zb_uint8_t param = tunnel_ctx.tunnel_io_slot[index];

        zb_zcl_tunnel_io_slot_t *io_slot = ZB_BUF_GET_PARAM(param, zb_zcl_tunnel_io_slot_t);
        zb_zcl_tunnel_io_param_t *io_param = ZB_BUF_GET_PARAM(param, zb_zcl_tunnel_io_param_t);

        // free internal data
        ZB_MEMMOVE(io_param, &(io_slot->io_param), sizeof(zb_zcl_tunnel_io_param_t));

        tunnel_ctx.tunnel_io_slot[index] = 0;
        if (tunnel_ctx.tunnel_cb != NULL)
        {
            tunnel_ctx.tunnel_cb(param);
        }
        else
        {
            zb_zcl_tunnel_default_user_app(param);
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< tunnel_invoke_user_app", (FMT__0));
}

static void tunnel_free_slot_tail(zb_uint8_t param)
{
    TRACE_MSG( TRACE_ZCL1, "> tunnel_free_slot_tail param %hd", (FMT__H, param));

    while (param != 0)
    {
        zb_zcl_tunnel_io_slot_continue_t *io_next = ZB_BUF_GET_PARAM(param, zb_zcl_tunnel_io_slot_continue_t);
        param = io_next->next_buf;
        zb_buf_free(param);
    }

    TRACE_MSG( TRACE_ZCL1, "< tunnel_free_slot_tail", (FMT__0));
}

void zb_zcl_tunnel_send_fist_block(zb_uint8_t param, zb_uint16_t index16)
{
    zb_uint8_t index = (zb_uint8_t)index16;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_send_fist_block param %hd index %d", (FMT__H_D,
               param, index16));

    if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN)
    {
        zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);

        if (io_slot)
        {
            tunnel_tx_data_portion(param, tunnel_ctx.tunnel_io_slot[index]);

            ZB_SCHEDULE_ALARM(zb_zcl_tunnel_timeout, index, ZB_ZCL_TUNNEL_TIMEOUT);
        }
        else
        {
            TRACE_MSG( TRACE_ZCL2, "slot is empty", (FMT__0));
            zb_buf_free(param);
        }
    }
    else
    {
        TRACE_MSG( TRACE_ZCL2, "wrong slot index", (FMT__0));
        zb_buf_free(param);
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_send_fist_block", (FMT__0));
}

void zb_zcl_tunnel_call_tx_data(zb_uint8_t param, zb_uint16_t index16)
{
    zb_uint8_t index = (zb_uint16_t)index16;

    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_call_tx_data, param %hd, index16 %d",
               (FMT__H_D, param, index16));

    tunnel_tx_data_portion(param, tunnel_ctx.tunnel_io_slot[index]);

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_call_tx_data", (FMT__0));
}


void zb_zcl_tunnel_timeout(zb_uint8_t index)
{
    TRACE_MSG( TRACE_ZCL1, "> zb_zcl_tunnel_timeout index %hd", (FMT__H, index));

    if (index != ZB_ZCL_TUNNEL_IO_SLOT_UNKNOWN && tunnel_ctx.tunnel_io_slot[index] != 0)
    {
        zb_zcl_tunnel_io_slot_t *io_slot = tunnel_get_io_slot(index);
        if (io_slot)
        {
            io_slot->io_param.op_status.status = ZB_ZCL_TUNNEL_STATUS_ERROR_TIMEOUT;

            tunnel_free_slot_tail(io_slot->io_param.next_buf);
            io_slot->io_param.next_buf = 0;

            tunnel_invoke_user_app(index);
        }
        else
        {
            TRACE_MSG( TRACE_ZCL1, "Error, NULL io_slot", (FMT__0));
        }
    }

    TRACE_MSG( TRACE_ZCL1, "< zb_zcl_tunnel_timeout", (FMT__0));
}
#endif /* if defined (ZB_ZCL_SUPPORT_CLUSTER_TUNNEL) */
