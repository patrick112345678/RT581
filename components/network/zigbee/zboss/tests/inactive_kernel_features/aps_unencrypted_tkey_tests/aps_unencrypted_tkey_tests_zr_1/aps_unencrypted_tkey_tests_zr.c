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
/* PURPOSE: APS Unencrypted Transport Key feature router
*/
#define ZB_TRACE_FILE_ID 45967
#include "aps_unencrypted_tkey_tests_zr.h"
#include "aps_unencrypted_tkey_tests_zr_hal.h"
#ifdef ZB_LIMIT_VISIBILITY
#include "zb_mac.h"
#endif

zb_ieee_addr_t g_zr_addr = ZR1_IEEE_ADDRESS;

#ifdef ZB_ASSERT_SEND_NWK_REPORT
void assert_indication_cb(zb_uint16_t file_id, zb_int_t line_number);
#endif

#if !defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to build led bulb demo
#endif

/** [COMMON_DECLARATION] */

/******************* Declare attributes ************************/
aps_unencrypted_tkey_tests_zr_ctx_t zr_ctx;

void test_leave_nwk(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, ">> test_leave_nwk param %hd", (FMT__H, param));

  /* We are going to leave */
  if (!param)
  {
    ZB_GET_OUT_BUF_DELAYED(test_leave_nwk);
  }
  else
  {
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_mgmt_leave_param_t *req_param;

    req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    /* Set dst_addr == local address for local leave */
    req_param->dst_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    zdo_mgmt_leave_req(param, NULL);
  }

  TRACE_MSG(TRACE_ERROR, "<< test_leave_nwk", (FMT__0));
}

void test_restart_join_nwk(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ERROR, "test_restart_join_nwk %hd", (FMT__H, param));
  if (param == ZB_NWK_LEAVE_TYPE_RESET)
  {
    bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
  }
}

static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  return 0x0002;
}

MAIN()
{
  ARGV_UNUSED;

  aps_unencrypted_tkey_zr_hal_init();

//! [switch_trace]
  ZB_SET_TRACE_ON();
//! [switch_trace]
   ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("led_bulb");

  zb_set_long_address(g_zr_addr);
  zb_set_network_router_role(APS_UNENCRYPTED_TKEY_TESTS_ZR_CHANNEL_MASK);
  zb_set_max_children(1);
  /* Erase NVRAM if BUTTON2 is pressed on start */
  zb_set_nvram_erase_at_start(aps_unencrypted_tkey_zc_hal_is_button_pressed());
  //zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_nwk_set_address_assignment_cb(addr_ass_cb);
#ifdef ZB_LIMIT_VISIBILITY
  mac_add_invisible_short(0x0003);
#endif

  zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));

  bulb_app_ctx_init(0);

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

  bulb_app_ctx_init();

  TRACE_MSG(TRACE_APP1, "<< bulb_device_app_init", (FMT__0));
}

void light_control_button_handler(zb_uint8_t button_no);

void tests_aps_unencrypted_tkey_zr_button_pressed(zb_uint8_t button_no)
{
  switch (button_no)
  {
    case LEAVE_NET_BUTTON:
    {
      switch (zr_ctx.button_leave_net.button_state)
      {
        case APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_IDLE:
          zr_ctx.button_leave_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_PRESSED;
          zr_ctx.button_leave_net.timestamp = ZB_TIMER_GET();
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_PRESSED:
          zr_ctx.button_leave_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_UNPRESSED;
          ZB_SCHEDULE_ALARM(light_control_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_DEBOUNCE_PERIOD);
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_UNPRESSED:
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

void light_control_button_handler(zb_uint8_t button_no)
{
  switch (button_no)
  {
    case LEAVE_NET_BUTTON:
      ZB_GET_OUT_BUF_DELAYED(test_leave_nwk);
      zr_ctx.button_leave_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZR_BUTTON_STATE_IDLE;
      break;
    default:
      break;
  }
}

void bulb_app_ctx_init()
{
  TRACE_MSG(TRACE_APP1, ">> bulb_app_ctx_init", (FMT__0));

  ZB_MEMSET(&zr_ctx, 0, sizeof(zr_ctx));

  TRACE_MSG(TRACE_APP1, "<< bulb_app_ctx_init", (FMT__0));
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
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;


      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP1, "Device announcement signal", (FMT__0));
        /*param = 0;*/
        break;

      case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        TRACE_MSG(TRACE_APP1, "Loading application production config", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_LEAVE:
        {
          zb_zdo_signal_leave_params_t *leave_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);
          test_restart_join_nwk(leave_params->leave_type);
        }
        break;
      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else
  {

    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}
