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
/*  PURPOSE: NCP High level transport implementation for the host side.
 *  Stubs for not yet used handlers */

#define ZB_TRACE_FILE_ID 95

#include "ncp_host_hl_transport_internal_api.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"



void ncp_host_handle_get_join_status_response(zb_ret_t status_code, zb_bool_t joined)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_get_rx_on_when_idle_response(zb_ret_t status_code,
                                                  zb_bool_t rx_on_when_idle)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_get_keepalive_timeout_response(zb_ret_t status_code,
                                                    zb_uint32_t keepalive_timeout)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_get_end_device_timeout_response(zb_ret_t status_code,
                                                     zb_uint8_t timeout)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_get_zigbee_role_response(zb_ret_t status_code,
                                              zb_uint8_t zigbee_role)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_get_nwk_keys_response(zb_ret_t status_code,
                                           zb_uint8_t *nwk_key1,
                                           zb_uint8_t key_number1,
                                           zb_uint8_t *nwk_key2,
                                           zb_uint8_t key_number2,
                                           zb_uint8_t *nwk_key3,
                                           zb_uint8_t key_number3)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_nwk_get_ieee_by_short_response(zb_ret_t status_code,
                                                    zb_ieee_addr_t ieee_addr)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_nwk_get_neighbor_by_ieee_response(zb_ret_t status_code)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_nwk_get_short_by_ieee_response(zb_ret_t status_code,
                                                    zb_uint16_t short_addr)
{
  ZB_ASSERT(0);
}

#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
void ncp_host_handle_nwk_route_reply_indication_adapter(zb_bufid_t buf)
{
  ZB_ASSERT(0);
}

void ncp_host_handle_nwk_route_request_send_indication_adapter(zb_bufid_t buf)
{
  ZB_ASSERT(0);
}

void ncp_host_handle_nwk_route_record_send_indication_adapter(zb_bufid_t buf)
{
  ZB_ASSERT(0);
}
#endif


void ncp_host_handle_af_delete_endpoint_response(zb_ret_t status_code)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_af_set_node_descriptor_response(zb_ret_t status_code)
{
  ZB_ASSERT(0);
}


void ncp_host_handle_af_set_power_descriptor_response(zb_ret_t status_code)
{
  ZB_ASSERT(0);
}

#pragma GCC diagnostic pop
