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

#define ZB_TEST_NAME RTP_NVRAM_02_DUT_ZC
#define ZB_TRACE_FILE_ID 40427

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_nvram_02_common.h"
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

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_dut_zc = IEEE_ADDR_DUT_ZC;

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
    zb_uint16_t data[TEST_APP_NVRAM_PAYLOAD_SIZE];
    zb_uint16_t data_version;
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

static zb_uint16_t g_test_nvram_data_version = 0;

static zb_uint16_t test_get_nvram_data_size();
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);

static void trigger_steering(zb_uint8_t unused);
static void trigger_nvram_write_app_dataset(zb_uint8_t unused);
static void force_nvram_to_migrate_by_filling_page(zb_uint8_t unused);


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
    zb_set_nvram_erase_at_start(ZB_FALSE);

    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);

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
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_SCHEDULE_CALLBACK(trigger_steering, 0);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        if (status == 0)
        {
            test_step_register(force_nvram_to_migrate_by_filling_page, 0, RTP_NVRAM_02_STEP_1_TIME_ZC);

            test_control_start(TEST_MODE, RTP_NVRAM_02_STEP_1_DELAY_ZC);
        }
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}


static void trigger_steering(zb_uint8_t unused)
{
    ZVUNUSED(unused);

    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}


static zb_uint16_t test_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "test_get_nvram_data_size, ret %hd", (FMT__H, sizeof(test_device_nvram_dataset_t)));
    return sizeof(test_device_nvram_dataset_t);
}


static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_int16_t i;
    zb_bool_t data_correct;
    test_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> test_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    if (ret == RET_OK)
    {
        data_correct = ZB_TRUE;

        if (ds.data_version != TEST_APP_NVRAM_REWRITES_COUNT - 1)
        {
            data_correct = ZB_FALSE;
        }

        for (i = 0; i < TEST_APP_NVRAM_PAYLOAD_SIZE; i++)
        {
            if (ds.data[i] != i)
            {
                data_correct = ZB_FALSE;
            }
        }

        g_test_nvram_data_version = ds.data_version;

        TRACE_MSG(TRACE_APP1, "test nvram app data validation: %hd", (FMT__H_D, data_correct));
    }

    TRACE_MSG(TRACE_APP1, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}


static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    zb_int16_t i;
    test_device_nvram_dataset_t ds;

    TRACE_MSG(TRACE_APP1, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    for (i = 0; i < TEST_APP_NVRAM_PAYLOAD_SIZE; i++)
    {
        ds.data[i] = i;
    }

    ds.data_version = g_test_nvram_data_version;

    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APP1, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}


static void trigger_nvram_write_app_dataset(zb_uint8_t unused)
{
    zb_ret_t ret;

    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, ">> trigger_nvram_write_app_dataset", (FMT__0));

    ret = zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
    g_test_nvram_data_version++;

    if (g_test_nvram_data_version < TEST_APP_NVRAM_REWRITES_COUNT)
    {
        ZB_SCHEDULE_APP_ALARM(trigger_nvram_write_app_dataset, 0, 1 * ZB_TIME_ONE_SECOND);
    }

    TRACE_MSG(TRACE_APP1, "<< trigger_nvram_write_app_dataset, version %d, ret %x",
              (FMT__D_D, g_test_nvram_data_version, ret));

    ZB_ASSERT(ret == RET_OK);
}


static void force_nvram_to_migrate_by_filling_page(zb_uint8_t param)
{
    ZB_SCHEDULE_APP_ALARM(trigger_nvram_write_app_dataset, param, 1 * ZB_TIME_ONE_SECOND);
}

/*! @} */
