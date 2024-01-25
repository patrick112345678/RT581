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
/* PURPOSE: 11.10 TP/ZDO/BV-10: ZC-ZDO-Receive Service Discovery
The DUT as ZigBee coordinator shall respond to an optional service discovery requests from a remote node.
*/

#define ZB_TEST_NAME TP_ZDO_BV_10_GZED1
#define ZB_TRACE_FILE_ID 40660

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_gzed1 = IEEE_ADDR_gZED1;

/* allocates buffer for next out command */
static void buffer_manager(zb_uint8_t param);
static void send_aps_packet(zb_uint8_t param);
static void complex_desc_req(zb_uint8_t param);
static void user_desc_req(zb_uint8_t param);
static void user_desc_set(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_gzed1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_gzed1);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_set_rx_on_when_idle(ZB_FALSE);

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


static void buffer_manager(zb_uint8_t param)
{
    zb_callback_t call_func = 0;

    switch (param)
    {
    case STEP_ZED1_COMPLEX_DESCR_REQ:
        call_func = complex_desc_req;
        break;
    case STEP_ZED1_USER_DESCR_REQ1:
        call_func = user_desc_req;
        break;
    case STEP_ZED1_DISCOVERY_REG_REQ:
        /* This step is omitted: Discovery Register Request is undefined */
        break;
    case STEP_ZED1_USER_DESCR_SET:
        call_func = user_desc_set;
        break;
    case STEP_ZED1_USER_DESCR_REQ2:
        call_func = user_desc_req;
        break;
    }

    if (call_func)
    {
        zb_buf_get_out_delayed(call_func);
    }
}


static void send_aps_packet(zb_bufid_t buf)
{
    zb_uint16_t req_clid = *ZB_BUF_GET_PARAM(buf, zb_uint16_t);
    zb_apsde_data_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);

    req->dst_addr.addr_short = 0x0000;
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 5;
    req->profileid = 0;
    req->clusterid = req_clid;
    req->src_endpoint = 0;
    req->dst_endpoint = 0;
    zb_buf_set_handle(buf, buf);

    TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
}


static void complex_desc_req(zb_bufid_t buf)
{
    zb_uint8_t *p_byte;
    zb_uint16_t *p_word;

    TRACE_MSG(TRACE_ZDO3, "TEST: complex_desc_req - nwk_addr = %h", (FMT__H, 0x0000));

    p_byte = zb_buf_initial_alloc(buf, 3 * sizeof(zb_uint8_t));
    *p_byte++ = zb_cert_test_inc_zdo_tsn();;
    p_byte = zb_put_next_htole16(p_byte, 0x0000);

    p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
    *p_word = TEST_ZDO_COMPLEX_DESCR_REQ_CLID;
    send_aps_packet(buf);
}


static void user_desc_req(zb_bufid_t buf)
{
    zb_uint8_t *p_byte;
    zb_uint16_t *p_word;

    TRACE_MSG(TRACE_ZDO3, "TEST: user_desc_req - nwk_addr = %h", (FMT__H, 0x0000));

    p_byte = zb_buf_initial_alloc(buf, 3 * sizeof(zb_uint8_t));
    *p_byte++ = zb_cert_test_inc_zdo_tsn();;
    p_byte = zb_put_next_htole16(p_byte, 0x0000);

    p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
    *p_word = TEST_ZDO_USER_DESCR_REQ_CLID;
    send_aps_packet(buf);
}


static void user_desc_set(zb_bufid_t buf)
{
    zb_uint8_t *p_byte;
    zb_uint16_t *p_word;
    zb_uint8_t desc_payload[] = "Dummy Text";

    TRACE_MSG(TRACE_ZDO3, "TEST: user_desc_set - nwk_addr = %h, length = %d",
              (FMT__H_D, ZB_PIBCACHE_NETWORK_ADDRESS(), sizeof(desc_payload)));

    /* tsn + nwk_addr + length + sizeof("Dummy Text") - sizeof(zero-terminated byte) */
    p_byte = zb_buf_initial_alloc(buf, 3 * sizeof(zb_uint8_t) + sizeof(desc_payload));
    *p_byte++ = zb_cert_test_inc_zdo_tsn();;
    p_byte = zb_put_next_htole16(p_byte, 0x0000);
    *p_byte++ = sizeof(desc_payload) - sizeof(zb_uint8_t);
    ZB_MEMCPY(p_byte, desc_payload, sizeof(desc_payload) - sizeof(zb_uint8_t));

    p_word = ZB_BUF_GET_PARAM(buf, zb_uint16_t);
    *p_word = TEST_ZDO_USER_DESCR_SET_CLID;

    send_aps_packet(buf);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(buffer_manager, STEP_ZED1_COMPLEX_DESCR_REQ, TIME_ZED1_COMPLEX_DESCR_REQ);
            ZB_SCHEDULE_ALARM(buffer_manager, STEP_ZED1_USER_DESCR_REQ1,   TIME_ZED1_USER_DESCR_REQ1);
            ZB_SCHEDULE_ALARM(buffer_manager, STEP_ZED1_DISCOVERY_REG_REQ, TIME_ZED1_DISCOVERY_REG_REQ);
            ZB_SCHEDULE_ALARM(buffer_manager, STEP_ZED1_USER_DESCR_SET,    TIME_ZED1_USER_DESCR_SET);
            ZB_SCHEDULE_ALARM(buffer_manager, STEP_ZED1_USER_DESCR_REQ2,   TIME_ZED1_USER_DESCR_REQ2);
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
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
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}

