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
/*  PURPOSE: Green Power Cluster (both roles)
*/

#define ZB_TRACE_FILE_ID 895

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP

#include "zb_zdo.h"
#include "zb_aps.h"
#include "zgp/zgp_internal.h"

static zb_uint8_t zgp_specific_cluster_cmd_handler(zb_uint8_t param);
static zb_bool_t zgp_handle_read_attr(zb_uint8_t param);

#ifdef ZB_ENABLE_ZGP_PROXY
static void zgp_handle_proxy_table_req(zb_uint8_t param);
static void zgp_proxy_table_entry_over_the_air_transmission(zb_bufid_t      buf,
        zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent);
static zb_uint8_t zgp_proxy_table_entry_size_over_the_air(zgp_tbl_ent_t *ent);
static void zgp_handle_proxy_commissioning_mode_req(zb_uint8_t param);
static void zgp_handle_gp_pairing_req(zb_uint8_t param);
static void zgp_handle_gp_response(zb_uint8_t param);
static void zgp_handle_gp_sink_table_response(zb_uint8_t param);
#endif  /* ZB_ENABLE_ZGP_PROXY */

#ifdef ZB_ENABLE_ZGP_SINK
static void zgp_handle_sink_table_req(zb_uint8_t param);
static void zgp_handle_gp_comm_notification_req(zb_uint8_t param);
static void zgp_handle_gp_notification_req(zb_uint8_t param);
static void zgp_handle_gp_pairing_configuration_req(zb_uint8_t param);
static zb_uint8_t zgp_handle_gp_sink_commissioning_mode(zb_uint8_t param);
static zb_uint8_t zgp_sink_table_entry_size_over_the_air(zgp_tbl_ent_t *ent);
#endif  /* ZB_ENABLE_ZGP_SINK */

#if defined ZB_ENABLE_ZGP_SINK || ZGP_COMMISSIONING_TOOL
static zb_uint16_t zgp_sink_fill_spec_entry_options(zgp_tbl_ent_t *ent);
static void zgp_handle_gp_proxy_table_response(zb_uint8_t param);
static void zgp_sink_table_entry_over_the_air_transmission(zb_bufid_t      buf,
        zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent,
        zb_uint16_t    options);
static zb_uint8_t zgp_sink_get_group_list_size(zgp_tbl_ent_t *ent);
#endif  /* defined ZB_ENABLE_ZGP_SINK || ZGP_COMMISSIONING_TOOL */

#define UNSPECIFIED_PAYLOAD_SIZE 0xff

#ifdef ZGP_CLUSTER_TEST
static zgp_cluster_app_zcl_cmd_handler_t zgp_cluster_app_zcl_cmd_handler = NULL;
#endif  /* ZGP_CLUSTER_TEST */

/**
   Structure for r/o constants reported by cluster
 */
static const ZB_CODE struct zgp_gp_ro_s
{
#ifdef ZB_ENABLE_ZGP_SINK
    zb_uint8_t gps_max_sink_table_entries;
    zb_uint32_t gps_functionality;
    zb_uint32_t gps_active_functionality;
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    zb_uint8_t gpp_max_proxy_table_entries;
    zb_uint32_t gpp_functionality;
    zb_uint32_t gpp_active_functionality;
#endif  /* ZB_ENABLE_ZGP_PROXY */
} s_gp_ro =
{
#ifdef ZB_ENABLE_ZGP_SINK
    ZB_ZGP_SINK_TBL_SIZE,
    ZGP_GPSB_FUNCTIONALITY,
    0xFFFFFF,
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
    ZB_ZGP_PROXY_TBL_SIZE,
    ZGP_GPPB_FUNCTIONALITY,
    0xFFFFFF
#endif  /* ZB_ENABLE_ZGP_PROXY */
};


#ifndef ZB_ENABLE_ZGP_ADVANCED
#ifdef ZB_ENABLE_ZGP_PROXY
#ifndef ZB_ENABLE_ZGP_COMBO
static const ZB_ZCL_DECLARE_GPPB_ATTRIB_LIST_CLI(s_gp_attr_list_cli,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key,
        (void *)&s_gp_ro.gpp_max_proxy_table_entries,
        (void *)&s_gp_ro.gpp_functionality,
        (void *)&s_gp_ro.gpp_active_functionality,
        (void *)&ZGP_CTXC().proxy_table  //Override handling into cluster
                                                );
static const ZB_ZCL_DECLARE_GPPB_ATTRIB_LIST_SRV(s_gp_attr_list_srv,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key
                                                );

ZB_ZCL_DECLARE_GP_CLUSTER_LIST(s_gp_cluster, s_gp_attr_list_srv, s_gp_attr_list_cli);
ZB_ZCL_DECLARE_GPPB_EP(s_gp_ep, ZGP_ENDPOINT, s_gp_cluster);
#endif  /* !ZB_ENABLE_ZGP_COMBO */
#endif  /* ZB_ENABLE_ZGP_PROXY */

#if defined ZB_ENABLE_ZGP_TARGET || defined ZB_ENABLE_ZGP_TARGET_PLUS
static const ZB_ZCL_DECLARE_GPT_ATTRIB_LIST_CLI(s_gp_attr_list_cli,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key
                                               );
static const ZB_ZCL_DECLARE_GPT_ATTRIB_LIST_SRV(s_gp_attr_list_srv,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key,
        (void *)&s_gp_ro.gps_max_sink_table_entries,
        &ZGP_CTXC().cluster.gps_communication_mode,
        &ZGP_CTXC().cluster.gps_commissioning_exit_mode,
        &ZGP_CTXC().cluster.gps_security_level,
        (void *)&s_gp_ro.gps_functionality,
        (void *)&s_gp_ro.gps_active_functionality,
        &ZGP_CTXC().cluster.gps_commissioning_window,
        (void *)&ZGP_CTXC().sink_table  //Override handling into cluster
                                               );

ZB_ZCL_DECLARE_GP_CLUSTER_LIST(s_gp_cluster, s_gp_attr_list_srv, s_gp_attr_list_cli);

#ifdef ZB_ENABLE_ZGP_TARGET
ZB_ZCL_DECLARE_GPT_EP(s_gp_ep, ZGP_ENDPOINT, s_gp_cluster);
#endif  /* ZB_ENABLE_ZGP_TARGET */
#ifdef ZB_ENABLE_ZGP_TARGET_PLUS
ZB_ZCL_DECLARE_GPTP_EP(s_gp_ep, ZGP_ENDPOINT, s_gp_cluster);
#endif  /* ZB_ENABLE_ZGP_TARGET_PLUS */
#endif  /* defined ZB_ENABLE_ZGP_TARGET || defined ZB_ENABLE_ZGP_TARGET_PLUS */
#ifdef ZB_ENABLE_ZGP_COMBO
static const ZB_ZCL_DECLARE_GPCB_ATTRIB_LIST_CLI(s_gp_attr_list_cli,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key,
        (void *)&s_gp_ro.gpp_max_proxy_table_entries,
        (void *)&s_gp_ro.gpp_functionality,
        (void *)&s_gp_ro.gpp_active_functionality,
        (void *)&ZGP_CTXC().proxy_table  //Override handling into cluster
                                                );
static const ZB_ZCL_DECLARE_GPCB_ATTRIB_LIST_SRV(s_gp_attr_list_srv,
        &ZGP_CTXC().cluster.gp_shared_security_key_type,
        &ZGP_CTXC().cluster.gp_shared_security_key,
        &ZGP_CTXC().cluster.gp_link_key,
        (void *)&s_gp_ro.gps_max_sink_table_entries,
        &ZGP_CTXC().cluster.gps_communication_mode,
        &ZGP_CTXC().cluster.gps_commissioning_exit_mode,
        &ZGP_CTXC().cluster.gps_security_level,
        (void *)&s_gp_ro.gps_functionality,
        (void *)&s_gp_ro.gps_active_functionality,
        &ZGP_CTXC().cluster.gps_commissioning_window,
        (void *)&ZGP_CTXC().sink_table  //Override handling into cluster
                                                );


ZB_ZCL_DECLARE_GP_CLUSTER_LIST(s_gp_cluster, s_gp_attr_list_srv, s_gp_attr_list_cli);
ZB_ZCL_DECLARE_GPCB_EP(s_gp_ep, ZGP_ENDPOINT, s_gp_cluster);
#endif  /* ZB_ENABLE_ZGP_COMBO */
#else
#error "Please define GPP and GPC cluster attributes list"
#endif  /* ZB_ENABLE_ZGP_ADVANCED */

/* Internal device context. Declared here to exclude application
 * modification. For now only GP is here. */
static ZB_ZCL_DECLARE_GP_CTX(s_internal_ctx, s_gp_ep);


zb_ret_t zgp_disable(void)
{
    zb_ret_t ret = RET_ERROR;

    if (!ZGP_CTXC().init_by_scheduler)
    {
        ZGP_CTXC().gp_disabled = ZB_TRUE;
    }

    TRACE_MSG(TRACE_ZDO1, "zgp_disable, ret 0x%x", (FMT__D, ret));
    return ret;
}


void zgp_init_by_scheduler(zb_uint8_t param)
{
    zb_af_simple_desc_1_1_t *sd;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, "> zgp_init_by_scheduler", (FMT__0));

    if (ZGP_CTXC().init_by_scheduler
            || ZGP_CTXC().gp_disabled)
    {
        TRACE_MSG(TRACE_ZDO1, "already inited %d, disabled %d, skip the initialization",
                  (FMT__D_D, ZGP_CTXC().init_by_scheduler, ZGP_CTXC().gp_disabled));
    }
    else
    {
        sd = (zb_af_simple_desc_1_1_t *)&simple_desc_s_gp_ep;

#ifndef ZB_ENABLE_ZGP_ADVANCED
#ifdef ZB_ENABLE_ZGP_COMBO
        sd->app_device_id = ZGP_DEVICE_COMBO_BASIC;
#else
        sd->app_device_id = ZGP_DEVICE_PROXY_BASIC;
#endif
#else
#ifdef ZB_ENABLE_ZGP_COMBO
        sd->app_device_id = ZGP_DEVICE_COMBO;
#else
        sd->app_device_id = ZGP_DEVICE_PROXY;
#endif
#endif  /* ZB_ENABLE_ZGP_ADVANCED */
#ifdef ZB_ENABLE_ZGP_TARGET
        sd->app_device_id = ZGP_DEVICE_TARGET;
#endif  /* ZB_ENABLE_ZGP_TARGET */
#ifdef ZB_ENABLE_ZGP_TARGET_PLUS
        sd->app_device_id = ZGP_DEVICE_TARGET_PLUS;
#endif  /* ZB_ENABLE_ZGP_TARGET_PLUS */
#ifdef ZGP_COMMISSIONING_TOOL
        sd->app_device_id = ZGP_DEVICE_COMMISSIONING_TOOL;
#endif  /* ZGP_COMMISSIONING_TOOL */

        TRACE_MSG(TRACE_ZDO3, "sd app_device_id %hd", (FMT__H, sd->app_device_id));

        zb_zcl_register_device_ctx((zb_af_device_ctx_t *)&s_internal_ctx);
        zb_add_simple_descriptor((zb_af_simple_desc_1_1_t *)&simple_desc_s_gp_ep);
        ZB_AF_SET_ENDPOINT_HANDLER(ZGP_ENDPOINT, zgp_specific_cluster_cmd_handler);

        zb_zcl_green_power_init_server();
        zb_zcl_green_power_init_client();

        ZGP_CTXC().init_by_scheduler = 1;
        zb_buf_get_out_delayed(zb_zgp_sync_pib);
    }

    TRACE_MSG(TRACE_ZDO1, "< zgp_init_by_scheduler", (FMT__0));
}

void zgp_gp_set_shared_security_key_type(enum zb_zgp_security_key_type_e type)
{
    ZGP_CTXC().cluster.gp_shared_security_key_type = type;

    switch (type)
    {
    case ZB_ZGP_SEC_KEY_TYPE_NWK:
    {
        ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY,
                  secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number),
                  ZB_CCM_KEY_SIZE);
    }
    break;
    case ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED:
    {
        if (zb_zgp_key_gen(ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED,
                           NULL, 0, ZGP_GP_SHARED_SECURITY_KEY) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "GROUP_NWK_DERIVED key generation failed!", (FMT__0));
        }
    }
    break;
    default:
        break;
    };

    TRACE_MSG(TRACE_ZGP3, "Store gp_shared_security_key: " TRACE_FORMAT_128,
              (FMT__A_A, TRACE_ARG_128(ZGP_CTXC().cluster.gp_shared_security_key)));
}

/** @brief Hook on Write attribute
 * Switch key if key_type was changed remotely. */
void zb_zcl_zgp_cluster_write_attr_hook(zb_uint8_t endpoint, zb_uint16_t attr_id, zb_uint8_t *new_value)
{
    zb_bool_t save_to_nvram = ZB_FALSE;

    ZVUNUSED(save_to_nvram);
    ZVUNUSED(endpoint);

    TRACE_MSG(TRACE_ZGP2, ">> zb_zcl_zgp_cluster_write_attr_hook attr_id %d",
              (FMT__D, attr_id));

    /* EES: currently ZGP cluster handle one by one specific attributes (e.g. ProxyTable)
       otherwise multiple attrs writing handled in ZCL general command handler */
#ifdef ZB_USE_NVRAM
    switch (attr_id)
    {
#ifdef ZB_ENABLE_ZGP_SINK
    case ZB_ZCL_ATTR_GPS_COMMUNICATION_MODE_ID:
    case ZB_ZCL_ATTR_GPS_COMMISSIONING_EXIT_MODE_ID:
    case ZB_ZCL_ATTR_GPS_COMMISSIONING_WINDOW_ID:
    case ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID:
#endif  /* ZB_ENABLE_ZGP_SINK */
    case ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_ID:
    case ZB_ZCL_ATTR_GP_LINK_KEY_ID:
    {
        save_to_nvram = ZB_TRUE;
        break;
    }
    case ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_TYPE_ID:
    {
        save_to_nvram = ZB_TRUE;
        zgp_gp_set_shared_security_key_type((enum zb_zgp_security_key_type_e) *new_value);
        break;
    }
    }

    if (save_to_nvram)
    {
        ZB_SCHEDULE_CALLBACK(zb_zgp_write_dataset, ZB_NVRAM_DATASET_GP_CLUSTER);
    }
#endif /* ZB_USE_NVRAM */

    TRACE_MSG(TRACE_ZGP2, "<< zb_zcl_zgp_cluster_write_attr_hook", (FMT__0));
}

zb_bool_t check_value_green_power(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);
void zb_zcl_zgp_cluster_write_attr_hook(zb_uint8_t endpoint, zb_uint16_t attr_id, zb_uint8_t *new_value);


static zb_bool_t zb_zcl_dummy_cluster_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    return ZB_TRUE;
}

void zb_zcl_green_power_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_GREEN_POWER,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                zb_zcl_zgp_cluster_write_attr_hook,
                                /* Don't need to register ZCL cluster handler because GP Endpoint
                                   zgp_specific_cluster_cmd_handler used instead */
                                zb_zcl_dummy_cluster_handler);
}

void zb_zcl_green_power_init_client()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_GREEN_POWER,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)NULL,
                                zb_zcl_zgp_cluster_write_attr_hook,
                                /* Don't need to register ZCL cluster handler because GP Endpoint
                                   zgp_specific_cluster_cmd_handler used instead */
                                zb_zcl_dummy_cluster_handler);
}

#ifdef ZGP_CLUSTER_TEST
void zgp_cluster_set_app_zcl_cmd_handler(zgp_cluster_app_zcl_cmd_handler_t handler)
{
    zgp_cluster_app_zcl_cmd_handler = handler;
}
#endif  /* ZGP_CLUSTER_TEST */

static zb_uint8_t zgp_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_bool_t processed = ZB_FALSE;
    zb_uint8_t status = ZB_ZCL_STATUS_SUCCESS;

    TRACE_MSG(TRACE_ZGP2, "> zgp_specific_cluster_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZGP2, "payload size: %i", (FMT__D, zb_buf_len(param)));

    if (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_GREEN_POWER)
    {
#ifdef ZGP_CLUSTER_TEST
        if (zgp_cluster_app_zcl_cmd_handler)
        {
            processed = (zb_bool_t)zgp_cluster_app_zcl_cmd_handler(param);
        }

        if (processed == ZB_FALSE)
        {
#endif  /* ZGP_CLUSTER_TEST */
            if (cmd_info->is_common_command)
            {
                switch (cmd_info->cmd_id)
                {
                case ZB_ZCL_CMD_READ_ATTRIB:
                    if (zgp_handle_read_attr(param))
                    {
                        processed = ZB_TRUE;
                    }
                    break;
                }
            } /* if read/write attr */
            else
            {
                if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
                {
#ifdef ZB_ENABLE_ZGP_PROXY
                    /* Proxy side, GP cluster commands */
                    switch (cmd_info->cmd_id)
                    {
                    case ZGP_CLIENT_CMD_GP_PAIRING:
                        zgp_handle_gp_pairing_req(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_CLIENT_CMD_GP_PROXY_COMMISSIONING_MODE:
                        zgp_handle_proxy_commissioning_mode_req(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_CLIENT_CMD_GP_RESPONSE:
                        zgp_handle_gp_response(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_CLIENT_CMD_GP_SINK_TABLE_RESPONSE:
                        zgp_handle_gp_sink_table_response(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_CLIENT_CMD_GP_PROXY_TABLE_REQUEST:
                        /*
                          Upon reception of the GP Proxy Table Request command, the device SHALL check if it implements a Proxy Table.
                          If not, it SHALL generate a ZCL Default Response command, with the Status code field carrying
                          UNSUP_CLUSTER_COMMAND, subject to the rules as specified in sec. 2.4.12 of [3].
                        */
                        zgp_handle_proxy_table_req(param);
                        processed = ZB_TRUE;
                        break;
                    }; /* switch */
#endif  /* ZB_ENABLE_ZGP_PROXY */
                } /* if (to Proxy) */
                else
                {
                    /* To Sink */
                    switch (cmd_info->cmd_id)
                    {
#ifdef ZB_ENABLE_ZGP_SINK
                    case ZGP_SERVER_CMD_GP_NOTIFICATION:
                        zgp_handle_gp_notification_req(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_SERVER_CMD_GP_COMMISSIONING_NOTIFICATION:
                        zgp_handle_gp_comm_notification_req(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_SERVER_CMD_GP_SINK_COMMISSIONING_MODE:
                        status = zgp_handle_gp_sink_commissioning_mode(param);
                        processed = ZB_TRUE;
                        break;
                    case ZGP_SERVER_CMD_GP_PAIRING_CONFIGURATION:
                        zgp_handle_gp_pairing_configuration_req(param);
                        processed = ZB_TRUE;
                        break;
#endif /* ZB_ENABLE_ZGP_SINK */
                    /*
                     * According to the ZGP test specification (GPP Attribute writing and reading test case),  DUT-GPP that does not
                     * implement the Sink Table, responds on Sink Table Request with ZCL Default Response command, with Command
                     * identified field carrying 0x0a and Status code field carrying UNSUP_CLUSTER_COMMAND. So, we have to handle
                     * the request anyway
                    */
                    case ZGP_SERVER_CMD_GP_SINK_TABLE_REQUEST:
#ifdef ZB_ENABLE_ZGP_SINK
                        zgp_handle_sink_table_req(param);
#else
                        {
                            zb_zcl_parsed_hdr_t cmd_info_tmp = *cmd_info;
                            zb_zcl_send_default_resp_ext(param, &cmd_info_tmp, ZB_ZCL_STATUS_UNSUP_CLUST_CMD);
                        }
#endif
                        processed = ZB_TRUE;
                        break;

#if defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL
                    case ZGP_SERVER_CMD_GP_PROXY_TABLE_RESPONSE:
                        zgp_handle_gp_proxy_table_response(param);
                        processed = ZB_TRUE;
                        break;
#endif  /* defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL */
                    }; /* switch */
                } /* else (to Sink) */
            }
#ifdef ZGP_CLUSTER_TEST
        }
#endif  /* ZGP_CLUSTER_TEST */
    } /* if GP cluster */

    if (status != ZB_ZCL_STATUS_SUCCESS)
    {
        ZB_ZCL_PROCESS_COMMAND_FINISH(param, cmd_info, status);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_specific_cluster_cmd_handler", (FMT__0));
    return processed;
}

#ifdef ZB_ENABLE_ZGP_PROXY
static void zgp_handle_read_proxy_table(zb_uint8_t param)
{
    zb_uint8_t          *cmd_ptr;
    zb_uint8_t           bytes_avail;
    zb_zcl_parsed_hdr_t  cmd_info;
    zb_uint8_t           entries_count;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_read_proxy_table param %hd", (FMT__H, param));

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t),
              sizeof(zb_zcl_parsed_hdr_t));

    entries_count = zb_zgp_proxy_table_non_empty_entries_count();

    TRACE_MSG(TRACE_ZGP2, "non-empty enties count: %hd", (FMT__H, entries_count));

    ZB_ZCL_GENERAL_INIT_READ_ATTR_RESP_EXT(
        param,
        cmd_ptr,
        (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI) ?
        ZB_ZCL_FRAME_DIRECTION_TO_SRV : ZB_ZCL_FRAME_DIRECTION_TO_CLI,
        cmd_info.seq_number,
        ZB_FALSE,
        0);
    ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_ptr, ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID);

    /* EES: in most cases current architecture not allowed
       transmit full proxy table */
    bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, cmd_ptr);

    if (bytes_avail < (ZB_ZGP_MIN_PROXY_TABLE_ENTRY_SIZE * entries_count))
    {
        ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_INSUFF_SPACE);
    }
    else
    {
        zb_uint8_t  i;
        zb_uint16_t size = 0;

        for (i = 0; i < entries_count; i++)
        {
            zgp_tbl_ent_t ent;

            if (zb_zgp_proxy_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
            {
                size += zgp_proxy_table_entry_size_over_the_air(&ent);
            }
            else
            {
                TRACE_MSG(TRACE_ZGP2, "Failed to get proxy entry by non empty list index: %hd", (FMT__H, i));
            }
        }

        if (bytes_avail < size)
        {
            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_INSUFF_SPACE);
        }
        else
        {
            zb_uindex_t i;

            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_SUCCESS);
            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING);
            ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_ptr, size);
            for (i = 0; i < entries_count; i++)
            {
                zgp_tbl_ent_t ent;

                if (zb_zgp_proxy_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
                {
                    zgp_proxy_table_entry_over_the_air_transmission(param,
                            &cmd_ptr,
                            &ent);
                }
            }
        }
    }

    ZB_ZCL_GENERAL_SEND_READ_ATTR_RESP(param, cmd_ptr,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                                       ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                                       cmd_info.profile_id,
                                       cmd_info.cluster_id,
                                       NULL);
}
#endif  /* ZB_ENABLE_ZGP_PROXY */

#ifdef ZB_ENABLE_ZGP_SINK
static void zgp_handle_read_sink_table(zb_uint8_t param)
{
    zb_uint8_t          *cmd_ptr;
    zb_uint8_t           bytes_avail;
    zb_zcl_parsed_hdr_t  cmd_info;
    zb_uint8_t           entries_count;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_read_sink_table param %hd", (FMT__H, param));

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t),
              sizeof(zb_zcl_parsed_hdr_t));

    entries_count = zb_zgp_sink_table_non_empty_entries_count();

    TRACE_MSG(TRACE_ZGP2, "non-empty enties count: %hd", (FMT__H, entries_count));

    ZB_ZCL_GENERAL_INIT_READ_ATTR_RESP_EXT(
        param,
        cmd_ptr,
        (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI) ?
        ZB_ZCL_FRAME_DIRECTION_TO_SRV : ZB_ZCL_FRAME_DIRECTION_TO_CLI,
        cmd_info.seq_number,
        ZB_FALSE,
        0);
    ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_ptr, ZB_ZCL_ATTR_GPS_SINK_TABLE_ID);

    /* EES: in most cases current architecture not allowed
       transmit full sink table */
    bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, cmd_ptr);

    if (bytes_avail < (ZB_ZGP_MIN_SINK_TABLE_ENTRY_SIZE * entries_count))
    {
        ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_INSUFF_SPACE);
    }
    else
    {
        zb_uint8_t  i;
        zb_uint16_t size = 0;

        for (i = 0; i < entries_count; i++)
        {
            zgp_tbl_ent_t ent;

            if (zb_zgp_sink_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
            {
                size += zgp_sink_table_entry_size_over_the_air(&ent);
            }
        }

        if (bytes_avail < size)
        {
            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_INSUFF_SPACE);
        }
        else
        {
            zb_uindex_t i;

            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_STATUS_SUCCESS);
            ZB_ZCL_PACKET_PUT_DATA8(cmd_ptr, ZB_ZCL_ATTR_TYPE_LONG_OCTET_STRING);
            ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_ptr, size);
            for (i = 0; i < entries_count; i++)
            {
                zgp_tbl_ent_t ent;

                if (zb_zgp_sink_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
                {
                    zgp_sink_table_entry_over_the_air_transmission(param,
                            &cmd_ptr,
                            &ent,
                            zgp_sink_fill_spec_entry_options(&ent));
                }
            }
        }
    }

    ZB_ZCL_GENERAL_SEND_READ_ATTR_RESP(param, cmd_ptr,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr,
                                       ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint,
                                       ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).dst_endpoint,
                                       cmd_info.profile_id,
                                       cmd_info.cluster_id,
                                       NULL);
}
#endif  /* ZB_ENABLE_ZGP_SINK */

static zb_bool_t zgp_handle_read_attr(zb_uint8_t param)
{
    zb_bool_t  processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_read_attr param %hd", (FMT__H, param));

    /* EES: currently ZGP cluster handle one by one specific attributes (e.g. ProxyTable)
       otherwise multiple attrs reading handled in ZCL general command handler */

    if (zb_buf_len(param) == sizeof(zb_zcl_read_attr_req_t))
    {
        zb_zcl_read_attr_req_t *read_attr_req = (zb_zcl_read_attr_req_t *)zb_buf_begin(param);

        switch (read_attr_req->attr_id[0])
        {
#ifdef ZB_ENABLE_ZGP_PROXY
        case ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID:
        {
            zgp_handle_read_proxy_table(param);
            processed = ZB_TRUE;
            break;
        }
#endif  /* ZB_ENABLE_ZGP_PROXY */
#ifdef ZB_ENABLE_ZGP_SINK
        case ZB_ZCL_ATTR_GPS_SINK_TABLE_ID:
        {
            zgp_handle_read_sink_table(param);
            processed = ZB_TRUE;
            break;
        }
#endif  /* ZB_ENABLE_ZGP_SINK */
        };
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_read_attr processed %hd", (FMT__H, processed));
    return processed;
}

#ifdef ZB_ENABLE_ZGP_PROXY
/**
 * @brief Transmit proxy entry lightweight list
 *
 * @param ptr [in]  Pointer to allocated memory space
 * @param ent [in]  Pointer to proxy table entry
 *
 * @see ZGP spec, A.3.4.2.2.1
 */
static void zgp_proxy_transmit_lwsink_addr_list(zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent)
{
    zb_uint8_t lwsink_addr_list_size;
    zb_uindex_t i;

    lwsink_addr_list_size = zgp_proxy_get_lwsink_addr_list_size(ent);
    ZB_ZCL_PACKET_PUT_DATA8(*ptr, lwsink_addr_list_size);

    for (i = 0; i < ZB_ZGP_MAX_LW_UNICAST_ADDR_PER_GPD; i++)
    {
        zb_address_ieee_ref_t addr_ref = ent->u.proxy.lwsaddr[i].addr_ref;

        if (addr_ref != 0xff)
        {
            zb_ieee_addr_t ieee_addr;
            zb_uint16_t    short_addr;

            zb_address_ieee_by_ref(ieee_addr, addr_ref);
            zb_address_short_by_ref(&short_addr, addr_ref);

            ZB_ZCL_PACKET_PUT_DATA_IEEE(*ptr, &ieee_addr);
            ZB_ZCL_PACKET_PUT_DATA16_VAL(*ptr, short_addr);
        }
    }
}

/**
 * @brief Transmit proxy entry sink group list
 *
 * @param ptr [in]  Pointer to allocated memory space
 * @param ent [in]  Pointer to proxy table entry
 *
 * @see ZGP spec, A.3.4.2.2.2.6
 */
static void zgp_proxy_transmit_sink_group_list(zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent)
{
    zb_uint8_t sink_group_list_size;
    zb_uindex_t i;

    sink_group_list_size = zgp_get_group_list_size(ent->u.proxy.sgrp);
    ZB_ZCL_PACKET_PUT_DATA8(*ptr, sink_group_list_size);

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
        zb_uint16_t group_id = ent->u.proxy.sgrp[i].sink_group;
        zb_uint16_t alias = ent->u.proxy.sgrp[i].alias;

        if (group_id != ZGP_PROXY_GROUP_INVALID_IDX)
        {
            /* 08/07/2016 MINOR: Must transmit all entries (also with derived aliases) [AEV] */
            /*if (alias!=ZGP_PROXY_GROUP_DERIVED_ALIAS)*/
            {
                ZB_ZCL_PACKET_PUT_2DATA16_VAL(*ptr, group_id, alias);
                /*
                        ZB_ZCL_PACKET_PUT_DATA16(*ptr, &group_id);
                        ZB_ZCL_PACKET_PUT_DATA16(*ptr, &alias);
                */
            }
        }
    }
}

/**
 * @brief Actualize proxy entry options with runtime fields
 *
 * @param ent [in,out]  Pointer to proxy table entry
 *
 */
static void zgp_proxy_table_entry_set_options_by_runtime_fields(zgp_tbl_ent_t *ent)
{
    if (ZGP_TBL_RUNTIME_GET_VALID(ent))
    {
        ZGP_TBL_SET_VALID(ent);
    }
    else
    {
        ZGP_TBL_CLR_VALID(ent);
    }

    if (ZGP_TBL_RUNTIME_GET_FIRST_TO_FORWARD(ent))
    {
        ZGP_TBL_SET_FIRST_TO_FORWARD(ent);
    }
    else
    {
        ZGP_TBL_CLR_FIRST_TO_FORWARD(ent);
    }

    if (ZGP_TBL_RUNTIME_GET_HAS_ALL_UNICAST_ROUTES(ent))
    {
        ZGP_TBL_SET_HAS_ALL_UNICAST_ROUTES(ent);
    }
    else
    {
        ZGP_TBL_CLR_HAS_ALL_UNICAST_ROUTES(ent);
    }
}

/**
 * @brief Transmit proxy entry over the air
 *
 * @param buf [in]  Pointer to memory buffer
 * @param ptr [in]  Pointer to allocated memory space
 * @param ent [in]  Pointer to proxy table entry
 *
 * @see ZGP spec, A.3.4.2.2.1
 */
static void zgp_proxy_table_entry_over_the_air_transmission(zb_bufid_t      buf,
        zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent)
{
    zb_uint8_t  bytes_avail;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_table_entry_over_the_air_transmission", (FMT__0));

    zgp_proxy_table_entry_set_options_by_runtime_fields(ent);

    TRACE_MSG(TRACE_ZGP3, "options: 0x%x", (FMT__D, ent->options));

    ZB_ZCL_PACKET_PUT_DATA16_VAL(*ptr, ent->options);

    if (ZGP_TBL_GET_APP_ID(ent) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(*ptr, ent->zgpd_id.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA_IEEE(*ptr, &ent->zgpd_id.ieee_addr);
        ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->endpoint);
    }

    /* A.3.4.2.2.1 GPD Assigned Alias parameter SHALL be included if AssignedAlias = 0b1, it SHALL be
     * omitted otherwise;
     */
    if (ZGP_TBL_PROXY_GET_ASSIGNED_ALIAS(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA16_VAL(*ptr, ent->zgpd_assigned_alias);
    }

    /* A.3.4.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_PROXY_GET_SEC_PRESENT(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->sec_options);
    }

    /* A.3.4.2.2.1 GPD security frame counter parameter SHALL:
     * be present and carry the value of the Security frame counter, if:
     *   SecurityUse = 0b1,
     *   SecurityUse = 0b0 and MAC sequence number capabilities = 0b1;
     * be omitted if SecurityUse = 0b0 and Sequence number capabilities = 0b0.
    */
    if (ZGP_TBL_PROXY_GET_SEC_PRESENT(ent) || ZGP_TBL_GET_SEQ_NUM_CAP(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(*ptr, ent->security_counter);
    }

    /* The security key for the GPD. It MAY be skipped, if common/derivable key is used (as indicated
     * in the Options parameter)
     */
    /* A.3.3.2.2.2.6 If SecurityLevel is 0b00 or if the SecurityKeyType has value 0b011 (GPD group
     * key), 0b001 (NWK key) or 0b111 (derived individual GPD key), the GPDkey parameter MAY be
     * omitted and the key MAY be stored in the gpSharedSecurityKey parameter instead. If
     * SecurityLevel has value other than 0b00 and the SecurityKeyType has value 0b111 (derived
     * individual GPD key), the GPDkey parameter MAY be omitted and the key MAY calculated on the fly,
     * based on the value stored in the gpSharedSecurityKey parameter.
     */
    if (ZGP_TBL_PROXY_GET_SEC_LEVEL(ent) == ZB_ZGP_SEC_LEVEL_NO_SECURITY ||
            (ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_NWK ||
             ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED ||
             ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL))
    {
        TRACE_MSG(TRACE_ZGP2, "skip transmit security key", (FMT__0));
    }
    else
    {
        bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(buf, *ptr);
        ZB_ASSERT(bytes_avail >= ZB_CCM_KEY_SIZE);
        ZB_ZCL_PACKET_PUT_DATA_N(*ptr, ent->zgpd_key, ZB_CCM_KEY_SIZE);
    }

    /* Lightweight sink address list parameter SHALL only be included
       if Lightweight unicast GPS sub-field of the Options parameter is set to 0b1;
       whereby the first octet indicates the number of entries in the list,
       and the entries of the list follow directly as defined in Table 40;
       no additional length/element number indication is included per entry;
       SHALL be omitted completely otherwise (i.e. even the length octet SHALL be omitted);
    */
    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_LW_GPS(ent->options))
    {
        zb_uint8_t addr_list_size = zgp_proxy_get_lwsink_addr_list_size(ent);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (addr_list_size * (sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t)));

        bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(buf, *ptr);


        ZB_ASSERT(bytes_avail >= total_bytes_to_write);
        ZVUNUSED(total_bytes_to_write);

        zgp_proxy_transmit_lwsink_addr_list(ptr, ent);
    }

    /* Sink group list parameter SHALL only be included
       if Commissioned Group GPS sub-field of the Options parameter is set to 0b1;
       whereby the first octet indicates the number of entries in the list,
       and the entries of the list follow directly, formatted as defined in Table 26;
       SHALL be completely omitted otherwise (i.e. even  the length octet SHALL be omitted);
    */
    /* A.3.4.2.2.2.6   Sink group list parameter
    The Sink group list contains the list of sink GroupIDs for this GPD, with the corresponding aliases.
    The entries in the Sink group list parameter SHALL be formatted as specified in Table 26.
    If the Pre-Commissioned Group GPS sub-field of the Options parameter is set, the Sink group list
    SHOULD be present.
    */
    /* [AEV] transmitted only precommissioned (full groupID+alias) table entries */
    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_PRECOMMISSIONED_GROUP_GPS(ent->options))
    {
        /* [AEV]: transmit sink group list */
        zb_uint8_t addr_list_size = zgp_get_group_list_size(ent->u.proxy.sgrp);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (addr_list_size * (sizeof(zb_uint16_t) + sizeof(zb_uint16_t)));

        bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(buf, *ptr);
        ZB_ASSERT(bytes_avail >= total_bytes_to_write);
        ZVUNUSED(total_bytes_to_write);

        zgp_proxy_transmit_sink_group_list(ptr, ent);
    }

    ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->groupcast_radius);

    /* Search Counter SHALL be included if EntryActive=0b0 or EntryValid=0b0 sub-field of the Options
     * parameter is set to 0b0, it SHALL be omitted otherwise.
     */
    /* 15-02014-006-GP_Errata_for_GP Basic_specification_14-0563.docx:
     * Search Counter SHALL be included if EntryActive=0b0 or EntryValid=0b0 sub-field of the Options parameter is set to 0b0,
     * it SHALL be omitted otherwise;
     */
    if (ZGP_TBL_GET_ACTIVE(ent) == 0 || ZGP_TBL_GET_VALID(ent) == 0)
    {
        zb_uint8_t counter;

        counter = ZGP_TBL_GET_SEARCH_COUNTER(ent);
        ZB_ZCL_PACKET_PUT_DATA8(*ptr, counter);
    }

    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_OPT_EXT(ent->options))
    {
        // TODO: implement include extension options
        // TODO: implement transmit full unicast sink address list
    }

    ZVUNUSED(bytes_avail);
    ZVUNUSED(bytes_avail);
    TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_table_entry_over_the_air_transmission", (FMT__0));
}

/**
 * @brief Calculate proxy entry size
 *
 * @param ent [in]  Pointer to proxy table entry
 *
 * @return Proxy entry size
 *
 * @see ZGP spec, A.3.4.2.2.1
 */
static zb_uint8_t zgp_proxy_table_entry_size_over_the_air(zgp_tbl_ent_t *ent)
{
    zb_uint8_t  size = 0;

    zgp_proxy_table_entry_set_options_by_runtime_fields(ent);

    TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_table_entry_size_over_the_air", (FMT__0));

    size += 2;

    if (ZGP_TBL_GET_APP_ID(ent) == ZB_ZGP_APP_ID_0000)
    {
        size += 4;
        TRACE_MSG(TRACE_ZGP3, "include: APP_ID_0000", (FMT__0));
    }
    else
    {
        size += 9;
        TRACE_MSG(TRACE_ZGP3, "include: APP_ID_0010, endpoint", (FMT__0));
    }

    /* A.3.4.2.2.1 GPD Assigned Alias parameter SHALL be included if AssignedAlias = 0b1, it SHALL be
     * omitted otherwise;
     */
    if (ZGP_TBL_PROXY_GET_ASSIGNED_ALIAS(ent))
    {
        size += 2;
        TRACE_MSG(TRACE_ZGP3, "include: assigned_alias", (FMT__0));
    }

    /* A.3.4.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_PROXY_GET_SEC_PRESENT(ent))
    {
        size += 1;
        TRACE_MSG(TRACE_ZGP3, "include: security options", (FMT__0));
    }

    /* A.3.4.2.2.1 GPD security frame counter parameter SHALL:
     * be present and carry the value of the Security frame counter, if:
     *   SecurityUse = 0b1,
     *   SecurityUse = 0b0 and MAC sequence number capabilities = 0b1;
     * be omitted if SecurityUse = 0b0 and Sequence number capabilities = 0b0.
    */
    if (ZGP_TBL_PROXY_GET_SEC_PRESENT(ent) || ZGP_TBL_GET_SEQ_NUM_CAP(ent))
    {
        size += 4;
        TRACE_MSG(TRACE_ZGP3, "include: security counter", (FMT__0));
    }

    /* The security key for the GPD. It MAY be skipped, if common/derivable key is used (as indicated
     * in the Options parameter)
     */
    /* A.3.3.2.2.2.6 If SecurityLevel is 0b00 or if the SecurityKeyType has value 0b011 (GPD group
     * key), 0b001 (NWK key) or 0b111 (derived individual GPD key), the GPDkey parameter MAY be
     * omitted and the key MAY be stored in the gpSharedSecurityKey parameter instead. If
     * SecurityLevel has value other than 0b00 and the SecurityKeyType has value 0b111 (derived
     * individual GPD key), the GPDkey parameter MAY be omitted and the key MAY calculated on the fly,
     * based on the value stored in the gpSharedSecurityKey parameter.
     */
    if (ZGP_TBL_PROXY_GET_SEC_LEVEL(ent) == ZB_ZGP_SEC_LEVEL_NO_SECURITY ||
            (ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_NWK ||
             ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED ||
             ZGP_TBL_PROXY_GET_SEC_KEY_TYPE(ent) == ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL))
    {
        //    TRACE_MSG(TRACE_ZGP2, "skip transmit security key", (FMT__0));
    }
    else
    {
        size += ZB_CCM_KEY_SIZE;
        TRACE_MSG(TRACE_ZGP3, "include: security key", (FMT__0));
    }

    /* Lightweight sink address list parameter SHALL only be included
       if Lightweight unicast GPS sub-field of the Options parameter is set to 0b1;
       whereby the first octet indicates the number of entries in the list,
       and the entries of the list follow directly as defined in Table 40;
       no additional length/element number indication is included per entry;
       SHALL be omitted completely otherwise (i.e. even the length octet SHALL be omitted);
    */
    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_LW_GPS(ent->options))
    {
        zb_uint8_t addr_list_size = zgp_proxy_get_lwsink_addr_list_size(ent);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (addr_list_size * (sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t)));

        size += total_bytes_to_write;
        TRACE_MSG(TRACE_ZGP3, "include: LW table size %hd", (FMT__H, total_bytes_to_write));
    }

    /* Sink group list parameter SHALL only be included
       if Commissioned Group GPS sub-field of the Options parameter is set to 0b1;
       whereby the first octet indicates the number of entries in the list,
       and the entries of the list follow directly, formatted as defined in Table 26;
       SHALL be completely omitted otherwise (i.e. even  the length octet SHALL be omitted);
    */
    /* A.3.4.2.2.2.6   Sink group list parameter
    The Sink group list contains the list of sink GroupIDs for this GPD, with the corresponding aliases.
    The entries in the Sink group list parameter SHALL be formatted as specified in Table 26.
    If the Pre-Commissioned Group GPS sub-field of the Options parameter is set, the Sink group list
    SHOULD be present.
    */
    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_PRECOMMISSIONED_GROUP_GPS(ent->options))
    {
        // [AEV]: transmit sink group list
        zb_uint8_t addr_list_size = zgp_get_group_list_size(ent->u.proxy.sgrp);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (addr_list_size * (sizeof(zb_uint16_t) + sizeof(zb_uint16_t)));

        size += total_bytes_to_write;
        TRACE_MSG(TRACE_ZGP3, "include: PrecomGroupcast table size %hd", (FMT__H, total_bytes_to_write));
    }

    size += 1;
    TRACE_MSG(TRACE_ZGP3, "include: groupcast radius", (FMT__0));

    /* Search Counter SHALL be included if EntryActive=0b0 or EntryValid=0b0 sub-field of the Options
     * parameter is set to 0b0, it SHALL be omitted otherwise.
     */
    if (ZGP_TBL_GET_ACTIVE(ent) == 0 || ZGP_TBL_GET_VALID(ent) == 0)
    {
        size += 1;
        TRACE_MSG(TRACE_ZGP3, "include: search counter", (FMT__0));
    }

    if (ZB_ZGP_PROXY_ENTRY_OPT_GET_OPT_EXT(ent->options))
    {
        /* TODO: implement include extension options
           TODO: implement transmit full unicast sink address list */
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_table_entry_size_over_the_air: %hd", (FMT__H, size));
    return size;
}

/**
 * @brief Perform send proxy table response
 *
 * @param param               [in]  Buffer reference
 * @param status              [in]  Response status
 * @param total_entries_count [in]  Total nonempty entries count in the memory
 * @param start_index         [in]  Start index of the nonempty entries list
 * @param ent                 [in]  Pointer to proxy table entry
 *
 * @see ZGP spec, A.3.4.4.2
 */
static void zgp_proxy_table_response_one_entry_req(zb_uint8_t     param,
        zb_uint8_t     status,
        zb_uint8_t     total_entries_count,
        zb_uint8_t     start_index,
        zgp_tbl_ent_t *ent)
{
    zb_uint8_t          *ptr;
    zb_zcl_parsed_hdr_t  cmd_info;
    zb_uint8_t           entries_count = 1;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_table_response_one_entry_req %hd",
              (FMT__H, param));

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t), sizeof(zb_zcl_parsed_hdr_t));

    ptr = ZB_ZCL_START_PACKET(param);
    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_RESP_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_SRV,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, cmd_info.seq_number, ZGP_SERVER_CMD_GP_PROXY_TABLE_RESPONSE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, status);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, total_entries_count);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, start_index);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, entries_count);

    zgp_proxy_table_entry_over_the_air_transmission(param, &ptr, ent);

    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                cmd_info.addr_data.common_data.source.u.short_addr,
                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, NULL);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_table_response_one_entry_req", (FMT__0));
}

/**
 * @brief Perform send proxy table response
 *
 * @param param               [in]  Buffer reference
 * @param status              [in]  Response status
 * @param total_entries_count [in]  Total nonempty entries count in the memory
 * @param start_index         [in]  Start index of the nonempty entries list
 * @param entries_count       [in]  Entries count included in the response
 *
 * @see ZGP spec, A.3.4.4.2
 */
static void zgp_proxy_table_response_req(zb_uint8_t     param,
        zb_uint8_t     status,
        zb_uint8_t     total_entries_count,
        zb_uint8_t     start_index,
        zb_uint8_t entries_count)
{
    zb_uint8_t          *ptr;
    zb_uint8_t          *entries_count_ptr;
    zb_zcl_parsed_hdr_t  cmd_info;
    zb_uint8_t           i = 0;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_proxy_table_response_req %hd",
              (FMT__H, param));

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t), sizeof(zb_zcl_parsed_hdr_t));

    ptr = ZB_ZCL_START_PACKET(param);
    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_RESP_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_SRV,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, cmd_info.seq_number, ZGP_SERVER_CMD_GP_PROXY_TABLE_RESPONSE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, status);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, total_entries_count);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, start_index);

    entries_count_ptr = ptr;
    ZB_ZCL_PACKET_PUT_DATA8(ptr, entries_count);

    if (entries_count)
    {
        zb_uint16_t                size = 0;
        zb_uint8_t                 bytes_avail;
        zb_zgp_ent_enumerate_ctx_t en_ctx;
        zgp_tbl_ent_t              ent;

        bytes_avail = ZB_ZCL_HI_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, ptr);

        TRACE_MSG(TRACE_ZGP4, "bytes_avail: %hd", (FMT__H, bytes_avail));

        en_ctx.entries_count = total_entries_count;
        en_ctx.idx = start_index;

        while (zgp_proxy_table_enumerate(&en_ctx, &ent) == RET_OK)
        {
            size += zgp_proxy_table_entry_size_over_the_air(&ent);

            if (bytes_avail < size)
            {
                break;
            }

            i++;
            zgp_proxy_table_entry_over_the_air_transmission(param, &ptr, &ent);
        }
    }

    TRACE_MSG(TRACE_ZGP2, "Update entries_count from %hd to %hd", (FMT__H_H, entries_count, i));
    *entries_count_ptr = i;

    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                cmd_info.addr_data.common_data.source.u.short_addr,
                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, NULL);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_proxy_table_response_req", (FMT__0));
}

/**
 * @brief Process proxy table request by ZGPD ID, perform proxy table response
 *
 * @param param    [in]  Buffer reference
 * @param options  [in]  Incoming options
 *
 * @see ZGP spec, A.3.4.3.1
 */
static void zgp_handle_proxy_table_req_by_gpd_id(zb_uint8_t param, zb_uint8_t options)
{
    zb_uint8_t          *ptr = zb_buf_begin(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_zgpd_id_t         zgpd_id;
    zgp_tbl_ent_t        ent;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_proxy_table_req_by_gpd_id %hd",
              (FMT__H, param));

    ptr += sizeof(zb_uint8_t);  //skip options field

    zgpd_id.app_id = ZB_ZGP_GP_PROXY_TBL_REQ_GET_APP_ID(options);

    if (zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32((zb_uint8_t *)&zgpd_id.addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(zgpd_id.addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&zgpd_id.addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&zgpd_id.endpoint, ptr);
    }

    if (zgp_proxy_table_read(&zgpd_id, &ent) == RET_OK)
    {
        /* A.3.4.4.2.1 If the triggering GP Proxy Table Request command contained a GPD ID field, the device SHALL
         * check if it has a Proxy Table entry for this GPD ID (and Endpoint, if ApplicationID =
         * 0b010). If yes, the device SHALL create a GP Proxy Table Response with Status SUCCESS, Total
         * number of non-empty Proxy Table entries carrying the total number of non-empty Proxy Table
         * entries on this device, Start index set to 0xff, Entries count field set to 0x01, and one
         * Proxy Table entry field for the requested GPD ID (and Endpoint, if ApplicationID = 0b010),
         * formatted as specified in sec. A.3.4.2.2.1, present.
         */
        zgp_proxy_table_response_one_entry_req(param,
                                               ZB_ZCL_STATUS_SUCCESS,
                                               zb_zgp_proxy_table_non_empty_entries_count(),
                                               0xFF,  /* Start_index */
                                               &ent);
    }
    else
    {
        if (ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr))
        {
            /* A.3.4.4.2.1 If the triggering GP Proxy Table Request was received in groupcast or broadcast, then the
             * GP Proxy Table Response SHOULD be skipped.
             */
            zb_buf_free(param);
        }
        else
        {
            /* A.3.4.4.2.1 If the entry requested by GPD ID (and Endpoint, if ApplicationID = 0b010) cannot be found,
             * and the triggering GP Proxy Table Request was received in unicast, then the GP Proxy Table
             * Response SHALL be sent with Status NOT_FOUND, Total number of non-empty Proxy Table entries
             * carrying the total number of non-empty Proxy Table entries on this device, Start index
             * carrying 0xFF, Entries count field set to 0x00, and any Proxy Table entry fields absent.
             */
            zgp_proxy_table_response_req(param,
                                         ZB_ZCL_STATUS_NOT_FOUND,
                                         zb_zgp_proxy_table_non_empty_entries_count(),
                                         0xFF,  /* Start_index */
                                         0x00); /* Entries count */
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_proxy_table_req_by_gpd_id", (FMT__0));
}

/**
 * @brief Process proxy table request by index, perform proxy table response
 *
 * @param param    [in]  Buffer reference
 * @param options  [in]  Incoming options
 *
 * @see ZGP spec, A.3.4.3.1
 */
static void zgp_handle_proxy_table_req_by_index(zb_uint8_t param, zb_uint8_t options)
{
    zb_uint8_t    *ptr = zb_buf_begin(param);
    zb_uint8_t     start_index;
    zb_uint8_t     entries_count;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_proxy_table_req_by_index %hd",
              (FMT__H, param));

    ZVUNUSED(options);

    ptr += sizeof(zb_uint8_t);  //skip options field

    ZB_ZCL_PACKET_GET_DATA8(&start_index, ptr);

    entries_count = zb_zgp_proxy_table_non_empty_entries_count();

    /* A.3.4.4.2.1 If the triggering GP Proxy Table Request command contained an Index field, the device SHALL
     * check if it has at least Index+1 non-empty Proxy Table entries.
     */
    if (start_index < entries_count)
    {
        /* A.3.4.4.2.1 If yes, the device SHALL create a GP Proxy Table Response with Status SUCCESS,
         * Total number of non-empty Proxy Table entries carrying the total number of non-empty Proxy
         * Table entries on this de-vice, Start index carrying the Index value from the triggering GP
         * Proxy Table Request, Entries count field set to the number of complete Proxy Table entries,
         * which are included, followed by those Proxy Table entry fields themselves, formatted as
         * specified in sec. A.3.4.2.2.1.
         */
        zgp_proxy_table_response_req(param,
                                     ZB_ZCL_STATUS_SUCCESS,
                                     entries_count,
                                     start_index,
                                     entries_count - start_index);
    }
    else
    {
        /* A.3.4.4.2.1 If not, the device SHALL create a a GP Proxy Table Response with Status
         * NOT_FOUND, Total num-ber of non-empty Proxy Table entries carrying the total number of
         * non-empty Proxy Table entries on this device, Start index carrying the Index value from the
         * triggering GP Proxy Table Request, Entries count field set to 0x00 and any Proxy Table entry
         * fields absent.
         */
        zgp_proxy_table_response_req(param,
                                     ZB_ZCL_STATUS_NOT_FOUND,
                                     entries_count,
                                     start_index,
                                     0x00);  /* Entries count */
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_proxy_table_req_by_index", (FMT__0));
}

/**
 * @brief Process proxy table request, perform proxy table response
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.4.3.1
 */
static void zgp_handle_proxy_table_req(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t          *ptr = zb_buf_begin(param);
    zb_uint8_t           options;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_proxy_table_req %hd",
              (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA8(&options, ptr);

    if (zb_zgp_is_proxy_table_empty())
    {
        if (ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr))
        {
            zb_buf_free(param);
        }
        else
        {
            zb_uint8_t start_index = 0xFF;

            /* A.3.4.4.2.1 If its Proxy Table is empty, and the triggering GP Proxy Table Request was received in
             * unicast, then the GP Proxy Table Response SHALL be sent with Status SUCCESS, Total number
             * of non-empty Proxy Table entries carrying 0x00, Start index carrying 0xFF (in case of
             * request by GPD ID) or the In-dex value from the triggering GP Sink Table Request (in case
             * of request by index), Entries count field set to 0x00, and any Proxy Table entry fields
             * absent.
             */
            /* 15-02014-005-GP_Errata_for_GP Basic_specification_14-0563.docx: If its Proxy Table is
             * empty, and the triggering GP Proxy Table Request was received in unicast, then the GP Proxy
             * Table Response SHALL be sent with Status NOT_FOUND, Total number of non-empty Proxy
             * Table entries carrying 0x00, Start index carrying 0xFF (in case of request by GPD ID) or
             * the Index value from the triggering GP Sink Table Request (in case of request by index),
             * Entries count field set to 0x00, and any Proxy Table entry fields absent.
             */
            if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options) == ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX)
            {
                ZB_ZCL_PACKET_GET_DATA8(&start_index, ptr);
            }

            TRACE_MSG(TRACE_ZGP2, "Proxy table is empty", (FMT__0));
            zgp_proxy_table_response_req(param,
                                         ZB_ZCL_STATUS_NOT_FOUND,
                                         0x00,  /* Number of non-empty Proxy Table entries */
                                         start_index,
                                         0x00);  /* Entries count field */
        }
    }
    else
    {
        switch (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options))
        {
        case ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID:
            zgp_handle_proxy_table_req_by_gpd_id(param, options);
            break;
        case ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX:
            zgp_handle_proxy_table_req_by_index(param, options);
            break;
        default:
            TRACE_MSG(TRACE_ZGP2, "Unsupported request type", (FMT__0));
            {
                zb_zcl_send_default_resp_ext(param,
                                             cmd_info,
                                             ZB_ZCL_STATUS_INVALID_FIELD);
            }
        };
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_proxy_table_req", (FMT__0));
}

/**
 * @brief Check that proxy support requested functionality
 *
 * @param rfb [in]  Requested functionality
 *
 * @return ZB_TRUE if requested functionality is supported, ZB_FALSE otherwise
 *
 * @see ZGP spec, A.3.4.2.7
 */

zb_bool_t zgp_proxy_is_support_functionality(zgp_gpp_functionality_t gpp_f)
{
    TRACE_MSG(TRACE_ZGP3, "zgp_proxy_is_support_functionality: 0x%x - %d", (FMT__D_H, s_gp_ro.gpp_functionality, gpp_f));
    return (zb_bool_t)(s_gp_ro.gpp_functionality & gpp_f);
}

/**
 * @brief Process GP Pairing request
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.5.2
 */
static void zgp_handle_gp_pairing_req(zb_uint8_t param)
{
    zb_uint8_t              *ptr = zb_buf_begin(param);
    zb_zgp_gp_pairing_req_t  req;
    zb_uint24_t              opt24;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_pairing_req %hd", (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA24(&opt24, ptr);
    req.options = 0;
    ZB_MEMCPY((zb_uint8_t *)&req.options, (zb_uint8_t *)&opt24, sizeof(zb_uint24_t));
    ZB_LETOH32_ONPLACE(req.options);

    TRACE_MSG(TRACE_ZGP3, "options 0x%08x", (FMT__L, req.options));

    if (ZB_ZGP_PAIRING_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32((zb_uint8_t *)&req.zgpd_addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(req.zgpd_addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&req.zgpd_addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&req.endpoint, ptr);
    }

    /* The RemoveGPD sub-field of the Options field, if set to 0b1,
       indicates that the GPD identified by the GPD ID is being removed
       from the network. Then, none of the optional fields is present.
    */
    if (!ZB_ZGP_PAIRING_OPT_GET_REMOVE_GPD(req.options))
    {
        if (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req.options) == ZGP_COMMUNICATION_MODE_FULL_UNICAST ||
                ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(req.options) == ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST)
        {
            ZB_ZCL_PACKET_GET_DATA_IEEE(&req.sink_ieee_addr, ptr);
            ZB_ZCL_PACKET_GET_DATA16(&req.sink_nwk_addr, ptr);
            ZB_LETOH32_ONPLACE(req.sink_nwk_addr);
            TRACE_MSG(TRACE_ZGP2, "gp_pairing UNICAST handle: 0x%lx", (FMT__D, req.sink_nwk_addr));
        }
        else
        {
            ZB_ZCL_PACKET_GET_DATA16(&req.sink_group_id, ptr);
            ZB_LETOH16_ONPLACE(req.sink_group_id);
            TRACE_MSG(TRACE_ZGP2, "gp_pairing GROUPCAST handle: 0x%lx", (FMT__D, req.sink_group_id));
        }

        if (ZB_ZGP_PAIRING_OPT_GET_ADD_SINK(req.options))
        {
            ZB_ZCL_PACKET_GET_DATA8(&req.dev_id, ptr);

            /* If the SecurityLevel is 0b00 and the GPD MAC sequence number capabilities sub-field is set
             * to 0b0, the GPDsecurityFrameCounter field SHALL NOT be present, the
             * GPDsecurityFrameCounterPresent sub-field of the Options field SHALL be set to 0b0.
             * The GPDsecurityFrameCounter field SHALL be present  and the GPDsecurityFrameCounterPresent
             * sub-field of the Options field SHALL be set to 0b1 whenever the AddSink sub-field of the
             * Options field is set to 0b1  and one of the following cases applies:
             * - if the SecurityLevel sub-field is set to 0b10 or 0b11 or;
             * - if the SecurityLevel is 0b00 and the GPD MAC sequence number capabilities sub-field is
             * set to 0b1.
             * The GPDsecurityFrameCounter field then carries the current value of the GPD security frame
             * counter field from the Sink Table entry corresponding to the GPD ID.
             */

            if (ZB_ZGP_PAIRING_OPT_GET_FRAME_CNT_PRESENT(req.options))
            {
                ZB_ZCL_PACKET_GET_DATA32((zb_uint8_t *)&req.sec_frame_counter, ptr);
                ZB_LETOH32_ONPLACE(req.sec_frame_counter);
            }

            if (ZB_ZGP_PAIRING_OPT_GET_SEC_KEY_PRESENT(req.options))
            {
                ZB_ZCL_PACKET_GET_DATA_N(req.key, ptr, ZB_CCM_KEY_SIZE);
            }

            if (ZB_ZGP_PAIRING_OPT_GET_ASSIGNED_ALIAS_PRESENT(req.options))
            {
                ZB_ZCL_PACKET_GET_DATA16(&req.assigned_alias, ptr);
                ZB_LETOH16_ONPLACE(req.assigned_alias);
                TRACE_MSG(TRACE_ZGP3, "Assigned alias is presented 0x%04x", (FMT__D, req.assigned_alias));
            }

            if (ZB_ZGP_PAIRING_OPT_GET_FRWD_RADIUS(req.options))
            {
                ZB_ZCL_PACKET_GET_DATA8(&req.frwd_radius, ptr);
            }
        }
    }

    zgp_proxy_handle_gp_pairing_req(param, &req);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_pairing_req", (FMT__0));
}

/**
 * @brief Process proxy commissioning mode request
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.5.3
 */
static void zgp_handle_proxy_commissioning_mode_req(zb_uint8_t param)
{
    zb_uint8_t    *ptr = zb_buf_begin(param);
    zb_uint8_t     options;
    zb_uint16_t    comm_wind = 0;
    zb_uint8_t     channel = 0xff;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_proxy_commissioning_mode param %hd",
              (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA8(&options, ptr);

    if (ZB_ZGP_COMM_MODE_OPT_GET_ON_COMM_WIND_EXP(options))
    {
        ZB_ZCL_PACKET_GET_DATA16(&comm_wind, ptr);
        ZB_LETOH16_ONPLACE(comm_wind);
    }

    /* In the current version of the GP specification, the Channel present
       sub-field SHALL always be set to 0b0 and the Channel field SHALL NOT be present.
    */
    if (ZB_ZGP_COMM_MODE_OPT_GET_CHNL_PRESENT(options))
    {
        ZB_ZCL_PACKET_GET_DATA8(&channel, ptr);
    }
    zgp_proxy_handle_commissioning_mode(param, options, comm_wind, channel);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_proxy_commissioning_mode", (FMT__0));
}

/**
 * @brief Process GP Response command
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.5.4
 */
static void zgp_handle_gp_response(zb_uint8_t param)
{
    zb_uint8_t           *ptr = zb_buf_begin(param);
    zb_zgp_gp_response_t *resp_param;
    zb_zgp_gp_response_t  resp;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_response %hd", (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA8(&resp.options, ptr);
    ZB_ZCL_PACKET_GET_DATA16(&resp.temp_master_addr, ptr);
    ZB_LETOH16_ONPLACE(resp.temp_master_addr);
    ZB_ZCL_PACKET_GET_DATA8(&resp.temp_master_tx_chnl, ptr);

    if (ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32((zb_uint8_t *)&resp.zgpd_addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(resp.zgpd_addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&resp.zgpd_addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&resp.endpoint, ptr);
    }

    ZB_ZCL_PACKET_GET_DATA8(&resp.gpd_cmd_id, ptr);

    ZB_ZCL_PACKET_GET_DATA8(&resp.payload[0], ptr);

    if (resp.payload[0] && resp.payload[0] != UNSPECIFIED_PAYLOAD_SIZE)
    {
        ZB_ASSERT(resp.payload[0] <= MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE);
        ZB_ZCL_PACKET_GET_DATA_N(&resp.payload[1], ptr, resp.payload[0]);
    }

    zb_buf_reuse(param);

    resp_param = ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t);
    ZB_MEMCPY(resp_param, &resp, sizeof(zb_zgp_gp_response_t));

    ZB_SCHEDULE_CALLBACK(zgp_proxy_handle_gp_response, param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_response", (FMT__0));
}

/**
 * @brief Process zcl gp sink table response
 *
 * @param param          [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.5.6
 *
 */
static void zgp_handle_gp_sink_table_response(zb_uint8_t param)
{

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_sink_table_response %hd", (FMT__H, param));

    zb_buf_free(param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_sink_table_response", (FMT__0));
}

void zb_zgp_cluster_gp_comm_notification_req(zb_uint8_t    param,
        zb_uint8_t    use_alias,
        zb_uint16_t   alias_addr,
        zb_uint8_t    alias_seq,
        zb_uint16_t   dst_addr,
        zb_uint8_t    dst_addr_mode,
        zb_uint16_t   options,
        zb_callback_t cb)
{
    zb_gpdf_info_t  gpdf_info;
    zb_uint8_t     *ptr = zb_buf_begin(param);
    zb_uint8_t      gpd_cmd_payload[MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE];
    zb_uint8_t      payload_size;
    zb_uint16_t     gpp_short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    zb_uint8_t      gpp_gpd_link;
    zb_uint32_t     frame_counter;
    zb_uint16_t     options_for_put = options;
    zb_uint16_t     options_for_format = options;
    zb_uint32_t     gpd_id_for_put;

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_cluster_gp_comm_notification_req param %hd",
              (FMT__H, param));

    payload_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZGP3, "payload size: %hd", (FMT__H, payload_size));

    if (payload_size)
    {
        ZB_ASSERT(payload_size <= MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE);
        ZB_MEMCPY(&gpd_cmd_payload, ptr, payload_size);
    }

    ZB_MEMCPY(&gpdf_info, ZB_BUF_GET_PARAM(param, zb_gpdf_info_t), sizeof(zb_gpdf_info_t));

    gpp_gpd_link = zb_zgp_cluster_encode_link_quality(gpdf_info.rssi, gpdf_info.lqi);

    ptr = ZB_ZCL_START_PACKET_REQ(param)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_COMMISSIONING_NOTIFICATION);

    TRACE_MSG(TRACE_ZGP3, "options: 0x%x", (FMT__D, options));

#ifdef ZB_CERTIFICATION_HACKS
    if (ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id)
    {
        TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_app_id %d", (FMT__H, ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id_value));
        options_for_put = (options & 0xfff8) | ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id_value;
        TRACE_MSG(TRACE_ZGP3, "options_for_put: 0x%04x", (FMT__D, options_for_put));
    }
    if (ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id_format)
    {
        TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_app_id_format %d", (FMT__H, ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id_value));
        options_for_format = (options & 0xfff8) | ZGP_CERT_HACKS().gp_proxy_replace_comm_app_id_value;
    }
    if (ZGP_CERT_HACKS().gp_proxy_replace_comm_options)
    {
        TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_options 0x%04x[mask 0x%04x]", (FMT__D_D, ZGP_CERT_HACKS().gp_proxy_replace_comm_options_value, ZGP_CERT_HACKS().gp_proxy_replace_comm_options_mask));
        options_for_put = (options & (~ZGP_CERT_HACKS().gp_proxy_replace_comm_options_mask)) | (ZGP_CERT_HACKS().gp_proxy_replace_comm_options_value & ZGP_CERT_HACKS().gp_proxy_replace_comm_options_mask);
        TRACE_MSG(TRACE_ZGP3, "options_for_put: 0x%04x", (FMT__D, options_for_put));
    }
#endif  /* ZB_CERTIFICATION_HACKS */

    ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, options_for_put);

    /* If the GPD command was received with the Maintenance FrameType, the ApplicationID sub-field of
     * the Options field SHALL be set to 0b000  and the GPD ID SHALL carry the value 0x00000000.
     */
    // When recieve maintenance frame zgpd_id already is empty

    if (ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(options_for_format) == ZB_ZGP_APP_ID_0000)
    {
        gpd_id_for_put = gpdf_info.zgpd_id.addr.src_id;

#ifdef ZB_CERTIFICATION_HACKS
        if (ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_id)
        {
            TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_gpd_id 0x%04x", (FMT__L, ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_id_value));
            gpd_id_for_put = ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_id_value;
        }
#endif  /* ZB_CERTIFICATION_HACKS */
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpd_id_for_put);
    }
    else
    {
#ifdef ZB_CERTIFICATION_HACKS
        if (ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_id)
        {
            TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_gpd_ieee", (FMT__0));
            ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_ieee_value);
        }
        else
#endif  /* ZB_CERTIFICATION_HACKS */
            ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &gpdf_info.zgpd_id.addr.ieee_addr);
#ifdef ZB_CERTIFICATION_HACKS
        if (ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_id)
        {
            TRACE_MSG(TRACE_ZGP3, "gp_proxy_replace_comm_gpd_endpoint, val=0x%hd", (FMT__H, ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_ep_value));
            ZB_ZCL_PACKET_PUT_DATA8(ptr, ZGP_CERT_HACKS().gp_proxy_replace_comm_gpd_ep_value);
        }
        else
#endif  /* ZB_CERTIFICATION_HACKS */
            ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info.zgpd_id.endpoint);
    }

    frame_counter = gpdf_info.sec_frame_counter;
    TRACE_MSG(TRACE_ZGP3, "sec frame counter: %d", (FMT__D, frame_counter));
    ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, frame_counter);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info.zgpd_cmd_id);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, payload_size);
    if (payload_size)
    {
        zb_uint8_t bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, ptr);

        TRACE_MSG(TRACE_ERROR, "payload size %hd bytes_avail %hd", (FMT__H_H, payload_size, bytes_avail));

        /* We can really go out of # of bytes available, but
         * ZB_ZCL_MAX_PAYLOAD_SIZE (48) is too conservative. Really we have 88b there. */
        ZB_ASSERT(bytes_avail >= (payload_size + (ZB_ZGP_COMM_NOTIF_OPT_GET_PROXY_INFO_PRESENT(options) ? 3 : 0) +
                                  (ZB_ZGP_COMM_NOTIF_OPT_GET_MIC_PRESENT(options) ? 4 : 0)));
        ZB_ZCL_PACKET_PUT_DATA_N(ptr, &gpd_cmd_payload, payload_size);
    }

    if (ZB_ZGP_COMM_NOTIF_OPT_GET_PROXY_INFO_PRESENT(options))
    {
        ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, gpp_short_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, gpp_gpd_link);
    }

    if (ZB_ZGP_COMM_NOTIF_OPT_GET_MIC_PRESENT(options))
    {
        zb_uint32_t mic = 0;

        ZB_MEMCPY(&mic, gpdf_info.mic, (ZB_CCM_M > sizeof(zb_uint32_t) ? sizeof(zb_uint32_t) : ZB_CCM_M));
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, mic);
    }


    if (use_alias == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZGP2, "use_alias:alias_addr:0x%hx, alias_seq:%hd", (FMT__D_H, alias_addr, alias_seq));
    }

    ZB_ZCL_FINISH_PACKET_O(param, ptr);
    {
        zb_addr_u a;

        a.addr_short = dst_addr;
        ZB_ZCL_SEND_COMMAND_SHORT_ALIAS(param, &a, dst_addr_mode, ZGP_ENDPOINT,
                                        ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                        ZB_ZCL_CLUSTER_ID_GREEN_POWER, 0,
                                        cb, use_alias, alias_addr, alias_seq);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_cluster_gp_comm_notification_req", (FMT__0));
}

void zb_zgp_cluster_gp_notification_req(zb_uint8_t    param,
                                        zb_uint8_t    use_alias,
                                        zb_uint16_t   alias_addr,
                                        zb_uint8_t    alias_seq,
                                        zb_uint16_t   dst_addr,
                                        zb_uint8_t    dst_addr_mode,
                                        zb_uint16_t   options,
                                        zb_uint8_t    groupcast_radius,
                                        zb_callback_t cb)
{
    zb_gpdf_info_t  gpdf_info;
    zb_uint8_t     *ptr = zb_buf_begin(param);
    zb_uint8_t      gpd_cmd_payload[MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE];
    zb_uint8_t      payload_size;
    zb_uint16_t     gpp_short_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    zb_uint8_t      gpp_gpd_link;
    zb_uint16_t     mod_opts = options;

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_cluster_gp_notification_req param %hd",
              (FMT__H, param));

    payload_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZGP2, "payload_size %hd", (FMT__H, payload_size));

    if (payload_size)
    {
        ZB_ASSERT(payload_size <= MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE);
        ZB_MEMCPY(gpd_cmd_payload, ptr, payload_size);
    }

    ZB_MEMCPY(&gpdf_info, ZB_BUF_GET_PARAM(param, zb_gpdf_info_t), sizeof(zb_gpdf_info_t));

    gpp_gpd_link = zb_zgp_cluster_encode_link_quality(gpdf_info.rssi, gpdf_info.lqi);

    ptr = ZB_ZCL_START_PACKET_REQ(param)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_NOTIFICATION);

#ifdef ZB_CERTIFICATION_HACKS
    if (ZGP_CERT_HACKS().gp_proxy_replace_gp_notif_sec_level)
    {
        mod_opts = (mod_opts & (~0xc0)) | (ZGP_CERT_HACKS().gp_proxy_replace_sec_level << 6);
    }
    if (ZGP_CERT_HACKS().gp_proxy_replace_gp_notif_sec_key_type)
    {
        mod_opts = (mod_opts & (~0x700)) | (ZGP_CERT_HACKS().gp_proxy_replace_sec_key_type << 8);
    }
#endif  /*ZB_CERTIFICATION_HACKS */
    ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, mod_opts);

    if (ZB_ZGP_GP_NOTIF_OPT_GET_APP_ID(options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpdf_info.zgpd_id.addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &gpdf_info.zgpd_id.addr.ieee_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info.zgpd_id.endpoint);
    }
#ifdef ZB_CERTIFICATION_HACKS
    if (ZGP_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, ZGP_CERT_HACKS().gp_proxy_replace_sec_frame_counter);
    }
    else
#endif  /* ZB_CERTIFICATION_HACKS */
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpdf_info.sec_frame_counter);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, gpdf_info.zgpd_cmd_id);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, payload_size);
    if (payload_size)
    {
        zb_uint8_t bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, ptr);

        TRACE_MSG(TRACE_ZGP2, "bytes_avail: %hd", (FMT__H, bytes_avail));
        ZB_ASSERT(bytes_avail >= (payload_size + (ZB_ZGP_GP_NOTIF_OPT_GET_PROXY_INFO_PRESENT(options) ? 3 : 0)));
        ZVUNUSED(bytes_avail);

        ZB_ZCL_PACKET_PUT_DATA_N(ptr, &gpd_cmd_payload, payload_size);
    }

    if (ZB_ZGP_GP_NOTIF_OPT_GET_PROXY_INFO_PRESENT(options))
    {
        ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, gpp_short_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, gpp_gpd_link);
    }

    if (use_alias == ZB_TRUE)
    {
        TRACE_MSG(TRACE_ZGP2, "use_alias:alias_addr:0x%hx, alias_seq:%hd", (FMT__D_H, alias_addr, alias_seq));
    }

    ZB_ZCL_FINISH_PACKET_O(param, ptr);

    {
        zb_addr_u a;

        a.addr_short = dst_addr;
        ZB_ZCL_SEND_COMMAND_SHORT_ALIAS(param, &a, dst_addr_mode, ZGP_ENDPOINT,
                                        ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                        ZB_ZCL_CLUSTER_ID_GREEN_POWER, groupcast_radius,
                                        cb, use_alias, alias_addr, alias_seq);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_cluster_gp_notification_req", (FMT__0));
}
#endif  /*ZB_ENABLE_ZGP_PROXY */

#ifdef ZB_ENABLE_ZGP_SINK
/**
 * @brief Check that sink support requested functionality
 *
 * @param rfb [in]  Requested functionality
 *
 * @return ZB_TRUE if requested functionality is supported, ZB_FALSE otherwise
 *
 * @see ZGP spec, A.3.3.2.7
 */
zb_bool_t zgp_sink_is_support_functionality(zgp_gps_functionality_t gps_f)
{
    return (zb_bool_t)(s_gp_ro.gps_functionality & gps_f);
}

/**
 * @brief Check that sink functionality support requested communication mode
 *
 * @param cm [in]  Requested communication mode
 *
 * @return ZB_TRUE if requested communication mode supported, ZB_FALSE otherwise
 *
 * @see ZGP spec, A.3.3.2.7
 */
zb_bool_t zgp_sink_is_support_communication_mode(zb_uint8_t cm)
{
    cm &= 3;

    switch (cm)
    {
    case ZGP_COMMUNICATION_MODE_FULL_UNICAST:
        return zgp_sink_is_support_functionality(ZGP_GPS_FULL_UNICAST_COMMUNICATION);
    case ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED:
        return zgp_sink_is_support_functionality(ZGP_GPS_DERIVED_GROUPCAST_COMMUNICATION);
    case ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED:
        return zgp_sink_is_support_functionality(ZGP_GPS_PRECOMMISSIONED_GROUPCAST_COMMUNICATION);
    case ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST:
        return zgp_sink_is_support_functionality(ZGP_GPS_LIGHTWEIGHT_UNICAST_COMMUNICATION);
    default:
        ZB_ASSERT(0);
    };
    return ZB_FALSE;
}

/**
 * @brief Calculate sink entry size for over the air transmission
 *
 * @param ent [in]  Pointer to sink table entry
 *
 * @return sink entry size in bytes
 *
 * @see ZGP spec, A.3.3.2.2.1
 */
static zb_uint8_t zgp_sink_table_entry_size_over_the_air(zgp_tbl_ent_t *ent)
{
    zb_uint8_t size = 0;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_table_entry_size_over_the_air", (FMT__0));

    size += 2;

    if (ZGP_TBL_GET_APP_ID(ent) == ZB_ZGP_APP_ID_0000)
    {
        size += 4;
    }
    else
    {
        size += 9;
    }

    size += 1;

    /* A.3.3.2.2.1 The Group list parameter:
       SHALL only be included if Communication mode sub-field of the Options parameter is set to 0b10;
         whereby the first octet indicates the number of entries in the list, and the entries of the
         list follow directly, formatted as specified in Table 26;
       SHALL be completely omitted otherwise (i.e. even  the length field SHALL be omitted);
     */
    if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zb_uint8_t group_list_size = zgp_sink_get_group_list_size(ent);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (group_list_size * (sizeof(zb_uint16_t) + sizeof(zb_uint16_t)));

        size += total_bytes_to_write;
    }

    /* Assigned Alias parameter SHALL be included if the AssignedAlias sub-field of the Options field
     * is set to 0b1, otherwise it SHALL be omitted;
     */
    if (ZGP_TBL_SINK_GET_ASSIGNED_ALIAS(ent))
    {
        size += 1;
    }

    size += 1;

    /* A.3.3.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent))
    {
        size += 1;
    }

    /* A.3.4.2.2.1 GPD security frame counter parameter SHALL:
     * be present and carry the value of the Security frame counter, if:
     *   SecurityUse = 0b1,
     *   SecurityUse = 0b0 and MAC sequence number capabilities = 0b1;
     * be omitted if SecurityUse = 0b0 and Sequence number capabilities = 0b0.
    */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent) || ZGP_TBL_GET_SEQ_NUM_CAP(ent))
    {
        size += 4;
    }

    /* A.3.3.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent))
    {
        size += ZB_CCM_KEY_SIZE;
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_table_entry_size_over_the_air: %hd",
              (FMT__H, size));
    return size;
}

/**
 * @brief Perform send sink table response
 *
 * @param param               [in]  Buffer reference
 * @param status              [in]  Response status
 * @param total_entries_count [in]  Total nonempty entries count in the memory
 * @param start_index         [in]  Start index of the nonempty entries list
 * @param entries_count       [in]  Entries count included in the response
 * @param ent                 [in]  Pointer to sink table entries list
 *
 * @see ZGP spec, A.3.4.4.2
 */
static void zgp_sink_table_response_req(zb_uint8_t     param,
                                        zb_uint8_t     status,
                                        zb_uint8_t     total_entries_count,
                                        zb_uint8_t     start_index,
                                        zb_uint8_t     entries_count,
                                        zgp_tbl_ent_t *entries)
{
    zb_uint8_t          *ptr;
    zb_zcl_parsed_hdr_t  cmd_info;
    zb_uint8_t           i;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_table_response_req %hd",
              (FMT__H, param));

    ZB_MEMCPY(&cmd_info, ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t), sizeof(zb_zcl_parsed_hdr_t));

    ptr = ZB_ZCL_START_PACKET(param);
    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_RESP_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_CLI,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, cmd_info.seq_number, ZGP_CLIENT_CMD_GP_SINK_TABLE_RESPONSE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, status);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, total_entries_count);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, start_index);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, entries_count);

    for (i = 0; i < entries_count; i++)
    {
        zgp_sink_table_entry_over_the_air_transmission(param, &ptr, entries + i,
                zgp_sink_fill_spec_entry_options(entries + i));
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                cmd_info.addr_data.common_data.source.u.short_addr,
                                ZB_APS_ADDR_MODE_16_ENDP_PRESENT, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, NULL);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_table_response_req", (FMT__0));
}

/**
 * @brief Process sink table request by ZGPD ID, perform sink table response
 *
 * @param param    [in]  Buffer reference
 * @param options  [in]  Incoming options
 *
 * @see ZGP spec, A.3.3.4.7
 */
static void zgp_handle_sink_table_req_by_gpd_id(zb_uint8_t param, zb_uint8_t options)
{
    zb_uint8_t          *ptr = zb_buf_begin(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_zgpd_id_t         zgpd_id;
    zgp_tbl_ent_t        ent;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_sink_table_req_by_gpd_id %hd",
              (FMT__H, param));

    ptr += sizeof(zb_uint8_t);  //skip options field

    zgpd_id.app_id = ZB_ZGP_GP_PROXY_TBL_REQ_GET_APP_ID(options);

    if (zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32(&zgpd_id.addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(zgpd_id.addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&zgpd_id.addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&zgpd_id.endpoint, ptr);
    }

    if (zgp_sink_table_read(&zgpd_id, &ent) == RET_OK)
    {
        /* A.3.3.5.6.1 If the triggering GP Sink Table Request command contained a GPD ID field, the
         * device SHALL check if it has a Sink Table entry for this GPD ID (and Endpoint, if
         * ApplicationID = 0b010). If yes, the de-vice SHALL create a GP Sink Table Response with Status
         * SUCCESS, Total number of non-empty Sink Table entries carrying the total number of non-empty
         * Sink Table entries on this device, Start index set to 0xff, Entries count field set to 0x01,
         * and one Sink Table entry field for the requested GPD ID (and Endpoint, if ApplicationID =
         * 0b010), formatted as specified in sec. A.3.3.2.2.1, present.
         */
        zgp_sink_table_response_req(param,
                                    ZB_ZCL_STATUS_SUCCESS,
                                    zb_zgp_sink_table_non_empty_entries_count(),
                                    0xFF,  /* Start_index */
                                    0x01,  /* Entries count */
                                    &ent);
    }
    else
    {
        if (ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr))
        {
            /* A.3.3.5.6.1 If the triggering GP Sink Table Request was received in groupcast or broadcast,
             * then the GP Sink Table Response SHOULD be skipped.
             */
            zb_buf_free(param);
        }
        else
        {
            /* A.3.3.5.6.1 If the entry requested by GPD ID (and Endpoint, if ApplicationID = 0b010)
             * cannot be found, and the triggering GP Sink Table Request was received in unicast, then the
             * GP Sink Table Response SHALL be sent with Status NOT_FOUND, Total number of non-empty Sink
             * Table entries carrying the total number of non-empty Sink Table entries on this device,
             * Start index carrying 0xFF, Entries count field set to 0x00, and any Sink Table entry fields
             * absent.
             */
            zgp_sink_table_response_req(param,
                                        ZB_ZCL_STATUS_NOT_FOUND,
                                        zb_zgp_sink_table_non_empty_entries_count(),
                                        0xFF,  /* Start_index */
                                        0x00,  /* Entries count */
                                        NULL); /* Sink Table entry fields absent */
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_sink_table_req_by_gpd_id", (FMT__0));
}

/**
 * @brief Process sink table request by index, perform sink table response
 *
 * @param param    [in]  Buffer reference
 * @param options  [in]  Incoming options
 *
 * @see ZGP spec, A.3.3.4.7
 */
static void zgp_handle_sink_table_req_by_index(zb_uint8_t param, zb_uint8_t options)
{
    zb_uint8_t    *ptr = zb_buf_begin(param);
    zb_uint8_t     start_index;
    zgp_tbl_ent_t  ent;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_sink_table_req_by_index %hd",
              (FMT__H, param));

    ZVUNUSED(options);

    ptr += sizeof(zb_uint8_t);  //skip options field

    ZB_ZCL_PACKET_GET_DATA8(&start_index, ptr);

    /* A.3.3.5.6.1 If the triggering GP Sink Table Request command contained an Index field, the
     * device SHALL check if it has at least Index+1 non-empty Sink Table entries.
     */
    if (start_index < zb_zgp_sink_table_non_empty_entries_count() &&
            zb_zgp_sink_table_get_entry_by_non_empty_list_index(start_index, &ent) == ZB_TRUE)
    {
        /* A.3.3.5.6.1 If yes, the device SHALL create a GP Sink Table Response with Status SUCCESS,
         * Total number of non-empty Sink Table entries carrying the total number of non-empty Sink
         * Table entries on this device, Start index carrying the Index value from the triggering GP
         * Sink Table Request, Entries count field set to the number of complete non-empty Sink Table
         * entries, which are included in this response, followed by those Sink Table entry fields
         * themselves, formatted as specified in sec. A.3.3.2.2.1.
         */
        zgp_sink_table_response_req(param,
                                    ZB_ZCL_STATUS_SUCCESS,
                                    zb_zgp_sink_table_non_empty_entries_count(),
                                    start_index,
                                    0x01,  /* Entries count */
                                    &ent);
    }
    else
    {
        /* A.3.3.5.6.1 If not, the device SHALL create a a GP Sink Table Response with Status NOT_FOUND,
         * Total number of non-empty Sink Table entries carrying the total number of non-empty Sink
         * Table entries on this de-vice, Start index carrying the Index value from the triggering GP
         * Sink Table Request, Entries count field set to 0x00 and any Sink Table entry fields absent.
         */
        zgp_sink_table_response_req(param,
                                    ZB_ZCL_STATUS_NOT_FOUND,
                                    zb_zgp_sink_table_non_empty_entries_count(),
                                    start_index,
                                    0x00,  /* Entries count */
                                    NULL); /* Sink Table entry fields absent */
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_sink_table_req_by_index", (FMT__0));
}

/**
 * @brief Process sink table request, perform sink table response
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.7
 */
static void zgp_handle_sink_table_req(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t          *ptr = zb_buf_begin(param);
    zb_uint8_t           options;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_sink_table_req %hd",
              (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA8(&options, ptr);

    if (zb_zgp_is_sink_table_empty())
    {
        if (ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr))
        {
            zb_buf_free(param);
        }
        else
        {
            zb_uint8_t start_index = 0xFF;

            /* A.3.3.5.6.1 If its Sink Table is empty, and the triggering GP Sink Table Request was
             * received in unicast, then the GP Sink Table Response SHALL be sent with Status SUCCESS,
             * Total number of non-empty Sink Table entries carrying 0x00, Start index carrying 0xFF (in
             * case of request by GPD ID) or the Index value from the triggering GP Sink Table Request (in
             * case of request by index), Entries count field set to 0x00, and any Sink Table entry fields
             * absent.
             */
            /* 15-02014-005-GP_Errata_for_GP Basic_specification_14-0563.docx: If its Sink Table is empty,
             * and the triggering GP Sink Table Request was received in unicast, then the GP Sink Table
             * Response SHALL be sent with Status NOT_FOUND, Total number of non-empty Sink Table
             * entries carrying 0x00, Start index carrying 0xFF (in case of request by GPD ID) or the
             * Index value from the triggering GP Sink Table Request (in case of request by index),
             * Entries count field set to 0x00, and any Sink Table entry fields absent.
             */

            if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options) == ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX)
            {
                ZB_ZCL_PACKET_GET_DATA8(&start_index, ptr);
            }

            TRACE_MSG(TRACE_ZGP2, "Sink table is empty", (FMT__0));
            zgp_sink_table_response_req(param,
                                        ZB_ZCL_STATUS_NOT_FOUND,
                                        0x00,  /* Number of non-empty Sink Table entries */
                                        start_index,
                                        0x00,  /* Entries count field */
                                        NULL); /* Sink Table entry fields absent */
        }
    }
    else
    {
        switch (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options))
        {
        case ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID:
            zgp_handle_sink_table_req_by_gpd_id(param, options);
            break;
        case ZGP_REQUEST_TABLE_ENTRIES_BY_INDEX:
            zgp_handle_sink_table_req_by_index(param, options);
            break;
        default:
            TRACE_MSG(TRACE_ZGP2, "Unsupported request type", (FMT__0));
            {
                zb_zcl_send_default_resp_ext(param,
                                             cmd_info,
                                             ZB_ZCL_STATUS_INVALID_FIELD);
            }
        };
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_sink_table_req", (FMT__0));
}

/**
 * @brief Process GP Commissioning notification request
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.3
 */
static void zgp_handle_gp_comm_notification_req(zb_uint8_t param)
{
    zb_uint8_t                        *ptr = zb_buf_begin(param);
    zb_zgp_gp_comm_notification_req_t  req;
    zb_zgp_gp_comm_notification_req_t *req_ptr;
    zb_uint8_t app_id;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_comm_notification_req %hd", (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA16(&req.options, ptr);
    ZB_LETOH16_ONPLACE(req.options);

    app_id = ZB_ZGP_COMM_NOTIF_OPT_GET_APP_ID(req.options);

    if (app_id == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32(&req.zgpd_addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(req.zgpd_addr.src_id);
    }
    else if (app_id == ZB_ZGP_APP_ID_0010)
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&req.zgpd_addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&req.endpoint, ptr);
    }
    else
    {
        TRACE_MSG(TRACE_ZGP2, "<< Drop, zgp_handle_gp_comm_notification_req (unknown APP_ID=%d) %hd", (FMT__H_H, app_id, param));
        zb_buf_free(param);
        return;
    }
    ZB_ZCL_PACKET_GET_DATA32(&req.gpd_sec_frame_counter, ptr);
    ZB_LETOH32_ONPLACE(req.gpd_sec_frame_counter);

    ZB_ZCL_PACKET_GET_DATA8(&req.gpd_cmd_id, ptr);

    ZB_ZCL_PACKET_GET_DATA8(&req.payload[0], ptr);
    if (req.payload[0] == UNSPECIFIED_PAYLOAD_SIZE)
    {
        req.payload[0] = 0;
    }

    if (req.payload[0])
    {
        if (req.payload[0] > (
                    (app_id == ZB_ZGP_APP_ID_0000) ?
                    ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0000 : ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0010))
        {
            TRACE_MSG(TRACE_ZGP2, "Too big gpd command payload: %hd, DROP", (FMT__H, req.payload[0]));
            zb_buf_free(param);
            TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_comm_notification_req", (FMT__0));
            return;
        }
        ZB_ASSERT(req.payload[0] < MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE);
        ZB_ZCL_PACKET_GET_DATA_N(&req.payload[1], ptr, req.payload[0]);
    }

    if (ZB_ZGP_COMM_NOTIF_OPT_GET_PROXY_INFO_PRESENT(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA16(&req.proxy_info.short_addr, ptr);
        ZB_LETOH16_ONPLACE(req.proxy_info.short_addr);
        ZB_ZCL_PACKET_GET_DATA8(&req.proxy_info.link, ptr);
    }
    else
    {
        /* TODO: get gpp_short_addr and gpp_gpd_link from NWK header */
    }

    if (ZB_ZGP_COMM_NOTIF_OPT_GET_MIC_PRESENT(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA32(&req.mic, ptr);
        ZB_LETOH32_ONPLACE(req.mic);
    }

    zb_buf_reuse(param);

    req_ptr = ZB_BUF_GET_PARAM(param, zb_zgp_gp_comm_notification_req_t);
    ZB_MEMCPY(req_ptr, &req, sizeof(zb_zgp_gp_comm_notification_req_t));

    ZB_SCHEDULE_CALLBACK(zgp_sink_handle_gp_comm_notification_req, param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_comm_notification_req", (FMT__0));
}

/**
 * @brief Process GP Notification request
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.1
 */
static void zgp_handle_gp_notification_req(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t          *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
    zb_uint8_t                   *ptr = zb_buf_begin(param);
    zb_zgp_gp_notification_req_t  req;
    zb_zgp_gp_notification_req_t *req_ptr;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_notification_req %hd", (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA16(&req.options, ptr);
    ZB_LETOH16_ONPLACE(req.options);

    /* clear reserved bit */
    req.options &= 0x7fff;

    if (!ZB_NWK_IS_ADDRESS_BROADCAST(cmd_info->addr_data.common_data.dst_addr))
    {
        ZB_ZGP_GP_NOTIF_OPT_SET_RECV_AS_UNICAST(req.options);
    }

    if (ZB_ZGP_GP_NOTIF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32(&req.zgpd_addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(req.zgpd_addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&req.zgpd_addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&req.endpoint, ptr);
    }
    ZB_ZCL_PACKET_GET_DATA32(&req.gpd_sec_frame_counter, ptr);
    ZB_LETOH32_ONPLACE(req.gpd_sec_frame_counter);

    ZB_ZCL_PACKET_GET_DATA8(&req.gpd_cmd_id, ptr);

    ZB_ZCL_PACKET_GET_DATA8(&req.payload[0], ptr);
    if (req.payload[0] == UNSPECIFIED_PAYLOAD_SIZE)
    {
        req.payload[0] = 0;
    }

    if (req.payload[0])
    {
        if (req.payload[0] > (
                    (ZB_ZGP_GP_NOTIF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000) ?
                    ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0000 : ZB_ZGP_MAX_GPDF_CMD_PAYLOAD_APP_ID_0010))
        {
            TRACE_MSG(TRACE_ZGP2, "Too big gpd command payload: %hd, DROP", (FMT__H, req.payload[0]));
            zb_buf_free(param);
            TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_notification_req", (FMT__0));
            return;
        }
        ZB_ASSERT(req.payload[0] < MAX_ZGP_CLUSTER_GPDF_PAYLOAD_SIZE);
        ZB_ZCL_PACKET_GET_DATA_N(&req.payload[1], ptr, req.payload[0]);
    }

    if (ZB_ZGP_GP_NOTIF_OPT_GET_PROXY_INFO_PRESENT(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA16(&req.proxy_info.short_addr, ptr);
        ZB_LETOH16_ONPLACE(req.proxy_info.short_addr);
        ZB_ZCL_PACKET_GET_DATA8(&req.proxy_info.link, ptr);
    }

    zb_buf_reuse(param);

    req_ptr = ZB_BUF_GET_PARAM(param, zb_zgp_gp_notification_req_t);
    ZB_MEMCPY(req_ptr, &req, sizeof(zb_zgp_gp_notification_req_t));

    ZB_SCHEDULE_CALLBACK(zgp_sink_handle_gp_notification_req, param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_notification_req", (FMT__0));
}

/**
 * @brief Process GP Pairing Configuration request
 *
 * @param param    [in]  Buffer reference
 *
 * @see ZGP spec, A.3.3.4.6
 */
static void zgp_handle_gp_pairing_configuration_req(zb_uint8_t param)
{
    zb_uint8_t               *ptr = zb_buf_begin(param);
    zb_zgp_gp_pairing_conf_t  req;
    zb_zgp_gp_pairing_conf_t *req_ptr;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_pairing_configuration_req %hd", (FMT__H, param));

    ZB_MEMSET(&req, 0, sizeof(req));

    ZB_ZCL_PACKET_GET_DATA8(&req.actions, ptr);
    TRACE_MSG(TRACE_ZGP3, "actions    : 0x%02x", (FMT__H, req.actions));

    ZB_ZCL_PACKET_GET_DATA16(&req.options, ptr);
    ZB_LETOH16_ONPLACE(req.options);
    TRACE_MSG(TRACE_ZGP3, "options    : 0x%04x", (FMT__D, req.options));

    if (ZB_ZGP_GP_PAIRING_CONF_OPT_GET_APP_ID(req.options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_GET_DATA32(&req.zgpd_addr.src_id, ptr);
        ZB_LETOH32_ONPLACE(req.zgpd_addr.src_id);
        TRACE_MSG(TRACE_ZGP3, "zgpd_src_id: 0x%lx", (FMT__L, req.zgpd_addr.src_id));
    }
    else
    {
        ZB_ZCL_PACKET_GET_DATA_IEEE(&req.zgpd_addr.ieee_addr, ptr);
        ZB_ZCL_PACKET_GET_DATA8(&req.endpoint, ptr);
        TRACE_MSG(TRACE_ZGP3, "endpoint   : %hd", (FMT__H, req.endpoint));
    }

    ZB_ZCL_PACKET_GET_DATA8(&req.device_id, ptr);
    TRACE_MSG(TRACE_ZGP3, "device_id  : %hd", (FMT__H, req.device_id));

    if (ZB_ZGP_GP_PAIRING_CONF_GET_COMMUNICATION_MODE(req.options) ==
            ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zb_uint8_t count;
        zb_uindex_t i;

        TRACE_MSG(TRACE_ZGP3, "GROUPCAST_PRECOMISSIONED", (FMT__0));
        ZB_MEMSET((char *)&req.sgrp, (-1), sizeof(req.sgrp));

        ZB_ZCL_PACKET_GET_DATA8(&count, ptr);
        TRACE_MSG(TRACE_ZGP3, "count      : %hd", (FMT__H, count));

        ZB_ASSERT(count <= ZB_ZGP_MAX_SINK_GROUP_PER_GPD);

        for (i = 0; i < count; i++)
        {
            ZB_ZCL_PACKET_GET_DATA16(&req.sgrp[i].sink_group, ptr);
            ZB_LETOH16_ONPLACE(req.sgrp[i].sink_group);
            ZB_ZCL_PACKET_GET_DATA16(&req.sgrp[i].alias, ptr);
            ZB_LETOH16_ONPLACE(req.sgrp[i].alias);

            TRACE_MSG(TRACE_ZGP3, "sink_group : 0x%x", (FMT__D, req.sgrp[i].sink_group));
            TRACE_MSG(TRACE_ZGP3, "alias      : 0x%x\n", (FMT__D, req.sgrp[i].alias));
        }
        TRACE_MSG(TRACE_ZGP3, "---end groups---", (FMT__0));
    }

    if (ZB_ZGP_GP_PAIRING_CONF_GET_ASSIGNED_ALIAS_PRESENT(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA16(&req.assigned_alias, ptr);
        ZB_LETOH16_ONPLACE(req.assigned_alias);
        TRACE_MSG(TRACE_ZGP3, "assigned_al: 0x%x", (FMT__D, req.assigned_alias));
    }

    ZB_ZCL_PACKET_GET_DATA8(&req.frwd_radius, ptr);
    TRACE_MSG(TRACE_ZGP3, "frwd_radius: %hd", (FMT__H, req.frwd_radius));

    if (ZB_ZGP_GP_PAIRING_CONF_GET_SEC_USE(req.options))
    {
        TRACE_MSG(TRACE_ZGP3, "USE SECURITY", (FMT__0));
        ZB_ZCL_PACKET_GET_DATA8(&req.sec_options, ptr);
        TRACE_MSG(TRACE_ZGP3, "seq_options: 0x%02x", (FMT__H, req.sec_options));

        ZB_ZCL_PACKET_GET_DATA32(&req.sec_frame_counter, ptr);
        ZB_LETOH32_ONPLACE(req.sec_frame_counter);
        TRACE_MSG(TRACE_ZGP3, "frame_count: 0x%08x", (FMT__L, req.sec_frame_counter));
        ZB_ZCL_PACKET_GET_DATA_N(req.key, ptr, ZB_CCM_KEY_SIZE);
    }
    else if (ZB_ZGP_GP_PAIRING_CONF_GET_SEQ_NUM_CAPS(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA32(&req.sec_frame_counter, ptr);
        ZB_LETOH32_ONPLACE(req.sec_frame_counter);
        TRACE_MSG(TRACE_ZGP3, "frame_count: 0x%08x", (FMT__L, req.sec_frame_counter));
    }

    ZB_ZCL_PACKET_GET_DATA8(&req.num_paired_endpoints, ptr);
    TRACE_MSG(TRACE_ZGP3, "num_pair_ep: %hd", (FMT__H, req.num_paired_endpoints));

    if (req.num_paired_endpoints && req.num_paired_endpoints < 0xfd)
    {
        zb_uindex_t i;

        /* 30/06/2016 Minor: < to <= [AEV] start */
        ZB_ASSERT(req.num_paired_endpoints <= ZB_ZGP_MAX_PAIRED_ENDPOINTS);
        /* 30/06/2016 Minor: [AEV] end */

        for (i = 0; i < req.num_paired_endpoints; i++)
        {
            ZB_ZCL_PACKET_GET_DATA8(&req.paired_endpoints[i], ptr);
            TRACE_MSG(TRACE_ZGP3, "paired_ep  : 0x%hx", (FMT__H, req.paired_endpoints[i]));
        }
        TRACE_MSG(TRACE_ZGP3, "---end endpoints---", (FMT__0));
    }

    if (ZB_ZGP_GP_PAIRING_CONF_GET_APP_INFO_PRESENT(req.options))
    {
        ZB_ZCL_PACKET_GET_DATA8(&req.app_info, ptr);
        TRACE_MSG(TRACE_ZGP3, "app_info   : 0x%hx", (FMT__H, req.app_info));

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_MANUF_ID_PRESENT(req.app_info))
        {
            ZB_ZCL_PACKET_GET_DATA16(&req.manuf_id, ptr);
            ZB_LETOH16_ONPLACE(req.manuf_id);
            TRACE_MSG(TRACE_ZGP3, "manuf_id   : 0x%x", (FMT__D, req.manuf_id));
        }

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_MODEL_ID_PRESENT(req.app_info))
        {
            ZB_ZCL_PACKET_GET_DATA16(&req.model_id, ptr);
            ZB_LETOH16_ONPLACE(req.model_id);
            TRACE_MSG(TRACE_ZGP3, "model_id   : 0x%x", (FMT__D, req.model_id));
        }

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_CMDS_PRESENT(req.app_info))
        {
            zb_uindex_t i;

            ZB_ZCL_PACKET_GET_DATA8(&req.num_gpd_cmds, ptr);
            TRACE_MSG(TRACE_ZGP3, "num_gpd_cmd: %hd", (FMT__H, req.num_gpd_cmds));

            ZB_ASSERT(req.num_gpd_cmds > 0);
            ZB_ASSERT(req.num_gpd_cmds <= ZB_ZGP_MAX_PAIRED_CONF_GPD_COMMANDS);

            for (i = 0; i < req.num_gpd_cmds; i++)
            {
                ZB_ZCL_PACKET_GET_DATA8(&req.gpd_cmds[i], ptr);
                TRACE_MSG(TRACE_ZGP3, "gpd_cmds   : 0x%hx", (FMT__H, req.gpd_cmds[i]));
            }
            TRACE_MSG(TRACE_ZGP3, "---end cmds---", (FMT__0));
        }
        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_CLUSTERS_PRESENT(req.app_info))
        {
            zb_uindex_t i, cl_num;

            ZB_ZCL_PACKET_GET_DATA8(&cl_num, ptr);

            req.cl.server_cl_num = (cl_num & 0xf);
            req.cl.client_cl_num = ((cl_num & 0xf0) >> 4);
            TRACE_MSG(TRACE_ZGP3, "num_cluster: server: %hd client: %hd", (FMT__H_H, req.cl.server_cl_num, req.cl.client_cl_num));

            ZB_ASSERT(req.cl.server_cl_num <= ZB_ZGP_MAX_PAIRED_CONF_CLUSTERS);
            ZB_ASSERT(req.cl.client_cl_num <= ZB_ZGP_MAX_PAIRED_CONF_CLUSTERS);

            for (i = 0; i < req.cl.server_cl_num; i++)
            {
                ZB_ZCL_PACKET_GET_DATA16(&req.cl.server_clusters[i], ptr);
                ZB_LETOH16_ONPLACE(req.cl.server_clusters[i]);
                TRACE_MSG(TRACE_ZGP3, "srvclusters: 0x%x", (FMT__D, req.cl.server_clusters[i]));
            }

            for (i = 0; i < req.cl.client_cl_num; i++)
            {
                ZB_ZCL_PACKET_GET_DATA16(&req.cl.client_clusters[i], ptr);
                ZB_LETOH16_ONPLACE(req.cl.client_clusters[i]);
                TRACE_MSG(TRACE_ZGP3, "cl_clusters: 0x%x", (FMT__D, req.cl.client_clusters[i]));
            }
            TRACE_MSG(TRACE_ZGP3, "---end clusters---", (FMT__0));
        }

        /* TODO: get other app info fields A.3.3.4.6 */
    }

    zb_buf_reuse(param);

    req_ptr = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_conf_t);
    ZB_MEMCPY(req_ptr, &req, sizeof(zb_zgp_gp_pairing_conf_t));

    ZB_SCHEDULE_CALLBACK(zgp_sink_handle_gp_pairing_configuration, param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_pairing_configuration_req", (FMT__0));
}

static zb_uint8_t zgp_handle_gp_sink_commissioning_mode(zb_uint8_t param)
{
    zb_uint8_t                 *ptr = zb_buf_begin(param);
    zb_zgp_gp_sink_comm_mode_t  req;
    zb_zgp_gp_sink_comm_mode_t *req_ptr;
    zb_af_endpoint_desc_t      *endpoint_desc;
    zb_uint8_t                  status = ZB_ZCL_STATUS_SUCCESS;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_sink_commissioning_mode %hd", (FMT__H, param));

    ZB_ZCL_PACKET_GET_DATA8(&req.options, ptr);

    ZB_ZCL_PACKET_GET_DATA16(&req.gpm_addr_for_sec, ptr);
    ZB_LETOH16_ONPLACE(req.gpm_addr_for_sec);

    ZB_ZCL_PACKET_GET_DATA16(&req.gpm_addr_for_pair, ptr);
    ZB_LETOH16_ONPLACE(req.gpm_addr_for_pair);

    ZB_ZCL_PACKET_GET_DATA8(&req.sink_endpoint, ptr);

    endpoint_desc = zb_af_get_endpoint_desc(req.sink_endpoint);
    if ((req.sink_endpoint != 0xff) && (endpoint_desc == NULL))
    {
        TRACE_MSG(TRACE_ZGP2, "Invalid endpoint", (FMT__0));
        status = ZB_ZCL_STATUS_NOT_FOUND;
    }

    if (((req.gpm_addr_for_pair) != 0xffff || (req.gpm_addr_for_sec != 0xffff)
            || ZB_ZGP_GP_SINK_COMM_MODE_GET_INVOLVE_GPM_IN_SECURITY(req.options)
            || ZB_ZGP_GP_SINK_COMM_MODE_GET_INVOLVE_GPM_IN_PAIRING(req.options))
            && (status == ZB_ZCL_STATUS_SUCCESS))
    {
        TRACE_MSG(TRACE_ZGP2, "Invalid GPM options", (FMT__0));
        status = ZB_ZCL_STATUS_INVALID_VALUE;
    }

    zb_buf_reuse(param);

    if (status == ZB_ZCL_STATUS_SUCCESS)
    {
        req_ptr = ZB_BUF_GET_PARAM(param, zb_zgp_gp_sink_comm_mode_t);
        ZB_MEMCPY(req_ptr, &req, sizeof(zb_zgp_gp_sink_comm_mode_t));

        ZB_SCHEDULE_CALLBACK(zgp_sink_handle_gp_sink_commissioning_mode, param);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_sink_commissioning_mode", (FMT__0));

    return status;
}

void zb_zgp_cluster_gp_response_send(zb_uint8_t    param,
                                     zb_uint16_t   dst_addr,
                                     zb_uint8_t    dst_addr_mode,
                                     zb_callback_t cb)
{
    zb_zgp_gp_response_t  resp;
    zb_uint8_t           *ptr;

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_cluster_gp_response_send", (FMT__0));

    ZB_MEMCPY(&resp, ZB_BUF_GET_PARAM(param, zb_zgp_gp_response_t),
              sizeof(zb_zgp_gp_response_t));

    ptr = ZB_ZCL_START_PACKET(param);
    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_CLI,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
            ZB_ZCL_DISABLE_DEFAULT_RESPONSE);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_CLIENT_CMD_GP_RESPONSE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, resp.options);
    ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, resp.temp_master_addr);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, resp.temp_master_tx_chnl);

    /* If the GPD command is to be sent with the Maintenance FrameType, the ApplicationID sub-field of
     * the Options field SHALL be set to 0b000  and the GPD ID SHALL carry the value 0x00000000.
     */
    /* The GPD ID field of the GP Response carrying GPD Channel Configuration command SHALL carry the
     * GPD ID 0x00000000, ApplicationID sub-field of the Options field SHALL be set to 0b000)  and the
     * Endpoint field is absent.
     */
    // When recieve maintenance frame zgpd_id already is empty

    if (ZB_ZGP_GP_RESPONSE_OPT_GET_APP_ID(resp.options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, resp.zgpd_addr.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &resp.zgpd_addr.ieee_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, resp.endpoint);
    }

    ZB_ZCL_PACKET_PUT_DATA8(ptr, resp.gpd_cmd_id);
    ZB_ZCL_PACKET_PUT_DATA8(ptr, resp.payload[0]);

    if (resp.payload[0])
    {
        zb_uint8_t bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(param, ptr);

        ZB_ASSERT(bytes_avail >= resp.payload[0]);
        ZVUNUSED(bytes_avail);

        ZB_ZCL_PACKET_PUT_DATA_N(ptr, &resp.payload[1], resp.payload[0]);
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_cluster_gp_response_send", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_SINK */

#if defined ZB_ENABLE_ZGP_SINK || ZGP_COMMISSIONING_TOOL
/*
Bits: 0-2 - AppID
      3-4 - Communication mode
        5 - Sequence number capabilities
        6 - RxOnCapability
        7 - FixedLocation
        8 - AssignedAlias
        9 - SecurityUse
    10-15 - Reserved
*/
/**
 * @brief Fill sink entry options
 *
 * @param ent [in]  Pointer to sink table entry
 *
 * @return Sink table entry options value
 *
 * @see ZGP spec, A.3.3.2.2.2.1
 */
static zb_uint16_t zgp_sink_fill_spec_entry_options(zgp_tbl_ent_t *ent)
{
    zb_uint16_t options = 0;

    options = ent->options & 0x3ff;  // get first 10 bits

    return options;
}

/**
 * @brief Calculate count of nonempty groups in group list
 *
 * @param ent [in]  Pointer to sink table entry
 *
 * @return Count of nonempty groups
*/
static zb_uint8_t zgp_sink_get_group_list_size(zgp_tbl_ent_t *ent)
{
    zb_uint8_t res = 0;
    zb_uint8_t i;

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
        if (ent->u.sink.sgrp[i].sink_group != 0xffff)
        {
            res++;
        }
    }
    return res;
}

/**
 * @brief Transmit sink entry group list
 *
 * @param ptr [in]  Pointer to allocated memory space
 * @param ent [in]  Pointer to sink table entry
 *
 * @see ZGP spec, A.3.3.2.2.1
 */
static void zgp_sink_transmit_group_list(zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent)
{
    zb_uint8_t group_list_size;
    zb_uint8_t i;

    group_list_size = zgp_sink_get_group_list_size(ent);
    ZB_ZCL_PACKET_PUT_DATA8(*ptr, group_list_size);

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
        if (ent->u.sink.sgrp[i].sink_group != 0xffff)
        {
            ZB_ZCL_PACKET_PUT_2DATA16_VAL(*ptr, ent->u.sink.sgrp[i].sink_group, ent->u.sink.sgrp[i].alias);
            /*
            ZB_ZCL_PACKET_PUT_DATA16(*ptr, &ent->u.sink.sgrp[i].sink_group);
            ZB_ZCL_PACKET_PUT_DATA16(*ptr, &ent->u.sink.sgrp[i].alias);
            */
        }
    }
}

/**
 * @brief Transmit sink entry over the air
 *
 * @param buf [in]  Pointer to memory buffer
 * @param ptr [in]  Pointer to allocated memory space
 * @param ent [in]  Pointer to sink table entry
 *
 * @see ZGP spec, A.3.3.2.2.1
 */
static void zgp_sink_table_entry_over_the_air_transmission(zb_bufid_t     buf,
        zb_uint8_t   **ptr,
        zgp_tbl_ent_t *ent,
        zb_uint16_t    options)
{
    zb_uint8_t  bytes_avail;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_sink_table_entry_over_the_air_transmission", (FMT__0));

    ZB_ZCL_PACKET_PUT_DATA16_VAL(*ptr, options);

    if (ZGP_TBL_GET_APP_ID(ent) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(*ptr, ent->zgpd_id.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA_IEEE(*ptr, &ent->zgpd_id.ieee_addr);
        ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->endpoint);
    }

    ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->u.sink.device_id);

    /* A.3.3.2.2.1 The Group list parameter:
       SHALL only be included if Communication mode sub-field of the Options parameter is set to 0b10;
         whereby the first octet indicates the number of entries in the list, and the entries of the
         list follow directly, formatted as specified in Table 26;
       SHALL be completely omitted otherwise (i.e. even  the length field SHALL be omitted);
     */
    if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zb_uint8_t group_list_size = zgp_sink_get_group_list_size(ent);
        zb_uint8_t total_bytes_to_write;

        total_bytes_to_write = sizeof(zb_uint8_t) +
                               (group_list_size * (sizeof(zb_uint16_t) + sizeof(zb_uint16_t)));

        bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(buf, *ptr);
        ZB_ASSERT(bytes_avail >= total_bytes_to_write);
        ZVUNUSED(bytes_avail);
        ZVUNUSED(total_bytes_to_write);
        zgp_sink_transmit_group_list(ptr, ent);
    }

    /* Assigned Alias parameter SHALL be included if the AssignedAlias sub-field of the Options field
     * is set to 0b1, otherwise it SHALL be omitted;
     */
    if (ZGP_TBL_SINK_GET_ASSIGNED_ALIAS(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA16_VAL(*ptr, ent->zgpd_assigned_alias);
    }

    ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->groupcast_radius);

    /* A.3.3.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA8(*ptr, ent->sec_options);
    }

    /* A.3.4.2.2.1 GPD security frame counter parameter SHALL:
     * be present and carry the value of the Security frame counter, if:
     *   SecurityUse = 0b1,
     *   SecurityUse = 0b0 and MAC sequence number capabilities = 0b1;
     * be omitted if SecurityUse = 0b0 and Sequence number capabilities = 0b0.
    */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent) || ZGP_TBL_GET_SEQ_NUM_CAP(ent))
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(*ptr, ent->security_counter);
    }

    /* A.3.3.2.2.1 The parameters Security Options and GPD key SHALL always all be included if the
     * SecurityUse sub-field is set to 0b1 (irrespective of the key type in use); SecurityUse
     * sub-field is set to 0b0, the parameters Security Options, and GPD key SHALL be omitted.
     */
    if (ZGP_TBL_SINK_GET_SEC_PRESENT(ent))
    {
        bytes_avail = ZB_ZCL_HI_WO_IEEE_MAX_PAYLOAD_SIZE - ZB_ZCL_GET_BYTES_WRITTEN(buf, *ptr);
        ZB_ASSERT(bytes_avail >= ZB_CCM_KEY_SIZE);
        ZB_ZCL_PACKET_PUT_DATA_N(*ptr, ent->zgpd_key, ZB_CCM_KEY_SIZE);
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_sink_table_entry_over_the_air_transmission", (FMT__0));
}
#endif  /* defined ZB_ENABLE_ZGP_SINK || ZGP_COMMISSIONING_TOOL */

void zb_zgp_cluster_proxy_commissioning_mode_req(zb_uint8_t    param,
        zb_uint8_t    options,
        zb_uint16_t   comm_wind,
        zb_uint8_t    channel,
        zb_callback_t cb)
{
    zb_uint8_t *ptr;
    zb_addr_u addr;

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_cluster_proxy_commissioning_mode_req param %hd options 0x%hx",
              (FMT__H_H, param, options));
    ptr = ZB_ZCL_START_PACKET(param);
    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_CLI,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
            ZB_ZCL_DISABLE_DEFAULT_RESPONSE);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_CLIENT_CMD_GP_PROXY_COMMISSIONING_MODE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, options);

    if (ZB_ZGP_COMM_MODE_OPT_GET_ON_COMM_WIND_EXP(options))
    {
        ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, comm_wind);
    }

    /* In the current version of the GP specification, the Channel present
       sub-field SHALL always be set to 0b0 and the Channel field SHALL NOT be present.
    */
    if (ZB_ZGP_COMM_MODE_OPT_GET_CHNL_PRESENT(options))
    {
        ZB_ZCL_PACKET_PUT_DATA8(ptr, channel);
    }

    addr.addr_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                addr,
                                ZB_ADDR_16BIT_DEV_OR_BROADCAST, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_cluster_proxy_commissioning_mode_req",
              (FMT__0));
}

void zb_zgp_cluster_gp_pairing_req(zb_uint8_t     param,
                                   zb_uint16_t    dst_addr,
                                   zb_uint8_t     dst_addr_mode,
                                   zb_uint32_t    options,
                                   zb_zgpd_addr_t gpd_id,
                                   zb_uint8_t     endpoint,
                                   zb_uint8_t     dev_id,
                                   zb_uint32_t    sec_frame_counter,
                                   zb_uint8_t    *key,
                                   zb_uint16_t    assigned_alias,
                                   zb_uint8_t     frwd_radius,
                                   zb_uint16_t    group_id,
                                   zb_callback_t  cb)
{
    zb_uint24_t     opt24;
    zb_uint8_t     *ptr = ZB_ZCL_START_PACKET(param);

    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_cluster_gp_pairing_req param %hd",
              (FMT__H, param));

    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL_A(ptr,
            ZB_ZCL_FRAME_DIRECTION_TO_CLI,
            ZB_ZCL_NOT_MANUFACTURER_SPECIFIC,
            ZB_ZCL_DISABLE_DEFAULT_RESPONSE);
    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_CLIENT_CMD_GP_PAIRING);

    ZB_MEMCPY(&opt24, &options, ZB_24BIT_SIZE);
    ZB_ZCL_PACKET_PUT_DATA24(ptr, &opt24);

    if (ZB_ZGP_PAIRING_OPT_GET_APP_ID(options) == ZB_ZGP_APP_ID_0000)
    {
        ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, gpd_id.src_id);
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &gpd_id.ieee_addr);
        ZB_ZCL_PACKET_PUT_DATA8(ptr, endpoint);
    }

    /* The RemoveGPD sub-field of the Options field, if set to 0b1,
       indicates that the GPD identified by the GPD ID is being removed
       from the network. Then, none of the optional fields is present.
    */
    if (!ZB_ZGP_PAIRING_OPT_GET_REMOVE_GPD(options))
    {
        /*  The presence of the addressing fields (SinkIEEEaddress, SinkNWKaddress, and SinkGroupID) is in-
            dicated by the sub-fields RemoveGPD and the Communication mode of the Options field, as shown
            in Table 37 below. Any of the fields can only be present, if the RemoveGPD sub-field is set to 0b0.
            The fields SinkIEEEaddress and SinkNWKaddress are only present if full or lightweight unicast
            communication mode is requested.*/

        if (ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(options) == ZGP_COMMUNICATION_MODE_FULL_UNICAST ||
                ZB_ZGP_PAIRING_OPT_GET_COMMUNICATION_MODE(options) == ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST)
        {
            ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, ZB_PIBCACHE_EXTENDED_ADDRESS());
            ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, ZB_PIBCACHE_NETWORK_ADDRESS());
        }
        else
        {
            ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, group_id);
        }

        if (ZB_ZGP_PAIRING_OPT_GET_ADD_SINK(options))
        {
            ZB_ZCL_PACKET_PUT_DATA8(ptr, dev_id);

            /* If the SecurityLevel is 0b00 and the GPD MAC sequence number capabilities sub-field is set
             * to 0b0, the GPDsecurityFrameCounter field SHALL NOT be present, the
             * GPDsecurityFrameCounterPresent sub-field of the Options field SHALL be set to 0b0.
             * The GPDsecurityFrameCounter field SHALL be present  and the GPDsecurityFrameCounterPresent
             * sub-field of the Options field SHALL be set to 0b1 whenever the AddSink sub-field of the
             * Options field is set to 0b1  and one of the following cases applies:
             * - if the SecurityLevel sub-field is set to 0b10 or 0b11 or;
             * - if the SecurityLevel is 0b00 and the GPD MAC sequence number capabilities sub-field is
             * set to 0b1.
             * The GPDsecurityFrameCounter field then carries the current value of the GPD security frame
             * counter field from the Sink Table entry corresponding to the GPD ID.
             */

            if (ZB_ZGP_PAIRING_OPT_GET_FRAME_CNT_PRESENT(options))
            {
                ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, sec_frame_counter);
            }

            if (ZB_ZGP_PAIRING_OPT_GET_SEC_KEY_PRESENT(options))
            {
                ZB_ZCL_PACKET_PUT_DATA_N(ptr, key, ZB_CCM_KEY_SIZE);
            }

            if (ZB_ZGP_PAIRING_OPT_GET_ASSIGNED_ALIAS_PRESENT(options))
            {
                ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, assigned_alias);
                TRACE_MSG(TRACE_ZGP3, "gpd alias sent: 0x%04x", (FMT__D, assigned_alias));
            }

            if (ZB_ZGP_PAIRING_OPT_GET_FRWD_RADIUS(options))
            {
                ZB_ZCL_PACKET_PUT_DATA8(ptr, frwd_radius);
            }
        }
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(param, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_cluster_gp_pairing_req", (FMT__0));
}

void zgp_cluster_send_gp_sink_table_request(zb_uint8_t    buf_ref,
        zb_uint16_t   dst_addr,
        zb_uint8_t    dst_addr_mode,
        zb_uint8_t    options,
        zb_zgpd_id_t *zgpd_id,
        zb_uint8_t    index,
        zb_callback_t cb)
{
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_cluster_send_gp_sink_table_request %hd", (FMT__H, buf_ref));

    ptr = ZB_ZCL_START_PACKET_REQ(buf_ref)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_SINK_TABLE_REQUEST);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, options);

    if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options) == ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID)
    {
        if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_APP_ID(options) == ZB_ZGP_APP_ID_0000)
        {
            ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, zgpd_id->addr.src_id);
        }
        else
        {
            ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &zgpd_id->addr.ieee_addr);
            ZB_ZCL_PACKET_PUT_DATA8(ptr, zgpd_id->endpoint);
        }
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA8(ptr, index);
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(buf_ref, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_cluster_send_gp_sink_table_request", (FMT__0));
}

void zgp_cluster_send_gp_proxy_table_request(zb_uint8_t    buf_ref,
        zb_uint16_t   dst_addr,
        zb_uint8_t    dst_addr_mode,
        zb_uint8_t    options,
        zb_zgpd_id_t *zgpd_id,
        zb_uint8_t    index,
        zb_callback_t cb)
{
    zb_uint8_t *ptr = ZB_ZCL_START_PACKET(buf_ref);

    TRACE_MSG(TRACE_ZGP2, ">> zgp_cluster_send_gp_proxy_table_request %hd", (FMT__H, buf_ref));

    ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL_A(ptr, ZB_ZCL_FRAME_DIRECTION_TO_CLI,
            ZB_FALSE, ZB_ZCL_DISABLE_DEFAULT_RESPONSE);

    ZB_ZCL_CONSTRUCT_COMMAND_HEADER(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_CLIENT_CMD_GP_PROXY_TABLE_REQUEST);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, options);

    if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_REQ_TYPE(options) == ZGP_REQUEST_TABLE_ENTRIES_BY_GPD_ID)
    {
        if (ZB_ZGP_GP_PROXY_TBL_REQ_GET_APP_ID(options) == ZB_ZGP_APP_ID_0000)
        {
            ZB_ZCL_PACKET_PUT_DATA32_VAL(ptr, zgpd_id->addr.src_id);
        }
        else
        {
            ZB_ZCL_PACKET_PUT_DATA_IEEE(ptr, &zgpd_id->addr.ieee_addr);
            ZB_ZCL_PACKET_PUT_DATA8(ptr, zgpd_id->endpoint);
        }
    }
    else
    {
        ZB_ZCL_PACKET_PUT_DATA8(ptr, index);
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(buf_ref, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_cluster_send_gp_proxy_table_request", (FMT__0));
}

#if defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL
/**
 * @brief Process zcl gp proxy table response
 *
 * @param param          [in]  Buffer reference
 *
 * @see ZGP spec, A.3.4.4.2
 *
 */
static void zgp_handle_gp_proxy_table_response(zb_uint8_t param)
{

    TRACE_MSG(TRACE_ZGP2, ">> zgp_handle_gp_proxy_table_response %hd", (FMT__H, param));

    zb_buf_free(param);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_handle_gp_proxy_table_response", (FMT__0));
}

void zgp_cluster_send_pairing_configuration(zb_uint8_t             buf_ref,
        zb_uint16_t            dst_addr,
        zb_uint8_t             dst_addr_mode,
        zb_uint8_t             actions,
        zb_zgp_sink_tbl_ent_t *ent,
        zb_uint8_t             num_paired_endpoints,
        zb_uint8_t            *paired_endpoints,
        zb_uint8_t             app_info,
        zb_uint16_t            manuf_id,
        zb_uint16_t            model_id,
        zb_uint8_t             num_gpd_commands,
        zb_uint8_t            *gpd_commands,
        zb_zgp_cluster_list_t *cluster_list,
        zb_callback_t          cb)
{
    zb_uint8_t  *ptr;
    zb_uint16_t  options;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_cluster_send_pairing_configuration %hd", (FMT__H, buf_ref));

    ptr = ZB_ZCL_START_PACKET_REQ(buf_ref)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_PAIRING_CONFIGURATION);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, actions);

    options = zgp_sink_fill_spec_entry_options(ent);
    options |= ((app_info ? 1 : 0) << 10);

    zgp_sink_table_entry_over_the_air_transmission(buf_ref, &ptr, ent, options);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, num_paired_endpoints);

    if (num_paired_endpoints && num_paired_endpoints < 0xfd)
    {
        zb_uindex_t i;

        ZB_ASSERT(num_paired_endpoints <= ZB_ZGP_MAX_PAIRED_ENDPOINTS);

        for (i = 0; i < num_paired_endpoints; i++)
        {
            ZB_ZCL_PACKET_PUT_DATA8(ptr, paired_endpoints[i]);
        }
    }

    if (ZB_ZGP_GP_PAIRING_CONF_GET_APP_INFO_PRESENT(options))
    {
        ZB_ZCL_PACKET_PUT_DATA8(ptr, app_info);

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_MANUF_ID_PRESENT(app_info))
        {
            ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, manuf_id);
        }

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_MODEL_ID_PRESENT(app_info))
        {
            ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, model_id);
        }

        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_CMDS_PRESENT(app_info))
        {
            zb_uindex_t i;

            ZB_ASSERT(num_gpd_commands > 0);

            ZB_ZCL_PACKET_PUT_DATA8(ptr, num_gpd_commands);

            for (i = 0; i < num_gpd_commands; i++)
            {
                ZB_ZCL_PACKET_PUT_DATA8(ptr, gpd_commands[i]);
            }
        }
        if (ZB_ZGP_GP_PAIRING_CONF_APP_INFO_GET_CLUSTERS_PRESENT(app_info))
        {
            zb_uindex_t i;

            ZB_ASSERT(cluster_list->server_cl_num <= ZB_ZGP_MAX_PAIRED_CONF_CLUSTERS);
            ZB_ASSERT(cluster_list->client_cl_num <= ZB_ZGP_MAX_PAIRED_CONF_CLUSTERS);

            ZB_ZCL_PACKET_PUT_DATA8(ptr, (((cluster_list->client_cl_num & 0xf) << 4) | (cluster_list->server_cl_num & 0xf)));

            for (i = 0; i < cluster_list->server_cl_num; i++)
            {
                ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, cluster_list->server_clusters[i]);
            }

            for (i = 0; i < cluster_list->client_cl_num; i++)
            {
                ZB_ZCL_PACKET_PUT_DATA16_VAL(ptr, cluster_list->client_clusters[i]);
            }
        }
    }

    ZB_ZCL_FINISH_N_SEND_PACKET(buf_ref, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_cluster_send_pairing_configuration", (FMT__0));
}
#endif  /* defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL */

#ifdef ZGP_COMMISSIONING_TOOL
void zgp_cluster_send_gp_sink_commissioning_mode(zb_uint8_t    buf_ref,
        zb_uint16_t   dst_addr,
        zb_uint8_t    dst_addr_mode,
        zb_uint8_t    options,
        zb_uint8_t    endpoint,
        zb_callback_t cb)
{
    zb_uint8_t  *ptr;
    zb_uint16_t  addr = 0xffff;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_cluster_send_gp_sink_commissioning_mode %hd", (FMT__H, buf_ref));

    ptr = ZB_ZCL_START_PACKET_REQ(buf_ref)
          ZB_ZCL_CONSTRUCT_SPECIFIC_COMMAND_REQ_FRAME_CONTROL(ptr, ZB_ZCL_DISABLE_DEFAULT_RESPONSE)
          ZB_ZCL_CONSTRUCT_COMMAND_HEADER_REQ(ptr, ZB_ZCL_GET_SEQ_NUM(), ZGP_SERVER_CMD_GP_SINK_COMMISSIONING_MODE);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, options);

    /* The GPM address for security field SHALL be set to 0xffff in the current version of the
     * specification. */
    ZB_ZCL_PACKET_PUT_2DATA16_VAL(ptr, addr, addr);

    /* The GPM address for pairing field SHALL be set to 0xffff in the current version of the
     * specification. */
    //  ZB_ZCL_PACKET_PUT_DATA16(ptr, &addr);

    ZB_ZCL_PACKET_PUT_DATA8(ptr, endpoint);

    ZB_ZCL_FINISH_N_SEND_PACKET(buf_ref, ptr,
                                dst_addr, dst_addr_mode, ZGP_ENDPOINT,
                                ZGP_ENDPOINT, ZB_AF_GP_PROFILE_ID,
                                ZB_ZCL_CLUSTER_ID_GREEN_POWER, cb);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_cluster_send_gp_sink_commissioning_mode", (FMT__0));
}
#endif /* ZGP_COMMISSIONING_TOOL */

#endif  /* #ifdef ZB_ENABLE_ZGP */
