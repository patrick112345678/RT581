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
/* PURPOSE: TP/R21/BV-03 - Router (DUT ZR): forming Distributed Network
*/


#define ZB_TEST_NAME TP_R21_BV_03_DUTZR
#define ZB_TRACE_FILE_ID 40778

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


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static void open_network_delayed(zb_uint8_t unused);
static void open_network(zb_uint8_t param);
static void close_network_delayed(zb_uint8_t unused);
static void close_network(zb_uint8_t param);

static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_4_dutzr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_dutzr);
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    /* Not mandatory, but possible: set address */
    zb_cert_test_set_network_addr(0x2bbb);
    zb_set_pan_id(g_dutzr_pan_id);

    /* Set well-known nwk key set in wireshark (also optional) */
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key_zr, 0);

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

    zb_set_network_router_role_legacy(DUT_CHANNEL_MASK);

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

            ZB_SCHEDULE_CALLBACK(close_network_delayed, 0);
            test_step_register(open_network_delayed, 0, TP_R21_BV_03_STEP_4_TIME_DUTZR);

            test_control_start(TEST_MODE, TP_R21_BV_03_STEP_4_DELAY_DUTZR);
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

static void open_network_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    if (zb_buf_get_out_delayed(open_network) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "open_network_delayed: buffer allocation error", (FMT__0));
    }
}
static void open_network(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_nlme_permit_joining_request_t *req;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, ">>open_network", (FMT__0));

    TRACE_MSG(TRACE_APS1, "Closing permit, buf = %d",
              (FMT__D, buf));

    req = zb_buf_get_tail(buf, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 0xff;
    zb_nlme_permit_joining_request(buf);

    TRACE_MSG(TRACE_APS1, "<<open_network", (FMT__0));
}

static void close_network_delayed(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    if (zb_buf_get_out_delayed(close_network) != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "close_network_delayed: buffer allocation error", (FMT__0));
    }
}

static void close_network(zb_uint8_t param)
{
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_APS1, ">>close_network", (FMT__0));

    TRACE_MSG(TRACE_APS1, "Closing permit, buf = %d",
              (FMT__D, param));

    req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = 0;
    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_APS1, "<<close_network", (FMT__0));
}

/*! @} */
