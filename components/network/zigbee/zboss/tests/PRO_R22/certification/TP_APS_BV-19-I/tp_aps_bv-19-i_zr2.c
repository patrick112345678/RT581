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
/* PURPOSE: 11.19 TP/APS/BV-19-I Source Binding - Router. gZR2.

Objective:  Verify that DUT Router can store its own binding entries

*/

#define ZB_TEST_NAME TP_APS_BV_19_I_ZR2
#define ZB_TRACE_FILE_ID 40862

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

static zb_uint8_t data_indication(zb_uint8_t param);

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

//#define TEST_CHANNEL (1l << 24)


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zr2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    /* zb_cert_test_set_security_level(0); */

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
            zb_af_set_data_indication(data_indication);
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


static zb_uint8_t data_indication(zb_bufid_t asdu)
{
    zb_ushort_t i;
    zb_uint8_t *ptr;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(asdu, zb_apsde_data_indication_t);

    if (ind->profileid == ZB_AF_ZDO_PROFILE_ID ||
            ind->profileid == ZB_AF_HA_PROFILE_ID)
    {
        return ZB_FALSE;
    }

    /* Remove APS header from the packet */
    ptr = (zb_uint8_t *)zb_buf_begin(asdu);

    TRACE_MSG(TRACE_APS3, "###data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
              asdu, (int)zb_buf_len(asdu), zb_buf_get_status(asdu)));

    for (i = 0 ; i < zb_buf_len(asdu) ; ++i)
    {
        TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
        if (ptr[i] != i % 32 + '0')
        {
            TRACE_MSG(TRACE_ERROR, "###Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                      (int)(i % 32 + '0'), (char)i % 32 + '0'));
        }
    }

    zb_buf_free(asdu);
    return ZB_TRUE;
}

