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
/* PURPOSE: ZC
*/

#define ZB_TEST_NAME TP_PED_10_ZC
#define ZB_TRACE_FILE_ID 40641

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;

static void send_mgmt_lqi_req_delayed(zb_uint8_t param);
static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp(zb_uint8_t param);

static zb_uint8_t s_mgmt_lqi_start_index = 0;
static zb_uint16_t s_dutzr_short_addr = 0x0000;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_1_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_c);

    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_set_max_children(3);

    zb_set_use_extended_pan_id(g_ext_pan_id);
    zb_set_pan_id(g_pan_id);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key0, 0);
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key1, 1);

    /* turn off security */
    zb_cert_test_set_security_level(SECURITY_LEVEL);

    zb_bdb_set_legacy_device_support(ZB_TRUE);
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
    zb_zdo_app_signal_hdr_t *sig_handler = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sig_handler);

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (status == 0)
        {
            TRACE_MSG(TRACE_APP1, "Device STARTED OK: %d", (FMT__D, ZB_ZDO_SIGNAL_DEVICE_UPDATE));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
    {
        zb_zdo_signal_device_update_params_t *update_params =
            ZB_ZDO_SIGNAL_GET_PARAMS(
                sig_handler,
                zb_zdo_signal_device_update_params_t);

        TRACE_MSG(TRACE_APP1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = %d",
                  (FMT__A_D, TRACE_ARG_64(update_params->long_addr), update_params->status));

        if ((ZB_IEEE_ADDR_CMP(g_ieee_addr_dutzr, update_params->long_addr) == ZB_TRUE) &&
                (update_params->status == 0))
        {
            TRACE_MSG(TRACE_APP1, "Schedule mgmt_lqi_req", (FMT__0));

            /* send a mgnt_lqi_req */
            ZB_SCHEDULE_ALARM(send_mgmt_lqi_req_delayed, 0, TEST_CHECK_DUTZR_NBT_DELAY);
        }

        break;
    }

#ifdef ZB_USE_SLEEP
    case ZB_COMMON_SIGNAL_CAN_SLEEP:
        zb_sleep_now();
        break;
#endif /* ZB_USE_SLEEP */

    default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal: status %hd signal %hd",
                  (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));
        break;
    }

    zb_buf_free(param);
}

static void send_mgmt_lqi_req_delayed(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(send_mgmt_lqi_req);
}


static void send_mgmt_lqi_req(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

    s_dutzr_short_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_dutzr);

    if (s_dutzr_short_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
        req_param->dst_addr = s_dutzr_short_addr;
        req_param->start_index = s_mgmt_lqi_start_index;

        zb_zdo_mgmt_lqi_req(param, mgmt_lqi_resp);
    }
    else
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp buf", (FMT__0));
}

static void mgmt_lqi_resp(zb_uint8_t param)
{
    zb_zdo_mgmt_lqi_resp_t *resp = (zb_zdo_mgmt_lqi_resp_t *) zb_buf_begin(param);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, param));

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        zb_uint8_t nbrs = resp->neighbor_table_list_count;

        s_mgmt_lqi_start_index += nbrs;
        if (s_mgmt_lqi_start_index < resp->neighbor_table_entries)
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved = %d, total = %d",
                      (FMT__D_D, nbrs, resp->neighbor_table_entries));
            zb_buf_get_out_delayed(send_mgmt_lqi_req);
        }
        else
        {
            TRACE_MSG(TRACE_ZDO2, "mgmt_lqi_resp: retrieved all entries - %d",
                      (FMT__D, resp->neighbor_table_entries));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "mgmt_lqi_resp: request failure with status = %d", (FMT__D, resp->status));
    }
    zb_buf_free(param);

    TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_resp", (FMT__0));
}

