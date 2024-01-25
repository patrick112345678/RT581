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

#define ZB_TRACE_FILE_ID 41679
#include "zboss_api.h"
#include "ncp/ncp_host_api.h"

#include "zb_common.h"
#include "zb_types.h"
#include "zb_aps.h"

#define CHANNEL_MASK (1l << 11)
#define PAN_ID 0x1AAA

/* IEEE address of the device */
zb_ieee_addr_t d_ieee_addr = {0x44, 0x33, 0x22, 0x11, 0x00, 0x50, 0x50, 0x50};
/* Standard network key */
zb_uint8_t nwk_key[ZB_CCM_KEY_SIZE] = {0x11, 0xaa, 0x22, 0xbb, 0x33, 0xcc, 0x44, 0xdd, 0, 0, 0, 0, 0, 0, 0, 0};

/* IEEE address of the router */
zb_ieee_addr_t child_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void ncp_host_handle_reset_response(zb_ret_t status_code, zb_bool_t is_solicited)
{
  zb_ret_t ret;
  static zb_uint8_t reset_count = 0;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_reset_response, is_solicited %hd", (FMT__H, is_solicited));

  ZB_ASSERT(status_code == RET_OK);

  if (0 == reset_count)
  {
    TRACE_MSG(TRACE_APP1, "First boot", (FMT__0));

    reset_count += 1;
    ret = ncp_host_factory_reset();
    ZB_ASSERT(ret == RET_OK);
  }
  else if (1 == reset_count)
  {
    TRACE_MSG(TRACE_APP1, "Second boot, after factory reset", (FMT__0));

    ret = ncp_host_get_module_version();
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Unexpected reboot", (FMT__0));
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_reset_response", (FMT__0));
}

void ncp_host_handle_get_module_version_response(zb_ret_t status_code, zb_uint32_t version)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_module_version_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  ZB_ASSERT(version == NCP_FW_VERSION);

#ifndef ZB_PRODUCTION_CONFIG
  ret = ncp_host_set_local_ieee_addr(0, d_ieee_addr);
#else
  ret = ncp_host_get_local_ieee_addr(0);
#endif
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_module_version_response", (FMT__0));
}

void ncp_host_handle_set_local_ieee_addr_response(zb_ret_t status_code)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_local_ieee_addr_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_get_local_ieee_addr(0);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_local_ieee_addr_response", (FMT__0));
}

void ncp_host_handle_get_local_ieee_addr_response(zb_ret_t status_code, zb_ieee_addr_t addr,
                                                  zb_uint8_t interface_num)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_local_ieee_addr_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  TRACE_MSG(TRACE_APP1, "d_ieee_addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(d_ieee_addr)));
  TRACE_MSG(TRACE_APP1, "addr " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
  ZB_ASSERT(ZB_IEEE_ADDR_CMP(addr, d_ieee_addr));
  ZB_ASSERT(interface_num == 0);

  ret = ncp_host_set_zigbee_channel_mask(0, CHANNEL_MASK);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_local_ieee_addr_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_channel_mask_response(zb_ret_t status_code)
{
  zb_ret_t ret;
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_channel_mask_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_set_zigbee_pan_id(PAN_ID);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_channel_mask_response", (FMT__0));
}

 void ncp_host_handle_set_zigbee_pan_id_response(zb_ret_t status_code)
{
  zb_ret_t ret;
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_pan_id_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_set_zigbee_role(ZB_NWK_DEVICE_TYPE_COORDINATOR);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_pan_id_response", (FMT__0));
}

void ncp_host_handle_set_zigbee_role_response(zb_ret_t status_code)
{
  zb_ret_t ret;
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_zigbee_role_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_set_nwk_key(nwk_key, 0);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_zigbee_role_response", (FMT__0));
}

void ncp_host_handle_set_nwk_key_response(zb_ret_t status_code)
{
  zb_ret_t ret;
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_set_nwk_key_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_get_nwk_keys();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_set_nwk_key_response", (FMT__0));
}

void ncp_host_handle_get_nwk_keys_response(zb_ret_t status_code,
                                           zb_uint8_t *nwk_key1, zb_uint8_t key_number1,
                                           zb_uint8_t *nwk_key2, zb_uint8_t key_number2,
                                           zb_uint8_t *nwk_key3, zb_uint8_t key_number3)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);

  ZVUNUSED(nwk_key1);
  ZVUNUSED(nwk_key2);
  ZVUNUSED(nwk_key3);
  ZVUNUSED(key_number1);
  ZVUNUSED(key_number2);
  ZVUNUSED(key_number3);

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_nwk_keys_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  req->scan_channels_list[0] = CHANNEL_MASK;
  req->scan_duration = 5;
  req->distributed_network = 0;
  req->distributed_network_address = 0;

  ncp_host_nwk_formation_adapter(buf);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_nwk_keys_response", (FMT__0));
}

void ncp_host_handle_nwk_formation_response_adapter(zb_bufid_t buf)
{
  zb_nlme_permit_joining_request_t *req;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_formation_response_adapter", (FMT__0));

  ZB_ASSERT(zb_buf_get_status(buf) == RET_OK);
  zb_buf_reuse(buf);

  req = ZB_BUF_GET_PARAM(buf, zb_nlme_permit_joining_request_t);
  req->permit_duration = 180;

  ncp_host_nwk_permit_joining_adapter(buf);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_formation_response_adapter", (FMT__0));
}

void ncp_host_handle_nwk_permit_joining_response_adapter(zb_bufid_t buf)
{
  zb_ret_t ret;
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_permit_joining_response_adapter", (FMT__0));

  ZB_ASSERT(zb_buf_get_status(buf) == RET_OK);
  zb_buf_free(buf);

  ret = ncp_host_get_zigbee_role();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_permit_joining_response_adapter", (FMT__0));
}

void ncp_host_handle_get_zigbee_role_response(zb_ret_t status_code, zb_uint8_t zigbee_role)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_role_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  ZB_ASSERT(zigbee_role == ZB_NWK_DEVICE_TYPE_COORDINATOR);

  ret = ncp_host_get_zigbee_channel_mask();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_role_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_channel_mask_response(zb_ret_t status_code, zb_uint8_t channel_list_len,
                                                      zb_uint8_t channel_page, zb_uint32_t channel_mask)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_channel_mask_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ZB_ASSERT(channel_mask == CHANNEL_MASK);
  ZB_ASSERT(channel_page == 0);
  ZB_ASSERT(channel_list_len == 1);

  ret = ncp_host_get_zigbee_channel();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_channel_mask_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_channel_response(zb_ret_t status_code, zb_uint8_t page, zb_uint8_t channel)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_channel_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  ZB_ASSERT(page == 0);
  ZB_ASSERT(channel == 11);

  ret = ncp_host_get_zigbee_pan_id();
  ZB_ASSERT(ret == RET_OK);


  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_channel_response", (FMT__0));
}

void ncp_host_handle_get_zigbee_pan_id_response(zb_ret_t status_code, zb_uint16_t pan_id)
{
  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  ZB_ASSERT(pan_id == PAN_ID);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_zigbee_pan_id_response", (FMT__0));
}

void ncp_host_handle_zdo_dev_annce_signal(zb_zdo_signal_device_annce_params_t *da)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_zdo_dev_annce_signal", (FMT__0));

  ZB_ASSERT(ZB_64BIT_ADDR_CMP(da->ieee_addr, child_ieee_addr));

  ret = ncp_host_nwk_get_ieee_by_short(da->device_short_addr);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_zdo_dev_annce_signal", (FMT__0));
}

void ncp_host_handle_nwk_get_ieee_by_short_response(zb_ret_t status_code, zb_ieee_addr_t ieee_addr)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_ieee_by_short_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);
  ZB_ASSERT(ZB_64BIT_ADDR_CMP(child_ieee_addr, ieee_addr));

  ret = ncp_host_nwk_get_neighbor_by_ieee(ieee_addr);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_ieee_by_short_response", (FMT__0));
}

void ncp_host_handle_nwk_get_neighbor_by_ieee_response(zb_ret_t status_code)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_neighbor_by_ieee_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  /* TODO: change child_ieee_addr to ieee_addr from Get Neighbor by IEEE command */
  ret = ncp_host_nwk_get_short_by_ieee(child_ieee_addr);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_neighbor_by_ieee_response", (FMT__0));
}

void ncp_host_handle_nwk_get_short_by_ieee_response(zb_ret_t status_code, zb_uint16_t short_addr)
{
  zb_ret_t ret;

  ZVUNUSED(short_addr);

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_nwk_get_short_by_ieee_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  ret = ncp_host_get_aps_key_by_ieee(child_ieee_addr);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_nwk_get_short_by_ieee_response", (FMT__0));
}

void ncp_host_handle_get_aps_key_by_ieee_response(zb_ret_t status_code, zb_uint8_t *aps_key)
{
  ZVUNUSED(aps_key);

  TRACE_MSG(TRACE_APP1, ">> ncp_host_handle_get_aps_key_by_ieee_response", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  TRACE_MSG(TRACE_APP1, "<< ncp_host_handle_get_aps_key_by_ieee_response", (FMT__0));
}

void ncp_host_handle_apsde_data_indication_adapter(zb_bufid_t buf)
{
  zb_uint8_t* data_ptr;
  zb_uint16_t addr;
  zb_uint8_t i;
  zb_ret_t ret;
  zb_apsde_data_req_t *req;
  zb_apsde_data_indication_t *ind =  ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t);

  addr = ind->src_addr;

  TRACE_MSG(TRACE_APP1, ">>  ncp_host_handle_apsde_data_indication_adapter", (FMT__0));

  data_ptr = (zb_uint8_t *)zb_buf_begin(buf);

  for (i = 0 ; i < zb_buf_len(buf); i++)
  {
    TRACE_MSG(TRACE_APP1, " 0x%hx", (FMT__H, data_ptr[i]));
  }

  req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);
  req->dst_addr.addr_short = addr;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);

  ret = ncp_host_apsde_data_request_adapter(buf);

  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_APP1, "<<  ncp_host_handle_apsde_data_indication_adapter", (FMT__0));
}

void ncp_host_handle_apsde_data_response_adapter(zb_bufid_t buf)
{
  TRACE_MSG(TRACE_APP1, ">>  ncp_host_handle_apsde_data_response_adapter", (FMT__0));

  zb_buf_free(buf);

  TRACE_MSG(TRACE_APP1, "<<  ncp_host_handle_apsde_data_response_adapter", (FMT__0));
}

void ncp_host_handle_apsme_bind_response_adapter(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_APP1, ">>  ncp_host_handle_apsme_bind_response_adapter", (FMT__0));

  ZB_ASSERT(status_code == RET_OK);

  TRACE_MSG(TRACE_APP1, "<<  ncp_host_handle_apsme_bind_response_adapter", (FMT__0));
}


int main()
{
  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT);

  ZB_INIT("zdo_echo_zc");

  while(1)
  {
    zb_sched_loop_iteration();
  }

  return 0;
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
        TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_ANNCE", (FMT__0));
        ncp_host_handle_zdo_dev_annce_signal(ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t));
        break;

      default:
        TRACE_MSG(TRACE_APP1, "Unhandled signal 0x%hx", (FMT__H, sig));
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}
