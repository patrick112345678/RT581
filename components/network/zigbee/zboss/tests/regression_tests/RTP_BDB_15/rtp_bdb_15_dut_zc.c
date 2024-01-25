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
/* PURPOSE: TH ZC
*/

#define ZB_TEST_NAME RTP_BDB_15_DUT_ZC
#define ZB_TRACE_FILE_ID 40256

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"

#include "rtp_bdb_15_common.h"
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
    zb_uint32_t reboots_count;
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

static zb_uint16_t test_get_nvram_data_size();
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);

static void trigger_steering(zb_uint8_t unused);

static zb_uint32_t g_reboots_count = 0;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);

    ARGV_UNUSED;

    ZB_INIT("zdo_dut_zc");

    zb_set_long_address(g_ieee_addr_dut_zc);

    zb_set_pan_id(0x1aaa);

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
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));

        /* Fail with assertion error if we have ZB_BDB_SIGNAL_STEERING after reboot */
        ZB_ASSERT(g_reboots_count == 0 || g_reboots_count == REBOOTS_COUNT_BEFORE_STEERING);

        break; /* ZB_BDB_SIGNAL_STEERING */

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_DEVICE_REBOOT, status %d", (FMT__D, status));
        if (status == 0)
        {
            ZB_ASSERT(!zb_bdb_is_factory_new());

            g_reboots_count++;
            zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);

            if (g_reboots_count == REBOOTS_COUNT_BEFORE_STEERING)
            {
                test_step_register(trigger_steering, 0, RTP_BDB_15_STEP_1_TIME_ZR);
                test_control_start(TEST_MODE, RTP_BDB_15_STEP_1_DELAY_ZR);
            }

        }
        break; /* ZB_BDB_SIGNAL_DEVICE_REBOOT */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d, sig %d", (FMT__D_D, status, sig));
        break;
    }

    zb_buf_free(param);
}

static void trigger_steering(zb_uint8_t unused)
{
    zb_bool_t steering_status = bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

    ZVUNUSED(unused);

    TRACE_MSG(TRACE_APP1, "trigger_steering(), status %hd", (FMT__H, steering_status));
}

static zb_uint16_t test_get_nvram_data_size()
{
    TRACE_MSG(TRACE_APP1, "test_get_nvram_data_size, ret %hd", (FMT__H, sizeof(test_device_nvram_dataset_t)));
    return sizeof(test_device_nvram_dataset_t);
}

static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    test_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, ">> test_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    ret = zb_nvram_read_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    ZB_ASSERT(ret == RET_OK);

    g_reboots_count = ds.reboots_count;

    TRACE_MSG(TRACE_APP1, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}

static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    test_device_nvram_dataset_t ds;

    TRACE_MSG(TRACE_APP1, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    ds.reboots_count = g_reboots_count;
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APP1, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}

/*! @} */
