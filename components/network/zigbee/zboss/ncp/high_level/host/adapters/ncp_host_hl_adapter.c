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
/*  PURPOSE: NCP High level transport (adapters layer) implementation for the host side */

#define ZB_TRACE_FILE_ID 17506

#include "ncp_host_hl_adapter.h"

void ncp_host_adapter_init_ctx(void)
{
  TRACE_MSG(TRACE_TRANSPORT1, ">> ncp_host_adapter_init_ctx", (FMT__0));
  ncp_host_zdo_adapter_init_ctx();
  ncp_host_aps_adapter_init_ctx();
  ncp_host_nvram_adapter_init_ctx();
  ncp_host_secur_adapter_init_ctx();
#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
  ncp_host_intrp_adapter_init_ctx();
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */
  TRACE_MSG(TRACE_TRANSPORT1, "<< ncp_host_adapter_init_ctx", (FMT__0));
}
