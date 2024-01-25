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
/* PURPOSE: TH ZR1 (initiator)
*/

#define ZB_TEST_NAME TP_BDB_FB_PRE_TC_04B_THR1
#define ZB_TRACE_FILE_ID 40835

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
#include "test_device_initiator.h"
#include "tp_bdb_fb_pre_tc_04b_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#if !defined(ZB_USE_NVRAM)
#error Define ZB_USE_NVRAM
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static zb_uint8_t g_nwk_key[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                                  };

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(fb_pre_tc_04b_thr1_basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(fb_pre_tc_04b_thr1_identify_attr_list, &attr_identify_time);

/* Temperature Measurement attributes data */
static zb_int16_t attr_temp_value     = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
static zb_int16_t attr_min_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_MIN_VALUE;
static zb_int16_t attr_max_temp_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_MAX_VALUE;
static zb_uint16_t attr_tolerance     = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_MAX_VALUE;
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(fb_pre_tc_04b_thr1_temp_meas_attr_list,
        &attr_temp_value,
        &attr_min_temp_value,
        &attr_max_temp_value,
        &attr_tolerance);

/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(fb_pre_tc_04b_thr1_initiator_clusters,
                               fb_pre_tc_04b_thr1_basic_attr_list,
                               fb_pre_tc_04b_thr1_identify_attr_list,
                               fb_pre_tc_04b_thr1_temp_meas_attr_list);

DECLARE_INITIATOR_EP(fb_pre_tc_04b_thr1_initiator_ep,
                     THR1_ENDPOINT,
                     fb_pre_tc_04b_thr1_initiator_clusters);

DECLARE_INITIATOR_CTX(fb_pre_tc_04b_thr1_initiator_ctx, fb_pre_tc_04b_thr1_initiator_ep);


/************************Test Functions*******************************/
enum test_steps_e
{
    ACTIVE_EP_REQ_FOR_UNKNOWN_DEVICE,
    ACTIVE_EP_REQ_MALFORMED,
    SIMPLE_DESC_REQ_FOR_UNKNOWN_DEVICE,
    SIMPLE_DESC_REQ_FOR_INACTIVE_EP,
    SIMPLE_DESC_REQ_INCORRECT_NWK_ADDR_OF_INTEREST,
    SIMPLE_DESC_REQ_INCORRECT_INTEREST_EP,
    SIMPLE_DESC_REQ_MALFORMED_WITH_NWK_ADDR,
    SIMPLE_DESC_REQ_MALFORMED_WITH_EP,
    MATCH_DESC_REQ_WITH_UNSUP_PROFILE_ID_UNICAST,
    MATCH_DESC_REQ_WITH_UNSUP_PROFILE_ID_BROADCAST,
    MATCH_DESC_REQ_NO_MATCHING_CLUSTER_UNICAST,
    MATCH_DESC_REQ_NO_MATCHING_CLUSTER_BROADCAST,
    MATCH_DESC_REQ_FOR_UNKNOWN_DEVICE,
    MATCH_DESC_REQ_NO_CLUSTER_LIST_UNICAST,
    MATCH_DESC_REQ_NO_CLUSTER_LIST_BROADCAST,
    MATCH_DESC_REQ_MALFORMED_INPUT_CLUSTER_LIST,
    MATCH_DESC_REQ_MALFORMED_OUTPUT_CLUSTER_LIST
};


extern zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_callback_t cb,
                                        zb_uint16_t addr, zb_uint8_t resp_count);


static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);


static void test_step_actions(zb_uint8_t unused);
static void send_active_ep_req(zb_uint8_t param);
static void send_simple_desc_req(zb_uint8_t param);
static void send_match_desc_req(zb_uint8_t param);


static int s_step_idx = 0;

static zb_uint16_t s_dest_nwk_addr;
static zb_uint16_t s_payload_nwk_addr;
static zb_uint16_t s_payload_ep;
static zb_uint16_t s_payload_profile_id;
static zb_uint16_t *s_cluster_list_ptr;

static zb_uint8_t  s_target_dut_ep;
static zb_uint16_t s_matching_clusters[2];
static int         s_matching_clusters_num;


static const zb_uint16_t s_matching_list[4] =
{
    ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
    ZB_ZCL_CLUSTER_ID_BASIC,
    ZB_ZCL_CLUSTER_ID_IDENTIFY,
    ZB_ZCL_CLUSTER_ID_ON_OFF
};

static const zb_uint16_t s_non_matching_list[4] =
{
    ZB_ZCL_CLUSTER_ID_THERMOSTAT_UI_CONFIG,
    ZB_ZCL_CLUSTER_ID_SHADE_CONFIG,
    ZB_ZCL_CLUSTER_ID_METERING,
    ZB_ZCL_CLUSTER_ID_TUNNEL
};

static const zb_uint16_t s_input_cluster_supported = ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT;
static const zb_uint16_t s_output_cluster_supported = ZB_ZCL_CLUSTER_ID_ON_OFF;



#define GO_TO_NEXT_STEP() do                                     \
  {                                                              \
    ++s_step_idx;                                                \
    ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_SHORT_DELAY);   \
  } while (0)


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");
#if UART_CONTROL
    test_control_init();
    zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

    zb_set_long_address(g_ieee_addr_thr1);
    /*
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;

    zb_cert_test_set_aps_use_nvram();

    zb_set_network_router_role_with_mode(TEST_BDB_PRIMARY_CHANNEL_SET, ZB_COMMISSIONING_BDB);
    */
    zb_set_network_router_role((1l << TEST_CHANNEL));
    zb_set_nvram_erase_at_start(ZB_TRUE);

    /* Assignment required to force Distributed formation */
    zb_aib_set_trust_center_address(g_unknown_ieee_addr);
    zb_secur_setup_nwk_key(g_nwk_key, 0);

    ZB_AF_REGISTER_DEVICE_CTX(&fb_pre_tc_04b_thr1_initiator_ctx);

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


/***********************************Implementation**********************************/

static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
    TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
              (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
    s_target_dut_ep = ep;
    if (s_matching_clusters_num < 2)
    {
        s_matching_clusters[s_matching_clusters_num++] = cluster;
    }
    return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    ZB_BDB().bdb_commissioning_time = FB_INITIATOR_DURATION;
    zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}



static void test_step_actions(zb_uint8_t unused)
{
    zb_callback_t next_cb = NULL;
    int stop_test = 0;

    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZDO1, ">>test_step_actions: test step = %d", (FMT__D, s_step_idx));

    s_dest_nwk_addr = zb_address_short_by_ieee(g_ieee_addr_dut);

    switch (s_step_idx)
    {
    case ACTIVE_EP_REQ_FOR_UNKNOWN_DEVICE:
    {
        s_payload_nwk_addr = zb_random();
        next_cb = send_active_ep_req;
    }
    break;
    case ACTIVE_EP_REQ_MALFORMED:
    {
        next_cb = send_active_ep_req;
    }
    break;
    case SIMPLE_DESC_REQ_FOR_UNKNOWN_DEVICE:
    {
        s_payload_nwk_addr = zb_random();
        s_payload_ep = s_target_dut_ep;
        next_cb = send_simple_desc_req;
    }
    break;
    case SIMPLE_DESC_REQ_FOR_INACTIVE_EP:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_ep = ~s_target_dut_ep;
        next_cb = send_simple_desc_req;
    }
    break;
    case SIMPLE_DESC_REQ_INCORRECT_NWK_ADDR_OF_INTEREST:
    {
        s_payload_nwk_addr = zb_random();
        s_payload_ep = 0;
        next_cb = send_simple_desc_req;
    }
    break;
    case SIMPLE_DESC_REQ_INCORRECT_INTEREST_EP:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_ep = 0xff;
        next_cb = send_simple_desc_req;
    }
    break;
    case SIMPLE_DESC_REQ_MALFORMED_WITH_NWK_ADDR:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        next_cb = send_simple_desc_req;
    }
    break;
    case SIMPLE_DESC_REQ_MALFORMED_WITH_EP:
    {
        s_payload_ep = 0xff;
        next_cb = send_simple_desc_req;
    }
    break;
    case MATCH_DESC_REQ_WITH_UNSUP_PROFILE_ID_UNICAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = 0x1234;
        s_cluster_list_ptr = (zb_uint16_t *) s_matching_list;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_WITH_UNSUP_PROFILE_ID_BROADCAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = 0x1234;
        s_dest_nwk_addr = 0xfffd;
        s_cluster_list_ptr = (zb_uint16_t *) s_matching_list;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_NO_MATCHING_CLUSTER_UNICAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        s_cluster_list_ptr = (zb_uint16_t *) s_non_matching_list;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_NO_MATCHING_CLUSTER_BROADCAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        s_dest_nwk_addr = 0xfffd;
        s_cluster_list_ptr = (zb_uint16_t *) s_non_matching_list;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_FOR_UNKNOWN_DEVICE:
    {
        s_payload_nwk_addr = zb_random();
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        s_cluster_list_ptr = (zb_uint16_t *) s_matching_list;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_NO_CLUSTER_LIST_UNICAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_NO_CLUSTER_LIST_BROADCAST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_dest_nwk_addr = 0xfffd;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_MALFORMED_INPUT_CLUSTER_LIST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        s_cluster_list_ptr = (zb_uint16_t *) &s_input_cluster_supported;
        next_cb = send_match_desc_req;
    }
    break;
    case MATCH_DESC_REQ_MALFORMED_OUTPUT_CLUSTER_LIST:
    {
        s_payload_nwk_addr = s_dest_nwk_addr;
        s_payload_profile_id = ZB_AF_HA_PROFILE_ID;
        s_cluster_list_ptr = (zb_uint16_t *) &s_output_cluster_supported;
        next_cb = send_match_desc_req;
    }
    break;
    default:
        TRACE_MSG(TRACE_ZDO1, "Unknown state transition", (FMT__0));
        stop_test = 1;
    }

    if (!stop_test)
    {
        zb_buf_get_out_delayed(next_cb);
    }

    TRACE_MSG(TRACE_ZDO1, "<<test_step_actions", (FMT__0));
}


static void send_active_ep_req(zb_uint8_t param)
{
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_ZDO2, ">>send_active_ep_req: buf_param = %d", (FMT__D, param));

    if (s_step_idx == ACTIVE_EP_REQ_FOR_UNKNOWN_DEVICE)
    {
        ptr = zb_buf_initial_alloc(param, sizeof(zb_uint16_t));
        ZB_HTOLE16(ptr, &s_payload_nwk_addr);
    }
    else
    {
        ptr = zb_buf_initial_alloc(param, 0);
    }

    zdo_send_req_by_short(ZDO_ACTIVE_EP_REQ_CLID,
                          param,
                          NULL,
                          s_dest_nwk_addr,
                          ZB_ZDO_CB_DEFAULT_COUNTER);

    GO_TO_NEXT_STEP();

    TRACE_MSG(TRACE_ZDO2, "<<send_active_ep_req", (FMT__0));
}


static void send_simple_desc_req(zb_uint8_t param)
{
    zb_uint8_t *ptr;

    TRACE_MSG(TRACE_ZDO2, ">>send_simple_desc_req: buf_param = %d", (FMT__D, param));

    switch (s_step_idx)
    {
    case SIMPLE_DESC_REQ_MALFORMED_WITH_NWK_ADDR:
    {
        ptr = zb_buf_initial_alloc(param, 2);
        ZB_HTOLE16(ptr, &s_payload_nwk_addr);
    }
    break;
    case SIMPLE_DESC_REQ_MALFORMED_WITH_EP:
    {
        ptr = zb_buf_initial_alloc(param, 1);
        *ptr = s_payload_ep;
    }
    break;
    default:
    {
        ptr = zb_buf_initial_alloc(param, 3);
        ZB_HTOLE16(ptr, &s_payload_nwk_addr);
        ptr += 2;
        *ptr = s_payload_ep;
    }
    }

    zdo_send_req_by_short(ZDO_SIMPLE_DESC_REQ_CLID,
                          param,
                          NULL,
                          s_dest_nwk_addr,
                          ZB_ZDO_CB_DEFAULT_COUNTER);

    GO_TO_NEXT_STEP();

    TRACE_MSG(TRACE_ZDO2, "<<send_simple_desc_req", (FMT__0));
}


static void send_match_desc_req(zb_uint8_t param)
{
    zb_uint8_t *ptr;
    zb_uint8_t req_size_common = 5;
    zb_uint8_t in_clusters_num, out_clusters_num;
    zb_uint8_t in_count, out_count;
    zb_uint8_t i;

    TRACE_MSG(TRACE_ZDO2, ">>send_match_desc_req: buf_param = %d", (FMT__D, param));

    switch (s_step_idx)
    {
    case MATCH_DESC_REQ_NO_CLUSTER_LIST_UNICAST:
    case MATCH_DESC_REQ_NO_CLUSTER_LIST_BROADCAST:
    {
        in_count = in_clusters_num = 0;
        out_count = out_clusters_num = 0;
    }
    break;
    case MATCH_DESC_REQ_MALFORMED_INPUT_CLUSTER_LIST:
    {
        out_count = out_clusters_num = 0;
        in_count = 1;
        in_clusters_num = 5;
    }
    break;
    case MATCH_DESC_REQ_MALFORMED_OUTPUT_CLUSTER_LIST:
    {
        in_count = in_clusters_num = 0;
        out_count = 1;
        out_clusters_num = 5;
    }
    break;
    default:
    {
        in_count = in_clusters_num = 3;
        out_count = out_clusters_num = 1;
    }
    }

    ptr = zb_buf_initial_alloc(param, req_size_common);

    ptr = zb_put_next_htole16(ptr, s_payload_nwk_addr);
    ptr = zb_put_next_htole16(ptr, s_payload_profile_id);

    *ptr = in_clusters_num;
    ptr = zb_buf_alloc_right(param, in_count * sizeof(zb_uint16_t) + 1);
    for (i = 0; i < in_count; ++i)
    {
        ptr = zb_put_next_htole16(ptr, s_cluster_list_ptr[i]);
    }

    *ptr = out_clusters_num;
    ptr = zb_buf_alloc_right(param, out_count * sizeof(zb_uint16_t));
    for (; i < in_count + out_count; ++i)
    {
        ptr = zb_put_next_htole16(ptr, s_cluster_list_ptr[i]);
    }

    zdo_send_req_by_short(ZDO_MATCH_DESC_REQ_CLID,
                          param,
                          NULL,
                          s_dest_nwk_addr,
                          ZB_ZDO_CB_DEFAULT_COUNTER);

    GO_TO_NEXT_STEP();

    TRACE_MSG(TRACE_ZDO2, "<<send_match_desc_req", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        {
            zb_ext_pan_id_t extended_pan_id;

            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

            zb_get_extended_pan_id(extended_pan_id);

            if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, extended_pan_id))
            {
                bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            }
            else
            {
                ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
            }
            break;
        }

        case ZB_BDB_SIGNAL_STEERING:
            TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
            ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
            break;

        case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
            TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
            if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
            {
                ZB_SCHEDULE_ALARM(test_step_actions, 0, THR1_SHORT_DELAY);
            }
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
    zb_buf_free(param);
}


/*! @} */
