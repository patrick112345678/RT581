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
/* PURPOSE: Dimmer switch for HA profile
*/

#define ZB_TRACE_FILE_ID 40188
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "test_config.h"

#define ZB_HA_DEFINE_DEVICE_DIMMER_SWITCH
#include "zb_ha.h"

#if defined ZB_ENABLE_HA

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif


#define HA_DIMMER_SWITCH_ENDPOINT DUT_ENDPOINT


#define TEST526_SEND_READ_ATTR()                                                           \
{                                                                                          \
 zb_uint8_t *cmd_ptr;                                                                      \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(zcl_cmd_buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE); \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);                     \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
    zcl_cmd_buf,                                                                           \
    cmd_ptr,                                                                               \
    DUT_ADDR,                                                                               \
    DUT_ADDR_MODE,                                                                          \
    DUT_ENDPOINT,                                                                           \
    HA_DIMMER_SWITCH_ENDPOINT,                                                      \
    ZB_AF_HA_PROFILE_ID,                                                                   \
    ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,                                                       \
    NULL);                                                                                 \
}

#define SEND_READ_ATTR_ON_OFF()                                            \
{                                                                                          \
 zb_uint8_t *cmd_ptr;                                                                      \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ(zcl_cmd_buf, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE); \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);                     \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
    zcl_cmd_buf,                                                                           \
    cmd_ptr,                                                                               \
    DUT_ADDR,                                                                               \
    DUT_ADDR_MODE,                                                                          \
    DUT_ENDPOINT,                                                                           \
    HA_DIMMER_SWITCH_ENDPOINT,                                                      \
    ZB_AF_HA_PROFILE_ID,                                                                   \
    ZB_ZCL_CLUSTER_ID_ON_OFF,                                                       \
    NULL);                                                                                 \
}

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

zb_ieee_addr_t g_ed_addr = ED_IEEE_ADDR;

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);


/********************* Declare device **************************/

ZB_HA_DECLARE_DIMMER_SWITCH_CLUSTER_LIST(dimmer_switch_clusters,
        basic_attr_list,
        identify_attr_list);

ZB_HA_DECLARE_DIMMER_SWITCH_EP(dimmer_switch_ep,
                               HA_DIMMER_SWITCH_ENDPOINT,
                               dimmer_switch_clusters);

ZB_HA_DECLARE_DIMMER_SWITCH_CTX(dimmer_switch_ctx, dimmer_switch_ep);

/*!
 * @brief Program states according to test scenario
 */
enum test_states_e
{
    TEST_STATE_1_VERIFY_LEVEL_OF_0,
    TEST_STATE_2_CHANGE_LIGHT_TO_FULL_ON,
    TEST_STATE_3_VERIFY_LEVEL_OF_255,
    TEST_STATE_4_CHANGE_LIGHT_TO_HALF,
    TEST_STATE_5_VERIFY_LEVEL_OF_128,
    TEST_STATE_6_TURN_LIGHTS_OFF,
    TEST_STATE_7_MOVE_TO_COLOR,

    TEST_STATE_FINISHED
};

void next_step(zb_uint8_t param);

/*! @brief Test harness state
    Takes values of @ref test_states_e
*/

zb_uint8_t g_curr_level;
zb_uint8_t g_test_state;
zb_uint8_t g_error_cnt;

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("dimmer_switch_th");

    ZB_SET_NIB_SECURITY_LEVEL(0);

    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);

    zb_set_default_ed_descriptor_values();
    ZB_AIB().aps_channel_mask = (1L << 17);
    /****************** Register Device ********************************/
    ZB_AF_REGISTER_DEVICE_CTX(&dimmer_switch_ctx);
    ZB_AF_SET_ENDPOINT_HANDLER(HA_DIMMER_SWITCH_ENDPOINT, zcl_specific_cluster_cmd_handler);

    ZB_SET_NIB_SECURITY_LEVEL(0);

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

void next_step(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);


    TRACE_MSG(TRACE_ZCL1, "> next_step: zcl_cmd_buf: %p, state %d",
              (FMT__P_D, zcl_cmd_buf, g_test_state));
    if (g_test_state == TEST_STATE_1_VERIFY_LEVEL_OF_0)
    {
        TRACE_MSG(TRACE_ZCL3, "Step 1 - Verify level of 0", (FMT__0));
        TEST526_SEND_READ_ATTR();
    }
    else if (g_test_state == TEST_STATE_2_CHANGE_LIGHT_TO_FULL_ON)
    {
        TRACE_MSG(TRACE_ZCL3, "TEST_STATE_2_CHANGE_LIGHT_TO_FULL_ON", (FMT__0));
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_WITH_ON_OFF_REQ(
            zcl_cmd_buf,
            DUT_ADDR,
            DUT_ADDR_MODE,
            DUT_ENDPOINT,
            HA_DIMMER_SWITCH_ENDPOINT,
            ZB_AF_HA_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL,
            ZB_ZCL_LEVEL_CONTROL_MOVE_MODE_UP,
            64);
    }
    else if (g_test_state == TEST_STATE_3_VERIFY_LEVEL_OF_255)
    {
        TRACE_MSG(TRACE_ZCL3, "TEST_STATE_3_VERIFY_LEVEL_OF_255", (FMT__0));
        TEST526_SEND_READ_ATTR();
    }
    else if (g_test_state == TEST_STATE_4_CHANGE_LIGHT_TO_HALF)
    {
        TRACE_MSG(TRACE_ZCL3, "TEST_STATE_4_CHANGE_LIGHT_TO_HALF", (FMT__0));
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ(
            zcl_cmd_buf,
            DUT_ADDR,
            DUT_ADDR_MODE,
            DUT_ENDPOINT,
            HA_DIMMER_SWITCH_ENDPOINT,
            ZB_AF_HA_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL,
            128,
            0);
    }
    else if (g_test_state == TEST_STATE_5_VERIFY_LEVEL_OF_128)
    {
        TRACE_MSG(TRACE_ZCL3, "TEST_STATE_5_VERIFY_LEVEL_OF_128", (FMT__0));
        TEST526_SEND_READ_ATTR();
    }
    else if (g_test_state == TEST_STATE_6_TURN_LIGHTS_OFF)
    {
        TRACE_MSG(TRACE_ZCL3, "TEST_STATE_6_TURN_LIGHTS_OFF", (FMT__0));
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(
            zcl_cmd_buf,
            DUT_ADDR,
            DUT_ADDR_MODE,
            DUT_ENDPOINT,
            HA_DIMMER_SWITCH_ENDPOINT,
            ZB_AF_HA_PROFILE_ID,
            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
            NULL);
    }
    else if (g_test_state == TEST_STATE_7_MOVE_TO_COLOR)
    {
        TRACE_MSG(TRACE_ZLL3, "TEST_STATE_7_MOVE_TO_COLOR", (FMT__0));
        ZB_ZCL_COLOR_CONTROL_SEND_MOVE_TO_COLOR_REQ(zcl_cmd_buf, DUT_ADDR, DUT_ADDR_MODE, DUT_ENDPOINT,
                HA_DIMMER_SWITCH_ENDPOINT, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL, ZB_ZCL_COLOR_CONTROL_COLOR_X_GREEN,
                ZB_ZCL_COLOR_CONTROL_COLOR_Y_GREEN, 0x0032);
    }

    if (g_test_state == TEST_STATE_FINISHED)
    {
        if (!(g_error_cnt))
        {
            TRACE_MSG(TRACE_ZCL1, "Test finished. Status: OK", (FMT__0));
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1,
                      "Test finished. Status: FAILED, error number %hd", (FMT__H, (g_error_cnt)));
        }
    }

    TRACE_MSG(TRACE_ZCL3, "< next_step: g_test_state: %hd", (FMT__H, g_test_state));
}

void read_attr_resp_handler(zb_buf_t *zcl_cmd_buf)
{
    zb_zcl_read_attr_res_t *read_attr_resp;
    zb_bool_t err = ZB_TRUE;

    TRACE_MSG(TRACE_ZCL3, "> read_attr_resp_handler", (FMT__0));
    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(zcl_cmd_buf, read_attr_resp);
    switch (g_test_state)
    {
    case TEST_STATE_1_VERIFY_LEVEL_OF_0:
        if (read_attr_resp)
        {
            if (read_attr_resp->status == ZB_ZCL_STATUS_SUCCESS && *read_attr_resp->attr_value == 0)
            {
                err = ZB_FALSE;
            }
            else
            {
                TRACE_MSG(TRACE_ZCL3, "ERROR! Status %i value %i", (FMT__H_H, read_attr_resp->status,
                          *read_attr_resp->attr_value));
            }
        }
        break;
    case TEST_STATE_3_VERIFY_LEVEL_OF_255:
        if (read_attr_resp)
        {
            if (read_attr_resp->status == ZB_ZCL_STATUS_SUCCESS && *read_attr_resp->attr_value == 255)
            {
                err = ZB_FALSE;
            }
        }
        break;
    /* TODO: CR:MEDIUM check value taking into account delta +-5 */
    /* NK: Fixed */
    case TEST_STATE_5_VERIFY_LEVEL_OF_128:
        if (read_attr_resp)
        {
            if (read_attr_resp->status == ZB_ZCL_STATUS_SUCCESS
                    && (*read_attr_resp->attr_value >= 128 - 5) && (*read_attr_resp->attr_value <= 128 + 5))
            {
                err = ZB_FALSE;
            }
        }
        break;
    }

    if (err)
    {
        ++g_error_cnt;
    }

    if (g_test_state != TEST_STATE_FINISHED)
    {
        ++g_test_state;
    }

    TRACE_MSG(TRACE_ZCL3, "< read_attr_resp_handler", (FMT__0));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;
    zb_zcl_default_resp_payload_t *default_res;

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
    {
        switch (cmd_info->cluster_id)
        {
        case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
        case ZB_ZCL_CLUSTER_ID_ON_OFF:
            if (cmd_info->is_common_command)
            {
                switch (cmd_info->cmd_id)
                {
                case ZB_ZCL_CMD_DEFAULT_RESP:
                    TRACE_MSG(TRACE_ZCL1, "Process general command %hd", (FMT__H, cmd_info->cmd_id));
                    default_res = ZB_ZCL_READ_DEFAULT_RESP(zcl_cmd_buf);
                    TRACE_MSG(TRACE_ZCL2, "ZB_ZCL_CMD_DEFAULT_RESP getted: command_id 0x%hx, status: 0x%hx",
                              (FMT__H_H, default_res->command_id, default_res->status));

                    if (default_res->status != ZB_ZCL_STATUS_SUCCESS)
                    {
                        TRACE_MSG(TRACE_ZCL1, "state %i error found", (FMT__H, g_test_state + 1));
                        ++g_error_cnt;
                        g_test_state = TEST_STATE_FINISHED;
                    }
                    else
                    {
                        TRACE_MSG(TRACE_ZCL2, "go to next g_test_state", (FMT__0));
                        ++g_test_state;
                    }

                    next_step(param);

                    break;

                case ZB_ZCL_CMD_READ_ATTRIB_RESP:
                    read_attr_resp_handler(zcl_cmd_buf);
                    next_step(param);
                    break;

                default:
                    TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
                    break;
                }
                cmd_processed = ZB_TRUE;
            }
            else
            {
                TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            break;
        }
    }
    else
    {
        /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
        switch (cmd_info->cluster_id)
        {
        case ZB_ZCL_CLUSTER_ID_IDENTIFY:
            if (cmd_info->is_common_command)
            {
                switch (cmd_info->cmd_id)
                {
                default:
                    TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            else
            {
                switch (cmd_info->cmd_id)
                {
                case ZB_ZCL_CMD_IDENTIFY_IDENTIFY_ID:
                    TRACE_MSG(TRACE_ZCL3, "Got cluster command 0x%04x", (FMT__D, cmd_info->cmd_id));
                    /* Process cluster command */
                    cmd_processed = ZB_TRUE;
                    break;

                default:
                    TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            break;
        }
    }

    TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %h", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));

        g_test_state = TEST_STATE_1_VERIFY_LEVEL_OF_0;
        g_error_cnt = 0;

        next_step(param);
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
    TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA

#include <stdio.h>
int main()
{
    printf(" HA is not supported\n");
    return 0;
}

#endif // defined ZB_ENABLE_HA
