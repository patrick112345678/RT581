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
/* PURPOSE: TH GPS
*/

#define ZB_TEST_NAME GPS_COMMISSIONING_BIDIR_ADD_TH_GPS
#define ZB_TRACE_FILE_ID 41496
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_mac_globals.h"
#include "zb_secur_api.h"
#include "zgp/zgp_internal.h"

#include "zb_ha.h"

#include "test_config.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER && defined ZB_CERTIFICATION_HACKS

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
static void next_step(zb_buf_t *zcl_cmd_buf);
static zb_uint8_t g_shared_key[16] = TEST_SEC_KEY;
static zb_uint8_t g_key_nwk[] = TEST_NWK_KEY;
static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;
//static zb_ieee_addr_t g_zgpd_addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
//static zb_ieee_addr_t g_zgpd_addr_w = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07};

//static zb_uint8_t g_delay_to_next = 0;
static zb_uint16_t g_remote_addr = 0;
//static zb_uint32_t g_fc_start;
static zb_uint32_t g_comm_sink = 0;
static zb_uint32_t g_nt_state = 0;

static void next_step_delayed(zb_uint8_t param);

void zb_zgps_channel_req_send_response(zb_uint8_t param);

/*! @brief Test harness state
    Takes values of @ref test_states_e
*/
/*! Program states according to test scenario */

enum nt_states_e
{
    TEST_STATE_SEND_NT_1,
    TEST_STATE_SEND_NT_2,
    TEST_STATE_SEND_NT_3,
    TEST_STATE_SEND_NT_4,
    TEST_STATE_NT_FINISHED
};

//#define SKIP_1P
//#define SKIP_2P

enum test_states_e
{
    TEST_STATE_INITIAL,

    TEST_STATE_WAIT_FOR_ASSOCIATION,
    TEST_STATE_READ_GPS_SINK_TABLE1,

#ifndef SKIP_1P
    TEST_STATE_START_SINK_COMMISSIONING1,
    TEST_STATE_STOP_SINK_COMMISSIONING1, //not necessary, because sink is in operational mode

    TEST_STATE_READ_GPS_SINK_TABLE2,
    TEST_STATE_START_SINK_COMMISSIONING1B,

    TEST_STATE_READ_GPS_SINK_TABLE3,
    TEST_STATE_START_SINK_COMMISSIONING1C,
    TEST_STATE_STOP_SINK_COMMISSIONING1C, //not necessary, because sink is in operational mode

    TEST_STATE_READ_GPS_SINK_TABLE4,
    TEST_STATE_START_SINK_COMMISSIONING1D,

    TEST_STATE_STOP_SINK_COMMISSIONING1D,
    TEST_STATE_READ_GPS_SINK_TABLE5, //1c finished

    TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY_TYPE,
    TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY,
    TEST_STATE_WRITE_GPS_COMMISSIONING_EXIT_MODE,
    TEST_STATE_START_SINK_COMMISSIONING2A,

    TEST_STATE_READ_GPS_SINK_TABLE6,

    TEST_STATE_STOP_SINK_COMMISSIONING2C,
    TEST_STATE_START_SINK_COMMISSIONING3,
#endif
#ifndef SKIP_2P
    TEST_STATE_STOP_SINK_COMMISSIONING4,
    TEST_STATE_READ_GPS_SINK_TABLE7,
    TEST_STATE_START_SINK_COMMISSIONING5,

    TEST_STATE_STOP_SINK_COMMISSIONING5,
    TEST_STATE_READ_GPS_SINK_TABLE8,
    TEST_STATE_START_SINK_COMMISSIONING6,
    TEST_STATE_READ_GPS_SINK_TABLE9,
    TEST_STATE_READ_GPS_SINK_TABLE10,
    TEST_STATE_READ_GPS_SINK_TABLE11,
    TEST_STATE_READ_GPS_SINK_TABLE12,
    TEST_STATE_READ_GPS_SINK_TABLE13,
    TEST_STATE_READ_GPS_SINK_TABLE14,

    TEST_STATE_STOP_SINK_COMMISSIONING6F,
    TEST_STATE_READ_GPS_SINK_TABLE15,
    TEST_STATE_START_SINK_COMMISSIONING6G,
    TEST_STATE_READ_GPS_SINK_TABLE16,

    TEST_STATE_STOP_SINK_COMMISSIONING6G,
    TEST_STATE_READ_GPS_SINK_TABLE17,
    TEST_STATE_START_SINK_COMMISSIONING6H,
    TEST_STATE_READ_GPS_SINK_TABLE18,
    TEST_STATE_READ_GPS_SINK_TABLE19,
    TEST_STATE_READ_GPS_SINK_TABLE20,
    TEST_STATE_STOP_SINK_COMMISSIONING6J,
    TEST_STATE_READ_GPS_SINK_TABLE21,
    TEST_STATE_START_SINK_COMMISSIONING7A,

    TEST_STATE_BROADCAST_GP_RESPONSE7A,

    TEST_STATE_STOP_SINK_COMMISSIONING7A,
    TEST_STATE_READ_GPS_SINK_TABLE22,
    TEST_STATE_START_SINK_COMMISSIONING7B,

    TEST_STATE_BROADCAST_GP_RESPONSE7B,

    TEST_STATE_STOP_SINK_COMMISSIONING7B,
    TEST_STATE_READ_GPS_SINK_TABLE23,
    TEST_STATE_START_SINK_COMMISSIONING7C,

    TEST_STATE_BROADCAST_GP_RESPONSE7C,

    TEST_STATE_STOP_SINK_COMMISSIONING7C,
    TEST_STATE_READ_GPS_SINK_TABLE24,
    TEST_STATE_START_SINK_COMMISSIONING7D,

    TEST_STATE_BROADCAST_GP_RESPONSE7D,

    TEST_STATE_STOP_SINK_COMMISSIONING7D,
    TEST_STATE_READ_GPS_SINK_TABLE25,
    TEST_STATE_START_SINK_COMMISSIONING8A,

    TEST_STATE_STOP_SINK_COMMISSIONING8A,
    TEST_STATE_READ_GPS_SINK_TABLE26,
    TEST_STATE_START_SINK_COMMISSIONING8B,

    TEST_STATE_STOP_SINK_COMMISSIONING8B,
    TEST_STATE_READ_GPS_SINK_TABLE27,
    TEST_STATE_START_SINK_COMMISSIONING8C,

    TEST_STATE_STOP_SINK_COMMISSIONING8C,
    TEST_STATE_READ_GPS_SINK_TABLE28,

#endif

    TEST_STATE_START_SINK_COMMISSIONING9A,
    TEST_STATE_READ_GPS_SINK_TABLE29,

    TEST_STATE_START_SINK_COMMISSIONING10A,

    TEST_STATE_STOP_SINK_COMMISSIONING10A,
    TEST_STATE_READ_GPS_SINK_TABLE30,
    TEST_STATE_START_SINK_COMMISSIONING10B,
    TEST_STATE_STOP_SINK_COMMISSIONING10B,
    TEST_STATE_FINISHED
};

//delays before each state in 0.1 seconds step
static zb_uint16_t test_delays[] =
{
    0, 5, //preparing
    5,
#ifndef SKIP_1P
    55, // 1A .. enter comm mode ready
    //wait for 2 ChReq packets
    2, 5, // 1B .. enter comm mode ready
    40, 5, 55,
    2, 5, // 1C
    55, 2, //1c finish
    2, 2, 2, 5,
    65,
    2, 50, 65,
#endif
#ifndef SKIP_2P
    2, 2,
    35, 2, 2, 46, 3, 2, 2, 2, 2, 2, 35, 2, 20,
    2, 2, 2, 28, 3, 3, 3, 40,
    10, 10, 7, 2,
    5, 10, 10, 2, 2,
    22, 10, 32, 2,
    20, 10, 55, 2, 70, 2, 2, 70, 2, 2, 110, 2, 2,
#endif
    30, 2, 60, 2, 2, 70,
    5000, 5, 5, 5
}; //finish

//warning: first state TEST_STATE_INITIAL = 0!
static zb_uint8_t g_test_state = TEST_STATE_INITIAL;
static zb_short_t g_error_cnt = 0;

static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
    zb_ieee_addr_t gps_addr = DUT_GPS_IEEE_ADDR;

    if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, gps_addr) )
    {
        TRACE_MSG(TRACE_ERROR, "Unknown device has joined! skip", (FMT__0));
    }
    else
        /* Use joined device as destination for outgoing APS packets */
        if (g_remote_addr == 0)
        {
            g_remote_addr = da->nwk_addr;
            TRACE_MSG(TRACE_APP3, "Joined GPS: 0x%4x", (FMT__D, g_remote_addr));
            ZB_SCHEDULE_ALARM(next_step_delayed, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100 * test_delays[g_test_state]));
        }
}

static void gpp_send_gp_notify_cb(zb_uint8_t param)
{
    ZVUNUSED(param);
    //  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    //  zb_gpdf_info_t *gpdf_info = ZB_GET_BUF_PARAM(buf, zb_gpdf_info_t);
    TRACE_MSG(TRACE_APP3, "notify: g_ns_state:%d g_test_state:%d", (FMT__H_H, g_nt_state, g_test_state));
    g_nt_state++;
    switch (g_test_state)
    {
    /*    case TEST_STATE_WAIT_FOR_DATA1:
          g_fc_start = gpdf_info->sec_frame_counter;
          TRACE_MSG(TRACE_APP3, "TEST_STATE_WAIT_FOR_DATA g_fc_start:%d", (FMT__H, g_fc_start));
          ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_TRUE;
          ZB_CERT_HACKS().gp_proxy_replace_sec_frame_counter=g_fc_start-2;
          ZB_SCHEDULE_ALARM(next_step_delayed, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100*test_delays[g_test_state]));
        break;*/
    case TEST_STATE_FINISHED:
        break;
    default:
        break;
    }
}

static void stop_commissioning_sink(zb_uint8_t param)
{
    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(stop_commissioning_sink);
    }
    else
    {
        ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_format = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_level = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_options = ZB_TRUE;
        zgp_cluster_send_gp_sink_commissioning_mode(param,
                g_remote_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(0, 0, 0, 1),
                0xff,
                NULL);
        ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
    }

}

static void start_commissioning_sink(zb_uint8_t param)
{
    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(start_commissioning_sink);
    }
    else
    {
        zgp_cluster_send_gp_sink_commissioning_mode(param,
                g_remote_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(1, 0, 0, 1),
                0xff,
                NULL);
        ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
    }

}

MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_gps");

    /* Need to recv GPDF */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 0;
#endif

    ZB_CERT_HACKS().gp_proxy_gp_notif_req_cb = gpp_send_gp_notify_cb;

    zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    /* use channel 11: most GPDs uses it */
    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpp_addr);

    zgp_cluster_set_app_zcl_cmd_handler(zcl_specific_cluster_cmd_handler);

    ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;

    ZB_NIB().nwk_use_multicast = ZB_FALSE;

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_shared_key, ZB_CCM_KEY_SIZE);

    //  ZB_ZGP_REGISTER_COMM_COMPLETED_CB(comm_cb);

    ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(TEST_KEY_TYPE);
    zb_zdo_register_device_annce_cb(test_device_annce_cb);

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    ZGP_CTX().device_role = ZGP_DEVICE_COMBO_BASIC;

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zcl_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void tmp_zb_zgps_commissioning_req_send_response_cb(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP3, "comm_req_cb", (FMT__0));
}

/*! Test step */
static void next_step(zb_buf_t *zcl_cmd_buf)
{
    zb_uint32_t val;
    zb_uint8_t *attr_val;
    zb_bool_t schedule_next = ZB_TRUE;

    if (g_test_state < TEST_STATE_FINISHED)
    {
        ++g_test_state;
    }

    TRACE_MSG(TRACE_ZCL1, "> next_step: zcl_cmd_buf: %p, g_test_state %d",
              (FMT__P_D, zcl_cmd_buf, g_test_state));

    switch (g_test_state)
    {
    /*    case TEST_STATE_WRITE_GPS_SECURITY_LEVEL:
          {
            TRACE_MSG(TRACE_APP3, "WRITE_GPS_SECURITY_LEVEL", (FMT__0));
            val = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC|(1<<2);
        attr_val = (zb_uint8_t*)&val;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
          }
          break;
        case TEST_STATE_WRITE_GPS_SECURITY_LEVEL_11:
          {
            TRACE_MSG(TRACE_APP3, "WRITE_GPS_SECURITY_LEVEL_11", (FMT__0));
            val = ZB_ZGP_SEC_LEVEL_FULL_WITH_ENC;
        attr_val = (zb_uint8_t*)&val;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
          }
          break;*/
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY_TYPE:
    {
        val = ZB_ZGP_SEC_KEY_TYPE_GROUP;
        attr_val = (zb_uint8_t *)&val;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_TYPE_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY:
    {
        zb_uint8_t key[] = TEST_SNK_KEY;
        attr_val = (zb_uint8_t *)key;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GP_SHARED_SECURITY_KEY_ID, ZB_ZCL_ATTR_TYPE_128_BIT_KEY, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
    case TEST_STATE_WRITE_GPS_COMMISSIONING_EXIT_MODE:
    {
        val = 0x02;
        attr_val = (zb_uint8_t *)&val;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GPS_COMMISSIONING_EXIT_MODE_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
    }
    break;
    /*    case TEST_STATE_WRITE_GPS_SECURITY_LEVEL_10:
          {
            TRACE_MSG(TRACE_APP3, "WRITE_GPS_SECURITY_LEVEL_10", (FMT__0));
            val = ZB_ZGP_SEC_LEVEL_FULL_NO_ENC;
        attr_val = (zb_uint8_t*)&val;
        zgp_cluster_write_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                               ZB_ZCL_ATTR_GPS_SECURITY_LEVEL_ID, ZB_ZCL_ATTR_TYPE_8BITMAP, attr_val,
                               ZB_ZCL_DISABLE_DEFAULT_RESPONSE, NULL);
          }
          break;
        case TEST_STATE_REPLACE_APP_ID:
          {
            TRACE_MSG(TRACE_APP3, "REPLACE_APP_ID", (FMT__0));
            zb_free_buf(zcl_cmd_buf);
            ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_value = 3;
            TRACE_MSG(TRACE_APP3, "replace app_id when comm notify send", (FMT__0));
          }
          break;
        case TEST_STATE_REPLACE_GPD_ID:
          {
            TRACE_MSG(TRACE_APP3, "REPLACE_GPD_ID", (FMT__0));
            zb_free_buf(zcl_cmd_buf);
            TRACE_MSG(TRACE_APP3, "replace gpd_id when comm notify send", (FMT__0));
            ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id_value = 0;
          }
          break;
        case TEST_STATE_REPLACE_IEEE_ID:
          {
            TRACE_MSG(TRACE_APP3, "REPLACE_IEEE_ID", (FMT__0));
            zb_free_buf(zcl_cmd_buf);
            TRACE_MSG(TRACE_APP3, "replace gpd_ieee when comm notify send", (FMT__0));
            ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_format = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_value = 2;
            ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_TRUE;
            ZB_64BIT_ADDR_ZERO(ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_ieee_value);
            ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_ep_value = 1;
          }
          break;
        case TEST_STATE_REPLACE_SEC_LVL:
          {
            TRACE_MSG(TRACE_APP3, "REPLACE_SEC_LVL", (FMT__0));
            zb_free_buf(zcl_cmd_buf);
            TRACE_MSG(TRACE_APP3, "replace sec_level when comm notify send", (FMT__0));
            ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_level = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_sec_level = 2;
          }
          break;
        case TEST_STATE_REPLACE_TEST6:
          {
            TRACE_MSG(TRACE_APP3, "REPLACE_TEST6", (FMT__0));
            zb_free_buf(zcl_cmd_buf);
            ZB_CERT_HACKS().gp_proxy_replace_comm_options = ZB_TRUE;
            ZB_CERT_HACKS().gp_proxy_replace_comm_options_value = 0xf000;
            ZB_CERT_HACKS().gp_proxy_replace_comm_options_mask  = 0xf000;

          }
          break;*/
    case TEST_STATE_BROADCAST_GP_RESPONSE7A:
    {
        TRACE_MSG(TRACE_APP3, "BROADCAST_GP_RESPONSE7A", (FMT__0));
        ZGP_CTX().sink_mode = ZB_ZGP_COMMISSIONING_MODE;
        ZGP_CTX().comm_data.selected_temp_master_idx = 0;
        ZGP_CTX().comm_data.temp_master_list[0].short_addr = ~g_remote_addr;
        ZGP_CTX().comm_data.zgpd_id.app_id = 0;
        ZGP_CTX().comm_data.zgpd_id.addr.src_id = 0;
        //        ZGP_CTX().comm_data.zgpd_id.addr = 0;
        //        ZGP_CTX().comm_data.zgpd_id.endpoint = 0;
        ZGP_CTX().comm_data.temp_master_tx_chnl = TEST_CHANNEL;
        zb_zgps_channel_req_send_response(ZB_REF_FROM_BUF(zcl_cmd_buf));
    }
    break;
    case TEST_STATE_BROADCAST_GP_RESPONSE7B:
    {
        TRACE_MSG(TRACE_APP3, "BROADCAST_GP_RESPONSE7B", (FMT__0));
        ZGP_CTX().sink_mode = ZB_ZGP_COMMISSIONING_MODE;
        ZGP_CTX().comm_data.selected_temp_master_idx = 0;
        ZGP_CTX().comm_data.temp_master_list[0].short_addr = ~g_remote_addr;
        ZGP_CTX().comm_data.zgpd_id.app_id = 0;
        ZGP_CTX().comm_data.zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID_FAKE;
        //        ZGP_CTX().comm_data.zgpd_id.addr = 0;
        //        ZGP_CTX().comm_data.zgpd_id.endpoint = 0;
        ZGP_CTX().comm_data.temp_master_tx_chnl = TEST_CHANNEL;
        zb_zgps_channel_req_send_response(ZB_REF_FROM_BUF(zcl_cmd_buf));
    }
    break;
    case TEST_STATE_BROADCAST_GP_RESPONSE7C:
    {
        TRACE_MSG(TRACE_APP3, "BROADCAST_GP_RESPONSE7C", (FMT__0));
        {
            zb_zgp_gp_response_t *resp_ptr;
            zb_zgp_gp_response_t  resp;
            zb_ieee_addr_t g_zgpd_addr = TH_GPD_IEEE_ADDR;
            resp.options = 0x02;
            resp.temp_master_addr = ~g_remote_addr;
            resp.temp_master_tx_chnl = TEST_CHANNEL;
            ZGP_CTX().comm_data.zgpd_id.app_id = 0;
            ZGP_CTX().comm_data.zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
            ZB_IEEE_ADDR_COPY(&resp.zgpd_addr, &g_zgpd_addr);
            //      resp.zgpd_addr = ZGP_CTX().comm_data.zgpd_id.addr;
            resp.endpoint = 1;
            resp.gpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING_REPLY;

            //      resp.payload[0]=fill_comm_resp_security_header(&resp.payload[1], &ent);
            resp.payload[0] = 1;
            TRACE_MSG(TRACE_ZGP2, "Payload_len:%d", (FMT__H, resp.payload[0]));
            resp.payload[1] = 0x30;


            ZB_ZCL_START_PACKET(zcl_cmd_buf);
            resp_ptr = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zgp_gp_response_t);
            ZB_MEMCPY(resp_ptr, &resp, sizeof(zb_zgp_gp_response_t));

            zb_zgp_cluster_gp_response_send(ZB_REF_FROM_BUF(zcl_cmd_buf),
                                            ZB_NWK_BROADCAST_ALL_DEVICES,
                                            ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                            tmp_zb_zgps_commissioning_req_send_response_cb);

        }
    }
    break;
    case TEST_STATE_BROADCAST_GP_RESPONSE7D:
        TRACE_MSG(TRACE_APP3, "BROADCAST_GP_RESPONSE7D", (FMT__0));
        {
            zb_zgp_gp_response_t *resp_ptr;
            zb_zgp_gp_response_t  resp;
            zb_ieee_addr_t g_zgpd_addr = TH_GPD_BAD_IEEE_ADDR;
            resp.options = 0x02;
            resp.temp_master_addr = ~g_remote_addr;
            resp.temp_master_tx_chnl = TEST_CHANNEL;
            ZGP_CTX().comm_data.zgpd_id.app_id = 0;
            ZGP_CTX().comm_data.zgpd_id.addr.src_id = TEST_ZGPD_SRC_ID;
            ZB_IEEE_ADDR_COPY(&resp.zgpd_addr, &g_zgpd_addr);
            //      resp.zgpd_addr = ZGP_CTX().comm_data.zgpd_id.addr;
            resp.endpoint = 1;
            resp.gpd_cmd_id = ZB_GPDF_CMD_COMMISSIONING_REPLY;

            //      resp.payload[0]=fill_comm_resp_security_header(&resp.payload[1], &ent);
            resp.payload[0] = 1;
            TRACE_MSG(TRACE_ZGP2, "Payload_len:%d", (FMT__H, resp.payload[0]));
            resp.payload[1] = 0x30;


            ZB_ZCL_START_PACKET(zcl_cmd_buf);
            resp_ptr = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zgp_gp_response_t);
            ZB_MEMCPY(resp_ptr, &resp, sizeof(zb_zgp_gp_response_t));

            zb_zgp_cluster_gp_response_send(ZB_REF_FROM_BUF(zcl_cmd_buf),
                                            ZB_NWK_BROADCAST_ALL_DEVICES,
                                            ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                            tmp_zb_zgps_commissioning_req_send_response_cb);
        }
        break;
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    case TEST_STATE_READ_GPS_SINK_TABLE5:
    case TEST_STATE_READ_GPS_SINK_TABLE6:
    case TEST_STATE_READ_GPS_SINK_TABLE7:
    case TEST_STATE_READ_GPS_SINK_TABLE8:
    case TEST_STATE_READ_GPS_SINK_TABLE9:
    case TEST_STATE_READ_GPS_SINK_TABLE10:
    case TEST_STATE_READ_GPS_SINK_TABLE11:
    case TEST_STATE_READ_GPS_SINK_TABLE12:
    case TEST_STATE_READ_GPS_SINK_TABLE13:
    case TEST_STATE_READ_GPS_SINK_TABLE14:
    case TEST_STATE_READ_GPS_SINK_TABLE15:
    case TEST_STATE_READ_GPS_SINK_TABLE16:
    case TEST_STATE_READ_GPS_SINK_TABLE17:
    case TEST_STATE_READ_GPS_SINK_TABLE18:
    case TEST_STATE_READ_GPS_SINK_TABLE19:
    case TEST_STATE_READ_GPS_SINK_TABLE20:
    case TEST_STATE_READ_GPS_SINK_TABLE21:
    case TEST_STATE_READ_GPS_SINK_TABLE22:
    case TEST_STATE_READ_GPS_SINK_TABLE23:
    case TEST_STATE_READ_GPS_SINK_TABLE24:
    case TEST_STATE_READ_GPS_SINK_TABLE25:
    case TEST_STATE_READ_GPS_SINK_TABLE26:
    case TEST_STATE_READ_GPS_SINK_TABLE27:
    case TEST_STATE_READ_GPS_SINK_TABLE28:
    case TEST_STATE_READ_GPS_SINK_TABLE29:
    case TEST_STATE_READ_GPS_SINK_TABLE30:
    {
        TRACE_MSG(TRACE_APP3, "READ_GPS_SINK_TABLE", (FMT__0));
        ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_FALSE;
        zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), g_remote_addr, DUT_GPS_ADDR_MODE,
                              ZB_ZCL_ATTR_GPS_SINK_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
        //        schedule_next = ZB_FALSE;
    }
    break;
    //    case TEST_STATE_WAIT_FOR_PAIRING1:
    /*    case TEST_STATE_WAIT_FOR_PAIRING2:
        case TEST_STATE_WAIT_FOR_PAIRING3:
        case TEST_STATE_WAIT_FOR_PAIRING4:    //handled in zcl_specific_cluster_cmd_handler
        case TEST_STATE_WAIT_FOR_DATA1:       //handled in gpp_send_gp_notify_cb*/
    case TEST_STATE_WAIT_FOR_ASSOCIATION: //handled in test_device_annce_cb
    {
        TRACE_MSG(TRACE_APP3, "switch to wait_for_pairing or data", (FMT__0));
        zb_free_buf(zcl_cmd_buf);
        schedule_next = ZB_FALSE;
    }
    break;
    case TEST_STATE_STOP_SINK_COMMISSIONING1:
    //    case TEST_STATE_STOP_SINK_COMMISSIONING1B:
    case TEST_STATE_STOP_SINK_COMMISSIONING1C:
    case TEST_STATE_STOP_SINK_COMMISSIONING1D:
    /*    case TEST_STATE_STOP_SINK_COMMISSIONING2A:
        case TEST_STATE_STOP_SINK_COMMISSIONING2B:*/
    case TEST_STATE_STOP_SINK_COMMISSIONING2C:
    /*    case TEST_STATE_STOP_SINK_COMMISSIONING2D:
        case TEST_STATE_STOP_SINK_COMMISSIONING3:*/
    case TEST_STATE_STOP_SINK_COMMISSIONING4:
    //    case TEST_STATE_STOP_SINK_COMMISSIONING4B:
    case TEST_STATE_STOP_SINK_COMMISSIONING5:
    case TEST_STATE_STOP_SINK_COMMISSIONING6F:
    case TEST_STATE_STOP_SINK_COMMISSIONING6G:
    case TEST_STATE_STOP_SINK_COMMISSIONING6J:
    //    case TEST_STATE_STOP_SINK_COMMISSIONING6:
    case TEST_STATE_STOP_SINK_COMMISSIONING7A:
    case TEST_STATE_STOP_SINK_COMMISSIONING7B:
    case TEST_STATE_STOP_SINK_COMMISSIONING7C:
    case TEST_STATE_STOP_SINK_COMMISSIONING7D:
    case TEST_STATE_STOP_SINK_COMMISSIONING8A:
    case TEST_STATE_STOP_SINK_COMMISSIONING8B:
    case TEST_STATE_STOP_SINK_COMMISSIONING8C:
    case TEST_STATE_STOP_SINK_COMMISSIONING10A:
    case TEST_STATE_STOP_SINK_COMMISSIONING10B:
    {
        TRACE_MSG(TRACE_APP3, "STOP_SINK_COMMISSIONING", (FMT__0));
        ZGP_CTX().sink_mode = ZB_ZGP_OPERATIONAL_MODE;
        //zb_free_buf(zcl_cmd_buf);
        ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
        ZB_CERT_HACKS().gp_proxy_replace_comm_app_id = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_gpd_id = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_options = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_comm_app_id_format = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_level = ZB_FALSE;
        ZB_CERT_HACKS().gp_proxy_replace_gp_notif_sec_frame_counter = ZB_FALSE;

        zgp_cluster_send_gp_sink_commissioning_mode(ZB_REF_FROM_BUF(zcl_cmd_buf),
                g_remote_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(0, 0, 0, 1),
                0xff,
                NULL);
        ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;

        //        ZB_SCHEDULE_ALARM(stop_commissioning_sink, ZB_REF_FROM_BUF(zcl_cmd_buf), 0);
        //        stop_commissioning_sink(ZB_REF_FROM_BUF(zcl_cmd_buf));
        //  zb_free_buf(zcl_cmd_buf);
        TRACE_MSG(TRACE_APP3, "stop_commissioning_sink :%d", (FMT__H, g_comm_sink));
    }
    break;
    case TEST_STATE_START_SINK_COMMISSIONING1:
    case TEST_STATE_START_SINK_COMMISSIONING1B:
    case TEST_STATE_START_SINK_COMMISSIONING1C:
    case TEST_STATE_START_SINK_COMMISSIONING1D:
    /*    case TEST_STATE_START_SINK_COMMISSIONING2B:
        case TEST_STATE_START_SINK_COMMISSIONING2C:
        case TEST_STATE_START_SINK_COMMISSIONING2D:*/
    case TEST_STATE_START_SINK_COMMISSIONING3:
    /*    case TEST_STATE_START_SINK_COMMISSIONING4A:
        case TEST_STATE_START_SINK_COMMISSIONING4B:*/
    case TEST_STATE_START_SINK_COMMISSIONING5:
    case TEST_STATE_START_SINK_COMMISSIONING6:
    case TEST_STATE_START_SINK_COMMISSIONING6G:
    case TEST_STATE_START_SINK_COMMISSIONING6H:
    case TEST_STATE_START_SINK_COMMISSIONING7A:
    case TEST_STATE_START_SINK_COMMISSIONING7B:
    case TEST_STATE_START_SINK_COMMISSIONING7C:
    case TEST_STATE_START_SINK_COMMISSIONING7D:
    case TEST_STATE_START_SINK_COMMISSIONING8A:
    case TEST_STATE_START_SINK_COMMISSIONING8B:
    case TEST_STATE_START_SINK_COMMISSIONING8C:
    case TEST_STATE_START_SINK_COMMISSIONING9A:
    case TEST_STATE_START_SINK_COMMISSIONING10A:
    case TEST_STATE_START_SINK_COMMISSIONING10B:
    {
        TRACE_MSG(TRACE_APP3, "START_SINK_COMMISSIONING", (FMT__0));
        ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
        zgp_cluster_send_gp_sink_commissioning_mode(ZB_REF_FROM_BUF(zcl_cmd_buf),
                g_remote_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(1, 0, 0, 1),
                0xff,
                NULL);
        ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
        //zb_free_buf(zcl_cmd_buf);
        //        ZB_SCHEDULE_ALARM(start_commissioning_sink, ZB_REF_FROM_BUF(zcl_cmd_buf), 0);
        //        start_commissioning_sink(ZB_REF_FROM_BUF(zcl_cmd_buf));
        g_comm_sink++;
        TRACE_MSG(TRACE_APP3, "start_commissioning_sink :%d", (FMT__H, g_comm_sink));
    }
    break;
    case TEST_STATE_START_SINK_COMMISSIONING2A:
    {
        TRACE_MSG(TRACE_APP3, "START_SINK_COMMISSIONING", (FMT__0));
        ZGP_CTX().device_role = ZGP_DEVICE_COMMISSIONING_TOOL;
        zgp_cluster_send_gp_sink_commissioning_mode(ZB_REF_FROM_BUF(zcl_cmd_buf),
                g_remote_addr,
                ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                ZB_ZGP_GP_SINK_COMM_MODE_FILL_OPT(1, 0, 0, 1),
                0xff,
                NULL);
        ZGP_CTX().device_role = ZGP_DEVICE_PROXY_BASIC;
        //zb_free_buf(zcl_cmd_buf);
        //        ZB_SCHEDULE_ALARM(start_commissioning_sink, ZB_REF_FROM_BUF(zcl_cmd_buf), 0);
        //        start_commissioning_sink(ZB_REF_FROM_BUF(zcl_cmd_buf));
        g_comm_sink++;
        TRACE_MSG(TRACE_APP3, "start_commissioning_sink :%d", (FMT__H, g_comm_sink));
    }
    break;
    case TEST_STATE_FINISHED:
    {
        zb_free_buf(zcl_cmd_buf);

        if (!g_error_cnt)
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_APP1, "Test finished. Status: FAILED, error number %hd", (FMT__H, g_error_cnt));
        }

        schedule_next = ZB_FALSE;
    }
    break;
    default:
        TRACE_MSG(TRACE_APP3, "case :%d not handled, STOP", (FMT__H, g_test_state));
        ZB_ASSERT(0);
        break;
    }

    TRACE_MSG(TRACE_APP3, "< next_step: g_test_state: %hd", (FMT__H, g_test_state));

    if (schedule_next)
    {
        ZB_SCHEDULE_ALARM(next_step_delayed, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100 * test_delays[g_test_state]));
    }

}

static void read_attr_handler(zb_buf_t *zcl_cmd_buf)
{
    zb_bool_t test_error = ZB_FALSE;

    switch (g_test_state)
    {
    case TEST_STATE_READ_GPS_SINK_TABLE1:
    case TEST_STATE_READ_GPS_SINK_TABLE2:
    case TEST_STATE_READ_GPS_SINK_TABLE3:
    case TEST_STATE_READ_GPS_SINK_TABLE4:
    case TEST_STATE_READ_GPS_SINK_TABLE5:
    case TEST_STATE_READ_GPS_SINK_TABLE6:
    case TEST_STATE_READ_GPS_SINK_TABLE7:
    case TEST_STATE_READ_GPS_SINK_TABLE8:
    case TEST_STATE_READ_GPS_SINK_TABLE9:
    case TEST_STATE_READ_GPS_SINK_TABLE10:
    case TEST_STATE_READ_GPS_SINK_TABLE11:
    case TEST_STATE_READ_GPS_SINK_TABLE12:
    {
        zb_zcl_read_attr_res_t *read_attr_resp;

        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(zcl_cmd_buf, read_attr_resp);
        TRACE_MSG(TRACE_APP2, "ZB_ZCL_CMD_READ_ATTRIB_RESP getted: attr_id 0x%hx, status: 0x%hx, value 0x%hd",
                  (FMT__D_H_D, read_attr_resp->attr_id, read_attr_resp->status, *read_attr_resp->attr_value));
        if (read_attr_resp->status != ZB_ZCL_STATUS_SUCCESS)
        {
            test_error = ZB_TRUE;
        }
    }
    break;
    }

    if (test_error)
    {
        ++g_error_cnt;
        TRACE_MSG(TRACE_APP1, "Error at state: %hd", (FMT__H, g_test_state));
    }
}

static void write_attr_handler(zb_buf_t *zcl_cmd_buf)
{
    zb_bool_t test_error = ZB_FALSE;
    ZVUNUSED(zcl_cmd_buf);

    /*
      switch (g_test_state)
      {
        case TEST_STATE_WRITE_GPS_SECURITY_LEVEL:
    //    case TEST_STATE_WRITE_GP_SHARED_SECURITY_KEY:
        {
          zb_zcl_write_attr_res_t *res;

          ZB_ZCL_GET_NEXT_WRITE_ATTR_RES(zcl_cmd_buf, res);

          if (res->status != ZB_ZCL_STATUS_SUCCESS)
          {
            test_error = ZB_TRUE;
          }
        }
        break;
      }
    */
    if (test_error)
    {
        ++g_error_cnt;
    }
    //  zb_free_buf(zcl_cmd_buf);
    //  if(g_test_state<TEST_STATE_FINISHED) ++g_test_state;
}


static void handle_gp_pairing(zb_buf_t *zcl_cmd_buf)
{
    ZVUNUSED(zcl_cmd_buf);

    TRACE_MSG(TRACE_APP1, ">> handle_gp_pairing %hd", (FMT__H, g_test_state));
}

static void next_step_delayed(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APP1, ">> next_step_delayed %hd", (FMT__H, param));

    if (param == 0)
    {
        ZB_GET_OUT_BUF_DELAYED(next_step_delayed);
    }
    else
    {
        next_step(ZB_BUF_FROM_REF(param));
    }
    TRACE_MSG(TRACE_APP1, "<< next_step_delayed", (FMT__0));
}

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    /** [VARIABLE] */
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;
    zb_zcl_default_resp_payload_t *default_res;
    /** [VARIABLE] */


    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %hd", (FMT__H, param));

    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %hd", (FMT__H, ZB_BUF_LEN(zcl_cmd_buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
    {
        TRACE_MSG(TRACE_ZCL1, "Skip command, Unsupported direction \"to server\"", (FMT__0));
    }
    else
    {
        /* Command from server to client */
        /** [HANDLER] */
        switch (cmd_info->cluster_id)
        {
        case ZB_ZCL_CLUSTER_ID_BASIC:
        case ZB_ZCL_CLUSTER_ID_GREEN_POWER:
        {
            if (cmd_info->is_common_command)
            {
                switch (cmd_info->cmd_id)
                {
                case ZB_ZCL_CMD_DEFAULT_RESP:
                    TRACE_MSG(TRACE_ZCL1, "Process general command %hd", (FMT__H, cmd_info->cmd_id));

                    default_res = ZB_ZCL_READ_DEFAULT_RESP(zcl_cmd_buf);
                    TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_DEFAULT_RESP getted: cmd_id 0x%hx, status: 0x%hx",
                              (FMT__H_H, default_res->command_id, default_res->status));
                    //        if(g_test_state<TEST_STATE_FINISHED) ++g_test_state;
                    next_step(zcl_cmd_buf);
                    break;

                case ZB_ZCL_CMD_READ_ATTRIB_RESP:
                    read_attr_handler(zcl_cmd_buf);
                    zb_free_buf(zcl_cmd_buf);
                    //              ZB_SCHEDULE_ALARM(next_step_delayed, ZB_REF_FROM_BUF(zcl_cmd_buf), ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000*g_delay_to_next));
                    break;
                case ZB_ZCL_CMD_WRITE_ATTRIB_RESP:
                    write_attr_handler(zcl_cmd_buf);
                    zb_free_buf(zcl_cmd_buf);
                    //              ZB_SCHEDULE_ALARM(next_step_delayed, ZB_REF_FROM_BUF(zcl_cmd_buf), ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000*g_delay_to_next));
                    //              next_step(zcl_cmd_buf);
                    break;
                default:
                    TRACE_MSG(TRACE_ERROR, "ERROR, Unsupported general command", (FMT__0));
                    break;
                }
                cmd_processed = ZB_TRUE;
            }
            else
            {
                switch (cmd_info->cmd_id)
                {
                case ZGP_CLIENT_CMD_GP_SINK_TABLE_RESPONSE:
                    cmd_processed = ZB_TRUE;
                    break;
                case ZGP_CLIENT_CMD_GP_PAIRING:
                    TRACE_MSG(TRACE_ERROR, "PAIRING received! (state :%d)", (FMT__H, g_test_state));
                    handle_gp_pairing(zcl_cmd_buf);
                    cmd_processed = ZB_FALSE;
                    //              zb_free_buf(zcl_cmd_buf);
                    //        ZB_SCHEDULE_ALARM(next_step_delayed, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100*test_delays[g_test_state]));
                    break;
                default:
                    TRACE_MSG(TRACE_ERROR, "Unknow cmd received", (FMT__0));
                }
            }
            break;
        }

        default:
            TRACE_MSG(TRACE_ERROR, "Cluster 0x%x is not supported in the test",
                      (FMT__D, cmd_info->cluster_id));
            break;
        }
        /** [HANDLER] */
    }

    TRACE_MSG(TRACE_ZCL1,
              "< zcl_specific_cluster_cmd_handler cmd_processed %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
#ifndef ZB_NSNG
        zb_osif_led_on(2);
#endif
        next_step(buf);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Error, Device start FAILED status %d",
                  (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER && defined ZB_CERTIFICATION_HACKS

#include <stdio.h>

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    ZVUNUSED(param);
}

MAIN()
{
    ARGV_UNUSED;

    printf(" HA or ZGP cluster or ZB_CERTIFICATION_HACKS is not supported\n");

    MAIN_RETURN(1);
}

#endif // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER && defined ZB_CERTIFICATION_HACKS
