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
/* PURPOSE: TH ZR1 - joining using install codes.
*/


#define ZB_TEST_NAME CS_ICK_TC_02_THR1
#define ZB_TRACE_FILE_ID 41018
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_ick_tc_02_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error define ZB_CERTIFICATION_HACKS
#endif


static void generate_keys();
static zb_bool_t upon_key_request(zb_uint8_t param);

static int s_req_attempt = 1;
static zb_uint8_t s_tclk_key1[ZB_CCM_KEY_SIZE];
static zb_uint8_t s_tclk_key2[ZB_CCM_KEY_SIZE];


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_BDB().bdb_join_uses_install_code_key = 1;
    zb_secur_ic_set(g_ic1);
    ZB_CERT_HACKS().req_key_call_cb = upon_key_request;
    generate_keys();

    if (zdo_dev_start() != RET_OK)
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


static void generate_keys()
{
    zb_uint8_t tmp[ZB_CCM_KEY_SIZE + 2];

    zb_sec_b6_hash(g_ic1, 18, tmp);
    ZB_MEMCPY(s_tclk_key1, tmp, ZB_CCM_KEY_SIZE);
    //! [trace_128]
    TRACE_MSG(TRACE_ERROR, "KEY: tclk_key1 = " TRACE_FORMAT_128,
              (FMT__A_A, TRACE_ARG_128(s_tclk_key1)));
    //! [trace_128]
    zb_sec_b6_hash(g_ic2, 18, tmp);
    ZB_MEMCPY(s_tclk_key2, tmp, ZB_CCM_KEY_SIZE);
    TRACE_MSG(TRACE_ERROR, "KEY: tclk_key2 = " TRACE_FORMAT_128,
              (FMT__A_A, TRACE_ARG_128(s_tclk_key2)));
}


static zb_bool_t upon_key_request(zb_uint8_t param)
{
    zb_aps_device_key_pair_set_t *aps_key;
    zb_ieee_addr_t tc_ieee_addr;

    TRACE_MSG(TRACE_ZDO3, ">>upon_key_request: buf = %d", (FMT__D, param));
    ZVUNUSED(param);

    ZB_IEEE_ADDR_COPY(tc_ieee_addr, ZB_AIB().trust_center_address);
    aps_key = zb_secur_get_link_key_by_address(tc_ieee_addr, ZB_SECUR_ANY_KEY_ATTR);
    if (!aps_key)
    {
        TRACE_MSG(TRACE_ZDO3, "upon_key_request: tc address is unknown", (FMT__0));
        return;
    }

    switch (s_req_attempt++)
    {
    case 1:
        /* Use dTCLK to secure Requet Key packet */
        ZB_MEMCPY(aps_key->link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
        break;
    case 2:
        /* Use key derived from g_ic2 to protect Request key */
        ZB_MEMCPY(aps_key->link_key, s_tclk_key2, ZB_CCM_KEY_SIZE);;
        break;
    case 3:
        /* Use key derived from g_ic1 to protect Request key */
        ZB_MEMCPY(aps_key->link_key, s_tclk_key1, ZB_CCM_KEY_SIZE);
        break;
    }

    TRACE_MSG(TRACE_ZDO3, "<<upon_key_request", (FMT__0));

    return ZB_FALSE;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            break;
        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
    zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
