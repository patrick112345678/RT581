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

#define ZB_TRACE_FILE_ID 41614
#include "zll/zb_zll_common.h"
#include "test_defs.h"

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
void read_attr_resp_handler(zb_buf_t * cmd_buf);

/** Test step enumeration. */
enum test_step_e
{
  TEST_STEP_INITIAL,            /**< Initial test pseudo-step (device startup). */
  TEST_STEP_START,              /**< Device start test. */

  // TODO Insert test steps ID

  TEST_STEP_FINISHED            /**< Test finished pseudo-step. */
};

zb_uint8_t g_test_step = TEST_STEP_INITIAL;
zb_uint8_t current_seq_number;

/** Device's extended address. */
zb_ieee_addr_t g_ed_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_ZLL(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attribute data */
zb_uint8_t g_attr_scenes_scene_count = ZB_ZCL_SCENES_SCENE_COUNT_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_current_scene = ZB_ZCL_SCENES_CURRENT_SCENE_DEFAULT_VALUE;
zb_uint16_t g_attr_scenes_current_group = ZB_ZCL_SCENES_CURRENT_GROUP_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_scene_valid = ZB_ZCL_SCENES_SCENE_VALID_DEFAULT_VALUE;
zb_uint8_t g_attr_scenes_name_support = ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* Scenes cluster attribute data */

zb_uint8_t g_level_control_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST_ZLL(
    level_control_attr_list, &g_level_control_current_level, &g_level_control_remaining_time);
/********************* Declare device **************************/
ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CLUSTER_LIST(non_color_scene_controller_clusters,
    basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list, on_off_attr_list, level_control_attr_list, ZB_ZCL_CLUSTER_MIXED_ROLE);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP(
    non_color_scene_controller_ep, ENDPOINT, non_color_scene_controller_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(
    non_color_scene_controller_ctx, non_color_scene_controller_ep);

/******************* Declare router parameters *****************/
#define DST_ADDR        0x01
#define DST_ENDPOINT    5
#define DST_ADDR_MODE   ZB_APS_ADDR_MODE_16_ENDP_PRESENT
/******************* Declare test data & constants *************/

zb_short_t g_error_cnt = 0;

zb_uint8_t dev_num = 0;

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

#ifndef KEIL
  if ( argc < 2 )
  {
    printf("%s <depth>\n", argv[0]);
    return 0;
  }
  dev_num = atoi(argv[1]);
#endif

  /* Init device, load IB values from nvram or set it to default */
{

      ZB_INIT("zed");

}
  ZB_SET_NIB_SECURITY_LEVEL(0);

  g_ed_addr[0]=2+dev_num;

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ed_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&non_color_scene_controller_ctx);
  //ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, zcl_specific_cluster_cmd_handler);

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;

  ZG->nwk.nib.security_level = 0;

  if (zb_zll_dev_start() != RET_OK)
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

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_ZCL1, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if(current_seq_number!=cmd_info->seq_number)
  {
    TRACE_MSG(TRACE_ZCL2, "step %hd current_seq_number %hd cmd_info->seq_number %hd", (FMT__H_H_H,
          g_test_step, current_seq_number, cmd_info->seq_number));
    return ZB_FALSE;
  }

  if( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI )
  {
    switch( cmd_info -> cluster_id )
    {
      case ZB_ZCL_CLUSTER_ID_ON_OFF:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              TRACE_MSG(TRACE_ZCL3, "Got response in cluster 0x%04x",
                        ( FMT__D, cmd_info->cluster_id));
              /* Process default response */
              break;

            case ZB_ZCL_CMD_READ_ATTRIB_RESP:
              read_attr_resp_handler(zcl_cmd_buf);
              break;

            default:
              TRACE_MSG(TRACE_ZCL2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        else
        {
          switch( cmd_info -> cmd_id )
          {
            // if server may send command to client then descript client activity here.

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

  test_next_step(param);

  TRACE_MSG(TRACE_ZCL1, "<< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_TRUE;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> zb_zdo_startup_complete %hd status %hd", (FMT__H_H, param, task_status->task));

  switch (task_status->task)
  {
  case ZB_ZLL_DEVICE_START_TASK:
    test_check_start_status(param);
    break;

  case ZB_ZLL_TRANSACTION_NWK_START_TASK:
    TRACE_MSG(TRACE_ZLL3, "New Network status %hd", (FMT__H, task_status->status));
    if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
      ++g_error_cnt;
    }
    break;

  case ZB_ZLL_TRANSACTION_JOIN_ROUTER_TASK:
    TRACE_MSG(TRACE_ZLL3, "Join Router status %hd", (FMT__H, task_status->status));
    if (task_status->status != ZB_ZLL_TASK_STATUS_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR status %hd", (FMT__H, task_status->status));
      ++g_error_cnt;
    }
    break;

  default:
    TRACE_MSG(TRACE_ERROR, "ERROR unsupported task %hd", (FMT__H, task_status->task));
    ++g_error_cnt;
    break;
  }

  if (g_error_cnt)
  {
    g_test_step = TEST_STEP_FINISHED;
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
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR wrong channel %hd (should be %hd)",
        (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
    ++g_error_cnt;
  }

  if( ZB_ZLL_IS_FACTORY_NEW() )
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

  if (task_status->status == ZB_ZLL_GENERAL_STATUS_SUCCESS && ! g_error_cnt)
  {
    TRACE_MSG(TRACE_ZLL3, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ZLL3, "ERROR Device start FAILED (errors: %hd)", (FMT__H, g_error_cnt));
  }

  TRACE_MSG(TRACE_ZLL3, "< test_check_start_status", (FMT__0));
}/* void test_check_start_status(zb_uint8_t param) */

/* For run next step without wait end previous command
 *
 * 10 sec:
 * ZB_SCHEDULE_ALARM(test_timer_next_step, 0, 10*ZB_TIME_ONE_SECOND);
 *
 * Use in test_next_step func
 * */
void test_timer_next_step(zb_uint8_t param)
{
  (void)param;

  TRACE_MSG(TRACE_ZLL3, "test_timer_next_step", (FMT__0));

  ZB_GET_OUT_BUF_DELAYED(test_next_step);
}

void test_next_step(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_uint8_t status;

  TRACE_MSG(TRACE_ZLL3, "> test_next_step param %hd step %hd", (FMT__H, param, g_test_step));

  switch (g_test_step )
  {
  case TEST_STEP_INITIAL:
    TRACE_MSG(TRACE_ZLL3, "Start commissioning", (FMT__0));
    g_test_step = TEST_STEP_START;
    status = zb_zll_start_commissioning(param);
    if (status != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Could not initiate commissioning: status %hd", (FMT__H, status));
      ++g_error_cnt;
    }
    break;

    // TODO Insert client activity on each test step
    // and switch g_test_step to next value

  //case TEST_STEP_START:
  //  TRACE_MSG(TRACE_ZLL3, "TEST_STEP_SEND_ON", (FMT__0));
  //  g_test_step = TEST_STEP_SEND_ON;
  //  ZB_ZCL_ON_OFF_SEND_ON_REQ(buffer, DST_ADDR, DST_ADDR_MODE, DST_ENDPOINT,
  //      ENDPOINT, ZB_AF_ZLL_PROFILE_ID, ZB_FALSE, NULL);
  //  break;

  //case TEST_STEP_SEND_ON_TIMED_OFF:
  //  g_test_step = TEST_STEP_FINISHED;
  //  break;

  default:
    g_test_step = TEST_STEP_FINISHED;
    TRACE_MSG(TRACE_ERROR, "ERROR step %hd shan't be processed", (FMT__H, g_test_step));
    ++g_error_cnt;
    break;
  }

  current_seq_number = ZCL_CTX().seq_number -1; //previous!

  if (g_test_step == TEST_STEP_FINISHED || g_error_cnt)
  {
    if (g_error_cnt)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR Test failed with %hd errors", (FMT__H, g_error_cnt));
    }
    else
    {
      TRACE_MSG(TRACE_INFO1, "Test finished. Status: OK", (FMT__0));
    }
    zb_free_buf(buffer);
  }

  TRACE_MSG(TRACE_ZLL3, "< test_next_step. Curr step %hd" , (FMT__H, g_test_step));
}/* void test_next_step(zb_uint8_t param) */

void read_attr_resp_handler(zb_buf_t * cmd_buf)
{
  zb_zcl_read_attr_res_t * read_attr_resp;

  TRACE_MSG(TRACE_ZCL1, ">> on_off_read_attr_resp_handler", (FMT__0));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(cmd_buf, read_attr_resp);
  TRACE_MSG(TRACE_ZCL3, "read_attr_resp %p", (FMT__P, read_attr_resp));
  if (read_attr_resp)
  {
    // TODO attributeID and test status oneration
    if (//(ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID != read_attr_resp->attr_id)  ||
        (ZB_ZCL_STATUS_SUCCESS != read_attr_resp->status))
    {
      TRACE_MSG(TRACE_ERROR,
                "ERROR incorrect response: id 0x%04x, status %h",
                (FMT__D_H, read_attr_resp->attr_id, read_attr_resp->status));
      g_error_cnt++;
    }

    switch (g_test_step)
    {
    // TODO test attribute value

    //case TEST_STEP_SEND_READ_ATTR:
    //  if (read_attr_resp->attr_value[0]!=0x01)
    //  {
    //    g_error_cnt++;
    //    TRACE_MSG(TRACE_ERROR, "ERROR on/off value %hx", (FMT__H, read_attr_resp->attr_value[0]));
    //  }
    //  break;

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

  TRACE_MSG(TRACE_ZCL1, "<< on_off_read_attr_resp_handler", (FMT__0));
}

#else // defined ZB_ENABLE_ZLL

#include <stdio.h>
int main()
{
  printf(" ZLL is not supported\n");
  return 0;
}

#endif // defined ZB_ENABLE_ZLL
