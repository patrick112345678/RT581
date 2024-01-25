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
/*     PURPOSE: Simple GW application
*/

#define ZB_TRACE_FILE_ID 45615

#include "ncp/zb_ncp_ll_dev.h"
#include "ncp_hl_proto.h"
#include "zb_ncp_ota.h"
#include "app.h"
#include "simple_gw_device.h"

#if !defined(ZB_NCP_TRANSPORT_TYPE_SPI)
/* #error This app is to be compiled with NCP transport */
#endif

#if !defined(ZB_COORDINATOR_ROLE)
#error define ZB_COORDINATOR_ROLE to compile
#endif

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);
void send_toggle_req(zb_uint8_t param, zb_uint16_t dev_idx);
void send_toggle_req_delayed(zb_uint8_t dev_idx);
void simple_gw_remove_and_rejoin_device_delayed(zb_uint8_t idx);
void simple_gw_remove_device_delayed(zb_uint8_t idx);
#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number);
#endif
static void ncp_report_successful_loading(void);
static void ncp_fw_start(zb_uint8_t unused);

zb_ieee_addr_t g_zc_addr = SIMPLE_GW_IEEE_ADDR;
zb_uint8_t g_active_cmds = 0;


/******************* Declare attributes ************************/

/* Switch config cluster attributes data */
zb_uint8_t attr_switch_type =
  ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_TYPE_TOGGLE;
zb_uint8_t attr_switch_actions =
  ZB_ZCL_ON_OFF_SWITCH_CONFIGURATION_SWITCH_ACTIONS_DEFAULT_VALUE;

ZB_ZCL_DECLARE_ON_OFF_SWITCH_CONFIGURATION_ATTRIB_LIST(switch_cfg_attr_list, &attr_switch_type, &attr_switch_actions);

/* Basic cluster attributes data */
zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
ZB_HA_DECLARE_SIMPLE_GW_CLUSTER_LIST(simple_gw_clusters, switch_cfg_attr_list, basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_SIMPLE_GW_EP(simple_gw_ep, SIMPLE_GW_ENDPOINT, simple_gw_clusters);

ZB_HA_DECLARE_SIMPLE_GW_CTX(simple_gw_ctx, simple_gw_ep);


simple_gw_device_ctx_t g_device_ctx;

static const zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};


MAIN()
{
  ARGV_UNUSED;

  ZB_MEMSET(&g_device_ctx, 0, sizeof(g_device_ctx));

  /* Uncomment to change trace level and mask. */
  ZB_SET_TRACE_LEVEL(4);
  ZB_SET_TRACE_MASK(0x1800);
  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("simple_gw_zc");

  ncp_report_successful_loading();

  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(SIMPLE_GW_CHANNEL_MASK);
  zb_set_nvram_erase_at_start(ZB_TRUE); //!
  zb_set_max_children(SIMPLE_GW_DEV_NUMBER);

  /* Optional step: Setup predefined nwk key - to easily decrypt ZB sniffer logs which does not
   * contain keys exchange. By default nwk key is randomly generated. */
  zb_secur_setup_nwk_key((zb_uint8_t *) g_key_nwk, 0);

  /* Register NVRAM application callbacks - to be able to store application data to the special
     dataset. */
#ifdef ZB_USE_NVRAM
  zb_nvram_register_app1_read_cb(simple_gw_nvram_read_app_data);
  zb_nvram_register_app1_write_cb(simple_gw_nvram_write_app_data, simple_gw_get_nvram_data_size);
#endif

  /* Register device ctx */
  ZB_AF_REGISTER_DEVICE_CTX(&simple_gw_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(SIMPLE_GW_ENDPOINT, zcl_specific_cluster_cmd_handler);

#ifdef ZB_ASSERT_SEND_NWK_REPORT
  zb_register_zboss_callback(ZB_ASSERT_INDICATION_CB, SET_ZBOSS_CB(assert_indication_cb));
#endif

  /*Start ZBOSS main loop. Step 8*/

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* Callback which will be called when associated a device. */
zb_uint16_t simple_gw_associate_cb(zb_uint16_t short_addr)
{
  zb_uint8_t dev_idx = simple_gw_get_dev_index_by_short_addr(short_addr);

  TRACE_MSG(TRACE_APP1, ">> simple_gw_associate_cb short_addr 0x%x", (FMT__D, short_addr));

  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    TRACE_MSG(TRACE_APP1, "It is known device, but it attempts to associate, strange...", (FMT__0));
    ZB_SCHEDULE_APP_ALARM(simple_gw_remove_and_rejoin_device_delayed, dev_idx, 5 * ZB_TIME_ONE_SECOND);
  }
  else
  {
    /* Ok, device is unknown, add to dev list. */
    dev_idx = simple_gw_get_dev_index_by_state(NO_DEVICE);
    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      TRACE_MSG(TRACE_APP1, "Somebody associated! Start searching onoff device.", (FMT__0));

      g_device_ctx.devices[dev_idx].short_addr = short_addr;

      /* Enable this define to allow processing only for devices whith IEEE Address
         0:0:0:1:x:x:x:x or 0:0:0:2:x:x:x:x. */
#ifdef SIMPLE_GW_IEEE_ADDR_FILTERING
      zb_ieee_addr_t ieee_addr;
      zb_ret_t ret = zb_address_ieee_by_short(short_addr, ieee_addr);

      if (!(ret == RET_OK && (((ieee_addr[4] == 1) || (ieee_addr[4] == 2)) && (ieee_addr[5] == 0)
                              && (ieee_addr[6] == 0) && (ieee_addr[7] == 0))))
      {
        ZB_SCHEDULE_APP_ALARM(simple_gw_remove_device_delayed, dev_idx, 10 * ZB_TIME_ONE_SECOND);
      }
      else
#endif
      {
        g_device_ctx.devices[dev_idx].dev_state = MATCH_DESC_DISCOVERY;

        ZB_SCHEDULE_APP_ALARM_CANCEL(find_onoff_device_tmo, dev_idx);
        ZB_SCHEDULE_APP_ALARM(find_onoff_device_tmo, dev_idx, 30 * ZB_TIME_ONE_SECOND);

        ZB_SCHEDULE_APP_ALARM(find_onoff_device_delayed, dev_idx, 5 * ZB_TIME_ONE_SECOND);
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< simple_gw_associate_cb", (FMT__0));
  return 0;
}

/* Callback which will be called on incoming Device Announce packet. */
void simple_gw_dev_annce_cb(zb_uint16_t short_addr)
{
  zb_uint8_t idx = simple_gw_get_dev_index_by_short_addr(short_addr);
  if (idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    if (g_device_ctx.devices[idx].dev_state == COMPLETED ||
        g_device_ctx.devices[idx].dev_state == COMPLETED_NO_TOGGLE)
    {
      /* If it is known device - restart communications with it. */
      TRACE_MSG(TRACE_APP1, "Restart communication with device 0x%x (idx %hd)", (FMT__D_H, short_addr, idx));
    }
  }
  else
  {
    /* Associated a new device */
    simple_gw_associate_cb(short_addr);
  }
}

/* Callback which will be called on incoming nwk Leave packet. */
void simple_gw_leave_indication(zb_ieee_addr_t dev_addr)
{
  zb_uint8_t dev_idx;

  TRACE_MSG(TRACE_APP1, "> simple_gw_leave_indication device_addr ieee" TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(dev_addr)));

  dev_idx = simple_gw_get_dev_index_by_ieee_addr(dev_addr);

  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    /* It is the device which we controlled before. Remove it from the dev list. */
    ZB_SCHEDULE_APP_ALARM_CANCEL(send_toggle_req_delayed, dev_idx);
    ZB_SCHEDULE_APP_ALARM_CANCEL(simple_gw_remove_device_delayed, dev_idx);
    ZB_SCHEDULE_APP_ALARM_CANCEL(find_onoff_device_tmo, dev_idx);
    g_device_ctx.devices[dev_idx].dev_state = NO_DEVICE;
#ifdef ZB_USE_NVRAM
    zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
  }

  TRACE_MSG(TRACE_APP1, "< simple_gw_leave_indication", (FMT__0));
}

/* Callback which will be called on incoming ZCL packet. */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_uint8_t cmd_processed = 0;
  zb_uint16_t dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).source.u.short_addr;
  zb_uint8_t dst_ep = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;

  TRACE_MSG(TRACE_APP1, "> zcl_specific_cluster_cmd_handler", (FMT__0));

  /* Uncomment to use destination address and destination endpoint of the incoming ZCL packet. */
  /* g_dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).source.u.short_addr; */
  /* g_endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(&cmd_info).src_endpoint; */

  TRACE_MSG(TRACE_APP3, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

  if( cmd_info -> cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV )
  {
    switch( cmd_info -> cluster_id )
    {
      case ZB_ZCL_CLUSTER_ID_BASIC:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_DEFAULT_RESP:
              TRACE_MSG(TRACE_APP2, "Got default response in cluster 0x%04x",
                        ( FMT__D, cmd_info->cluster_id));
              break;

            default:
              TRACE_MSG(TRACE_APP2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        else
        {
          switch( cmd_info -> cmd_id )
          {
            default:
              TRACE_MSG(TRACE_APP2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        break;
      default:
        TRACE_MSG(TRACE_APP2, "Cluster %d command %hd, skip it", (FMT__D_H, cmd_info->cluster_id, cmd_info->cmd_id));
        break;
    }
  }
  else if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    switch( cmd_info -> cluster_id )
    {
      case ZB_ZCL_CLUSTER_ID_BASIC:
        if( cmd_info -> is_common_command )
        {
          switch( cmd_info -> cmd_id )
          {
            case ZB_ZCL_CMD_REPORT_ATTRIB:
              TRACE_MSG(TRACE_APP2, "Got reporting from cluster 0x%04x", (FMT__D, cmd_info->cluster_id));
              break;

            default:
              TRACE_MSG(TRACE_APP2, "Skip general command %hd", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        else
        {
          switch( cmd_info -> cmd_id )
          {
            default:
              TRACE_MSG(TRACE_APP2, "Cluster command %hd, skip it", (FMT__H, cmd_info->cmd_id));
              break;
          }
        }
        break;

      case ZB_ZCL_CLUSTER_ID_IAS_ZONE:
        if ( !(cmd_info -> is_common_command) )
        {
          switch( cmd_info -> cmd_id )
          {
            /* Answer to Zone Enroll Request - just to enable incoming notifications from IAS Zone sensor. */
            case ZB_ZCL_CMD_IAS_ZONE_ZONE_ENROLL_REQUEST_ID:
              ZB_ZCL_IAS_ZONE_SEND_ZONE_ENROLL_RES(zcl_cmd_buf, dst_addr,
                                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT, dst_ep,
                                                   SIMPLE_GW_ENDPOINT, ZB_AF_HA_PROFILE_ID, ZB_FALSE, NULL,
                                                   ZB_ZCL_IAS_ZONE_ENROLL_RESPONCE_CODE_SUCCESS, 7 /* ZONE_ID */);
              cmd_processed = 1;
              break;

            default:
              break;
          }
        }
        break;

      default:
        TRACE_MSG(TRACE_APP2, "Cluster %d command %hd, skip it", (FMT__D_H, cmd_info->cluster_id, cmd_info->cmd_id));
        break;
    }
  }

  TRACE_MSG(TRACE_APP1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
  return cmd_processed;
}

void find_onoff_device_delayed(zb_uint8_t idx)
{
  zb_buf_get_out_delayed_ext(find_onoff_device, idx, 0);
}

void find_onoff_device(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_APP1, ">> find_onoff_device %hd", (FMT__H, param));

  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

    req->nwk_addr = g_device_ctx.devices[dev_idx].short_addr;
    req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    req->profile_id = ZB_AF_HA_PROFILE_ID;
    /* We are searching for On/Off Server */
    req->num_in_clusters = 1;
    req->num_out_clusters = 0;
    req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_ON_OFF;

    zb_zdo_match_desc_req(param, find_onoff_device_cb);
  }
  else
  {
    TRACE_MSG(TRACE_APP2, "No devices in match descriptor discovery state were found!", (FMT__0));
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_APP1, "<< find_onoff_device %hd", (FMT__H, param));
}

void find_onoff_device_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(buf);
  zb_uint8_t *match_ep = NULL;
  zb_apsde_data_indication_t *ind = NULL;
  zb_uint8_t dev_idx = SIMPLE_GW_INVALID_DEV_INDEX;

  TRACE_MSG(TRACE_APP1, ">> find_onoff_device_cb param %hd, status %hd", (FMT__H_H, param, resp->status));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ind = ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t);
    dev_idx = simple_gw_get_dev_index_by_short_addr(ind->src_addr);

    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      if (resp->match_len)
      {
        ZB_SCHEDULE_APP_ALARM_CANCEL(find_onoff_device_tmo, dev_idx);

        /* Match EP list follows right after response header */
        match_ep = (zb_uint8_t*)(resp + 1);

        /* we are searching for exact cluster, so only 1 EP maybe found */
        g_device_ctx.devices[dev_idx].short_addr = ind->src_addr;
        g_device_ctx.devices[dev_idx].endpoint = *match_ep;
        g_device_ctx.devices[dev_idx].dev_state = IEEE_ADDR_DISCOVERY;

        TRACE_MSG(TRACE_APP2, "found dev addr 0x%x ep %hd dev_idx %hd",
                  (FMT__D_H_H, g_device_ctx.devices[dev_idx].short_addr, g_device_ctx.devices[dev_idx].endpoint, dev_idx));

        /* Next step is to resolve the IEEE address of the bulb */
        ZB_SCHEDULE_APP_CALLBACK2(device_ieee_addr_req, param, dev_idx);
        param = 0;
      }
      else
      {
        g_device_ctx.devices[dev_idx].short_addr = ind->src_addr;
        ZB_SCHEDULE_APP_ALARM_CANCEL(find_onoff_device_tmo, dev_idx);
#if defined IAS_CIE_ENABLED
        ZB_SCHEDULE_APP_ALARM_CANCEL(find_ias_zone_device_tmo, dev_idx);
        ZB_SCHEDULE_APP_ALARM(find_ias_zone_device_tmo, dev_idx, 30 * ZB_TIME_ONE_SECOND);

        ZB_SCHEDULE_APP_ALARM(find_ias_zone_device_delayed, dev_idx, 5 * ZB_TIME_ONE_SECOND);
#else  /* IAS_CIE_ENABLED */
        /* It is not onoff device, but lets keep it w/o any additional configuration. */
        g_device_ctx.devices[dev_idx].dev_state = COMPLETED_NO_TOGGLE;
#endif  /* IAS_CIE_ENABLED */
      }
    }
    else
    {
      TRACE_MSG(TRACE_APP2, "Device not found!", (FMT__0));
    }
  }

  if (param)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_APP1, "<< find_onoff_device_cb", (FMT__0));
}

void device_ieee_addr_req(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_bufid_t  buf = param;
  zb_zdo_ieee_addr_req_param_t *req_param;
  \
  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_ieee_addr_req_param_t);

    req_param->nwk_addr = g_device_ctx.devices[dev_idx].short_addr;
    req_param->dst_addr = req_param->nwk_addr;
    req_param->start_index = 0;
    req_param->request_type = 0;
    zb_zdo_ieee_addr_req(buf, device_ieee_addr_req_cb);
  }
  else
  {
    TRACE_MSG(TRACE_APP2, "No devives in discovery state were found!", (FMT__0));
    zb_buf_free(buf);
  }
}

void device_ieee_addr_req_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t nwk_addr;
  zb_uint8_t dev_idx;

  TRACE_MSG(TRACE_APP2, ">> device_ieee_addr_req_cb param %hd", (FMT__H, param));

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(buf);
  TRACE_MSG(TRACE_APP2, "resp status %hd, nwk addr %d", (FMT__H_D, resp->status, resp->nwk_addr));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_LETOH64(ieee_addr, resp->ieee_addr);
    ZB_LETOH16(&nwk_addr, &resp->nwk_addr);

    dev_idx = simple_gw_get_dev_index_by_short_addr(nwk_addr);

    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      ZB_MEMCPY(g_device_ctx.devices[dev_idx].ieee_addr, ieee_addr, sizeof(zb_ieee_addr_t));
      g_device_ctx.devices[dev_idx].dev_state = CONFIGURE_BINDING;

      /* The next step is to bind the Light control to the bulb */
      ZB_SCHEDULE_APP_CALLBACK2(bind_device, param, dev_idx);
      param = 0;
    }
    else
    {
      TRACE_MSG(TRACE_APP2, "This resp is not for our device", (FMT__0));
    }
  }

  if (param)
  {
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_APP2, "<< device_ieee_addr_req_cb", (FMT__0));
}

void bind_device(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_bufid_t buf = param;
  zb_zdo_bind_req_param_t *req;

  TRACE_MSG(TRACE_APP2, ">> bind_device param %hd", (FMT__H, param));

  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    req = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);
    ZB_MEMCPY(&req->src_address, g_device_ctx.devices[dev_idx].ieee_addr, sizeof(zb_ieee_addr_t));
    req->src_endp = g_device_ctx.devices[dev_idx].endpoint;
    req->cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
    req->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    ZB_MEMCPY(&req->dst_address.addr_long, (zb_uint8_t*)ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));
    req->dst_endp = SIMPLE_GW_ENDPOINT;
    req->req_dst_addr = g_device_ctx.devices[dev_idx].short_addr;

    zb_zdo_bind_req(param, bind_device_cb);
  }
  else
  {
    TRACE_MSG(TRACE_APP2, "No devives in binding configuration state were found!", (FMT__0));
    zb_buf_free(buf);
  }

  TRACE_MSG(TRACE_APP2, "<< bind_device", (FMT__0));
}

void bind_device_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_bind_resp_t *bind_resp = (zb_zdo_bind_resp_t*)zb_buf_begin(buf);

  if (bind_resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    zb_uint8_t dev_idx = simple_gw_get_dev_index_by_state(CONFIGURE_BINDING);

    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      g_device_ctx.devices[dev_idx].dev_state = CONFIGURE_REPORTING;

      ZB_SCHEDULE_APP_CALLBACK2(configure_reporting, param, dev_idx);
      param = 0;
    }
  }

  if (param)
  {
    zb_buf_free(buf);
  }
}

void configure_reporting(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_bufid_t buf = param;
  zb_uint8_t *cmd_ptr;

  TRACE_MSG(TRACE_APP2, ">> configure_reporting param %hd", (FMT__H, param));

  if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
  {
    ZB_ZCL_GENERAL_INIT_CONFIGURE_REPORTING_SRV_REQ(buf,
                                                    cmd_ptr,
                                                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

    ZB_ZCL_GENERAL_ADD_SEND_REPORT_CONFIGURE_REPORTING_REQ(
      cmd_ptr, ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, ZB_ZCL_ATTR_TYPE_BOOL, SIMPLE_GW_REPORTING_MIN_INTERVAL,
      SIMPLE_GW_REPORTING_MAX_INTERVAL(dev_idx), 0);

    ZB_ZCL_GENERAL_SEND_CONFIGURE_REPORTING_REQ(buf, cmd_ptr, g_device_ctx.devices[dev_idx].short_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                                g_device_ctx.devices[dev_idx].endpoint, SIMPLE_GW_ENDPOINT,
                                                ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_ON_OFF, configure_reporting_cb);
  }
  else
  {
    TRACE_MSG(TRACE_APP2, "No devives in reporting configuration state were found!", (FMT__0));
    zb_buf_free(buf);
  }
  TRACE_MSG(TRACE_APP2, "<< configure_reporting", (FMT__0));
}


void configure_reporting_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  /* Uncomment to use APSDE-DATA.confirm parameters. */
  /* zb_apsde_data_confirm_t *aps_data_conf = ZB_BUF_GET_PARAM(buf, zb_apsde_data_confirm_t); */

  TRACE_MSG(TRACE_APP2, ">> configure_reporting_cb param %hd status %hd", (FMT__H_H, param, zb_buf_get_status(param)));

  if (zb_buf_get_status(param) == (zb_uint8_t)RET_OK)
  {
    zb_uint8_t dev_idx = simple_gw_get_dev_index_by_state(CONFIGURE_REPORTING);

    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      g_device_ctx.devices[dev_idx].dev_state = COMPLETED;

#ifdef ZB_USE_NVRAM
      /* Save all application data to the NVRAM */
      zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif

      /* Start operating with this device */
      g_device_ctx.devices[dev_idx].pending_toggle = dev_idx + 1;
    }
  }

  zb_buf_free(buf);
  TRACE_MSG(TRACE_APP2, "<< configure_reporting_cb", (FMT__0));
}

void send_toggle_req_cb(zb_uint8_t param)
{
  zb_zcl_command_send_status_t *cmd_send_status = ZB_BUF_GET_PARAM(param, zb_zcl_command_send_status_t);
  zb_uint8_t idx = SIMPLE_GW_INVALID_DEV_INDEX;
  zb_uint16_t short_addr = ZB_NWK_BROADCAST_ALL_DEVICES;
  TRACE_MSG(TRACE_APP2, ">> send_toggle_req_cb param %hd status %hd dst_addr %d", (FMT__H_H_D, param, cmd_send_status->status, cmd_send_status->dst_addr));

  if (cmd_send_status->dst_addr.addr_type == ZB_ZCL_ADDR_TYPE_SHORT)
  {
    short_addr = cmd_send_status->dst_addr.u.short_addr;
  }
  else
  {
    short_addr = zb_address_short_by_ieee(cmd_send_status->dst_addr.u.ieee_addr);
  }

  idx = simple_gw_get_dev_index_by_short_addr(short_addr);

  if (cmd_send_status->status != RET_OK)
  {
    if (idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      TRACE_MSG(TRACE_APP2, "command status is not ok - stop communications with this device for 5 min", (FMT__0));
      g_device_ctx.devices[idx].pending_toggle = SIMPLE_GW_COMMUNICATION_PROBLEMS_TIMEOUT + idx + 1;
      --g_active_cmds;
    }
  }
  else
  {
    if ((idx != SIMPLE_GW_INVALID_DEV_INDEX) && (g_device_ctx.devices[idx].dev_state == COMPLETED))
    {
      --g_active_cmds;
    }
  }
  zb_buf_free(param);
  TRACE_MSG(TRACE_APP2, "<< send_toggle_req_cb g_active_cmds %hd", (FMT__H, g_active_cmds));
}

void send_toggle_req(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_bufid_t buf = param;

  ZB_ASSERT(param);

  TRACE_MSG(TRACE_APP2, ">> send_toggle_req param %hd idx %hd", (FMT__H_H, param, (zb_uint8_t)dev_idx));

  if (g_device_ctx.devices[dev_idx].dev_state == COMPLETED)
  {
    ++g_active_cmds;

    ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(
      buf,
      g_device_ctx.devices[dev_idx].short_addr,
      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
      g_device_ctx.devices[dev_idx].endpoint,
      SIMPLE_GW_ENDPOINT,
      ZB_AF_HA_PROFILE_ID,
      ZB_FALSE, send_toggle_req_cb);

    param = 0;
  }

  if (param)
  {
    zb_buf_free(buf);
  }
  TRACE_MSG(TRACE_APP2, "<< send_toggle_req g_active_cmds %hd", (FMT__H, g_active_cmds));
}

void send_toggle_req_delayed(zb_uint8_t dev_idx)
{
  zb_buf_get_out_delayed_ext(send_toggle_req, dev_idx, 0);
}

#ifdef ZB_USE_NVRAM
zb_uint16_t simple_gw_get_nvram_data_size()
{
  TRACE_MSG(TRACE_APP1, "simple_gw_get_nvram_data_size, ret %hd", (FMT__H, sizeof(simple_gw_device_nvram_dataset_t)));
  return sizeof(simple_gw_device_nvram_dataset_t);
}

void simple_gw_nvram_read_app_data(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  simple_gw_device_nvram_dataset_t ds;
  zb_ret_t ret;
  zb_uint8_t i;

  TRACE_MSG(TRACE_APP1, ">> simple_gw_nvram_read_app_data page %hd pos %d", (FMT__H_D, page, pos));

  ZB_ASSERT(payload_length == sizeof(ds));

  ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  if (ret == RET_OK)
  {
    ZB_MEMCPY(g_device_ctx.devices, &ds, sizeof(simple_gw_device_nvram_dataset_t));

    /* Reset timeouts for all bulbs */
    for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
    {
      if (g_device_ctx.devices[i].dev_state == COMPLETED)
      {
        g_device_ctx.devices[i].pending_toggle = i + 1;
      }
    }
  }

  TRACE_MSG(TRACE_APP1, "<< simple_gw_nvram_read_app_data ret %d", (FMT__D, ret));
}

zb_ret_t simple_gw_nvram_write_app_data(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  simple_gw_device_nvram_dataset_t ds;

  TRACE_MSG(TRACE_APP1, ">> simple_gw_nvram_write_app_data, page %hd, pos %d", (FMT__H_D, page, pos));

  ZB_MEMCPY(&ds, g_device_ctx.devices, sizeof(simple_gw_device_nvram_dataset_t));

  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APP1, "<< simple_gw_nvram_write_app_data, ret %d", (FMT__D, ret));

  return ret;
}

#endif  /* ZB_USE_NVRAM */

void find_onoff_device_tmo(zb_uint8_t dev_idx)
{
  TRACE_MSG(TRACE_APP1, "find_onoff_device_tmo: dev_idx %hd", (FMT__H, dev_idx));

  ZB_ASSERT(dev_idx < SIMPLE_GW_DEV_NUMBER);
  if ((g_device_ctx.devices[dev_idx].dev_state != NO_DEVICE) &&
      (g_device_ctx.devices[dev_idx].dev_state != COMPLETED))
  {
    ZB_SCHEDULE_APP_CALLBACK(simple_gw_remove_device_delayed, dev_idx);
  }
}

zb_uint8_t simple_gw_get_dev_index_by_state(zb_uint8_t dev_state)
{
  zb_uint8_t i;
  for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
  {
    if (g_device_ctx.devices[i].dev_state == dev_state)
    {
      return i;
    }
  }

  return SIMPLE_GW_INVALID_DEV_INDEX;
}

zb_uint8_t simple_gw_get_dev_index_by_short_addr(zb_uint16_t short_addr)
{
  zb_uint8_t i;
  for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
  {
    if (g_device_ctx.devices[i].short_addr == short_addr)
    {
      return i;
    }
  }

  return SIMPLE_GW_INVALID_DEV_INDEX;
}

zb_uint8_t simple_gw_get_dev_index_by_ieee_addr(zb_ieee_addr_t ieee_addr)
{
  zb_uint8_t i;
  for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
  {
    if (ZB_IEEE_ADDR_CMP(g_device_ctx.devices[i].ieee_addr, ieee_addr))
    {
      return i;
    }
  }

  return SIMPLE_GW_INVALID_DEV_INDEX;
}

void simple_gw_send_leave_req(zb_uint8_t param, zb_uint16_t short_addr, zb_bool_t rejoin_flag)
{
  zb_bufid_t buf = param;
  zb_zdo_mgmt_leave_param_t *req_param;
  zb_address_ieee_ref_t addr_ref;

  if (zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    req_param->dst_addr = short_addr;
    req_param->rejoin = (rejoin_flag ? 1 : 0);
    zdo_mgmt_leave_req(param, NULL);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "tried to remove 0x%xd, but device is already left", (FMT__D, short_addr));
    zb_buf_free(buf);
  }
}

void simple_gw_leave_device(zb_uint8_t param, zb_uint16_t short_addr)
{
  TRACE_MSG(TRACE_APP1, ">> simple_gw_leave_device param %hd short_addr %d", (FMT__H_D, param, short_addr));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(simple_gw_leave_device, short_addr, 0);
  }
  else
  {
    simple_gw_send_leave_req(param, short_addr, ZB_FALSE);
  }

  TRACE_MSG(TRACE_APP1, "<< simple_gw_leave_device", (FMT__0));
}

void simple_gw_leave_and_rejoin_device(zb_uint8_t param, zb_uint16_t short_addr)
{
  TRACE_MSG(TRACE_APP1, ">> simple_gw_leave_and_rejoin_device param %hd short_addr %d", (FMT__H_D, param, short_addr));

  if (!param)
  {
    zb_buf_get_out_delayed_ext(simple_gw_leave_and_rejoin_device, short_addr, 0);
  }
  else
  {
    simple_gw_send_leave_req(param, short_addr, ZB_TRUE);
  }

  TRACE_MSG(TRACE_APP1, "<< simple_gw_leave_and_rejoin_device", (FMT__0));
}

void simple_gw_remove_device(zb_uint8_t idx)
{
  ZB_SCHEDULE_APP_ALARM_CANCEL(send_toggle_req_delayed, idx);
  ZB_BZERO(&g_device_ctx.devices[idx], sizeof(simple_gw_device_params_t));
#ifdef ZB_USE_NVRAM
  zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#endif
}

void simple_gw_remove_and_rejoin_device_delayed(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_APP1, "simple_gw_remove_and_rejoin_device_delayed: short_addr 0x%x", (FMT__D, g_device_ctx.devices[idx].short_addr));

  zb_buf_get_out_delayed_ext(simple_gw_leave_and_rejoin_device, g_device_ctx.devices[idx].short_addr, 0);
  simple_gw_remove_device(idx);
}

void simple_gw_remove_device_delayed(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_APP1, "simple_gw_remove_device_delayed: short_addr 0x%x", (FMT__D, g_device_ctx.devices[idx].short_addr));

  zb_buf_get_out_delayed_ext(simple_gw_leave_device, g_device_ctx.devices[idx].short_addr, 0);
  simple_gw_remove_device(idx);
}

void toggle_bulbs(zb_uint8_t param)
{
  zb_uint8_t i;
  ZVUNUSED(param);

  for (i = 0; i < ZB_ARRAY_SIZE(g_device_ctx.devices); ++i)
  {
    if (g_device_ctx.devices[i].dev_state == COMPLETED)
    {
      if (g_device_ctx.devices[i].pending_toggle > 0)
      {
        --(g_device_ctx.devices[i].pending_toggle);
      }
      else
      {
        zb_uint16_t rand_seed = SIMPLE_GW_TOGGLE_TIMEOUT;
        TRACE_MSG(TRACE_APP1, "toggle_bulbs: schedule %hd bulb toggle after %d sec", (FMT__H_D, i, rand_seed));
        ZB_SCHEDULE_APP_ALARM(send_toggle_req_delayed, i, (rand_seed + 1) * ZB_TIME_ONE_SECOND);
        g_device_ctx.devices[i].pending_toggle = i + 1;
      }
    }
  }
  ZB_SCHEDULE_APP_ALARM(toggle_bulbs, 0, SIMPLE_GW_TOGGLE_ITER_TIMEOUT);
}

/* Callback which will be called on startup procedure complete (successfull or not). */
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
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        /* Turn off link key exchange if legacy device support (<ZB3.0) is neeeded */
        zb_bdb_set_legacy_device_support(1);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

        ZB_SCHEDULE_APP_CALLBACK(ncp_fw_start, 0);
        ZB_SCHEDULE_APP_CALLBACK(ncp_hl_send_reset_resp, ZB_GET_APP_SIGNAL_STATUS(param));

        ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_bulbs, ZB_ALARM_ANY_PARAM);
        ZB_SCHEDULE_APP_ALARM(toggle_bulbs, 0, ZB_TIME_ONE_SECOND * 5);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APP1, "Successfull steering", (FMT__0));
        break;

//! [signal_leave_ind]
      case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
      {
        zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
        if (!leave_ind_params->rejoin)
        {
          simple_gw_leave_indication(leave_ind_params->device_addr);
        }
      }
      break;
//! [signal_leave_ind]

//! [signal_device_annce]
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
        zb_zdo_signal_device_annce_params_t *dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);
        simple_gw_dev_annce_cb(dev_annce_params->device_short_addr);
      }
      break;
//! [signal_device_annce]

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
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

  if (param)
  {
    zb_buf_free(param);
  }
}

static void ncp_report_successful_loading(void)
{
  zb_uint32_t old_value = 0;
  zb_uint32_t reporting_value = NCP_OTA_IMAGE_IS_LOADED_SUCCESSFULLY;

  zb_osif_nvram_read_memory(ZBS_BL_META_ADDR, sizeof(old_value), (zb_uint8_t*)&old_value);

  if (old_value != NCP_OTA_IMAGE_IS_LOADED_SUCCESSFULLY)
  {
    zb_osif_nvram_write_memory(ZBS_BL_META_ADDR, sizeof(reporting_value), (zb_uint8_t *)&reporting_value);
  }
}

static void ncp_fw_start(zb_uint8_t unused)
{
  ncp_ll_packet_received_cb_t cb = ncp_hl_proto_init();
  (void)unused;
  ncp_ll_proto_init(cb);

  /* Normal ZBOSS start begin: load NVRAM and production config (if exists), then NWK reset and load PIB from NIB. */
  /* ZG->zdo.commissioning_type = ZB_COMMISSIONING_NCP; */

  /* zboss_start_continue(); */
}
