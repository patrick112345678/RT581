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
/* PURPOSE: TH ZC requiring TCLK update
*/

#define ZB_TEST_NAME CS_NFS_TC_01_THC1
#define ZB_TRACE_FILE_ID 41008
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_nfs_tc_01_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error Define ZB_USE_NVRAM
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif


enum thc1_test_step_e
{
    DROP_REQUEST_KEY,
    SEND_TCLK_UNSECURED,
    SEND_TCLK_WITH_WRONG_PROTECTION,
    SEND_TCLK_WITH_WRONG_TC_ADDR,
    SEND_TCLK_NOT_FOR_DUT,
    SEND_TCLK_WITH_WRONG_MIC,
    SEND_TCLK_WITH_DATA_KEY_PROTECTION,
    SEND_TCLK_WITH_KEY_TK_PROTECTION,
    SEND_TCLK_WITH_NWK_KEY_PROTECTION,
    SEND_TCLK_WITH_OLD_APS_SEC_COUNTER,
    SEND_TCLK_WITH_APP_LINK_KEY,
    SEND_TCLK_WITH_NWK_KEY,
    SEND_TCLK_WITH_DTCLK,
    DTCLK_SEC_COUNTER_NEGATIVE_TEST,
    DTCLK_SEC_COUNTER_POSITIVE_TEST,
    SEND_CONF_KEY_PROTECTED_WITH_DTCLK,
    DROP_CONFIRM_KEY,
    SUCCESSFUL_COMMISSIONING,
    SEND_APS_REMOVE_PROTECTED_WITH_DTCLK,
    THC1_TEST_STEP_COUNTS
};


static zb_bool_t upon_request_key_receipt(zb_uint8_t param, zb_uint16_t keypair_i);
static void upon_sending_confirm_key(zb_uint8_t param, zb_uint16_t keypair_i);
static zb_bool_t check_cmd_and_change_mic(zb_uint8_t *ccm_p,
        zb_uint8_t *hdr,
        zb_ushort_t hdr_len);
static void upon_aps_secured_frame_sending(zb_uint32_t *p_counter);

static zb_bool_t test_fsm(zb_aps_device_key_pair_set_t *aps_key);
static void revert_key(zb_uint8_t unused);
static void check_tclk_update_done(zb_uint8_t unused);

/* Creates frame with Transport Key command carrying updated TCLK */
static void prepare_tk_cmd(zb_buf_t *buf, int local_step_idx);
static void send_tk_via_aps(zb_uint8_t param, zb_uint16_t local_step_idx);
static void send_tk_via_nwk(zb_uint8_t param, zb_uint16_t local_step_idx);
/* APS and NWK secured commands */
static void send_buffer_test(zb_uint8_t param);
static void buffer_test_resp_cb(zb_uint8_t status);
static void send_aps_remove(zb_uint8_t param);


/* NVRAM support */
static void save_test_state();
#ifdef USE_NVRAM_IN_TEST
static void read_thc_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t write_thc_app_data_cb(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t thc_nvram_get_app_data_size_cb();
#endif /* USE_NVRAM_IN_TEST */


static int s_step_idx;
static int s_dut_request_key_attempt;
static zb_uint8_t s_saved_aps_frame_counter;
static zb_uint32_t s_saved_aps_secur_counter;
static zb_uint8_t s_saved_link_key[ZB_CCM_KEY_SIZE];
static zb_aps_device_key_pair_set_t *s_temp_aps_key_ptr;
static zb_aps_device_key_pair_set_t s_saved_aps_key;
static zb_uint16_t                  s_saved_keypair_i;


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
    ZB_AIB().aps_use_nvram = 1;

    ZB_CERT_HACKS().req_key_ind_cb = upon_request_key_receipt;
    ZB_CERT_HACKS().allow_nwk_encryption_for_aps_frames = 1;

#ifdef USE_NVRAM_IN_TEST
    zb_nvram_register_app1_read_cb(read_thc_app_data_cb);
    zb_nvram_register_app1_write_cb(write_thc_app_data_cb, thc_nvram_get_app_data_size_cb);
#endif

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


static zb_bool_t upon_request_key_receipt(zb_uint8_t param, zb_uint16_t keypair_i)
{
    zb_bool_t ret;

    TRACE_MSG(TRACE_ZDO3, ">>upon_request_key_receipt: buf = %d, keypair_i = %d, key_req_attempt = %d",
              (FMT__D_D_D, param, keypair_i, s_dut_request_key_attempt + 1));
    ZVUNUSED(param);

    zb_aps_keypair_read_by_idx(keypair_i, &s_saved_aps_key);

    ++s_dut_request_key_attempt;
    ret = test_fsm(&s_saved_aps_key);
    ZB_SCHEDULE_ALARM_CANCEL(check_tclk_update_done, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_ALARM(check_tclk_update_done, 0, THC1_WAIT_FOR_TCLK_UPDATE_DELAY);

    TRACE_MSG(TRACE_ZDO3, "<<upon_request_key_receipt", (FMT__0));

    return ret;
}


static void upon_sending_confirm_key(zb_uint8_t param, zb_uint16_t keypair_i)
{
    ZVUNUSED(param);
    if (keypair_i != 0xffff)
    {
        zb_aps_keypair_read_by_idx(keypair_i, &s_saved_aps_key);
    }

    if (s_step_idx == SEND_CONF_KEY_PROTECTED_WITH_DTCLK)
    {
        ZB_MEMCPY(s_saved_link_key, s_saved_aps_key.link_key, ZB_CCM_KEY_SIZE);
        s_temp_aps_key_ptr = &s_saved_aps_key;
        ZB_MEMCPY(s_saved_aps_key.link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
        zb_aps_keypair_write(&s_saved_aps_key, keypair_i);
        s_saved_keypair_i = keypair_i;

        ZB_SCHEDULE_ALARM_CANCEL(check_tclk_update_done, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_ALARM(check_tclk_update_done, 0, THC1_WAIT_FOR_TCLK_UPDATE_DELAY);

        ZB_SCHEDULE_ALARM(revert_key, 0, THC1_REVERT_KEY_DELAY);
    }
}


static zb_bool_t check_cmd_and_change_mic(zb_uint8_t *ccm_p,
        zb_uint8_t *hdr,
        zb_ushort_t hdr_len)
{
    zb_aps_command_pkt_header_t *aps_hdr = (zb_aps_command_pkt_header_t *) hdr;
    zb_uint8_t *cmd_id_ptr = hdr + hdr_len;

    TRACE_MSG(TRACE_SECUR1, "TEST: check_and_change_mic - ccm_p = %p, hdr = %p, len = %d",
              (FMT__P_P_D, ccm_p, hdr, hdr_len));

    if ( (ZB_APS_FC_GET_FRAME_TYPE(aps_hdr->fc) ==  ZB_APS_FRAME_COMMAND) &&
            (aps_hdr->aps_counter == s_saved_aps_frame_counter) &&
            (*cmd_id_ptr == APS_CMD_TRANSPORT_KEY) )
    {
        int i;

        TRACE_MSG(TRACE_SECUR1, "TEST: Got my tk with tclk: ccm = " TRACE_FORMAT_128, (FMT__B, ccm_p));
        /* corrupt MIC */
        for (i = 0; i < 4; ++i)
        {
            zb_uint8_t *byte_ptr = &ccm_p[i];
            *byte_ptr = ~*byte_ptr + i; /* corrupt MIC */
        }
        return ZB_TRUE;
    }
    else
    {
        return ZB_FALSE;
    }
}


static void upon_aps_secured_frame_sending(zb_uint32_t *p_counter)
{
    if (s_step_idx == DTCLK_SEC_COUNTER_NEGATIVE_TEST)
    {
        *p_counter = s_saved_aps_secur_counter;
    }
}


static zb_bool_t test_fsm(zb_aps_device_key_pair_set_t *aps_key)
{
    zb_bool_t ret = ZB_TRUE; /* drop request Key */

    TRACE_MSG(TRACE_ZDO1, ">>test_fsm: step = %d", (FMT__D, s_step_idx));

    ZVUNUSED(aps_key);

    switch (s_step_idx)
    {
    case DROP_REQUEST_KEY:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Drop incoming Requet Key, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        break;

    case SEND_TCLK_UNSECURED:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK unsecured, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_aps, s_step_idx);
        break;

    case SEND_TCLK_WITH_WRONG_PROTECTION:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK unsecured with aps security header, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_TCLK_WITH_WRONG_TC_ADDR:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with src addr != TC addr (in tk payload), req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_aps, s_step_idx);
        break;

    case SEND_TCLK_NOT_FOR_DUT:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with dest addr != DUT addr (in tk payload), req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_aps, s_step_idx);
        break;

    case SEND_TCLK_WITH_WRONG_MIC:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with wrong MIC, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        s_saved_aps_frame_counter = ZB_AIB_APS_COUNTER();
        ZB_CERT_HACKS().ccm_check_cb = check_cmd_and_change_mic;
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_TCLK_WITH_DATA_KEY_PROTECTION:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with data key protection, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_TCLK_WITH_KEY_TK_PROTECTION:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with key-transport-key protection, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_TCLK_WITH_NWK_KEY_PROTECTION:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with nwk-key protection, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_TCLK_WITH_OLD_APS_SEC_COUNTER:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send TCLK with Old APS security counter, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        if (s_dut_request_key_attempt < 3)
        {
            ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, SEND_TCLK_WITH_DATA_KEY_PROTECTION);
        }
        else
        {
            ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        }
        break;

    case SEND_TCLK_WITH_APP_LINK_KEY:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send Application link key instead of TC LK, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_aps, s_step_idx);
        break;

    case SEND_TCLK_WITH_NWK_KEY:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send Standard network key instead of TC LK, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_aps, s_step_idx);
        break;

    case SEND_TCLK_WITH_DTCLK:
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: Send link key with dTCLK, req_key_attempt = %d",
                  (FMT__D, s_dut_request_key_attempt));
        /* Need to save aps security counter */
        ZB_GET_OUT_BUF_DELAYED2(send_tk_via_nwk, s_step_idx);
        break;

    case SEND_CONF_KEY_PROTECTED_WITH_DTCLK:
        TRACE_MSG(TRACE_ZDO1, "TEST: Send Confirm key protected with dTCLK", (FMT__0));
        ZB_CERT_HACKS().deliver_conf_key_cb = upon_sending_confirm_key;
        ret = ZB_FALSE;
        break;

    case DROP_CONFIRM_KEY:
        TRACE_MSG(TRACE_ZDO1, "TEST: Drop Confirm dTCLK", (FMT__0));
        /* Give more time for this test step - think this will be enough.
         * Do'nt want create callback for in_verify_key and handle test case using it.
         */
        ZB_SCHEDULE_ALARM_CANCEL(check_tclk_update_done, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_ALARM(check_tclk_update_done, 0, 2 * THC1_WAIT_FOR_TCLK_UPDATE_DELAY);
        ZB_CERT_HACKS().drop_verify_key_indication = 1;
        ret = ZB_FALSE;
        break;

    case SUCCESSFUL_COMMISSIONING:
        TRACE_MSG(TRACE_ZDO1, "TEST: Successful commissioning", (FMT__0));
        ret = ZB_FALSE;
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "TEST: ERROR - unknown state transition", (FMT__0));
        ret = ZB_FALSE;
    }

    TRACE_MSG(TRACE_ZDO1, "<<test_fsm", (FMT__0));

    return ret;
}


static void revert_key(zb_uint8_t unused)
{
    TRACE_MSG(TRACE_ZDO3, "revert_key to dTCLK", (FMT__0));
    ZVUNUSED(unused);

    if (s_temp_aps_key_ptr)
    {
        ZB_MEMCPY(s_temp_aps_key_ptr->link_key, s_saved_link_key, ZB_CCM_KEY_SIZE);
        zb_aps_keypair_write(s_temp_aps_key_ptr, s_saved_keypair_i);
        s_temp_aps_key_ptr = NULL;
    }
}


static void check_tclk_update_done(zb_uint8_t unused)
{
    zb_aps_device_key_pair_set_t *aps_key;

    ZVUNUSED(unused);

    aps_key = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_VERIFIED_KEY);

    s_dut_request_key_attempt = 0;
    if (aps_key)
    {
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: TH has verified link key for DUT (successful tclk update)",
                  (FMT__0));

        switch (s_step_idx)
        {
        case SEND_TCLK_WITH_DTCLK:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
            ZB_GET_OUT_BUF_DELAYED(send_buffer_test);
            break;

        case SEND_CONF_KEY_PROTECTED_WITH_DTCLK:
            TRACE_MSG(TRACE_ERROR, "TEST: verify that dut on network", (FMT__0));
            ZB_CERT_HACKS().deliver_conf_key_cb = NULL;
            ZB_GET_OUT_BUF_DELAYED(send_buffer_test);
            break;

        case DROP_CONFIRM_KEY:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
            ZB_CERT_HACKS().drop_verify_key_indication = 0;
            ++s_step_idx;
            /* save data to NVRAM */
            save_test_state();
            break;

        case SUCCESSFUL_COMMISSIONING:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
            ZB_GET_OUT_BUF_DELAYED(send_aps_remove);
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
            ++s_step_idx;
            /* save data to NVRAM */
            save_test_state();
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZDO1, "TEST: DUT fails to update it's tclk", (FMT__0));

        switch (s_step_idx)
        {
        case SEND_TCLK_WITH_DTCLK:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
            break;

        case SEND_CONF_KEY_PROTECTED_WITH_DTCLK:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
            ZB_CERT_HACKS().deliver_conf_key_cb = NULL;
            break;

        case DROP_CONFIRM_KEY:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
            ZB_CERT_HACKS().drop_verify_key_indication = 0;
            break;

        case SUCCESSFUL_COMMISSIONING:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
            break;

        default:
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
        }
        ++s_step_idx;
        /* save data to NVRAM */
        save_test_state();
    }
}



static void prepare_tk_cmd(zb_buf_t *buf, int local_step_idx)
{
    zb_transport_key_dsc_pkt_t *dsc;
    zb_uint8_t size = sizeof(zb_uint8_t) * (1 + ZB_CCM_KEY_SIZE);
    zb_uint8_t new_key[ZB_CCM_KEY_SIZE];
    zb_uint8_t *byte_ptr;
    int i, modify_ieee_addr = 0;

    switch (local_step_idx)
    {
    case SEND_TCLK_WITH_APP_LINK_KEY:
        size += sizeof(zb_transport_key_app_pkt);
        break;
    case SEND_TCLK_WITH_NWK_KEY:
        size += sizeof(zb_transport_key_nwk_pkt);
        break;
    default:
        size += sizeof(zb_transport_key_tc_pkt);
    }

    ZB_BUF_INITIAL_ALLOC(buf, size, dsc);

    for (i = 0; i < ZB_CCM_KEY_SIZE / 2; ++i)
    {
        zb_uint16_t *key_ptr = (zb_uint16_t *) &new_key[i * 2];
        *key_ptr = zb_random();
    }

    switch (local_step_idx)
    {
    case SEND_TCLK_WITH_APP_LINK_KEY:
    {
        dsc->key_type = ZB_APP_LINK_KEY;
        ZB_IEEE_ADDR_COPY(dsc->key_data.app.partner_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
        dsc->key_data.app.initiator = 1;
    }
    break;

    case SEND_TCLK_WITH_NWK_KEY:
    {
        dsc->key_type = ZB_STANDARD_NETWORK_KEY;
        ZB_MEMCPY(new_key, g_nwk_key, ZB_CCM_KEY_SIZE);

        dsc->key_data.nwk.seq_number = ZB_NIB().active_key_seq_number;
        ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.dest_address, g_ieee_addr_dut);
        ZB_IEEE_ADDR_COPY(dsc->key_data.nwk.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
    }
    break;

    case SEND_TCLK_WITH_DTCLK:
    {
        ZB_MEMCPY(new_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
        zb_secur_update_key_pair(g_ieee_addr_dut, new_key, ZB_SECUR_UNIQUE_KEY, ZB_SECUR_UNVERIFIED_KEY);
    }
    /* FALLTHROUGH */
    default:
        dsc->key_type = ZB_TC_LINK_KEY;
        ZB_IEEE_ADDR_COPY(dsc->key_data.tc.dest_address, g_ieee_addr_dut);
        ZB_IEEE_ADDR_COPY(dsc->key_data.tc.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
    }

    ZB_MEMCPY(dsc->key, new_key, ZB_CCM_KEY_SIZE);

    if (local_step_idx == SEND_TCLK_WITH_WRONG_TC_ADDR)
    {
        byte_ptr = (zb_uint8_t *) &dsc->key_data.tc.source_address;
        modify_ieee_addr = 1;
    }
    else if (local_step_idx == SEND_TCLK_NOT_FOR_DUT)
    {
        byte_ptr = (zb_uint8_t *) &dsc->key_data.tc.dest_address;
        modify_ieee_addr = 1;
    }

    if (modify_ieee_addr)
    {
        for (i = 0; i < (int) sizeof(zb_ieee_addr_t); ++i, ++byte_ptr)
        {
            *byte_ptr = ~*byte_ptr;
        }
    }
}


static void send_tk_via_aps(zb_uint8_t param, zb_uint16_t local_step_idx)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t dest_short = zb_address_short_by_ieee(g_ieee_addr_dut);
    zb_secur_key_id_t secur_type = ZB_SECUR_KEY_LOAD_KEY;

    if (local_step_idx == SEND_TCLK_UNSECURED)
    {
        secur_type = ZB_NOT_SECUR;
    }

    prepare_tk_cmd(buf, local_step_idx);
    zb_aps_send_command(param, dest_short, APS_CMD_TRANSPORT_KEY, ZB_TRUE, secur_type);
}


static void send_tk_via_nwk(zb_uint8_t param, zb_uint16_t local_step_idx)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_uint16_t dest_short = zb_address_short_by_ieee(g_ieee_addr_dut);
    zb_aps_command_pkt_header_t *hdr;
    zb_uint32_t *mic_ptr;
    zb_secur_key_id_t secur_type = ZB_SECUR_KEY_LOAD_KEY;
    int send_unprotected = 0;
    int use_old_sec_counter = 0;
    int add_key_seqn = 0;

    switch (local_step_idx)
    {
    case SEND_TCLK_WITH_WRONG_PROTECTION:
        send_unprotected = 1;
        break;

    case SEND_TCLK_WITH_DATA_KEY_PROTECTION:
        secur_type = ZB_SECUR_DATA_KEY;
        break;

    case SEND_TCLK_WITH_KEY_TK_PROTECTION:
        secur_type = ZB_SECUR_KEY_TRANSPORT_KEY;
        break;

    case SEND_TCLK_WITH_NWK_KEY_PROTECTION:
        secur_type = ZB_SECUR_NWK_KEY;
        break;
    case SEND_TCLK_WITH_OLD_APS_SEC_COUNTER:
        use_old_sec_counter = 1;
        break;
    case SEND_TCLK_WITH_NWK_KEY:
        add_key_seqn = 1;
        break;
    }

    prepare_tk_cmd(buf, local_step_idx);
    zb_aps_command_add_secur(buf, APS_CMD_TRANSPORT_KEY, secur_type, dest_short);

    hdr = (zb_aps_command_pkt_header_t *)ZB_BUF_BEGIN(buf);
    ZB_APS_FC_SET_FRAME_TYPE(hdr->fc, ZB_APS_FRAME_COMMAND);
    hdr->aps_counter = ZB_AIB_APS_COUNTER();
    s_saved_aps_frame_counter = ZB_AIB_APS_COUNTER();
    ZB_AIB_APS_COUNTER_INC();

    if (send_unprotected)
    {
        ZB_BUF_ALLOC_RIGHT(buf, sizeof(zb_uint32_t), mic_ptr);
        *mic_ptr = 0x00000000;

        /* Remove APS encryption added by zb_aps_command_add_secur */
        buf->u.hdr.encrypt_type &= ~ZB_SECUR_APS_ENCR;
    }

    {
        zb_uint8_t *cnt_ptr = (zb_uint8_t *) hdr + sizeof(zb_aps_command_pkt_header_t) +
                              sizeof(zb_uint8_t);
        zb_uint32_t new_cnt_value;

        ZB_LETOH32(&new_cnt_value, cnt_ptr);
        s_saved_aps_secur_counter = new_cnt_value;
        if (use_old_sec_counter)
        {
            new_cnt_value = 0;
            ZB_HTOLE32(cnt_ptr, &new_cnt_value);
        }
    }

    if (add_key_seqn)
    {
        zb_uint8_t buffer[sizeof(zb_uint8_t) * (1 + ZB_CCM_KEY_SIZE) + sizeof(zb_transport_key_nwk_pkt)];
        zb_uint8_t *payload = (zb_uint8_t *) (hdr + 1) + sizeof(zb_aps_data_aux_nonce_frame_hdr_t);
        zb_uint8_t payload_size = sizeof(zb_uint8_t) * (1 + ZB_CCM_KEY_SIZE) +
                                  sizeof(zb_transport_key_nwk_pkt);

        zb_buf_smart_alloc_right(buf, sizeof(zb_uint8_t));
        ZB_MEMCPY(buffer, payload, payload_size);
        ZB_MEMCPY(payload + 1, buffer, payload_size);
        *payload = ZB_NIB().active_key_seq_number;
    }

    fill_nldereq(param, dest_short, ZB_TRUE);
    ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
}


static void send_buffer_test(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_buffer_test_req_param_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_buffer_test_req_param_t));

    if (s_step_idx == DTCLK_SEC_COUNTER_NEGATIVE_TEST)
    {
        TRACE_MSG(TRACE_ZDO1, "TEST: APS incoming frame counter test", (FMT__0));
        ZB_CERT_HACKS().secur_aps_counter_hack_cb = upon_aps_secured_frame_sending;
    }
    else if (s_step_idx == DTCLK_SEC_COUNTER_POSITIVE_TEST)
    {
        TRACE_MSG(TRACE_ZDO1, "TEST: APS outgoing frame counter test", (FMT__0));
    }
    else if (s_step_idx == SEND_CONF_KEY_PROTECTED_WITH_DTCLK)
    {
        TRACE_MSG(TRACE_ZDO1, "TEST: check that DUT is not on network", (FMT__0));
    }

    BUFFER_TEST_REQ_SET_DEFAULT(req);
    req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
    zb_tp_buffer_test_request(param, buffer_test_resp_cb);
}


static void buffer_test_resp_cb(zb_uint8_t status)
{
    if (s_step_idx == DTCLK_SEC_COUNTER_NEGATIVE_TEST)
    {
        ZB_CERT_HACKS().secur_aps_counter_hack_cb = NULL;
        ++s_step_idx;
        if (!status)
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
        }
        ZB_GET_OUT_BUF_DELAYED(send_buffer_test);
    }
    else if (s_step_idx == DTCLK_SEC_COUNTER_POSITIVE_TEST)
    {
        ++s_step_idx;
        if (!status)
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
        }
        /* save data to NVRAM */
        save_test_state();
    }
    else if (s_step_idx == SEND_CONF_KEY_PROTECTED_WITH_DTCLK)
    {
        ++s_step_idx;
        if (!status)
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP FAILED", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on command", (FMT__0));
            TRACE_MSG(TRACE_ERROR, "TEST: VERDICT - STEP PASSED", (FMT__0));
        }
        /* save data to NVRAM */
        save_test_state();
    }
}


static void send_aps_remove(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_apsme_remove_device_req_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsme_remove_device_req_t));
    zb_aps_device_key_pair_set_t *aps_key;

    aps_key = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_VERIFIED_KEY);
    if (aps_key)
    {
        ZB_MEMCPY(s_saved_link_key, aps_key->link_key, ZB_CCM_KEY_SIZE);
        s_temp_aps_key_ptr = aps_key;

        ZB_MEMCPY(aps_key->link_key, ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);
        ZB_SCHEDULE_ALARM(revert_key, 0, THC1_REVERT_KEY_DELAY);
    }

    ZB_IEEE_ADDR_COPY(req->parent_address, g_ieee_addr_dut);
    ZB_IEEE_ADDR_COPY(req->child_address, g_ieee_addr_dut);

    zb_secur_apsme_remove_device(param);

    TRACE_MSG(TRACE_ERROR, "TEST: Procedure completed - verify last step manually", (FMT__0));
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



static void save_test_state()
{
    zb_address_ieee_ref_t ieee_ref;
    zb_aps_device_key_pair_set_t *aps_key;
    zb_ret_t ret;

    TRACE_MSG(TRACE_APS3, ">>save_test_state: saved value = %d", (FMT__D, s_step_idx));

#ifndef USE_NVRAM_IN_TEST
    aps_key = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_PROVISIONAL_KEY);
    if (aps_key)
    {
        aps_key->incoming_frame_counter = 0;
        aps_key->outgoing_frame_counter = 0;
    }

    ret = zb_address_by_ieee(g_ieee_addr_dut, ZB_FALSE, ZB_FALSE, &ieee_ref);
    if (ret == RET_OK)
    {
        zb_uint8_t n = ZG->nwk.neighbor.addr_to_neighbor[ieee_ref];
        zb_uint8_t dev_type = ZG->nwk.neighbor.neighbor[n].device_type;
        zb_nwk_forget_device(ieee_ref);
        if (dev_type == ZB_NWK_DEVICE_TYPE_ROUTER)
        {
            ZB_NIB().router_child_num--;
        }
        else
        {
            ZB_NIB().ed_child_num--;
        }
        zb_get_out_buf_delayed(zb_nwk_update_beacon_payload);
    }
#else
    /* Need factory new TH ZC that able to save it's test state between reboots */
    ZVUNUSED(ieee_ref);
    ZVUNUSED(aps_key);
    ZVUNUSED(ret);
    zb_nvram_clear();
    zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif

    TRACE_MSG(TRACE_APS3, "<<save_test_state", (FMT__0));
}


#ifdef USE_NVRAM_IN_TEST
static void read_thc_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
    zb_ret_t ret;
    thc_nvram_app_dataset_t ds;

    if (payload_length == sizeof(ds))
    {
        ret = zb_osif_nvram_read(page, pos, (zb_uint8_t *)&ds, sizeof(ds));
        if (ret == RET_OK)
        {
            s_step_idx = ds.current_test_step;
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "read_thc_app_data_cb: nvram read error %d", (FMT__D, ret));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "read_thc_app_data_cb ds mismatch: got %d wants %d",
                  (FMT__D_D, payload_length, sizeof(ds)));
    }
}


static zb_ret_t write_thc_app_data_cb(zb_uint8_t page, zb_uint32_t pos)
{
    zb_ret_t ret = RET_OK;
    thc_nvram_app_dataset_t ds;

    TRACE_MSG(TRACE_APS3, ">>write_thc_app_data_cb", (FMT__0));

    ds.current_test_step = (zb_uint32_t) s_step_idx;
    ret = zb_osif_nvram_write(page, pos, (zb_uint8_t *)&ds, sizeof(ds));

    TRACE_MSG(TRACE_APS3, "<<write_thc_app_data_cb ret %d", (FMT__D, ret));
    return ret;
}


static zb_uint16_t thc_nvram_get_app_data_size_cb()
{
    return sizeof(thc_nvram_app_dataset_t);
}
#endif /* USE_NVRAM_IN_TEST */

/*! @} */
