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

#define ZB_TRACE_FILE_ID 17507
#include "zboss_api.h"
#include "ncp/ncp_host_api.h"

#include "zb_common.h"
#include "zb_types.h"
#include "zb_aps.h"

#define CHANNEL_MASK (1l << 19)
#define PAN_ID 0x1AAA
/* IEEE address of the device */
zb_ieee_addr_t d_ieee_addr = {0x44, 0x33, 0x22, 0x11, 0x00, 0x50, 0x50, 0x50};
/* Standard network key */
zb_uint8_t nwk_key[ZB_CCM_KEY_SIZE] = {0x11, 0xaa, 0x22, 0xbb, 0x33, 0xcc, 0x44, 0xdd, 0, 0, 0, 0, 0, 0, 0, 0};

/* IEEE address of the router */
zb_ieee_addr_t child_ieee_addr = {0x11, 0x11, 0xef, 0xcd, 0xab, 0x50, 0x50, 0x50};
zb_uint16_t child_nwk_addr;

void ncp_host_handle_zdo_ieee_addr_request_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_nwk_addr_request_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_power_descriptor_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_node_descriptor_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_simple_descriptor_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_active_ep_response_adapter(zb_bufid_t buf);
void ncp_host_handle_zdo_match_desc_response_adapter(zb_bufid_t buf);

void ncp_host_handle_reset_response(zb_ret_t status_code, zb_bool_t is_solicited)
{
    zb_ret_t ret;
    static zb_uint8_t reset_count = 0;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_reset_response, is_solicited %hd", (FMT__H, is_solicited));

    ZB_ASSERT(status_code == RET_OK);

    if (0 == reset_count)
    {
        TRACE_MSG(TRACE_APP1, "First boot", (FMT__0));

        reset_count += 1;
        ret = ncp_host_factory_reset();
        ZB_ASSERT(ret == RET_OK);
    }
    else if (1 == reset_count)
    {
        TRACE_MSG(TRACE_APP1, "Second boot, after factory reset", (FMT__0));

        ret = ncp_host_get_module_version();
        ZB_ASSERT(ret == RET_OK);
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Unexpected reboot", (FMT__0));
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_reset_response", (FMT__0));
}

void ncp_host_handle_get_module_version_response(zb_ret_t status_code, zb_uint32_t version)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_module_version_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(version == NCP_FW_VERSION);

    ret = ncp_host_set_zigbee_channel_mask(0, CHANNEL_MASK);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_module_version_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_channel_mask_response(zb_ret_t status_code)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_channel_mask_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_set_zigbee_pan_id(PAN_ID);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_channel_mask_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_pan_id_response(zb_ret_t status_code)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_pan_id_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_set_zigbee_role(ZB_NWK_DEVICE_TYPE_COORDINATOR);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_pan_id_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_role_response(zb_ret_t status_code)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_role_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_set_nwk_key(nwk_key, 0);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_role_response", (FMT__0));
}

void ncp_host_handle_set_nwk_key_response(zb_ret_t status_code)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_nwk_key_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_get_nwk_keys();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_nwk_key_response", (FMT__0));
}

void ncp_host_handle_get_nwk_keys_response(zb_ret_t status_code,
        zb_uint8_t *nwk_key1, zb_uint8_t key_number1,
        zb_uint8_t *nwk_key2, zb_uint8_t key_number2,
        zb_uint8_t *nwk_key3, zb_uint8_t key_number3)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);

    ZVUNUSED(nwk_key1);
    ZVUNUSED(nwk_key2);
    ZVUNUSED(nwk_key3);
    ZVUNUSED(key_number1);
    ZVUNUSED(key_number2);
    ZVUNUSED(key_number3);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_nwk_keys_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    req->scan_channels_list[0] = CHANNEL_MASK;
    req->scan_duration = 5;
    req->distributed_network = 0;
    req->distributed_network_address = 0;

    ncp_host_nwk_formation_adapter(buf);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_nwk_keys_response", (FMT__0));
}

void ncp_host_handle_nwk_formation_response_adapter(zb_bufid_t buf)
{
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_formation_response_adapter", (FMT__0));

    ZB_ASSERT(zb_buf_get_status(buf) == RET_OK);
    zb_buf_reuse(buf);

    req = ZB_BUF_GET_PARAM(buf, zb_nlme_permit_joining_request_t);
    req->permit_duration = 180;

    ncp_host_nwk_permit_joining_adapter(buf);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_formation_response_adapter", (FMT__0));
}

void ncp_host_handle_nwk_permit_joining_response_adapter(zb_bufid_t buf)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_permit_joining_response_adapter", (FMT__0));

    ZB_ASSERT(zb_buf_get_status(buf) == RET_OK);
    zb_buf_free(buf);

    /* ret = ncp_host_secur_add_ic(child_iee_addr, CHILD_IC_TYPE, child_ic); */
    ret = ncp_host_get_zigbee_role();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_permit_joining_response_adapter", (FMT__0));
}

void ncp_host_handle_get_zigbee_role_response(zb_ret_t status_code, zb_uint8_t zigbee_role)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_role_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(zigbee_role == ZB_NWK_DEVICE_TYPE_COORDINATOR);

    ret = ncp_host_get_zigbee_channel_mask();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_role_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_channel_mask_response(zb_ret_t status_code, zb_uint8_t channel_list_len,
        zb_uint8_t channel_page, zb_uint32_t channel_mask)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_channel_mask_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ZB_ASSERT(channel_mask == CHANNEL_MASK);
    ZB_ASSERT(channel_page == 0);
    ZB_ASSERT(channel_list_len == 1);

    ret = ncp_host_get_zigbee_channel();
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_channel_mask_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_channel_response(zb_ret_t status_code, zb_uint8_t page, zb_uint8_t channel)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_channel_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(page == 0);
    ZB_ASSERT(channel == 19);

    ret = ncp_host_get_zigbee_pan_id();
    ZB_ASSERT(ret == RET_OK);


    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_channel_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_pan_id_response(zb_ret_t status_code, zb_uint16_t pan_id)
{
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(pan_id == PAN_ID);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));
}

void ncp_host_handle_zdo_dev_annce_signal(zb_zdo_signal_device_annce_params_t *da)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_dev_annce_signal", (FMT__0));

    ZB_ASSERT(ZB_64BIT_ADDR_CMP(da->ieee_addr, child_ieee_addr));

    ret = ncp_host_nwk_get_ieee_by_short(da->device_short_addr);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_dev_annce_signal", (FMT__0));
}

void ncp_host_handle_nwk_get_ieee_by_short_response(zb_ret_t status_code, zb_ieee_addr_t ieee_addr)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_ieee_by_short_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(ZB_64BIT_ADDR_CMP(child_ieee_addr, ieee_addr));

    ret = ncp_host_nwk_get_neighbor_by_ieee(ieee_addr);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_ieee_by_short_response", (FMT__0));
}

void ncp_host_handle_nwk_get_neighbor_by_ieee_response(zb_ret_t status_code)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_neighbor_by_ieee_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    /* TODO: change child_ieee_addr to ieee_addr from Get Neighbor by IEEE command */
    ret = ncp_host_nwk_get_short_by_ieee(child_ieee_addr);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_neighbor_by_ieee_response", (FMT__0));
}

void ncp_host_handle_nwk_get_short_by_ieee_response(zb_ret_t status_code, zb_uint16_t short_addr)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_ieee_addr_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_short_by_ieee_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    child_nwk_addr = short_addr;

    req_param->nwk_addr = short_addr;
    req_param->dst_addr = req_param->nwk_addr;
    req_param->start_index = 0;
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    ncp_host_zdo_ieee_addr_request_adapter(buf, ncp_host_handle_zdo_ieee_addr_request_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_short_by_ieee_response", (FMT__0));
}

void ncp_host_handle_zdo_ieee_addr_request_response_adapter(zb_bufid_t buf)
{
    zb_zdo_ieee_addr_resp_t *resp;
    zb_zdo_nwk_addr_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_ieee_addr_request_response_adapter", (FMT__0));

    resp = (zb_zdo_ieee_addr_resp_t *)zb_buf_begin(buf);
    ZB_ASSERT(ZB_64BIT_ADDR_CMP(child_ieee_addr, resp->ieee_addr_remote_dev));

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_nwk_addr_req_param_t);

    req_param->dst_addr = resp->nwk_addr_remote_dev;
    ZB_MEMCPY(req_param->ieee_addr, child_ieee_addr, sizeof(zb_ieee_addr_t));
    req_param->start_index = 0;
    req_param->request_type = ZB_ZDO_SINGLE_DEVICE_RESP;
    ncp_host_zdo_nwk_addr_request_adapter(buf, ncp_host_handle_zdo_nwk_addr_request_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_ieee_addr_request_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_nwk_addr_request_response_adapter(zb_bufid_t buf)
{
    zb_zdo_nwk_addr_resp_head_t *resp;
    zb_zdo_power_desc_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_nwk_addr_request_response_adapter", (FMT__0));

    resp = (zb_zdo_nwk_addr_resp_head_t *)zb_buf_begin(buf);
    ZB_ASSERT(ZB_64BIT_ADDR_CMP(child_ieee_addr, resp->ieee_addr));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_power_desc_req_t));
    req->nwk_addr = resp->nwk_addr;
    ncp_host_zdo_power_desc_req_adapter(buf, ncp_host_handle_zdo_power_descriptor_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_nwk_addr_request_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_power_descriptor_response_adapter(zb_bufid_t buf)
{
    zb_zdo_power_desc_resp_t *resp;
    zb_zdo_node_desc_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_power_descriptor_response_adapter", (FMT__0));

    resp = (zb_zdo_power_desc_resp_t *)zb_buf_begin(buf);
    ZB_ASSERT(resp->power_desc.power_desc_flags == 0);

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_node_desc_req_t));
    req->nwk_addr = child_nwk_addr;
    ncp_host_zdo_node_desc_req_adapter(buf, ncp_host_handle_zdo_node_descriptor_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_power_descriptor_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_node_descriptor_response_adapter(zb_bufid_t buf)
{
    zb_zdo_node_desc_resp_t *resp;
    zb_zdo_simple_desc_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_node_descriptor_response_adapter", (FMT__0));

    resp = (zb_zdo_node_desc_resp_t *)zb_buf_begin(buf);
    ZB_ASSERT(resp->node_desc.manufacturer_code == NCP_FW_VERSION);
    ZB_ASSERT(resp->node_desc.max_buf_size == 108);
    ZB_ASSERT(resp->node_desc.max_incoming_transfer_size == 103);
    ZB_ASSERT(resp->node_desc.max_outgoing_transfer_size == 103);

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_simple_desc_req_t));
    req->nwk_addr = child_nwk_addr;
    req->endpoint = 1;
    ncp_host_zdo_simple_desc_req_adapter(buf, ncp_host_handle_zdo_simple_descriptor_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_node_descriptor_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_simple_descriptor_response_adapter(zb_bufid_t buf)
{
    zb_zdo_simple_desc_resp_t *resp;
    zb_zdo_active_ep_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_simple_descriptor_response_adapter", (FMT__0));

    resp = (zb_zdo_simple_desc_resp_t *)zb_buf_begin(buf);
    ZB_ASSERT(resp->simple_desc.endpoint == 1);
    ZB_ASSERT(resp->simple_desc.app_profile_id == ZB_AF_SE_PROFILE_ID);
    ZB_ASSERT(resp->simple_desc.app_device_id == 222);
    ZB_ASSERT(resp->simple_desc.app_device_version == 9);
    ZB_ASSERT(resp->simple_desc.app_input_cluster_count == 3);
    ZB_ASSERT(resp->simple_desc.app_output_cluster_count == 2);

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_active_ep_req_t));
    req->nwk_addr = child_nwk_addr;
    ncp_host_zdo_active_ep_req_adapter(buf, ncp_host_handle_zdo_active_ep_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_simple_descriptor_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_active_ep_response_adapter(zb_bufid_t buf)
{
    zb_zdo_ep_resp_t *resp;
    zb_uint8_t *ep_list_ptr;
    zb_zdo_match_desc_param_t *req;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_active_ep_response_adapter", (FMT__0));

    resp = (zb_zdo_ep_resp_t *)zb_buf_begin(buf);
    ep_list_ptr = (zb_uint8_t *)(resp + sizeof(zb_zdo_ep_resp_t));
    ZB_ASSERT(resp->ep_count == 1);
    ZB_ASSERT(*ep_list_ptr == 1);

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (3 + 2) * sizeof(zb_uint16_t));

    req->nwk_addr = child_nwk_addr;
    req->addr_of_interest = child_nwk_addr;
    req->profile_id = ZB_AF_SE_PROFILE_ID;
    req->num_in_clusters = 3;
    req->num_out_clusters = 2;
    req->cluster_list[0] = 1000;
    req->cluster_list[1] = 1001;
    req->cluster_list[2] = 1002;
    req->cluster_list[3] = 2000;
    req->cluster_list[4] = ZB_ZCL_CLUSTER_ID_IDENTIFY;

    zb_zdo_match_desc_req(buf, NULL);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_active_ep_response_adapter", (FMT__0));
}

void ncp_host_handle_zdo_match_desc_response_adapter(zb_bufid_t buf)
{
    zb_zdo_match_desc_resp_t *resp;
    zb_uint8_t *match_ep;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_match_desc_response_adapter", (FMT__0));

    resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(buf);
    match_ep = (zb_uint8_t *)(resp + sizeof(zb_zdo_match_desc_resp_t));
    ZB_ASSERT(resp->match_len == 1);
    ZB_ASSERT(*match_ep == 1);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_match_desc_response_adapter", (FMT__0));
}

int main()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT);

    ZB_INIT("zdo_desc_zc");

    while (1)
    {
        zb_sched_loop_iteration();
    }

    return 0;
}

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));
            ncp_host_handle_zdo_dev_annce_signal(ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t));
            break;

        default:
            TRACE_MSG(TRACE_APP1, "Unhandled signal 0x%hx", (FMT__H, sig));
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}
