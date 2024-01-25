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
/* PURPOSE: Dimmable light for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41650
#include "zll/zb_zll_common.h"
#include "test_defs.h"

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_NON_COLOR_SCENE_CONTROLLER

#define ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP_2(ep_name, ep_id1, cluster_list1, ep_id2, cluster_list2)  \
      ZB_ZCL_DECLARE_DEVICE_SCENE_TABLE_RECORD_TYPE_ZLL(                            \
          zb_zll_non_color_scene_controller_scene_table_ ## ep_name ## _t,           \
          ZB_ZCL_SCENES_FIELDSETS_LENGTH_2(ZB_ZCL_CLUSTER_ID_ON_OFF,                \
                                           ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL));       \
                                                                                    \
      ZB_ZCL_DEFINE_DEVICE_SCENE_TABLE_ZLL(                                         \
          zb_zll_non_color_scene_controller_scene_table_ ## ep_name ## _t,           \
          g_zb_zll_non_color_scene_controller_scene_table_ ## ep_name);              \
      ZB_ZCL_DECLARE_NON_COLOR_SCENE_CONTROLLER_SIMPLE_DESC(                        \
          ep_name ## 1,                                                             \
          ep_id1,                                                                   \
          ZB_ZLL_NON_COLOR_SCENE_CONTROLLER_IN_CLUSTER_NUM,                         \
          ZB_ZLL_NON_COLOR_SCENE_CONTROLLER_OUT_CLUSTER_NUM);                       \
                                                                                    \
      ZB_ZCL_DECLARE_DEVICE_SCENE_TABLE_RECORD_TYPE_ZLL(                            \
          zb_zll_color_scene_controller_scene_table_ ## ep_name ## _t,               \
          ZB_ZCL_SCENES_FIELDSETS_LENGTH_3(ZB_ZCL_CLUSTER_ID_ON_OFF,                \
                                           ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,         \
                                           ZB_ZCL_CLUSTER_ID_COLOR_CONTROL));       \
                                                                                    \
      ZB_ZCL_DEFINE_DEVICE_SCENE_TABLE_ZLL(                                         \
          zb_zll_color_scene_controller_scene_table_ ## ep_name ## _t,               \
          g_zb_zll_color_scene_controller_scene_table_ ## ep_name);                  \
      ZB_ZCL_DECLARE_COLOR_SCENE_CONTROLLER_SIMPLE_DESC(                            \
          ep_name ## 2,                                                             \
          ep_id2,                                                                   \
          ZB_ZLL_COLOR_SCENE_CONTROLLER_IN_CLUSTER_NUM,                             \
          ZB_ZLL_COLOR_SCENE_CONTROLLER_OUT_CLUSTER_NUM);                           \
                                                                                    \
      ZB_AF_START_DECLARE_ENDPOINT_LIST(ep_name)                                    \
        ZB_AF_SET_ENDPOINT_DESC(                                                    \
            ep_id1,                                                                 \
            ZB_AF_ZLL_PROFILE_ID,                                                    \
            sizeof(zb_zll_non_color_scene_controller_scene_table_ ## ep_name ## _t), \
            &g_zb_zll_non_color_scene_controller_scene_table_ ## ep_name,            \
            ZB_ZCL_ARRAY_SIZE(                                                      \
                cluster_list1,                                                      \
                zb_zcl_cluster_desc_t),                                             \
            cluster_list1,                                                          \
            (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name##1),                   \
        ZB_AF_SET_ENDPOINT_DESC(                                                    \
            ep_id2,                                                                 \
            ZB_AF_ZLL_PROFILE_ID,                                                    \
            sizeof(zb_zll_color_scene_controller_scene_table_ ## ep_name ## _t),     \
            &g_zb_zll_color_scene_controller_scene_table_ ## ep_name,                \
            ZB_ZCL_ARRAY_SIZE(                                                      \
                cluster_list2,                                                      \
                zb_zcl_cluster_desc_t),                                             \
            cluster_list2,                                                          \
            (zb_af_simple_desc_1_1_t*)&simple_desc_##ep_name##2)                    \
      ZB_AF_FINISH_DECLARE_ENDPOINT_LIST




void zcl_identify_notification(zb_uint8_t param);
void zcl_identify_notification2(zb_uint8_t param);

void zll_task_state_changed(zb_uint8_t param);

void test_check_start_status(zb_uint8_t param);

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif /* ! defined ZB_ROUTER_ROLE */

/** @brief Endpoint the device operates at. */
#define ENDPOINT  5
#define ENDPOINT2  7

static zb_uint8_t g_error_cnt = 0;

zb_ieee_addr_t g_zc_addr = {1, 0, 0, 0, 0, 0, 0, 0};

/********************************* Declare attributes ****************/
//* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_ZLL(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);

/** [BASIC_DECLARE] */
/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_app_version = 0;
zb_uint8_t g_attr_stack_version = 0;
zb_uint8_t g_attr_hardware_version = 0;
zb_char_t g_attr_manufacturer_name[] = "\x15" "Test manufacture name";
zb_char_t g_attr_model_id[] = "\x03" "1.0";
zb_char_t g_attr_date_code[] = "\x0a" "20130509GP";
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;
zb_char_t g_attr_sw_build_id[] = "\x0f" "111.111.111.111";

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_ZLL(basic_attr_list, &g_attr_zcl_version, &g_attr_app_version,
    &g_attr_stack_version, &g_attr_hardware_version, &g_attr_manufacturer_name, &g_attr_model_id,
    &g_attr_date_code, &g_attr_power_source, &g_attr_sw_build_id);
/** [BASIC_DECLARE] */

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

/* Level cluster attribute data */

zb_uint8_t g_level_control_current_level = ZB_ZCL_LEVEL_CONTROL_CURRENT_LEVEL_DEFAULT_VALUE;
zb_uint16_t g_level_control_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST_ZLL(
    level_control_attr_list, &g_level_control_current_level, &g_level_control_remaining_time);

/********************* Declare device **************************/
ZB_ZLL_DECLARE_DIMMABLE_LIGHT_CLUSTER_LIST(
    dimmable_light_clusters,
    basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list, on_off_attr_list, level_control_attr_list,
    ZB_FALSE);

ZB_ZLL_DECLARE_DIMMABLE_PLUGIN_UNIT_CLUSTER_LIST(
    dimmable_plugin_unit_clusters,
    basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list, on_off_attr_list, level_control_attr_list,
    ZB_FALSE);

ZB_ZLL_DECLARE_DIMMABLE_LIGHT_EP(dimmable_light_ep, ENDPOINT, dimmable_light_clusters);

ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_EP_2(test_scene_controller_eps,
    ENDPOINT, dimmable_light_clusters,
    ENDPOINT2, dimmable_plugin_unit_clusters);

//ZB_ZLL_DECLARE_NON_COLOR_SCENE_CONTROLLER_CTX(
//    non_color_scene_controller_ctx, non_color_scene_controller_ep);
zb_af_device_ctx_t test_scene_controller_ctx = { 2, test_scene_controller_eps, 0, NULL, 0, NULL };

  zb_uint16_t g_dst_addr;
  zb_uint8_t g_addr_mode;
  zb_uint8_t g_endpoint;

MAIN()
{
  ARGV_UNUSED;

#ifndef KEIL
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zr");


  zb_set_default_ed_descriptor_values();

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&test_scene_controller_ctx);
  //ZB_AF_SET_ENDPOINT_HANDLER(HA_OUTPUT_ENDPOINT, zcl_specific_cluster_cmd_handler);
  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ENDPOINT, zcl_identify_notification);
  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ENDPOINT2, zcl_identify_notification2);

  ZB_ZLL_REGISTER_COMMISSIONING_CB(zll_task_state_changed);

  ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;

  ZB_SET_NIB_SECURITY_LEVEL(0);

  if (zb_zll_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR zb_zll_dev_start failed", (FMT__0));
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
  zb_zcl_parsed_hdr_t cmd_info;
  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

  ZB_ZCL_COPY_PARSED_HEADER(zcl_cmd_buf, &cmd_info);

  g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr;
  g_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint;
  g_addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;

  ZB_ZCL_DEBUG_DUMP_HEADER(&cmd_info);
  TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info.cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Unsupported \"from server\" command direction",
        (FMT__0));
  }

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ZB_FALSE;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  if (buf->u.hdr.status == 0 || buf->u.hdr.status == ZB_NWK_STATUS_ALREADY_PRESENT)
  {
    TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
  }
  zb_free_buf(buf);
}

void zcl_identify_notification(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZCL3, ">< zcl_identify_notification: %hd", (FMT__H, param));
}

void zcl_identify_notification2(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZCL3, ">< zcl_identify_notification2: %hd", (FMT__H, param));
}

void zll_task_state_changed(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> zll_task_state_changed param %hd", (FMT__H, param));

  if (task_status->task == ZB_ZLL_DEVICE_START_TASK)
  {
    test_check_start_status(param);
  }

  zb_free_buf(ZB_BUF_FROM_REF(param));

  TRACE_MSG(TRACE_ZLL3, "< zll_task_state_changed", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_check_start_status(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> test_check_start_status param %hd", (FMT__D, param));

  if (ZB_PIBCACHE_CURRENT_CHANNEL() != MY_CHANNEL)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR wrong channel %hd (should be %hd)",
        (FMT__H_H, ZB_PIBCACHE_CURRENT_CHANNEL(), (zb_uint8_t)MY_CHANNEL));
    ++g_error_cnt;
  }

  if (ZB_PIBCACHE_NETWORK_ADDRESS() != 0xffff)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR Network address is 0x%04x (should be 0xffff)",
        (FMT__D, ZB_PIBCACHE_NETWORK_ADDRESS()));
    ++g_error_cnt;
  }

  if (! ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
  {
    TRACE_MSG(TRACE_ERROR, "ERROR Receiver should be turned on", (FMT__0));
    ++g_error_cnt;
  }

  if (! ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()))
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR extended PAN Id is not zero: %s",
        (FMT__A, ZB_NIB_EXT_PAN_ID()));
    ++g_error_cnt;
  }

  if (ZB_PIBCACHE_PAN_ID() != 0xffff)
  {
    TRACE_MSG(
        TRACE_ERROR,
        "ERROR PAN Id is 0x%04x (should be 0xffff)",
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

#else /* defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_NON_COLOR_SCENE_CONTROLLER */

#include <stdio.h>

int main()
{
  printf("ZLL is not supported\n");
  return 0;
}

#endif /* defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_NON_COLOR_SCENE_CONTROLLER */
