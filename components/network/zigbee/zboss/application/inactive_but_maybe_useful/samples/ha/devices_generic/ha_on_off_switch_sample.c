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
/* PURPOSE: Simple switch for HA profile
*/

#define ZB_TRACE_FILE_ID 40163
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#ifdef ZB_USE_BUTTONS
#include "zb_led_button.h"

void send_toogle_req(zb_uint8_t param);
void button_press_handler(zb_uint8_t param);

/* Button press is detected by polling, so force wake up every
 * BUTTON_POLL_PERIOD  beacon intervals to not miss button press */
#define BUTTON_POLL_PERIOD        4
#endif /* ZB_USE_BUTTONS */

#define ZB_HA_DEFINE_DEVICE_ON_OFF_SWITCH
#include "zb_ha.h"

#if ! defined ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

/* Handler for specific zcl commands */
zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);

/* Parse read attributes response */
void on_off_read_attr_resp_handler(zb_buf_t *cmd_buf);

zb_bool_t cmd_in_progress = ZB_FALSE;


/** [COMMON_DECLARATION] */
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
#define HA_SWITCH_ENDPOINT          10

ZB_HA_DECLARE_ON_OFF_SWITCH_CLUSTER_LIST(on_off_switch_clusters, switch_cfg_attr_list, basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_ON_OFF_SWITCH_EP(on_off_switch_ep, HA_SWITCH_ENDPOINT, on_off_switch_clusters);

ZB_HA_DECLARE_ON_OFF_SWITCH_CTX(on_off_switch_ctx, on_off_switch_ep);
/** [COMMON_DECLARATION] */

static const zb_uint16_t DST_ADDR = 0x0000;
#define DST_ENDPOINT 5
#define DST_ADDR_MODE ZB_APS_ADDR_MODE_16_ENDP_PRESENT

/*! Identifier of the attribute to read */
zb_uint16_t g_attr2read = ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;


zb_ieee_addr_t g_ed_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("ha_on_off_switch_sample");

  ZB_AIB().aps_use_nvram = 0;

#ifdef ZB_USE_BUTTONS
  zb_osif_led_button_init();
#endif /* ZB_USE_BUTTONS */

  zb_set_long_address(g_ed_addr);
  zb_set_rx_on_when_idle(ZB_FALSE);
  zb_set_default_ed_descriptor_values();
  zb_set_network_ed_role(ZB_DEFAULT_APS_CHANNEL_MASK);

  /****************** Register Device ********************************/
  /** [REGISTER] */
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_switch_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(HA_SWITCH_ENDPOINT, zcl_specific_cluster_cmd_handler);
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


zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
/** [VARIABLE] */
  zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t unknown_cmd_received = ZB_TRUE;
/** [VARIABLE] */

  TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_ZCL3, "payload size: %i", (FMT__D, ZB_BUF_LEN(zcl_cmd_buf)));

/** [HANDLER] */
  if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI)
  {
    if (cmd_info->cmd_id == ZB_ZCL_CMD_DEFAULT_RESP)
    {
      unknown_cmd_received = ZB_FALSE;

      cmd_in_progress = ZB_FALSE;

      zb_free_buf(zcl_cmd_buf);
    }
  }
/** [HANDLER] */

  TRACE_MSG(TRACE_ZCL1, "< zcl_specific_cluster_cmd_handler", (FMT__0));
  return ! unknown_cmd_received;
}


void send_toogle_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  ZB_ASSERT(param);

  if (!cmd_in_progress)
  {
    cmd_in_progress = ZB_TRUE;

    ZB_ZCL_ON_OFF_SEND_TOGGLE_REQ(
      buf,
      DST_ADDR,
      DST_ADDR_MODE,
      DST_ENDPOINT,
      HA_SWITCH_ENDPOINT,
      ZB_AF_HA_PROFILE_ID,
      ZB_FALSE, NULL);
  }
  else
  {
    zb_free_buf(buf);
  }
}


#ifdef ZB_USE_BUTTONS
void button_press_handler(zb_uint8_t param)
{
  if (!param)
  {
    /* Button is pressed, get buffer for outgoing command */
    ZB_GET_OUT_BUF_DELAYED(button_press_handler);
  }
  else
  {
    send_toogle_req(param);
  }
}


/* Dummy function to wakeup our scheduler and not miss
 * button press */
void button_poll_magic(zb_uint8_t param)
{
  ZB_ASSERT(param == 0);

  ZB_SCHEDULE_APP_ALARM(button_poll_magic, 0, BUTTON_POLL_PERIOD);
}
#endif /* ZB_USE_BUTTONS */


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

    /* It is a good place to start custom automatic activities (if any) */
#ifdef ZB_USE_BUTTONS
    zb_button_register_handler(0, 0, button_press_handler);
    ZB_SCHEDULE_APP_ALARM(button_poll_magic, 0, BUTTON_POLL_PERIOD);
#endif /* ZB_USE_BUTTONS */

        /* ZB_SET_BUTTON_PRESS_CB(zb_sub_ghz_demo_button_press); */
        /* ZB_ENABLE_BUTTON(); */

        /* ZB_SCHEDULE_APP_ALARM(zb_sub_ghz_demo_toggle, param, ZB_SUB_GHZ_DEMO_TIME_BEFORE_FIRST_TOGGLE); */
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
