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
/* PURPOSE: Test for ZC application written using ZDO.
*/


#define ZB_TEST_NAME TP_PRO_BV_2930_ZR2
#define ZB_TRACE_FILE_ID 40647

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

/*
  ZR joins to ZC, then sends APS packet.
 */

static const zb_ieee_addr_t g_zr2_addr = IEEE_ADDR_ZR2;
static zb_ieee_addr_t g_zc_addr = IEEE_ADDR_ZC;
static zb_ieee_addr_t g_zc2_addr = IEEE_ADDR_ZC2;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_3_zr2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_zr2_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

    MAC_ADD_VISIBLE_LONG(g_zc_addr);
    MAC_ADD_VISIBLE_LONG(g_zc2_addr);

    zb_set_max_children(0);

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
    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            zb_enable_auto_pan_id_conflict_resolution(ZB_TRUE);
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            break;

        case ZB_NLME_STATUS_INDICATION:
        {
            zb_zdo_signal_nlme_status_indication_params_t *params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_nlme_status_indication_params_t);

            TRACE_MSG(TRACE_ZDO1, "NLME status indication: status 0x%hx address 0x%x",
                      (FMT__H_D, params->nlme_status.status, params->nlme_status.network_addr));
        }
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


/*! @} */
