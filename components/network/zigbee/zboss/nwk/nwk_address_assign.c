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
/* PURPOSE: Network address assign
*/
#define ZB_TRACE_FILE_ID 2229
#include <stdlib.h>

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "nwk_internal.h"

/*! \addtogroup ZB_NWK */
/*! @{ */


/* all routines called from nwk (bank1) - not need to be banker */

zb_bool_t zb_nwk_check_assigned_short_addr(zb_uint16_t short_addr)
{
  zb_address_ieee_ref_t ref;

  /* not broadcast address */
  if (ZB_NWK_IS_ADDRESS_BROADCAST(short_addr) != 0U)
  {
    return ZB_FALSE;
  }
  /* not coordinator */
  else if (short_addr == 0x0000U)
  {
    return ZB_FALSE;
  }
  /* unknown address */
  else if (zb_address_by_short((short_addr), ZB_FALSE, ZB_FALSE, &ref) == RET_OK)
  {
    return ZB_FALSE;
  }
  /* not equal my address */
  else if (ZB_PIBCACHE_NETWORK_ADDRESS() == short_addr)
  {
    return ZB_FALSE;
  }
  else
  {
    return ZB_TRUE;
  }
}


#if defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE

/* see 3.6.1.6 for Distributed address assign mechanism */
/* and doc/html/nwk/Zigbee-Tree-Routing-How-It-Works-and-Why-It-Sucks.html */
zb_uint16_t zb_nwk_daa_calc_cskip(zb_uint8_t depth )
{
  zb_uint16_t ret = 0;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_daa_calc_cskip depth %d", (FMT__D, depth));

  TRACE_MSG(TRACE_NWK1, "max_routers %hd max_children %hd, max_depth %hd depth %hd",
            (FMT__H_H_H_H, ZB_NIB().max_routers, ZB_NIB().max_children,
             ZB_NIB().max_depth, depth));

  if ( ZB_NIB().max_routers == 1 )
  {
    ret = 1 + ZB_NIB().max_children*(ZB_NIB().max_depth - depth - 1);
  }
  else
  {
    zb_uint16_t tmp = 1, i = 0;

    for (i = 0; i < ZB_NIB().max_depth - depth - 1; i++)
    {
      tmp *= ZB_NIB().max_routers;
    }

    ret = (zb_uint16_t)((1 + ZB_NIB().max_children - ZB_NIB().max_routers - ZB_NIB().max_children*(int)tmp)
                        /(1 - (int)ZB_NIB().max_routers));
  }

  ZB_ASSERT(ZB_NIB().max_children == 0 || ret != 0);

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_daa_calc_cskip %d", (FMT__D, ret));
  return ret;
}

/* ZLL ED maybe address assignment capable. ZLL uses ZB PRO stack -
 * stochastic addr assignment may be used */
#elif defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN && (defined ZB_ROUTER_ROLE || defined ZB_ZLL_ADDR_ASSIGN_CAPABLE)  /* Zigbee pro */

/*
 * Select new stochastic address
 *
 * New address must be from the allowed range, and be unknown for device
 * and not equal current address
 */
zb_uint16_t zb_nwk_get_stoch_addr(void)
{
  zb_uint16_t res_addr;
  do {
    res_addr = ZB_RANDOM_U16();

  } while ( !zb_nwk_check_assigned_short_addr(res_addr) );

  return(res_addr);
}

#endif


/*! @} */
