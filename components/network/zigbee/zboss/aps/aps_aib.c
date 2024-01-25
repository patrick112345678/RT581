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
/* PURPOSE: AIB getters and setters
*/

#define ZB_TRACE_FILE_ID 3005

#include "zb_common.h"
#include "zb_aps.h"

void zb_apsme_get_request(zb_uint8_t param)
{
    zb_ret_t status = (zb_ret_t)ZB_APS_STATUS_SUCCESS;
    zb_apsme_get_request_t req_param;
    zb_apsme_get_confirm_t *conf;

    TRACE_MSG(TRACE_APS2, ">>zb_apsme_get_req %d", (FMT__D, param));
    ZB_MEMCPY(&req_param, zb_buf_begin(param), sizeof(req_param));

    switch (req_param.aib_attr)
    {
    case ZB_APS_AIB_DESIGNATED_COORD:
        conf = zb_buf_initial_alloc(param, sizeof(zb_apsme_get_confirm_t) + sizeof(zb_uint8_t));
        conf->aib_length = (zb_uint8_t)sizeof(zb_uint8_t);
        *(((zb_uint8_t *)conf) + sizeof(zb_apsme_get_confirm_t)) = ZB_AIB().aps_designated_coordinator;
        break;
    case ZB_APS_AIB_USE_INSECURE_JOIN:
        conf = zb_buf_initial_alloc(param, sizeof(zb_apsme_get_confirm_t) + sizeof(zb_uint8_t));
        conf->aib_length = (zb_uint8_t)sizeof(zb_uint8_t);
        *(((zb_uint8_t *)conf) + sizeof(zb_apsme_get_confirm_t)) = ZB_AIB().aps_insecure_join;
        break;
    case ZB_APS_AIB_CHANNEL_MASK_LIST:
    {
        zb_channel_page_t chan_list[ZB_CHANNEL_PAGES_NUM];
        conf = zb_buf_initial_alloc(param, sizeof(zb_apsme_get_confirm_t) + ZB_CHANNEL_PAGES_NUM * sizeof(zb_channel_page_t));
        conf->aib_length = ZB_CHANNEL_PAGES_NUM * (zb_uint8_t)sizeof(zb_channel_page_t);
        zb_channel_page_list_copy(chan_list, ZB_AIB().aps_channel_mask_list);
        ZB_MEMCPY((zb_uint8_t *)conf + sizeof(zb_apsme_get_confirm_t), chan_list, sizeof(chan_list));
        break;
    }
    case ZB_APS_AIB_USE_EXT_PANID:
        conf = zb_buf_initial_alloc(param, sizeof(zb_apsme_get_confirm_t) + sizeof(zb_ext_pan_id_t));
        conf->aib_length = (zb_uint8_t)sizeof(zb_ext_pan_id_t);
        ZB_64BIT_ADDR_COPY((((zb_uint8_t *)conf) + sizeof(zb_apsme_get_confirm_t)), ZB_AIB().aps_use_extended_pan_id);
        break;
    default:
        conf = zb_buf_initial_alloc(param, sizeof(zb_apsme_get_confirm_t));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_UNSUPPORTED_ATTRIBUTE);
        conf->aib_length = 0;
        break;
    }
    conf->aib_attr = req_param.aib_attr;
    conf->status = status;
    if (req_param.confirm_cb != NULL)
    {
        TRACE_MSG(TRACE_APS2, "call APSME-GET.confirm %hd status %hd", (FMT__H_H, param, status));
        ZB_SCHEDULE_CALLBACK(req_param.confirm_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
    TRACE_MSG(TRACE_APS2, "<<zb_apsme_get_req %d", (FMT__D, param));
}


void zb_apsme_set_request(zb_uint8_t param)
{
    zb_apsme_set_request_t *req_param = zb_buf_begin(param);
    zb_uint8_t *ptr = (zb_uint8_t *)req_param;
    zb_apsme_set_confirm_t conf;
    zb_apsme_set_confirm_t *conf_p;
    zb_callback_t confirm_cb;

    TRACE_MSG(TRACE_APS2, ">>zb_apsme_set_req %d", (FMT__D, param));

    conf.aib_attr = req_param->aib_attr;
    conf.status = (zb_ret_t)ZB_APS_STATUS_SUCCESS;
    ptr += sizeof(zb_apsme_set_request_t);
    switch (req_param->aib_attr)
    {
    case ZB_APS_AIB_DESIGNATED_COORD:
        ZB_AIB().aps_designated_coordinator = *ptr;
        break;
    case ZB_APS_AIB_USE_INSECURE_JOIN:
        ZB_AIB().aps_insecure_join = *ptr;
        break;
    case ZB_APS_AIB_CHANNEL_MASK_LIST:
    {
        zb_channel_page_t chan_list[ZB_CHANNEL_PAGES_NUM];
        ZB_MEMCPY(chan_list, ptr, sizeof(chan_list));
        zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, chan_list);
        break;
    }
    case ZB_APS_AIB_USE_EXT_PANID:
        ZB_EXTPANID_COPY(ZB_AIB().aps_use_extended_pan_id, ptr);
        break;
    default:
        conf.status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_UNSUPPORTED_ATTRIBUTE);
        break;
    }
    confirm_cb = req_param->confirm_cb;
    conf_p = zb_buf_initial_alloc(param, sizeof(zb_apsme_set_confirm_t));
    ZB_MEMCPY(conf_p, &conf, sizeof(*conf_p));
    if (confirm_cb != NULL)
    {
        TRACE_MSG(TRACE_APS2, "call APSME-SET.confirm %hd status %hd", (FMT__H_H, param, conf.status));
        ZB_SCHEDULE_CALLBACK(confirm_cb, param);
    }
    else
    {
        zb_buf_free(param);
    }
    TRACE_MSG(TRACE_APS2, "<<zb_apsme_set_req %d", (FMT__D, param));
}


void zb_aib_tcpol_set_update_trust_center_link_keys_required(zb_bool_t enable)
{
    ZB_TCPOL().update_trust_center_link_keys_required = enable;
}

zb_bool_t zb_aib_tcpol_get_update_trust_center_link_keys_required(void)
{
    return (zb_bool_t)ZB_TCPOL().update_trust_center_link_keys_required;
}

zb_bool_t zb_aib_tcpol_get_allow_unsecure_tc_rejoins(void)
{
    return (zb_bool_t)ZB_TCPOL().allow_unsecure_tc_rejoins;
}

void zb_aib_tcpol_set_authenticate_always(zb_bool_t authenticate_always)
{
    ZB_TCPOL().authenticate_always = ZB_B2U(authenticate_always);
}


#ifdef ZB_DISTRIBUTED_SECURITY_ON

void zb_aib_tcpol_set_is_distributed_security(zb_bool_t enable)
{
    ZB_TCPOL().is_distributed = enable;
}

zb_bool_t zb_aib_tcpol_get_is_distributed_security(void)
{
    return ZB_TCPOL().is_distributed;
}

#endif /* ZB_DISTRIBUTED_SECURITY_ON */

void zb_aib_set_trust_center_address(const zb_ieee_addr_t address)
{
    ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, address);
}

void zb_aib_get_trust_center_address(zb_ieee_addr_t address)
{
    ZB_IEEE_ADDR_COPY(address, ZB_AIB().trust_center_address);
}

zb_uint16_t zb_aib_get_trust_center_short_address(void)
{
#ifndef ZB_COORDINATOR_ONLY
    return zb_address_short_by_ieee(ZB_AIB().trust_center_address);
#else
    return 0x0000;
#endif /* ZB_COORDINATOR_ONLY */
}

zb_bool_t zb_aib_trust_center_address_zero(void)
{
    return (zb_bool_t)ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address);
}

zb_bool_t zb_aib_trust_center_address_unknown(void)
{
    return (zb_bool_t)ZB_IEEE_ADDR_IS_UNKNOWN(ZB_AIB().trust_center_address);
}

zb_bool_t zb_aib_trust_center_address_cmp(const zb_ieee_addr_t address)
{
    return (zb_bool_t)ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, address);
}

void zb_aib_set_coordinator_version(zb_uint8_t version)
{
    ZB_AIB().coordinator_version = version;
}

zb_uint8_t zb_aib_get_coordinator_version(void)
{
    return ZB_AIB().coordinator_version;
}

/* AIB channel mask page helpers */

void zb_aib_get_channel_page_list(zb_channel_list_t list)
{
    zb_channel_page_list_copy(list, ZB_AIB().aps_channel_mask_list);
}

void zb_aib_channel_page_list_set_mask(zb_uint8_t  idx,
                                       zb_uint32_t mask)
{
    zb_channel_page_list_set_mask(ZB_AIB().aps_channel_mask_list, idx, mask);
}

zb_uint32_t zb_aib_channel_page_list_get_mask(zb_uint8_t idx)
{
    return zb_channel_page_list_get_mask(ZB_AIB().aps_channel_mask_list, idx);
}

void zb_aib_channel_page_list_set_page(zb_uint8_t idx,
                                       zb_uint8_t page)
{
    zb_channel_page_list_set_page(ZB_AIB().aps_channel_mask_list, idx, page);
}

zb_uint8_t zb_aib_channel_page_list_get_page(zb_uint8_t idx)
{
    return zb_channel_page_list_get_page(ZB_AIB().aps_channel_mask_list, idx);
}

void  zb_aib_channel_page_list_set_2_4GHz_mask(zb_uint32_t mask)
{
    zb_channel_list_init(ZB_AIB().aps_channel_mask_list);
    zb_channel_page_list_set_2_4GHz_mask(ZB_AIB().aps_channel_mask_list, mask);
}

zb_uint32_t zb_aib_channel_page_list_get_2_4GHz_mask()
{
    return zb_channel_page_list_get_2_4GHz_mask(ZB_AIB().aps_channel_mask_list);
}

zb_uint8_t zb_aib_channel_page_list_get_first_filled_page(void)
{
    return zb_channel_page_list_get_first_filled_page(ZB_AIB().aps_channel_mask_list);
}
