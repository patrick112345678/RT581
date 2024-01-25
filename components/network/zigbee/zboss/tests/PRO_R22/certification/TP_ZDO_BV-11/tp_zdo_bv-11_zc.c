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
/* PURPOSE: TP/ZDO/BV-11: ZED-ZDO-Transmit Service Discovery
The DUT as ZigBee end device shall request service discovery to a
ZigBee coordinator. Coordinator side.
*/

#define ZB_TEST_NAME TP_ZDO_BV_11_ZC
#define ZB_TRACE_FILE_ID 40825

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

ZB_DECLARE_SIMPLE_DESC(10, 10);

static zb_af_simple_desc_10_10_t test_simple_desc;

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_zc");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


    zb_set_node_power_descriptor(ZB_POWER_MODE_SYNC_ON_WHEN_IDLE,
                                 ZB_POWER_SRC_CONSTANT | ZB_POWER_SRC_RECHARGEABLE_BATTERY | ZB_POWER_SRC_DISPOSABLE_BATTERY,
                                 ZB_POWER_SRC_CONSTANT, ZB_POWER_LEVEL_100);

    zb_cert_test_set_common_channel_settings();
    /*
      simple descriptor for test
      SimpleDescriptor=
      Endpoint=0x01, Application profile identifier=0x0103, Application device
      identifier=0x0000, Application device version=0b0000, Application
      flags=0b0000, Application input cluster count=0x0A, Application input
      cluster list=0x00 0x03 0x04 0x38 0x54 0x70 0x8c 0xc4 0xe0 0xff,
      Application output cluster count=0x0A, Application output cluster
      list=0x00 0x01 0x02 0x1c 0x38 0x70 0x8c 0xa8 0xc4 0xff
    */


    zb_set_simple_descriptor((zb_af_simple_desc_1_1_t *)&test_simple_desc,
                             1 /* endpoint */,                0x0103 /* app_profile_id */,
                             0x0 /* app_device_id */,         0x0   /* app_device_version*/,
                             0xA /* app_input_cluster_count */, 0xA /* app_output_cluster_count */);


    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 0,  0x00);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 1,  0x03);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 2,  0x04);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 3,  0x38);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 4,  0x54);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 5,  0x70);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 6,  0x8c);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 7,  0xc4);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 8,  0xe0);
    zb_set_input_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 9,  0xff);


    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 0,  0x00);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 1,  0x01);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 2,  0x02);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 3,  0x1c);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 4,  0x38);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 5,  0x70);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 6,  0x8c);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 7,  0xa8);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 8,  0xc4);
    zb_set_output_cluster_id((zb_af_simple_desc_1_1_t *)&test_simple_desc, 9,  0xff);

    zb_add_simple_descriptor((zb_af_simple_desc_1_1_t *)&test_simple_desc);


    /* let's always be coordinator */
    zb_cert_test_set_zc_role();

    /* set ieee addr */
    zb_set_long_address(g_ieee_addr);
    zb_bdb_set_legacy_device_support(ZB_TRUE);
    zb_set_pan_id(TEST_PAN_ID);

    /* turn off security */
    /* zb_cert_test_set_security_level(0); */

    /* accept only one child */
    zb_set_max_children(2);
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
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
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
