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
/*  PURPOSE: Simple GW application IAS CIE additions
*/

#define ZB_TRACE_FILE_ID 63593

#include "simple_gw.h"
#include "simple_gw_device.h"

#if defined IAS_CIE_ENABLED

extern simple_gw_device_ctx_t g_device_ctx;
extern zb_ieee_addr_t g_zc_addr;

void simple_gw_remove_device_delayed(zb_uint8_t idx);

void find_ias_zone_device_tmo(zb_uint8_t dev_idx)
{
  TRACE_MSG(TRACE_APP1, "find_ias_zone_device_tmo", (FMT__0));

  ZB_ASSERT(dev_idx < SIMPLE_GW_DEV_NUMBER);
  if ((g_device_ctx.devices[dev_idx].dev_state != NO_DEVICE) &&
      (g_device_ctx.devices[dev_idx].dev_state != COMPLETED_NO_TOGGLE))
  {
    ZB_SCHEDULE_APP_CALLBACK(simple_gw_remove_device_delayed, dev_idx);
  }
}

void find_ias_zone_device_delayed(zb_uint8_t idx)
{
  zb_buf_get_out_delayed_ext(find_ias_zone_device, g_device_ctx.devices[idx].short_addr, 0);
}

void find_ias_zone_device(zb_uint8_t param, zb_uint16_t short_addr)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_param_t *req;

  TRACE_MSG(TRACE_APP1, ">> find_ias_zone_device %hd", (FMT__H, param));

  req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

  req->nwk_addr = short_addr;
  req->addr_of_interest = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  req->profile_id = ZB_AF_HA_PROFILE_ID;
  /* We are searching for IAS ZONE Server */
  req->num_in_clusters = 1;
  req->num_out_clusters = 0;
  req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_IAS_ZONE;

  zb_zdo_match_desc_req(param, find_ias_zone_device_cb);

  TRACE_MSG(TRACE_APP1, "<< find_ias_zone_device %hd", (FMT__H, param));
}

void find_ias_zone_device_cb(zb_uint8_t param)
{
  zb_bufid_t buf = param;
  zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t*)zb_buf_begin(buf);
  zb_uint8_t *match_ep = NULL;
  zb_apsde_data_indication_t *ind = NULL;
  zb_uint8_t dev_idx = SIMPLE_GW_INVALID_DEV_INDEX;

  TRACE_MSG(TRACE_APP1, ">> find_ias_zone_device_cb param %hd, resp match_len %hd", (FMT__H_H, param, resp->match_len));

  if (resp->status == ZB_ZDP_STATUS_SUCCESS)
  {
    ind = ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t);
    dev_idx = simple_gw_get_dev_index_by_short_addr(ind->src_addr);

    if (dev_idx != SIMPLE_GW_INVALID_DEV_INDEX)
    {
      if (resp->match_len)
      {
      /* Match EP list follows right after response header */
      match_ep = (zb_uint8_t*)(resp + 1);

      /* we are searching for exact cluster, so only 1 EP maybe found */
      g_device_ctx.devices[dev_idx].short_addr = ind->src_addr;
      g_device_ctx.devices[dev_idx].endpoint = *match_ep;
      g_device_ctx.devices[dev_idx].dev_state = WRITE_CIE_ADDR;

      ZB_SCHEDULE_APP_ALARM_CANCEL(find_ias_zone_device_tmo, dev_idx);

      TRACE_MSG(TRACE_APP2, "found dev addr %d ep %hd dev_idx %hd",
                (FMT__D_H_H, g_device_ctx.devices[dev_idx].short_addr, g_device_ctx.devices[dev_idx].endpoint, dev_idx));

      /* Next step is to send CIE address */
      ZB_SCHEDULE_APP_CALLBACK2(write_cie_addr, param, dev_idx);
        param = 0;
    }
    else
    {
      g_device_ctx.devices[dev_idx].short_addr = ind->src_addr;
      /* It is neither onoff nor ias zone device, but lets keep it w/o any
      additional configuration. */
      g_device_ctx.devices[dev_idx].dev_state = COMPLETED_NO_TOGGLE;
      ZB_SCHEDULE_APP_ALARM_CANCEL(find_ias_zone_device_tmo, dev_idx);
    }
    }
    else
    {
      TRACE_MSG(TRACE_APP2, "Device not found!", (FMT__0));
    }
  }

  if (param)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APP1, "<< find_ias_zone_device_cb", (FMT__0));
}

void write_cie_addr(zb_uint8_t param, zb_uint16_t dev_idx)
{
  zb_uint8_t *cmd_ptr;

  g_device_ctx.devices[dev_idx].dev_state = COMPLETED_NO_TOGGLE;

  ZB_ZCL_GENERAL_INIT_WRITE_ATTR_REQ(param, cmd_ptr, ZB_ZCL_ENABLE_DEFAULT_RESPONSE);

  ZB_ZCL_GENERAL_ADD_VALUE_WRITE_ATTR_REQ(cmd_ptr, ZB_ZCL_ATTR_IAS_ZONE_IAS_CIE_ADDRESS_ID,
                                          ZB_ZCL_ATTR_TYPE_IEEE_ADDR, (zb_uint8_t*)g_zc_addr);

  ZB_ZCL_GENERAL_SEND_WRITE_ATTR_REQ(param, cmd_ptr, g_device_ctx.devices[dev_idx].short_addr,
                                     ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                     g_device_ctx.devices[dev_idx].endpoint, SIMPLE_GW_ENDPOINT,
                                     ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_IAS_ZONE, NULL);
}

#endif  /* IAS_CIE_ENABLED */


