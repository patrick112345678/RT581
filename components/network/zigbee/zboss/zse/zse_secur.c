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
/*  PURPOSE: SE security related routines moved from the core
*/

#define ZB_TRACE_FILE_ID 107
#include "zb_common.h"

#ifdef ZB_ENABLE_SE
void zb_se_delete_cbke_link_key(zb_ieee_addr_t ieee_address)
{
  zb_uindex_t idx;

  TRACE_MSG(TRACE_SECUR1, "zb_se_delete_cbke_link_key addr " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ieee_address)));

  for (idx = 0; idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ++idx)
  {
    TRACE_MSG(TRACE_SECUR1, "idx %hd", (FMT__H, idx));
    if (zb_aps_keypair_load_by_idx(idx) == RET_OK &&
        ZB_IEEE_ADDR_CMP(ZB_AIB().aps_device_key_pair_storage.cached.device_address, ieee_address) &&
        ZB_AIB().aps_device_key_pair_storage.cached.key_attributes == ZB_SECUR_VERIFIED_KEY &&
        ZB_AIB().aps_device_key_pair_storage.cached.key_source == ZB_SECUR_KEY_SRC_CBKE)
    {
      TRACE_MSG(
        TRACE_SECUR2, "Delete CBKE key for idx %d attr %hd source %hd",
        (FMT__D_H_H,
         idx,
         ZB_AIB().aps_device_key_pair_storage.cached.key_attributes,
         ZB_AIB().aps_device_key_pair_storage.cached.key_source));
      TRACE_MSG(TRACE_SECUR2, "key " TRACE_FORMAT_128,
                (FMT__B, TRACE_ARG_128(ZB_AIB().aps_device_key_pair_storage.cached.link_key)));
      zb_secur_delete_link_key_by_idx(idx);
      break;
    }
  }
}
#endif
