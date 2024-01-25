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
/* PURPOSE: Dimmable light sample (HA profile)
*/
//! [trace_file_id]
#define ZB_TRACE_FILE_ID 41673
#include "bulb.h"
//! [trace_file_id]
#ifdef LIGHT_SAMPLE_BUTTONS
#include "bulb_hal.h"
#endif
#include "zb_ha_bulb.h"

#include "zb_common.h"
#include "zb_zdo.h"
#include "zb_secur.h"

#include "../common/zb_trace_signal_params.h"

zb_ieee_addr_t g_zr_addr = BULB_IEEE_ADDRESS;

#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number);
#endif

#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to build led bulb demo
#endif

void zcl_reset_to_defaults_cb(zb_uint8_t param) ZB_CALLBACK;

#ifdef ZB_ZGP_SINK
void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                              zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);
#else
void report_attribute_cb(zb_uint16_t addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                              zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);
#endif /* ZB_ZGP_SINK */

/** [COMMON_DECLARATION] */

/******************* Declare attributes ************************/
bulb_device_ctx_t g_dev_ctx;

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
                                    &g_dev_ctx.identify_attr.identify_time);


/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attribute data */
zb_uint8_t g_attr_scenes_scene_count;
zb_uint8_t g_attr_scenes_current_scene;
zb_uint8_t g_attr_scenes_scene_valid;
zb_uint8_t g_attr_scenes_name_support;
zb_uint16_t g_attr_scenes_current_group;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
    &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
    &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

#define ZB_HA_DEFINE_DEVICE_DIMMABLE_LIGHT

/* Basic cluster attributes data */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
  basic_attr_list,
  &g_dev_ctx.basic_attr.zcl_version,
  &g_dev_ctx.basic_attr.app_version,
  &g_dev_ctx.basic_attr.stack_version,
  &g_dev_ctx.basic_attr.hw_version,
  &g_dev_ctx.basic_attr.mf_name,
  &g_dev_ctx.basic_attr.model_id,
  &g_dev_ctx.basic_attr.date_code,
  &g_dev_ctx.basic_attr.power_source,
  &g_dev_ctx.basic_attr.location_id,
  &g_dev_ctx.basic_attr.ph_env,
  &g_dev_ctx.basic_attr.sw_build_id);

#ifdef ZB_ENABLE_ZLL
/* On/Off cluster attributes additions data */
zb_bool_t g_attr_global_scene_ctrl  = ZB_TRUE;
zb_uint16_t g_attr_on_time  = 0;
zb_uint16_t g_attr_off_wait_time  = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST_EXT(on_off_attr_list, &g_dev_ctx.on_off_attr.on_off,
    &g_attr_global_scene_ctrl, &g_attr_on_time, &g_attr_off_wait_time);
#else
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_dev_ctx.on_off_attr.on_off);
#endif

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list,
                                         &g_dev_ctx.level_control_attr.current_level, &g_dev_ctx.level_control_attr.remaining_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_LIGHT_CLUSTER_LIST(
  dimmable_light_clusters, basic_attr_list,
  identify_attr_list,
  groups_attr_list,
  scenes_attr_list,
  on_off_attr_list,
  level_control_attr_list
);

ZB_HA_DECLARE_LIGHT_EP(
  dimmable_light_ep, HA_DIMMABLE_LIGHT_ENDPOINT,
  dimmable_light_clusters);

ZB_HA_DECLARE_LIGHT_CTX(
  dimmable_light_ctx,
  dimmable_light_ep);

MAIN()
{
  ARGV_UNUSED;

#ifdef LIGHT_SAMPLE_BUTTONS
  bulb_hal_init();
#endif

//! [switch_trace]
  ZB_SET_TRACE_ON();
//! [switch_trace]
  /* ZB_SET_TRAF_DUMP_ON(); */

  ZB_INIT("bulb");

  zb_set_long_address(g_zr_addr);

  zb_set_network_router_role_legacy(BULB_DEFAULT_APS_CHANNEL_MASK);
  zb_set_max_children(1);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

  bulb_device_app_init(0);

  /* Test get functions */

  zb_char_t *version = zb_get_version();
  TRACE_MSG(TRACE_ERROR, "ZB version is %s", (FMT__P, version));

  zb_ieee_addr_t addr;
  zb_get_long_address(addr);
  TRACE_MSG(TRACE_ERROR, "Long address is " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(addr)));

  zb_uint8_t rx_on_when_idle = zb_get_rx_on_when_idle();
  TRACE_MSG(TRACE_APP1, "rx on when idle = %hd",
            (FMT__H, rx_on_when_idle));

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "ERROR dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

void bulb_do_identify(zb_uint8_t param);

void bulb_device_app_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> bulb_device_app_init", (FMT__0));

  ZVUNUSED(param);

  /****************** Register Device ********************************/

  ZB_ZCL_REGISTER_DEVICE_CB(test_device_cb);
  ZB_AF_REGISTER_DEVICE_CTX(&dimmable_light_ctx);

  ZB_AF_SET_ENDPOINT_HANDLER(HA_DIMMABLE_LIGHT_ENDPOINT,
                             zcl_specific_cluster_cmd_handler);

  ZB_ZCL_SET_DEFAULT_VALUE_CB(zcl_reset_to_defaults_cb);
  ZB_ZCL_SET_REPORT_ATTR_CB((zb_zcl_report_attr_cb_t)report_attribute_cb);

  ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(HA_DIMMABLE_LIGHT_ENDPOINT, bulb_do_identify);

  bulb_app_ctx_init();

  /* Init HA attributes */
  bulb_clusters_attr_init(0);

#ifdef LIGHT_SAMPLE_BUTTONS
  bulb_hal_set_level(g_dev_ctx.level_control_attr.current_level);
#endif

#ifdef ZB_USE_NVRAM
  zb_nvram_register_app1_read_cb(bulb_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(bulb_nvram_write_app_data, bulb_get_nvram_data_size);
#endif

  TRACE_MSG(TRACE_APP1, "<< bulb_device_app_init", (FMT__0));
}

void bulb_clusters_attr_init(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, ">> bulb_clusters_attr_init", (FMT__0));
  ZVUNUSED(param);
  /* Basic cluster attributes data */
  g_dev_ctx.basic_attr.zcl_version  = ZB_ZCL_VERSION;
  g_dev_ctx.basic_attr.app_version = BULB_INIT_BASIC_APP_VERSION;
  g_dev_ctx.basic_attr.stack_version = BULB_INIT_BASIC_STACK_VERSION;
  g_dev_ctx.basic_attr.hw_version = BULB_INIT_BASIC_HW_VERSION;

  ZB_ZCL_SET_STRING_VAL(
    g_dev_ctx.basic_attr.mf_name,
    BULB_INIT_BASIC_MANUF_NAME,
    ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MANUF_NAME));

  ZB_ZCL_SET_STRING_VAL(
    g_dev_ctx.basic_attr.model_id,
    BULB_INIT_BASIC_MODEL_ID,
    ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_MODEL_ID));

  ZB_ZCL_SET_STRING_VAL(
    g_dev_ctx.basic_attr.date_code,
    BULB_INIT_BASIC_DATE_CODE,
    ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_DATE_CODE));


  g_dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE;

  ZB_ZCL_SET_STRING_VAL(
    g_dev_ctx.basic_attr.location_id,
    BULB_INIT_BASIC_LOCATION_ID,
    ZB_ZCL_STRING_CONST_SIZE(BULB_INIT_BASIC_LOCATION_ID));


  g_dev_ctx.basic_attr.ph_env = BULB_INIT_BASIC_PH_ENV;

  /* Identify cluster attributes data */
  g_dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

  /* On/Off cluster attributes data */
  g_dev_ctx.on_off_attr.on_off = (zb_bool_t)ZB_ZCL_ON_OFF_IS_ON;

  g_dev_ctx.level_control_attr.current_level = ZB_ZCL_LEVEL_CONTROL_LEVEL_MAX_VALUE;
  g_dev_ctx.level_control_attr.remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

  TRACE_MSG(TRACE_APP1, "<< bulb_clusters_attr_init", (FMT__0));
}

void bulb_app_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> bulb_app_ctx_init", (FMT__0));

  ZB_MEMSET(&g_dev_ctx, 0, sizeof(g_dev_ctx));

  TRACE_MSG(TRACE_APP1, "<< bulb_app_ctx_init", (FMT__0));
}

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

  return ZB_FALSE;
}

static void remove_child_zed_delayed2(zb_uint8_t param, zb_uint16_t param2)
{
  zb_ieee_addr_t g_ieee_addr_zed = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
  zb_bufid_t buf = param;
  zb_nlme_leave_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_leave_request_t);

  ZB_MEMCPY(req->device_address, g_ieee_addr_zed, sizeof(zb_ieee_addr_t));
  req->remove_children = ZB_FALSE;
  req->rejoin = (zb_uint8_t)param2;
  ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
}

static void remove_child_zed(zb_uint8_t param)
{
  zb_buf_get_out_delayed_ext(remove_child_zed_delayed2, (zb_uint16_t)param, 0);
}

static void rejoin_me_delayed(zb_uint8_t param, zb_uint16_t param2)
{
  zb_bufid_t buf = param;

  TRACE_MSG(TRACE_ERROR, ">>rejoin_me", (FMT__0));

  zdo_initiate_rejoin(buf,
                      ZB_NIB_EXT_PAN_ID(),
                      ZB_AIB().aps_channel_mask_list,
                      (zb_uint8_t)param2);

  TRACE_MSG(TRACE_ERROR, "<<rejoin_me", (FMT__0));
}

static void rejoin_me(zb_uint8_t param)
{
  zb_buf_get_out_delayed_ext(rejoin_me_delayed, (zb_uint16_t)param, 0);
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      {
        static zb_bool_t rejoined = ZB_FALSE;
        if (!rejoined)
        {
          rejoined = ZB_TRUE;
          ZB_SCHEDULE_ALARM(rejoin_me, ZB_TRUE,  20 * ZB_TIME_ONE_SECOND);
          ZB_SCHEDULE_ALARM(rejoin_me, ZB_FALSE, 30 * ZB_TIME_ONE_SECOND);
        }
#ifdef LIGHT_SAMPLE_BUTTONS
        bulb_hal_set_connect(ZB_TRUE);
#endif

        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
      }
      break;

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
      {
        static zb_uint8_t counter = 0;

        ++counter;
        if (1 == counter)
        {
          ZB_SCHEDULE_ALARM(remove_child_zed,
                            ZB_FALSE, /* rejoin flag */
                            20 * ZB_TIME_ONE_SECOND);
        }
        else if (3 == counter)
        {
          ZB_SCHEDULE_ALARM(remove_child_zed,
                            ZB_TRUE, /* rejoin flag */
                            20 * ZB_TIME_ONE_SECOND);
        }
      }
      break;
    }

    zb_trace_signal_params(sig, sg_p);
  }
  else
  {
#ifdef LIGHT_SAMPLE_BUTTONS
    bulb_hal_set_connect(ZB_FALSE);
#endif

    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

zb_ret_t level_control_set_level(zb_uint8_t new_level)
{
  TRACE_MSG(TRACE_APP1, "> level_control_set_level", (FMT__0));
  g_dev_ctx.level_control_attr.current_level = new_level;

#ifdef LIGHT_SAMPLE_BUTTONS
  bulb_hal_set_level(new_level);
#endif

  TRACE_MSG(TRACE_APP1, "New level is %i", (FMT__H, new_level));
  TRACE_MSG(TRACE_APP1, "< level_control_set_level", (FMT__0));

  return RET_OK;
}

void zcl_reset_to_defaults_cb(zb_uint8_t param) ZB_CALLBACK
{
  (void)param;
  TRACE_MSG(TRACE_APP1, ">> zcl_reset_to_defaults_cb", (FMT__0));

  /* Reset zcl attrs to default values */
  zb_zcl_init_reporting_info();
  zb_zcl_reset_reporting_ctx();
  /* Init default reporting again */

  zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
  zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);

  TRACE_MSG(TRACE_APP1, "<< zcl_reset_to_defaults_cb", (FMT__0));
}

#ifdef ZB_ZGP_SINK
void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                              zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
#else
void report_attribute_cb(zb_uint16_t addr, zb_uint8_t ep, zb_uint16_t cluster_id,
                              zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
#endif /* ZB_ZGP_SINK */
{
  ZVUNUSED(ep);
  TRACE_MSG(TRACE_APP1, ">> report_attribute_cb addr %d ep %hd, cluster 0x%x, attr %d",
      (FMT__D_H_D_D, addr, ep, cluster_id, attr_id));
  TRACE_MSG(TRACE_APP1, "<< report_attribute_cb", (FMT__0));
}

void test_device_cb(zb_uint8_t param)
{
  zb_bufid_t buffer = param;
  zb_zcl_device_callback_param_t *device_cb_param =
    ZB_BUF_GET_PARAM(buffer, zb_zcl_device_callback_param_t);
  TRACE_MSG(TRACE_APP1, "> test_device_cb param %hd id %hd",
            (FMT__H_H, param, device_cb_param->device_cb_id));

  device_cb_param->status = RET_OK;
  switch (device_cb_param->device_cb_id)
  {
    case ZB_ZCL_LEVEL_CONTROL_SET_VALUE_CB_ID:

      TRACE_MSG(TRACE_APP1, "Level control setting", (FMT__0));
      device_cb_param->status =
        level_control_set_level(device_cb_param->cb_param.level_control_set_value_param.new_value);

#ifdef ZB_USE_NVRAM
      zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif

      break;

    case ZB_ZCL_SET_ATTR_VALUE_CB_ID:
      TRACE_MSG(TRACE_APP1, "on/off setting to %hd", (FMT__H, device_cb_param->cb_param.set_attr_value_param.values.data8));

      if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF)
      {
        if (device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID)
        {
          g_dev_ctx.on_off_attr.on_off = (zb_bool_t)device_cb_param->cb_param.set_attr_value_param.values.data8;
        }
      }
      else if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
      {
        if (device_cb_param->cb_param.set_attr_value_param.attr_id == ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID)
        {
          g_dev_ctx.level_control_attr.current_level = device_cb_param->cb_param.set_attr_value_param.values.data16;
        }
      }
      else
      {
        /* Other clusters can be processed here */
      }

      if (device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF ||
          device_cb_param->cb_param.set_attr_value_param.cluster_id == ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL)
      {
        if (g_dev_ctx.on_off_attr.on_off)
        {
#ifdef LIGHT_SAMPLE_BUTTONS
          bulb_hal_set_on_off(ZB_TRUE);
          bulb_hal_set_level(g_dev_ctx.level_control_attr.current_level);
#endif
        }
        else
        {
#ifdef LIGHT_SAMPLE_BUTTONS
          bulb_hal_set_on_off(ZB_FALSE);
#endif
        }
#ifdef ZB_USE_NVRAM
        zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
      }

      break;

    default:
      device_cb_param->status = RET_ERROR;
     break;
  }
  TRACE_MSG(TRACE_APP1, "< test_device_cb %hd", (FMT__H, device_cb_param->status));
}

void bulb_identify_led(zb_uint8_t led_state)
{
  TRACE_MSG(TRACE_APP1, "bulb_identify_led %hd", (FMT__H, led_state));
#ifdef LIGHT_SAMPLE_BUTTONS
  bulb_hal_set_on_off((zb_bool_t)led_state);
  if (led_state)
  {
    bulb_hal_set_level(g_dev_ctx.level_control_attr.current_level);
  }
#endif
  ZB_SCHEDULE_ALARM(bulb_identify_led, !led_state, ZB_TIME_ONE_SECOND / 2);
}

void bulb_do_identify(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "> bulb_do_identify %hd", (FMT__H, param));

  ZB_SCHEDULE_ALARM_CANCEL(bulb_identify_led, ZB_ALARM_ANY_PARAM);

  if (param == 1)
  {
    /* start identifying */
    bulb_identify_led(!g_dev_ctx.on_off_attr.on_off);
  }
  else
  {
    /* stop identifying, return led to the previous mode */
#ifdef LIGHT_SAMPLE_BUTTONS
    bulb_hal_set_on_off(g_dev_ctx.on_off_attr.on_off);
    if (g_dev_ctx.on_off_attr.on_off)
    {
      bulb_hal_set_level(g_dev_ctx.level_control_attr.current_level);
    }
#endif
  }

  TRACE_MSG(TRACE_APP1, "< bulb_do_identify", (FMT__0));
}

#ifdef ZB_USE_NVRAM
zb_uint16_t bulb_get_nvram_data_size()
{
  TRACE_MSG(TRACE_APP1, "bulb_get_nvram_data_size, ret %hd", (FMT__H, sizeof(bulb_device_nvram_dataset_t)));
  return sizeof(bulb_device_nvram_dataset_t);
}

void bulb_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  bulb_device_nvram_dataset_t ds;
  zb_ret_t ret;
//! [trace_msg]
  TRACE_MSG(TRACE_APP1, ">> bulb_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));
//! [trace_msg]
  ZB_ASSERT(payload_length == sizeof(ds));

  ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    zb_zcl_attr_t *attr_desc;

    attr_desc = zb_zcl_get_attr_desc_a(HA_DIMMABLE_LIGHT_ENDPOINT,ZB_ZCL_CLUSTER_ID_ON_OFF,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

    ZB_ZCL_SET_DIRECTLY_ATTR_VAL8(attr_desc, ds.onoff_state);

    attr_desc = zb_zcl_get_attr_desc_a(HA_DIMMABLE_LIGHT_ENDPOINT, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

    ZB_ZCL_SET_DIRECTLY_ATTR_VAL16(attr_desc, ds.current_level);

    if (ds.onoff_state)
    {
#ifdef LIGHT_SAMPLE_BUTTONS
      bulb_hal_set_level(ds.current_level);
#endif
    }
    else
    {
#ifdef LIGHT_SAMPLE_BUTTONS
      bulb_hal_set_on_off(ZB_FALSE);
#endif
    }
  }

  TRACE_MSG(TRACE_APP1, "<< bulb_nvram_read_app_data ret %d", (FMT__D, ret));
}

zb_ret_t bulb_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  bulb_device_nvram_dataset_t ds;
  zb_zcl_attr_t *attr_desc = zb_zcl_get_attr_desc_a(HA_DIMMABLE_LIGHT_ENDPOINT,ZB_ZCL_CLUSTER_ID_ON_OFF,
                                                    ZB_ZCL_CLUSTER_SERVER_ROLE,ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID);

  TRACE_MSG(TRACE_APP1, ">> bulb_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ds.onoff_state = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  attr_desc = zb_zcl_get_attr_desc_a(HA_DIMMABLE_LIGHT_ENDPOINT, ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL,
                                     ZB_ZCL_CLUSTER_SERVER_ROLE, ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID);

  ds.current_level = ZB_ZCL_GET_ATTRIBUTE_VAL_8(attr_desc);

  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< bulb_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}
#endif /* ZB_USE_NVRAM */
