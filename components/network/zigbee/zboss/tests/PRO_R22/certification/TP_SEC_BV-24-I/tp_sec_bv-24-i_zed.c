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
/* PURPOSE: 11.24 TP/SEC/BV-24-I Security NWK Key Swith (No Preconfigured Key)-ZR
Objective: DUT as ZR issues new NWK Key and implements a SWITCH
*/

#define ZB_TEST_NAME TP_SEC_BV_24_I_ZED
#define ZB_TRACE_FILE_ID 40790
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;

static zb_bool_t is_first_start = ZB_TRUE;

static void start_fixed_poll(zb_uint8_t unused);
static void rejoin_me(zb_uint8_t param);
static void send_request_key(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRAF_DUMP_ON();

    ZB_INIT("zdo_3_zed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr_ed1);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();

    MAC_ADD_VISIBLE_LONG((zb_uint8_t *) g_ieee_addr_r1);
    MAC_ADD_INVISIBLE_SHORT(0);
    /* become an ED */
    zb_set_rx_on_when_idle(ZB_FALSE);
    zb_set_ed_timeout(ED_AGING_TIMEOUT_4MIN);

    zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZED1);

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

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
        if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
        {
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            if (is_first_start)
            {
                ZB_SCHEDULE_ALARM(send_request_key, 0, ZB_TIME_ONE_SECOND);
                is_first_start = ZB_FALSE;

                ZB_SCHEDULE_ALARM(start_fixed_poll, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));

                test_step_register(rejoin_me, 0, TP_SEC_BV_24_I_STEP_6_TIME_ZED);

                test_control_start(TEST_MODE, TP_SEC_BV_24_I_STEP_6_DELAY_ZED);
            }
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
        if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
        {
            TRACE_MSG(TRACE_APS1, "zboss_signal_handler: status OK, status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status FAILED, status %d",
                      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
        }
        break;
    }

    zb_buf_free(param);
}


static void start_fixed_poll(zb_uint8_t unused)
{
    TRACE_MSG(TRACE_ZDO1, ">>start_fixed_poll", (FMT__0));

    ZVUNUSED(unused);

#ifndef NCP_MODE_HOST
    ZDO_CTX().pim.poll_in_progress = ZB_FALSE;
#else
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif

    zb_zdo_pim_stop_poll(0);
    zb_zdo_pim_set_long_poll_interval(TIME_ZED_POLL_TIMEOUT_MS);
    zb_zdo_pim_permit_turbo_poll(ZB_FALSE); /* prohibit adaptive poll */
    zb_zdo_pim_start_poll(0);
    TRACE_MSG(TRACE_ZDO1, "<<start_fixed_poll", (FMT__0));
}

static void rejoin_me(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    if (buf)
    {
#ifndef NCP_MODE_HOST
        ZG->nwk.handle.tmp.rejoin.unsecured_rejoin = 0;
        ZG->aps.authenticated = ZB_FALSE;
        zb_secur_rejoin_after_security_failure(buf);
#else
        ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get buf!", (FMT__0));
    }
}

static void send_request_key(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();

    ZVUNUSED(param);

    TRACE_MSG(TRACE_ERROR, ">>send_request_key", (FMT__0));

    if (buf)
    {
        zb_apsme_request_key_req_t *req_param;

        req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_request_key_req_t);

        ZB_IEEE_ADDR_COPY(req_param->dest_address, &g_ieee_addr_c);
        req_param->key_type = ZB_REQUEST_TC_LINK_KEY;

        zb_secur_apsme_request_key(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
    }

    TRACE_MSG(TRACE_ERROR, "<<send_request_key", (FMT__0));
}
