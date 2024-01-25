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
/* PURPOSE: Parent loss test - ZC test harness
*/


#define ZB_TRACE_FILE_ID 63797
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../nwk/nwk_internal.h"

#include "zboss_api.h"
#include "pl_common.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

zb_ieee_addr_t g_zc_addr = PL_ZC_IEEE_ADDR;
zb_ieee_addr_t g_zr_addr = PL_ZR1_IEEE_ADDR;
zb_ieee_addr_t g_zed_addr = PL_ZED_IEEE_ADDR;

static void pl_th_zc_init(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_set_max_children(1);
  zb_set_long_address(g_zc_addr);
  zb_set_network_coordinator_role_legacy(TEST_CHANNEL_MASK);

  //MAC_ADD_VISIBLE_LONG(g_zr_addr);
}


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_SET_TRAF_DUMP_ON();

  ZB_INIT("pl_th_zc");

  pl_th_zc_init(0);

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


static void pl_send_permit_joining(zb_uint8_t param)
{
  zb_zdo_mgmt_permit_joining_req_param_t *req =
    ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_permit_joining_req_param_t);

  ZB_BZERO(req, sizeof(zb_zdo_mgmt_permit_joining_req_param_t));
  req->permit_duration = 2;     /* 2 seconds */
  req->dest_addr = ZB_PIBCACHE_NETWORK_ADDRESS();

  zb_zdo_mgmt_permit_joining_req(param, NULL);
}

static void send_addr_conflict(zb_uint8_t param)
{
  zb_nlme_send_status_t *request =
    ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_nlme_send_status_t);

  request->dest_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  request->status.status = ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT;
  request->status.network_addr = zb_address_short_by_ieee(g_zr_addr);
  request->ndsu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
  ZB_SCHEDULE_ALARM(zb_nlme_send_status, param, ZB_NWK_OCTETS_TO_BI(ZB_NWKC_MAX_BROADCAST_JITTER_OCTETS));

  TRACE_MSG(TRACE_APP1, "Sending address conflict message", (FMT__0));
}

/* Will choose random panid */
static void send_panid_change(zb_uint8_t param)
{
  ZG->nwk.handle.state = ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION;
  zb_panid_conflict_network_update(param);
}

static void send_channel_change(zb_uint8_t param)
{
  zb_uint8_t i;
  zb_int8_t new_channel;
  zb_uint32_t aps_channel_mask;
  zb_zdo_mgmt_nwk_update_req_t *req;

  new_channel = 0;
  aps_channel_mask = zb_aib_channel_page_list_get_2_4GHz_mask();
  for (i = 11; i < 27; i++)
  {
    if (( aps_channel_mask ^ 1L << ZB_PIBCACHE_CURRENT_CHANNEL()) &
        (1L << i))
    {
      new_channel = i;
      break;
    }
  }

  if (new_channel)
  {
      req = ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_nwk_update_req_t);
      req->hdr.scan_channels  = (1L<<new_channel);
      req->hdr.scan_duration  = 0xFE;      /* Channel change command */
      req->scan_count         = 1;
      req->update_id          = ZB_NIB_UPDATE_ID();
      req->dst_addr           =  0xFFFD; /* to RxOnWhenIdle=True devices */

      zb_zdo_mgmt_nwk_update_req(param, NULL);

      ZB_SCHEDULE_ALARM(zb_zdo_do_set_channel, new_channel,
                        ZB_NWK_OCTETS_TO_BI(ZB_NWK_JITTER(
			  ZB_NWK_MAX_BROADCAST_RETRIES * ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS
			)));
  }
  else
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}

static void kick_router_away(zb_uint8_t param)
{
  zb_zdo_mgmt_leave_param_t *req_param;

  req_param = ZB_GET_BUF_PARAM(ZB_BUF_FROM_REF(param), zb_zdo_mgmt_leave_param_t);
  ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_leave_param_t));

  req_param->dst_addr = zb_address_short_by_ieee(g_zr_addr);
  zdo_mgmt_leave_req(param, NULL);
}

static void test_entry_point(zb_uint8_t param)
{
  zb_ieee_addr_t ieee_addr;
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);
  zb_zdo_signal_device_annce_params_t *dev_annce_params =
    ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

  static zb_uint8_t ed_started_n = 0;

  zb_address_ieee_by_short(dev_annce_params->device_short_addr, ieee_addr);

  if (ZB_IEEE_ADDR_CMP(ieee_addr, g_zed_addr))
  {
    ed_started_n++;

    switch(ed_started_n)
    {
      case 1:
        TRACE_MSG(TRACE_ERROR, "DBG: bcast time %d, jitter(bcast) %d",
                  (FMT__D_D, ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS, ZB_NWK_JITTER(ZB_NWK_BROADCAST_DELIVERY_TIME_OCTETS)));

        MAC_ADD_INVISIBLE_SHORT(zb_address_short_by_ieee(g_zed_addr));
        ZB_SCHEDULE_ALARM(send_addr_conflict, param, 30 * ZB_TIME_ONE_SECOND);
        param = 0;
        break;

      case 2:
        ZB_SCHEDULE_ALARM(send_panid_change, param, 30 * ZB_TIME_ONE_SECOND);
        param = 0;
        break;

      case 3:
        ZB_SCHEDULE_ALARM(send_channel_change, param, 30 * ZB_TIME_ONE_SECOND);
        param = 0;
        break;

      case 4:
        MAC_REMOVE_INVISIBLE_SHORT(zb_address_short_by_ieee(g_zed_addr));
        ZB_SCHEDULE_ALARM(kick_router_away, param, 30 * ZB_TIME_ONE_SECOND);
        param = 0;
        break;

      case 5:
        TRACE_MSG(TRACE_APP1, "Test finished!", (FMT__0));
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Invalid rejoin number", (FMT__0));
        break;
    }
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ZCL1, ">> zboss_signal_handler %h", (FMT__H, param));

  if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Skip production config ready signal %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

    zb_free_buf(ZB_BUF_FROM_REF(param));
    return;
  }

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

        ZB_SCHEDULE_CALLBACK(pl_send_permit_joining, param);
        param = 0;
        break;

     case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        ZB_SCHEDULE_CALLBACK(test_entry_point, param);
        param = 0;
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d",
              (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_free_buf(ZB_BUF_FROM_REF(param));
  }

  TRACE_MSG(TRACE_ZCL1, "<< zboss_signal_handler", (FMT__0));
}
