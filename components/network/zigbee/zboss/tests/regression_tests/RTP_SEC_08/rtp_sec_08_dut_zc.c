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
/* PURPOSE: DUT ZC
*/
#define ZB_TEST_NAME RTP_SEC_08_DUT_ZC

#define ZB_TRACE_FILE_ID 40349
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_sec_08_common.h"
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

static void dispatch_test_procedure(zb_uint8_t param);
static void test_get_ic_list(zb_uint8_t param, zb_uint16_t start_index);
static void test_fill_ic_table(zb_uint8_t param);

typedef enum test_step_e
{
    TEST_STEP_REMOVE_NON_EXISTING_IC_BY_IDX,
    TEST_STEP_FILL_IC_TABLE,
    TEST_STEP_GET_ALL_IC_1,
    TEST_STEP_GET_IC_BY_IDX,
    TEST_STEP_REMOVE_IC_BY_IDX,
    TEST_STEP_GET_ALL_IC_2,
    TEST_STEP_REMOVE_ALL_IC,
    TEST_STEP_GET_ALL_IC_3,
    TEST_STEP_COMPLETE
} test_step_t;

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;

static zb_uint8_t g_ic[ZB_CCM_KEY_SIZE + ZB_CCM_KEY_CRC_SIZE] = ZB_REG_TEST_DEFAULT_INSTALL_CODE;

static zb_ieee_addr_t g_ic_devices[] =
{
    IEEE_ADDR_TH_ZR1,
    IEEE_ADDR_TH_ZR2,
    IEEE_ADDR_TH_ZR3,
    IEEE_ADDR_TH_ZR4,
    IEEE_ADDR_TH_ZR5
};

static zb_uindex_t g_current_ic_device_idx = 0;
static zb_uindex_t g_current_test_step = 0;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zc");

    zb_set_long_address(g_ieee_addr_dut_zc);

    zb_secur_setup_nwk_key((zb_uint8_t *) g_nwk_key, 0);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
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
    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(dispatch_test_procedure, 0, RTP_SEC_08_STEP_1_TIME_ZC);

            test_control_start(TEST_MODE, RTP_SEC_08_STEP_1_DELAY_ZC);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


/* Get installcode by index */
static void test_remove_all_ic_cb(zb_uint8_t param)
{
    zb_secur_ic_remove_all_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_resp_t);

    TRACE_MSG(TRACE_APP1, ">> test_remove_all_ic_cb, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_remove_all_ic_cb: status %d", (FMT__D, resp->status));

    ZB_SCHEDULE_APP_CALLBACK(dispatch_test_procedure, param);

    TRACE_MSG(TRACE_APP1, "<< test_remove_all_ic_cb, param %hd", (FMT__H, param));
}


static void test_remove_all_ic(zb_uint8_t param)
{
    zb_secur_ic_remove_all_req_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> test_remove_all_ic, param %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_req_t);
    req_param->response_cb = test_remove_all_ic_cb;

    zb_secur_ic_remove_all_req(param);

    TRACE_MSG(TRACE_APP1, "<< test_remove_all_ic", (FMT__0));
}


/* Get installcode by index */
static void test_get_ic_by_idx_cb(zb_uint8_t param)
{
    zb_secur_ic_get_by_idx_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_resp_t);

    TRACE_MSG(TRACE_APP1, ">> test_get_ic_by_idx_cb, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_get_ic_by_idx_cb: status %d", (FMT__D, resp->status));
    TRACE_MSG(TRACE_APP1, "  test_get_ic_by_idx_cb: ic " TRACE_FORMAT_128 ", device_address " TRACE_FORMAT_64,
              (FMT__B_A, TRACE_ARG_128(resp->installcode), TRACE_ARG_64(resp->device_address)));

    ZB_SCHEDULE_APP_CALLBACK(dispatch_test_procedure, param);

    TRACE_MSG(TRACE_APP1, "<< test_get_ic_by_idx_cb, param %hd", (FMT__H, param));
}


static void test_get_ic_by_idx(zb_uint8_t param, zb_uint16_t ic_index)
{
    zb_secur_ic_get_by_idx_req_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> test_get_ic_by_idx, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_get_ic_by_idx: ic_index %hd", (FMT__H, ic_index));

    req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_req_t);
    req_param->response_cb = test_get_ic_by_idx_cb;
    req_param->ic_index = ic_index;

    zb_secur_ic_get_by_idx_req(param);

    TRACE_MSG(TRACE_APP1, "<< test_get_ic_by_idx", (FMT__0));
}


/* Get installcodes list */
static void test_get_ic_list_cb(zb_uint8_t param)
{
    zb_secur_ic_get_list_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_resp_t);
    zb_uindex_t ic_entry_index = 0;
    zb_aps_installcode_nvram_t *ic_table = (zb_aps_installcode_nvram_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP1, ">> test_get_ic_list_cb, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: status %d", (FMT__D, resp->status));
    TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: ic_table_entries %hd", (FMT__H, resp->ic_table_entries));
    TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: start_index %hd", (FMT__H, resp->start_index));
    TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: ic_table_list_count %hd", (FMT__H, resp->ic_table_list_count));

    ZB_ASSERT(resp->status == RET_OK);

    for (ic_entry_index = 0; ic_entry_index < resp->ic_table_list_count; ic_entry_index++)
    {
        zb_aps_installcode_nvram_t *ic_entry = &ic_table[ic_entry_index];

        TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: entry %hd", (FMT__H, ic_entry_index));
        /* IC type is stored in two last bits of options field */
        TRACE_MSG(TRACE_APP1, "    ic_type: %hd", (FMT__H, ic_entry->options & 0x3));
        TRACE_MSG(TRACE_APP1, "    index %d, ic " TRACE_FORMAT_128 ", device_address " TRACE_FORMAT_64,
                  (FMT__D_B_A, ic_entry_index, TRACE_ARG_128(ic_entry->installcode), TRACE_ARG_64(ic_entry->device_address)));

        TRACE_MSG(TRACE_APP1, "  test_get_ic_list_cb: entry %hd", (FMT__H, ic_entry_index));
    }

    if (resp->start_index + resp->ic_table_list_count < resp->ic_table_entries)
    {
        ZB_SCHEDULE_APP_CALLBACK2(test_get_ic_list, param, resp->start_index + resp->ic_table_list_count);
    }
    else
    {
        ZB_SCHEDULE_APP_CALLBACK(dispatch_test_procedure, param);
    }

    TRACE_MSG(TRACE_APP1, "<< test_get_ic_list_cb, param %hd", (FMT__H, param));
}


static void test_get_ic_list(zb_uint8_t param, zb_uint16_t start_index)
{
    zb_secur_ic_get_list_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> test_get_ic_list, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_get_ic_list: start_idx %hd", (FMT__H, start_index));

    req = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_req_t);
    req->response_cb = test_get_ic_list_cb;
    req->start_index = start_index;

    zb_secur_ic_get_list_req(param);

    TRACE_MSG(TRACE_APP1, "<< test_get_ic_list", (FMT__0));
}

/* Add installcodes */
static void test_fill_ic_table_cb(zb_ret_t status)
{
    TRACE_MSG(TRACE_APP1, ">> test_fill_ic_table_cb", (FMT__0));

    TRACE_MSG(TRACE_APP1, "  test_fill_ic_table_cb: status %d", (FMT__D, status));

    g_current_ic_device_idx++;

    if (g_current_ic_device_idx == ZB_ARRAY_SIZE(g_ic_devices))
    {
        ZB_SCHEDULE_CALLBACK(dispatch_test_procedure, 0);
    }
    else
    {
        ZB_SCHEDULE_CALLBACK(test_fill_ic_table, 0);
    }

    TRACE_MSG(TRACE_APP1, "<< test_fill_ic_table_cb", (FMT__0));
}


static void test_fill_ic_table(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, ">> test_fill_ic_table, param %hd, idx %hd of %hd",
              (FMT__H_H_H, param, g_current_ic_device_idx, ZB_ARRAY_SIZE(g_ic_devices) - 1));

    zb_secur_ic_add(g_ic_devices[g_current_ic_device_idx], ZB_IC_TYPE_128, g_ic, test_fill_ic_table_cb);

    TRACE_MSG(TRACE_APP1, "<< test_fill_ic_table", (FMT__0));
}


/* Remove installcode */
static void test_remove_ic_by_device_idx_cb(zb_uint8_t param)
{
    zb_secur_ic_remove_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_resp_t);

    TRACE_MSG(TRACE_APP1, ">> test_remove_ic_by_device_idx_cb, param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APP1, "  test_remove_ic_by_device_idx_cb: status %d", (FMT__D, resp->status));

    ZB_SCHEDULE_APP_CALLBACK(dispatch_test_procedure, param);

    TRACE_MSG(TRACE_APP1, "<< test_remove_ic_by_device_idx_cb, param %hd", (FMT__H, param));
}


static void test_remove_ic_by_device_idx(zb_uint8_t param, zb_uint16_t device_index)
{
    zb_secur_ic_remove_req_t *req_param;

    TRACE_MSG(TRACE_APP1, ">> test_remove_ic_by_device_idx, param %hd", (FMT__H, param));

    req_param = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_req_t);
    req_param->response_cb = test_remove_ic_by_device_idx_cb;
    ZB_IEEE_ADDR_COPY(req_param->device_address, g_ic_devices[device_index]);
    zb_secur_ic_remove_req(param);

    TRACE_MSG(TRACE_APP1, "<< test_remove_ic_by_device_idx", (FMT__0));
}

/* Dispatch test procedure */
static void dispatch_test_procedure(zb_uint8_t param)
{
    if (param == 0)
    {
        zb_buf_get_out_delayed(dispatch_test_procedure);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> dispatch_test_procedure, param %hd, test_step %hd", (FMT__H_H, param, g_current_test_step));

    switch (g_current_test_step)
    {
    case TEST_STEP_REMOVE_NON_EXISTING_IC_BY_IDX:
        ZB_SCHEDULE_CALLBACK2(test_remove_ic_by_device_idx, param, 0);
        break; /* TEST_STEP_REMOVE_NON_EXISTING_IC_BY_IDX */

    case TEST_STEP_FILL_IC_TABLE:
        zb_buf_free(param);
        ZB_SCHEDULE_APP_CALLBACK(test_fill_ic_table, 0);
        break; /* TEST_STEP_FILL_IC_TABLE */

    case TEST_STEP_GET_ALL_IC_1:
        ZB_SCHEDULE_APP_CALLBACK2(test_get_ic_list, param, 0);
        break; /* TEST_STEP_GET_ALL_IC_1 */

    case TEST_STEP_GET_IC_BY_IDX:
        ZB_SCHEDULE_CALLBACK2(test_get_ic_by_idx, param, 0);
        break; /* TEST_STEP_GET_IC_BY_IDX */

    case TEST_STEP_REMOVE_IC_BY_IDX:
        ZB_SCHEDULE_CALLBACK2(test_remove_ic_by_device_idx, param, 0);
        break; /* TEST_STEP_REMOVE_IC_BY_IDX */

    case TEST_STEP_GET_ALL_IC_2:
        ZB_SCHEDULE_APP_CALLBACK2(test_get_ic_list, param, 0);
        break; /* TEST_STEP_GET_ALL_IC_2 */

    case TEST_STEP_REMOVE_ALL_IC:
        ZB_SCHEDULE_CALLBACK(test_remove_all_ic, param);
        break; /* TEST_STEP_REMOVE_ALL_IC */

    case TEST_STEP_GET_ALL_IC_3:
        ZB_SCHEDULE_APP_CALLBACK2(test_get_ic_list, param, 0);
        break; /* TEST_STEP_GET_ALL_IC_3 */

    case TEST_STEP_COMPLETE:
        TRACE_MSG(TRACE_APP1, " test procedure is completed", (FMT__0));
        break;
    }

    g_current_test_step++;

    TRACE_MSG(TRACE_APP1, "<< dispatch_test_procedure", (FMT__0));
}


/*! @} */
