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
/* PURPOSE: 11.19 TP/APS/BV-19-I Source Binding - Router

DUTZR1.
Objective:  Verify that DUT Router can store its own binding entries

*/

#define ZB_TEST_NAME TP_APS_BV_19_I_ZR1
#define ZB_TRACE_FILE_ID 40863

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

static void zb_test_bind(zb_uint8_t param);
static void zb_test_bind_cb(zb_uint8_t param);

static void send_data(zb_bufid_t buf);

#if 0
static void data_indication(zb_uint8_t param);
#endif

#define SEND_TMOUT ZB_MILLISECONDS_TO_BEACON_INTERVAL(60000)

#ifdef ZB_EMBER_TESTS
#define ZR2_EMBER
#define ZC_EMBER
#endif

static zb_ieee_addr_t g_ieee_addr   = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#ifdef ZR2_EMBER
static zb_ieee_addr_t g_ieee_addr_d = {0xEE, 0x23, 0x07, 0x00, 0x00, 0xED, 0x21, 0x00};
#else
static zb_ieee_addr_t g_ieee_addr_d = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
#endif

//#define TEST_CHANNEL (1l << 24)


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_set_max_children(0);

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


//! [tp_aps_bv_19_i_zr1]
static void zb_test_bind(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_apsme_binding_req_t *req;
    ZVUNUSED(param);

    req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);
    ZB_MEMCPY(&req->src_addr, &g_ieee_addr, sizeof(zb_ieee_addr_t));
    req->src_endpoint = 1;
    req->clusterid = 1;
    req->addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(&req->dst_addr.addr_long, &g_ieee_addr_d, sizeof(zb_ieee_addr_t));
    req->dst_endpoint = 0xF0;
    req->confirm_cb = zb_test_bind_cb;

    zb_apsme_bind_request(buf);
}

static void zb_test_bind_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, "-zb_apsme_bind_request status %d", (FMT__D, zb_buf_get_status(param)));
    if (zb_buf_get_status(param) != RET_OK)
    {
        TRACE_MSG(TRACE_APP1, "Failed to create binding!", (FMT__0));
    }
    zb_bufid_t buf = param;

    zb_buf_reuse(buf);
    send_data(buf);
}

//! [tp_aps_bv_19_i_zr1]


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(zb_test_bind, 0, SEND_TMOUT);
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
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    zb_buf_free(param);
}


static void send_data(zb_bufid_t buf)
{
    zb_apsde_data_req_t *req;
    zb_uint8_t *ptr = NULL;
    zb_short_t i;

    ptr = zb_buf_initial_alloc(buf, 70);
    req = zb_buf_get_tail(buf, sizeof(zb_apsde_data_req_t));
    req->addr_mode = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->clusterid = 1;
    req->radius = 1;
    req->profileid = 0;
    req->src_endpoint = 1;

    zb_buf_set_handle(buf, 0x11);

    for (i = 0 ; i < 70 ; ++i)
    {
        ptr[i] = i % 32 + '0';
    }
    TRACE_MSG(TRACE_APS3, "###Sending apsde_data.request", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
}

#if 0
//! [data_indication]
static void data_indication(zb_bufid_t asdu)
{
    zb_ushort_t i;
    zb_uint8_t *ptr;

    /* Remove APS header from the packet */
    ZB_APS_HDR_CUT_P(asdu, ptr);

    TRACE_MSG(TRACE_APS3, "###data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
              asdu, (int)zb_buf_len(asdu), zb_buf_get_status(asdu)));

    for (i = 0 ; i < zb_buf_len(asdu) ; ++i)
    {
        TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
        if (ptr[i] != i % 32 + '0')
        {
            TRACE_MSG(TRACE_ERROR, "###Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                      (int)(i % 32 + '0'), (char)i % 32 + '0'));
        }
    }
    zb_buf_free(asdu);
}
//! [data_indication]
#endif
