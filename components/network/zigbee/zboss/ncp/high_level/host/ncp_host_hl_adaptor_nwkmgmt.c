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
#define ZB_TRACE_FILE_ID 3

#include "ncp_host_hl_adaptor.h"

zb_ret_t ncp_host_nwk_formation(zb_bufid_t buf)
{
  zb_ret_t ret = RET_BUSY;
  zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_formation", (FMT__0));

  ret = ncp_host_nwk_formation_transport(req);
  zb_buf_free(buf);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_formation, ret %d", (FMT__D, ret));

  return ret;
}

zb_ret_t ncp_host_nwk_permit_joining(zb_bufid_t buf)
{
  zb_ret_t ret = RET_BUSY;
  zb_nlme_permit_joining_request_t *req = ZB_BUF_GET_PARAM(buf, zb_nlme_permit_joining_request_t);

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_nwk_permit_joining", (FMT__0));

  ret = ncp_host_nwk_permit_joining_transport(req);
  zb_buf_free(buf);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_nwk_permit_joining, ret %d", (FMT__D, ret));

  return ret;
}

void adaptor_handle_nwk_formation_response(zb_ret_t status)
{
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

  TRACE_MSG(TRACE_TRANSPORT3, ">> adaptor_handle_nwk_formation_response", (FMT__0));

  zb_buf_set_status(buf, status);
  ncp_host_handle_nwk_formation_response(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< adaptor_handle_nwk_formation_response", (FMT__0));
}

void adaptor_handle_nwk_permit_joining_response(zb_ret_t status)
{
  zb_bufid_t buf = zb_buf_get(ZB_TRUE, 0);

  TRACE_MSG(TRACE_TRANSPORT3, ">> adaptor_handle_nwk_permit_joining_response", (FMT__0));

  zb_buf_set_status(buf, status);
  ncp_host_handle_nwk_permit_joining_response(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<< adaptor_handle_nwk_permit_joining_response", (FMT__0));
}
