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
/* PURPOSE: APS Unencrypted Transport Key feature coordinator
*/

#define ZB_TRACE_FILE_ID 40956
#include "aps_unencrypted_tkey_tests_zc.h"
#include "aps_unencrypted_tkey_tests_zc_hal.h"
#ifdef ZB_LIMIT_VISIBILITY
#include "zb_mac.h"
#endif

zb_ieee_addr_t g_zc_addr = APS_UNENCRYPTED_TKEY_TESTS_ZC_ADDRESS;

aps_unencrypted_tkey_tests_zc_ctx_t zc_ctx;

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static void init_ctx()
{
  zb_uint8_t i;

  ZB_BZERO(&zc_ctx, sizeof(aps_unencrypted_tkey_tests_zc_ctx_t));

  for (i = 0; i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES; ++i)
  {
    zc_ctx.devices[i].led_num = 0xFF;
  }
}

void aps_unencrypted_tkey_tests_zc_send_leave_req_cb(zb_uint8_t param)
{
  zb_free_buf(ZB_BUF_FROM_REF(param));
}

void aps_unencrypted_tkey_tests_zc_send_leave_req(zb_uint8_t param, zb_uint16_t short_addr)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zdo_mgmt_leave_param_t *req_param;
  zb_address_ieee_ref_t addr_ref;
  zb_uint8_t i;

  for (i=0; i<APS_UNENCRYPTED_TKEY_TESTS_DEVICES && zc_ctx.devices[i].nwk_addr != short_addr; i++);
  if (i<APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
  {
    aps_unencrypted_tkey_zc_device_leaved_indication(zc_ctx.devices[i].led_num);
    zc_ctx.devices[i].led_num = 0xFF;
  }

  if (zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
    req_param = ZB_GET_BUF_PARAM(buf, zb_zdo_mgmt_leave_param_t);
    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

    req_param->dst_addr = short_addr;
    req_param->rejoin = 0;
    zdo_mgmt_leave_req(param, NULL);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "tried to remove 0x%xd, but device is already left", (FMT__D, short_addr));
    zb_free_buf(buf);
  }
}

void aps_unencrypted_tkey_tests_zc_delayed_leave(zb_uint8_t param)
{
  if (!param)
  {
    zb_uint8_t i;
    for (i=0; i<APS_UNENCRYPTED_TKEY_TESTS_DEVICES &&
              zc_ctx.devices[i].dev_type != SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY; i++);
    if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
    {
      zb_uint16_t short_addr = zc_ctx.devices[i].nwk_addr;
      zc_ctx.devices[i].dev_type = SIMPLE_DEV_TYPE_UNUSED;
      ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_send_leave_req, short_addr);
    }
  }
  else
  {
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_free_buf(buf);
  }
}

void aps_unencrypted_tkey_tests_zc_open_network(zb_uint8_t param, zb_uint16_t user_param)
{
  zb_zdo_mgmt_permit_joining_req_param_t *req_param;
  zb_uint8_t permit = ZB_GET_LOW_BYTE(user_param);
  zb_uint8_t for_myself = ZB_GET_HI_BYTE(user_param);

  TRACE_MSG(TRACE_APP3, "> sp_permit_joining %hd permit %hd for_myself %hd", (FMT__H_H_H, param, permit, for_myself));

  ZB_ASSERT(param);

  req_param = ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_permit_joining_req_param_t);

  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
  req_param->dest_addr = (for_myself) ? ZB_PIBCACHE_NETWORK_ADDRESS() :
                                        ZB_NWK_BROADCAST_ROUTER_COORDINATOR;
  req_param->permit_duration = (permit) ? ZB_BDBC_MIN_COMMISSIONING_TIME_S : 0;
  req_param->tc_significance = 1;
  zb_zdo_mgmt_permit_joining_req(param, NULL);

  if (!for_myself)
  {
    /* Repeat the same command for myself */
    ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_open_network, (user_param) | (1 << 8));
  }

  TRACE_MSG(TRACE_APP3, "< simple_gw_permit_joining_myself", (FMT__0));
}

void aps_unencrypted_tkey_tests_zc_button_handler(zb_uint8_t button_no);

void aps_unencrypted_tkey_tests_zc_button_pressed(zb_uint8_t button_no)
{
  switch (button_no)
  {
    case OPEN_NET_BUTTON:
    {
      switch (zc_ctx.button_open_net.button_state)
      {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
          zc_ctx.button_open_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
          zc_ctx.button_open_net.timestamp = ZB_TIMER_GET();
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
          zc_ctx.button_open_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
          ZB_SCHEDULE_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
          break;
      }
      break;
    }
    case APS_TKEY_SECURITY_BUTTON:
    {
      switch (zc_ctx.button_aps_unencrypted_tkey.button_state)
      {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
          zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
          zc_ctx.button_aps_unencrypted_tkey.timestamp = ZB_TIMER_GET();
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
          zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
          ZB_SCHEDULE_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
          break;
      }
      break;
    }
    case ZDO_LEAVE_BUTTON:
    {
      switch (zc_ctx.button_zdo_leave.button_state)
      {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
          zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
          zc_ctx.button_zdo_leave.timestamp = ZB_TIMER_GET();
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
          zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
          ZB_SCHEDULE_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
          break;
      }
      break;
    }
    case CLOSE_NET_BUTTON:
    {
      switch (zc_ctx.button_close_net.button_state)
      {
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE:
          zc_ctx.button_close_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED;
          zc_ctx.button_close_net.timestamp = ZB_TIMER_GET();
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_PRESSED:
          zc_ctx.button_close_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED;
          ZB_SCHEDULE_ALARM(aps_unencrypted_tkey_tests_zc_button_handler, button_no, APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_DEBOUNCE_PERIOD);
          break;
        case APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_UNPRESSED:
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

void aps_unencrypted_tkey_tests_zc_button_handler(zb_uint8_t button_no)
{
  switch (button_no)
  {
    case OPEN_NET_BUTTON:
      ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_open_network, 1);
      zc_ctx.button_open_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
      break;
    case APS_TKEY_SECURITY_BUTTON:
      zb_is_aps_unencrypted_transport_key_join_on() ?
      zb_set_aps_unencrypted_transport_key_join_off() :
      zb_set_aps_unencrypted_transport_key_join_on();
      zc_ctx.button_aps_unencrypted_tkey.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
      break;
    case ZDO_LEAVE_BUTTON:
      ZB_SCHEDULE_CALLBACK(aps_unencrypted_tkey_tests_zc_delayed_leave, 0);
      zc_ctx.button_zdo_leave.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
      break;
    case CLOSE_NET_BUTTON:
      ZB_GET_OUT_BUF_DELAYED2(aps_unencrypted_tkey_tests_zc_open_network, 0);
      zc_ctx.button_close_net.button_state = APS_UNENCRYPTED_TKEY_TESTS_ZC_BUTTON_STATE_IDLE;
      break;
    default:
      break;
  }
}

static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  return 0x0001;
}

MAIN()
{
  ARGV_UNUSED;

  aps_unencrypted_tkey_zc_hal_init();
  init_ctx();

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("zdo_zc");

  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role(APS_UNENCRYPTED_TKEY_TESTS_CHANNEL_MASK);
  zb_set_max_children(1);

  zb_nwk_set_address_assignment_cb(addr_ass_cb);
#ifdef ZB_LIMIT_VISIBILITY
  mac_add_invisible_short(0x0002);
  mac_add_invisible_short(0x0003);
#endif

  /* nRF52840: Erase NVRAM if BUTTON3 is pressed on start */
  zb_set_nvram_erase_at_start(aps_unencrypted_tkey_zc_hal_is_button_pressed());
  //zb_set_nvram_erase_at_start(ZB_TRUE);

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

static void device_annce_handler(zb_uint8_t buf_ref)
{
  zb_uint8_t i;

  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_signal_device_annce_params_t *dev_annce_params;

  zb_get_app_signal(buf_ref, &sg_p);
  dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

  TRACE_MSG(TRACE_APP1, "dev_annce short_addr 0x%x", (FMT__D, dev_annce_params->device_short_addr));

  for (i=0; i<APS_UNENCRYPTED_TKEY_TESTS_DEVICES; i++)
  {
    if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNUSED)
    {
      zc_ctx.devices[i].nwk_addr = dev_annce_params->device_short_addr;
      zb_address_ieee_by_short(zc_ctx.devices[i].nwk_addr,zc_ctx.devices[i].ieee_addr);
      zc_ctx.devices[i].dev_type = zb_is_aps_unencrypted_transport_key_join_on() ?
                                   (zb_uint8_t)SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY :
                                   (zb_uint8_t)SIMPLE_DEV_TYPE_APS_ENCRYPTED_TKEY;

      if (zc_ctx.devices[i].dev_type == (zb_uint8_t)SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY)
      {
        zb_uint8_t led_ind = aps_unencrypted_tkey_zc_device_joined_indication();
        zc_ctx.devices[i].led_num = led_ind;
      }
      break;
    }
  }
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
        zb_bdb_set_legacy_device_support(1);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ZB_SCHEDULE_ALARM(device_annce_handler, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
        param=0;
        break;
      case ZB_ZDO_SIGNAL_LEAVE_INDICATION:
        {
          zb_uint8_t i;
          zb_zdo_signal_leave_indication_params_t *leave_ind_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_indication_params_t);
          for (i=0; i<APS_UNENCRYPTED_TKEY_TESTS_DEVICES &&
                (ZB_MEMCMP(&zc_ctx.devices[i].ieee_addr, &leave_ind_params->device_addr, sizeof(zb_ieee_addr_t)) != 0 ||
                zc_ctx.devices[i].dev_type != SIMPLE_DEV_TYPE_APS_UNENCRYPTED_TKEY); i++);
          if (i < APS_UNENCRYPTED_TKEY_TESTS_DEVICES)
          {
            zc_ctx.devices[i].dev_type = SIMPLE_DEV_TYPE_UNUSED;
            aps_unencrypted_tkey_zc_device_leaved_indication(zc_ctx.devices[i].led_num);
          }
        }
        break;
      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal 0x%hx", (FMT__H, sig));
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
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}
