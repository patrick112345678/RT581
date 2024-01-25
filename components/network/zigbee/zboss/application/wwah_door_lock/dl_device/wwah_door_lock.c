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
/* PURPOSE: ZB WWAH ZED device with RxOnWhenIdle
*/
#define ZB_TRACE_FILE_ID 40131
#include "zboss_api.h"
#include "zb_wwah_door_lock.h"
#include "zcl/zb_zcl_wwah.h"

static char dl_installcode[]= "966b9f3ef98ae605 9708";
static zb_ieee_addr_t g_dl_addr = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

door_lock_ctx_t g_dev_ctx;

/******************* Declare attributes ************************/
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

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
  &g_dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_OTA_UPGRADE_ATTRIB_LIST(ota_upgrade_attr_list,
  &g_dev_ctx.ota_attr.upgrade_server,
  &g_dev_ctx.ota_attr.file_offset,
  &g_dev_ctx.ota_attr.file_version,
  &g_dev_ctx.ota_attr.stack_version,
  &g_dev_ctx.ota_attr.downloaded_file_ver,
  &g_dev_ctx.ota_attr.downloaded_stack_ver,
  &g_dev_ctx.ota_attr.image_status,
  &g_dev_ctx.ota_attr.manufacturer,
  &g_dev_ctx.ota_attr.image_type,
  &g_dev_ctx.ota_attr.min_block_reque,
  &g_dev_ctx.ota_attr.image_stamp,
  &g_dev_ctx.ota_attr.server_addr,
  &g_dev_ctx.ota_attr.server_ep,
  DL_INIT_OTA_HW_VERSION,
  DL_OTA_IMAGE_BLOCK_DATA_SIZE_MAX,
  DL_OTA_UPGRADE_QUERY_TIMER_COUNTER);

/* Poll Control cluster attributes data */
ZB_ZCL_DECLARE_POLL_CONTROL_ATTRIB_LIST(
        poll_control_attr_list,
        &g_dev_ctx.poll_control_attrs.checkin_interval,
        &g_dev_ctx.poll_control_attrs.long_poll_interval,
        &g_dev_ctx.poll_control_attrs.short_poll_interval,
        &g_dev_ctx.poll_control_attrs.fast_poll_timeout,
        &g_dev_ctx.poll_control_attrs.checkin_interval_min,
        &g_dev_ctx.poll_control_attrs.long_poll_interval_min,
        &g_dev_ctx.poll_control_attrs.fast_poll_timeout_max);

ZB_ZCL_DECLARE_WWAH_ATTRIB_LIST(wwah_attr_list);

ZB_ZCL_START_DECLARE_ATTRIB_LIST(door_lock_attr_list)
ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID, (&g_dev_ctx.door_lock_attr.lock_state))
ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_DOOR_LOCK_LOCK_TYPE_ID, (&g_dev_ctx.door_lock_attr.lock_type))
ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_DOOR_LOCK_ACTUATOR_ENABLED_ID, (&g_dev_ctx.door_lock_attr.actuator_enabled))
ZB_ZCL_SET_ATTR_DESC(ZB_ZCL_ATTR_DOOR_LOCK_RF_OPERATION_EVENT_MASK_ID, (&g_dev_ctx.door_lock_attr.rf_operation_event_mask))
ZB_ZCL_FINISH_DECLARE_ATTRIB_LIST;

/********************* Declare device **************************/

ZB_HA_DECLARE_WWAH_DOOR_LOCK_CLUSTER_LIST(
        wwah_ha_clusters,
        basic_attr_list,
        wwah_attr_list,
        door_lock_attr_list,
        identify_attr_list,
        ota_upgrade_attr_list,
        poll_control_attr_list);


ZB_HA_DECLARE_WWAH_DOOR_LOCK_EP(wwah_ha_ep, WWAH_DOOR_LOCK_EP, wwah_ha_clusters);
ZBOSS_DECLARE_DEVICE_CTX_1_EP(device_ctx, wwah_ha_ep);

void dl_device_interface_cb(zb_uint8_t param);
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_OFF();

  ZB_INIT("wwah_door_lock");

  zb_zcl_wwah_set_wwah_behavior(ZB_ZCL_WWAH_BEHAVIOR_SERVER);

  /****************** Register Device ********************************/
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(WWAH_DOOR_LOCK_EP, zcl_specific_cluster_cmd_handler);

  zb_set_long_address(g_dl_addr);

  zb_set_network_ed_role(DEV_CHANNEL_MASK);

  dl_hw_init(0);

  /****************** Register Device ********************************/

  zb_nvram_register_app1_read_cb(dl_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(dl_nvram_write_app_data, dl_get_nvram_data_size);

  /********** SP device configuration **********/
  ZB_ZCL_REGISTER_DEVICE_CB(dl_device_interface_cb);

  dl_app_ctx_init();

  /* Init HA attributes */
  dl_clusters_attr_init(1);

  /* Act as non Sleepy End Device */
  zb_set_rx_on_when_idle(ZB_TRUE);

#ifdef ZB_SE_BDB_MIXED
  zb_se_set_bdb_mode_enabled(1);
#endif

  zb_zcl_wwah_init_server_attr();
  setup_debug_report();

#ifdef ZB_WWAH_DOOR_LOCK_DEFAULT_REPORTING
  /* Set reporting configuration */
  dl_init_default_reporting();
#endif

#ifdef ZB_WWAH_TESTING_WITH_TH_WITHOUT_ENCRYPTION_CONFIGURATION
  /* Use this define for TH ZC that does not automatically set up encryption
   * for Basic and other clusters */
  zb_zcl_set_cluster_encryption(WWAH_DOOR_LOCK_EP, ZB_ZCL_CLUSTER_ID_BASIC, 1);
  zb_zcl_set_cluster_encryption(WWAH_DOOR_LOCK_EP, ZB_ZCL_CLUSTER_ID_TIME, 1);
  zb_zcl_set_cluster_encryption(WWAH_DOOR_LOCK_EP, ZB_ZCL_CLUSTER_ID_POLL_CONTROL, 1);
#endif

  if (zboss_start_no_autostart() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start_no_autostart() failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/*********************  Device-specific functions  **************************/
zb_bool_t dl_read_time_attrs_handle(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_bool_t processed = ZB_FALSE;
  zb_zcl_read_attr_res_t *resp = NULL;

  zb_uint8_t time_status;
  zb_uint32_t nw_time = 0;
  zb_uint8_t fails = 0;

  TRACE_MSG(TRACE_ZCL1, ">> dl_read_time_attrs_handle param %hd", (FMT__H, param));

  ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);

  while (resp)
  {
    if (resp->status != ZB_ZCL_STATUS_SUCCESS)
    {
      TRACE_MSG(TRACE_ZCL1, "Attribute read error: attr 0x%x, status %d", (FMT__D_D, resp->attr_id, resp->status));
      ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);
      fails++;
      continue;
    }

    TRACE_MSG(TRACE_ZCL1, "read attr resp: src_addr 0x%x ep %hd",
              (FMT__D_D, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr, ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint));

    if (ZB_ZCL_ATTR_TIME_TIME_STATUS_ID == resp->attr_id)
    {
      time_status = resp->attr_value[0];
      TRACE_MSG(TRACE_ZCL1, "Time status attr value: 0x%hx", (FMT__H, time_status));
    }
    else if (ZB_ZCL_ATTR_TIME_TIME_ID == resp->attr_id)
    {
      nw_time = ZB_ZCL_ATTR_GET32(resp->attr_value);
      TRACE_MSG(TRACE_ZCL1, "Time attr value: %ld", (FMT__L, nw_time));
    }
    else
    {
      TRACE_MSG(TRACE_ZCL1, "Unknown attribute: attr 0x%x", (FMT__D, resp->attr_id));
      fails++;
    }

    ZB_ZCL_GENERAL_GET_NEXT_READ_ATTR_RES(param, resp);
  }


  if (0 == fails)
  {
    g_dev_ctx.last_obtained_time = nw_time;
    g_dev_ctx.obtained_at = zb_get_utc_time();
    processed = ZB_TRUE;
    zb_buf_free(param);
  }
  else
  {
    TRACE_MSG(TRACE_ZCL1, "Mallformed Read Time Status attribute response", (FMT__0));
  }

  TRACE_MSG(TRACE_ZCL1, "<< dl_read_time_attrs_handle", (FMT__0));

  return processed;
}


zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(param, zb_zcl_parsed_hdr_t);
  zb_uint8_t cmd_processed = ZB_FALSE;

  TRACE_MSG(TRACE_APP1, ">> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(param)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI &&
      cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_TIME &&
      cmd_info->cmd_id == ZB_ZCL_CMD_READ_ATTRIB_RESP)
  {
    cmd_processed = dl_read_time_attrs_handle(param);
  }

  TRACE_MSG(TRACE_APP1, "<< zcl_specific_cluster_cmd_handler processed %hd", (FMT__H, cmd_processed));
  return cmd_processed;
}


/** Application signal handler. Used for informing application about important ZBOSS
    events/states (device started first time/rebooted, key establishment completed etc).
    Application may ignore signals in which it is not interested.
*/
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch (sig)
    {
      case ZB_ZDO_SIGNAL_SKIP_STARTUP:
#ifndef ZB_MACSPLIT_HOST
        TRACE_MSG(TRACE_APP1, "ZB_ZDO_SIGNAL_SKIP_STARTUP: boot, not started yet", (FMT__0));
        /* Init default reporting info after NVRAM is loaded. */
        zb_secur_ic_str_set(dl_installcode);
        zboss_start_continue();
#endif /* ZB_MACSPLIT_HOST */
        break;
#ifdef ZB_MACSPLIT_HOST
      case ZB_MACSPLIT_DEVICE_BOOT:
        TRACE_MSG(TRACE_APP1, "ZB_MACSPLIT_DEVICE_BOOT: boot, not started yet", (FMT__0));
        /* Init default reporting info after NVRAM is loaded. */
        zb_secur_ic_str_set(dl_installcode);
        zboss_start_continue();
        break;
#endif /* ZB_MACSPLIT_HOST */

      case ZB_SIGNAL_DEVICE_FIRST_START:
      case ZB_SIGNAL_DEVICE_REBOOT:
      case ZB_BDB_SIGNAL_STEERING:
      {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK: first_start %hd",
                  (FMT__H, sig == ZB_SIGNAL_DEVICE_FIRST_START));
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
#ifdef ZB_REJOIN_BACKOFF
        zb_zdo_rejoin_backoff_cancel();
#endif
        /* Set Operational state in CommissionState attribute */
#ifdef ZB_USE_OSIF_OTA_ROUTINES
#ifndef ZB_WWAH_TESTING_DISABLE_OTA
        /* Some WWAH tests require device not to send ZCL packets during their steps,
         * so it can be needed to disable or delay OTA for them */
        zb_buf_get_in_delayed(dl_ota_start_upgrade);
#endif
#endif
        zb_zcl_wwah_init_server();
        setup_debug_report();
        zb_zcl_wwah_update_time(0);

        /* WWAH tests require ZED's ability to handle Network Update command and change PANID */
        zb_enable_panid_conflict_resolution(ZB_TRUE);

        /* NOTE: it is not needed to start Poll Controll process manually here
         * as it should be already started during ZCL periodic activities initialization */

        /* check, if joined, go to Normal (-restricted) mode */
        if (ZB_JOINED())
        {
          g_dev_ctx.dev_state = DL_DEV_NORMAL;
        }
      }
      break;
      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      {
        /* application data should be in the buf */
        if (zb_buf_len(param) > sizeof(zb_zdo_app_signal_hdr_t))
        {
          dl_production_config_t *prod_cfg = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, dl_production_config_t);
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
          if (prod_cfg->version >= 1)
          {
            ZB_ZCL_SET_STRING_VAL(g_dev_ctx.basic_attr.mf_name, prod_cfg->manuf_name, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->manuf_name));
            ZB_ZCL_SET_STRING_VAL(g_dev_ctx.basic_attr.model_id, prod_cfg->model_id, ZB_ZCL_STRING_CONST_SIZE(prod_cfg->model_id));
            g_dev_ctx.ota_attr.manufacturer = prod_cfg->manuf_code;

            zb_set_node_descriptor_manufacturer_code_req(prod_cfg->manuf_code, NULL);
          }
        }

        break;
      }

      default:
        TRACE_MSG(TRACE_APP1, "zboss_signal_handler: skip sig %hd status %hd",
                  (FMT__H_H, sig, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
  }
  else
  {
    switch (sig)
    {
      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        TRACE_MSG(TRACE_ERROR, "Production confing is not ready...", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

void dl_device_interface_cb(zb_uint8_t param)
{
  zb_zcl_attr_t *attr_desc;
  zb_uint16_t mask;
  zb_zcl_device_callback_param_t *device_cb_param =
          ZB_BUF_GET_PARAM(param, zb_zcl_device_callback_param_t);
  const zb_zcl_parsed_hdr_t *in_cmd_info = ZB_ZCL_DEVICE_CMD_PARAM_CMD_INFO(param);

  TRACE_MSG(TRACE_APP1, "> dl_device_interface_cb param %hd id %hd", (FMT__H_H,
          param, device_cb_param->device_cb_id));

  /* WARNING: Default status rewrited */
  device_cb_param->status = RET_OK;

  switch (device_cb_param->device_cb_id)
  {
#ifdef ZB_USE_OSIF_OTA_ROUTINES
    case ZB_ZCL_OTA_UPGRADE_VALUE_CB_ID:
      dl_process_ota_upgrade_cb(param);
      break;
#endif

    case ZB_ZCL_BASIC_RESET_CB_ID:
      dl_basic_reset_to_defaults_cb(0);
      break;

    case ZB_ZCL_WWAH_DEBUG_REPORT_QUERY_CB_ID:
      dl_process_wwah_debug_report_query_cb(param);
      break;
    case ZB_ZCL_DOOR_LOCK_LOCK_DOOR_CB_ID:
    {
      zb_uint8_t lock_state = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED;

      g_dev_ctx.door_lock_client_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr;
      g_dev_ctx.door_lock_client_endpoint =  ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).src_endpoint;

      ZVUNUSED(zb_zcl_set_attr_val(WWAH_DOOR_LOCK_EP,
                                   ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
                                   &lock_state,
                                   ZB_FALSE));
      attr_desc = zb_zcl_get_attr_desc_a(WWAH_DOOR_LOCK_EP,
                                         ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                                         ZB_ZCL_ATTR_DOOR_LOCK_RF_OPERATION_EVENT_MASK_ID);
      ZB_ASSERT(attr_desc);
      mask = ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc);
      if ((zb_bool_t)(mask & ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_LOCK))
      {
        zb_buf_get_out_delayed_ext(dl_send_notification, ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_LOCK, 0);
      }
    }
      break;
    case ZB_ZCL_DOOR_LOCK_UNLOCK_DOOR_CB_ID:
    {
      zb_uint8_t lock_state = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_UNLOCKED;

      g_dev_ctx.door_lock_client_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).source.u.short_addr;
      g_dev_ctx.door_lock_client_endpoint =  ZB_ZCL_PARSED_HDR_SHORT_DATA(in_cmd_info).src_endpoint;

      ZVUNUSED(zb_zcl_set_attr_val(WWAH_DOOR_LOCK_EP,
                                   ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                   ZB_ZCL_CLUSTER_SERVER_ROLE,
                                   ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_ID,
                                   &lock_state,
                                   ZB_FALSE));
      attr_desc = zb_zcl_get_attr_desc_a(WWAH_DOOR_LOCK_EP,
                                         ZB_ZCL_CLUSTER_ID_DOOR_LOCK,
                                         ZB_ZCL_CLUSTER_SERVER_ROLE,
                                         ZB_ZCL_ATTR_DOOR_LOCK_RF_OPERATION_EVENT_MASK_ID);
      ZB_ASSERT(attr_desc);
      mask = ZB_ZCL_GET_ATTRIBUTE_VAL_16(attr_desc);
      if ((zb_bool_t)(mask & ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_UNLOCK))
      {
        zb_buf_get_out_delayed_ext(dl_send_notification, ZB_ZCL_DOOR_LOCK_RF_OPERATION_EVENT_MASK_UNLOCK, 0);
      }

    }
      break;
    case ZB_ZCL_WWAH_ENABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
    {
      const zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t *pl_in =
              ZB_ZCL_DEVICE_CMD_PARAM_IN_GET(param, zb_zcl_wwah_enable_wwah_app_event_retry_algorithm_t);

      dl_enable_event_retry(pl_in);
      break;
    }
    case ZB_ZCL_WWAH_DISABLE_APP_EVENT_RETRY_ALGORITHM_CB_ID:
    {
      dl_disable_event_retry();
      break;
    }
    default:
      device_cb_param->status = RET_ERROR;
      break;
  }

  TRACE_MSG(TRACE_APP1, "< dl_device_interface_cb %hd", (FMT__H, device_cb_param->status));
}
