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
/* PURPOSE: Zigbee cluster library, Startup Attribute Sets
*/

#define ZB_TRACE_FILE_ID 2058

#include "zb_common.h"

#if defined ZB_ENABLE_ZCL

#if defined ZB_ENABLE_HA_SAS

#include "zb_zcl.h"
#include "ha/zb_ha_sas.h"

static zb_ieee_addr_t g_zb_ha_sas_default_ext_pan_id = ZB_HA_SAS_EXT_PAN_ID;

void zb_ha_process_sas(void)
{
  TRACE_MSG(TRACE_ZCL1, "> zb_ha_process_sas", (FMT__0));

  /* Our stack had already set protocol version as appropriate (see
     ZB_PROTOCOL_VERSION) */
  /* Our stack had already set stack profile version, see ZB_STACK_PROFILE */
  /* Startup control parameter is an indicator one, not implemented */
  /* Config_Rejoin_Interval and Config_Max_Rejoin_Interval attributes are
     optional, so not supported */
  /* Master, network, and default trust center link keys are not set because
     they are same as stack defaults. Also this point should be controlled for
     possible changes in the future. */
  if (!ZB_JOINED())
  {
    ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_HA_SAS_SHORT_ADDRESS;
    ZB_PIBCACHE_PAN_ID() = ZB_HA_SAS_PAN_ID;
    ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), g_zb_ha_sas_default_ext_pan_id);
#ifndef DEBUG
    zb_aib_channel_page_list_set_2_4GHz_mask(ZB_HA_SAS_CHANNEL_MASK); /* MMDEVSTUBS */
#endif
    ZB_IEEE_ADDR_ZERO(ZB_AIB().trust_center_address);
  }
  COMM_CTX().discovery_ctx.nwk_scan_attempts = ZB_HA_SAS_SCAN_ATTEMPTS;
  COMM_CTX().discovery_ctx.nwk_time_btwn_scans = ZB_HA_SAS_TIME_BTWN_SCANS;

  TRACE_MSG(TRACE_ZCL1, "< zb_ha_process_sas", (FMT__0));
}

#endif /* defined ZB_ENABLE_HA_SAS */

#endif /* defined ZB_ENABLE_ZCL */
