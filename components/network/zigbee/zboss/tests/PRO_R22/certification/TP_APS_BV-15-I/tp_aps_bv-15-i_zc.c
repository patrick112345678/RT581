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
/* PURPOSE: TP/APS/BV-15-I Group Management-Group Addition-Tx
*/

#define ZB_TEST_NAME TP_APS_BV_15_I_ZC
#define ZB_TRACE_FILE_ID 40713
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_aps_bv-15-i_common.h"
#include "../common/zb_cert_test_globals.h"

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static const zb_ieee_addr_t g_ieee_addr_r2 = IEEE_ADDR_R2;

static void send_data1(zb_uint8_t param);
static void send_data2(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;


    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zc_role();

    zb_set_pan_id(0x1aaa);
    zb_set_long_address(g_ieee_addr_c);
    ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);
    zb_set_max_children(2);

    /* zb_cert_test_set_security_level(0); */
    zb_bdb_set_legacy_device_support(ZB_TRUE);
    zb_set_nvram_erase_at_start(ZB_TRUE);
    if ( zboss_start() != RET_OK )
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


#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    zb_uint16_t res = (zb_uint16_t)~0;

    TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));

    if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_r1))
    {
        res = ZR1_SHORT_ADDR;
    }
    else if (ZB_IEEE_ADDR_CMP(ieee_addr, g_ieee_addr_r2))
    {
        res = ZR2_SHORT_ADDR;
    }

    TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, res));
    return res;
}
#endif


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
#ifdef ZB_PRO_ADDRESS_ASSIGNMENT_CB
            /* Setup address assignment callback */
            zb_nwk_set_address_assignment_cb(addr_ass_cb);
#endif
            /* 1st data transmission to zr1 */
            test_step_register(send_data1, 0, TP_APS_BV_15_I_STEP_1_TIME_ZC);
            /* 2nd data transmission to group 0x0001 0xF0 */
            test_step_register(send_data2, 0, TP_APS_BV_15_I_STEP_2_TIME_ZC);
            test_control_start(TEST_MODE, TP_APS_BV_15_I_STEP_1_DELAY_ZC);
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

static void buffer_test_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));
    if (param == ZB_TP_BUFFER_TEST_OK)
    {
        TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
    }
}

static void send_data1(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_buffer_test_req_param_t *req_param = NULL;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, ">>send_data1", (FMT__0));

    if (!buf)
    {
        TRACE_MSG(TRACE_APS1, "send_data1: error - unable to get data buffer", (FMT__0));
        ZB_EXIT(1);
    }

    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t *) g_ieee_addr_r1);

    zb_tp_buffer_test_request(buf, buffer_test_cb);
    TRACE_MSG(TRACE_APS1, "<<send_data1", (FMT__0));
}

static void send_data2(zb_uint8_t param)
{
    zb_bufid_t buf = zb_buf_get_out();
    zb_buffer_test_req_param_t *req_param;
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APS1, ">>send_data2", (FMT__0));

    if (!buf)
    {
        TRACE_MSG(TRACE_APS1, "send_data2: error - unable to get data buffer", (FMT__0));
        ZB_EXIT(1);
    }
    req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
    BUFFER_TEST_REQ_SET_DEFAULT(req_param);
    req_param->addr_mode = ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT;
    req_param->dst_addr = GROUP_ADDR;

    zb_tp_buffer_test_request(buf, buffer_test_cb);

    TRACE_MSG(TRACE_APS1, "<<send_data2", (FMT__0));
}

