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
/* PURPOSE: Shade sample for HA profile
*/

#define ZB_TRACE_FILE_ID 40171
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#define ZB_HA_DEFINE_DEVICE_SHADE
#include "zb_ha.h"

#if defined ZB_ENABLE_HA

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile ze tests
#endif


/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);

/* Groups cluster attributes data */
zb_uint8_t g_attr_name_support = 0;

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);

/* Scenes cluster attribute data */
zb_uint8_t g_attr_scenes_scene_count;
zb_uint8_t g_attr_scenes_current_scene;
zb_uint16_t g_attr_scenes_current_group;
zb_uint8_t g_attr_scenes_scene_valid;
zb_uint8_t g_attr_scenes_name_support;

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list, &g_attr_scenes_scene_count,
                                  &g_attr_scenes_current_scene, &g_attr_scenes_current_group,
                                  &g_attr_scenes_scene_valid, &g_attr_scenes_name_support);

/* Level control cluster attribute data */
zb_uint8_t attr_current_level = 0;
zb_uint16_t attr_remaining_time = ZB_ZCL_LEVEL_CONTROL_REMAINING_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_LEVEL_CONTROL_ATTRIB_LIST(level_control_attr_list, &attr_current_level, &attr_remaining_time);

/* On/Off cluster attribute data */
zb_uint8_t attr_on_off = 0;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);

/* Shade configuration cluster attribute data */
zb_uint8_t attr_status = ZB_ZCL_SHADE_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint16_t attr_closed_limit = ZB_ZCL_SHADE_CONFIG_CLOSED_LIMIT_DEFAULT_VALUE;
zb_uint8_t attr_mode = ZB_ZCL_SHADE_CONFIG_MODE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_SHADE_CONFIG_ATTRIB_LIST(shade_config_attr_list, &attr_status, &attr_closed_limit, &attr_mode);

/********************* Declare device **************************/
#define HA_SHADE_ENDPOINT          11

ZB_HA_DECLARE_SHADE_CLUSTER_LIST(shade_clusters, basic_attr_list, identify_attr_list, groups_attr_list, scenes_attr_list, shade_config_attr_list, on_off_attr_list, level_control_attr_list);

ZB_HA_DECLARE_SHADE_EP(shade_ep, HA_SHADE_ENDPOINT, shade_clusters);

ZB_HA_DECLARE_SHADE_CTX(shade_ctx, shade_ep);

#define DST_ADDR 0
#define DST_ENDPOINT 5
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT


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
#ifndef ZB8051
  ZB_INIT("ha_dut");

  ZB_SET_NIB_SECURITY_LEVEL(0);

  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&shade_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_SHADE_ENDPOINT, zcl_specific_cluster_cmd_handler);

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

zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t unknown_cmd_received = ZB_TRUE;

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
  {
    TRACE_MSG(TRACE_ERROR, "Unsupported direction \"to server\"", (FMT__0));
    unknown_cmd_received = ZB_TRUE;
  }
  else
  {
    /* Command from server to client */
    switch (cmd_info->cluster_id)
    {
    case ZB_ZCL_CLUSTER_ID_SHADE_CONFIG:
      if (cmd_info->is_common_command)
      {
        switch (cmd_info->cmd_id)
        {
          case ZB_ZCL_CMD_DEFAULT_RESP:
            TRACE_MSG(
                TRACE_ZCL3,
                "Got response in cluster 0x%04x",
                ( FMT__D, cmd_info->cluster_id));
            /* Process default response */
            unknown_cmd_received = ZB_FALSE;
            break;
          case ZB_ZCL_CMD_READ_ATTRIB_RESP:
            /* Process read attributes response */
            unknown_cmd_received = ZB_FALSE;
            break;
          default:
            TRACE_MSG(TRACE_ERROR, "Unsupported general command", (FMT__0));
            unknown_cmd_received = ZB_TRUE;
            break;
        }
      }
      else
      {
        TRACE_MSG(
            TRACE_ERROR,
            "Cluster-specific commands not supported for OnOff cluster",
            (FMT__0));
      }
      break;
    default:
      TRACE_MSG(
          TRACE_ERROR,
          "Cluster 0x%hx is not supported (yet?)",
          (FMT__H, cmd_info->cluster_id));
      break;
    }
  }

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler %i", (FMT__0));
  return ! unknown_cmd_received;
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %h", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
    /* It is a good place to start custom automatic activities (if any) */
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
