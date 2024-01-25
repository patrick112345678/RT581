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
/* PURPOSE: Simple coordinator for GP Combo device
*/

#define ZB_TRACE_FILE_ID 40153
#include "zboss_api.h"


#include "simple_combo_match.h"
#ifdef ZB_USE_BUTTONS
#include "zb_led_button.h"
#endif

#include "simple_combo_zcl.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif

zb_uint8_t g_key_nwk[16] = { 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0, 0, 0, 0, 0, 0, 0, 0};
zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xab};
#define ENDPOINT  10
#define RESTART_COMM_TIMEOUT 3 * ZB_TIME_ONE_SECOND

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_COMBO_CLUSTER_LIST( sample_clusters,
          basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_COMBO_EP(sample_ep, ENDPOINT, sample_clusters);

ZB_HA_DECLARE_COMBO_CTX(sample_ctx, sample_ep);

/******************* Declare server parameters *****************/

/******************* Declare test data & constants *************/

zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param);

void start_comm(zb_uint8_t param);
void stop_comm(zb_uint8_t param);

/* override */
zb_ret_t zb_zgp_convert_8bit_vector(zb_uint8_t vector_8bit_cmd_id,      /* press or release cmd */
                                    zb_uint8_t switch_type,             /* see zb_zgpd_switch_type_e */
                                    zb_uint8_t num_of_contacs,
                                    zb_uint8_t contact_status,
                                    zb_uint8_t *zgp_cmd_out)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_APP1, ">> zb_zgp_convert_8bit_vector", (FMT__0));

  if ((vector_8bit_cmd_id == ZB_GPDF_CMD_8BIT_VECTOR_PRESS) &&
      (num_of_contacs == 2) &&
      (switch_type == ZB_GPD_SWITCH_TYPE_BUTTON))
  {
    if (contact_status == 0b10)
    {
      *zgp_cmd_out = ZB_GPDF_CMD_ON;
      ret = RET_OK;
    }
    else if (contact_status == 0b01)
    {
      *zgp_cmd_out = ZB_GPDF_CMD_OFF;
      ret = RET_OK;
    }
#if 0
    if (contact_status == 0b10)
    {
      *zgp_cmd_out = ZB_GPDF_CMD_MOVE_UP;
      ret = RET_OK;
    }
    else if (contact_status == 0b01)
    {
      *zgp_cmd_out = ZB_GPDF_CMD_MOVE_DOWN;
      ret = RET_OK;
    }
#endif
  }

  TRACE_MSG(TRACE_APP1, "<< zb_zgp_convert_8bit_vector, ret %hd", (FMT__H, ret));

  return ret;
}

MAIN()
{
  ARGV_UNUSED;

  /* ZB_SET_TRACE_OFF(); */
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("zc_combo");
  TRACE_MSG(TRACE_ERROR, "simple_combo_zc starting", (FMT__0));

  //! [set_nwk_key]
  /* use well-known key to simplify decrypt in Wireshark */
  zb_secur_setup_nwk_key(g_key_nwk, 0);
  //! [set_nwk_key]
  /* setup match for ZGPD command to ZCl translation */
  ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

  /****************** Register HA Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);
  ZB_AF_SET_ENDPOINT_HANDLER(ENDPOINT, test_specific_cluster_cmd_handler);

  //! [set_skip_gpdf]
  #if defined ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  /* ZB_TRUE: Ignore direct GPDF recv: always work thru the proxy.
     ZB_FALSE: Process GPDF: work as combo (default). */
  zb_zgp_set_skip_gpdf(1);
  #endif
  //! [set_skip_gpdf]

  //! [set_secur_level]
  zb_zgps_set_security_level(ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
    ZB_ZGP_SEC_LEVEL_NO_SECURITY,
    ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
    ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC));
  //! [set_secur_level]

  //ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_GROUP);
  //ZB_MEMCPY(ZGP_GP_SHARED_SECURITY_KEY, g_group_key, ZB_CCM_KEY_SIZE);

  //! [set_comm_mode]
  zb_zgps_set_communication_mode(ZGP_COMMUNICATION_MODE_LIGHTWEIGHT_UNICAST);
  //! [set_comm_mode]

  zb_set_long_address(g_zc_addr);
  /*zb_set_network_coordinator_role(ZB_DEFAULT_APS_CHANNEL_MASK);*/
  zb_set_network_coordinator_role(1l<<21);
  zb_set_nvram_erase_at_start(ZB_TRUE);

#ifdef ZB_USE_BUTTONS
  /* Buttons. */
  /* Left button - start comm. mode */
  zb_button_register_handler(0, 0, start_comm);
  /* Right button - stop comm. mode */
  zb_button_register_handler(1, 0, stop_comm);
#endif
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


#ifdef ZB_USE_BUTTONS
/**
   Switch off led3 blinking with given period

   To be called via alarm.

   @param blink_arg - argument passed to @ref short_blink_by_led3
 */
void led3_blink_off(zb_uint8_t blink_arg)
{
  TRACE_MSG(TRACE_APP1, "led3_blink_off 0x%hx", (FMT__H, blink_arg));
#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(blink_arg);
#endif
}

/**
   Switch on led3 blinking with given period

   @param blink_arg - argument created by ZB_LED_ARG_CREATE macro: led# + blink period
 */
void short_blink_by_led3(zb_uint8_t blink_arg)
{
  TRACE_MSG(TRACE_APP1, "short_blink_by_led3 0x%hx", (FMT__H, blink_arg));
  ZB_SCHEDULE_APP_ALARM(led3_blink_off, blink_arg, ZB_MILLISECONDS_TO_BEACON_INTERVAL(4000));
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(blink_arg);
#endif
}
#endif


/**
   Application specific handler for incoming ZCL packets

   Sinse we are GPCB (Combo), ZBOSS passes there GPDF translated to ZCL
   commands.

   @param param - ZCL packet.
 */
zb_uint8_t test_specific_cluster_cmd_handler(zb_uint8_t param)
{
  zb_bufid_t zcl_cmd_buf = param;
  zb_zcl_parsed_hdr_t *cmd_info = ZB_BUF_GET_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
  zb_bool_t processed = ZB_FALSE;
  static int state = 0;

  TRACE_MSG(TRACE_APP1, "test_specific_cluster_cmd_handler %i", (FMT__H, param));
  TRACE_MSG(TRACE_APP1, "payload size: %i", (FMT__D, zb_buf_len(zcl_cmd_buf)));

  switch (cmd_info->cluster_id)
  {
    case ZB_ZCL_CLUSTER_ID_ON_OFF:
      TRACE_MSG(TRACE_APP1, "ON/OFF cluster", (FMT__0));
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        switch (cmd_info->cmd_id)
        {
          case ZB_ZCL_CMD_ON_OFF_ON_ID:
            TRACE_MSG(TRACE_APP1, "ON", (FMT__0));
#ifdef ZB_USE_BUTTONS
            zb_osif_led_on(0);
#endif
            state = 1;
            processed = ZB_TRUE;
            break;
          case ZB_ZCL_CMD_ON_OFF_OFF_ID:
            TRACE_MSG(TRACE_APP1, "OFF", (FMT__0));
#ifdef ZB_USE_BUTTONS
            zb_osif_led_off(0);
#endif
            processed = ZB_TRUE;
            state = 1;
            break;
          case ZB_ZCL_CMD_ON_OFF_TOGGLE_ID:
            TRACE_MSG(TRACE_APP1, "TOGGLE", (FMT__0));
            state = !state;
#ifdef ZB_USE_BUTTONS
            if (state)
            {
              zb_osif_led_on(0);
            }
            else
            {
              zb_osif_led_off(0);
            }
#endif
            processed = ZB_TRUE;
            break;
        }
      }
      break;
    case ZB_ZCL_CLUSTER_ID_SCENES:
      TRACE_MSG(TRACE_APP1, "SCENE cluster", (FMT__0));
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        if (cmd_info->cmd_id == ZB_ZCL_CMD_SCENES_RECALL_SCENE)
        {
          zb_zcl_scenes_recall_scene_req_t *req;
          req = (zb_zcl_scenes_recall_scene_req_t *)zb_buf_begin(zcl_cmd_buf);
          TRACE_MSG(TRACE_APP1, "SCENES_RECALL_SCENE scene_id %hd", (FMT__H, req->scene_id));
          if (req->scene_id <= 3)
          {
            zb_int_t i;
            for (i = 0 ; i < 2 ; ++i)
            {
              if (req->scene_id & (1 << i))
              {
#ifdef ZB_USE_BUTTONS
                zb_osif_led_on(1 + i);
#endif
              }
              else
              {
#ifdef ZB_USE_BUTTONS
                zb_osif_led_off(1 + i);
#endif
              }
            }
          }
        }
        else
        {
          TRACE_MSG(TRACE_APP1, "Scene cmd %hd", (FMT__H, cmd_info->cmd_id));
        }
        processed = ZB_TRUE;
      }
      break;
    case ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
      TRACE_MSG(TRACE_APP1, "LEVEL CONTROL cluster", (FMT__0));
      if (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_SRV)
      {
        TRACE_MSG(TRACE_APP1, "Level control cmd %hd, options %hd", (FMT__H_H, cmd_info->cmd_id, *((zb_uint8_t *)zb_buf_begin(zcl_cmd_buf))));
        processed = ZB_TRUE;
      }
      break;
    case ZB_ZCL_CLUSTER_ID_BASIC:
      TRACE_MSG(TRACE_APP1, "BASIC cluster", (FMT__0));
#ifdef ZB_USE_BUTTONS
      short_blink_by_led3(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_PER_SEC));
#endif
      break;
    case ZB_ZCL_CLUSTER_ID_POWER_CONFIG:
      TRACE_MSG(TRACE_APP1, "POWER CONFIG cluster", (FMT__0));
#ifdef ZB_USE_BUTTONS
      short_blink_by_led3(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case ZB_ZCL_CLUSTER_ID_IAS_ZONE:
      TRACE_MSG(TRACE_APP1, "IAS ZONE cluster", (FMT__0));
#ifdef ZB_USE_BUTTONS
      short_blink_by_led3(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_QUATER_SEC));
#endif
      break;
  }
  if (processed)
  {
    zb_buf_free(param);
  }

  return processed;
}


/**
   Callback for ZGP commissioning complete

   @param zgpd_id - commissioned ZGPD id (valid if result ==
   ZB_ZGP_COMMISSIONING_COMPLETED or ZB_ZGP_ZGPD_DECOMMISSIONED).
   @param result - commissioning status
 */
void comm_done_cb(zb_zgpd_id_t zgpd_id,
                  zb_zgp_comm_status_t result)
{
  ZVUNUSED(zgpd_id);
  ZVUNUSED(result);
  TRACE_MSG(TRACE_APP1, "commissioning stopped", (FMT__0));
#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
}


/**
   Start ZGP commissioning

   @param param - not used.
 */
//! [start_comm]
void start_comm(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "start commissioning", (FMT__0));
  zb_zgps_start_commissioning(0);

#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));

#endif
  /* Switch off trace if directly communicate with GPD because it slows down
     communication so we can't fit into bidirectional commissioning timings */
  if (!zb_zgp_get_skip_gpdf())
  {
//! [set_trace]
    ZB_SET_TRACE_LEVEL(1);
    ZB_SET_TRACE_MASK(0);
//! [set_trace]
  }
}
//! [start_comm]

/**
   Force ZGP commissioning stop

   @param param - not used.
 */
//! [stop_comm]
void stop_comm(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "stop commissioning", (FMT__0));
  zb_zgps_stop_commissioning();
#ifdef ZB_USE_BUTTONS
  zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
}
//! [stop_comm]

/**
   ZDO start/commissioning state change callback

   @param - buffer with event
 */
//! [gcomm_complete]
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_ZCL1, "> zboss_signal_handler %hd", (FMT__H, param));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

#ifndef ZB_USE_BUTTONS
        /* if we do not have buttons, use alarm */
        ZB_SCHEDULE_APP_ALARM(start_comm, 0,
                              ZB_MILLISECONDS_TO_BEACON_INTERVAL(12000));
#endif
        break;
      case ZB_ZGP_SIGNAL_COMMISSIONING:
      {
        zb_zgp_signal_commissioning_params_t *comm_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zgp_signal_commissioning_params_t);
        comm_done_cb(comm_params->zgpd_id, comm_params->result);
      }
      break;

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
      {
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
        break;
      }

//! [gcomm_complete]
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

  TRACE_MSG(TRACE_ZCL1, "< zboss_signal_handler", (FMT__0));
}
