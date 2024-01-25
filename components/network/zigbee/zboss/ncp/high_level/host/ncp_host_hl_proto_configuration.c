/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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

static void handle_reset_response(ncp_hl_response_header_t *response, zb_uint16_t len)
{
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_reset_response, status_code: %d", (FMT__D, error_code));
    ZVUNUSED(len);

    if (response->tsn != 0xFF)
    {
        ncp_host_mark_blocking_request_finished();
    }

    ncp_host_handle_reset_response(error_code);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_reset_response", (FMT__0));
}


static void handle_get_module_version_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint32_t version = 0;
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_module_version_response, status_code %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();

    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_module_version_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u32(&body, &version);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_module_version_response, version 0x%x",
                  (FMT__D, version));
    }

    ncp_host_handle_get_module_version_response(error_code, version);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_module_version_response", (FMT__0));
}


static void handle_get_local_ieee_addr_response(ncp_hl_response_header_t *response,
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


static void handle_set_zigbee_channel_mask_response(ncp_hl_response_header_t *response,
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

static void handle_get_zigbee_channel_mask_response(ncp_hl_response_header_t *response,
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

static void handle_get_zigbee_channel_response(ncp_hl_response_header_t *response,
        zb_uint16_t len)
{
    ncp_host_hl_rx_buf_handle_t body;
    zb_uint8_t channel;
    zb_uint8_t page = 0;
    zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);

    TRACE_MSG(TRACE_TRANSPORT3, ">> handle_get_zigbee_channel_response, status_code: %d",
              (FMT__D, error_code));

    ncp_host_mark_blocking_request_finished();
    if (error_code == RET_OK)
    {
        TRACE_MSG(TRACE_TRANSPORT3, ">> parse handle_get_zigbee_channel_response", (FMT__0));

        ncp_host_hl_init_response_body(response, len, &body);
        ncp_host_hl_buf_get_u8(&body, &channel);

        TRACE_MSG(TRACE_TRANSPORT3, "<< parse handle_get_zigbee_channel_response, page %hd, channel %hd",
                  (FMT__H_H, page, channel));
    }
    ncp_host_handle_get_zigbee_channel_response(error_code, page, channel);

    TRACE_MSG(TRACE_TRANSPORT3, "<< handle_get_zigbee_channel_response", (FMT__0));
}

static void handle_set_zigbee_pan_id_response(ncp_hl_response_header_t *response,
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

static void handle_get_zigbee_pan_id_response(ncp_hl_response_header_t *response,
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

static void handle_set_zigbee_role_response(ncp_hl_response_header_t *response,
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

static void handle_get_zigbee_role_response(ncp_hl_response_header_t *response,
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

static void handle_set_nwk_key_response(ncp_hl_response_header_t *response,
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

static void handle_get_nwk_keys_response(ncp_hl_response_header_t *response, zb_uint16_t len)
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

static void handle_get_aps_key_by_ieee_response(ncp_hl_response_header_t *response,
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

void ncp_host_handle_configuration_response(void *data, zb_uint16_t len)
{
    ncp_hl_response_header_t *response_header = (ncp_hl_response_header_t *)data;

    TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_configuration_response", (FMT__0));

    switch (response_header->hdr.call_id)
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
    case NCP_HL_SET_ZIGBEE_ROLE:
        handle_set_zigbee_role_response(response_header, len);
        break;
    case NCP_HL_GET_ZIGBEE_ROLE:
        handle_get_zigbee_role_response(response_header, len);
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
    default:
        /* TODO: implement handlers for other responses! */
        ZB_ASSERT(0);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_configuration_response", (FMT__0));
}

void ncp_host_handle_configuration_indication(void *data, zb_uint16_t len)
{
    /* TODO: implement it! */
    ZB_ASSERT(0);
}

zb_ret_t ncp_host_get_module_version(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_module_version", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_MODULE_VERSION, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_LOCAL_IEEE_ADDR, &body))
    {
        ncp_host_hl_buf_put_u8(&body, interface_num);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_local_ieee_addr, ret %d", (FMT__D, ret));

    return ret;
}


zb_ret_t ncp_host_set_zigbee_channel_mask(zb_uint8_t page, zb_uint32_t channel_mask)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2,
              ">> ncp_host_set_zigbee_channel_mask, page 0x%x, channel_mask 0x%x",
              (FMT__D_D, page, channel_mask));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_ZIGBEE_CHANNEL_MASK, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_CHANNEL_MASK, &body))
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

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_zigbe_channel", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_CHANNEL, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_PAN_ID, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_PAN_ID, &body))
    {
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_zigbee_pan_id, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_set_zigbee_role(zb_uint8_t zigbee_role)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2,
              ">> ncp_host_set_zigbee_role, zigbee_role %d ",
              (FMT__D, zigbee_role));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_ZIGBEE_ROLE, &body))
    {
        ncp_host_hl_buf_put_u8(&body, zigbee_role);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_set_zigbee_role, ret %d", (FMT__D, ret));

    return ret;
}

zb_ret_t ncp_host_get_zigbee_role(void)
{
    zb_ret_t ret = RET_BUSY;
    ncp_host_hl_tx_buf_handle_t body;

    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_get_zigbee_role ", (FMT__0));

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_ZIGBEE_ROLE, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SET_NWK_KEY, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_NWK_KEYS, &body))
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

    if (!ncp_host_get_buf_for_blocking_request(NCP_HL_GET_APS_KEY_BY_IEEE, &body))
    {
        ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
        ret = ncp_host_hl_send_packet(&body);
    }

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_get_aps_key_by_ieee, ret %d", (FMT__D, ret));

    return ret;
}
