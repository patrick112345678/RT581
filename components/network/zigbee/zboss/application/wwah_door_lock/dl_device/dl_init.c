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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 40133

#include "zboss_api.h"
#include "zb_wwah_door_lock.h"


void dl_app_ctx_init(void)
{
  TRACE_MSG(TRACE_APP1, ">> dl_app_ctx_init", (FMT__0));

  ZB_BZERO(&g_dev_ctx, sizeof(g_dev_ctx));

  TRACE_MSG(TRACE_APP1, "<< dl_app_ctx_init", (FMT__0));
}

void dl_clusters_attr_init(zb_uint8_t first_init)
{
  zb_ieee_addr_t default_ota_server_addr = DL_OTA_UPGRADE_SERVER;

  TRACE_MSG(TRACE_APP1, ">> dl_clusters_attr_init first_init %hd", (FMT__H, first_init));

  if (first_init)
  {
    /* Basic cluster attributes data */
    g_dev_ctx.basic_attr.zcl_version  = ZB_ZCL_VERSION;
    g_dev_ctx.basic_attr.app_version = DL_INIT_BASIC_APP_VERSION;
    g_dev_ctx.basic_attr.stack_version = DL_INIT_BASIC_STACK_VERSION;
    g_dev_ctx.basic_attr.hw_version = DL_INIT_BASIC_HW_VERSION;

    ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.mf_name,
            DL_INIT_BASIC_MANUF_NAME,
            ZB_ZCL_STRING_CONST_SIZE(DL_INIT_BASIC_MANUF_NAME));

    ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.model_id,
            DL_INIT_BASIC_MODEL_ID,
            ZB_ZCL_STRING_CONST_SIZE(DL_INIT_BASIC_MODEL_ID));

    ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.date_code,
            DL_INIT_BASIC_DATE_CODE,
            ZB_ZCL_STRING_CONST_SIZE(DL_INIT_BASIC_DATE_CODE));


    g_dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_MAINS_SINGLE_PHASE;

    ZB_ZCL_SET_STRING_VAL(
            g_dev_ctx.basic_attr.location_id,
            DL_INIT_BASIC_LOCATION_ID,
            ZB_ZCL_STRING_CONST_SIZE(DL_INIT_BASIC_LOCATION_ID));


    g_dev_ctx.basic_attr.ph_env = DL_INIT_BASIC_PH_ENV;
    g_dev_ctx.ota_attr.manufacturer = DL_INIT_OTA_MANUFACTURER;

    ZB_ZCL_SET_STRING_LENGTH(g_dev_ctx.basic_attr.sw_build_id, 0);

  }

  /* Identify cluster attributes data */
  g_dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

  /* OTA Upgrade client cluster attributes data */
  ZB_IEEE_ADDR_COPY(g_dev_ctx.ota_attr.upgrade_server, default_ota_server_addr);
  g_dev_ctx.ota_attr.file_offset = ZB_ZCL_OTA_UPGRADE_FILE_OFFSET_DEF_VALUE;
  g_dev_ctx.ota_attr.file_version = DL_INIT_OTA_FILE_VERSION;
  g_dev_ctx.ota_attr.stack_version = ZB_ZCL_OTA_UPGRADE_STACK_VERSION_DEF_VALUE;
  g_dev_ctx.ota_attr.downloaded_file_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_FILE_VERSION_DEF_VALUE;
  g_dev_ctx.ota_attr.downloaded_stack_ver = ZB_ZCL_OTA_UPGRADE_DOWNLOADED_STACK_DEF_VALUE;
  g_dev_ctx.ota_attr.image_status = ZB_ZCL_OTA_UPGRADE_IMAGE_STATUS_DEF_VALUE;
  g_dev_ctx.ota_attr.image_type = DL_INIT_OTA_IMAGE_TYPE;
  g_dev_ctx.ota_attr.min_block_reque = DL_INIT_OTA_MIN_BLOCK_REQUE;
  g_dev_ctx.ota_attr.image_stamp = DL_INIT_OTA_IMAGE_STAMP;
  g_dev_ctx.ota_attr.server_ep = ZB_ZCL_OTA_UPGRADE_SERVER_ENDPOINT_DEF_VALUE;
  g_dev_ctx.ota_attr.server_addr = ZB_ZCL_OTA_UPGRADE_SERVER_ADDR_DEF_VALUE;
  zb_zcl_wwah_init_server_attr();

  g_dev_ctx.poll_control_attrs.checkin_interval       = 0x28;   // (40 quarterseconds)
  g_dev_ctx.poll_control_attrs.long_poll_interval     = 0x14;   // (20 quarterseconds)
  g_dev_ctx.poll_control_attrs.short_poll_interval    = 0x02;   // (2 quarterseconds)
  g_dev_ctx.poll_control_attrs.fast_poll_timeout      = 0x28;   // (40 quarterseconds)
  g_dev_ctx.poll_control_attrs.checkin_interval_min   = 0x0080; // (128 quarterseconds)
  g_dev_ctx.poll_control_attrs.long_poll_interval_min = 0x0C;   // (12 quarterseconds)
  g_dev_ctx.poll_control_attrs.fast_poll_timeout_max  = 0x00F0; // (240 quarterseconds)

  /* Door Lock cluster attributes data */
  g_dev_ctx.door_lock_attr.lock_state = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_DEFAULT_VALUE;
  g_dev_ctx.door_lock_attr.lock_type = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_TYPE_DEFAULT_VALUE;
  g_dev_ctx.door_lock_attr.actuator_enabled = ZB_ZCL_ATTR_DOOR_LOCK_ACTUATOR_ENABLED_DEFAULT_VALUE;
  g_dev_ctx.door_lock_attr.rf_operation_event_mask = ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_DEFAULT_VALUE;
  g_dev_ctx.last_obtained_time = ZB_ZCL_TIME_TIME_DEFAULT_VALUE;
  g_dev_ctx.obtained_at = zb_get_utc_time();

  if (!first_init)
  {
    zb_bufid_t canceled_param = zb_zcl_poll_control_stop();
    zb_zcl_poll_control_start(canceled_param, WWAH_DOOR_LOCK_EP);
  }

  TRACE_MSG(TRACE_APP1, "<< dl_clusters_attr_init", (FMT__0));
}

void dl_basic_reset_to_defaults_cb(zb_uint8_t param)
{

  (void)param;
  TRACE_MSG(TRACE_APP1, ">> dl_basic_reset_to_defaults_cb", (FMT__0));

  dl_clusters_attr_init(0);
  setup_debug_report();

  zb_zcl_wwah_update_time(0);
  zb_zcl_wwah_stop_periodic_checkin();

  dl_write_app_data(0);
#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_ZCL_REPORTING_DATA);
#endif

  TRACE_MSG(TRACE_APP1, "<< dl_basic_reset_to_defaults", (FMT__0));
}

zb_ret_t dl_hw_init(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP1, ">> dl_hw_init", (FMT__0));

  ret = dl_hal_init();

  if (ret != RET_BUSY)
  {
    zb_set_nvram_erase_at_start( dl_get_button_state(DL_BUTTON_PIN) );
  }

  return ret;
}

#ifdef ZB_WWAH_DOOR_LOCK_DEFAULT_REPORTING
void dl_init_default_reporting(zb_uint8_t param)
{
  zb_zcl_reporting_info_t rep_info;
  ZB_BZERO(&rep_info, sizeof(rep_info));

  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZCL1, ">> dl_init_default_reporting", (FMT__0));
  {
    rep_info.direction = ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT;
    rep_info.ep = WWAH_DOOR_LOCK_EP;
    rep_info.cluster_id = ZB_ZCL_CLUSTER_ID_DOOR_LOCK;
    rep_info.cluster_role = ZB_ZCL_CLUSTER_SERVER_ROLE;
    rep_info.attr_id = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID;
    rep_info.dst.profile_id = ZB_AF_HA_PROFILE_ID;

    rep_info.u.send_info.min_interval = 0;
    rep_info.u.send_info.max_interval = 0;
    rep_info.u.send_info.delta.u32 = 0; /* not an analog data type*/
  }
  zb_zcl_put_reporting_info(&rep_info, ZB_TRUE /* override */);

  TRACE_MSG(TRACE_ZCL1, "<< dl_init_default_reporting", (FMT__0));
}
#endif
