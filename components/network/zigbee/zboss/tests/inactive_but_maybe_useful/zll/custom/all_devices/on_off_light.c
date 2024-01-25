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
/* PURPOSE: On/Off light for ZLL profile, spec 5.2.1
*/

#define ZB_TRACE_FILE_ID 41660
#include "zll/zb_zll_common.h"

#define MY_CHANNEL 11

#if defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_ON_OFF_LIGHT

void zll_task_state_changed(zb_uint8_t param);

void test_device_cb(zb_uint8_t  param);

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr tests
#endif /* ! defined ZB_ROUTER_ROLE */

/** @brief Endpoint the device operates at. */
#define ENDPOINT  5

zb_ieee_addr_t g_zr_addr = {1, 0, 0, 0, 0, 0, 0, 0};

/** [COMMON_DECLARATION] */
/******************* Declare attributes ************************/

//* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_ZLL(on_off_attr_list, &g_attr_on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);

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

/********************* Declare device **************************/
ZB_ZLL_DECLARE_ON_OFF_LIGHT_CLUSTER_LIST(
    on_off_light_clusters,
    basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list, on_off_attr_list, ZB_FALSE);

ZB_ZLL_DECLARE_ON_OFF_LIGHT_EP(on_off_light_ep, ENDPOINT, on_off_light_clusters);

ZB_ZLL_DECLARE_ON_OFF_LIGHT_CTX(on_off_light_ctx, on_off_light_ep);
/** [COMMON_DECLARATION] */

MAIN()
{
  ARGV_UNUSED;

#if ! (defined KEIL || defined ZB_PLATFORM_LINUX_ARM_2400)

  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zr1");


  zb_set_default_ed_descriptor_values();

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zr_addr);

  /** [REGISTER] */
  /* Register device list */
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_light_ctx);

  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);
  /** [REGISTER] */

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

void zll_task_state_changed(zb_uint8_t param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zll_transaction_task_status_t* task_status =
      ZB_GET_BUF_PARAM(buffer, zb_zll_transaction_task_status_t);

  TRACE_MSG(TRACE_ZLL3, "> zll_task_state_changed param %hd", (FMT__H, param));

  if (task_status->task == ZB_ZLL_DEVICE_START_TASK)
  {
  }

  zb_free_buf(ZB_BUF_FROM_REF(param));

  TRACE_MSG(TRACE_ZLL3, "< zll_task_state_changed", (FMT__0));
}/* void zll_task_state_changed(zb_uint8_t param) */

void test_device_cb(zb_uint8_t  param)
{
  zb_buf_t* buffer = ZB_BUF_FROM_REF(param);
  zb_zcl_device_callback_param_t *cb_param = ZB_GET_BUF_PARAM(buffer, zb_zcl_device_callback_param_t);
  switch (cb_param->device_cb_id)
  {
    default:
      break;
  }
}

#else /* defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_ON_OFF_LIGHT */

#include <stdio.h>

int main()
{
  printf("ZLL is not supported\n");
  return 0;
}

#endif /* defined ZB_ENABLE_ZLL && defined ZB_ZLL_DEFINE_DEVICE_ON_OFF_LIGHT */
