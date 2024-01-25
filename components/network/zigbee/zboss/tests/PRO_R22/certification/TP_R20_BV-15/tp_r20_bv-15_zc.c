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
/* PURPOSE: TP/R20/BV-15: Messagess with Wildcard Profile
To confirm that a device will answer a Match Descriptor request and other APS data messages using the wildcard profile ID (0xFFFF).
Coordinator side
*/

#define ZB_TEST_NAME TP_R20_BV_15_ZC
#define ZB_TRACE_FILE_ID 40658

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../common/zb_cert_test_globals.h"
#include "test_common.h"


static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t g_ieee_addr_r = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    zb_set_pan_id(0x1aaa);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    zb_set_nvram_erase_at_start(ZB_TRUE);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void match_desc_req_wildcard(zb_uint8_t param);


static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "Test status: OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
    }
}


static void buffer_test_req_wildcard(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->src_ep = 1;
    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_r); //send to router
    req_param->len = 0x0A;
    req_param->profile_id = 0xffff; /* wildcard */

    zb_tp_buffer_test_request(param, buffer_test_cb);
}


static void match_desc_callback(zb_uint8_t param)
{
    static zb_ushort_t cnt = 0;

    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;
    zb_uint8_t *match_list = (zb_uint8_t *)(resp + 1);
    zb_uint8_t g_error = 0;

    TRACE_MSG(TRACE_APS1, "match_desc_callback status %hd, addr 0x%x",
              (FMT__H, resp->status, resp->nwk_addr));
    if (resp->status != ZB_ZDP_STATUS_SUCCESS || resp->nwk_addr != zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_r))
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect status/addr", (FMT__0));
        g_error++;
    }
    /*
      asdu=Match_Descr_rsp(Status=0x00=Success, NWKAddrOfInterest=0x0000,
      MatchLength=0x01, MatchList=0x01)
    */
    TRACE_MSG(TRACE_APS1, "match_len %hd, list %hd ", (FMT__H_H, resp->match_len, *match_list));
    if (resp->match_len != 1 || *match_list != 1)
    {
        TRACE_MSG(TRACE_APS1, "Error incorrect match result", (FMT__0));
        g_error++;
    }

    TRACE_MSG(TRACE_APS1, "error counter %hd", (FMT__H, g_error));


    if (g_error != 0)
    {
        TRACE_MSG(TRACE_APS1, "Test FAILED", (FMT__0));
        zb_buf_free(param);
    }
    else
    {

        if (cnt == 0)
        {
            ZB_SCHEDULE_CALLBACK(match_desc_req_wildcard, param);
        }
        else if (cnt == 1)
        {
            ZB_SCHEDULE_CALLBACK(buffer_test_req_wildcard, param);
        }
        else
        {
            zb_buf_free(param);
        }
        cnt++;
    }
}


static void match_desc_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, ">>match_desc_req", (FMT__0));

    if (buf)
    {
        zb_zdo_match_desc_param_t *req;

        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t));

        /*
          NWKAddrOfInterest=0x0000 16-bit DUT ZC NWK address
          ProfileID=Profile of interest to match=0x0103
          NumInClusters=Number of input clusters to match=0x02,
          InClusterList=matching cluster list=0x54 0xe0
          NumOutClusters=return value=0x03
          OutClusterList=return value=0x1c 0x38 0xa8
        */

        req->nwk_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_r); //send to router
        req->addr_of_interest = req->nwk_addr;
        req->profile_id = 0x7f01;
        req->num_in_clusters = 2;
        req->num_out_clusters = 3;
        req->cluster_list[0] = 0x54;
        req->cluster_list[1] = 0xe0;

        req->cluster_list[2] = 0x1c;
        req->cluster_list[3] = 0x38;
        req->cluster_list[4] = 0xa8;

        zb_zdo_match_desc_req(buf, match_desc_callback);
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "<<match_desc_req", (FMT__0));
}


static void match_desc_req_wildcard(zb_uint8_t param)
{
    zb_zdo_match_desc_param_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + (2 + 3) * sizeof(zb_uint16_t));

    req->nwk_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_r); //send to router
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = 0xffff;                                /* windcard */
    req->num_in_clusters = 2;
    req->num_out_clusters = 3;
    req->cluster_list[0] = 0x54;
    req->cluster_list[1] = 0xe0;

    req->cluster_list[2] = 0x1c;
    req->cluster_list[3] = 0x38;
    req->cluster_list[4] = 0xa8;

    zb_zdo_match_desc_req(param, match_desc_callback);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(match_desc_req, 0, 15 * ZB_TIME_ONE_SECOND);
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}
