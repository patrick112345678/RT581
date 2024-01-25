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
/* PURPOSE: CLIENT: Sub-GHz cluster
*/

#define ZB_TRACE_FILE_ID 1944

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ) || defined DOXYGEN

#include "zcl/zb_zcl_subghz.h"

#if !defined ZB_ENABLE_SE_MIN_CONFIG && !defined ZB_ENABLE_HA
#error "Profile not defined"
#endif /* !ZB_ENABLE_SE_MIN_CONFIG && !ZB_ENABLE_HA */

static zb_uint8_t gs_subghz_client_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_SUBGHZ_CLIENT_ROLE_RECEIVED_CMD_LIST
};

static zb_uint8_t gs_subghz_client_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_SUBGHZ_CLIENT_ROLE_GENERATED_CMD_LIST
};

static zb_discover_cmd_list_t gs_subghz_client_cmd_list =
{
    sizeof(gs_subghz_client_received_commands), gs_subghz_client_received_commands,
    sizeof(gs_subghz_client_generated_commands), gs_subghz_client_generated_commands
};

/* *****************************************************************************
 * Internal calls
 * *****************************************************************************/

static void zb_subghz_send_suspend_signal(zb_uint8_t param);

static void zb_subghz_cli_resume_zcl_messages(zb_uint8_t param);


/* sends ZCL default response */
static zb_bool_t zb_subghz_send_default_response(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info,
        zb_zcl_status_t status)
{

    TRACE_MSG(TRACE_ZCL1, "== zb_subghz_send_default_response: status == %d", (FMT__D, status));

    if (!cmd_info->disable_default_response)
    {
        ZB_ZCL_SEND_DEFAULT_RESP(param,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr,
                                 ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint,
                                 ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_endpoint,
                                 cmd_info->profile_id,
                                 cmd_info->cluster_id,
                                 cmd_info->seq_number,
                                 cmd_info->cmd_id,
                                 status);
        return ZB_TRUE;
    }

    return ZB_FALSE;
}

static void zb_subghz_send_suspend_signal(zb_uint8_t param)
{
    if (param == 0U)
    {
        (void)zb_buf_get_out_delayed(zb_subghz_send_suspend_signal);
    }
    else
    {
        if (ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
        {
            zb_uint_t *susp_period = (zb_uint_t *)zb_app_signal_pack(param, ZB_SIGNAL_SUBGHZ_SUSPEND, RET_OK, (zb_uint8_t)sizeof(zb_uint_t));
            *susp_period = 0;
#ifdef ZB_ED_ROLE
            {
                zb_time_t t;

                if (zb_schedule_get_alarm_time(zb_subghz_cli_resume_zcl_messages, 0, &t) == RET_OK)
                {
                    *susp_period = ZB_TIME_SUBTRACT(t, ZB_TIMER_GET());
                    *susp_period = ZB_TIME_BEACON_INTERVAL_TO_MSEC(*susp_period);
                    *susp_period /= (1000 * 60);
                }
            }
#endif
            if (*susp_period == 0U)
            {
                *susp_period = (zb_uint_t) -1;
            }
            ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
        }
        else
        {
            (void)zb_app_signal_pack(param, ZB_SIGNAL_SUBGHZ_RESUME, RET_OK, 0);
            ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);
        }
    }
}


/* *****************************************************************************
 * Client-side implementation
 * *****************************************************************************/

/* sends GetSuspendZCLMessagesStatus command */
void zb_subghz_cli_get_suspend_zcl_messages_status(zb_uint8_t param,
        zb_addr_u *dst_addr,
        zb_uint8_t dst_addr_mode,
        zb_uint8_t dst_ep,
        zb_uint8_t src_ep)
{
    TRACE_MSG(TRACE_ZCL1, ">> zb_subghz_cli_get_suspend_zcl_messages_status", (FMT__0));

    zb_zcl_send_cmd(param,
                    dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep,
                    NULL, 0, NULL,
                    ZB_ZCL_CLUSTER_ID_SUB_GHZ,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    (zb_uint8_t)ZB_ZCL_SUBGHZ_CLI_CMD_GET_SUSPEND_ZCL_MESSAGES_STATUS,
                    NULL
                   );

    TRACE_MSG(TRACE_ZCL1, "<< zb_subghz_cli_get_suspend_zcl_messages_status", (FMT__0));
}


/* resumes sending ZCL messages by client */
static void zb_subghz_cli_resume_zcl_messages(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_FALSE;
    ZB_SCHEDULE_CALLBACK(zb_subghz_send_suspend_signal, param);
    TRACE_MSG(TRACE_ZCL1, "== zb_subghz_cli_resume_zcl_messages: ZCL exchange RESUMED", (FMT__0));
}


/* process SuspendZCLMessages command */
static zb_bool_t zb_subghz_cli_suspend_zcl_messages(zb_uint8_t param)
{
    zb_uint8_t suspension_period;

    suspension_period = *((zb_uint8_t *)zb_buf_begin(param));
    TRACE_MSG(TRACE_ZCL1, ">> zb_subghz_cli_suspend_zcl_messages: %d min", (FMT__D, suspension_period));

    /* D.14.2.3.1.4 Effect on Receipt
     *
     * On receipt of this command with a non-zero payload, the device shall suspend its ZCL
     * communications to the server device for the period indicated by the payload, at which time
     * normal operation may resume.
     */
    if (suspension_period != 0U)
    {
        ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_TRUE;
        ZB_SCHEDULE_ALARM_CANCEL(zb_subghz_cli_resume_zcl_messages, 0);
        ZB_SCHEDULE_ALARM(zb_subghz_cli_resume_zcl_messages, 0, (zb_time_t)suspension_period * 60U * ZB_TIME_ONE_SECOND);
        TRACE_MSG(TRACE_ZCL1, "ZCL exchange SUSPENDED period %d minutes", (FMT__D, suspension_period));
        /* stop turbo poll */
        zb_zdo_pim_turbo_poll_continuous_leave(0);
        zb_zdo_turbo_poll_packets_leave(0);
#if defined ZB_SUSPEND_APSDE_DATAREQ_BY_SUBGHZ_CLUSTER && defined SNCP_MODE
        zb_aps_cancel_outgoing_trans(0);
#endif
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "ZCL exchange UNSUSPENDED", (FMT__0));
        ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages = ZB_FALSE;
        ZB_SCHEDULE_ALARM_CANCEL(zb_subghz_cli_resume_zcl_messages, 0);
    }
    ZB_SCHEDULE_CALLBACK(zb_subghz_send_suspend_signal, 0);

    TRACE_MSG(TRACE_ZCL1, "<< zb_subghz_cli_suspend_zcl_messages", (FMT__0));

    /* zb_buf_free(param); */
    return ZB_TRUE;
}


static zb_bool_t zb_subghz_cli_handle_server_commands(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_bool_t res;

    switch ((zb_zcl_subghz_srv_cmd_t)cmd_info->cmd_id)
    {
    case ZB_ZCL_SUBGHZ_SRV_CMD_SUSPEND_ZCL_MESSAGES:
        (void)zb_subghz_cli_suspend_zcl_messages(param);
        /* FIXME: Do we need a ZCL default response here given that
         *        zb_subghz_cli_suspend_zcl_messages() is always successful
         *        If NO, then uncomment zb_buf_free(param); in zb_subghz_cli_suspend_zcl_messages()
         */
        res = zb_subghz_send_default_response(param, cmd_info, ZB_ZCL_STATUS_SUCCESS);
        break;

    default:
        res = zb_subghz_send_default_response(param, cmd_info, ZB_ZCL_STATUS_UNSUP_CLUST_CMD);
        break;
    }

    return res;
}


static void discover_zc_endpoint_cb(zb_uint8_t param)
{
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(param);

    if (resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len > 0U)
    {
        ZCL_CTX().subghz_ctx.cli.zc_ep = *(zb_uint8_t *)(resp + 1);
        TRACE_MSG(TRACE_ZCL3, "discover_zc_endpoint_cb: ZC ep, %hd", (FMT__H, ZCL_CTX().subghz_ctx.cli.zc_ep));
        ZB_SCHEDULE_CALLBACK(zb_subghz_start_suspend_status_poll, param);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "discover_zc_endpoint_cb: error", (FMT__0));
        zb_buf_free(param);
    }
}


void zb_subghz_start_suspend_status_poll(zb_uint8_t param)
{
    zb_bool_t is_joined;
    zb_bool_t is_rx_on_when_idle;
    zb_uint8_t current_page;

    ZB_SCHEDULE_ALARM_CANCEL(zb_subghz_start_suspend_status_poll, 0);
    TRACE_MSG(TRACE_ZCL3, "zb_subghz_start_suspend_status_poll param %hd joined %d",
              (FMT__H_D, param, zb_zdo_joined()));

    is_joined = zb_zdo_joined();
    is_rx_on_when_idle = zb_get_rx_on_when_idle();
    current_page = zb_get_current_page();
    if (is_joined
            && !is_rx_on_when_idle
            && current_page != 0U)
    {
        if (param == 0U)
        {
            (void)zb_buf_get_out_delayed(zb_subghz_start_suspend_status_poll);
        }
        else
        {
            zb_time_t next_timeout = zb_zdo_get_poll_interval_ms();
            if (next_timeout < zb_mac_duty_cycle_get_time_period_sec() * 1000U)
            {
                next_timeout = zb_mac_duty_cycle_get_time_period_sec() * 1000U;
            }

            if (ZCL_CTX().subghz_ctx.cli.zc_ep == 0U)
            {
                zb_zdo_match_desc_param_t *req;

                req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + (1U) * sizeof(zb_uint16_t));
                req->nwk_addr = 0;
                req->addr_of_interest = 0;
                req->profile_id = ZB_ZCL_SUBGHZ_CLUSTER_PROFILE_ID();
                req->num_in_clusters = 1;
                req->num_out_clusters = 0;
                req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_SUB_GHZ;
                TRACE_MSG(TRACE_ZCL3, "zb_subghz_start_suspend_status_poll: no ep, ask ZC", (FMT__0));
                (void)zb_zdo_match_desc_req(param, discover_zc_endpoint_cb);
            }
            else
            {
                zb_addr_u dst_addr;
                dst_addr.addr_short = 0;

                zb_subghz_cli_get_suspend_zcl_messages_status(param,
                        &dst_addr,
                        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                        ZCL_CTX().subghz_ctx.cli.zc_ep,
                        ZCL_CTX().subghz_ctx.ep);

                TRACE_MSG(TRACE_ZCL3, "zb_subghz_start_suspend_status_poll reschedule after %d ms", (FMT__D, next_timeout));
                ZB_SCHEDULE_ALARM(zb_subghz_start_suspend_status_poll, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(next_timeout));
            }
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL3, "zb_subghz_start_suspend_status_poll: stop/free", (FMT__0));
        zb_buf_free(param);
    }
}


/* *****************************************************************************
 * Cluster entry point for ZB Stack
 * *****************************************************************************/

static zb_bool_t zb_zcl_process_c_subghz_specific_command(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_bool_t res = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_process_c_subghz_specific_command, param %hd", (FMT__H, param));

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_subghz_client_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);


    ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_SUB_GHZ);

    if (ZB_ZCL_FRAME_DIRECTION_TO_CLI == cmd_info.cmd_direction)
    {
        res = zb_subghz_cli_handle_server_commands(param, &cmd_info);
    }

    if (res == ZB_FALSE)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_process_c_subghz_specific_command: processed == %d", (FMT__D, res));
    return res;
}

void zb_zcl_subghz_init_client(void)
{
    ZCL_CTX().subghz_ctx.ep = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_SUB_GHZ, ZB_ZCL_CLUSTER_CLIENT_ROLE);
    TRACE_MSG(TRACE_ZCL1, "zb_subghz_init CLIENT ep %hd", (FMT__H, ZCL_CTX().subghz_ctx.ep));

    (void)zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_SUB_GHZ,
                                      ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                      NULL,
                                      NULL,
                                      zb_zcl_process_c_subghz_specific_command);
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ || defined DOXYGEN */
