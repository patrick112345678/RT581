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
/* PURPOSE: test for binding processes for the NCP host.
*/
#define ZB_TRACE_FILE_ID 41737

#include "zboss_api.h"
#include "zb_common.h"
#include "zb_types.h"
#include "zb_aps.h"
#include "binding.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

#define PAN_ID 0x1aaa
#define CHANNEL_MASK (1l << 11)

zb_ieee_addr_t g_zc_addr = {0xde, 0xad, 0xf0, 0x0d, 0xde, 0xad, 0xf0, 0x0d};
zb_ieee_addr_t g_zr_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint16_t g_remote_addr = 0x0000;
static zb_bool_t g_is_apsme_bound = ZB_FALSE;
static zb_bool_t g_is_zdo_bound = ZB_FALSE;

/* Test functions */
static void zc_send_data(zb_uint8_t param);

static void zc_perform_apsme_bind_unbind(zb_uint8_t param);

static void zc_zdo_bind_unbind_cb(zb_uint8_t param);
static void zc_perform_zdo_bind_unbind(zb_uint8_t param);


static void zc_perform_apsme_bind_unbind(zb_uint8_t param)
{
    zb_apsme_binding_req_t *req;

    if (param == 0)
    {
        zb_buf_get_out_delayed(zc_perform_apsme_bind_unbind);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> zc_perform_apsme_bind", (FMT__0));

    req = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

    ZB_IEEE_ADDR_COPY(req->src_addr, &g_zc_addr);
    ZB_IEEE_ADDR_COPY(req->dst_addr.addr_long, &g_zr_addr);

    req->src_endpoint = DUT_ENDPOINT;
    req->clusterid = TEST_CLUSTER_ID;
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    req->dst_endpoint = TH_ENDPOINT;

    if (!g_is_apsme_bound)
    {
        zb_apsme_bind_request(param);

        ZB_SCHEDULE_APP_ALARM(zc_send_data, 0, ZC_TEST_SEND_DATA_DELAY);
    }
    else
    {
        ZB_SCHEDULE_APP_ALARM_CANCEL(zc_send_data, ZB_ALARM_ANY_PARAM);

        zb_apsme_unbind_request(param);
    }

    g_is_apsme_bound = !g_is_apsme_bound;

    TRACE_MSG(TRACE_APP1, "<< zc_perform_apsme_bind", (FMT__0));
}


static void zc_zdo_bind_unbind_cb(zb_uint8_t param)
{
    zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> zc_zdo_bind_unbind_cb", (FMT__0));

    ZB_ASSERT(bind_resp->status == RET_OK);

    zb_buf_free(param);

    TRACE_MSG(TRACE_APP1, "<< zc_zdo_bind_unbind_cb", (FMT__0));
}


static void zc_perform_zdo_bind_unbind(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *req;

    if (param == 0)
    {
        zb_buf_get_out_delayed(zc_perform_zdo_bind_unbind);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> zc_perform_zdo_bind_unbind", (FMT__0));

    req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);

    ZB_IEEE_ADDR_COPY(req->src_address, g_zc_addr);
    req->src_endp = DUT_ENDPOINT;
    req->cluster_id = TEST_CLUSTER_ID;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_IEEE_ADDR_COPY(req->dst_address.addr_long, g_zr_addr);
    req->dst_endp = TH_ENDPOINT;
    req->req_dst_addr = zb_address_short_by_ieee(g_zc_addr);

    if (!g_is_zdo_bound)
    {
        zb_zdo_bind_req(param, zc_zdo_bind_unbind_cb);

        ZB_SCHEDULE_APP_ALARM(zc_send_data, 0, ZC_TEST_SEND_DATA_DELAY);
    }
    else
    {
        ZB_SCHEDULE_APP_ALARM_CANCEL(zc_send_data, ZB_ALARM_ANY_PARAM);

        zb_zdo_unbind_req(param, zc_zdo_bind_unbind_cb);
    }

    g_is_zdo_bound = !g_is_zdo_bound;

    TRACE_MSG(TRACE_APP1, "<< zc_perform_zdo_bind_unbind", (FMT__0));
}


static void zc_send_data(zb_uint8_t param)
{
    zb_apsde_data_req_t *req = NULL;
    zb_uint8_t *data_ptr = NULL;
    zb_short_t i;

    if (param == 0)
    {
        zb_buf_get_out_delayed(zc_send_data);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> zc_send_data", (FMT__0));

    /* Allocate space in request buffer */
    data_ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);

    /* Fill request options */
    req = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));
    req->dst_addr.addr_short = g_remote_addr;
    req->addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = DUT_ENDPOINT;
    req->dst_endpoint = TH_ENDPOINT;
    req->clusterid = TEST_CLUSTER_ID;

    /* Fill data */
    for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
    {
        data_ptr[i] = i % 32 + '0';
    }

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);

    ZB_SCHEDULE_ALARM(zc_send_data, 0, TEST_SEND_DATA_DELAY);

    TRACE_MSG(TRACE_APP1, "<< zc_send_data", (FMT__0));
}


static void handle_zdo_dev_annce_signal(zb_zdo_signal_device_annce_params_t *da)
{
    TRACE_MSG(TRACE_APP1, ">> handle_zdo_dev_annce_signal", (FMT__0));

    if (!ZB_64BIT_ADDR_CMP(da->ieee_addr, g_zr_addr))
    {
        TRACE_MSG(TRACE_APP1, "Unknown device is joined, nwk_addr %d", (FMT__D, da->device_short_addr));
    }

    if (g_remote_addr == 0)
    {
        g_remote_addr = da->device_short_addr;

        ZB_SCHEDULE_APP_ALARM(zc_perform_apsme_bind_unbind, 0, ZC_TEST_APS_BIND_DELAY);
        ZB_SCHEDULE_APP_ALARM(zc_perform_apsme_bind_unbind, 0, ZC_TEST_APS_UNBIND_DELAY);

        ZB_SCHEDULE_APP_ALARM(zc_perform_zdo_bind_unbind, 0, ZC_TEST_ZDO_BIND_DELAY);
        ZB_SCHEDULE_APP_ALARM(zc_perform_zdo_bind_unbind, 0, ZC_TEST_ZDO_UNBIND_DELAY);
    }

    TRACE_MSG(TRACE_APP1, "<< handle_zdo_dev_annce_signal", (FMT__0));
}


int main()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT | TRACE_SUBSYSTEM_ZDO);

    ZB_INIT("zdo_zc");

    zb_set_long_address(g_zc_addr);
    zb_set_pan_id(PAN_ID);

    zb_set_network_coordinator_role(CHANNEL_MASK);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();


    return 0;
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd, signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_DEVICE_FIRST_START", (FMT__0));

        TRACE_MSG(TRACE_APP1, "Device started with status %hd", (FMT__H, status));

        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break;

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_DEVICE_REBOOT", (FMT__0));

        TRACE_MSG(TRACE_APP1, "Device rebooted, status %hd", (FMT__H, status));
        break;

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));
        handle_zdo_dev_annce_signal(ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t));
        break;

    default:
        break;
    }

    zb_buf_free(param);
}

/*! @} */
