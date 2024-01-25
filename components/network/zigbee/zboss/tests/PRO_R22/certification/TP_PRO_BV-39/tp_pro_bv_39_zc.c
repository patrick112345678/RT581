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
/* PURPOSE:
*/



#define ZB_TEST_NAME TP_PRO_BV_39_ZC
#define ZB_TRACE_FILE_ID 40509

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define TEST_BUFFER_LEN 0x0010
#define TEST_NEW_CHANNEL 13
#define TEST_SCAN_DURATION 0xFE
#define TEST_SCAN_COUNT 0x01
#define TEST_SCAN_CHANNELS_MASK ZB_MAC_ALL_CHANNELS_MASK
#define TEST_TIMEOUT_SEND_UPDATE_REQ (20 * ZB_TIME_ONE_SECOND)

static void mgmt_nwk_update(zb_uint8_t param);
static void mgmt_nwk_update_cb(zb_uint8_t param);
static void buffer_test_cb(zb_uint8_t param);
static void send_data(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t r_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static const zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                        };


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_pan_id(0x1aaa);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);
    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    /* accept only one child */
    zb_set_max_children(1);

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


static void mgmt_nwk_update_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_mgmt_nwk_update_notify_hdr_t *notify_resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zdp_cmd;

    if (notify_resp->status == ZB_NWK_STATUS_INVALID_REQUEST)
    {
        TRACE_MSG(TRACE_APS2, "mgmt_nwk_update_cb: mgmt_nwk_update_notify received, invalid status - Ok", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_nwk_update_cb: mgmt_nwk_update_notify received, ERROR incorrect status %hd", (FMT__D, notify_resp->status));
    }
}


static void mgmt_nwk_update(zb_uint8_t param)
{
    zb_bufid_t buf = 0;
    zb_uint8_t curr_chan = zb_get_current_channel();
    zb_uint8_t new_chan = TEST_NEW_CHANNEL;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, "###mgmt_nwk_update: %hd; current chan = %hd; new chan = %hd;", (FMT__H_H_H, param, curr_chan, new_chan));
    buf = zb_buf_get_out();
    if (buf)
    {
        zb_zdo_mgmt_nwk_update_req_t *req;
        req = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_nwk_update_req_t);

        /*TODO: different than the one current network is on */
        req->hdr.scan_channels = ( 1l << new_chan );
        req->hdr.scan_duration = TEST_SCAN_DURATION;
        req->scan_count        = TEST_SCAN_COUNT;
        req->dst_addr          = /* zb_address_short_by_ieee(r_ieee_addr) */ ZB_NWK_BROADCAST_ALL_DEVICES;

        zb_zdo_mgmt_nwk_update_req(buf, mgmt_nwk_update_cb);

        /* Should change the channel manually, is it correct? */
        ZB_SCHEDULE_ALARM(
            zb_zdo_do_set_channel,
            new_chan,
            ZB_NWK_OCTETS_TO_BI(
                ZB_NWK_JITTER(
                    ZB_NWK_MAX_BROADCAST_RETRIES * ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
    }
}


static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "###buffer_test_cb: %hd", (FMT__H, param));

    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
    }

    /* param is not the buffer, so we do NOT need to free it */
}


static void send_data(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_buffer_test_req_param_t *req_param;

    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, ">>send_data", (FMT__0));

    if (buf)
    {
        req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
        BUFFER_TEST_REQ_SET_DEFAULT(req_param);

        req_param->len      = TEST_BUFFER_LEN;
        req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t *) r_ieee_addr);

        zb_tp_buffer_test_request(buf, buffer_test_cb);
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_APS1, "<<send_data", (FMT__0));
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

            ZB_SCHEDULE_ALARM(mgmt_nwk_update, 0, TEST_TIMEOUT_SEND_UPDATE_REQ);

            TRACE_MSG(TRACE_APS2, "###ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS = %hd;", (FMT__H, ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS));

            ZB_SCHEDULE_ALARM(send_data,
                              0,
                              TEST_TIMEOUT_SEND_UPDATE_REQ +
                              ZB_NWK_OCTETS_TO_BI(ZB_NWK_MAX_BROADCAST_RETRIES *
                                                  ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS) +
                              5 * ZB_TIME_ONE_SECOND);
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
        TRACE_MSG(TRACE_ERROR, "Device START FAILED status %d", (FMT__D, status));
    }

    zb_buf_free(param);
}
