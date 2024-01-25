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
/* PURPOSE: TP/APS/BV-18-I Group Management-Group Remove All
*/

#define ZB_TEST_NAME TP_APS_BV_18_I_ZR
#define ZB_TRACE_FILE_ID 40689

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#define TEST_GROUP 2

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

static zb_uint8_t test_data_indication_cb(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_zr");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr);
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();

    ZB_CERT_HACKS().allow_entry_for_unregistered_ep = 1;
    /* zb_cert_test_set_security_level(0); */

    zb_af_set_data_indication(test_data_indication_cb);

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


static void remove_groups(zb_uint8_t param)
{
    zb_apsme_remove_all_groups_req_t *req_remove_param = ZB_BUF_GET_PARAM(param, zb_apsme_remove_all_groups_req_t);

    TRACE_MSG(TRACE_APP1, ">> remove_groups param %hd", (FMT__H, param));

    ZB_BZERO(req_remove_param, sizeof(*req_remove_param));

    req_remove_param->endpoint = ZB_TEST_PROFILE_EP;
    zb_apsme_remove_all_groups_request(param);

    TRACE_MSG(TRACE_APP1, "<< remove_groups", (FMT__0));
}


static void dummy_handler(zb_uint8_t unused)
{
    ZVUNUSED(unused);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);
    zb_bufid_t buf;
    zb_apsme_add_group_req_t *req_param;
    zb_uint16_t i = 0;

    TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        {
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

            buf = param;

            if (buf)
            {
                req_param = NULL;
                req_param = ZB_BUF_GET_PARAM(buf, zb_apsme_add_group_req_t);

                /* Need to disable ZB_ENABLE_ZCL to be able add entry
                 * in group table with nonregistered endpoint id
                 */

                for (i = 0; i < TEST_GROUP; ++i)
                {
                    req_param->endpoint = ZB_TEST_PROFILE_EP;
                    req_param->group_address = i + 1;

                    /* Register dummy callback just to do not free
                     * the buffer after zb_apsme_add_group_request().
                     */
                    req_param->confirm_cb = dummy_handler;

                    zb_apsme_add_group_request(buf);

                    TRACE_MSG(TRACE_APS1,
                              "zb_apsme_add_group_request: group_addr = %hd;",
                              (FMT__H, req_param->group_address));
                }
            }
            else
            {
                TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
                ZB_ASSERT(ZB_FALSE);
            }

            break;
        }

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


static zb_uint8_t test_data_indication_cb(zb_uint8_t param)
{
    zb_apsde_data_indication_t *ind;

    TRACE_MSG(TRACE_APP1, ">> test_data_indication_cb, param %hd", (FMT__H, param));

    ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

    switch (ind->profileid)
    {
    case ZB_TEST_PROFILE_ID:
        TRACE_MSG(TRACE_APP1, "  Test Profile packet detected, counter %hd", (FMT__H, ind->aps_counter));

        if (ind->group_addr == TEST_GROUP)
        {
            TRACE_MSG(TRACE_APP1, "  schedule groups removing", (FMT__0));
            zb_buf_get_out_delayed(remove_groups);
        }
        break;

    default:
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< test_data_indication_cb", (FMT__0));

    return ZB_FALSE;
}

