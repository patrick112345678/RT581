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
/* PURPOSE: TP/NWK/BV-02 ZR

See 11.2 TP/NWK/BV-02 ZR-ZDO-APL RX Join/Leave
*/

#define ZB_TEST_NAME TP_NWK_BV_02_ZR
#define ZB_TRACE_FILE_ID 40506

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_nwk_bv-02_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_r = IEEE_ADDR_R;

static void send_data(zb_uint8_t param);
static void zb_nwk_leave_req(zb_uint8_t param);

static void zb_start_join(zb_uint8_t param);
static zb_ret_t initiate_rejoin1(zb_bufid_t buf);

static zb_bool_t flag;

MAIN()
{
    ARGV_UNUSED;
    flag = ZB_TRUE;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("tp_nwk_bv_02_ZR");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_r);

    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_set_max_children(0);

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

            if (flag)
            {
                /* Leave net after join */
                ZB_SCHEDULE_ALARM(zb_nwk_leave_req, 0, TIME_ZR_LEAVE);

                /* Send test buffer request after leave procedure (from unjoined device) */
                ZB_SCHEDULE_ALARM(send_data, 0, TIME_ZR_DATA1);

                /* Join to the coordinator */
                ZB_SCHEDULE_ALARM(zb_start_join, 0, TIME_ZR_JOIN1);

#if 0
                /* Rejoin after leave from ZC - we don't need it, just need an error from buffertest */
                ZB_SCHEDULE_ALARM(zb_start_join, 0, TIME_ZR_JOIN2);
#endif
                /* Send test buffer request to the coordinator after leave request from coordinator */
                ZB_SCHEDULE_ALARM(send_data, 0, TIME_ZR_DATA2);

                flag = ZB_FALSE;
            }
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


static void zb_nwk_leave_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_nlme_leave_request_t *lr = NULL;

    TRACE_MSG(TRACE_ERROR, ">>zb_nwk_leave_req", (FMT__0));
    ZVUNUSED(param);

    if (buf)
    {
        lr = ZB_BUF_GET_PARAM(buf, zb_nlme_leave_request_t);
        ZB_64BIT_ADDR_ZERO(lr->device_address);
        lr->remove_children = 0;
        lr->rejoin = 0;
        ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ERROR, "<<zb_nwk_leave_req", (FMT__0));
}


static zb_ret_t initiate_rejoin1(zb_bufid_t buf)
{
    zb_nlme_join_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_join_request_t);
    zb_ext_pan_id_t aps_use_extended_pan_id;

    TRACE_MSG(TRACE_INFO3, "###initiate_rejoin1", (FMT__0));

    ZB_MEMSET(req, 0, sizeof(*req));

    zb_get_use_extended_pan_id(aps_use_extended_pan_id);

    ZB_EXTPANID_COPY(req->extended_pan_id, aps_use_extended_pan_id);

#ifdef ZB_ROUTER_ROLE
    if (zb_get_device_type() == ZB_NWK_DEVICE_TYPE_NONE)
    {
        zb_cert_test_set_zr_role();
        ZB_MAC_CAP_SET_ROUTER_CAPS(req->capability_information);
        TRACE_MSG(TRACE_APS1, "###Rejoin to pan " TRACE_FORMAT_64 " as ZR", (FMT__A, TRACE_ARG_64(aps_use_extended_pan_id)));
    }
    else
#endif
    {
        TRACE_MSG(TRACE_APS1, "###Rejoin to pan " TRACE_FORMAT_64 " as ZE", (FMT__A, TRACE_ARG_64(aps_use_extended_pan_id)));
        if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
        {
            ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(req->capability_information, 1);
        }
    }

    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(req->capability_information, 1);
    req->rejoin_network = ZB_NLME_REJOIN_METHOD_REJOIN;

    /*zb_channel_page_list_copy(req->scan_channels_list, ZB_AIB().aps_channel_mask_list);*/
    zb_aib_get_channel_page_list(req->scan_channels_list);

    req->scan_duration = ZB_DEFAULT_SCAN_DURATION;

#ifndef NCP_MODE_HOST
    ZG->zdo.handle.rejoin = ZB_TRUE;
    ZB_SET_JOINED_STATUS(ZB_FALSE);
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

    return zb_schedule_callback(zb_nlme_join_request, buf);
}


static void zb_start_join(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_bufid_t buf = zb_buf_get_out();
    zb_ext_pan_id_t aps_use_extended_pan_id;
    ZVUNUSED(param);
    TRACE_MSG(TRACE_ERROR, "###zb_start_join", (FMT__0));

#ifdef ZB_NWK_BLACKLIST
    zb_nwk_blacklist_reset();
#endif

    if (!buf)
    {
        TRACE_MSG(TRACE_ERROR, "###zb_start_join: error - unable to get data buffer", (FMT__0));
        return;
    }

    COMM_CTX().discovery_ctx.disc_count = COMM_CTX().discovery_ctx.nwk_scan_attempts;
    //  ZG->zdo.handle.started = 0;

    zb_set_long_address(g_ieee_addr_r);

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_get_use_extended_pan_id(aps_use_extended_pan_id);

    /* zb_cert_test_set_security_level(0); */
    if (!ZB_EXTPANID_IS_ZERO(aps_use_extended_pan_id))
    {
        ret = initiate_rejoin1(buf);
    }
    else
    {
        zb_nlme_network_discovery_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_discovery_request_t);
#ifdef ZB_ROUTER_ROLE
        if (zb_get_device_type() == ZB_NWK_DEVICE_TYPE_NONE)
        {
            zb_cert_test_set_device_type(ZB_NWK_DEVICE_TYPE_ROUTER);
        }
#endif

        zb_aib_get_channel_page_list(req->scan_channels_list);

        req->scan_duration = ZB_DEFAULT_SCAN_DURATION;
        TRACE_MSG(TRACE_APS1, "###disc, then join by assoc", (FMT__0));
        ret = zb_schedule_callback(zb_nlme_network_discovery_request, buf);
    }
}


static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "###buffer_test_cb %hd", (FMT__H, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "###status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "###status ERROR", (FMT__0));
    }
}


static void send_data(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_buffer_test_req_param_t *req_param;
    zb_address_ieee_ref_t addr_ref;

    TRACE_MSG(TRACE_APS1, "###send_data to coordinator", (FMT__H, param));
    if (!buf)
    {
        TRACE_MSG(TRACE_APS1, "###send_data: error - unble to get data buffer", (FMT__0));
        return;
    }
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->len = 0x0A;
    req_param->dst_addr = 0x0000;
    req_param->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

    /* ZC address has been deleted from addr map; add it again to be able to process
       apsde.data_request (not assert in APS) */
    zb_address_by_short(0x0000, /* create */ ZB_TRUE, /* lock */ ZB_FALSE, &addr_ref);
    zb_tp_buffer_test_request(buf, buffer_test_cb);
}

/*! @} */
