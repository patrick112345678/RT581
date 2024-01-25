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
/* PURPOSE: TP/PRO/BV-28 Network Formation Address Assignment
*/

#define ZB_TEST_NAME TP_PRO_BV_28_ZED
#define ZB_TRACE_FILE_ID 40618

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../common/zb_cert_test_globals.h"

//#define TEST_CHANNEL (1l << 24)

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
    zb_uint8_t dev_num;
    zb_uint8_t align[3];
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;

static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint8_t g_dev_num = 1;

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size();
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
#endif

#ifndef ZB_TRACE_LEVEL
void test_minimal_init()
{
    /* Minimal ZBOSS init */
    zb_globals_init();
    ZB_PLATFORM_INIT();
    TRACE_INIT("monitor");
    ZB_ENABLE_ALL_INTER();
}
#endif

MAIN()
{
#ifndef ZB_TRACE_LEVEL
    test_minimal_init();
#endif

    zb_cert_test_set_aps_use_nvram();

#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
#endif

#ifdef ZB_USE_NVRAM
    /* Load nvram manually to init g_dev_num, then erase nvram manually to do clear start */
    zb_osif_nvram_init("zdo_zed");
    zb_nvram_load();
    zb_nvram_erase();
#endif

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("zdo_zed");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    /* Register again after init */
#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
#endif

    /* set ieee addr */
    TRACE_MSG(TRACE_ERROR, "g_dev_num %hd", (FMT__H, g_dev_num));
    g_ieee_addr[0] = g_dev_num;
    zb_set_long_address(g_ieee_addr);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    /* become an ED */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zed_role();
    zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
    zb_set_rx_on_when_idle(ZB_TRUE);

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

#ifdef ZB_USE_NVRAM
static zb_uint16_t test_get_nvram_data_size()
{
    TRACE_MSG(TRACE_ERROR, "test_get_nvram_data_size, ret %hd", (FMT__H, sizeof(test_device_nvram_dataset_t)));
    return sizeof(test_device_nvram_dataset_t);
}

static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    test_device_nvram_dataset_t ds;
    zb_ret_t ret;

    TRACE_MSG(TRACE_ERROR, ">> test_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

    ZB_ASSERT(payload_length == sizeof(ds));

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    if (ret == RET_OK)
    {
        g_dev_num = ds.dev_num;
    }

    TRACE_MSG(TRACE_ERROR, "<< test_nvram_read_app_data ret %d", (FMT__D, ret));
}

static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;
    test_device_nvram_dataset_t ds;

    TRACE_MSG(TRACE_ERROR, ">> test_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

    ds.dev_num = (g_dev_num <= 10) ? g_dev_num : 1;
    /* ds.dev_num = g_dev_num; */

    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_ERROR, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}
#endif /* ZB_USE_NVRAM */

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
            ++g_dev_num;
            zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
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
