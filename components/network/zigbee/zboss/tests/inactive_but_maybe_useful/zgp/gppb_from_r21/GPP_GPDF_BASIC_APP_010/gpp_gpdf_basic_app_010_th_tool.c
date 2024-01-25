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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPP_GPDF_BASIC_APP_010_TH_TOOL
#define ZB_TRACE_FILE_ID 41514
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zb_secur_api.h"
#include "zgp/zgp_internal.h"

#ifndef ZB_NSNG
#include "zb_led_button.h"
#endif

#include "zb_ha.h"

#include "test_config.h"

#include "../include/simple_combo_match_info.h"
#include "../include/simple_combo_controller.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#define ENDPOINT 10

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;
static zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};

static zb_uint8_t g_comm_cnt = 0;

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
static zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param);
static void next_step(zb_buf_t *zcl_cmd_buf);

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(BASIC_ATTR_LIST, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(IDENTIFY_ATTR_LIST, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_SAMPLE_CLUSTER_LIST(SAMPLE_CLUSTERS,
                                  BASIC_ATTR_LIST,
                                  IDENTIFY_ATTR_LIST);

ZB_HA_DECLARE_SAMPLE_EP(SAMPLE_EP, ENDPOINT, SAMPLE_CLUSTERS, SDESC_EP);

ZB_HA_DECLARE_SAMPLE_CTX(SAMPLE_CTX, SAMPLE_EP);

/*! Program states according to test scenario */
enum test_states_e
{
    TEST_STATE_INITIAL,
    TEST_STATE_READ_GPP_PROXY_TABLE1,
    TEST_STATE_FINISHED
};

/*! @brief Test harness state
    Takes values of @ref test_states_e
*/
//warning: first state TEST_STATE_INITIAL cause assertion failed error!
static zb_uint8_t g_test_state = TEST_STATE_READ_GPP_PROXY_TABLE1;
static zb_short_t g_error_cnt = 0;

MAIN()
{
    ARGV_UNUSED;
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_secur_setup_nwk_key(g_key_nwk, 0);

    zb_set_default_ed_descriptor_values();

    /* Must use NVRAM for ZGP */
    ZB_AIB().aps_use_nvram = 1;

    ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&SAMPLE_CTX);
    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, test_specific_cluster_cmd_handler);

    zgp_cluster_set_app_zcl_cmd_handler(zcl_specific_cluster_cmd_handler);

    /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
    ZG->nwk.skip_gpdf = 1;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
    ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_USE_BUTTONS
    /* Left - start comm. mode */
    //  zb_button_register_handler(0, 0, start_comm);
    /* Right - stop comm. mode */
    //  zb_button_register_handler(1, 0, stop_comm);
#endif
#ifdef ZB_CERTIFICATION_HACKS
    ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

    ZB_NIB().nwk_use_multicast = ZB_FALSE;
    ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

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

/*! Test step */
static void next_step(zb_buf_t *zcl_cmd_buf)
{
    TRACE_MSG(TRACE_ZCL1, "> next_step: zcl_cmd_buf: %p, state %d",
              (FMT__P_D, zcl_cmd_buf, g_test_state));

    if (g_test_state == TEST_STATE_READ_GPP_PROXY_TABLE1)
    {
        zgp_cluster_read_attr(ZB_REF_FROM_BUF(zcl_cmd_buf), DUT_GPPB_ADDR, DUT_GPPB_ADDR_MODE,
                              ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                              ZB_ZCL_ENABLE_DEFAULT_RESPONSE, NULL);
    }

    if (g_test_state == TEST_STATE_FINISHED)
    {
        zb_free_buf(zcl_cmd_buf);
        if (!g_error_cnt)
        {
            TRACE_MSG(TRACE_ZCL1, "Test finished. Status: OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "Test finished. Status: FAILED, error number %hd", (FMT__H, g_error_cnt));
        }
    }

    TRACE_MSG(TRACE_ZCL3, "< next_step: g_test_state: %hd", (FMT__H, g_test_state));
}

static void read_attr_handler(zb_buf_t *zcl_cmd_buf)
{
    zb_bool_t test_error = ZB_FALSE;

    switch (g_test_state)
    {
    case TEST_STATE_READ_GPP_PROXY_TABLE1:
    {
        zb_zcl_read_attr_res_t *read_attr_resp;

        ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(zcl_cmd_buf, read_attr_resp);
        TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_READ_ATTRIB_RESP getted: attr_id 0x%hx, status: 0x%hx, value 0x%hd",
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
        TRACE_MSG(TRACE_ZCL1, "Error at state: %hd", (FMT__H, g_test_state));
    }
    ++g_test_state;
}

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    /** [VARIABLE] */
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;
    /** [VARIABLE] */


    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %hd", (FMT__H, param));

    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %hd", (FMT__H, ZB_BUF_LEN(zcl_cmd_buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        TRACE_MSG(TRACE_ZCL1, "Skip command, Unsupported direction \"to client\"", (FMT__0));
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
                case ZB_ZCL_CMD_READ_ATTRIB_RESP:
                    read_attr_handler(zcl_cmd_buf);
                    ++g_test_state;
                    next_step(zcl_cmd_buf);
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

static void comm_done_cb(zb_zgpd_id_t *zgpd_id,
                         zb_zgp_comm_status_t result)
{
    ZVUNUSED(zgpd_id);
    ZVUNUSED(result);
    TRACE_MSG(TRACE_APP1, "commissioning stopped", (FMT__0));
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif

    if (g_comm_cnt < MAX_COMMISSIONING_COUNT)
    {
        ZB_SCHEDULE_ALARM(next_step_delayed, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(15000));
    }
}

static void start_comm(zb_uint8_t param)
{
    ZVUNUSED(param);
    TRACE_MSG(TRACE_APP1, "start commissioning", (FMT__0));
    ZB_ZGP_REGISTER_COMM_COMPLETED_CB(comm_done_cb);
    zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() * ZB_TIME_ONE_SECOND);

#ifdef ZB_USE_BUTTONS
    zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
}

static void prepare_for_test(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;
    start_comm(0);
}

static zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "test_specific_cluster_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    switch (cmd_info->cluster_id)
    {
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
        if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
        {
            switch (cmd_info->cmd_id)
            {
            case ZB_ZCL_CMD_ON_OFF_ON_ID:
                TRACE_MSG(TRACE_ZCL1, "ON", (FMT__0));
                processed = ZB_TRUE;
                break;
            case ZB_ZCL_CMD_ON_OFF_OFF_ID:
                TRACE_MSG(TRACE_ZCL1, "OFF", (FMT__0));
                processed = ZB_TRUE;
                break;
            case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
                TRACE_MSG(TRACE_ZCL1, "TOGGLE", (FMT__0));
                processed = ZB_TRUE;
                break;
            }
        }
        break;
    }

    if (processed)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }

    return processed;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_ALARM(prepare_for_test, 0,
                          ZB_MILLISECONDS_TO_BEACON_INTERVAL(2000));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Error, Device start FAILED status %d",
                  (FMT__D, (int)buf->u.hdr.status));
    }
    zb_free_buf(buf);

    TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include <stdio.h>
int main()
{
    printf(" HA and ZGP sink is not supported\n");
    return 0
}

#endif // defined ZB_ENABLE_HA
