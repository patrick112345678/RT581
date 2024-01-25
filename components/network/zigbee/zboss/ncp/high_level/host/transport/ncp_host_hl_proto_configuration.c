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
/*  PURPOSE: NCP High level transport implementation for the host side: CONFIGURATION category
*/

#define ZB_TRACE_FILE_ID 14

#include "ncp_host_hl_proto.h"

typedef struct zb_ncp_host_proto_config_ctx_s
{
  /* the flag is active if reset request is sent, but response is not received yet */
  zb_bool_t is_reset_request_pending;
} zb_ncp_host_proto_config_ctx_t;

static zb_ncp_host_proto_config_ctx_t g_ncp_host_proto_config_ctx;

void ncp_host_proto_configuration_init_ctx(void)
{
  ZB_BZERO(&g_ncp_host_proto_config_ctx, sizeof(g_ncp_host_proto_config_ctx));

  g_ncp_host_proto_config_ctx.is_reset_request_pending = ZB_FALSE;
}


static void handle_reset_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_bool_t is_solicited = g_ncp_host_proto_config_ctx.is_reset_request_pending;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_reset_response, status_code: %d, solicited %hd",
    (FMT__D_H, error_code, is_solicited));

  ZVUNUSED(len);

  g_ncp_host_proto_config_ctx.is_reset_request_pending = ZB_FALSE;

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_reset_response(error_code, is_solicited);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_reset_response", (FMT__0));
}


static void handle_get_module_version_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint32_t firmware_version = 0;
  zb_uint32_t stack_version = 0;
  zb_uint32_t ncp_protocol_version = 0;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_module_version_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_module_version_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u32(&body, &firmware_version);
    ncp_host_hl_buf_get_u32(&body, &stack_version);
    ncp_host_hl_buf_get_u32(&body, &ncp_protocol_version);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_module_version_response, version 0x%x",
              (FMT__D, firmware_version));
  }

  ncp_host_handle_get_module_version_response(error_code, firmware_version,
                                              stack_version, ncp_protocol_version);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_module_version_response", (FMT__0));
}


static void handle_get_local_ieee_addr_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_ieee_addr_t addr;
  zb_uint8_t mac_interface;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_local_ieee_addr_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_local_ieee_addr_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &mac_interface);
    ncp_host_hl_buf_get_u64addr(&body, addr);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_local_ieee_addr_response, address " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(addr)));
  }
  ncp_host_handle_get_local_ieee_addr_response(response->status_code, addr, mac_interface);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_local_ieee_addr_response", (FMT__0));
}

#if defined ZB_PRODUCTION_CONFIG && defined ZB_NCP_PRODUCTION_CONFIG_ON_HOST
static void handle_request_production_config_response(ncp_hl_response_header_t* response,
                                                        zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint16_t production_config_data_len = 0;
  zb_uint8_t *production_config_data = NULL;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_request_production_config_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse request_production_config_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u16(&body, &production_config_data_len);
    ncp_host_hl_buf_get_ptr(&body, (zb_uint8_t**)&production_config_data, production_config_data_len);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse request_production_config_response, length %d",
              (FMT__D, production_config_data_len));
  }

  ncp_host_handle_request_production_config_response(response->status_code,
    production_config_data_len, production_config_data);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_request_production_config_response", (FMT__0));
}
#endif /* ZB_PRODUCTION_CONFIG && ZB_NCP_PRODUCTION_CONFIG_ON_HOST */


static void handle_set_local_ieee_addr_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_local_ieee_addr_response, status_code %d",
            (FMT__D, error_code));

  ZVUNUSED(len);

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_local_ieee_addr_response(response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_local_ieee_addr_response", (FMT__0));
}


static void handle_get_rx_on_when_idle_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t rx_on_when_idle;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_rx_on_when_idle_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_rx_on_when_idle_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &rx_on_when_idle);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_rx_on_when_idle_response, rx_on_when_idle %hd",
      (FMT__H, rx_on_when_idle));
  }

  ncp_host_handle_get_rx_on_when_idle_response(response->status_code, rx_on_when_idle);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_rx_on_when_idle_response", (FMT__0));
}


static void handle_set_rx_on_when_idle_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_rx_on_when_idle_response, status_code %d",
            (FMT__D, error_code));

  ZVUNUSED(len);

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_rx_on_when_idle_response(response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_rx_on_when_idle_response", (FMT__0));
}


static void handle_set_end_device_timeout_response(ncp_hl_response_header_t* response,
                                                  zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_end_device_timeout_response, status_code %d",
            (FMT__D, error_code));

  ZVUNUSED(len);

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_end_device_timeout_response(response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_end_device_timeout_response", (FMT__0));
}


static void handle_get_end_device_timeout_response(ncp_hl_response_header_t* response,
                                                   zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t timeout;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_end_device_timeout_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_end_device_timeout_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &timeout);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_end_device_timeout_response, timeout %hd",
      (FMT__H, timeout));
  }

  ncp_host_handle_get_end_device_timeout_response(response->status_code, timeout);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_end_device_timeout_response", (FMT__0));
}


static void handle_set_zigbee_channel_mask_response(ncp_hl_response_header_t* response,
                                                    zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_zigbee_channel_mask_response, status_code: %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_zigbee_channel_mask_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_zigbee_channel_mask_response", (FMT__0));
}

static void handle_get_zigbee_channel_mask_response(ncp_hl_response_header_t* response,
                                                    zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t channel_list_len;
  zb_uint8_t channel_page;
  zb_uint32_t channel_mask;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_zigbee_channel_mask_response, status_code: %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();
  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_zigbee_channel_mask_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &channel_list_len);
    ncp_host_hl_buf_get_u8(&body, &channel_page);
    ncp_host_hl_buf_get_u32(&body, &channel_mask);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_zigbee_channel_mask_response, channel_list_len %hd, channel_page %hd, channel_mask 0x%x", (FMT__H_H_D, channel_list_len, channel_page, channel_mask));
  }
  ncp_host_handle_get_zigbee_channel_mask_response(error_code, channel_list_len,
                                                   channel_page, channel_mask);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_zigbee_channel_mask_response", (FMT__0));
}

static void handle_get_zigbee_channel_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t channel;
  zb_uint8_t page;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_zigbee_channel_response, status_code: %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();
  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_zigbee_channel_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &page);
    ncp_host_hl_buf_get_u8(&body, &channel);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_zigbee_channel_response, page %hd, channel %hd",
              (FMT__H_H, page, channel));
  }
  ncp_host_handle_get_zigbee_channel_response(error_code, page, channel);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_zigbee_channel_response", (FMT__0));
}

static void handle_set_zigbee_pan_id_response(ncp_hl_response_header_t* response,
                                              zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_zigbee_pan_id_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_zigbee_pan_id_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_zigbee_pan_id_response", (FMT__0));
}


static void handle_set_zigbee_extended_pan_id_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_zigbee_extended_pan_id_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_handle_set_zigbee_extended_pan_id_response(error_code);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_zigbee_extended_pan_id_response", (FMT__0));
}


static void handle_get_zigbee_pan_id_response(ncp_hl_response_header_t* response,
                                              zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint16_t pan_id;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_zigbee_pan_id_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_zigbee_pan_id_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u16(&body, &pan_id);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_zigbee_pan_id_response, pan_id 0x%x",
              (FMT__D, pan_id));
  }
  ncp_host_handle_get_zigbee_pan_id_response(error_code, pan_id);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_zigbee_pan_id_response", (FMT__0));
}

static void handle_set_zigbee_role_response(ncp_hl_response_header_t* response,
                                            zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_zigbee_role_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_zigbee_role_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_zigbee_role_response", (FMT__0));
}


static void handle_set_max_children_response(ncp_hl_response_header_t* response,
                                             zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_max_children_response, status_code %d", (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_max_children_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_max_children_response", (FMT__0));
}


static void handle_get_zigbee_role_response(ncp_hl_response_header_t* response,
                                            zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t zigbee_role;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);


  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_zigbee_role_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_zigbee_role_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &zigbee_role);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_zigbee_role_response, zigbee_role %d",
              (FMT__D, zigbee_role));
  }
  ncp_host_handle_get_zigbee_role_response(error_code, zigbee_role);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_zigbee_role_response", (FMT__0));
}

static void handle_set_nwk_key_response(ncp_hl_response_header_t* response,
                                        zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_nwk_key_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_nwk_key_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_nwk_key_response", (FMT__0));
}

static void handle_get_nwk_keys_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t nwk_key1[ZB_CCM_KEY_SIZE];
  zb_uint8_t nwk_key2[ZB_CCM_KEY_SIZE];
  zb_uint8_t nwk_key3[ZB_CCM_KEY_SIZE];
  zb_uint8_t key_number1;
  zb_uint8_t key_number2;
  zb_uint8_t key_number3;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_nwk_keys_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_nwk_keys_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u128key(&body, nwk_key1);
    ncp_host_hl_buf_get_u8(&body, &key_number1);
    ncp_host_hl_buf_get_u128key(&body, nwk_key2);
    ncp_host_hl_buf_get_u8(&body, &key_number2);
    ncp_host_hl_buf_get_u128key(&body, nwk_key3);
    ncp_host_hl_buf_get_u8(&body, &key_number3);

    TRACE_MSG(TRACE_TRANSPORT3, "    key number %d, nwk key " TRACE_FORMAT_128,
            (FMT__H_A_A, key_number1, TRACE_ARG_128(nwk_key1)));
    TRACE_MSG(TRACE_TRANSPORT3, "    key number %d, nwk key " TRACE_FORMAT_128,
            (FMT__H_A_A, key_number2, TRACE_ARG_128(nwk_key2)));
    TRACE_MSG(TRACE_TRANSPORT3, "    key number %d, nwk key " TRACE_FORMAT_128,
            (FMT__H_A_A, key_number3, TRACE_ARG_128(nwk_key3)));

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_nwk_keys_response", (FMT__0));
  }
  ncp_host_handle_get_nwk_keys_response(response->status_code,
                                        nwk_key1, key_number1,
                                        nwk_key2, key_number2,
                                        nwk_key3, key_number3);


  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_nwk_keys_response", (FMT__0));
}

static void handle_get_aps_key_by_ieee_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t aps_key[ZB_CCM_KEY_SIZE];
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);


  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_aps_key_by_ieee_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_aps_key_by_ieee_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u128key(&body, aps_key);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_aps_key_by_ieee_response, aps_key " TRACE_FORMAT_128,
              (FMT__A_A, TRACE_ARG_128(aps_key)));
  }
  ncp_host_handle_get_aps_key_by_ieee_response(error_code, aps_key);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_aps_key_by_ieee_response", (FMT__0));
}

static void handle_get_join_status_response(ncp_hl_response_header_t* response,
                                            zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t joined;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_join_status_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_join_status_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &joined);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_join_status_response, joined = %hd ", (FMT__H, joined));
  }
  ncp_host_handle_get_join_status_response(error_code, (zb_bool_t)joined);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_join_status_response", (FMT__0));
}

static void handle_get_authentication_status_response(ncp_hl_response_header_t* response,
                                                      zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t authenticated;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_authentication_status_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_authentication_status_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &authenticated);

    TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_authentication_status_response, authenticated = %hd ",
              (FMT__H, authenticated));
  }
  ncp_host_handle_get_authentication_status_response(error_code, (zb_bool_t)authenticated);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_authentication_status_response", (FMT__0));
}

static void handle_get_parent_address_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint16_t parent_address = 0xFFFF;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_parent_address_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u16(&body, &parent_address);

    TRACE_MSG(TRACE_TRANSPORT3, "parent address: 0x%x", (FMT__D, parent_address));
  }

  ncp_host_handle_get_parent_address_response(error_code, parent_address);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_parent_address_response", (FMT__0));
}

static void handle_get_extended_pan_id_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_ext_pan_id_t extended_pan_id;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_extended_pan_id_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u64addr(&body, extended_pan_id);

    TRACE_MSG(TRACE_TRANSPORT3, "ext pan id: " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(extended_pan_id)));
  }

  ncp_host_handle_get_extended_pan_id_response(error_code, extended_pan_id);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_extended_pan_id_response", (FMT__0));
}

static void handle_get_coordinator_version_response(ncp_hl_response_header_t* response,
                                                    zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t coordinator_version;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_coordinator_version_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u8(&body, &coordinator_version);

    TRACE_MSG(TRACE_TRANSPORT3, "coordinator version: %d", (FMT__D, coordinator_version));
  }

  ncp_host_handle_get_coordinator_version_response(error_code, coordinator_version);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_coordinator_version_response", (FMT__0));
}

static void handle_get_short_address_response(ncp_hl_response_header_t* response,
                                              zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint16_t short_address;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_short_address_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u16(&body, &short_address);

    TRACE_MSG(TRACE_TRANSPORT3, "short address: 0x%x", (FMT__D, short_address));
  }

  ncp_host_handle_get_short_address_response(error_code, short_address);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_short_address_response", (FMT__0));
}

static void handle_get_trust_center_address_response(ncp_hl_response_header_t* response,
                                                     zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_ieee_addr_t trust_center_address;
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_trust_center_address_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);
    ncp_host_hl_buf_get_u64addr(&body, trust_center_address);

    TRACE_MSG(TRACE_TRANSPORT3, "trust center address: " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(trust_center_address)));
  }

  ncp_host_handle_get_trust_center_address_response(error_code, trust_center_address);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_trust_center_address_response", (FMT__0));
}


static void handle_nvram_clear_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nvram_clear_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nvram_clear_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nvram_clear_response", (FMT__0));
}


static void handle_nvram_erase_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nvram_erase_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nvram_erase_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nvram_erase_response", (FMT__0));
}


static void handle_nvram_read_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;

  zb_uint8_t *data = NULL;
  zb_uint16_t data_len = 0;
  zb_uint16_t ds_type = 0;
  zb_uint16_t ds_version = 0;
  zb_uint16_t nvram_version = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nvram_read_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u16(&body, &nvram_version);
    ncp_host_hl_buf_get_u16(&body, &ds_type);
    ncp_host_hl_buf_get_u16(&body, &ds_version);
    ncp_host_hl_buf_get_u16(&body, &data_len);
    ncp_host_hl_buf_get_ptr(&body, &data, data_len);
  }

  ncp_host_handle_nvram_read_response(error_code, data, data_len,
                                      (zb_nvram_dataset_types_t)ds_type, ds_version, nvram_version);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nvram_read_response", (FMT__0));
}


static void handle_nvram_write_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_nvram_write_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nvram_write_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_nvram_write_response", (FMT__0));
}


static void handle_set_tc_policy_response(ncp_hl_response_header_t *response,
                                          zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_tc_policy_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_tc_policy_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_tc_policy_response", (FMT__0));
}


#ifdef ZB_LIMIT_VISIBILITY
static void handle_mac_add_visible_long_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_mac_add_visible_long_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_handle_mac_add_visible_long_response(error_code);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_mac_add_visible_long_response", (FMT__0));
}


static void handle_mac_add_invisible_short_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_mac_add_invisible_short_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_handle_mac_add_invisible_short_response(error_code);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_mac_add_invisible_short_response", (FMT__0));
}
#endif /* ZB_LIMIT_VISIBILITY */


void handle_get_serial_number_response(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t serial_number[ZB_NCP_SERIAL_NUMBER_LENGTH];

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_serial_number_response, status_code %d",
            (FMT__D, error_code));

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_array(&body, serial_number, ZB_NCP_SERIAL_NUMBER_LENGTH);
  }

  ncp_host_handle_get_serial_number_response(error_code, serial_number);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_serial_number_response", (FMT__0));
}


void handle_get_vendor_data(ncp_hl_response_header_t* response, zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t vendor_data_size;
  zb_uint8_t vendor_data[ZB_NCP_MAX_VENDOR_DATA_SIZE];

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_vendor_data, status_code %d",
            (FMT__D, error_code));

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u8(&body, &vendor_data_size);
    ncp_host_hl_buf_get_array(&body, vendor_data, vendor_data_size);
  }

  ncp_host_handle_get_vendor_data_response(error_code, vendor_data_size, vendor_data);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_vendor_data", (FMT__0));
}


void ncp_host_handle_configuration_response(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_configuration_response", (FMT__0));

  switch(response_header->hdr.call_id)
  {
    case NCP_HL_NCP_RESET:
      handle_reset_response(response_header, len);
      break;
    case NCP_HL_GET_MODULE_VERSION:
      handle_get_module_version_response(response_header, len);
      break;
    case NCP_HL_GET_LOCAL_IEEE_ADDR:
      handle_get_local_ieee_addr_response(response_header, len);
      break;
#if defined ZB_PRODUCTION_CONFIG && defined ZB_NCP_PRODUCTION_CONFIG_ON_HOST
    case NCP_HL_PRODUCTION_CONFIG_READ:
      handle_request_production_config_response(response_header, len);
      break;
#endif /* ZB_PRODUCTION_CONFIG && ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
    case NCP_HL_SET_LOCAL_IEEE_ADDR:
      handle_set_local_ieee_addr_response(response_header, len);
      break;
    case NCP_HL_GET_RX_ON_WHEN_IDLE:
      handle_get_rx_on_when_idle_response(response_header, len);
      break;
    case NCP_HL_SET_RX_ON_WHEN_IDLE:
      handle_set_rx_on_when_idle_response(response_header, len);
      break;
    case NCP_HL_GET_ED_TIMEOUT:
      handle_get_end_device_timeout_response(response_header, len);
      break;
    case NCP_HL_SET_ED_TIMEOUT:
      handle_set_end_device_timeout_response(response_header, len);
      break;
    case NCP_HL_SET_ZIGBEE_CHANNEL_MASK:
      handle_set_zigbee_channel_mask_response(response_header, len);
      break;
    case NCP_HL_GET_ZIGBEE_CHANNEL_MASK:
      handle_get_zigbee_channel_mask_response(response_header, len);
      break;
    case NCP_HL_GET_ZIGBEE_CHANNEL:
      handle_get_zigbee_channel_response(response_header, len);
      break;
    case NCP_HL_SET_PAN_ID:
      handle_set_zigbee_pan_id_response(response_header, len);
      break;
    case NCP_HL_GET_PAN_ID:
      handle_get_zigbee_pan_id_response(response_header, len);
      break;
    case NCP_HL_SET_EXTENDED_PAN_ID:
      handle_set_zigbee_extended_pan_id_response(response_header, len);
      break;
    case NCP_HL_SET_ZIGBEE_ROLE:
      handle_set_zigbee_role_response(response_header, len);
      break;
    case NCP_HL_GET_ZIGBEE_ROLE:
      handle_get_zigbee_role_response(response_header, len);
      break;
    case NCP_HL_SET_MAX_CHILDREN:
      handle_set_max_children_response(response_header, len);
      break;
    case NCP_HL_SET_NWK_KEY:
      handle_set_nwk_key_response(response_header, len);
      break;
    case NCP_HL_GET_NWK_KEYS:
      handle_get_nwk_keys_response(response_header, len);
      break;
    case NCP_HL_GET_APS_KEY_BY_IEEE:
      handle_get_aps_key_by_ieee_response(response_header, len);
      break;
    case NCP_HL_GET_JOINED:
      handle_get_join_status_response(response_header, len);
      break;
    case NCP_HL_GET_AUTHENTICATED:
      handle_get_authentication_status_response(response_header, len);
      break;
    case NCP_HL_GET_PARENT_ADDRESS:
      handle_get_parent_address_response(response_header, len);
      break;
    case NCP_HL_GET_EXTENDED_PAN_ID:
      handle_get_extended_pan_id_response(response_header, len);
      break;
    case NCP_HL_GET_COORDINATOR_VERSION:
      handle_get_coordinator_version_response(response_header, len);
      break;
    case NCP_HL_GET_SHORT_ADDRESS:
      handle_get_short_address_response(response_header, len);
      break;
    case NCP_HL_GET_TRUST_CENTER_ADDRESS:
      handle_get_trust_center_address_response(response_header, len);
      break;
    case NCP_HL_NVRAM_CLEAR:
      handle_nvram_clear_response(response_header, len);
      break;
    case NCP_HL_NVRAM_ERASE:
      handle_nvram_erase_response(response_header, len);
      break;
    case NCP_HL_NVRAM_READ:
      handle_nvram_read_response(response_header, len);
      break;
    case NCP_HL_NVRAM_WRITE:
      handle_nvram_write_response(response_header, len);
      break;
    case NCP_HL_SET_TC_POLICY:
      handle_set_tc_policy_response(response_header, len);
      break;
#ifdef ZB_LIMIT_VISIBILITY
    case NCP_HL_ADD_INVISIBLE_SHORT:
      handle_mac_add_invisible_short_response(response_header, len);
      break;
    case NCP_HL_ADD_VISIBLE_DEV:
      handle_mac_add_visible_long_response(response_header, len);
      break;
#endif /* ZB_LIMIT_VISIBILITY */
    case NCP_HL_GET_SERIAL_NUMBER:
      handle_get_serial_number_response(response_header, len);
      break;
    case NCP_HL_GET_VENDOR_DATA:
      handle_get_vendor_data(response_header, len);
      break;
    default:
      ZB_ASSERT(0);
    }

   TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_configuration_response", (FMT__0));
}

static void handle_reset_indication(ncp_hl_ind_header_t* indication, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t reset_src;

  TRACE_MSG(TRACE_TRANSPORT3,">> handle_reset_indication", (FMT__0));

  ncp_host_hl_init_indication_body(indication, len, &body);
  ncp_host_hl_buf_get_u8(&body, &reset_src);

  TRACE_MSG(TRACE_TRANSPORT3,"  reset_src 0x%hx", (FMT__H, reset_src));

  ncp_host_handle_reset_indication(reset_src);

  TRACE_MSG(TRACE_TRANSPORT3,"<< handle_reset_indication", (FMT__0));
}

void ncp_host_handle_configuration_indication(void* data, zb_uint16_t len)
{
  ncp_hl_ind_header_t* indication_header = (ncp_hl_ind_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_configuration_indication", (FMT__0));

  switch(indication_header->call_id)
  {
    case NCP_HL_NCP_RESET_IND:
      handle_reset_indication(indication_header, len);
      break;
    default:
      ZB_ASSERT(0);
    }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_configuration_indication", (FMT__0));
}

static zb_ret_t ncp_host_reset_general(zb_uint8_t options)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_reset_general", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NCP_RESET, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, options);
    ret = ncp_host_hl_send_packet(&body);

    if (ret == RET_OK)
    {
      /* Set pending status for the sent reset request until confirm receiving */
      g_ncp_host_proto_config_ctx.is_reset_request_pending = ZB_TRUE;
    }
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_reset_general, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_reset(void)
{
  zb_ret_t ret = RET_BUSY;
  zb_uint8_t options = NO_OPTIONS;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_reset", (FMT__0));

  ret = ncp_host_reset_general(options);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_reset, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_reset_nvram_erase(void)
{
  zb_ret_t ret = RET_BUSY;
  zb_uint8_t options = NVRAM_ERASE;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_reset_nvram_erase", (FMT__0));

  ret = ncp_host_reset_general(options);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_reset_nvram_erase, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_factory_reset(void)
{
  zb_ret_t ret = RET_BUSY;
  zb_uint8_t options = FACTORY_RESET;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_factory_reset", (FMT__0));

  ret = ncp_host_reset_general(options);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_factory_reset, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_module_version(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_module_version", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_MODULE_VERSION, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_module_version, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_get_local_ieee_addr(zb_uint8_t interface_num)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_local_ieee_addr", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_LOCAL_IEEE_ADDR, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, interface_num);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_local_ieee_addr, ret %d", (FMT__D, ret));

  return ret;
}

#if defined ZB_PRODUCTION_CONFIG && defined ZB_NCP_PRODUCTION_CONFIG_ON_HOST
zb_ret_t ncp_host_request_production_config(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_request_production_config", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_PRODUCTION_CONFIG_READ, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_request_production_config, ret %d", (FMT__D, ret));

  return ret;
}
#endif /* ZB_PRODUCTION_CONFIG && ZB_NCP_PRODUCTION_CONFIG_ON_HOST */


zb_ret_t ncp_host_set_local_ieee_addr(zb_uint8_t interface_num, const zb_ieee_addr_t addr)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_local_ieee_addr", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_LOCAL_IEEE_ADDR, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, interface_num);
    ncp_host_hl_buf_put_u64addr(&body, addr);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_local_ieee_addr, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_get_rx_on_when_idle(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_rx_on_when_idle", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_RX_ON_WHEN_IDLE, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_rx_on_when_idle, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_rx_on_when_idle(zb_bool_t rx_on_when_idle)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_rx_on_when_idle", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_RX_ON_WHEN_IDLE, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, rx_on_when_idle);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_rx_on_when_idle, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_end_device_timeout(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_end_device_timeout", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ED_TIMEOUT, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_end_device_timeout, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_end_device_timeout(zb_uint8_t timeout)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_end_device_timeout", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_ED_TIMEOUT, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, timeout);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_end_device_timeout, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_zigbee_channel_mask(zb_uint8_t page, zb_uint32_t channel_mask)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,
            ">> ncp_host_set_zigbee_channel_mask, page 0x%x, channel_mask 0x%x",
            (FMT__D_D, page, channel_mask));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_ZIGBEE_CHANNEL_MASK, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, page);
    ncp_host_hl_buf_put_u32(&body, channel_mask);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_zigbee_channel_mask, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_zigbee_channel_mask(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_zigbee_channel_mask", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_CHANNEL_MASK, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_zigbee_channel_mask, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_zigbee_channel(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_zigbee_channel", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_CHANNEL, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_zigbee_channel, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_set_zigbee_pan_id(zb_uint16_t pan_id)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,
            ">> ncp_host_set_zigbee_pan_id, pan_id 0x%x",
            (FMT__D, pan_id));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_PAN_ID, &body, NULL))
  {
    ncp_host_hl_buf_put_u16(&body, pan_id);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_zigbee_pan_id, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_zigbee_pan_id(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_zigbee_pan_id ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_PAN_ID, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_zigbee_pan_id, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_zigbee_extended_pan_id(const zb_ext_pan_id_t extended_pan_id)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_zigbee_extended_pan_id, ext_pan_id " TRACE_FORMAT_64,
    (FMT__A, TRACE_ARG_64(extended_pan_id)));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_EXTENDED_PAN_ID, &body, NULL))
  {
    ncp_host_hl_buf_put_u64addr(&body, extended_pan_id);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_zigbee_extended_pan_id, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_zigbee_role(zb_uint8_t zigbee_role)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,
            ">> ncp_host_set_zigbee_role, zigbee_role %d ",
            (FMT__D, zigbee_role));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_ZIGBEE_ROLE, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, zigbee_role);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_zigbee_role, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_max_children(zb_uint8_t max_children)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_max_children, zigbee_role %hd", (FMT__H, max_children));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_MAX_CHILDREN, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, max_children);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_max_children, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_get_zigbee_role(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,">> ncp_host_get_zigbee_role ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_ROLE, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_zigbee_role, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_set_nwk_key(zb_uint8_t *nwk_key, zb_uint8_t key_number)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,
            ">> ncp_host_set_nwk_key, key number %d, nwk key " TRACE_FORMAT_128,
            (FMT__H_A_A, key_number, TRACE_ARG_128(nwk_key)));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_NWK_KEY, &body, NULL))
  {
    ncp_host_hl_buf_put_u128key(&body, nwk_key);
    ncp_host_hl_buf_put_u8(&body, key_number);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_nwk_key, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_nwk_keys(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_nwk_keys", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_NWK_KEYS, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_nwk_keys, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_aps_key_by_ieee(zb_ieee_addr_t ieee_addr)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2,
            ">> ncp_host_get_aps_key_by_ieee, ieee_addr " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ieee_addr)));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_APS_KEY_BY_IEEE, &body, NULL))
  {
    ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_aps_key_by_ieee, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_get_join_status(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_join_status ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_JOINED, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_join_status ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_authentication_status(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_authentication_status ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_AUTHENTICATED, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_authentication_status ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_parent_address(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_parent_address ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_PARENT_ADDRESS, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_parent_address ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_extended_pan_id(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_extended_pan_id ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_EXTENDED_PAN_ID, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_extended_pan_id ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_coordinator_version(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_coordinator_version ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_COORDINATOR_VERSION, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_coordinator_version ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_short_address(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_short_address ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_SHORT_ADDRESS, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_short_address ", (FMT__0));

  return ret;
}

zb_ret_t ncp_host_get_trust_center_address(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_trust_center_address ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_TRUST_CENTER_ADDRESS, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_trust_center_address ", (FMT__0));

  return ret;
}


static zb_size_t put_nvram_write_header(ncp_host_hl_tx_buf_handle_t *body,
                                        ncp_hl_nvram_write_req_hdr_t *request)
{
  zb_size_t bytes_written = 0;

  ncp_host_hl_buf_put_u8(body, request->dataset_qnt);
  bytes_written += 1;

  ZB_ASSERT(bytes_written == sizeof(ncp_hl_nvram_write_req_hdr_t));
  return bytes_written;
}


static zb_size_t put_nvram_write_dataset(ncp_host_hl_tx_buf_handle_t *body,
                                         ncp_hl_nvram_write_req_ds_hdr_t *request)
{
  zb_size_t bytes_written = 0;
  zb_uint16_t dataset_len = request->len;

  ncp_host_hl_buf_put_u16(body, request->type);
  bytes_written += 2;

  ncp_host_hl_buf_put_u16(body, request->version);
  bytes_written += 2;

  ncp_host_hl_buf_put_u16(body, request->len);
  bytes_written += 2;

  ncp_host_hl_buf_put_array(body, (zb_uint8_t*)(request + 1), dataset_len);
  bytes_written += dataset_len;

  ZB_ASSERT(bytes_written == sizeof(*request) + dataset_len);
  return bytes_written;
}


zb_ret_t ncp_host_nvram_write_request(zb_uint8_t* request, zb_size_t len)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  ZVUNUSED(len);
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nvram_write_request, len %d", (FMT__D, len));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NVRAM_WRITE, &body, NULL))
  {
    ncp_hl_nvram_write_req_hdr_t *common_hdr;
    zb_uint8_t dataset_qnt;

    common_hdr = (ncp_hl_nvram_write_req_hdr_t*)request;
    request += put_nvram_write_header(&body, common_hdr);

    for (dataset_qnt = common_hdr->dataset_qnt; dataset_qnt > 0; dataset_qnt--)
    {
      ncp_hl_nvram_write_req_ds_hdr_t *ds_hdr;

      ds_hdr = (ncp_hl_nvram_write_req_ds_hdr_t*)request;
      request += put_nvram_write_dataset(&body, ds_hdr);
    }

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nvram_write_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_nvram_read_request(zb_uint16_t dataset_type)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nvram_read_request, ds %d", (FMT__D, dataset_type));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NVRAM_READ, &body, NULL))
  {
    ncp_host_hl_buf_put_u16(&body, dataset_type);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nvram_read_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_nvram_erase_request(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nvram_erase_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NVRAM_ERASE, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nvram_erase_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_nvram_clear_request(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nvram_clear_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_NVRAM_CLEAR, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nvram_clear_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_set_tc_policy(zb_uint16_t policy_type, zb_uint8_t value)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_set_tc_policy ", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_TC_POLICY, &body, NULL))
  {
    ncp_host_hl_buf_put_u16(&body, policy_type);
    ncp_host_hl_buf_put_u8(&body, value);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_tc_policy ", (FMT__0));

  return ret;
}

#if defined ZB_ENABLE_ZGP

zb_ret_t ncp_host_disable_gppb(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_disable_gppb", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_DISABLE_GPPB, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_disable_gppb", (FMT__0));

  return ret;
}


zb_ret_t ncp_host_gp_set_shared_key_type(zb_uint8_t gp_security_key_type)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_gp_set_shared_key_type, %d",
            (FMT__D, gp_security_key_type));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GP_SET_SHARED_KEY_TYPE, &body, NULL))
  {
    ncp_host_hl_buf_put_u8(&body, gp_security_key_type);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_gp_set_shared_key_type", (FMT__0));

  return ret;
}


zb_ret_t ncp_host_gp_set_default_link_key(zb_uint8_t* gp_link_key)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_gp_set_default_link_key " TRACE_FORMAT_128,
            (FMT__A_A, TRACE_ARG_128(gp_link_key)));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GP_SET_DEFAULT_LINK_KEY, &body, NULL))
  {
    ncp_host_hl_buf_put_u128key(&body, gp_link_key);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_gp_set_default_link_key", (FMT__0));

  return ret;
}

#endif /* ZB_ENABLE_ZGP */


#ifdef ZB_LIMIT_VISIBILITY
zb_ret_t ncp_host_mac_add_invisible_short(zb_uint16_t invisible_address)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_mac_add_invisible_short", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_ADD_INVISIBLE_SHORT, &body, NULL))
  {
    ncp_host_hl_buf_put_u16(&body, invisible_address);

    TRACE_MSG(TRACE_TRANSPORT2, "  invisible_address 0x%x", (FMT__D, invisible_address));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_mac_add_invisible_short, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_mac_add_visible_long(const zb_ieee_addr_t long_address)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_mac_add_visible_long", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_ADD_VISIBLE_DEV, &body, NULL))
  {
    ncp_host_hl_buf_put_u64addr(&body, long_address);

    TRACE_MSG(TRACE_TRANSPORT2, "  long_address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(long_address)));

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_mac_add_visible_long, ret %d", (FMT__D, ret));

  return ret;
}
#endif /* ZB_LIMIT_VISIBILITY */


zb_ret_t ncp_host_get_serial_number(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_serial_number", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_SERIAL_NUMBER, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_serial_number, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_get_vendor_data(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_vendor_data", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_VENDOR_DATA, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_vendor_data, ret %d", (FMT__D, ret));

  return ret;
}
