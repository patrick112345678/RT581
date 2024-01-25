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

/* Callbacks */
void ncp_host_handle_bind_response(zb_uint8_t param);
void ncp_host_handle_unbind_response(zb_uint8_t param);


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
    zb_bufid_t buf = zb_buf_get_out();
    zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_role_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    req->scan_channels_list[0] = CHANNEL_MASK;
    req->scan_duration = 5;
    req->distributed_network = 0;
    req->distributed_network_address = 0;

    ncp_host_nwk_formation_adapter(buf);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_role_response", (FMT__0));
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
    zb_ret_t ret;
    zb_uint16_t app_input_cluster_list[] = {ZB_ZCL_CLUSTER_ID_IDENTIFY, ZB_ZCL_CLUSTER_ID_BASIC,
                                            ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_CLUSTER_ID_GROUPS,
                                            ZB_ZCL_CLUSTER_ID_SCENES
                                           };

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(pan_id == PAN_ID);

    ret = ncp_host_af_set_simple_descriptor(1, ZB_AF_HA_PROFILE_ID, NCP_FW_VERSION, 8, 5, 0,
                                            app_input_cluster_list, 0);
    ZB_ASSERT(ret == RET_OK);
    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));
}

void ncp_host_handle_af_set_simple_descriptor_response(zb_ret_t status_code)
{
    zb_ret_t ret;
    static zb_uint8_t resp_count = 0;
    zb_uint16_t app_input_cluster_list[] = {ZB_ZCL_CLUSTER_ID_IDENTIFY, ZB_ZCL_CLUSTER_ID_BASIC};
    zb_uint16_t app_output_cluster_list[] = {ZB_ZCL_CLUSTER_ID_CALENDAR, ZB_ZCL_CLUSTER_ID_METERING,
                                             ZB_ZCL_CLUSTER_ID_PRICE, ZB_ZCL_CLUSTER_ID_MESSAGING,
                                             ZB_ZCL_CLUSTER_ID_IDENTIFY
                                            };

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_af_set_simple_descriptor_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    if (resp_count == 0)
    {
        ret = ncp_host_af_set_simple_descriptor(20, ZB_AF_SE_PROFILE_ID, NCP_FW_VERSION, 9, 2, 5,
                                                app_input_cluster_list, app_output_cluster_list);
        ZB_ASSERT(ret == RET_OK);
        resp_count++;
    }
    else
    {
        ret = ncp_host_af_delete_endpoint(1);
        ZB_ASSERT(ret == RET_OK);
    }

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_af_set_simple_descriptor_response", (FMT__0));
}

void ncp_host_handle_af_delete_endpoint_response(zb_ret_t status_code)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_af_delete_endpoint_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_af_set_node_descriptor(ZB_NWK_DEVICE_TYPE_COORDINATOR, 0x0e, 0xFACE);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_af_delete_endpoint_response", (FMT__0));
}

void ncp_host_handle_af_set_node_descriptor_response(zb_ret_t status_code)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_af_set_node_descriptor_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    ret = ncp_host_af_set_power_descriptor(ZB_POWER_MODE_SYNC_ON_WHEN_IDLE, 7,
                                           ZB_POWER_SRC_DISPOSABLE_BATTERY, ZB_POWER_LEVEL_100);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_af_set_node_descriptor_response", (FMT__0));
}

void ncp_host_handle_af_set_power_descriptor_response(zb_ret_t status_code)
{
    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_af_set_power_descriptor_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_af_set_power_descriptor_response", (FMT__0));
}

void ncp_host_handle_zdo_dev_annce_indication(zb_zdo_signal_device_annce_params_t *da)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_bind_req_param_t *req = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
    zb_uint32_t i;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_dev_annce_indication", (FMT__0));

    ZB_ASSERT(ZB_64BIT_ADDR_CMP(da->ieee_addr, child_ieee_addr));

    child_nwk_addr = da->device_short_addr;

    ZB_IEEE_ADDR_COPY(req->src_address, child_ieee_addr);
    req->src_endp = 1;
    req->cluster_id = ZB_ZCL_CLUSTER_ID_IDENTIFY;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, d_ieee_addr);
    req->dst_endp = 20;
    req->req_dst_addr = child_nwk_addr;

    for (i = 0; i < 100000; i++)
    {
    }

    ncp_host_zdo_bind_req_adapter(buf, ncp_host_handle_bind_response);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_dev_annce_indication", (FMT__0));
}

void ncp_host_handle_bind_response(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);
    zb_zdo_bind_req_param_t *unbind_req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_bind_response: param = %hd, status = %hd",
              (FMT__H_H, param, bind_resp->status));

    ZB_ASSERT(bind_resp->status == RET_OK);

    ZB_IEEE_ADDR_COPY(unbind_req->src_address, child_ieee_addr);
    unbind_req->src_endp = 1;
    unbind_req->cluster_id = ZB_ZCL_CLUSTER_ID_IDENTIFY;
    unbind_req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(unbind_req->dst_address.addr_long, d_ieee_addr);
    unbind_req->dst_endp = 20;
    unbind_req->req_dst_addr = child_nwk_addr;

    ncp_host_zdo_unbind_req_adapter(param, ncp_host_handle_unbind_response);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_bind_response", (FMT__0));

}

void ncp_host_handle_unbind_response(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *unbind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_unbind_response", (FMT__0));

    ZB_ASSERT(unbind_resp->status == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_unbind_response", (FMT__0));
}

int main()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT);

    ZB_INIT("zdo_af_desc");

    while (1)
    {
        zb_sched_loop_iteration();
    }

    return 0;
}

void zboss_signal_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
}
