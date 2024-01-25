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
/* PURPOSE: TH ZC1 - join devices using install codes.
*/

#define ZB_TEST_NAME CS_ICK_TC_03_THC1
#define ZB_TRACE_FILE_ID 41142
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_ick_tc_03_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif


static int s_dut_join_attempt;
static int s_dut_req_key_attempt;
static zb_aps_device_key_pair_set_t *s_temp_aps_key;
static zb_uint8_t s_temp_tclk[ZB_CCM_KEY_SIZE];
static zb_uint8_t s_gen_tclk[ZB_CCM_KEY_SIZE];


static void generate_tclk2();
static void add_link_key_entry_for_dut();
static void upon_transport_nwk_key_cb(zb_uint8_t param);
static zb_bool_t upon_receive_req_key_cb(zb_uint8_t param, zb_uint16_t keypair_i);
static void revert_key(zb_uint8_t unused);
static void device_annce_cb(zb_zdo_device_annce_t *da);
static void remove_dut(zb_uint8_t param);


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thc1");

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thc1);

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;
    ZB_BDB().bdb_join_uses_install_code_key = 1;
    ZB_AIB().aps_use_nvram = 1;

    generate_tclk2();
    ZB_CERT_HACKS().deliver_nwk_key_cb = upon_transport_nwk_key_cb;

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


static void generate_tclk2()
{
    zb_uint8_t tmp[ZB_CCM_KEY_SIZE + 2];

    zb_sec_b6_hash(g_ic2, 18, tmp);
    ZB_MEMCPY(s_gen_tclk, tmp, ZB_CCM_KEY_SIZE);
    TRACE_MSG(TRACE_ERROR, "THC1: tclk_key2 = " TRACE_FORMAT_128,
              (FMT__A_A, TRACE_ARG_128(s_gen_tclk)));
}


static void add_link_key_entry_for_dut()
{
    zb_aps_device_key_pair_set_t *aps_key;
    zb_ret_t ret;

    ret = zb_secur_ic_get_key_by_address(g_ieee_addr_dut, s_temp_tclk);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: can not get ic-derived key from nvram", (FMT__0));
        return;
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "THC1: remember tclk1: " TRACE_FORMAT_128,
                  (FMT__A_A, TRACE_ARG_128(s_temp_tclk)));
    }

    /* need to create link key entry for DUT manually because
     * thc1 must send first Transport Key cmd (with nwk key) protected with dTCLK,
     * not key derived from ic1
     */
    aps_key = zb_secur_update_key_pair(g_ieee_addr_dut, ZB_AIB().tc_standard_key,
                                       ZB_SECUR_UNIQUE_KEY, ZB_SECUR_PROVISIONAL_KEY);
    TRACE_MSG(TRACE_SECUR1, "Created provisional standart key aps_key %p", (FMT__P, aps_key));
}


static void upon_transport_nwk_key_cb(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO3, ">>upon_transport_nwk_key_cb: buf = %d, join_n = %d",
              (FMT__D_D, param, s_dut_join_attempt + 1));
    ZVUNUSED(param);

    switch (++s_dut_join_attempt)
    {
    case 1:
        /* force thc1 to protect transport key with dTCLK */
        add_link_key_entry_for_dut();
        break;
    case 2:
        /* force thc1 to use key derived from ic */
        zb_secur_delete_link_keys_by_long_addr(g_ieee_addr_dut);
        /* don't call this callback anymore */
        ZB_CERT_HACKS().deliver_nwk_key_cb = NULL;
        break;
    }

    TRACE_MSG(TRACE_ZDO3, "<<upon_transport_nwk_key_cb", (FMT__0));
}


static zb_bool_t upon_receive_req_key_cb(zb_uint8_t param, zb_uint16_t keypair_i)
{
    zb_aps_device_key_pair_set_t *aps_key;
    TRACE_MSG(TRACE_ZDO3, ">>upon_receive_req_key_cb: buf = %d, keypair_i = %d, req_n = %d",
              (FMT__D_D_D, param, keypair_i, s_dut_req_key_attempt + 1));
    ZVUNUSED(param);

    if (keypair_i != 0xffff)
    {
        aps_key = &ZB_AIB().aps_device_key_pair_set[keypair_i];
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "ERROR: unknown keypair", (FMT__0));
    }

    switch (++s_dut_req_key_attempt)
    {
    case 1:
        /* revert key to ic1-derived after sending Transport Key
         * it will help to decrypt next Request Key
         */
        s_temp_aps_key = aps_key;
        ZB_SCHEDULE_ALARM(revert_key, 0, THC1_REVERT_KEY_DELAY);
        /* Use dTCLK to secure Requet Key packet */
        ZB_MEMCPY(aps_key->link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
        break;
    case 2:
        s_temp_aps_key = aps_key;
        ZB_SCHEDULE_ALARM(revert_key, 0, THC1_REVERT_KEY_DELAY);
        /* Use key derived from g_ic2 to protect Request key */
        ZB_MEMCPY(aps_key->link_key, s_gen_tclk, ZB_CCM_KEY_SIZE);;
        break;
    case 3:
        /* Use key derived from g_ic1 to protect Request key */
        ZB_MEMCPY(aps_key->link_key, s_temp_tclk, ZB_CCM_KEY_SIZE);
        break;
    }

    TRACE_MSG(TRACE_ZDO3, "<<upon_receive_req_key_cb", (FMT__0));

    return ZB_FALSE;
}


static void revert_key(zb_uint8_t unused)
{
    TRACE_MSG(TRACE_ZDO3, "<<revert_key to ic1-derived: req_n = %d",
              (FMT__D, s_dut_req_key_attempt));
    ZVUNUSED(unused);

    if (s_temp_aps_key)
    {
        ZB_MEMCPY(s_temp_aps_key->link_key, s_temp_tclk, ZB_CCM_KEY_SIZE);
        s_temp_aps_key = NULL;
    }
}


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
    TRACE_MSG(TRACE_ZDO1, ">>dev_annce_cb, ieee = " TRACE_FORMAT_64 " addr = 0x%x",
              (FMT__A_H, TRACE_ARG_64(da->ieee_addr), da->nwk_addr));
    if ( (ZB_IEEE_ADDR_CMP(g_ieee_addr_dut, da->ieee_addr) == ZB_TRUE) &&
            (s_dut_join_attempt) == 2 )
    {
        ZB_SCHEDULE_ALARM(remove_dut, 0, THC1_REMOVE_DUT_DELAY);
        TRACE_MSG(TRACE_ZDO2, "THC1: DUT is joined!", (FMT__0));
    }

    TRACE_MSG(TRACE_ZDO1, "<<dev_annce_cb", (FMT__0));
}


static void remove_dut(zb_uint8_t param)
{
    zb_address_ieee_ref_t addr_ref;

    ZVUNUSED(param);
    TRACE_MSG(TRACE_ZDO1, "THC1: removing DUT", (FMT__0));
    if (zb_address_by_ieee(g_ieee_addr_dut, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
        zb_nwk_forget_device(addr_ref);
        /* Go to next step: sending transport Key (with tclk) encrypted by wrong keys  */
        ZB_CERT_HACKS().req_key_ind_cb = upon_receive_req_key_cb;
    }
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            zb_secur_ic_add(g_ieee_addr_dut, g_ic1);
            zb_zdo_register_device_annce_cb(device_annce_cb);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        default:
            TRACE_MSG(TRACE_APS1, "Unknown signal - %hd", (FMT__H, sig));
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
