/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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

/**
 * PURPOSE: TH ZC
 */

#define ZB_TEST_NAME TP_AN_NOH_4_1_TH_ZC
#define ZB_TRACE_FILE_ID 64197

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "zb_mem_config_max.h"

#include "tc_sw_01_common.h"
#include "zboss_tcswap.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif

#define TCSW_NVRAM_DATASET_SIZE 1024

zb_ieee_addr_t g_ieee_addr_th_zc   = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00};

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

zb_uint8_t g_nvram_buf[TCSW_NVRAM_DATASET_SIZE] = {0};
zb_bool_t  tc_db_loaded = ZB_FALSE;

zb_ret_t tcsw_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret;

    TRACE_MSG(TRACE_APP1, "+tcsw_nvram_write_app_data", (FMT__0));

    ret = zb_nvram_write_data(page, pos, g_nvram_buf, TCSW_NVRAM_DATASET_SIZE);

    TRACE_MSG(TRACE_APP1, "-tcsw_nvram_write_app_data, ret %hd", (FMT__H, ret));

    return ret;
}

void tcsw_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    TRACE_MSG(TRACE_APP1, "tcsw_nvram_read_app_data", (FMT__0));

    ZB_ASSERT(payload_length == TCSW_NVRAM_DATASET_SIZE);
    zb_nvram_read_data(page, pos, g_nvram_buf, TCSW_NVRAM_DATASET_SIZE);
    tc_db_loaded = ZB_TRUE;
}

zb_uint16_t tcsw_get_nvram_data_size(void)
{
    TRACE_MSG(TRACE_APP1, "tcsw_get_nvram_data_size", (FMT__0));
    return TCSW_NVRAM_DATASET_SIZE;
}

MAIN()
{
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_long_address(g_ieee_addr_th_zc);
    zb_set_network_coordinator_role(1l << TEST_CHANNEL);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_FALSE);

    /* Register NVRAM application callbacks - to be able to store application data to the special
       dataset. */

    zb_nvram_register_app1_read_cb(tcsw_nvram_read_app_data);
    zb_nvram_register_app1_write_cb(tcsw_nvram_write_app_data, tcsw_get_nvram_data_size);

    if (zb_zdo_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void zb_time_to_die(zb_uint8_t param)
{
    ZVUNUSED(param);

    TRACE_MSG(TRACE_APP1, "TIME TO DIE", (FMT__0));

    /* Note that this call keeps app1 datased with TC swap DB saved there */
    zb_nvram_clear();

    ZB_SCHEDULE_CALLBACK(zb_reset, 0);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_APP1, "Signal ZB_ZDO_SIGNAL_SKIP_STARTUP, status %d", (FMT__D, status));
        if (status == 0)
        {
            if (tc_db_loaded)
            {
                zb_ret_t ret = zb_tcsw_start_restore_db(g_nvram_buf, TCSW_NVRAM_DATASET_SIZE, ZB_FALSE);
                TRACE_MSG(TRACE_APP1, "RESTORE TC DB, ret %hd", (FMT__H, ret));

                g_ieee_addr_th_zc[7] += 1;
                zb_set_long_address(g_ieee_addr_th_zc);

                tc_db_loaded = ZB_FALSE;
            }
            /* Just restored DB of hashed TCLKs and long panid. Continue start - do Formation */
            zboss_start_continue();
        }
        break;/* ZB_ZDO_SIGNAL_SKIP_STARTUP */

    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));
        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_TCSWAP_DB_BACKUP_REQUIRED_SIGNAL:
    {
        zb_ret_t ret = zb_tcsw_start_backup_db(g_nvram_buf, TCSW_NVRAM_DATASET_SIZE);
        TRACE_MSG(TRACE_APP1, "BACKUP TC DB, ret %hd", (FMT__H, ret));

        ZB_SCHEDULE_CALLBACK(zb_nvram_write_dataset, ZB_NVRAM_APP_DATA1);
    }
    break;

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ZB_SCHEDULE_ALARM(zb_time_to_die, 0, 20 * ZB_TIME_ONE_SECOND);
        break;

    default:
        TRACE_MSG(TRACE_APP1, "Unknown signal, status %d", (FMT__D, status));
        break;
    }

    zb_buf_free(param);
}
