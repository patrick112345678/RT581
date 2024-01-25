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
/*  PURPOSE: stubs for NCP host
*/

#define ZB_TRACE_FILE_ID 16

#include "zb_common.h"

void zb_nwk_unlock_in(zb_uint8_t param)
{
    ZVUNUSED(param);
}

zb_ret_t zb_mac_logic_iteration(void)
{
    return RET_OK;
}

/* to be implemented! */

/* 1-st priority */
/* Notes on zb_set_* functions:
   In case of monolithic build these functions set values in global contexts.
   The values processed in zboss_start().
   The most straight-forward way to implement zb_set_* functions in NCP host is
   to set values in the contexts as well and send needed requests in zboss_start().

   An alternative solution is to send requests immediately but in this case we need
   requests serialization on the host and original semantic is lost, e.g. we won't be able
   to call zb_set_nvram_erase_at_start AFTER any other call because it cancels all the other changes.
*/

/* Set Zigbee Role
   Legacy support is not needed right now and
   commissioning logic is to be implemented on the host side.
*/
/* implemented in zb_zboss_api_common.c */


/* TODO: implement an NCP command or move blacklist entirely to the host. See ZOI-200. */
void zb_nwk_blacklist_reset(void)
{
}

void zb_nwk_blacklist_add(zb_ext_pan_id_t ext_pan_id)
{
    ZVUNUSED(ext_pan_id);
}

zb_bool_t zb_nwk_blacklist_is_full(void)
{
    return ZB_FALSE;
}

/* 2-nd priority */
/* Could be implemented with the use of Get Neighbor by IEEE.
   Caching may be helpful but how to implement this cache?

   We need to preserve current semantic, so we need to get neighbor by SHORT address.
   Two ways:
   1) get IEEE address with Get IEEE by short and then use Get Neighbor by IEEE.
   2) implement Get Neighbor by short.
   But we need results immediately so we can't send requests asynchronously. TBD.

   Used in samples only and only to trace LQI and RSSI values.
*/
void zb_zdo_get_diag_data(zb_uint16_t short_address, zb_uint8_t *lqi, zb_int8_t *rssi)
{
    ZVUNUSED(short_address);

    *lqi = ZB_MAC_LQI_UNDEFINED;
    *rssi = ZB_MAC_RSSI_UNDEFINED;
}




/* TBD */
/* it looks like this function is used mostly to check
   if the device is joined to some network or not.
   Exceptions: SE commissioning logic,
               ZCL common - it uses this function to determine whether to secure an outgoing frame.
                            (whether to set ZB_APSDE_TX_OPT_SECURITY_ENABLED)
 */
zb_aps_device_key_pair_set_t *zb_secur_get_link_key_by_address(zb_ieee_addr_t address, zb_secur_key_attributes_t attr)
{
    ZVUNUSED(address);
    ZVUNUSED(attr);

    return NULL;
}


/* used in zb_address.c:zb_address_update() in conflict resolving */
zb_bool_t zb_nwk_neighbor_exists(zb_address_ieee_ref_t addr_ref)
{
    ZVUNUSED(addr_ref);

    return ZB_FALSE;
}


void zb_check_oom_status(zb_uint8_t param)
{
    ZVUNUSED(param);
}


zb_uint16_t zb_aps_get_max_trans_size(zb_uint16_t short_addr)
{
    ZVUNUSED(short_addr);

    return 0;
}


void zb_zdo_rejoin_backoff_continue(zb_uint8_t param)
{
    ZVUNUSED(param);
}


zb_ret_t zb_zdo_rejoin_backoff_start(zb_bool_t insecure_rejoin)
{
    ZVUNUSED(insecure_rejoin);
    ZB_ASSERT(ZB_FALSE);
    return 0;
}


zb_bool_t zb_zdo_rejoin_backoff_force(void)
{
    ZB_ASSERT(ZB_FALSE);
    return 0;
}

zb_bool_t zb_zdo_rejoin_backoff_is_running(void)
{
    ZB_ASSERT(ZB_FALSE);
    return 0;
}


void zb_zdo_rejoin_backoff_cancel(void)
{
    ZB_ASSERT(ZB_FALSE);
}
