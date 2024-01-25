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
/* PURPOSE: ZLL reset to factory new
*/

#define ZB_TRACE_FILE_ID 2119
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#include "zll/zll_commissioning_internals.h"

#if defined ZB_ENABLE_ZLL


typedef struct zb_zll_reset_factory_new_param_s
{
    zb_uchar_t device_index;
}
zb_zll_reset_factory_new_param_t;

#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER
void zll_process_reset_to_fn(zb_uint8_t param);
#endif /* ZB_ZLL_ENABLE_COMMISSIONING_SERVER */


#ifdef ZB_ZLL_ENABLE_COMMISSIONING_CLIENT

void zll_reset_to_fn_confirm(zb_uint8_t param);

zb_ret_t zb_zll_reset_to_fn(zb_uint8_t param)
{
    zb_zll_reset_factory_new_param_t *reset_fn_param = ZB_BUF_GET_PARAM(param, zb_zll_reset_factory_new_param_t);
    zb_ret_t ret = RET_OK;
    zb_zll_ext_device_info_t *device_info;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zll_reset_to_fn, param %hd", (FMT__H, param));

    // Check that transaction is in process (flag transaction_active)
    if (!ZLL_TRAN_CTX().transaction_id)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR no active transaction", (FMT__0));
        ret = RET_INVALID_STATE;
    }
    else if (ZLL_GET_TRANSACTION_TASK_ID() == ZB_ZLL_TRANSACTION_RESET)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR reset request transmitting", (FMT__0));
        ret = RET_INVALID_STATE;
    }
    else
    {
        if (reset_fn_param->device_index <= ZLL_TRAN_CTX().n_device_infos)
        {
            device_info = &ZLL_TRAN_CTX().device_infos[reset_fn_param->device_index];
        }
        else
        {
            ret = RET_INVALID_PARAMETER_1;
            TRACE_MSG(TRACE_ERROR, "ERROR invalid parameter dev index %hd, total dev cnt %hd",
                      (FMT__H_H, reset_fn_param->device_index, ZLL_TRAN_CTX().n_device_infos));
        }
    }

    // Fill in Reset to factory new request
    if (ret == RET_OK)
    {
        ZLL_SET_TRANSACTION_TASK_ID(ZB_ZLL_TRANSACTION_RESET);
    }

    // Send Reset to factory new request to the target
    if (ret == RET_OK)
    {
        TRACE_MSG(TRACE_ZCL3, "call RESET_TO_FN_REQ", (FMT__0));
        ZB_ZLL_COMMISSIONING_SEND_RESET_TO_FN_REQ(
            param,
            device_info->device_addr,
            zll_reset_to_fn_confirm,
            ret);

        TRACE_MSG(TRACE_ZCL3, "RESET_TO_FN_REQ status %hd", (FMT__H, ret));
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zll_reset_to_fn ret %hd", (FMT__H, ret));
    return ret;
}

void zll_reset_to_fn_confirm(zb_uint8_t param)
{
    zb_uint8_t status = zb_buf_get_status(param);

    TRACE_MSG(TRACE_ZLL1, "> zll_reset_to_fn_confirm %hd", (FMT__H, param));

    zll_notify_task_result(
        param,
        (   (status == RET_OK )
            ?  ZB_ZLL_TASK_STATUS_FINISHED
            : ZB_ZLL_TASK_STATUS_FAILED));

    TRACE_MSG(TRACE_ZLL1, "< zll_reset_to_fn_confirm", (FMT__0));
}

#endif /* ZB_ZLL_ENABLE_COMMISSIONING_CLIENT */


#ifdef ZB_ZLL_ENABLE_COMMISSIONING_SERVER

zb_ret_t zll_reset_to_fn_req_handler(zb_uint8_t param)
{
    zb_bufid_t  buffer = param;
    zb_zll_commissioning_reset_to_fn_t reset_fn_req;
    zb_zcl_parsed_hdr_t cmd_info;
    zb_ret_t ret = RET_OK;
    zb_uint8_t resp_status = ZB_ZLL_GENERAL_STATUS_SUCCESS;

    TRACE_MSG(TRACE_ZCL1, ">> zll_reset_to_fn_req_handler, param %hd", (FMT__H, param));

    ZB_ZCL_COPY_PARSED_HEADER(buffer, &cmd_info);
    ZB_ZLL_COMMISSIONING_GET_RESET_TO_FN_REQ(&reset_fn_req, buffer, ret);

    if (ret == ZB_ZCL_PARSE_STATUS_FAILURE)
    {
        ret = RET_ILLEGAL_REQUEST;
        TRACE_MSG(TRACE_ERROR, "ERROR could not parse reset factory new request.", (FMT__0));
    }
    else if (reset_fn_req.trans_id != ZLL_TRAN_CTX().transaction_id)
    {
        ret = RET_INVALID_STATE;
        TRACE_MSG(TRACE_ERROR, "ERROR invalid transaction ID", (FMT__0));
    }

    if (ZB_ZLL_IS_FACTORY_NEW())
    {
        /*
        AT: see ZLL specification 8.4.7.2 Target procedure
        If the target is factory new or if a reset to factory new request inter-PAN command frame is received
        with an invalid transaction identifier (i.e. the frame was not received within the current active
        transaction), it shall discard the frame and perform no further processing.
        */
        ret = RET_INVALID_STATE;
        TRACE_MSG(TRACE_ZLL2, "This device, already factory new, so drop this request", (FMT__0));
    }

    if (ret == RET_OK)
    {
        TRACE_MSG(TRACE_ZLL2, "NFN device, call leave nwk", (FMT__0));
        ret = zb_buf_get_out_delayed(zll_process_reset_to_fn);

        /* AT: See: ZLL specification 8.4.7.2 Target procedure
        On receipt of a reset to factory new request inter-PAN command frame by a non-factory new target
        with a valid transaction identifier (i.e. immediately following a device discovery), it shall perform a
        leave request on the network by issuing the NLME-LEAVE.request primitive to the NWK layer and
        wait for the corresponding NLME-LEAVE.confirm response.
        ...
        The target shall not send a response to a reset to factory new request inter-PAN command frame.

        So i don't think that below code is required by specification.
        */

        if (ret != RET_INVALID_STATE)
        {
            if (ret != RET_OK)
            {
                TRACE_MSG(TRACE_ZLL1, "send join end device resp FAIL", (FMT__0));
                resp_status = ZB_ZLL_GENERAL_STATUS_FAILURE;
            }
            TRACE_MSG(TRACE_ZLL1, "send join end device resp, status %hd", (FMT__H, resp_status));
            ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ED_RES(
                buffer,
                resp_status,
                cmd_info.seq_number,
                cmd_info.addr_data.intrp_data.src_addr,
                NULL, ret);
        }
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zll_join_ed_req_handler, ret %hd", (FMT__H, ret));
    return ret;
}

void zll_process_reset_to_fn(zb_uint8_t param)
{
    zb_bufid_t  buffer = param;
    zb_ret_t status;

    TRACE_MSG(TRACE_ZLL2, ">> zll_process_reset_to_fn param %hd", (FMT__H, param));
    /* AT: here we need reset values to factory new
     * see:void zll_dev_start_continue_1(zb_uint8_t param)
     * restore ZLL ctx data (panId, flags)
     * extPanId also can be reseted in nwk_do_leave function before writing common data into storage
     */
    ZB_ZLL_SET_FACTORY_NEW();
    // Why aps_use_extended_pan_id??  ZB_MEMSET(ZB_AIB().aps_use_extended_pan_id, 0x00, sizeof(ZB_AIB().aps_use_extended_pan_id));

    ZLL_LEAVE_NWK(buffer, status);
    if (status != RET_OK)
    {
        TRACE_MSG(TRACE_ZLL1, "error in leave nwk status %hd", (FMT__H, status));
        zll_notify_task_result(buffer, ZB_ZLL_TASK_STATUS_FAILED);
    }

    /* Continue processing in zll_leave_nwk_confirm() */

    TRACE_MSG(TRACE_ZLL2, "<< zll_process_reset_to_fn param %hd", (FMT__H, param));
}

#endif /* ZB_ZLL_ENABLE_COMMISSIONING_SERVER */


#endif /* ZB_ENABLE_ZLL */
