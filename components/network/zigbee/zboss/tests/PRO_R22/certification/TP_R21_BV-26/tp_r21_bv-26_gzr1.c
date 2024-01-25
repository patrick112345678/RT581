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
/* PURPOSE: TP_R21_BV-26 ZigBee Router (gZR1)
*/

#define ZB_TEST_NAME TP_R21_BV_26_GZR1
#define ZB_TRACE_FILE_ID 40501

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

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzr1 = IEEE_ADDR_gZR1;
static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void send_mgmt_lqi_req(zb_uint8_t param);
static void mgmt_lqi_resp(zb_uint8_t param);


static zb_uint8_t s_mgmt_lqi_start_index = 0;
static zb_uint16_t s_dut_short_addr = 0x0000;


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_3_gzr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzr1);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
    zb_set_max_children(0);

#ifdef SECURITY_LEVEL
    zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

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
            ZB_SCHEDULE_ALARM(send_mgmt_lqi_req, 0, TEST_RETRIEVE_DUT_NBT_DELAY);
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


static void send_mgmt_lqi_req(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    ZVUNUSED(param);

    TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_resp buf = %d", (FMT__D, buf));

    if (buf)
    {
        zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_lqi_param_t);

        s_dut_short_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_dut);

        if (s_dut_short_addr != ZB_UNKNOWN_SHORT_ADDR)
        {
            req_param->dst_addr = s_dut_short_addr;
            req_param->start_index = s_mgmt_lqi_start_index;

            zb_zdo_mgmt_lqi_req(buf, mgmt_lqi_resp);
        }
        else
        {
            zb_buf_free(buf);
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "TEST FAILED: Could not get out buf!", (FMT__0));
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

/*! @} */

