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
/* PURPOSE: HA Output device sample
*/

#define ZB_TRACE_FILE_ID 40162
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#ifdef ZB_SUB_GHZ_DEMO
#include "zb_cortex_sub_ghz_leds.h"

void zb_sub_ghz_demo_on_off_leds(zb_bool_t on_leds);
#endif

#define ZB_HA_DEFINE_DEVICE_ON_OFF_OUTPUT
#include "zb_ha.h"


#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

/* Handler for ZCL commands */
zb_uint8_t zcl_custom_cmd_handler(zb_uint8_t param);

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

/* Basic cluster attributes data */
#define HA_OUTPUT_ENDPOINT          5

/** [COMMON_DECLARATION] */
/********************* Declare attributes  **************************/

/* On/Off cluster attributes data */
zb_uint8_t g_attr_on_off  = ZB_FALSE;

ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &g_attr_on_off);

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

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

/********************* Declare device **************************/
ZB_HA_DECLARE_ON_OFF_OUTPUT_CLUSTER_LIST(on_off_output_clusters,
    on_off_attr_list, basic_attr_list, identify_attr_list, groups_attr_list,
    scenes_attr_list);
ZB_HA_DECLARE_ON_OFF_OUTPUT_EP(on_off_output_ep, HA_OUTPUT_ENDPOINT, on_off_output_clusters);

ZB_HA_DECLARE_ON_OFF_OUTPUT_CTX(on_off_output_ctx, on_off_output_ep);
/** [COMMON_DECLARATION] */

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("ha_on_off_output_sample_subghz");

  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);
  zb_set_long_address(g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;
  zb_set_network_coordinator_role(ZB_DEFAULT_APS_CHANNEL_MASK);

  /* Register device list */
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_output_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_OUTPUT_ENDPOINT, zcl_custom_cmd_handler);
  /** [REGISTER] */

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

/* Processes (actually reports as unsupported) OnOff.Toggle command. All other
 * commands left for default processing.
 */
/** [VARIABLE] */
zb_uint8_t zcl_custom_cmd_handler(zb_uint8_t param)
{
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t * cmd_info = ZB_GET_BUF_PARAM(
      zcl_cmd_buf,
      zb_zcl_parsed_hdr_t);
  /* Store some values - cmd_info will be overwritten */
  zb_uint8_t processed = ZB_FALSE;
  zb_uint16_t dst_addr = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).dst_addr;
  zb_uint8_t endpoint = ZB_ZCL_PARSED_HDR_SHORT_DATA(cmd_info).src_endpoint;
  zb_uint8_t seq_num = cmd_info->seq_number;
  /** [VARIABLE] */

  TRACE_MSG(TRACE_ZCL1, "> zcl_custom_cmd_handler", (FMT__0));

  if (  (cmd_info->cmd_direction  == ZB_ZCL_FRAME_DIRECTION_TO_SRV) &&
        (cmd_info->cluster_id     == ZB_ZCL_CLUSTER_ID_ON_OFF)   )
  {
#ifdef ZB_SUB_GHZ_DEMO
    switch (cmd_info->cmd_id)
    {
      case ZB_ZCL_CMD_ON_OFF_ON_ID:
        zb_sub_ghz_demo_on_off_leds(ZB_TRUE);
        break;
      case ZB_ZCL_CMD_ON_OFF_OFF_ID:
        zb_sub_ghz_demo_on_off_leds(ZB_FALSE);
        break;
      case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
        /* Check the attr value and toggle LED */
        if (g_attr_on_off)
        {
          zb_sub_ghz_demo_on_off_leds(ZB_FALSE);
        }
        else
        {
          zb_sub_ghz_demo_on_off_leds(ZB_TRUE);
        }
        break;
      default: break;
    }
    processed = ZB_FALSE;
#else
    if (cmd_info->cmd_id  ==  ZB_ZCL_CMD_ON_OFF_TOGGLE_ID)
    {
      /* Unsupported command example: send default response and mark command as
      * procesed */
      /** [Send default response] */
      ZB_ZCL_SEND_DEFAULT_RESP(
        zcl_cmd_buf,
        dst_addr,
        ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
        endpoint,
        HA_OUTPUT_ENDPOINT,
        ZB_AF_HA_PROFILE_ID,
        ZB_ZCL_CLUSTER_ID_ON_OFF,
        seq_num,
        ZB_ZCL_CMD_ON_OFF_TOGGLE_ID,
        ZB_ZCL_STATUS_UNSUP_CLUST_CMD);
      /** [Send default response] */
      processed = ZB_TRUE;
    }
#endif
  }

  TRACE_MSG(TRACE_ZCL1, "< zcl_custom_cmd_handler %hd", (FMT__H, processed));
  return processed;
}


#ifdef ZB_SUB_GHZ_DEMO
void zb_sub_ghz_demo_on_off_leds(zb_bool_t on_leds)
{
  if (on_leds)
  {
    zb_cortexm4_set_led(LED1);
#ifdef STM32F4_DISCOVERY
        zb_cortexm4_set_led(LED2);
        zb_cortexm4_set_led(LED3);
        zb_cortexm4_set_led(LED4);
#endif
  }
  else
  {
    zb_cortexm4_clear_led(LED1);
#ifdef STM32F4_DISCOVERY
        zb_cortexm4_clear_led(LED2);
        zb_cortexm4_clear_led(LED3);
        zb_cortexm4_clear_led(LED4);
#endif
  }
}
#endif


void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, ">>zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
      break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal %hd", (FMT__H, sig));
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
    ZB_FREE_BUF(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_APP1, "<<zboss_signal_handler", (FMT__0));
}
