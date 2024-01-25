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
/* PURPOSE: Very simple test: just trace. To be used for OTA test.
*/

#define ZB_TRACE_FILE_ID 41733
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

MAIN()
{
  zb_uint_t i;
  ARGV_UNUSED;

  ZB_SET_TRACE_ON();
  ZB_PLATFORM_INIT();
  TRACE_INIT("");

  for (i = 0 ; i < 10 ; ++i)
  {
    TRACE_MSG(TRACE_ERROR, "trace trace", (FMT__0));
  }

  while(1)
  {
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

#if 0
void zb_zdo_startup_complete(zb_uint8_t param)
{
}
#endif
/*! @} */
