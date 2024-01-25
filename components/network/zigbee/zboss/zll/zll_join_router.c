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
/* PURPOSE: ZLL join router functions
*/

#define ZB_TRACE_FILE_ID 2116
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#include "zll/zll_commissioning_internals.h"

#if defined ZB_ENABLE_ZLL

#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER
void zll_call_start_router_cmd(zb_uint8_t param);
void zll_call_leave_nwk_cmd(zb_uint8_t param);
#endif

#ifdef ZB_ZLL_ENABLE_COMMISSIONING_CLIENT

void zll_join_router_timeout(zb_uint8_t param);
void zll_join_router_res_handler_continue(zb_uint8_t param);

void zb_zll_join_router(zb_uint8_t param)
{
    zb_zll_join_router_param_t *join_router_param =
        ZB_BUF_GET_PARAM(param, zb_zll_join_router_param_t);
    zb_ret_t ret = RET_OK;
    zb_uint16_t target_addr;
    zb_zll_ext_device_info_t *device_info;
    zb_zll_group_id_range_t device_group_range = {0, 0};
    zb_zll_group_id_range_t free_group_range = {0, 0};
    zb_zll_addr_range_t free_addr_range = {0, 0};

    TRACE_MSG(TRACE_ZLL1, ">> zb_zll_join_router, param %hd", (FMT__H, param));

    if (!ZLL_TRAN_CTX().transaction_id)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR no active transaction", (FMT__0));
        ret = RET_INVALID_STATE;
    }
    else
    {
        if (join_router_param->device_index <= ZLL_TRAN_CTX().n_device_infos)
        {
            device_info = &ZLL_TRAN_CTX().device_infos[join_router_param->device_index];
        }
        else
        {
            ret = RET_INVALID_PARAMETER_1;
            TRACE_MSG(TRACE_ERROR, "ERROR invalid parameter dev index %hd, total dev cnt %hd",
                      (FMT__H_H, join_router_param->device_index, ZLL_TRAN_CTX().n_device_infos));
        }
    }
    if (ret == RET_OK)
    {
        ZLL_ACTION_SEQUENCE_INIT();
        ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK);

        target_addr = zll_get_new_addr();
        TRACE_MSG(TRACE_ZLL3, "target addr %x", (FMT__D, target_addr));
        if (target_addr == ZB_ZLL_ADDR_ERROR)
        {
            ret = RET_ERROR;
            TRACE_MSG(TRACE_ERROR, "Error getting address", (FMT__0));
        }
    }

    if (ret == RET_OK)
    {
        if (device_info->total_group_id_count)
        {
            TRACE_MSG(TRACE_ZLL3, "get group range %hd", (FMT__H, device_info->total_group_id_count));
            ret = zll_get_group_id_range(&device_group_range, device_info->total_group_id_count);
        }

        if (ret == RET_OK && ZB_ZLL_DEVICE_INFO_GET_ADDR_ASSIGNMENT(device_info->zll_info))
        {
            TRACE_MSG(TRACE_ZLL3, "get default group range", (FMT__0));
            ret = zll_get_group_id_range(&free_group_range, ZB_ZLL_DEFAULT_GROUP_ID_RANGE);

            if (ret == RET_OK)
            {
                TRACE_MSG(TRACE_ZLL3, "get addr range", (FMT__0));
                ret = zll_get_addr_range(&free_addr_range);
            }
        }
    }

    if (ret == RET_OK)
    {
        TRACE_MSG(TRACE_ZLL3, "call NETWORK_JOIN_ROUTER_REQ", (FMT__0));

        /* for TP-PRE-TC-04 update existing address info */
        {
            zb_address_ieee_ref_t ref;
            zb_address_update(device_info->device_addr, target_addr, ZB_FALSE, &ref);
        }

        ZB_IEEE_ADDR_COPY(ZLL_TRAN_CTX().command_data.nwk_join_router.target_ieee_addr,
                          device_info->device_addr);

        ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_REQ(
            param,
            ZLL_DEVICE_INFO().key_index,
            ZLL_DEVICE_INFO().encr_nwk_key,
            target_addr,
            device_group_range.group_id_begin,
            device_group_range.group_id_end,
            free_addr_range.addr_begin,
            free_addr_range.addr_end,
            free_group_range.group_id_begin,
            free_group_range.group_id_end,
            device_info->device_addr,
            NULL,
            ret);

        TRACE_MSG(TRACE_ZLL3, "NETWORK_JOIN_ROUTER_REQ status %hd", (FMT__H, ret));
    }

    if (ret == RET_OK)
    {
        ZB_SCHEDULE_ALARM(zll_join_router_timeout, ZB_ALARM_ANY_PARAM, ZB_ZLL_APLC_INTRP_TRANSID_LIFETIME);
    }

    if (ret != RET_OK && ZLL_GET_TRANSACTION_TASK_ID() == ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK)
    {
        TRACE_MSG(TRACE_ZLL1, "error occured, rollback addr and group id", (FMT__0));
        ZLL_ACTION_SEQUENCE_ROLLBACK();
    }

    TRACE_MSG(TRACE_ZLL1, "<< zb_zll_join_router ret %hd", (FMT__H, ret));
}

void zll_join_router_timeout(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZLL1, ">> zll_join_router_timeout, param %hd, task %hd",
              (FMT__H_H, param, ZLL_GET_TRANSACTION_TASK_ID()));

    if (ZLL_GET_TRANSACTION_TASK_ID() == ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK)
    {
        if (param == ZB_ALARM_ANY_PARAM)
        {
            TRACE_MSG(TRACE_ZLL3, "reschedule itself with new buffer", (FMT__0));
            zb_buf_get_out_delayed(zll_join_router_timeout);
        }
        else
        {
            TRACE_MSG(TRACE_ZLL3, "rollback transaction, inform user", (FMT__0));
            zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
            ZLL_ACTION_SEQUENCE_ROLLBACK();
        }
    }

    TRACE_MSG(TRACE_ZLL1, "<< zll_join_router_timeout", (FMT__0));
}


zb_ret_t zll_join_router_res_handler(zb_uint8_t param)
{
    zb_zll_commissioning_network_join_router_res_t response;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_ret_t ret;

    TRACE_MSG(TRACE_ZLL1, ">> zll_join_router_res_handler, param %hd", (FMT__H, param));

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
    ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ROUTER_RES(&response, param, ret);

    if (ret == ZB_ZCL_PARSE_STATUS_FAILURE)
    {
        ret = RET_ILLEGAL_REQUEST;
        TRACE_MSG(TRACE_ERROR, "ERROR could not parse start join router response.", (FMT__0));
        zb_buf_free(param);
    }
    else if (response.trans_id != ZLL_TRAN_CTX().transaction_id
             || ZLL_GET_TRANSACTION_TASK_ID() != ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK)
    {
        ret = RET_INVALID_STATE;
        TRACE_MSG(TRACE_ZLL1, "ERROR invalid transaction task %hd",
                  (FMT__H, ZLL_GET_TRANSACTION_TASK_ID()));
        zb_buf_free(param);
    }
    else
    {
        ZB_SCHEDULE_ALARM_CANCEL(zll_join_router_timeout, ZB_ALARM_ANY_PARAM);

        TRACE_MSG(TRACE_ZLL3, "response status %hd", (FMT__H, response.status));
        if (response.status) /* Start new network failed. */
        {
            ret = RET_ERROR;
            TRACE_MSG(TRACE_ZLL1, "ERROR Target failed to join router %hd", (FMT__H, response.status));
            zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
            ZLL_ACTION_SEQUENCE_ROLLBACK();
        }
        else
        {
            TRACE_MSG(TRACE_ZLL3, "wait for min_startup_delay and continue", (FMT__0));
            ret = zb_schedule_alarm(zll_join_router_res_handler_continue, param,
                                    ZB_ZLL_APLC_MIN_STARTUP_DELAY_TIME);
        }
    }

    TRACE_MSG(TRACE_ZLL1, "<< zll_join_router_res_handler ret %hd", (FMT__H, ret));
    return ret;
}

void zll_network_joined_router_signal(zb_uint8_t param)
{
    zb_ret_t status = RET_ERROR;
    zb_zll_sub_device_info_t *dev_info = ZB_ZLL_TRANS_GET_CURRENT_DEV_EP_INFO();
    zb_bdb_signal_touchlink_nwk_joined_router_t *sig_params =
        (zb_bdb_signal_touchlink_nwk_joined_router_t *)zb_app_signal_pack(
            param,
            ZB_BDB_SIGNAL_TOUCHLINK_NWK_JOINED_ROUTER,
            RET_OK,
            sizeof(zb_bdb_signal_touchlink_nwk_started_params_t));

    TRACE_MSG(TRACE_ZLL1, "zll_network_joined_router_signal", (FMT__0));
    ZB_IEEE_ADDR_COPY(sig_params->device_ieee_addr, ZLL_TRAN_CTX().command_data.nwk_join_router.target_ieee_addr);
    ZB_ZLL_FIND_DEVICE_INFO_BY_ADDR(ZLL_TRAN_CTX().command_data.nwk_join_router.target_ieee_addr, status);
    /* Assume here that we have 1 sub-device */
    ZB_ASSERT((status == RET_OK) && (dev_info != NULL));
    sig_params->endpoint = dev_info->ep_id;
    sig_params->profile_id = dev_info->profile_id;
    TRACE_MSG(TRACE_APP1, "profile 0x%x ep %hd", (FMT__D_H, sig_params->profile_id, sig_params->endpoint));
    ZB_SCHEDULE_CALLBACK(zb_zdo_startup_complete, param);
}

void zll_join_router_res_handler_continue(zb_uint8_t param)
{
    zb_bufid_t  buffer = param;
    zb_uint8_t status;

    TRACE_MSG(TRACE_ZLL1, ">> zll_join_router_res_handler_continue param %hd", (FMT__H, param));

    zll_save_nwk_prefs(ZB_NIB_EXT_PAN_ID(),
                       ZB_PIBCACHE_PAN_ID(),
                       ZB_PIBCACHE_NETWORK_ADDRESS(),
                       (ZLL_DEVICE_INFO().nwk_channel) ?
                       ZLL_DEVICE_INFO().nwk_channel :
                       ZB_PIBCACHE_NETWORK_ADDRESS());

    /* NK: Hope we will not start new network before sending this signal... */
    zb_buf_get_out_delayed(zll_network_joined_router_signal);

    ZLL_REJOIN_NWK(buffer, ZB_NIB_EXT_PAN_ID(),
                   (ZLL_DEVICE_INFO().nwk_channel) ?
                   ZLL_DEVICE_INFO().nwk_channel :
                   ZB_PIBCACHE_NETWORK_ADDRESS(), status);

    if (status != RET_OK)
    {
        zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_FAILED);
        ZLL_ACTION_SEQUENCE_ROLLBACK();
        zb_buf_free(param);
    }

    /* If rejoin request is scheduked, request status will be informed
     * to user app from zb_zdo_startup_complete () */

    TRACE_MSG(TRACE_ZLL1, "<< zll_join_router_res_handler_continue", (FMT__0));
}

#endif /* ZB_ZLL_ENABLE_COMMISSIONING_CLIENT */



#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER

#if defined ZB_ROUTER_ROLE

zb_ret_t zll_join_router_req_handler(zb_uint8_t param)
{
    zb_zll_commissioning_network_join_router_req_t join_router_req;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_ret_t ret = RET_OK;
    zb_uint8_t resp_status = ZB_ZLL_GENERAL_STATUS_SUCCESS;

    TRACE_MSG(TRACE_ZLL1, ">> zll_join_router_req_handler, param %hd", (FMT__H, param));

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
    ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ROUTER_REQ(&join_router_req, param, ret);


    if (ret == ZB_ZCL_PARSE_STATUS_FAILURE)
    {
        ret = RET_ILLEGAL_REQUEST;
        TRACE_MSG(TRACE_ERROR, "ERROR could not parse start join router request.", (FMT__0));
    }
    else if (join_router_req.trans_id != ZLL_TRAN_CTX().transaction_id)
    {
        ret = RET_INVALID_STATE;
        TRACE_MSG(TRACE_ERROR, "ERROR invalid transaction ID", (FMT__0));
        zb_buf_free(param);
    }
    else
    {
        TRACE_MSG(TRACE_ZLL3, "req.ext pan id " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(join_router_req.ext_pan_id)));
        TRACE_MSG(TRACE_ZLL3, "NIB ext pan id " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));

        /* Debug this point: maybe ZB_NIB_EXT_PAN_ID() aps_use_extended_pan_id should be used */
        if (ZB_EXTPANID_CMP(join_router_req.ext_pan_id, ZB_NIB_EXT_PAN_ID()))
        {
            TRACE_MSG(TRACE_ZLL1, "We already joined this network", (FMT__0));
            ret = RET_ERROR;
        }
        else
        {
            TRACE_MSG(TRACE_ZLL3, "check_action_allowed %p", (FMT__P, ZLL_TRAN_CTX().check_action_allowed));
            if (ZLL_TRAN_CTX().check_action_allowed)
            {
                ret = ZLL_TRAN_CTX().check_action_allowed(ZB_ZLL_ACTION_JOIN_ROUTER);
                if (!ret)
                {
                    ret = RET_ERROR;
                    TRACE_MSG(TRACE_ZLL1, "User App rejected joining to the new network", (FMT__0));
                }
            }
        }
    }

    if (ret == RET_OK)
    {
        // save parameters for start router
        ZB_EXTPANID_COPY(ZLL_TRAN_CTX().command_data.start_router_param.ext_pan_id,
                         join_router_req.ext_pan_id);
        ZLL_TRAN_CTX().command_data.start_router_param.short_pan_id = join_router_req.pan_id;
        ZLL_TRAN_CTX().command_data.start_router_param.channel = join_router_req.channel;
        ZLL_TRAN_CTX().command_data.start_router_param.short_addr = join_router_req.short_addr;

        if (ZB_NIB_SECURITY_LEVEL())
        {
            zll_calc_enc_dec_nwk_key(join_router_req.encr_nwk_key,
                                     secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number),
                                     join_router_req.key_idx,
                                     ZLL_DEVICE_INFO().transaction_id,
                                     ZLL_DEVICE_INFO().response_id,
                                     ZB_FALSE);
            ZG->aps.authenticated = ZB_TRUE;
        }

        TRACE_MSG(TRACE_ZLL3, "set tran task ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK_TGT", (FMT__0));
        ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK_TGT);

        if (ZB_ZLL_DEVICE_INFO_GET_FACTORY_NEW(ZLL_DEVICE_INFO().zll_info))
        {
            ZB_SET_JOINED_STATUS(ZB_TRUE);

            ZB_ZLL_CLEAR_FACTORY_NEW();

            TRACE_MSG(TRACE_ZLL2, "FN device, call start_router (1)", (FMT__0));
            ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_RES(
                param,
                resp_status,
                cmd_info.seq_number,
                cmd_info.addr_data.intrp_data.src_addr,
                /* restart router on the new channel after sending resp */
                zll_call_start_router_cmd,
                ret);
        }
        else
        {
            TRACE_MSG(TRACE_ZLL2, "NFN device, will call leave nwk", (FMT__0));
            ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_RES(
                param,
                resp_status,
                cmd_info.seq_number,
                cmd_info.addr_data.intrp_data.src_addr,
                /* call leave after sending resp */
                zll_call_leave_nwk_cmd,
                ret);
        }
    }
    TRACE_MSG(TRACE_ZCL1, "<< zll_join_router_req_handler, ret %hd", (FMT__H, ret));
    return ret;
}

void zll_network_join_router_resp_confirm(zb_uint8_t param)
{
    ZVUNUSED(param);
}

void zll_call_start_router_cmd(zb_uint8_t param)
{
    zb_zll_start_router_param_t *start_param = ZB_BUF_GET_PARAM(param, zb_zll_start_router_param_t);

    TRACE_MSG(TRACE_ZLL2, ">> zll_call_start_router_cmd param %hd", (FMT__H, param));

    ZB_MEMCPY(start_param, &ZLL_TRAN_CTX().command_data.start_router_param, sizeof(zb_zll_start_router_param_t));

    zll_start_router(param);

    TRACE_MSG(TRACE_ZLL2, "<< zll_call_start_router_cmd param %hd", (FMT__H, param));
}

void zll_call_leave_nwk_cmd(zb_uint8_t param)
{
    zb_ret_t status;

    TRACE_MSG(TRACE_ZLL2, ">> zll_call_leave_nwk_cmd param %hd", (FMT__H, param));

    ZLL_LEAVE_NWK(param, status);
    if (status != RET_OK)
    {
        TRACE_MSG(TRACE_ZLL1, "error in leave nwk status %hd", (FMT__H, status));
        zll_notify_task_result(param, ZB_ZLL_TASK_STATUS_FAILED);
    }

    /* Continue processing in zll_leave_nwk_confirm() */

    TRACE_MSG(TRACE_ZLL2, "<< zll_call_leave_nwk_cmd param %hd", (FMT__H, param));
}


#else /* ZB_ROUTER_ROLE */

zb_ret_t zll_join_router_req_handler(zb_uint8_t param)
{
    zb_bufid_t  buffer = param;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_ZCL1, ">> zll_join_router_req_handler, param %hd ROUTER ROLE NOT SUPPORTED",
              (FMT__H, param));

    ZB_ZCL_COPY_PARSED_HEADER(buffer, &cmd_info);

    ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_RES(
        buffer,
        ZB_ZLL_GENERAL_STATUS_FAILURE,
        cmd_info.seq_number,
        cmd_info.addr_data.intrp_data.src_addr,
        NULL, ret);

    TRACE_MSG(TRACE_ZCL1, "<< zll_join_router_req_handler", (FMT__0));
    return ret;
}

#endif /* ZB_ROUTER_ROLE */

#endif /* ZB_ZLL_ENABLE_COMMISSIONING_SERVER */


#ifdef ZB_COMPILE_ZLL_SAMPLE

void test_call_join_router(zb_bufid_t buffer, zb_uint16_t dev_index, zb_callback_t cb)
{
    zb_ret_t ret = RET_OK;
    (void)cb;
    (void)ret;

    ZB_ZLL_JOIN_ROUTER(buffer, dev_index, cb, ret);
}

#endif

#endif /* ZB_ENABLE_ZLL */
