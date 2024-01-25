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
/*  PURPOSE: NCP High level transport implementation for the host side: SECUR category
*/

#define ZB_TRACE_FILE_ID 18

#include "ncp_host_hl_proto.h"

static void handle_secur_add_ic_response(ncp_hl_response_header_t* response,
                                         zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_ic_add_response, status_code %d",
            (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_secur_add_ic_response(error_code, response->tsn);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_ic_add_response", (FMT__0));
}


static void handle_secur_set_local_ic_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_set_local_ic_response, status_code %d", (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_set_local_ic_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_set_local_ic_response", (FMT__0));
}


static void handle_secur_get_local_ic_response(ncp_hl_response_header_t* response,
                                               zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t installcode_len;
  zb_uint8_t installcode_type;
  zb_uint8_t installcode[ZB_CCM_KEY_SIZE+ZB_CCM_KEY_CRC_SIZE];

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_get_local_ic_response, status_code %d, len %d",
    (FMT__D_D, error_code, len));

  ZB_BZERO(&installcode, sizeof(installcode));

  if (error_code == RET_OK)
  {
    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u8(&body, &installcode_len);
    ncp_host_hl_buf_get_array(&body, installcode, installcode_len);

    TRACE_MSG(TRACE_TRANSPORT3, "  ic_len: %hd", (FMT__H, installcode_len));
    TRACE_MSG(TRACE_TRANSPORT3, "  ic: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(installcode)));

    installcode_type = 0;

    while (installcode_type != ZB_IC_TYPE_MAX)
    {
      if (ZB_IC_SIZE_BY_TYPE(installcode_type) + ZB_CCM_KEY_CRC_SIZE == installcode_len)
      {
        break;
      }

      installcode_type++;
    }

    ZB_ASSERT(installcode_type != ZB_IC_TYPE_MAX);
  }

  ncp_host_handle_secur_get_local_ic_response(error_code, installcode_type, installcode);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_get_local_ic_response", (FMT__0));
}


static void handle_secur_nwk_initiate_key_switch_procedure_response(ncp_hl_response_header_t* response,
                                                                    zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_nwk_initiate_key_switch_procedure_response, status_code %d", (FMT__D, error_code));

  ncp_host_mark_blocking_request_finished();

  ncp_host_handle_nwk_initiate_key_switch_procedure_response(error_code);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_nwk_initiate_key_switch_procedure_response", (FMT__0));
}


static void handle_secur_get_ic_list_response(ncp_hl_response_header_t* response,
                                              zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_uint8_t ncp_tsn = response->tsn;
  ncp_host_hl_rx_buf_handle_t body;

  zb_uint8_t ic_table_entries;
  zb_uint8_t start_index;
  zb_uint8_t ic_table_list_count;

  ncp_host_secur_installcode_record_t ic_table[ZB_APS_PAYLOAD_MAX_LEN / sizeof(ncp_host_secur_installcode_record_t)];
  ncp_host_secur_installcode_record_t *resp_record;
  zb_uindex_t table_entry_index;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_get_ic_list_response, status_code %d, len %d",
            (FMT__D_D, error_code, len));

  ZB_BZERO(ic_table, sizeof(ic_table));

  if (error_code == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "  >> parse handle_secur_get_ic_list_response", (FMT__0));

    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u8(&body, &ic_table_entries);
    TRACE_MSG(TRACE_TRANSPORT3, "    >> ic_table_entries = %hd ", (FMT__H, ic_table_entries));

    ncp_host_hl_buf_get_u8(&body, &start_index);
    TRACE_MSG(TRACE_TRANSPORT3, "    >> start_index = %hd ", (FMT__H, start_index));

    ncp_host_hl_buf_get_u8(&body, &ic_table_list_count);
    TRACE_MSG(TRACE_TRANSPORT3, "    >> ic_table_list_count = %hd ", (FMT__H, ic_table_list_count));

    for (table_entry_index = 0; table_entry_index < ic_table_list_count; table_entry_index++)
    {
      resp_record = &ic_table[table_entry_index];

      ncp_host_hl_buf_get_u64addr(&body, resp_record->device_address);
      ncp_host_hl_buf_get_u8(&body, &resp_record->options);

      ncp_host_hl_buf_get_array(&body, resp_record->installcode, ZB_IC_SIZE_BY_TYPE(resp_record->options));

      TRACE_MSG(TRACE_TRANSPORT3, "    >> ic_entry #%hd ", (FMT__H, table_entry_index));

      TRACE_MSG(TRACE_TRANSPORT3, "      device_address: " TRACE_FORMAT_64,
        (FMT__A, TRACE_ARG_64(resp_record->device_address)));
      TRACE_MSG(TRACE_TRANSPORT3, "      options: %hx", (FMT__H, resp_record->options));
      TRACE_MSG(TRACE_TRANSPORT3, "      ic: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(resp_record->installcode)));

      TRACE_MSG(TRACE_TRANSPORT3, "    << ic_entry", (FMT__0));
    }

    TRACE_MSG(TRACE_TRANSPORT3, "  << parse handle_secur_get_ic_list_response", (FMT__0));
  }

  ncp_host_handle_secur_get_ic_list_response(error_code, ncp_tsn,
    ic_table_entries, start_index, ic_table_list_count, ic_table);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_get_ic_list_response", (FMT__0));
}


static void handle_secur_get_ic_by_idx_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_uint8_t ncp_tsn = response->tsn;
  ncp_host_hl_rx_buf_handle_t body;

  ncp_host_secur_installcode_record_t ic_record;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_get_ic_by_idx_response, status_code %d, len %d",
            (FMT__D_D, error_code, len));

  ZB_BZERO(&ic_record, sizeof(ic_record));

  if (error_code == RET_OK)
  {

    ncp_host_hl_init_response_body(response, len, &body);

    ncp_host_hl_buf_get_u64addr(&body, ic_record.device_address);
    ncp_host_hl_buf_get_u8(&body, &ic_record.options);

    ncp_host_hl_buf_get_array(&body, ic_record.installcode, ZB_IC_SIZE_BY_TYPE(ic_record.options));

    TRACE_MSG(TRACE_TRANSPORT3, "      device_address: " TRACE_FORMAT_64,
      (FMT__A, TRACE_ARG_64(ic_record.device_address)));
    TRACE_MSG(TRACE_TRANSPORT3, "      options: %hx",
      (FMT__H, ic_record.options));
    TRACE_MSG(TRACE_TRANSPORT3, "      ic: " TRACE_FORMAT_128,
      (FMT__B, TRACE_ARG_128(ic_record.installcode)));
  }

  ncp_host_handle_secur_get_ic_by_idx_response(error_code, ncp_tsn, &ic_record);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_get_ic_by_idx_response", (FMT__0));
}


static void handle_secur_remove_ic_response(ncp_hl_response_header_t* response,
                                            zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_uint8_t ncp_tsn = response->tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_remove_ic_response, status_code %d, len %d",
            (FMT__D_D, error_code, len));

  ncp_host_handle_secur_remove_ic_response(error_code, ncp_tsn);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_remove_ic_response", (FMT__0));
}


static void handle_secur_remove_all_ic_response(ncp_hl_response_header_t* response,
                                                zb_uint16_t len)
{
  zb_ret_t error_code = ERROR_CODE(response->status_category, response->status_code);
  zb_uint8_t ncp_tsn = response->tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_remove_all_ic_response, status_code %d, len %d",
            (FMT__D_D, error_code, len));

  ncp_host_handle_secur_remove_all_ic_response(error_code, ncp_tsn);

  ncp_host_mark_blocking_request_finished();

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_remove_all_ic_response", (FMT__0));
}


void ncp_host_handle_secur_response(void* data, zb_uint16_t len)
{
  ncp_hl_response_header_t* response_header = (ncp_hl_response_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_response", (FMT__0));

  switch(response_header->hdr.call_id)
  {
    case NCP_HL_SECUR_ADD_IC:
      handle_secur_add_ic_response(response_header, len);
      break;
    case NCP_HL_SECUR_SET_LOCAL_IC:
      handle_secur_set_local_ic_response(response_header, len);
      break;
    case NCP_HL_SECUR_GET_LOCAL_IC:
      handle_secur_get_local_ic_response(response_header, len);
      break;
    case NCP_HL_SECUR_NWK_INITIATE_KEY_SWITCH_PROCEDURE:
      handle_secur_nwk_initiate_key_switch_procedure_response(response_header, len);
      break;
    case NCP_HL_SECUR_GET_IC_LIST:
      handle_secur_get_ic_list_response(response_header, len);
      break;
    case NCP_HL_SECUR_GET_IC_BY_IDX:
      handle_secur_get_ic_by_idx_response(response_header, len);
      break;
    case NCP_HL_SECUR_DEL_IC:
      handle_secur_remove_ic_response(response_header, len);
      break;
    case NCP_HL_SECUR_REMOVE_ALL_IC:
      handle_secur_remove_all_ic_response(response_header, len);
      break;
    default:
      ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_response", (FMT__0));
}


static void handle_secur_tclk_indication(ncp_hl_ind_header_t* indication, zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_ieee_addr_t trust_center_address;
  zb_uint8_t key_type;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_tclk_indication", (FMT__0));

  ncp_host_hl_init_indication_body(indication, len, &body);

  ncp_host_hl_buf_get_u64addr(&body, trust_center_address);
  ncp_host_hl_buf_get_u8(&body, &key_type);

  TRACE_MSG(TRACE_TRANSPORT3, "trust_center_address" TRACE_FORMAT_64 " key_type: 0x%x",
            (FMT__A_D, TRACE_ARG_64(trust_center_address), key_type));

  ncp_host_handle_secur_tclk_indication(RET_OK, trust_center_address, key_type);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_tclk_indication", (FMT__0));
}


static void handle_secur_tclk_exchange_failed_indication(ncp_hl_ind_header_t* indication,
                                                         zb_uint16_t len)
{
  ncp_host_hl_rx_buf_handle_t body;
  zb_uint8_t status_category;
  zb_uint8_t status_code;

  TRACE_MSG(TRACE_TRANSPORT3, ">> handle_secur_tclk_exchange_failed_indication", (FMT__0));

  ncp_host_hl_init_indication_body(indication, len, &body);

  ncp_host_hl_buf_get_u8(&body, &status_category);
  ncp_host_hl_buf_get_u8(&body, &status_code);

  ncp_host_handle_secur_tclk_indication(ERROR_CODE(status_category, status_code), NULL, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< handle_secur_tclk_exchange_failed_indication", (FMT__0));
}


void ncp_host_handle_secur_indication(void* data, zb_uint16_t len)
{
  ncp_hl_ind_header_t* indication_header = (ncp_hl_ind_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_secur_indication", (FMT__0));

  switch(indication_header->call_id)
  {
    case NCP_HL_SECUR_TCLK_IND:
      handle_secur_tclk_indication(indication_header, len);
      break;
    case NCP_HL_SECUR_TCLK_EXCHANGE_FAILED_IND:
      handle_secur_tclk_exchange_failed_indication(indication_header, len);
      break;
    default:
      ZB_ASSERT(0);
    }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_secur_indication", (FMT__0));
}


zb_ret_t ncp_host_secur_add_ic(zb_ieee_addr_t address, zb_uint8_t ic_type, zb_uint8_t *ic, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_add_ic, address " TRACE_FORMAT_64 ", ic_type %d ",
            (FMT__A_D, TRACE_ARG_64(address), ic_type));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_ADD_IC, &body, tsn))
  {
    ncp_host_hl_buf_put_u64addr(&body, address);
    ncp_host_hl_buf_put_array(&body, ic, ZB_IC_SIZE_BY_TYPE(ic_type) + 2);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_add_ic, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_set_local_ic(zb_uint8_t ic_type, zb_uint8_t *ic)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_set_local_ic, type %hd", (FMT__H, ic_type));

  TRACE_MSG(TRACE_SECUR1, "  ic: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_SET_LOCAL_IC, &body, NULL))
  {
    /* installcode size + 2 trailing bytes of CRC */
    ncp_host_hl_buf_put_array(&body, ic, ZB_IC_SIZE_BY_TYPE(ic_type) + 2);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_set_local_ic, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_get_local_ic(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_get_local_ic", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_GET_LOCAL_IC, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_get_local_ic, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_nwk_initiate_key_switch_procedure(void)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_nwk_initiate_key_switch_procedure", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_NWK_INITIATE_KEY_SWITCH_PROCEDURE, &body, NULL))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_nwk_initiate_key_switch_procedure, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_get_ic_list_request(zb_uint8_t start_index, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_get_ic_list_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_GET_IC_LIST, &body, tsn))
  {
    ncp_host_hl_buf_put_u8(&body, start_index);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_get_ic_list_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_get_ic_by_idx_request(zb_uint8_t ic_index, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_get_ic_by_idx_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_GET_IC_BY_IDX, &body, tsn))
  {
    ncp_host_hl_buf_put_u8(&body, ic_index);

    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_get_ic_by_idx_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_remove_ic_request(zb_ieee_addr_t ieee_addr, zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_remove_ic_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_DEL_IC, &body, tsn))
  {
    ncp_host_hl_buf_put_u64addr(&body, ieee_addr);
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_remove_ic_request, ret %d", (FMT__D, ret));

  return ret;
}


zb_ret_t ncp_host_secur_remove_all_ic_request(zb_uint8_t *tsn)
{
  zb_ret_t ret = RET_BUSY;
  ncp_host_hl_tx_buf_handle_t body;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_secur_remove_all_ic_request", (FMT__0));

  if (!ncp_host_get_buf_for_blocking_request(NCP_HL_SECUR_REMOVE_ALL_IC, &body, tsn))
  {
    ret = ncp_host_hl_send_packet(&body);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_secur_remove_all_ic_request, ret %d", (FMT__D, ret));

  return ret;
}
