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
/* PURPOSE: ZR
*/

#define ZB_TEST_NAME TP_PRO_BV_04_ZR1
#define ZB_TRACE_FILE_ID 40578

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_nwk_neighbor.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_2_zr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_r1);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */
    zb_set_max_children(0);

    ZB_CERT_HACKS().disable_in_out_cost_updating = 1;
    ZB_CERT_HACKS().delay_pending_tx_on_rresp = 0;
    ZB_CERT_HACKS().use_route_for_neighbor = 1;

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

static void change_link_status(zb_uint8_t param)
{
    zb_neighbor_tbl_ent_t *nbt;
    ZVUNUSED(param);

    if (RET_OK == zb_nwk_neighbor_get_by_short(0, &nbt))
    {
        nbt->u.base.age = 0;
        nbt->u.base.outgoing_cost = 3;
        nbt->lqi = NWK_COST_TO_LQI(3);
        TRACE_MSG(TRACE_ZDO1, "ZC lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(3)));
    }

    if (RET_OK == zb_nwk_neighbor_get_by_ieee((zb_uint8_t *)g_ieee_addr_r2, &nbt))
    {
        nbt->u.base.age = 0;
        nbt->u.base.outgoing_cost = 1;
        nbt->lqi = NWK_COST_TO_LQI(1);
        TRACE_MSG(TRACE_ZDO1, "ZC lqi was %hd. Modify outgoing_cost to %hd, lqi %hd", (FMT__H_H_H, nbt->lqi, nbt->u.base.outgoing_cost, NWK_COST_TO_LQI(1)));
    }

    ZB_SCHEDULE_ALARM(change_link_status, 0, 1 * ZB_TIME_ONE_SECOND);
}
/*
static void send_round_request(zb_uint8_t param)
{
  zb_nlme_route_discovery_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_route_discovery_request_t);

  TRACE_MSG(TRACE_ERROR, ">> send_round_request", (FMT__0));

  req->address_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  req->network_addr = zb_address_short_by_ieee(g_ieee_addr_ed);
  req->radius = 3;
  req->no_route_cache = ZB_FALSE;

  TRACE_MSG(TRACE_ERROR, "<< send_round_request addr = 0x%x", (FMT__D, req->network_addr));

  ZB_SCHEDULE_CALLBACK(zb_nlme_route_discovery_request, param);
}
*/

#if 0
static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
    }
}

static void send_buffer_test_request(zb_uint8_t param)
{
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_ERROR, "send_data", (FMT__0));

    req_param = ZB_BUF_GET_PARAM(param, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t *)g_ieee_addr_ed);

    TRACE_MSG(TRACE_ERROR, "send to 0x%x", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(param, buffer_test_cb);
}
#endif

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, status, sig));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            ZB_SCHEDULE_CALLBACK(change_link_status, 0);

            /* send data to zed1 to force route discovery from r1 to zed1 */
            //        ZB_SCHEDULE_ALARM(send_buffer_test_request/*send_round_request*/, param, 30*ZB_TIME_ONE_SECOND);
            break;

        default:
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }

}

