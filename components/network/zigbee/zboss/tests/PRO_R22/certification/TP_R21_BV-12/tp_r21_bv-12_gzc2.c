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

#define ZB_TEST_NAME TP_R21_BV_12_GZC2
#define ZB_TRACE_FILE_ID 40528

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

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_dutzr = IEEE_ADDR_DUT_ZR;
static const zb_ieee_addr_t g_ieee_addr_dutzed = IEEE_ADDR_DUT_ZED;
static const zb_ieee_addr_t g_ieee_addr_gzc2 = IEEE_ADDR_gZC2;

/* At start association is enabled */
static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void test_close_association_at_start(zb_uint8_t param);
static void test_open_association(zb_uint8_t param);
static void test_close_association(zb_uint8_t param);
static void test_toggle_permit_join(zb_uint8_t param);


static zb_uint8_t s_join_duration = 0xff; /* association allowed */
static zb_uint16_t s_child_zr_addr;
static zb_uint16_t s_child_zed_addr;
static zb_bool_t s_device_started = ZB_FALSE;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_2_gzc2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_gzc2);
    zb_set_use_extended_pan_id(g_ext_pan_id2);

    zb_set_pan_id(g_pan_id2);
    /* let's always be coordinator */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();
    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key_gzc2, 0);
    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            if (s_device_started == ZB_FALSE)
            {
                /* previnting multiple calls of this code after devices has been started */
                s_device_started = ZB_TRUE;

                TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

                zb_zdo_register_device_annce_cb(dev_annce_cb);

                ZB_SCHEDULE_CALLBACK(test_close_association_at_start, 0);
                TRACE_MSG(TRACE_ERROR, "will open 30s later", (FMT__0));
                ZB_SCHEDULE_ALARM(test_open_association, 0, 30 * ZB_TIME_ONE_SECOND);
                //ZB_SCHEDULE_ALARM(test_open_association, 0, GZC2_OPEN_NETWORK_DELAY);
            }
            break;

        case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
            if (s_join_duration)
            {
                ZB_SCHEDULE_CALLBACK(test_close_association, 0);
            }

            ZB_SCHEDULE_ALARM_CANCEL(test_open_association, ZB_ALARM_ANY_PARAM);
            ZB_SCHEDULE_ALARM(test_open_association, 0, GZC1_REOPEN_NETWORK_DELAY);

#ifdef ZB_USE_SLEEP
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            zb_sleep_now();
            break;
#endif /* ZB_USE_SLEEP */

        default:
            TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status %hd signal %hd",
                  (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));
    }

    zb_buf_free(param);
}


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 "addr = %h",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dutzr, da->ieee_addr) == ZB_TRUE)
    {
        s_child_zr_addr = da->nwk_addr;
    }
    if (ZB_IEEE_ADDR_CMP(g_ieee_addr_dutzed, da->ieee_addr) == ZB_TRUE)
    {
        s_child_zed_addr = da->nwk_addr;
    }
    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void test_close_association_at_start(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(test_toggle_permit_join);
}


static void test_open_association(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(test_toggle_permit_join);
}


static void test_close_association(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(test_toggle_permit_join);
}


static void test_toggle_permit_join(zb_uint8_t param)
{
    zb_nlme_permit_joining_request_t *req;

    TRACE_MSG(TRACE_APS1, ">>test_toggle_permit_join", (FMT__0));

    s_join_duration = (s_join_duration) ? 0 : 0xfe;
    req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));
    req->permit_duration = s_join_duration;

    TRACE_MSG(TRACE_APS1, "Toggling permit join on ZC, buf = %d, join duration = %d",
              (FMT__D_D, param, s_join_duration));

    zb_nlme_permit_joining_request(param);

    TRACE_MSG(TRACE_APS1, "<<test_toggle_permit_join", (FMT__0));
}

#if 0
static void test_send_nwk_leave(zb_ieee_addr_t *ieee_addr, zb_bufid_t buf)
{
    zb_nlme_leave_request_t *req;

    TRACE_MSG(TRACE_APS1, ">>test_send_nwk_leave, device = " TRACE_FORMAT_64 "buf = %h",
              (FMT__A_H, TRACE_ARG_64(ieee_addr), buf));

    req = zb_buf_get_tail(buf, sizeof(zb_nlme_leave_request_t));
    ZB_IEEE_ADDR_COPY(req->device_address, ieee_addr);
    req->remove_children = ZB_FALSE;
    req->rejoin = ZB_FALSE;
    zb_nlme_leave_request(buf);

    TRACE_MSG(TRACE_APS1, ">>test_send_nwk_leave", (FMT__0));
}
#endif


/*! @} */


/*! @} */

