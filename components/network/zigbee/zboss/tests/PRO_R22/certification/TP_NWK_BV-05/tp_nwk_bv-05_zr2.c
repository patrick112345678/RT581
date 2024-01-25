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
/* PURPOSE: TP/NWK/BV-05 NWK Maximum Depth
Verify that DUT, in position of a router in a larger network interoperability scenario, acts
correctly after joining at maximum depth (Pro Max depth=15; OK to join at depth 0xF)
*/

#define ZB_TEST_NAME TP_NWK_BV_05_ZR2
#define ZB_TRACE_FILE_ID 40751

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

#ifdef ONLY_TWO_DEV_FOR_ROUTERS
static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};

static zb_uint8_t g_dev_num = 2;

typedef ZB_PACKED_PRE struct test_device_nvram_dataset_s
{
    zb_uint8_t dev_num;
    zb_uint8_t align[3];
} ZB_PACKED_STRUCT test_device_nvram_dataset_t;


static zb_uint16_t test_get_nvram_data_size();
static void test_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t test_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos);
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */


MAIN()
{
    ARGV_UNUSED;

#ifdef ONLY_TWO_DEV_FOR_ROUTERS
    zb_cert_test_set_aps_use_nvram();

#ifdef ZB_USE_NVRAM
    zb_init_buffers();
    zb_buf_get_out();
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);

    /* Load nvram manually to init g_dev_num, then erase nvram manually to do clear start */
    zb_osif_nvram_init("");
    zb_nvram_load();
    zb_nvram_erase();
#endif
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */

    ZB_INIT("zdo_zr2");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

#ifdef ONLY_TWO_DEV_FOR_ROUTERS
    /* Register again after init */
#ifdef ZB_USE_NVRAM
    zb_nvram_register_app1_read_cb(test_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(test_nvram_write_app_data, test_get_nvram_data_size);
#endif

    TRACE_MSG(TRACE_ERROR, "g_dev_num %hd", (FMT__H, g_dev_num));
    g_ieee_addr[4] = g_dev_num;
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */

    zb_set_long_address(g_ieee_addr_r2);
    /* join as a router */
    zb_cert_test_set_common_channel_settings();
    zb_cert_test_set_zr_role();
    /* zb_cert_test_set_security_level(0); */
    ZB_CERT_HACKS().extended_beacon_send_jitter = ZB_TRUE;
    zb_set_max_children(1);

#ifndef ONLY_TWO_DEV_FOR_ROUTERS
    zb_set_nvram_erase_at_start(ZB_TRUE);
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */

    //MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r1);
    //MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r3);
    /* MAC_ADD_INVISIBLE_SHORT(0x0000); */

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


#ifndef ONLY_TWO_DEV_FOR_ROUTERS
static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
    ZVUNUSED(ieee_addr);
    /* fix address for next ZR */
    return 0x0003;
}
#endif /* !ONLY_TWO_DEV_FOR_ROUTERS */


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    /* TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status)); */

    if (0 == status)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_CALLBACK(test_after_startup_action, 0);
#ifdef ONLY_TWO_DEV_FOR_ROUTERS
            if (g_dev_num < 16)
            {
                g_dev_num += 2;
            }
            else
            {
                ++g_dev_num;
            }

            zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */
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

    zb_buf_free(param);

#ifndef ONLY_TWO_DEV_FOR_ROUTERS
    zb_nwk_set_address_assignment_cb(addr_ass_cb);
#endif /* !ONLY_TWO_DEV_FOR_ROUTERS */
}


#ifdef ONLY_TWO_DEV_FOR_ROUTERS
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

    ds.dev_num = (18 != g_dev_num) ? g_dev_num : 2;

    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_ERROR, "<< test_nvram_write_app_data, ret %d", (FMT__D, ret));

    return ret;
}
#endif /* ZB_USE_NVRAM */
#endif /* ONLY_TWO_DEV_FOR_ROUTERS */
