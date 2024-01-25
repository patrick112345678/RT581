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
/* PURPOSE: DUT ZR
*/

#define ZB_TEST_NAME RTP_OSIF_01_DUT_ZR
#define ZB_TRACE_FILE_ID 40297

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_osif_01_common.h"
#include "../common/zb_reg_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error define ZB_USE_NVRAM
#endif

#if !defined ZB_NSNG && !defined ZB_NRF52_RADIO_STATISTICS
#error define ZB_NRF52_RADIO_STATISTICS
#endif

static zb_ieee_addr_t g_ieee_addr_dut_zr = IEEE_ADDR_DUT_ZR;

static void trigger_steering(zb_uint8_t unused);
static void send_buffer_test_request_delayed(zb_uint8_t unused);
static void send_buffer_test_request(zb_uint8_t param);
static void write_radio_stats_to_trace(zb_uint8_t unused);
static void dump_radio_stat(zb_uint8_t unused);

static zb_uint16_t g_remote_addr = 0;

#ifndef ZB_NSNG
static zb_nrf52_radio_stats_t g_test_stats;
#endif

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);

    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zr");

    zb_set_long_address(g_ieee_addr_dut_zr);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_router_role((1l << TEST_CHANNEL));
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

    MAIN_RETURN(0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(write_radio_stats_to_trace, 0, RTP_OSIF_01_STEP_1_TIME_ZR);
            test_step_register(send_buffer_test_request_delayed, 0, RTP_OSIF_01_STEP_2_TIME_ZR);
            test_step_register(write_radio_stats_to_trace, 0, RTP_OSIF_01_STEP_3_TIME_ZR);
            test_step_register(send_buffer_test_request_delayed, 0, RTP_OSIF_01_STEP_4_TIME_ZR);
            test_step_register(write_radio_stats_to_trace, 0, RTP_OSIF_01_STEP_5_TIME_ZR);

            /* This step is needed for CTH */
            test_step_register(write_radio_stats_to_trace, 0, RTP_OSIF_01_STEP_6_TIME_ZR);

            test_control_start(TEST_MODE, RTP_OSIF_01_STEP_1_DELAY_ZR);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */


    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

static void send_buffer_test_request_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    g_remote_addr = 0;
    zb_buf_get_out_delayed(send_buffer_test_request);
}

static void send_buffer_test_request(zb_uint8_t param)
{
    zb_bufid_t buf = param;
    zb_buffer_test_req_param_t *req_param;

    TRACE_MSG(TRACE_APP1, ">>send_buffer_test_request", (FMT__0));

    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = g_remote_addr;

    TRACE_MSG(TRACE_APP1, "send buffer test request to 0x%x", (FMT__H, req_param->dst_addr));

    zb_tp_buffer_test_request(buf, NULL);

    TRACE_MSG(TRACE_APP1, "<<send_buffer_test_request", (FMT__0));
}

static void write_radio_stats_to_trace(zb_uint8_t unused)
{
    ZVUNUSED(unused);

#ifndef ZB_NSNG
    zb_nrf52_radio_stats_t *stats;
    ZB_OSIF_GLOBAL_LOCK();
    stats = zb_nrf52_get_radio_stats();
    g_test_stats = *stats;
    ZB_OSIF_GLOBAL_UNLOCK();
#endif

    ZB_SCHEDULE_CALLBACK(dump_radio_stat, 0);
}

static void dump_radio_stat(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_APP1, "NRF52840 radio stats:", (FMT__0));
#ifndef ZB_NSNG
    TRACE_MSG(TRACE_APP1, "rx_successful=%ld", (FMT__L, g_test_stats.rx_successful));
    TRACE_MSG(TRACE_APP1, "rx_err_none=%ld", (FMT__L, g_test_stats.rx_err_none));
    TRACE_MSG(TRACE_APP1, "rx_err_invalid_frame=%ld", (FMT__L, g_test_stats.rx_err_invalid_frame));
    TRACE_MSG(TRACE_APP1, "rx_err_invalid_fcs=%ld", (FMT__L, g_test_stats.rx_err_invalid_fcs));
    TRACE_MSG(TRACE_APP1, "rx_err_invalid_dest_addr=%ld", (FMT__L, g_test_stats.rx_err_invalid_dest_addr));
    TRACE_MSG(TRACE_APP1, "rx_err_runtime=%ld", (FMT__L, g_test_stats.rx_err_runtime));
    TRACE_MSG(TRACE_APP1, "rx_err_timeslot_ended=%ld", (FMT__L, g_test_stats.rx_err_timeslot_ended));
    TRACE_MSG(TRACE_APP1, "rx_err_aborted=%ld", (FMT__L, g_test_stats.rx_err_aborted));
    TRACE_MSG(TRACE_APP1, "tx_successful=%ld", (FMT__L, g_test_stats.tx_successful));
    TRACE_MSG(TRACE_APP1, "tx_err_none=%ld", (FMT__L, g_test_stats.tx_err_none));
    TRACE_MSG(TRACE_APP1, "tx_err_busy_channel=%ld", (FMT__L, g_test_stats.tx_err_busy_channel));
    TRACE_MSG(TRACE_APP1, "tx_err_invalid_ack=%ld", (FMT__L, g_test_stats.tx_err_invalid_ack));
    TRACE_MSG(TRACE_APP1, "tx_err_no_mem=%ld", (FMT__L, g_test_stats.tx_err_no_mem));
    TRACE_MSG(TRACE_APP1, "tx_err_timeslot_ended=%ld", (FMT__L, g_test_stats.tx_err_timeslot_ended));
    TRACE_MSG(TRACE_APP1, "tx_err_no_ack=%ld", (FMT__L, g_test_stats.tx_err_no_ack));
    TRACE_MSG(TRACE_APP1, "tx_err_aborted=%ld", (FMT__L, g_test_stats.tx_err_aborted));
    TRACE_MSG(TRACE_APP1, "tx_err_timeslot_denied=%ld", (FMT__L, g_test_stats.tx_err_timeslot_denied));
#endif
}

/*! @} */
