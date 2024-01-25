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

/* IEEE address of the device */
zb_ieee_addr_t d_ieee_addr = {0x44, 0x33, 0x22, 0x11, 0x00, 0x50, 0x50, 0x50};

void ncp_host_handle_zdo_permit_joining_response_adapter(zb_bufid_t buf);

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

    ret = ncp_host_set_zigbee_role(ZB_NWK_DEVICE_TYPE_ROUTER);
    ZB_ASSERT(ret == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_channel_mask_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_role_response(zb_ret_t status_code)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_nlme_network_discovery_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_discovery_request_t);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_role_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);

    req->scan_channels_list[0] = CHANNEL_MASK;
    req->scan_duration = ZB_DEFAULT_SCAN_DURATION;

    ncp_host_nwk_discovery_adapter(buf);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_role_response", (FMT__0));
}

void ncp_host_handle_nwk_discovery_response_adapter(zb_bufid_t buf)
{
    zb_nlme_network_discovery_confirm_t *cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(buf);
    zb_nlme_network_descriptor_t *dsc = (zb_nlme_network_descriptor_t *)(cnf + 1);
    zb_nlme_join_request_t *req;
    zb_ext_pan_id_t pan_id;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_discovery_response_adapter", (FMT__0));

    ZB_ASSERT(cnf->status == RET_OK);
    ZB_ASSERT(cnf->network_count == 1);

    req = ZB_BUF_GET_PARAM(buf, zb_nlme_join_request_t);
    zb_address_get_pan_id(dsc[0].panid_ref, pan_id);
    ZB_MEMCPY(req->extended_pan_id, pan_id, sizeof(zb_ext_pan_id_t));
    req->rejoin_network = ZB_NLME_REJOIN_METHOD_ASSOCIATION;
    req->scan_channels_list[0] = CHANNEL_MASK;
    req->scan_duration = ZB_DEFAULT_SCAN_DURATION;
    req->capability_information = 0x8E;
    req->security_enable = 2;

    ncp_host_nwk_nlme_join_adapter(buf);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_discovery_response_adapter", (FMT__0));
}

void ncp_host_handle_nwk_nlme_join_response_adapter(zb_bufid_t buf)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_nlme_join_response_adapter", (FMT__0));

    zb_buf_free(buf);
    ret = ncp_host_get_join_status();

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_nlme_join_response_adapter", (FMT__0));
}

void ncp_host_handle_get_join_status_response(zb_ret_t status_code, zb_uint8_t joined)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_join_status_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(joined == 1);

    ret = ncp_host_get_authentication_status();

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_join_status_response", (FMT__0));
}

void ncp_host_handle_get_authentication_status_response(zb_ret_t status_code, zb_uint8_t authenticated)
{
    zb_uint8_t zdp_tsn;
    zb_bufid_t buf = zb_buf_get_out();
    zb_zdo_mgmt_permit_joining_req_param_t *req_param
        = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_permit_joining_req_param_t);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_authentication_status_response", (FMT__0));

    ZB_ASSERT(status_code == RET_OK);
    ZB_ASSERT(authenticated == 1);

    req_param->dest_addr = 0;
    req_param->permit_duration = 0;
    zdp_tsn = ncp_host_zdo_permit_joining_request_adapter(buf,
              ncp_host_handle_zdo_permit_joining_response_adapter);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_authentication_status_response", (FMT__0));
}

void ncp_host_handle_zdo_permit_joining_response_adapter(zb_bufid_t buf)
{
    zb_zdo_mgmt_permit_joining_resp_t *resp = (zb_zdo_mgmt_permit_joining_resp_t *)zb_buf_begin(buf);

    TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_permit_joining_response", (FMT__0));

    ZB_ASSERT(resp->status == RET_OK);

    TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_permit_joining_response", (FMT__0));
}

int main()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT);

    ZB_INIT("zdo_permit_joining_zr");

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
