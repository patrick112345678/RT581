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

#define ZB_TEST_NAME RTP_OTA_CLI_06_TH_ZC
#define ZB_TRACE_FILE_ID 40298

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "../common/zb_reg_test_globals.h"
#include "rtp_ota_cli_06_th_zc.h"
#include "rtp_ota_cli_06_common.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

static zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
static zb_uint8_t g_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;

#define ENDPOINT  5

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(rtp_ota_cli_06_th_zc_basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Time server cluster attributes data */

static zb_uint32_t g_time = ZB_ZCL_TIME_TIME_DEFAULT_VALUE;
static zb_uint8_t g_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
static zb_uint32_t g_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
static zb_uint32_t g_dst_start = 0;
static zb_uint32_t g_dst_end = 0;
static zb_uint32_t g_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
static zb_uint32_t g_standard_time = 0;
static zb_uint32_t g_local_time = 0;
static zb_uint32_t g_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
static zb_uint32_t g_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(rtp_ota_cli_06_th_zc_time_attr_list, &g_time, &g_time_status, &g_time_zone,
                                &g_dst_start, &g_dst_end, &g_dst_shift, &g_standard_time,
                                &g_local_time, &g_last_set_time, &g_valid_until_time);

/* OTA Upgrade server cluster attributes data */
static zb_uint8_t query_jitter = 1;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(rtp_ota_cli_06_th_zc_ota_upgrade_attr_list, &query_jitter, &g_time, 1);

/********************* Declare device **************************/

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_CLUSTER_LIST(rtp_ota_cli_06_th_zc_ota_upgrade_server_clusters,
        rtp_ota_cli_06_th_zc_basic_attr_list,
        rtp_ota_cli_06_th_zc_time_attr_list,
        rtp_ota_cli_06_th_zc_ota_upgrade_attr_list);

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_EP(rtp_ota_cli_06_th_zc_ota_upgrade_server_ep, ENDPOINT, rtp_ota_cli_06_th_zc_ota_upgrade_server_clusters);

ZB_HA_DECLARE_OTA_UPGRADE_SERVER_CTX(rtp_ota_cli_06_th_zc_ota_upgrade_server_ctx, rtp_ota_cli_06_th_zc_ota_upgrade_server_ep);

/******************* Declare test data & constants *************/

typedef ZB_PACKED_PRE struct ota_upgrade_test_file_s
{
    zb_zcl_ota_upgrade_file_header_t head;
    zb_uint8_t arr[32];

} ZB_PACKED_STRUCT ota_upgrade_test_file_t;

static ota_upgrade_test_file_t ota_file =
{
    {
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_ID,         // OTA upgrade file identifier
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_FILE_VERSION,    // OTA Header version
        0x38,                                           // OTA Header length
        0x10,                                           // OTA Header Field control
        OTA_UPGRADE_TEST_MANUFACTURER,                  // Manufacturer code
        OTA_UPGRADE_TEST_IMAGE_TYPE,                    // Image type
        OTA_UPGRADE_TEST_FILE_VERSION_NEW,              // File version
        ZB_ZCL_OTA_UPGRADE_FILE_HEADER_STACK_PRO,       // Zigbee Stack version
        // OTA Header string
        {
            0x54, 0x68, 0x65, 0x20, 0x6c, 0x61, 0x74, 0x65,   0x73, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x67,
            0x72, 0x65, 0x61, 0x74, 0x65, 0x73, 0x74, 0x20,   0x75, 0x70, 0x67, 0x72, 0x61, 0x64, 0x65, 0x2e,
        },
        OTA_UPGRADE_TEST_IMAGE_SIZE,                    // Total Image size (including header)
    },
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    }
};

static void insert_ota_file(zb_uint8_t param);

static zb_ret_t next_data_ind_cb(zb_uint8_t index,
                                 zb_zcl_parsed_hdr_t *zcl_hdr,
                                 zb_uint32_t offset,
                                 zb_uint8_t size,
                                 zb_uint8_t **data)
{
    ZVUNUSED(index);
    ZVUNUSED(zcl_hdr);
    ZVUNUSED(size);
    *data = ((zb_uint8_t *)&ota_file + offset);
    return RET_OK;
}

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_long_address(g_zc_addr);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_secur_setup_nwk_key(g_key, 0);

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&rtp_ota_cli_06_th_zc_ota_upgrade_server_ctx);

    zb_zcl_time_init_server();
    zb_zcl_ota_upgrade_init_server(ENDPOINT, next_data_ind_cb);

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
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %h", (FMT__H, param));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break;

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        if (status == 0)
        {
            zb_zcl_time_update_current_time(ENDPOINT);

            test_step_register(insert_ota_file, 0, RTP_OTA_CLI_06_STEP_1_TIME_ZC);

            test_control_start(TEST_MODE, RTP_OTA_CLI_06_STEP_1_DELAY_ZC);
        }

        break; /* ZB_ZDO_SIGNAL_DEVICE_ANNCE */

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
    }

    if (param)
    {
        zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}

static void insert_ota_file(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZCL1, "> insert_ota_file %hd", (FMT__H, param));

    if (!param)
    {
        zb_buf_get_out_delayed(insert_ota_file);
    }
    else
    {
        zb_ret_t ret;

        ZB_ZCL_OTA_UPGRADE_INSERT_FILE(param, ENDPOINT, 0, (zb_uint8_t *)(&ota_file),
                                       g_time + OTA_UPGRADE_TEST_UPGRADE_TIME_DELTA, ZB_TRUE, ret);

        ZB_ASSERT(ret == RET_OK);
    }

    TRACE_MSG(TRACE_ZCL1, "< insert_ota_file", (FMT__0));
}
