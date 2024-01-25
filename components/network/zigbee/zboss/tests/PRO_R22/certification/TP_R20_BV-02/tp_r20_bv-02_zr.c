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


#define ZB_TEST_NAME TP_R20_BV_02_ZR1
#define ZB_TRACE_FILE_ID 40822
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "../common/zb_cert_test_globals.h"
#include "tp_r20_bv-02_common.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_ieee_addr_t g_ieee_addr_c = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_uint8_t g_key_c[16] = { 0x12, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_2_zr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_long_address(g_ieee_addr);

    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    zb_zdo_set_aps_unsecure_join(ZB_TRUE);

    zb_set_max_children(2);
    zb_bdb_set_legacy_device_support(ZB_TRUE);
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

#if 0
static void zb_send_test_update(zb_uint8_t param);
#endif

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
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
#if 0
            ZB_SCHEDULE_ALARM(zb_send_test_update, ZB_REF_FROM_BUF(zb_buf_get(ZB_TRUE, 0)), 40 * ZB_TIME_ONE_SECOND);
#endif
            ZB_CERT_HACKS().send_update_device_unencrypted = 1;
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


#if 0
static void zb_send_test_update(zb_uint8_t param)
{

    zb_apsme_update_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_update_device_req_t);
    zb_apsme_update_device_pkt_t *p;


    req->status = ZB_STD_SEQ_UNSECURED_JOIN;
    ZB_IEEE_ADDR_COPY(req->dest_address, ZB_AIB().trust_center_address);
    req->device_short_address = 0x1234;
    ZB_IEEE_ADDR_ZERO(req->device_address);


    ZB_BUF_INITIAL_ALLOC(param, sizeof(zb_apsme_update_device_pkt_t), p);

    ZB_IEEE_ADDR_COPY(p->device_address, req->device_address);
    ZB_HTOLE16(&p->device_short_address, &req->device_short_address);
    p->status = req->status;
    TRACE_MSG(TRACE_SECUR3, "send UPDATE-DEVICE " TRACE_FORMAT_64 " %d st %hd to " TRACE_FORMAT_64, (FMT__A_D_H, TRACE_ARG_64(p->device_address),     p->device_short_address, p->status, TRACE_ARG_64(req->dest_address)));

    zb_aps_send_command(param,
                        zb_address_short_by_ieee(req->dest_address),
                        APS_CMD_UPDATE_DEVICE, ZB_FALSE, 0xff);
}
#endif

/*! @} */
