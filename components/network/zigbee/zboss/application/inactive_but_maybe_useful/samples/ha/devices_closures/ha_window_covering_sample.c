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
/* PURPOSE: Window covering for HA profile
*/

#define ZB_TRACE_FILE_ID 40172
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#define ZB_HA_DEFINE_DEVICE_WINDOW_COVERING
#include "zb_ha.h"

#if defined ZB_ENABLE_HA

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif


#define HA_WINDOW_COVERING_ENDPOINT          12

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/** [COMMON_DECLARATION] */
/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = 0;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

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

/* Window covering attribute variables  */
zb_uint8_t g_attr_window_covering_type = ZB_ZCL_WINDOW_COVERING_WINDOW_COVERING_TYPE_DEFAULT_VALUE;
zb_uint8_t g_attr_config_status = ZB_ZCL_WINDOW_COVERING_CONFIG_STATUS_DEFAULT_VALUE;
zb_uint8_t g_attr_current_position_lift_percentage =
  ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_LIFT_PERCENTAGE_DEFAULT_VALUE;
zb_uint8_t g_attr_current_position_tilt_percentage =
  ZB_ZCL_WINDOW_COVERING_CURRENT_POSITION_TILT_PERCENTAGE_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_open_limit_lift =
  ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_closed_limit_lift =
  ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_LIFT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_open_limit_tilt =
  ZB_ZCL_WINDOW_COVERING_INSTALLED_OPEN_LIMIT_TILT_DEFAULT_VALUE;
zb_uint16_t g_attr_installed_closed_limit_tilt =
  ZB_ZCL_WINDOW_COVERING_INSTALLED_CLOSED_LIMIT_TILT_DEFAULT_VALUE;
zb_uint8_t g_attr_mode = ZB_ZCL_WINDOW_COVERING_MODE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_WINDOW_COVERING_CLUSTER_ATTRIB_LIST(window_covering_attr_list,
                                                   &g_attr_window_covering_type,
                                                   &g_attr_config_status,
                                                   &g_attr_current_position_lift_percentage,
                                                   &g_attr_current_position_tilt_percentage,
                                                   &g_attr_installed_open_limit_lift,
                                                   &g_attr_installed_closed_limit_lift,
                                                   &g_attr_installed_open_limit_tilt,
                                                   &g_attr_installed_closed_limit_tilt,
                                                   &g_attr_mode);

/********************* Declare device **************************/

ZB_HA_DECLARE_WINDOW_COVERING_CLUSTER_LIST(window_covering_clusters,
                                           basic_attr_list,
                                           identify_attr_list,
                                           groups_attr_list,
                                           scenes_attr_list,
                                           window_covering_attr_list);

ZB_HA_DECLARE_WINDOW_COVERING_EP(
  window_covering_ep, HA_WINDOW_COVERING_ENDPOINT, window_covering_clusters);

ZB_HA_DECLARE_WINDOW_COVERING_CTX(window_covering_ctx, window_covering_ep);
/** [COMMON_DECLARATION] */

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
  ZB_INIT("window_covering_th");

  ZB_SET_NIB_SECURITY_LEVEL(0);

  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;

  zb_set_default_ed_descriptor_values();

  /****************** Register Device ********************************/
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&window_covering_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_WINDOW_COVERING_ENDPOINT, zcl_specific_cluster_cmd_handler);
  /** [REGISTER] */

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

/** [VARIABLE] */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t cmd_processed = ZB_FALSE;
  /** [VARIABLE] */

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
     TRACE_MSG(TRACE_ZCL1,
               "CLNT role cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
  }
  else
  {
    /** [HANDLER] */
    /* Command from client to server ZB_ZCL_FRAME_DIRECTION_TO_SRV */
    switch (cmd_info->cluster_id)
    {
      case ZB_ZCL_CLUSTER_ID_WINDOW_COVERING:
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
            case ZB_ZCL_CMD_WINDOW_COVERING_UP_OPEN:
              TRACE_MSG(TRACE_ZCL3, "Got cluster command 0x%04x", (FMT__D, cmd_info->cluster_id));
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
        TRACE_MSG(TRACE_ZCL1,
                  "SRV role, cluster 0x%d is not supported", (FMT__D, cmd_info->cluster_id));
        break;
    }
    /** [HANDLER] */
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
