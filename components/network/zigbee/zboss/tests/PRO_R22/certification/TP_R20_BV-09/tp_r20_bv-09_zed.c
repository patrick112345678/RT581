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
/* PURPOSE: ZED
*/

#define ZB_TEST_NAME TP_R20_BV_09_ZED1
#define ZB_TRACE_FILE_ID 40627

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_WITH_EMBER_COORDINATOR

//#define TEST_CHANNEL (1l << 24)

#ifdef TEST_WITH_EMBER_COORDINATOR
/* Ember EM-ISA3-01 */
static zb_ieee_addr_t g_ieee_addr_c = {0xf0, 0x23, 0x07, 0x00, 0x00, 0xed, 0x21, 0x00};
#else
static zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#endif

static zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static zb_uint8_t g_key_c[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

static zb_uint8_t rejoin_done = 0;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_3_zed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();

    zb_set_rx_on_when_idle(ZB_TRUE);
    zb_bdb_set_legacy_device_support(ZB_TRUE);
    //zb_aps_set_global_tc_key_type(ZB_FALSE);

    MAC_ADD_INVISIBLE_SHORT(0);   /* do not see ZC */

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);
#if 0
    ZB_AIB().enable_alldoors_key = ZB_TRUE; /* ??? */
#endif

    zb_set_nvram_erase_at_start(ZB_TRUE);
    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zb_secur_update_key_pair(g_ieee_addr_c,
                                 g_key_c,
                                 ZB_SECUR_GLOBAL_KEY,
                                 ZB_SECUR_VERIFIED_KEY,
                                 ZB_SECUR_KEY_SRC_UNKNOWN);
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


static void rejoin_me(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_ext_pan_id_t extended_pan_id;
    zb_channel_list_t channel_list;

    ZVUNUSED(param);

    if (buf)
    {
        zb_get_extended_pan_id(extended_pan_id);
        zb_aib_get_channel_page_list(channel_list);

        zdo_initiate_rejoin(buf, extended_pan_id, channel_list, ZB_TRUE);
        rejoin_done = 1;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get buf!", (FMT__0));
    }
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

            if (!rejoin_done)
            {
                ZB_SCHEDULE_ALARM(rejoin_me, 0, 3 * ZB_TIME_ONE_SECOND);
            }
            break;

        case ZB_COMMON_SIGNAL_CAN_SLEEP:
#ifdef ZB_USE_SLEEP
            zb_sleep_now();
#endif /* ZB_USE_SLEEP */
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

    if (param)
    {
        zb_buf_free(param);
    }
}
