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
/* PURPOSE: Functions common to ZGP subsystem
*/

#define ZB_TRACE_FILE_ID 2102

#include "zb_common.h"

#ifdef ZB_ENABLE_ZGP

#include "zb_mac.h"
#include "zb_zdo.h"
#include "zb_aps.h"
#include "zgp/zgp_internal.h"
#ifndef ZB_ZGPD_ROLE
#include "zboss_api_zgp.h"

zb_zgp_ctx_t zb_zgp_ctx;
#ifdef ZB_CERTIFICATION_HACKS
zgp_cert_hacks_t zb_zgp_cert_hacks;
#endif

static void zgp_send_dev_annce_for_alias(zb_uint8_t param, zb_uint16_t addr);

zb_zgp_ctx_t *zb_zgp_ctx_get()
{
    return &zb_zgp_ctx;
}

#ifdef ZB_CERTIFICATION_HACKS
zgp_cert_hacks_t *zb_zgp_cert_hacks_get()
{
    return &zb_zgp_cert_hacks;
}
#endif


void zb_zgp_write_dataset(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZGP1, "schedule nvram save for %hd dataset", (FMT__H, param));
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset((zb_nvram_dataset_types_t)param);
}

#if defined ZB_ENABLE_ZGP_INFRA
void zb_set_zgp_device_role(zb_uint16_t device_role)
{
    ZGP_CTXC().device_role = device_role;
}
#endif

void zb_zgp_init()
{
    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_init", (FMT__0));

    ZB_BZERO(&ZGP_CTXC(), sizeof(zb_zgp_ctx_t));

    ZB_MEMCPY(ZGP_GP_LINK_KEY, ZB_ZGP_DEFAULT_LINK_KEY, sizeof(ZGP_GP_LINK_KEY));
    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE (ZB_ZGP_DEFAULT_SHARED_SECURITY_KEY_TYPE);

#ifdef ZB_ENABLE_ZGP_SINK
    ZGP_GPS_COMMUNICATION_MODE = ZB_ZGP_DEFAULT_COMMUNICATION_MODE;
    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZB_ZGP_DEFAULT_COMMISSIONING_EXIT_MODE;
    ZGP_GPS_COMMISSIONING_WINDOW = ZB_ZGP_DEFAULT_COMMISSIONING_WINDOW;
    ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                                 ZB_ZGP_DEFAULT_SECURITY_LEVEL,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                                 ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);
    ZB_ZGP_SET_PROXY_COMM_MODE_COMMUNICATION(ZB_ZGP_PROXY_COMMISSIONING_DEFAULT_COMMUNICATION_MODE);

#endif  /* ZB_ENABLE_ZGP_SINK */

    zgp_tbl_init();

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_init", (FMT__0));
}

void zb_zgp_set_shared_security_key_type(zb_uint_t key_type)
{
    ZGP_GP_SHARED_SECURITY_KEY_TYPE = key_type;
}

void zb_zgp_set_shared_security_key(zb_uint8_t *key)
{
    ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, key, ZB_CCM_KEY_SIZE);
}

void zgp_set_link_key(zb_uint8_t *key)
{
    ZB_MEMCPY(ZGP_GP_LINK_KEY, key, ZB_CCM_KEY_SIZE);
}

zb_uint16_t zgp_calc_alias_source_address(zb_zgpd_id_t *zgpd_id)
{
    zb_uint32_t addr;
    zb_uint16_t lsb12, lsb34;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_calc_alias_source_address", (FMT__0));

    /* src_id is already converted to the host endian. So addr is in the host endian always. */
    if (zgpd_id->app_id == ZB_ZGP_APP_ID_0000)
    {
        addr = zgpd_id->addr.src_id;
    }
    else
    {
        //    addr = (*((zb_uint32_t *)&zgpd_id->addr.ieee_addr));
        ZB_LETOH32(&addr, zgpd_id->addr.ieee_addr);
    }

    TRACE_MSG(TRACE_ZGP3, "addr: 0x%x", (FMT__D, addr));

    lsb12 = addr & 0xffff;
    lsb34 = (addr >> 16) & 0xffff;
    /*
    lsb12 = *(zb_uint16_t *)&addr;
    lsb34 = *((zb_uint16_t *)&addr + 1);
    */

    TRACE_MSG(TRACE_ZGP3, "lsb12: 0x%x, lsb34: 0x%x", (FMT__D_D, lsb12, lsb34));

    if ((lsb12 == 0) || (lsb12 > 0xfff7))
    {
        zb_uint16_t xor;

        xor = lsb12 ^ lsb34;

        TRACE_MSG(TRACE_ZGP3, "xor: 0x%x", (FMT__D, xor));

        if (xor == 0 || xor > 0xfff7)
        {
            if (lsb12 == 0)
            {
                lsb12 = 0x0007;
            }

            if (lsb12 > 0xfff7)
            {
                lsb12 = lsb12 - 0x0008;
            }
        }
        else
        {
            lsb12 = xor;
        }
    }
    TRACE_MSG(TRACE_ZGP2, "<< zgp_calc_alias_source_address 0x%hx", (FMT__D, lsb12));
    return lsb12;
}

#if defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL
static zb_uint16_t zgp_get_sink_entry_alias(zb_zgp_gp_pairing_send_req_t *req)
{
    zb_uint16_t alias = 0;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_get_sink_entry_alias", (FMT__0));

    if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED ||
            ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        if (ZGP_PAIRING_SEND_GET_ASSIGNED_ALIAS(req))
        {
            alias = req->zgpd_assigned_alias;
            TRACE_MSG(TRACE_ZGP3, "assigned alias", (FMT__0));
        }
        else
        {
            if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED)
            {
                zb_zgpd_id_t zgpd_id;

                ZB_MAKE_ZGPD_ID(zgpd_id,
                                ZGP_PAIRING_SEND_GET_APP_ID(req),
                                req->endpoint,
                                req->zgpd_id);
                alias = zgp_calc_alias_source_address(&zgpd_id);
                TRACE_MSG(TRACE_ZGP3, "derived alias", (FMT__0));
            }
            else
            {
                zb_uindex_t i;

                for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
                {
                    if (req->sgrp[i].sink_group != ZGP_PROXY_GROUP_INVALID_IDX &&
                            req->sgrp[i].alias != ZGP_PROXY_GROUP_DERIVED_ALIAS)
                    {
                        alias = req->sgrp[i].alias;
                        TRACE_MSG(TRACE_ZGP2, "precom alias", (FMT__0));
                        break;
                    }
                }
            }
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_get_sink_entry_alias 0x%x", (FMT__D, alias));
    return alias;
}

static zb_uint16_t zgp_get_sink_entry_group(zb_zgp_gp_pairing_send_req_t *req)
{
    zb_uint16_t group = 0;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_get_sink_entry_group", (FMT__0));

    if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED)
    {
        zb_zgpd_id_t zgpd_id;

        ZB_MAKE_ZGPD_ID(zgpd_id,
                        ZGP_PAIRING_SEND_GET_APP_ID(req),
                        req->endpoint,
                        req->zgpd_id);
        group = zgp_calc_alias_source_address(&zgpd_id);
        TRACE_MSG(TRACE_ZGP3, "derived group", (FMT__0));
    }
    else if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zb_uindex_t i;

        for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
        {
            if (req->sgrp[i].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
            {
                group = req->sgrp[i].sink_group;
                req->sgrp[i].sink_group = ZGP_PROXY_GROUP_INVALID_IDX;
                break;
            }
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_get_sink_entry_group 0x%x", (FMT__D, group));
    return group;
}

static void zgp_cluster_send_gp_pairing_internal(zb_uint8_t param, zb_callback_t cb)
{
    zb_zgp_gp_pairing_send_req_t *req = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_send_req_t);
    zb_uint8_t                    sec_lvl = ZGP_PAIRING_SEND_GET_SEC_LEVEL(req);
    zb_uint16_t                   alias;
    zb_uint16_t                   palias;
    zb_uint16_t                   group;
    zb_uint8_t                    frame_cnt_prsnt = 0;
    zb_zgp_gp_pairing_send_req_t request;

    ZB_ASSERT(!(ZGP_PAIRING_SEND_GET_ADD_SINK(req) && ZGP_PAIRING_SEND_GET_REMOVE_GPD(req)));

    alias = zgp_get_sink_entry_alias(req);
    group = zgp_get_sink_entry_group(req);

    palias = alias;

    if (alias == ZGP_PROXY_GROUP_DERIVED_ALIAS)
    {
        zb_zgpd_id_t zgpd_id;

        ZB_MAKE_ZGPD_ID(zgpd_id,
                        ZGP_PAIRING_SEND_GET_APP_ID(req),
                        req->endpoint,
                        req->zgpd_id);
        alias = zgp_calc_alias_source_address(&zgpd_id);
    }

    if (sec_lvl != ZB_ZGP_SEC_LEVEL_NO_SECURITY || ZGP_PAIRING_SEND_GET_SEQ_NUM_CAP(req))
    {
        frame_cnt_prsnt = 1;
    }

#ifdef ZB_CERTIFICATION_HACKS
    if (ZGP_CERT_HACKS().gp_sink_replace_sec_lvl_on_pairing)
    {
        sec_lvl = ZGP_CERT_HACKS().gp_sink_sec_lvl_on_pairing;
    }
#endif
    {
        ZB_MEMCPY(&request, req, sizeof(zb_zgp_gp_pairing_send_req_t));
        req = &request;

        zb_zgp_cluster_gp_pairing_req(param,
                                      ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE,
                                      ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                      ZB_ZGP_FILL_GP_PAIRING_OPTIONS(
                                          ZGP_PAIRING_SEND_GET_APP_ID(req),
                                          ZGP_PAIRING_SEND_GET_ADD_SINK(req),
                                          ZGP_PAIRING_SEND_GET_REMOVE_GPD(req),
                                          ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req),
                                          ZGP_PAIRING_SEND_GET_FIXED_LOCATION(req),
                                          ZGP_PAIRING_SEND_GET_SEQ_NUM_CAP(req),
                                          sec_lvl,
                                          ZGP_PAIRING_SEND_GET_SEC_KEY_TYPE(req),
                                          frame_cnt_prsnt,
                                          ZGP_PAIRING_SEND_GET_SEC_KEY_TYPE(req) != ZB_ZGP_SEC_KEY_TYPE_NO_KEY,
                                          (palias ? 1 : 0),
                                          (req->groupcast_radius ? 1 : 0)
                                      ),
                                      req->zgpd_id,
                                      req->endpoint,
                                      req->device_id,
                                      req->security_counter,
                                      &req->zgpd_key[0],
                                      palias,
                                      req->groupcast_radius,
                                      group,
                                      cb);
    }

    if (ZGP_PAIRING_SEND_GET_SEND_DEV_ANNCE(req) && alias)
    {
        zb_buf_get_out_delayed_ext(zgp_send_dev_annce_for_alias, alias, 0);
    }
}

static void zgp_cluster_send_gp_pairing_dup(zb_uint8_t param, zb_uint16_t user_param)
{
    zb_zgp_gp_pairing_send_req_t *req = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_send_req_t);

    TRACE_MSG(TRACE_ZGP3, ">> zgp_cluster_send_gp_pairing_dup %hd %hd", (FMT__H_H, param, user_param));

    zb_buf_copy(param, user_param);
    zgp_cluster_send_gp_pairing_internal(param, NULL);

    if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zgp_get_sink_entry_group(req);  /* mark sink_group as ZGP_PROXY_GROUP_INVALID_IDX */

        if (!zgp_get_group_list_size(req->sgrp))
        {
            ZB_ZGP_GP_PAIRING_OPTIONS_SET_DUP_COMPLETE(req);
        }
    }
    else
    {
        ZB_ZGP_GP_PAIRING_OPTIONS_SET_DUP_COMPLETE(req);
    }
    zgp_cluster_send_gp_pairing((zb_uint8_t)user_param);

    TRACE_MSG(TRACE_ZGP3, "<< zgp_cluster_send_gp_pairing_dup", (FMT__0));
}

void zgp_cluster_send_gp_pairing(zb_uint8_t param)
{
    zb_zgp_gp_pairing_send_req_t *req = ZB_BUF_GET_PARAM(param, zb_zgp_gp_pairing_send_req_t);

    TRACE_MSG(TRACE_ZGP2, "Broadcasting GP Pairing: param %hd, AddSink %hd, RemoveGPD %hd",
              (FMT__H_H_H, param, ZGP_PAIRING_SEND_GET_ADD_SINK(req), ZGP_PAIRING_SEND_GET_REMOVE_GPD(req)));

    /* EES: if need send incorrect LW unicast pairing remove, see A.3.5.2.4 */
    if (ZGP_PAIRING_SEND_GET_SEND_INCORRECT_LW_PAIR_REMOVE(req))
    {
        if (!ZGP_PAIRING_SEND_GET_DUP_COMPLETE(req))
        {
            zb_buf_get_out_delayed_ext(zgp_cluster_send_gp_pairing_dup, param, 0);
        }
        else
        {
            ZB_ZGP_GP_PAIRING_OPTIONS_UPDATE_SEND_INCORRECT_LW_PAIR_REMOVE(req);
            zgp_cluster_send_gp_pairing_internal(param, req->callback);
        }
    }
    else if (ZGP_PAIRING_SEND_GET_COMMUNICATION_MODE(req) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED &&
             zgp_get_group_list_size(req->sgrp) > 1)
    {
        zb_buf_get_out_delayed_ext(zgp_cluster_send_gp_pairing_dup, param, 0);
    }
    else
    {
        zgp_cluster_send_gp_pairing_internal(param, req->callback);
    }
}
#endif  /* defined ZB_ENABLE_ZGP_SINK || defined ZGP_COMMISSIONING_TOOL */

zb_uint8_t zgp_get_group_list_size(zgp_pair_group_list_t *group_list)
{
    zb_uint8_t ret = 0;
    zb_uint8_t i;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_get_group_list_size", (FMT__0));

    for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
    {
        //search that table already have this group-alias pair
        if (group_list[i].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
        {
            TRACE_MSG(TRACE_ZGP2, "group: 0x%x", (FMT__D, group_list[i].sink_group));
            ret++;
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_get_group_list_size %hd", (FMT__H, ret));
    return ret;
}

static void zgp_send_dev_annce_for_alias(zb_uint8_t param, zb_uint16_t addr)
{
    /* A.3.6.3.4.2 The ZigBee Device_annce command SHALL always be sent using  the Alias source address as NWK
     * source address, a fixed NWK sequence number of 0x00, and a fixed APS counter of 0x00.
     * The payload of the ZigBee Device_annce command SHALL carry the following information the same for
     * ApplicationID 0b000 and 0b010: the NWKAddr field SHALL carry the alias for the GPD, either the
     * calculated Alias NWK source address (see sec. A.3.6.3.3) or the AssignedAlias; the IEEEAddr field
     * SHALL carry the 0xffffffffffffffff value indicating invalid IEEE address [3], and the Capability
     * field with the values as indicated in Figure 78.
     */
    zb_zdo_device_annce_t da;

    da.tsn = 0;
    ZB_HTOLE16((zb_uint8_t *)&da.nwk_addr, &addr);
    ZB_64BIT_ADDR_UNKNOWN(da.ieee_addr);
    da.capability = 0;
    /* Security capability Inherited from the GPP */
    da.capability |= (ZB_ZDO_NODE_DESC()->mac_capability_flags & 0x40);  // get security cap
    zdo_send_device_annce_ex(param, &da, ZB_TRUE);
}

#ifdef ZB_ENABLE_ZGP_DIRECT
/**
 * Operation is asynchronous, execution will continue with calling zb_gp_data_cfm
 * with status ZB_ZGP_STATUS_ENTRY_REMOVED and handle provided by handle parameter.
 */
void zgp_clean_zgpd_info_from_queue(zb_uint8_t    buf_ref,
                                    zb_zgpd_id_t *zgpd_id,
                                    zb_uint8_t    handle)
{
    zb_gp_data_req_t *req;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_clean_zgpd_info_from_queue buf_ref %hd, zgpd_id %p, handle %hd",
              (FMT__H_P_H, buf_ref, zgpd_id, handle));

    req = zb_buf_initial_alloc(buf_ref, sizeof(zb_gp_data_req_t) - sizeof(req->pld));
    req->handle = handle;
    req->action = ZB_GP_DATA_REQ_ACTION_REMOVE_GPDF;
    ZB_MEMCPY(&req->zgpd_id, zgpd_id, sizeof(zb_zgpd_id_t));
    ZB_SCHEDULE_CALLBACK(zb_gp_data_req, buf_ref);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_clean_zgpd_info_from_queue", (FMT__0));
}

/* @brief Issue GP-DATA.request
 *
 * It's the place to make general processing of GP-DATA.requests
 * before passing them to the next layer.
 *
 * @param param  Buffer reference with request
 */
void schedule_gp_txdata_req(zb_uint8_t param)
{
    zb_gp_data_req_t *req = (zb_gp_data_req_t *)zb_buf_begin(param);
    ZVUNUSED(req);

    TRACE_MSG(TRACE_ZGP3, ">> schedule_gp_txdata_req param %hd", (FMT__H, param));

    /* EES: looks like its ugly lifehack for GP SP.
     * We'll need to setup ZB_GP_DATA_REQ_USE_MAC_ACK_BIT at upper layer special in SP logic */
    /* Always set MAC Ack if possible */
    /*
    if (ZB_GP_DATA_REQ_FRAME_TYPE(req->tx_options) == ZGP_FRAME_TYPE_DATA
        && req->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
    {
      req->tx_options |= ZB_GP_DATA_REQ_USE_MAC_ACK_BIT;
    }
    */

    ZB_SCHEDULE_CALLBACK(zb_gp_data_req, param);

    TRACE_MSG(TRACE_ZGP3, "<< schedule_gp_txdata_req", (FMT__0));
}

void zgp_channel_config_add_to_queue(zb_uint8_t param, zb_uint8_t payload)
{
    zb_gp_data_req_t *req;

    TRACE_MSG(TRACE_ZGP2, ">> zgp_channel_config_add_to_queue %hd", (FMT__H, param));
    req = zb_buf_initial_alloc(param, sizeof(zb_gp_data_req_t) - sizeof(req->pld) + 1);

    ZB_BZERO(req, sizeof(zb_gp_data_req_t) - sizeof(req->pld) + 1);

    req->handle = ZB_ZGP_HANDLE_ADD_CHANNEL_CONFIG;
    req->action = ZB_GP_DATA_REQ_ACTION_ADD_GPDF;
    req->tx_options = ZB_GP_DATA_REQ_USE_GP_TX_QUEUE;
    req->cmd_id = ZB_GPDF_CMD_CHANNEL_CONFIGURATION;
    req->payload_len = 1;
    req->pld[0] = payload;

    if (!ZB_ZGPD_IS_SPECIFIED(&ZGP_CTXC().comm_data.zgpd_id))
    {
        req->tx_options |= ZB_GP_DATA_REQ_MAINT_FRAME_TYPE;
    }
    else
    {
        ZB_MEMCPY(&req->zgpd_id, &ZGP_CTXC().comm_data.zgpd_id, sizeof(zb_zgpd_id_t));
    }

    req->tx_q_ent_lifetime = ZB_ZGP_CHANNEL_REQ_ON_TX_CH_TIMEOUT;

    schedule_gp_txdata_req(param);

    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_ADDED_TO_Q);

    TRACE_MSG(TRACE_ZGP2, "<< zgp_channel_config_add_to_queue", (FMT__0));
}

static void zgp_mlme_set_cfm_cb(zb_uint8_t param)
{
    zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZGP2, ">> zgp_mlme_set_cfm_cb %hd", (FMT__H, param));

    ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

    if (cfm->status == MAC_SUCCESS)
    {
        /* switch work on temp channel flag */
        if (ZGP_CTXC().comm_data.state == ZGP_COMM_STATE_CHANNEL_CONFIG_SET_TEMP_CHANNEL)
        {
            ZGP_CTXC().comm_data.is_work_on_temp_channel = ZB_TRUE;
        }
        else
        {
            ZGP_CTXC().comm_data.is_work_on_temp_channel = ZB_FALSE;
        }
    }

#ifdef ZB_ENABLE_ZGP_SINK
    if ((param != 0U) && ZGP_IS_SINK_IN_COMMISSIONING_MODE())
    {
        zb_gp_sink_mlme_set_cfm_cb(param);
    }
    else
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
        if ((param != 0U) && ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
        {
            zb_gp_proxy_mlme_set_cfm_cb(param);
        }
        else
#endif  /* ZB_ENABLE_ZGP_PROXY */
            if (param != 0U)
            {
                TRACE_MSG(TRACE_ZGP2, "Back to COMM_STATE_IDLE", (FMT__0));
                ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
                zb_buf_free(param);
            }
    TRACE_MSG(TRACE_ZGP2, "<< zgp_mlme_set_cfm_cb", (FMT__0));
}

void zgp_channel_config_transceiver_channel_change(zb_uint8_t param,
        zb_bool_t  back)
{
    TRACE_MSG(TRACE_ZGP2, ">> zgp_channel_config_transceiver_channel_change %hd",
              (FMT__H, param));

    /* First set channel, then add entry to the queue */
    if (ZGP_CTXC().comm_data.oper_channel != ZGP_CTXC().comm_data.temp_master_tx_chnl)
    {
        zb_uint8_t *from_channel = &ZGP_CTXC().comm_data.oper_channel;
        zb_uint8_t *to_channel = &ZGP_CTXC().comm_data.temp_master_tx_chnl;

        /* Transciever change channel */
        if (back)
        {
            from_channel = &ZGP_CTXC().comm_data.temp_master_tx_chnl;
            to_channel = &ZGP_CTXC().comm_data.oper_channel;
        }
        else
        {
            ZB_ASSERT(!ZGP_CTXC().comm_data.is_work_on_temp_channel);
        }
        TRACE_MSG(TRACE_ZGP1, "Set channel from %hd to %hd",
                  (FMT__H_H, *from_channel, *to_channel));
        zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL,
                       (void *)to_channel, sizeof(zb_uint8_t),
                       zgp_mlme_set_cfm_cb);
    }
    else
    {

        zb_buf_reuse(param);
        {
            zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

            cfm->status = MAC_SUCCESS;
            cfm->pib_attr = ZB_PHY_PIB_CURRENT_CHANNEL;
            zgp_mlme_set_cfm_cb(param);
        }
    }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_channel_config_transceiver_channel_change", (FMT__0));
}

void zgp_back_to_oper_channel(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZGP3, ">> zgp_back_to_oper_channel tx_channel %hd, oper_channel %hd",
              (FMT__H_H, ZGP_CTXC().comm_data.temp_master_tx_chnl,
               ZGP_CTXC().comm_data.oper_channel));

    ZB_ASSERT(param);

    zgp_channel_config_transceiver_channel_change(param, ZB_TRUE);

    TRACE_MSG(TRACE_ZGP3, "<< zgp_back_to_oper_channel", (FMT__0));
}

static void zgp_mlme_get_cfm_cb(zb_uint8_t param)
{
    zb_mlme_get_confirm_t *cfm = (zb_mlme_get_confirm_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_ZGP2, ">> zgp_mlme_get_cfm_cb %hd", (FMT__H, param));

    ZB_ASSERT(cfm->pib_attr == ZB_PHY_PIB_CURRENT_CHANNEL);

    if (cfm->status == MAC_SUCCESS)
    {
        ZGP_CTXC().comm_data.oper_channel = *(zb_uint8_t *)(cfm + 1);
        TRACE_MSG(TRACE_ZGP2, "Operational channel: %hd", (FMT__H, ZGP_CTXC().comm_data.oper_channel));
    }

#ifdef ZB_ENABLE_ZGP_SINK
    if ((param != 0U) && ZGP_IS_SINK_IN_COMMISSIONING_MODE())
    {
        zb_gp_sink_mlme_get_cfm_cb(param);
    }
    else
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
        if ((param != 0U) && ZGP_IS_PROXY_IN_COMMISSIONING_MODE())
        {
            zb_gp_proxy_mlme_get_cfm_cb(param);
        }
        else
#endif  /* ZB_ENABLE_ZGP_PROXY */
            if (param != 0U)
            {
                TRACE_MSG(TRACE_ZGP2, "Back to COMM_STATE_IDLE", (FMT__0));
                ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_IDLE);
                zb_buf_free(param);
            }

    TRACE_MSG(TRACE_ZGP2, "<< zgp_mlme_get_cfm_cb", (FMT__0));
}

void zb_zgp_channel_config_get_current_channel(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZGP2, ">> zb_zgp_channel_config_get_current_channel %hd",
              (FMT__H, param));
    zb_buf_reuse(param);

    ZB_ZGP_SET_COMM_STATE(ZGP_COMM_STATE_CHANNEL_CONFIG_GET_CUR_CHANNEL);

    /* Get current channel */
    zb_nwk_pib_get(param, ZB_PHY_PIB_CURRENT_CHANNEL, zgp_mlme_get_cfm_cb);

    TRACE_MSG(TRACE_ZGP2, "<< zb_zgp_channel_config_get_current_channel", (FMT__0));
}
#endif  /* ZB_ENABLE_ZGP_DIRECT */

#ifdef ZB_ENABLE_ZGP_SINK
/**
 * @brief Clear TempMaster commissioning context
 *
 */
void zb_zgps_clear_temp_master_list_ctx(void)
{
    zb_uindex_t i;

    TRACE_MSG(TRACE_ZGP1, ">> zb_zgps_clear_temp_master_list_ctx", (FMT__0));

    ZGP_CTXC().comm_data.selected_temp_master_idx = ZB_ZGP_UNSEL_TEMP_MASTER_IDX;

    for (i = 0; i < ZB_ZGP_MAX_TEMP_MASTER_COUNT; i++)
    {
        ZGP_CTXC().comm_data.temp_master_list[i].short_addr = ZB_ZGP_TEMP_MASTER_EMPTY_ENTRY;
        ZGP_CTXC().comm_data.temp_master_list[i].link = 0;
    }

    TRACE_MSG(TRACE_ZGP1, "<< zb_zgps_clear_temp_master_list_ctx", (FMT__0));
}

void zb_zgps_unbind_aps_group_for_aliasing(zb_zgp_sink_tbl_ent_t *ent)
{
    if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED ||
            ZGP_TBL_SINK_GET_COMMUNICATION_MODE(ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
    {
        zb_uint8_t i;

        for (i = 0; i < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; i++)
        {
            if (ent->u.sink.sgrp[i].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
            {
                TRACE_MSG(TRACE_ZGP2, "unbind sink group[%d]: 0x%04x", (FMT__H_D, i, ent->u.sink.sgrp[i].sink_group));
                zb_apsme_remove_group_internal(ent->u.sink.sgrp[i].sink_group, ZGP_ENDPOINT);
            }
        }
    }
}
#endif  /* ZB_ENABLE_ZGP_SINK */

void zgp_cluster_send_proxy_commissioning_mode_enter_req(zb_uint8_t    param,
        zb_uint8_t    exit_mode,
        zb_uint16_t   comm_window,
        zb_callback_t cb)
{
    TRACE_MSG(TRACE_ZGP2, "Broadcasting GP Proxy Commissioning Mode: Enter", (FMT__0));

    /* A.3.3.5.3 In the current version of the GP specification, the Channel present sub-field SHALL always be set
     * to 0b0 and the Channel field SHALL NOT be present.
     */
#define ZGP_PROXY_COMM_MODE_CHANNEL_PRESENT 0

    zb_zgp_cluster_proxy_commissioning_mode_req(param,
            ZB_ZGP_FILL_PROXY_COMM_MODE_OPTIONS(
                ZGP_PROXY_COMM_MODE_ENTER,
                exit_mode,
                ZGP_PROXY_COMM_MODE_CHANNEL_PRESENT,
                ZGP_PROXY_COMM_MODE_IS_UNICAST()),
            comm_window,
            /* Channel field SHALL NOT be present */
            0,
            cb);
}

void zgp_cluster_send_proxy_commissioning_mode_leave_req(zb_uint8_t param, zb_callback_t cb)
{
    TRACE_MSG(TRACE_ZGP2, "Broadcasting GP Proxy Commissioning Mode: Leave", (FMT__0));

    /* A.3.3.5.3 In the current version of the GP specification, the Channel present sub-field SHALL always be set
     * to 0b0 and the Channel field SHALL NOT be present.
     */
#define ZGP_PROXY_COMM_MODE_CHANNEL_PRESENT 0

    zb_zgp_cluster_proxy_commissioning_mode_req(param,
            ZB_ZGP_FILL_PROXY_COMM_MODE_OPTIONS(
                ZGP_PROXY_COMM_MODE_LEAVE,
                /* A.3.3.5.3 When the Action sub-field is set to 0b0, the value of the Exit mode sub-field is
                 * ignored.
                 */
                0,
                ZGP_PROXY_COMM_MODE_CHANNEL_PRESENT,
                /* A.3.3.5.3 When the Action sub-field is set to 0b0, the value of the Unicast communication
                 * sub-field is ignored.
                 */
                ZGP_PROXY_COMM_MODE_IS_UNICAST()),
            ZGP_GPS_COMMISSIONING_WINDOW,
            /* Channel field SHALL NOT be present */
            0,
            cb);
}

/**
 * @brief Hanlde dev_annce at proxy/sink side
 *
 * @param da   [in]   Pointer to the dev_annce info
 *
 * @see ZGP spec, A.3.5.2.3
 */
void zgp_handle_dev_annce(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZGP3, ">> zgp_handle_dev_annce: 0x%hx", (FMT__D, da->nwk_addr));

    /* On receipt of Zigbee Update Device and Device_annce commands with IEEE address other than
     * 0xffffffffffffffff, the Basic Proxy SHALL check if it has the announced device listed in the
     * SinkAddressList of any of its Proxy Table entries. If yes, the mapping of the Sink IEEE address
     * to the Sink NWK 4address SHALL be updated. Further, the proxy SHALL check if the NWKAddr field
     * of the received Device_annce matches any of the aliases used by this proxy. If that's the
     * case, an address conflict with a regular Zigbee device is discovered and the proxy SHALL act
     * according to Zigbee [1] address conflict announcement procedure, i.e. the proxy SHALL send,
     * after randomly chosen delay from between Dmin and Dmax (see A.3.6.3.1), the Zigbee
     * Device_annce command (unless identical frame was received within this time), formatted as
     * described in A.3.6.3.4.2, to force the regular Zigbee device to change its short address. The
     * alias SHALL NOT be changed.
     */
    if (!ZB_IS_64BIT_ADDR_UNKNOWN(da->ieee_addr))
    {
        zb_uint8_t  entries_count;
        zb_uint8_t  i;
        zb_uint8_t  found = 0;

        TRACE_MSG(TRACE_ZGP3, "ieee: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(da->ieee_addr)));
#ifdef ZB_ENABLE_ZGP_SINK
        entries_count = zb_zgp_sink_table_non_empty_entries_count();

        for (i = 0; i < entries_count; i++)
        {
            zgp_tbl_ent_t ent;

            if (zb_zgp_sink_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
            {
                if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_PRECOMMISSIONED)
                {
                    zb_uint8_t j;

                    for (j = 0; j < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; j++)
                    {
                        if (ent.u.sink.sgrp[j].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
                        {
                            if (ent.u.sink.sgrp[j].alias == da->nwk_addr)
                            {
                                found = 1;
                                break;
                            }
                        }
                    }
                }
                else if (ZGP_TBL_SINK_GET_COMMUNICATION_MODE(&ent) == ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED)
                {
                    zb_uint16_t  alias;
                    zb_zgpd_id_t zgpd_id;

                    if (ZGP_TBL_SINK_GET_ASSIGNED_ALIAS(&ent))
                    {
                        alias = ent.zgpd_assigned_alias;
                    }
                    else
                    {
                        ZB_MAKE_ZGPD_ID(zgpd_id,
                                        ZGP_TBL_GET_APP_ID(&ent),
                                        ent.endpoint,
                                        ent.zgpd_id);
                        alias = zgp_calc_alias_source_address(&zgpd_id);
                    }

                    if (alias == da->nwk_addr)
                    {
                        found = 1;
                        break;
                    }
                }
            }

            if (found)
            {
                break;
            }
        }
#endif  /* ZB_ENABLE_ZGP_SINK */

#ifdef ZB_ENABLE_ZGP_PROXY
        if (!found)
        {
            entries_count = zb_zgp_proxy_table_non_empty_entries_count();

            for (i = 0; i < entries_count; i++)
            {
                zgp_tbl_ent_t ent;

                if (zb_zgp_proxy_table_get_entry_by_non_empty_list_index(i, &ent) == ZB_TRUE)
                {
                    if (ZGP_TBL_PROXY_GET_ASSIGNED_ALIAS(&ent))
                    {
                        TRACE_MSG(TRACE_ZGP3, "assigned alias: 0x%x", (FMT__D, ent.zgpd_assigned_alias));
                        if (ent.zgpd_assigned_alias == da->nwk_addr)
                        {
                            found = 1;
                            break;
                        }
                    }

                    if (ZGP_TBL_GET_CGGC(&ent))
                    {
                        zb_uint8_t j;

                        for (j = 0; j < ZB_ZGP_MAX_SINK_GROUP_PER_GPD; j++)
                        {
                            if (ent.u.proxy.sgrp[j].sink_group != ZGP_PROXY_GROUP_INVALID_IDX)
                            {
                                TRACE_MSG(TRACE_ZGP3, "precommissioned alias: 0x%x", (FMT__D, ent.u.proxy.sgrp[j].alias));
                                if (ent.u.proxy.sgrp[j].alias == da->nwk_addr)
                                {
                                    found = 1;
                                    break;
                                }
                            }
                        }
                    }

                    if (ZGP_TBL_GET_DGGC(&ent))
                    {
                        zb_uint16_t  alias;
                        zb_zgpd_id_t zgpd_id;

                        ZB_MAKE_ZGPD_ID(zgpd_id,
                                        ZGP_TBL_GET_APP_ID(&ent),
                                        ent.endpoint,
                                        ent.zgpd_id);
                        alias = zgp_calc_alias_source_address(&zgpd_id);

                        TRACE_MSG(TRACE_ZGP3, "derived alias: 0x%x", (FMT__D, alias));

                        if (alias == da->nwk_addr)
                        {
                            found = 1;
                            break;
                        }
                    }
                }

                if (found)
                {
                    break;
                }
            }
        }
#endif  /* ZB_ENABLE_ZGP_PROXY */

        if (found)
        {
            zb_buf_get_out_delayed_ext(zgp_send_dev_annce_for_alias, da->nwk_addr, 0);
        }
    }
    TRACE_MSG(TRACE_ZGP3, "<< zgp_handle_dev_annce", (FMT__0));
}

#else

void zb_zgp_init()
{
    TRACE_MSG(TRACE_ZGP2, "+ zb_zgp_init", (FMT__0));
}

#endif /* !ZB_ZGPD_ROLE */



zb_uint16_t zb_zgp_ctx_size()
{
    zb_uint16_t size = 0;

#ifndef ZB_ZGPD_ROLE
    size = sizeof(zb_zgp_ctx_t);
#endif

    return size;
}

#ifndef ZB_ZGPD_ROLE
void zb_zgp_sync_pib(zb_uint8_t param)
{
    zb_mlme_set_request_t *req;
    zb_uint8_t value = ZGP_CTXC().skip_gpdf;

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(value));
    req->pib_attr = ZB_PIB_ATTRIBUTE_SKIP_ALL_GPF;
    req->pib_index = 0;
    req->pib_length = sizeof(value);
    ZB_MEMCPY((req + 1), &value, sizeof(value));
    req->confirm_cb_u.cb = NULL;

    ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

void zb_zgp_set_skip_gpdf(zb_uint8_t skip)
{
    ZGP_CTXC().skip_gpdf = skip;
    if (ZGP_CTXC().init_by_scheduler)
    {
        zb_buf_get_out_delayed(zb_zgp_sync_pib);
    } /* else we will sync pib after zgp_init_by_scheduler */
}

zb_uint8_t zb_zgp_get_skip_gpdf(void)
{
    return ZGP_CTXC().skip_gpdf;
}
#endif

/**
 * @brief Parse ZGP Stub NWK header of GPDF
 *
 * @param gpdf      [in]   Pointer to the beginning of ZGP Stub NWK header
 * @param gpdf_len  [in]   Length from beginning of ZGP Stub NWK header to the end of MIC field
 * @param gpdf_info [out]  GPDF info structure
 *
 * @return Number of bytes parsed in ZGP Stub NWK header if parsing successful \n
 *         0 if parsing has failed
 * @see ZGP spec, A.1.4.1
 */
zb_uint8_t zgp_parse_gpdf_nwk_hdr(zb_uint8_t *gpdf, zb_uint8_t gpdf_len, zb_gpdf_info_t *gpdf_info)
{
    zb_uint8_t *start_ptr = gpdf;
    zb_uint8_t  min_expected_gpdf_len;

    TRACE_MSG(TRACE_ZGP3, ">> zgp_parse_gpdf_nwk_hdr gpdf %p, gpdf_len %hd, gpdf_info %p",
              (FMT__P_H_P, gpdf, gpdf_len, gpdf_info));

    gpdf_info->nwk_frame_ctl = *gpdf;
    gpdf++;

    if (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl) == 1)
    {
        gpdf_info->nwk_ext_frame_ctl = *gpdf;
        gpdf++;
    }
    else
    {
        /* ZGP spec, A.1.4.1.3 (about default value of Extended NWK frame control):
         *  Default value to be used on reception, if the Extended NWK Frame Control field
         *  is not present, is 0b00 */
        gpdf_info->nwk_ext_frame_ctl = 0;
    }

    gpdf_info->zgpd_id.app_id = ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl);

    min_expected_gpdf_len = sizeof(gpdf_info->nwk_frame_ctl) + /* NWK frame control size */
                            /* NWK extended frame control size */
                            ((ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl) == 1) ? 1 : 0) +
                            /* SrcID size */
                            ZGPD_SRC_ID_SIZE(gpdf_info->zgpd_id.app_id,
                                    ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl)) +
                            /* Frame counter field size */
                            ((ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) > ZB_ZGP_SEC_LEVEL_REDUCED)
                             ? 4 : 0);

    if (gpdf_len < min_expected_gpdf_len)
    {
        TRACE_MSG(TRACE_ZGP1, "Error: Expected NWK hdr len > (%hd) bytes, actual len (%hd)",
                  (FMT__H_H, min_expected_gpdf_len, gpdf_len));

        TRACE_MSG(TRACE_ZGP3, "<< zgp_parse_gpdf_nwk_hdr, ret %hd", (FMT__H, 0));

        return 0;
    }

    /* ZGP spec, A.1.4.1.4
     * The ZGPDSrcID field is present if the FrameType sub-field is set to 0b00
     * and the ApplicationID sub-field of the Extended NWK Frame Control field
     * is set to 0b000 (or not present) */
    if (ZGPD_SRC_ID_SIZE(gpdf_info->zgpd_id.app_id,
                         ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl)) > 0)
    {
        ZB_MEMCPY((zb_uint8_t *)&gpdf_info->zgpd_id.addr.src_id, gpdf, sizeof(gpdf_info->zgpd_id.addr.src_id));
        gpdf += sizeof(gpdf_info->zgpd_id.addr.src_id);
    }
    else if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0000)
    {
        gpdf_info->zgpd_id.addr.src_id = ZB_ZGP_SRC_ID_UNSPECIFIED;
    }

    /* See A.1.4.1.5 Endpoint field  */
    if (gpdf_info->zgpd_id.app_id == ZB_ZGP_APP_ID_0010)
    {
        gpdf_info->zgpd_id.endpoint = *gpdf;
        gpdf++;
    }
    else
    {
        gpdf_info->zgpd_id.endpoint = 0;
    }

    /* ZGP spec, A.1.4.1.3 (about security frame counter presence):
     *
     * If the SecurityLevel is set to 0b00, the SecurityKey sub-field is ignored
     * on reception, and the fields Security frame counter and MIC are not present.
     * ...
     * If the SecurityLevel is set to 0b01, the Security Frame counter field is not present,
     * the MAC sequence number field carries the 1LSB of the frame counter.
     * ...
     * If the SecurityLevel is set to 0b10 or 0b11, the Security Frame counter field is present,
     * has the length of 4B, and carries the full 4B security frame counter,
     */
    if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
    {
        if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_REDUCED)
        {
            gpdf_info->sec_frame_counter = gpdf_info->mac_seq_num;
        }
        else
        {
            ZB_LETOH32((zb_uint8_t *)&gpdf_info->sec_frame_counter, gpdf);
            gpdf += sizeof(zb_uint32_t);
        }
    }
    else
    {
        gpdf_info->sec_frame_counter = gpdf_info->mac_seq_num;
    }

    /*
    The Auto-Commissioning sub-field has different meaning in a Data (0b00) and
    Maintenance (0b01) FrameType.

    In a Maintenance FrameType, the Auto-Commissioning sub-field, if set to 0b0,
    indicates that the GPD will enter the receive mode gpdRxOffset ms after
    completion of this GPDF transmission, for at least gpdMinRxWindow.  If the value
    of this sub-field is 0b1,  then the GPD will not enter the receive mode after
    sending this particular GPDF.
     */
    if (ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl) == ZGP_FRAME_TYPE_MAINTENANCE
            && !ZB_GPDF_NFC_GET_AUTO_COMMISSIONING(gpdf_info->nwk_frame_ctl))
    {
        ZB_GPDF_EXT_NFC_SET_RX_AFTER_TX(gpdf_info->nwk_ext_frame_ctl, 1);
    }

    TRACE_MSG(TRACE_ZGP3, "<< zgp_parse_gpdf_nwk_hdr, ret %hd", (FMT__H, (gpdf - start_ptr)));

    return (gpdf - start_ptr);
}

#ifdef ZB_TRACE_LEVEL
/**
 * @brief dump @ref zb_gpdf_info_t structure into trace log
 */
void zb_zgp_dump_gpdf_info(zb_gpdf_info_t *gpdf)
{
    TRACE_MSG(TRACE_ZGP3, "------GPDF info------", (FMT__0));
    TRACE_MSG(TRACE_ZGP3, "mac_seq_num %hd", (FMT__H, gpdf->mac_seq_num));
    TRACE_MSG(TRACE_ZGP3, "nwk_frame_ctl 0x%hx", (FMT__H, gpdf->nwk_frame_ctl));
    TRACE_MSG(TRACE_ZGP3, "frame recv timestamp %ld %hd %hd", (FMT__L_H_H, gpdf->recv_timestamp, gpdf->lqi, gpdf->rssi));
    TRACE_MSG(TRACE_ZGP3, "nwk_ext_frame_ctl 0x%hx", (FMT__H, gpdf->nwk_ext_frame_ctl));
    ZB_DUMP_ZGPD_ID(gpdf->zgpd_id);
    TRACE_MSG(TRACE_ZGP3, "sec_frame_counter %d", (FMT__D, gpdf->sec_frame_counter));
    TRACE_MSG(TRACE_ZGP3, "zgpd_cmd_id  0x%hx (encr, if sec_lvl 3)", (FMT__H, gpdf->zgpd_cmd_id));
    TRACE_MSG(TRACE_ZGP3, "---------------------", (FMT__0));
}

void zb_zgp_dump_zgpd_id(zb_zgpd_id_t *id)
{
    TRACE_MSG(TRACE_ZGP3, "zgpd_id.app_id %hd", (FMT__H, (id)->app_id));
    if ((id)->app_id == ZB_ZGP_APP_ID_0000)
    {
        TRACE_MSG(TRACE_ZGP3, "zgpd_id.addr.src_id 0x%x", (FMT__D, (id)->addr.src_id));
    }
    else
    {
        TRACE_MSG(TRACE_ZGP3, "zgpd_id.endpoint %hd", (FMT__H, (id)->endpoint));
        ZB_DUMP_IEEE_ADDR((id)->addr.ieee_addr);
    }
}
#endif  /* ZB_TRACE_LEVEL */

void zb_finish_gpdf_packet(zb_bufid_t buf_ref, zb_uint8_t **ptr)
{
    *ptr = zb_buf_alloc_left(buf_ref, *ptr - (zb_uint8_t *)zb_buf_data0(buf_ref));
}

#endif  /* #ifdef ZB_ENABLE_ZGP */
