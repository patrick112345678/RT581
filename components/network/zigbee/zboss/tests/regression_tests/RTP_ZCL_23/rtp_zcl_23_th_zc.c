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

#define ZB_TEST_NAME RTP_ZCL_23_TH_ZC

#define ZB_TEST_TAG_1 RTP_ZCL
#define ZB_TEST_TAG_2 RTP_WWAH

#define ZB_TRACE_FILE_ID 64918

#include "zboss_api.h"
#include "zb_secur_api.h"
#include "zb_common.h"
#include "../nwk/nwk_internal.h"
#include "device_th.h"
#include "rtp_zcl_23_common.h"
#include "../common/zb_reg_test_globals.h"
#include "se/zb_se_keep_alive.h"

#ifndef NCP_MODE_HOST

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error ZB_USE_NVRAM is not compiled!
#endif

#ifndef ZB_ZCL_SUPPORT_CLUSTER_WWAH
#error ZB_ZCL_SUPPORT_CLUSTER_WWAH is not compiled!
#endif

#ifndef ZB_STACK_REGRESSION_TESTING_API
#error define ZB_STACK_REGRESSION_TESTING_API
#endif

/** [DECLARE_CLUSTERS] */

/*********************  Clusters' attributes  **************************/

/* Identify cluster attributes data */
static zb_uint16_t g_attr_identify_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

/* Time cluster attributes data */
static zb_time_t g_attr_time_time = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_uint8_t g_attr_time_time_status = ZB_ZCL_TIME_TIME_STATUS_DEFAULT_VALUE;
static zb_int32_t g_attr_time_time_zone = ZB_ZCL_TIME_TIME_ZONE_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_dst_start = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_uint32_t g_attr_time_dst_end = ZB_ZCL_TIME_TIME_MIN_VALUE;
static zb_int32_t g_attr_time_dst_shift = ZB_ZCL_TIME_DST_SHIFT_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_standard_time = ZB_ZCL_TIME_STANDARD_TIME_DEFAULT_VALUE;
static zb_uint32_t g_attr_time_local_time = ZB_ZCL_TIME_LOCAL_TIME_DEFAULT_VALUE;
static zb_time_t g_attr_time_last_set_time = ZB_ZCL_TIME_LAST_SET_TIME_DEFAULT_VALUE;
static zb_time_t g_attr_time_valid_until_time = ZB_ZCL_TIME_VALID_UNTIL_TIME_DEFAULT_VALUE;
static zb_uint8_t g_attr_keep_alive_base = 1;
static zb_uint16_t g_attr_keep_alive_jitter = 15;

/* Keep-Alive cluster attributes */
ZB_ZCL_DECLARE_KEEP_ALIVE_ATTR_LIST_FULL(rtp_zcl_23_th_zc_keep_alive_attr_list,
        &g_attr_keep_alive_base,
        &g_attr_keep_alive_jitter);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_23_th_zc_identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(rtp_zcl_23_th_zc_time_attr_list, &g_attr_time_time, &g_attr_time_time_status, &g_attr_time_time_zone, &g_attr_time_dst_start, &g_attr_time_dst_end, &g_attr_time_dst_shift, &g_attr_time_standard_time, &g_attr_time_local_time, &g_attr_time_last_set_time, &g_attr_time_valid_until_time);

/* Poll Control cluster attributes data */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST_CLIENT(rtp_zcl_23_th_zc_poll_control_attr_list);

/* OTA Upgrade server cluster attributes data */
static zb_uint8_t g_query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_MAX_VALUE;
static zb_uint32_t g_current_time = 0x12345678;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(rtp_zcl_23_th_zc_ota_upgrade_attr_list, &g_query_jitter, &g_current_time, 1);
ZB_ZCL_DECLARE_WWAH_CLIENT_ATTRIB_LIST(rtp_zcl_23_th_zc_wwah_client_attr_list);

/********************* Declare device **************************/
ZB_HA_DECLARE_WWAH_CLUSTER_LIST_ZC(rtp_zcl_23_th_zc_wwah_ha_clusters,
                                   rtp_zcl_23_th_zc_identify_attr_list,
                                   rtp_zcl_23_th_zc_time_attr_list,
                                   rtp_zcl_23_th_zc_keep_alive_attr_list,
                                   rtp_zcl_23_th_zc_poll_control_attr_list,
                                   rtp_zcl_23_th_zc_ota_upgrade_attr_list,
                                   rtp_zcl_23_th_zc_wwah_client_attr_list);
ZB_HA_DECLARE_WWAH_EP_ZC(rtp_zcl_23_th_zc_wwah_ha_ep, ZC_HA_EP, rtp_zcl_23_th_zc_wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(rtp_zcl_23_th_zc_device_ctx, rtp_zcl_23_th_zc_wwah_ha_ep);

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_DUT_ZC;

static zb_uint16_t g_dut_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
static zb_ieee_addr_t g_dut_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint16_t g_dut_ep = ZB_APS_BROADCAST_ENDPOINT_NUMBER;

static zb_uint8_t g_pending_new_channel = 0;
static zb_uint16_t g_pending_new_pan_id = 0x0000;


/*********************  Device-specific functions  **************************/

/**
 * Test 19
 */
typedef enum reg_test_step_e
{
    /* Initial configuration */
    REG_TEST_STEP_FIND_WWAH_SERVER,

    /* case a) read PendingNetworkUpdateChannel, PendingNetworkUpdatePANID
     * and verify that their values are 0xFF and 0xFFFF */
    REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_1,

    /* case b) send Set Pending Network Update with current channel and PANID */
    REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_1,

    /* case c) read PendingNetworkUpdateChannel, PendingNetworkUpdatePANID
     * and verify that their values are equal to current channel and PANID */
    REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_2,

    /* case d) send NWK Mgmt Network Update with new channel */
    /* DUT should not attempt to change channel (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_1,

    /* case e) send NWK Mgmt Network Update with new PANID */
    /* DUT should not attempt to change PANID (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_PANID_1,

    /* case f) send Set Pending Network Update with new channel and current PANID */
    REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_2,

    /* case g) read PendingNetworkUpdateChannel, PendingNetworkUpdatePANID
     * and verify that their values are equal to new channel and current PANID */
    REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_3,

    /* case h) send NWK Mgmt Network Update with a different new channel */
    /* DUT should not attempt to change channel (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_2_1,

    /* case i) send NWK Mgmt Network Update with new PANID */
    /* DUT should not attempt to change PANID (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_PANID_2,

    /* case j) send NWK Mgmt Network Update with a new channel */
    /* DUT and TH should change the channel */
    REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_2_2,

    /* case k) send Set Pending Network Update with current channel and new PANID */
    REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_3,

    /* case k (additional) ) read PendingNetworkUpdateChannel, PendingNetworkUpdatePANID
     * and verify that their values are equal to current channel and new PANID */
    REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_4,

    /* case l) send NWK Mgmt Network Update with new channel */
    /* DUT should not attempt to change channel (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_3,

    /* case m) send NWK Mgmt Network Update with a different new PANID */
    /* DUT should not attempt to change PANID (rejoin) */
    REG_TEST_STEP_NETWORK_UPDATE_PANID_3_1,

    /* case n) send NWK Mgmt Network Update with the new PANID */
    /* DUT and TH should change the PANID */
    REG_TEST_STEP_NETWORK_UPDATE_PANID_3_2,

    /* case n (additional) ) read PendingNetworkUpdateChannel, PendingNetworkUpdatePANID
     * and verify that their values are equal to current channel and new PANID */
    REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_5,

    REG_TEST_STEPS_COUNT
} reg_test_step_t;

static void test_step_next(zb_uint8_t param);
static void test_step_next_extended(zb_uint8_t param, zb_uint16_t delay);
static void test_step_dispatch(zb_uint8_t param);

static void send_read_attr2(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID1, zb_uint16_t attributeID2);

static reg_test_step_t g_test_step = REG_TEST_STEP_FIND_WWAH_SERVER;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | 0xffff);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_long_address(g_ieee_addr_zc);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));

    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_23_th_zc_device_ctx);

    zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_CLIENT);

    if (zboss_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
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
    zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(param);

    TRACE_MSG(TRACE_APP1, ">> zboss_signal_handler %hd sig %hd status %hd",
              (FMT__H_H_H, param, sig, ZB_GET_APP_SIGNAL_STATUS(param)));

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        zboss_start_continue();
        break;

    case ZB_SIGNAL_DEVICE_FIRST_START:
    case ZB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device started, status %d", (FMT__D, status));

        if (status == 0)
        {
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
    {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        g_dut_short_addr = dev_annce_params->device_short_addr;
        ZB_IEEE_ADDR_COPY(g_dut_ieee_addr, dev_annce_params->ieee_addr);

        test_step_register(test_step_dispatch, 0, RTP_ZCL_23_STEP_TIME_ZC);
        test_control_start(TEST_MODE, RTP_ZCL_23_STEP_1_DELAY_ZC);
    }
    break;

    case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "signal: ZB_BDB_SIGNAL_STEERING, status %d", (FMT__D, status));
        break; /* ZB_BDB_SIGNAL_STEERING */

    default:
        TRACE_MSG(TRACE_APS1, "Unknown signal %d, status %d", (FMT__D_D, sig, status));
        break;
    }

    if (param != ZB_BUF_INVALID)
    {
        zb_buf_free(param);
    }
}


static void test_step_send_wwah_match_desc_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;

    TRACE_MSG(TRACE_APP1, ">> test_step_send_wwah_match_desc_cb, param %hd, status %d", (FMT__H_D, param, resp->status));

    ZB_ASSERT(resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len == 1);
    ZB_ASSERT(g_dut_short_addr == resp->nwk_addr);

    g_dut_ep = *((zb_uint8_t *)(resp + 1));

    ZB_SCHEDULE_APP_CALLBACK(test_step_next, param);

    TRACE_MSG(TRACE_APP1, "<< test_step_send_wwah_match_desc_cb", (FMT__0));
}


static void test_step_send_wwah_match_desc(zb_uint8_t param)
{
    zb_zdo_match_desc_param_t *req;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_step_send_wwah_match_desc);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_step_send_wwah_match_desc, param %hd", (FMT__H, param));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

    req->nwk_addr = g_dut_short_addr;
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_WWAH;

    zb_zdo_match_desc_req(param, test_step_send_wwah_match_desc_cb);

    TRACE_MSG(TRACE_APP1, "<< test_step_send_wwah_match_desc", (FMT__0));
}


static void test_step_read_wwah_pending_network_update_attrs(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_read_wwah_pending_network_update_attrs, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);
    send_read_attr2(param, ZB_ZCL_CLUSTER_ID_WWAH,
                    ZB_ZCL_ATTR_WWAH_PENDING_NETWORK_UPDATE_CHANNEL_ID,
                    ZB_ZCL_ATTR_WWAH_PENDING_NETWORK_UPDATE_PANID_ID);

    ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);

    TRACE_MSG(TRACE_APP1, "<< test_step_read_wwah_pending_network_update_attrs", (FMT__0));
}


static zb_uint8_t test_get_next_channel_id(zb_uint8_t channel_id)
{
    zb_uint8_t next_channel_id;

    TRACE_MSG(TRACE_APP1, ">> test_get_next_channel_id, channel_id %hd", (FMT__H, channel_id));

    next_channel_id = ((channel_id - 11 + 1) % 21) + 11;

    TRACE_MSG(TRACE_APP1, "<< test_get_next_channel_id, next_channel_id %hd", (FMT__H, next_channel_id));

    return next_channel_id;
}


static zb_uint16_t test_get_next_pan_id(zb_uint16_t pan_id)
{
    zb_uint16_t next_pan_id;

    TRACE_MSG(TRACE_APP1, ">> test_get_next_pan_id, channel_id %hd", (FMT__H, pan_id));

    next_pan_id = pan_id + 1;

    TRACE_MSG(TRACE_APP1, "<< test_get_next_pan_id, next_channel_id %hd", (FMT__H, next_pan_id));

    return next_pan_id;
}


static void test_step_set_pending_network_update_1(zb_uint8_t param)
{
    g_pending_new_channel = zb_get_current_channel();
    g_pending_new_pan_id = zb_get_pan_id();

    TRACE_MSG(TRACE_APP1, ">> test_step_set_pending_network_update_1, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    ZB_ZCL_WWAH_SEND_SET_PENDING_NETWORK_UPDATE(param, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, g_pending_new_channel, &g_pending_new_pan_id);

    ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);

    TRACE_MSG(TRACE_APP1, "<< test_step_set_pending_network_update_1", (FMT__0));
}


static void test_step_nwk_update_channel(zb_uint8_t param, zb_uint8_t new_channel)
{
    zb_zdo_mgmt_nwk_update_req_t *req;

    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_channel, param %hd, current_channel %hd, new_channel %hd",
              (FMT__H_H_H, param, zb_get_current_channel(), new_channel));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    req = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_nwk_update_req_t);

    req->hdr.scan_channels = ( 1l << new_channel );
    req->hdr.scan_duration = RTP_ZCL_23_TEST_SCAN_DURATION;
    req->scan_count = RTP_ZCL_23_TEST_SCAN_COUNT;
    req->dst_addr = g_dut_short_addr;

    zb_zdo_mgmt_nwk_update_req(param, NULL);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_channel", (FMT__0));
}


static void test_step_nwk_update_pan_id(zb_uint8_t param, zb_uint16_t new_pan_id)
{
    zb_nwk_update_cmd_t *upd;
    zb_nwk_hdr_t *nwhdr;

    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_pan_id, param %hd, current_pan_id 0x%x, new_pan_id 0x%x",
              (FMT__H_D_D, param, zb_get_pan_id(), new_pan_id));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    nwhdr = nwk_alloc_and_fill_hdr(param, ZB_PIBCACHE_NETWORK_ADDRESS(), g_dut_short_addr,
                                   ZB_FALSE, ZB_NIB_SECURITY_LEVEL(), ZB_TRUE, ZB_TRUE);

    nwhdr->radius = ZB_NIB_MAX_DEPTH();

    upd = (zb_nwk_update_cmd_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_NETWORK_UPDATE, sizeof(zb_nwk_update_cmd_t));

    /* Update type: PAN Identifier Update, Update Information Count: 1 */
    upd->command_options = 1;
    ZB_IEEE_ADDR_COPY(upd->epid, ZB_NIB_EXT_PAN_ID());
    ++ZB_NIB_UPDATE_ID();
    upd->update_id = ZB_NIB_UPDATE_ID();
    ZB_HTOLE16(&upd->new_panid, &new_pan_id);

    (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_NSDU_HANDLE);
    ZB_SCHEDULE_APP_CALLBACK(zb_nwk_forward, param);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_pan_id", (FMT__0));
}


static void test_step_nwk_update_channel_1(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_channel_1, param %hd", (FMT__H, param));

    test_step_nwk_update_channel(param, test_get_next_channel_id(g_pending_new_channel));
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_channel_1", (FMT__0));
}


static void test_step_nwk_update_pan_id_1(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_pan_id_1, param %hd", (FMT__H, param));

    test_step_nwk_update_pan_id(param, test_get_next_pan_id(g_pending_new_pan_id));
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_pan_id_1", (FMT__0));
}


static void test_step_set_pending_network_update_2(zb_uint8_t param)
{
    g_pending_new_channel = test_get_next_channel_id(zb_get_current_channel());
    g_pending_new_pan_id = zb_get_pan_id();

    TRACE_MSG(TRACE_APP1, ">> test_step_set_pending_network_update_2, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    ZB_ZCL_WWAH_SEND_SET_PENDING_NETWORK_UPDATE(param, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, g_pending_new_channel, &g_pending_new_pan_id);

    ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);

    TRACE_MSG(TRACE_APP1, "<< test_step_set_pending_network_update_2", (FMT__0));
}


static void test_step_nwk_update_channel_2_1(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_channel_2_1, param %hd", (FMT__H, param));

    test_step_nwk_update_channel(param, test_get_next_channel_id(g_pending_new_channel));
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_channel_2_1", (FMT__0));
}


static void test_step_nwk_update_pan_id_2(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_pan_id_2, param %hd", (FMT__H, param));

    test_step_nwk_update_pan_id(param, test_get_next_pan_id(g_pending_new_pan_id));
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_pan_id_2", (FMT__0));
}


static void test_step_nwk_update_channel_2_2(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_channel_2_2, param %hd", (FMT__H, param));

    test_step_nwk_update_channel(param, g_pending_new_channel);
    ZB_SCHEDULE_APP_ALARM(zb_zdo_do_set_channel, g_pending_new_channel, RTP_ZCL_23_DELAY_TO_REAL_CHANNEL_CHANGE);

    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_channel_2_2", (FMT__0));
}


static void test_step_set_pending_network_update_3(zb_uint8_t param)
{
    g_pending_new_channel = zb_get_current_channel();
    g_pending_new_pan_id = test_get_next_pan_id(zb_get_pan_id());

    TRACE_MSG(TRACE_APP1, ">> test_step_set_pending_network_update_3, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    ZB_ZCL_WWAH_SEND_SET_PENDING_NETWORK_UPDATE(param, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL, g_pending_new_channel, &g_pending_new_pan_id);

    ZB_SCHEDULE_APP_CALLBACK(test_step_next, ZB_BUF_INVALID);

    TRACE_MSG(TRACE_APP1, "<< test_step_set_pending_network_update_3", (FMT__0));
}


static void test_step_nwk_update_channel_3(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_channel_3, param %hd", (FMT__H, param));

    test_step_nwk_update_channel(param, test_get_next_channel_id(g_pending_new_channel));

    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_channel_3", (FMT__0));
}


static void test_step_nwk_update_pan_id_3_1(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_pan_id_3_1, param %hd", (FMT__H, param));

    test_step_nwk_update_pan_id(param, test_get_next_pan_id(g_pending_new_pan_id));
    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_pan_id_3_1", (FMT__0));
}


static void test_update_real_pan_id(zb_uint8_t param)
{
    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_update_real_pan_id);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_update_real_pan_id, param %hd, new_pan_id 0x%x", (FMT__H_D, param, g_pending_new_pan_id));

    ZB_PIBCACHE_PAN_ID() = g_pending_new_pan_id;
    ZG->nwk.handle.new_panid = 0;
    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_PANID, &ZB_PIBCACHE_PAN_ID(), 2, NULL);

    TRACE_MSG(TRACE_APP1, "<< test_update_real_pan_id", (FMT__0));
}


static void test_step_nwk_update_pan_id_3_2(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_nwk_update_pan_id_3_2, param %hd", (FMT__H, param));

    test_step_nwk_update_pan_id(param, g_pending_new_pan_id);
    ZB_SCHEDULE_APP_ALARM(test_update_real_pan_id, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_TO_REAL_PANID_CHANGE);

    ZB_SCHEDULE_APP_CALLBACK2(test_step_next_extended, ZB_BUF_INVALID, RTP_ZCL_23_DELAY_AFTER_NWK_UPDATE);

    TRACE_MSG(TRACE_APP1, "<< test_step_nwk_update_pan_id_3_2", (FMT__0));
}


static void test_step_dispatch(zb_uint8_t param)
{
    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_step_dispatch);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_step_dispatch, param %hd, current_step %d", (FMT__H_D, param, g_test_step));

    switch (g_test_step)
    {
    case REG_TEST_STEP_FIND_WWAH_SERVER:
        ZB_SCHEDULE_APP_CALLBACK(test_step_send_wwah_match_desc, param);
        break;

    case REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_pending_network_update_attrs, param);
        break;

    case REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_set_pending_network_update_1, param);
        break;

    case REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_pending_network_update_attrs, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_channel_1, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_PANID_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_pan_id_1, param);
        break;

    case REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_set_pending_network_update_2, param);
        break;

    case REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_3:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_pending_network_update_attrs, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_2_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_channel_2_1, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_PANID_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_pan_id_2, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_2_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_channel_2_2, param);
        break;

    case REG_TEST_STEP_SET_PENDING_NETWORK_UPDATE_3:
        ZB_SCHEDULE_APP_CALLBACK(test_step_set_pending_network_update_3, param);
        break;

    case REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_4:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_pending_network_update_attrs, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_CHANNEL_3:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_channel_3, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_PANID_3_1:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_pan_id_3_1, param);
        break;

    case REG_TEST_STEP_NETWORK_UPDATE_PANID_3_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_nwk_update_pan_id_3_2, param);
        break;

    case REG_TEST_STEP_READ_WWAH_PENDING_NETWORK_UPDATE_ATTRS_5:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_wwah_pending_network_update_attrs, param);
        break;

    default:
        TRACE_MSG(TRACE_APP1, "test procedure is completed", (FMT__0));
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< test_step_dispatch", (FMT__0));
}


static void test_step_next(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_dispatch", (FMT__0));

    g_test_step++;

    ZB_SCHEDULE_APP_ALARM(test_step_dispatch, param, RTP_ZCL_23_STEP_TIME_ZC);

    TRACE_MSG(TRACE_APP1, "<< test_step_dispatch", (FMT__0));
}


static void test_step_next_extended(zb_uint8_t param, zb_uint16_t delay)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_next_extended", (FMT__0));

    g_test_step++;

    ZB_SCHEDULE_APP_ALARM(test_step_dispatch, param, delay);

    TRACE_MSG(TRACE_APP1, "<< test_step_next_extended", (FMT__0));
}


static void send_read_attr2(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID1, zb_uint16_t attributeID2)
{
    zb_uint8_t *cmd_ptr;
    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID1));
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID2));
    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
        (buffer), cmd_ptr, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, g_dut_ep, ZC_HA_EP,
        ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

#endif /* NCP_MODE_HOST */