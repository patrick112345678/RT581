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
/* PURPOSE: TP/PRO/BV-37 Frequency Agility - Receipt of a
Mgmt_NWK_Update_req - ZR, coordinator side
*/

#define ZB_TEST_NAME TP_PRO_BV_37_ZC
#define ZB_TRACE_FILE_ID 40606

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

#define TEST_SCAN_DURATION       5
#define TEST_SCAN_DURATION_ERROR 6
#define TEST_SCAN_COUNT          1

#define SEND_TMOUT ZB_MILLISECONDS_TO_BEACON_INTERVAL(40000)

static const zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static const zb_ieee_addr_t r_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_int_t g_errors = 0;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_pan_id(0x1aaa);

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

static void mgmt_nwk_update_ok_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_mgmt_nwk_update_notify_hdr_t *notify_resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zdp_cmd;

    TRACE_MSG(TRACE_APS3,
              "notify_resp status %hd, scanned_channels %x %x, total_transmissions %hd, "
              "transmission_failures %hd, scanned_channels_list_count %hd, buf len %hd",
              (FMT__H_D_D_H_H_H_H, notify_resp->status, (zb_uint16_t)notify_resp->scanned_channels,
               *((zb_uint16_t *)&notify_resp->scanned_channels + 1),
               notify_resp->total_transmissions, notify_resp->transmission_failures,
               notify_resp->scanned_channels_list_count, zb_buf_len(param)));

    if (notify_resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        TRACE_MSG(TRACE_APS2, "mgmt_nwk_update_notify received, Ok", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_nwk_update_notify received, ERROR incorrect status %x",
                  (FMT__D, notify_resp->status));
        g_errors++;
    }

    if (!g_errors)
    {
        TRACE_MSG(TRACE_APS2, "Test is finished, status: OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS2, "Test is finished, status: Failed, errors %d", (FMT__D, g_errors));
    }

    zb_buf_free(param);
}

static void mgmt_nwk_update_error_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_mgmt_nwk_update_notify_hdr_t *notify_resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zdp_cmd;
    zb_zdo_mgmt_nwk_update_req_t *req;

    if (notify_resp->status == ZB_NWK_STATUS_INVALID_REQUEST)
    {
        TRACE_MSG(TRACE_APS2, "mgmt_nwk_update_notify received, invalid status - Ok", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_nwk_update_notify received, ERROR incorrect status %x",
                  (FMT__D, notify_resp->status));
        g_errors++;
    }

    req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_req_t);

    req->hdr.scan_channels = ZB_MAC_ALL_CHANNELS_MASK;
    req->hdr.scan_duration = TEST_SCAN_DURATION;
    req->scan_count = TEST_SCAN_COUNT;

    req->dst_addr = zb_address_short_by_ieee((zb_uint8_t *) r_ieee_addr);

    zb_zdo_mgmt_nwk_update_req(param, mgmt_nwk_update_ok_cb);
}

static void mgmt_nwk_update_error(zb_uint8_t param)
{
    zb_bufid_t asdu;

    ZVUNUSED(param);

    asdu = zb_buf_get_out();
    if (!asdu)
    {
        TRACE_MSG(TRACE_ERROR, "out buf alloc failed!", (FMT__0));
    }
    else
    {
        zb_zdo_mgmt_nwk_update_req_t *req;

        req = ZB_BUF_GET_PARAM(asdu, zb_zdo_mgmt_nwk_update_req_t);

        req->hdr.scan_channels = ZB_MAC_ALL_CHANNELS_MASK;
        req->hdr.scan_duration = TEST_SCAN_DURATION_ERROR;
        req->scan_count = TEST_SCAN_COUNT;
        req->dst_addr = zb_address_short_by_ieee((zb_uint8_t *) r_ieee_addr);

        ZB_CERT_HACKS().zdo_mgmt_nwk_update_force_scan_count = 1;

        zb_zdo_mgmt_nwk_update_req(asdu, mgmt_nwk_update_error_cb);

        ZB_CERT_HACKS().zdo_mgmt_nwk_update_force_scan_count = 0;
    }

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

            zb_schedule_alarm(mgmt_nwk_update_error, 0, SEND_TMOUT);
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

    if (param)
    {
        zb_buf_free(param);
    }
}


