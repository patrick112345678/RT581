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
/*  PURPOSE: NCP High level transport (adaptors layer) implementation for the host side: APS category
*/
#define ZB_TRACE_FILE_ID 4

#include "ncp_host_hl_adaptor.h"

void adaptor_handle_apsde_data_indication(zb_apsde_data_indication_t *ind,
                                          zb_uint8_t *data_ptr,
                                          zb_uint8_t params_len, zb_uint16_t data_len)
{
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);
  zb_uint8_t i;
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> adaptor_handle_apsde_data_indication ", (FMT__0));

  ptr = (zb_uint8_t *)zb_buf_initial_alloc(buf, sizeof(zb_uint8_t) + sizeof(zb_uint16_t) + data_len);

  ZB_MEMCPY(ptr, &params_len, sizeof(zb_uint8_t));
  ptr = ptr + sizeof(zb_uint8_t);
  ZB_MEMCPY(ptr, &data_len, sizeof(zb_uint16_t));
  ptr = ptr + sizeof(zb_uint16_t);
  ZB_MEMCPY(ptr, data_ptr, data_len);

  ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_apsde_data_indication_t), ind, sizeof(zb_apsde_data_indication_t));

  ncp_host_handle_apsde_data_indication(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< adaptor_handle_apsde_data_indication", (FMT__0));
}

void adaptor_handle_apsde_data_request_response(zb_apsde_data_confirm_t *conf)
{
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

  TRACE_MSG(TRACE_TRANSPORT3, ">> adaptor_handle_apsde_data_request_response ", (FMT__0));

  ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_apsde_data_confirm_t), conf, sizeof(zb_apsde_data_confirm_t));

  ncp_host_handle_apsde_data_request_response(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< adaptor_handle_apsde_data_request_response ", (FMT__0));
}


zb_ret_t ncp_host_apsde_data_request(zb_bufid_t buf)
{
  zb_apsde_data_req_t *req = ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t);
  zb_uint8_t param_len;
  zb_uint16_t data_len;
  zb_uint8_t *data_ptr;

  zb_uint8_t tsn;
  zb_ret_t ret = RET_BUSY;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_apsde_data_request", (FMT__0));

  data_ptr = (zb_uint8_t *)zb_buf_begin(buf);
  param_len = *data_ptr;
  data_ptr = data_ptr + 1;
  data_len = *(zb_uint16_t *)data_ptr;
  data_ptr = data_ptr + 2;

  ret = ncp_host_apsde_data_request_transport(req, param_len, data_len, data_ptr, &tsn);

  zb_buf_free(buf);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_apsde_data_request, ret %d", (FMT__D, ret));
}
