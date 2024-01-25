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
/* PURPOSE: Non color scene controller for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41585
#include "zll/zb_zll_common.h"
#include "test_defs.h"
#include "zb_bdb_internal.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_DIMMABLE_LIGHT

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile zed tests
#endif

/** Number of the endpoint device operates on. */
#define ENDPOINT  10

/** ZCL command handler. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** Start device parameters checker. */
void test_check_start_status(zb_uint8_t param);

/** Next test step initiator. */
void test_next_step(zb_uint8_t param);

/** Test read attribute */
void read_attr_resp_handler(zb_buf_t *cmd_buf);

/** Test step enumeration. */
enum test_step_e
{
    TEST_STEP_INITIAL,            /**< Initial test pseudo-step (device startup). */
    TEST_STEP_START,              /**< Device start test. */

    TEST_STEP_SEND_ON,            /**< ZED unicasts a ZCL on command frame to ZR. */
    TEST_STEP_SEND_MOVE_LEVEL_80, /**< ZED unicasts a ZCL move to level (with on/off) with the level = 0x80. */
    TEST_STEP_SEND_OFF,           /**< ZED unicasts a ZCL off command frame to ZR. */

    TEST_STEP_SEND_READ_ATTR,     /**< 1 ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x80. */
    TEST_STEP_SEND_MOVE_40,       /**< 2a ZED unicasts a ZCL move (with on/off) with mode = to 0x00 (move up) and rate = 0x40.
                                     ZR turns on and increases to full brightness over 4 seconds. */
    TEST_STEP_SEND_READ_ATTR2,    /**< 2b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0xfe. */
    TEST_STEP_SEND_MOVE_LEVEL_80_0,/**< 3a ZED unicasts a ZCL move to level with Level = of 0x80 and
                                    transition time = 0x0000. ZR immediately reduces its brightness to its mid point. */
    TEST_STEP_SEND_READ_ATTR3,    /**< 3b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x80. */
    TEST_STEP_SEND_OFF2,          /**< 4a ZED unicasts a ZCL off command to ZR. ZR turns off. */
    TEST_STEP_SEND_READ_ATTR4,    /**< 4b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x80. */
    TEST_STEP_SEND_ON2,           /**< 5a ZED unicasts a ZCL on command to ZR. ZR turns off. */
    TEST_STEP_SEND_READ_ATTR5,    /**< 5b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x80. */
    TEST_STEP_SEND_STEP,          /**< 6a ZED unicasts a ZCL step command to ZR, with step mode = 0x01 (step down),
                                     step size = 0x40 and transition time = 0x0014 (2s). ZR reduces its brightness. */
    TEST_STEP_SEND_STEP2,         /**< 6b ZED unicasts a ZCL step command to ZR, with step mode = 0x01 (step down),
                                     step size = 0x40 and transition time = 0x0014 (2s). ZR reduces its brightness
                                     to its minimum level.*/
    TEST_STEP_SEND_READ_ATTR6,    /**< 6c ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x01. */
    TEST_STEP_SEND_TOGGLE, /*17*/ /**< 7a ZED unicasts a ZCL toggle command to ZR. ZR turns off. */
    TEST_STEP_SEND_READ_ATTR7,    /**< 7b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x01. */
    TEST_STEP_SEND_READ_ATTR8,    /**< 7b ZED unicasts a ZCL read attributes OnOff. OnOff = 0x00. */
    TEST_STEP_SEND_TOGGLE2,       /**< 8a ZED unicasts a ZCL toggle command to ZR. ZR turns on. */
    TEST_STEP_SEND_READ_ATTR9,    /**< 8b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x01. */
    TEST_STEP_SEND_READ_ATTR10,   /**< 8b ZED unicasts a ZCL read attributes OnOff. OnOff = 0x01. */
    TEST_STEP_SEND_MOVE_0_A,      /**< 9a ZED unicasts a ZCL move command with mode = of 0x00 (move up) and
                                    rate = 0x0a. ZR begins to increase its brightness. */
    TEST_STEP_SEND_STOP,   /*24*/ /**< 9b ZED unicasts a ZCL stop [0x03] command to ZR. ZR stops adjusting its brightness.*/
    TEST_STEP_SEND_READ_ATTR11,   /**< 9c ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x65. */
    TEST_STEP_SEND_MOVE_1_4,      /**< 10a ZED unicasts a ZCL move command to ZR, with mode = 0x01 (move down) and
                                     rate = 0x04. ZR begins to decrease its brightness. */
    TEST_STEP_SEND_STOP2,  /*27*/ /**< 10b ZED unicasts a ZCL stop [0x07] command to ZR. ZR stops adjusting its brightness.*/
    TEST_STEP_SEND_READ_ATTR12,   /**< 10c ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x3d. */
    TEST_STEP_SEND_MOVE_LEVEL_0_258,/**< 11a ZED unicasts a ZCL move to level (with on/off) command to ZR, with level = 0x00
                                     and transition time = 0x0258 (60s). ZR “light” immediately begins to decrease its brightness. */
    TEST_STEP_SEND_READ_ATTR13,   /**< 11b ZED unicasts a ZCL read attributes RemainingTime . RemainingTime = 0x001f4. */
    TEST_STEP_SEND_READ_ATTR14,   /**< 11c ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x01. */
    TEST_STEP_SEND_READ_ATTR15,   /**< 11c ZED unicasts a ZCL read attributes OnOff. OnOff = 0x00. */
    TEST_STEP_SEND_STEP3,  /*33*/ /**< 12a ZED unicasts a ZCL step with on/off command to ZR, with step mode = 0x00 (step up),
                                     step size = 0x01 and transition time =  0xffff (as fast as possible).
                                     ZR turns on at its minimum brightness.*/
    TEST_STEP_SEND_READ_ATTR16,   /**< 12b ZED unicasts a ZCL read attributes CurrentLevel. CurrentLevel = 0x02. */
    TEST_STEP_SEND_READ_ATTR17,   /**< 12b ZED unicasts a ZCL read attributes OnOff. OnOff = 0x01. */

    TEST_STEP_FINISHED    /*36*/  /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
        ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(non_color_scene_controller_ep, ENDPOINT,
        non_color_scene_controller_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(non_color_scene_controller_ctx,
        non_color_scene_controller_ep);

/******************* Declare router parameters *****************/
#define DST_ADDR        0x0001
#define DST_ENDPOINT    5
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/

zb_short_t g_error_cnt = 0;

/* Send command "read attribute" to server */
#define SEND_READ_ATTR(buffer, clusterID, attributeID)                                     \
{                                                                                          \
  zb_uint8_t *cmd_ptr;                                                                     \
  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ((buffer), cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);    \
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(cmd_ptr, (attributeID));                             \
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(                                                       \
      (buffer), cmd_ptr, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT, ENDPOINT,                  \
      ZB_AF_ZLL_PROFILE_ID, (clusterID), NULL);                                            \
}

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR || defined ZB_PLATFORM_LINUX_ARM_2400)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zed1");


    ZB_SET_NIB_SECURITY_LEVEL(0);

    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();

    /****************** Register Device ********************************/
    /* Register device list */
    ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
    //ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

    ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

#if 0
    ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
#endif

    ZG->nwk.nib.security_level = 0;

    ZB_AIB().aps_use_nvram = 1;
    ZB_AIB().aps_nvram_erase_at_start = 1;
    ZB_BDB().bdb_mode = ZB_TRUE;

    if (zb_zdo_start_no_autostart() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
        ++g_error_cnt;
    }
    else
    {
        zcl_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

    if ( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
    {
        switch ( cmd_info -> cluster_id )
        {
        case ZB_ZCL_CLUSTER_ID_ON_OFF:
        case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
            if ( cmd_info -> is_common_command )
            {
                switch ( cmd_info -> cmd_id )
                {
                case ZB_ZCL_CMD_DEFAULT_RESP:
                    TRACE_MSG(TRACE_ZCL3, "Got response in cluster 0x%04x",
                              ( FMT__D, cmd_info->cluster_id));
                    /* Process default response */
                    cmd_processed = ZB_TRUE;
                    break;

                case ZB_ZCL_CMD_READ_ATTRIB_RESP:
                    read_attr_resp_handler(zcl_cmd_buf);
                    cmd_processed = ZB_TRUE;
                    break;

                default:
                    TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
                    break;
                }
            }
            else
            {
                switch ( cmd_info -> cmd_id )
                {
                //case ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR_RES:
                //  TRACE_MSG(TRACE_ZCL1, "Response:  ZB_ZCL_CMD_DOOR_LOCK_LOCK_DOOR_RES", (FMT__0));
                //  plpayload = ZB_ZCL_DOOR_LOCK_READ_LOCK_DOOR_RES(zcl_cmd_buf);
                //  break;
                default:
                    TRACE_MSG(TRACE_ZCL2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
                    //++g_error_cnt;
                    break;
                }
            }
            break;

        default:
            TRACE_MSG(TRACE_ZCL1, "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
            ++g_error_cnt;
            break;
        }
    }
    else
    {
        /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
        TRACE_MSG(TRACE_ZCL1, "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
        ++g_error_cnt;
    }

    // for TEST_STEP_SEND_READ_ATTR13 next step only zb_scheduler_alarm!
    if (g_test_step == TEST_STEP_SEND_READ_ATTR13)
    {
        zb_free_buf(zcl_cmd_buf);
    }
    else
    {
        test_next_step(param);
    }

    cmd_processed = ZB_TRUE;

    TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    switch (sig)
    {
    case ZB_ZDO_SIGNAL_SKIP_STARTUP:
        TRACE_MSG(TRACE_ZLL1, "Device start skip startup", (FMT__0));
        test_check_start_status(param);
        break;
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_ZLL1, "Device first start", (FMT__0));
        break;
    case ZB_BDB_SIGNAL_TOUCHLINK:
        TRACE_MSG(TRACE_ZLL1, "Touchlink commissioning as initiator done ok", (FMT__0));
        break;
    default:
        TRACE_MSG(TRACE_ZLL1, "Unknown signal %hd status %hd", (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
        break;
    }

    if (g_test_step == TEST_STEP_FINISHED)
    {
        zb_free_buf(ZB_BUF_FROM_REF(param));
    }
    else
    {
        test_next_step(param);
    }

    TRACE_MSG(TRACE_ZLL3, "< zb_zdo_startup_complete", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
    zb_buf_t *buffer = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));
#if 0
    if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)",
                  (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
        ++g_error_cnt;
    }
#endif
    if ( ZB_ZLL_IS_FACTORY_NEW() )
    {
        if (ZB_PIBCACHE_NETWORK_ADDRESS() != 0xffff)
        {
            TRACE_MSG(TRACE_ERROR, "ERROR Network address is 0x%04x (should be 0xffff)",
                      (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
            ++g_error_cnt;
        }
    }

    if (! ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
        TRACE_MSG(TRACE_ERROR, "ERROR Receiver should be turned on", (FMT__0));
        ++g_error_cnt;
    }
    /*
      if (! ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
      {
        TRACE_MSG(TRACE_ERROR, "ERROR extended PAN Id is not zero: %s",
            (FMT__A, ZB_NIB_EXT_PAN_ID()));
        ++g_error_cnt;
      }

      if (ZB_PIBCACHE_PAN_ID() != 0xffff)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR PAN Id is 0x%04x (should be 0xffff)",
            (FMT__D, ZB_PIBCACHE_PAN_ID()));
        ++g_error_cnt;
      }
      */
    if (ZB_GET_APP_SIGNAL_STATUS(param) == ZB_ZLL_GENERAL_STATUS_SUCCESS && ! g_error_cnt)
    {
        TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (errors: %hd)", (FMT__H, g_error_cnt));
    }

    TRACE_MSG(TRACE_ZLL3, "< test_check_start_status", (FMT__0));
}/* void test_check_start_status(zb_uint8_t param) */

void test_timer_next_step(zb_uint8_t param)
{
    (void)param;

    //g_test_step++;
    ZB_GET_OUT_BUF_DELAYED(test_next_step);
}

void test_next_step(zb_uint8_t param)
{
    zb_buf_t *buffer = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

    switch (g_test_step )
    {
    case TEST_STEP_INITIAL:
        TRACE_MSG(TRACE_ZLL3, "Start commissioning", (FMT__0));
        g_test_step = TEST_STEP_START;
        bdb_start_top_level_commissioning(ZB_BDB_TOUCHLINK_COMMISSIONING);
        break;

    case TEST_STEP_START:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON (p1)", (FMT__0));
        g_test_step = TEST_STEP_SEND_ON;
        ZB_ZCL_ON_OFF_SEND_ON_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                  ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_ON:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_LEVEL_80 (p2)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_LEVEL_80;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x80, 0x00);
        break;

    case TEST_STEP_SEND_MOVE_LEVEL_80:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_OFF (p3)", (FMT__0));
        g_test_step = TEST_STEP_SEND_OFF;
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                   ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_OFF:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR (1)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_40 (2a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_40;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x00, 0x40);
        break;

    case TEST_STEP_SEND_MOVE_40:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR2 (2b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR2;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_LEVEL_80_0 (3a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_LEVEL_80_0;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x80, 0x0000);
        break;

    case TEST_STEP_SEND_MOVE_LEVEL_80_0:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR3 (3b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR3;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR3:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_OFF2 (4a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_OFF2;
        ZB_ZCL_ON_OFF_SEND_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                   ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_OFF2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR4 (4b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR4;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR4:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON2 (5a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_ON2;
        ZB_ZCL_ON_OFF_SEND_ON_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                  ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_ON2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR5 (5b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR5;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR5:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_STEP (6a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_STEP;
        ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                           ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x01, 0x40, 0x0014);
        //ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 2*ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_STEP:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_STEP2 (6b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_STEP2;
        ZB_ZCL_LEVEL_CONTROL_SEND_STEP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                           ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x01, 0x40, 0x0014);
        //ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 2*ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_STEP2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR6 (6c)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR6;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR6:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_TOGGLE (7a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_TOGGLE;
        ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                      ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_TOGGLE:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR7 (7b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR7;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR7:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR8 (7b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR8;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR8:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_TOGGLE2 (8a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_TOGGLE2;
        ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                      ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
        break;

    case TEST_STEP_SEND_TOGGLE2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR9 (8b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR9;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR9:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR10 (8b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR10;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR10:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_0_A (9a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_0_A;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                           ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_TRUE, NULL, 0x00, 0x0a);
        ZB_SCHEDULE_ALARM(test_timer_next_step, 0, /*10*/ 10.8 * ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_MOVE_0_A:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_STOP (9b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_STOP;
        ZB_ZCL_LEVEL_CONTROL_SEND_STOP_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                           ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_TRUE, NULL);
        ZB_GET_OUT_BUF_DELAYED(test_next_step);
        break;

    case TEST_STEP_SEND_STOP:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR11 (9c)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR11;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR11:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_1_4 (10a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_1_4;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                                           ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_TRUE, NULL, 0x01, 0x04);
        ZB_SCHEDULE_ALARM(test_timer_next_step, 0, /*10*/8.3 * ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_MOVE_1_4:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_STOP2 (10b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_STOP2;
        ZB_ZCL_LEVEL_CONTROL_SEND_STOP_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_TRUE, NULL);
        ZB_GET_OUT_BUF_DELAYED(test_next_step);
        break;

    case TEST_STEP_SEND_STOP2:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR12 (10c)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR12;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR12:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_MOVE_LEVEL_0_258 (11a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_MOVE_LEVEL_0_258;
        ZB_ZCL_LEVEL_CONTROL_SEND_MOVE_TO_LEVEL_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_TRUE, NULL, 0x00, 0x0258);
        ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10 * ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_MOVE_LEVEL_0_258:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR13 (11b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR13;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_REMAINING_TIME_ID);
        ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 70 * ZB_TIME_ONE_SECOND);
        break;

    case TEST_STEP_SEND_READ_ATTR13:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR14 (11c)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR14;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR14:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR15 (11c)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR15;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR15:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_STEP3 (12a)", (FMT__0));
        g_test_step = TEST_STEP_SEND_STEP3;
        ZB_ZCL_LEVEL_CONTROL_SEND_STEP_WITH_ON_OFF_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
                ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL, 0x00, 0x01, 0xffff);
        break;

    case TEST_STEP_SEND_STEP3:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR16 (12b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR16;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR16:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_READ_ATTR17 (12b)", (FMT__0));
        g_test_step = TEST_STEP_SEND_READ_ATTR17;
        SEND_READ_ATTR(buffer, ZB_ZCL_CLUSTER_ID_ON_OFF, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);
        break;

    case TEST_STEP_SEND_READ_ATTR17:
        TRACE_MSG(TRACE_ZLL3, "TEST_STEP_FINISHED", (FMT__0));
        g_test_step = TEST_STEP_FINISHED;
        break;

    default:
        g_test_step = TEST_STEP_FINISHED;
        TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
        ++g_error_cnt;
        break;
    }

    if (g_test_step == TEST_STEP_FINISHED /*|| g_error_cnt*/)
    {
        if (g_error_cnt)
        {
            TRACE_MSG(TRACE_ERROR, "ERROR Test failed with %hd errors", (FMT__H, g_error_cnt));
        }
        else
        {
            TRACE_MSG(TRACE_ERROR, "Test finished. Status: OK", (FMT__0));
        }
        zb_free_buf(buffer);
    }

    TRACE_MSG(TRACE_ZLL3, "< test_next_step. Curr step %hd", (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */

void read_attr_resp_handler(zb_buf_t *cmd_buf)
{
    zb_zcl_read_attr_res_t *read_attr_resp;

    TRACE_MSG(TRACE_ZCL1, ">> read_attr_resp_handler", (FMT__0));

    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
    TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));
    if (read_attr_resp)
    {
        if (ZB_ZCL_STATUS_SUCCESS != read_attr_resp->status)
        {
            TRACE_MSG(TRACE_ERROR,
                      "ERROR incorrect response: id 0x%04x, status %d",
                      (FMT__D_H, read_attr_resp->attr_id, read_attr_resp->status));
            g_error_cnt++;
        }

        switch (g_test_step)
        {
        case TEST_STEP_SEND_READ_ATTR:
        case TEST_STEP_SEND_READ_ATTR3:
        case TEST_STEP_SEND_READ_ATTR4:
        case TEST_STEP_SEND_READ_ATTR5:
            if (read_attr_resp->attr_value[0] != 0x80 || ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR2:
            if (read_attr_resp->attr_value[0] != 0xff /*0xfe*/ || ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR6:     // step 6c
        case TEST_STEP_SEND_READ_ATTR7:     // step 7b
        case TEST_STEP_SEND_READ_ATTR9:     // step 8b
        case TEST_STEP_SEND_READ_ATTR14:    // step 11c
            if (read_attr_resp->attr_value[0] != 0x00 /*0x01*/ || ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR10:
        case TEST_STEP_SEND_READ_ATTR17:
            if (read_attr_resp->attr_value[0] != 0x01 || ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR8:
        case TEST_STEP_SEND_READ_ATTR15:
            if (read_attr_resp->attr_value[0] != 0x00 || ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR11:
            //5 - delta
            if (0x65 - 5 > read_attr_resp->attr_value[0] || read_attr_resp->attr_value[0] > 0x65 + 5 ||
                    ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR12:
            //2 - delta
            if (0x3d - 2 > read_attr_resp->attr_value[0] || read_attr_resp->attr_value[0] > 0x3d + 2 ||
                    ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        case TEST_STEP_SEND_READ_ATTR13:
            //20 - delta
            // *(zb_uint16_t*)read_attr_resp->attr_value  must be about 0x01f4
            if (0x01f4 - 20 >  *(zb_uint16_t *)read_attr_resp->attr_value ||
                    *(zb_uint16_t *)read_attr_resp->attr_value > 0x01f4 + 20 ||
                    ZB_ZCL_ATTR_LEVEL_CONTROL_REMAINING_TIME_ID != read_attr_resp->attr_id)
            {
                //TODO: fix until RemainingTime calculation will be added
                if (*(zb_uint16_t *)read_attr_resp->attr_value != 0)
                {
                    g_error_cnt++;
                    TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                              read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
                }
            }
            TRACE_MSG(TRACE_ERROR, "value 0x%x 0x%x", (FMT__H_H, read_attr_resp->attr_value[0], read_attr_resp->attr_value[1]));
            break;

        case TEST_STEP_SEND_READ_ATTR16:
            if (read_attr_resp->attr_value[0] != 0x01 /*0x02*/ || ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID != read_attr_resp->attr_id)
            {
                g_error_cnt++;
                TRACE_MSG(TRACE_ERROR, "ERROR id 0x%x value %hx step %d", (FMT__D_H_H,
                          read_attr_resp->attr_id, read_attr_resp->attr_value[0], g_test_step));
            }
            break;

        default:
            TRACE_MSG(
                TRACE_ERROR,
                "ERROR! Unexpected read attributes response: test step 0x%hx",
                (FMT__D, g_test_step));
            ++g_error_cnt;
            break;
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "ERROR, No info on attribute(s) read", (FMT__0));
        g_error_cnt++;
    }

    TRACE_MSG(TRACE_ZCL1, "<< read_attr_resp_handler", (FMT__0));
}

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
    printf(" ZLL is not supported\n");
    return 0;
}

#endif // defined ZB_ENABLE_ZLL
