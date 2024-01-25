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

#define ZB_TEST_NAME RTP_ZCL_21_TH_ZC

#define ZB_TEST_TAG_1 RTP_ZCL
#define ZB_TEST_TAG_2 RTP_WWAH

#define ZB_TRACE_FILE_ID 64916

#include "zboss_api.h"
#include "zb_common.h"
#include "device_th.h"
#include "rtp_zcl_21_common.h"
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
ZB_ZCL_DECLARE_KEEP_ALIVE_ATTR_LIST_FULL(rtp_zcl_21_th_zc_keep_alive_attr_list,
        &g_attr_keep_alive_base,
        &g_attr_keep_alive_jitter);

/* Identify cluster attributes */
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(rtp_zcl_21_th_zc_identify_attr_list, &g_attr_identify_identify_time);
ZB_ZCL_DECLARE_TIME_ATTRIB_LIST(rtp_zcl_21_th_zc_time_attr_list, &g_attr_time_time, &g_attr_time_time_status, &g_attr_time_time_zone, &g_attr_time_dst_start, &g_attr_time_dst_end, &g_attr_time_dst_shift, &g_attr_time_standard_time, &g_attr_time_local_time, &g_attr_time_last_set_time, &g_attr_time_valid_until_time);

/* Poll Control cluster attributes data */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST_CLIENT(rtp_zcl_21_th_zc_poll_control_attr_list);

/* OTA Upgrade server cluster attributes data */
static zb_uint8_t g_query_jitter = ZB_ZCL_OTA_UPGRADE_QUERY_JITTER_MAX_VALUE;
static zb_uint32_t g_current_time = 0x12345678;

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST_SERVER(rtp_zcl_21_th_zc_ota_upgrade_attr_list, &g_query_jitter, &g_current_time, 1);
ZB_ZCL_DECLARE_WWAH_CLIENT_ATTRIB_LIST(rtp_zcl_21_th_zc_wwah_client_attr_list);

/********************* Declare device **************************/
ZB_HA_DECLARE_WWAH_CLUSTER_LIST_ZC(rtp_zcl_21_th_zc_wwah_ha_clusters,
                                   rtp_zcl_21_th_zc_identify_attr_list,
                                   rtp_zcl_21_th_zc_time_attr_list,
                                   rtp_zcl_21_th_zc_keep_alive_attr_list,
                                   rtp_zcl_21_th_zc_poll_control_attr_list,
                                   rtp_zcl_21_th_zc_ota_upgrade_attr_list,
                                   rtp_zcl_21_th_zc_wwah_client_attr_list);
ZB_HA_DECLARE_WWAH_EP_ZC(rtp_zcl_21_th_zc_wwah_ha_ep, ZC_HA_EP, rtp_zcl_21_th_zc_wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(rtp_zcl_21_th_zc_device_ctx, rtp_zcl_21_th_zc_wwah_ha_ep);

static zb_uint8_t g_nwk_key[16] = ZB_REG_TEST_DEFAULT_NWK_KEY;
static zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_DUT_ZC;

static zb_uint16_t g_dut_short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
static zb_ieee_addr_t g_dut_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static zb_uint16_t g_dut_ep = ZB_APS_BROADCAST_ENDPOINT_NUMBER;

/*********************  Device-specific functions  **************************/

typedef enum reg_test_step_e
{
    REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED,
    REG_TEST_STEP_ENABLE_BAD_PARENT_DISCOVERY,
    REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED_2,
    REG_TEST_STEP_READ_CHECKIN_INTERVAL,
    REG_TEST_STEP_WRITE_CHECKIN_INTERVAL,
    REG_TEST_STEP_PERFORM_POLL_CONTROL_BINDING,
    REG_TEST_STEPS_COUNT
} reg_test_step_t;

static void test_send_wwah_match_desc_cb(zb_uint8_t param);
static void test_send_wwah_match_desc(zb_uint8_t param);

static void test_step_dispatch(zb_uint8_t param);

static void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID);
static void send_write_attr(zb_bufid_t buffer, zb_uint16_t clusterID,
                            zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal);

static reg_test_step_t g_test_step = REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED;

MAIN()
{
    ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP);
    ZB_SET_TRACE_LEVEL(4);
    ARGV_UNUSED;

    ZB_INIT("zdo_th_zc");

    zb_set_long_address(g_ieee_addr_zc);

    zb_reg_test_set_common_channel_settings();
    zb_set_network_coordinator_role((1l << TEST_CHANNEL));
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    zb_set_nvram_erase_at_start(ZB_TRUE);

    ZB_AF_REGISTER_DEVICE_CTX(&rtp_zcl_21_th_zc_device_ctx);

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
            ZB_SCHEDULE_APP_ALARM(test_send_wwah_match_desc, 0, 5 * ZB_TIME_ONE_SECOND);
        }
        break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
    {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        g_dut_short_addr = dev_annce_params->device_short_addr;
        ZB_IEEE_ADDR_COPY(g_dut_ieee_addr, dev_annce_params->ieee_addr);

        /* REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_1_TIME_ZC);
        /* REG_TEST_STEP_ENABLE_BAD_PARENT_DISCOVERY */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_2_TIME_ZC);
        /* REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED_2 */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_3_TIME_ZC);
        /* REG_TEST_STEP_READ_CHECKIN_INTERVAL */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_4_TIME_ZC);
        /* REG_TEST_STEP_WRITE_CHECKIN_INTERVAL */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_5_TIME_ZC);
        /* REG_TEST_STEP_PERFORM_POLL_CONTROL_BINDING */
        test_step_register(test_step_dispatch, 0, RTP_ZCL_21_STEP_6_TIME_ZC);

        test_control_start(TEST_MODE, RTP_ZCL_21_STEP_1_DELAY_ZC);
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


static void test_send_wwah_match_desc_cb(zb_uint8_t param)
{
    zb_uint8_t *zdp_cmd = zb_buf_begin(param);
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zdp_cmd;

    TRACE_MSG(TRACE_APP1, ">> test_send_wwah_match_desc_cb, param %hd, status %d", (FMT__H_D, param, resp->status));

    ZB_ASSERT(resp->status == ZB_ZDP_STATUS_SUCCESS && resp->match_len == 1);
    ZB_ASSERT(g_dut_short_addr == resp->nwk_addr);

    g_dut_ep = *((zb_uint8_t *)(resp + 1));

    zb_buf_free(param);

    ZB_SCHEDULE_APP_CALLBACK(test_step_dispatch, 0);

    TRACE_MSG(TRACE_APP1, "<< test_send_wwah_match_desc_cb", (FMT__0));
}


static void test_send_wwah_match_desc(zb_uint8_t param)
{
    zb_zdo_match_desc_param_t *req;

    if (param == ZB_BUF_INVALID)
    {
        zb_buf_get_out_delayed(test_send_wwah_match_desc);
        return;
    }

    TRACE_MSG(TRACE_APP1, ">> test_send_wwah_match_desc, param %hd", (FMT__H, param));

    req = zb_buf_initial_alloc(param, sizeof(zb_zdo_match_desc_param_t) + 1 * sizeof(zb_uint16_t));

    req->nwk_addr = g_dut_short_addr;
    req->addr_of_interest = req->nwk_addr;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_WWAH;

    zb_zdo_match_desc_req(param, test_send_wwah_match_desc_cb);

    TRACE_MSG(TRACE_APP1, "<< test_send_wwah_match_desc", (FMT__0));
}


static void test_step_read_bad_parent_discovery_enabled(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_read_bad_parent_discovery_enabled, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);
    send_read_attr(param, ZB_ZCL_CLUSTER_ID_WWAH, ZB_ZCL_ATTR_WWAH_WWAH_BAD_PARENT_RECOVERY_ENABLED_ID);

    g_test_step++;

    TRACE_MSG(TRACE_APP1, "<< test_step_read_bad_parent_discovery_enabled", (FMT__0));
}


static void test_step_read_checkin_interval(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_read_checkin_interval, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);
    send_read_attr(param, ZB_ZCL_CLUSTER_ID_POLL_CONTROL, ZB_ZCL_ATTR_POLL_CONTROL_CHECKIN_INTERVAL_ID);

    g_test_step++;

    TRACE_MSG(TRACE_APP1, "<< test_step_read_checkin_interval", (FMT__0));
}


static void test_step_write_checkin_interval(zb_uint8_t param)
{
    zb_uint32_t check_in_interval = 0x80;

    TRACE_MSG(TRACE_APP1, ">> test_step_write_checkin_interval, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    send_write_attr(param, ZB_ZCL_CLUSTER_ID_POLL_CONTROL, ZB_ZCL_ATTR_POLL_CONTROL_CHECKIN_INTERVAL_ID,
                    ZB_ZCL_ATTR_TYPE_U32, (zb_uint8_t *)&check_in_interval);

    g_test_step++;

    TRACE_MSG(TRACE_APP1, "<< test_step_write_checkin_interval", (FMT__0));
}


static void test_step_enable_bad_parent_discovery(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> test_step_enable_bad_parent_discovery, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);
    ZB_ZCL_WWAH_SEND_ENABLE_WWAH_BAD_PARENT_RECOVERY(param, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
            g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL);

    g_test_step++;

    TRACE_MSG(TRACE_APP1, "<< test_step_enable_bad_parent_discovery", (FMT__0));
}


static void test_step_perform_poll_control_binding(zb_uint8_t param)
{
    zb_zdo_bind_req_param_t *req;

    TRACE_MSG(TRACE_APP2, ">> test_step_perform_poll_control_binding, param %hd", (FMT__H, param));

    ZB_ASSERT(param != ZB_BUF_INVALID);

    req = ZB_BUF_GET_PARAM(param, zb_zdo_bind_req_param_t);
    ZB_IEEE_ADDR_COPY(req->src_address, g_dut_ieee_addr);

    req->src_endp = 1;
    req->cluster_id = ZB_ZCL_CLUSTER_ID_POLL_CONTROL;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    zb_get_long_address(req->dst_address.addr_long);

    req->dst_endp = ZC_HA_EP;
    req->req_dst_addr = g_dut_short_addr;

    zb_zdo_bind_req(param, NULL);

    g_test_step++;

    TRACE_MSG(TRACE_APP2, "<< test_step_perform_poll_control_bindings", (FMT__0));
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
    case REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_bad_parent_discovery_enabled, param);
        break;

    case REG_TEST_STEP_ENABLE_BAD_PARENT_DISCOVERY:
        ZB_SCHEDULE_APP_CALLBACK(test_step_enable_bad_parent_discovery, param);
        break;

    case REG_TEST_STEP_READ_BAD_PARENT_DISCOVERY_ENABLED_2:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_bad_parent_discovery_enabled, param);
        break;

    case REG_TEST_STEP_READ_CHECKIN_INTERVAL:
        ZB_SCHEDULE_APP_CALLBACK(test_step_read_checkin_interval, param);
        break;

    case REG_TEST_STEP_WRITE_CHECKIN_INTERVAL:
        ZB_SCHEDULE_APP_CALLBACK(test_step_write_checkin_interval, param);
        break;

    case REG_TEST_STEP_PERFORM_POLL_CONTROL_BINDING:
        ZB_SCHEDULE_APP_CALLBACK(test_step_perform_poll_control_binding, param);
        break;

    default:
        TRACE_MSG(TRACE_APP1, "test procedure is completed", (FMT__0));
        break;
    }

    TRACE_MSG(TRACE_APP1, "<< test_step_dispatch", (FMT__0));
}


static void send_read_attr(zb_bufid_t buffer, zb_uint16_t clusterID, zb_uint16_t attributeID)
{
    zb_uint8_t *cmd_ptr;
    ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));
    ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(
        (buffer), cmd_ptr, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT, g_dut_ep, ZC_HA_EP,
        ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}


static void send_write_attr(zb_bufid_t buffer, zb_uint16_t clusterID,
                            zb_uint16_t attributeID, zb_uint8_t attrType, zb_uint8_t *attrVal)
{
    zb_uint8_t *cmd_ptr;
    ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
    ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, (attributeID), (attrType), (attrVal));
    ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ((buffer), cmd_ptr, g_dut_short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                       g_dut_ep, ZC_HA_EP, ZB_AF_HA_PROFILE_ID, (clusterID), NULL);
}

#endif /* NCP_MODE_HOST */