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
/* PURPOSE: Network confirm routine
*/

#define ZB_TRACE_FILE_ID 2142
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

/*! \addtogroup ZB_APS */
/*! @{ */


/* Note: all that callbacks are temporary here. It must be somewhere in */

#ifndef ZB_LITE_NO_NLME_ROUTE_DISCOVERY
void zb_nlme_route_discovery_confirm(zb_uint8_t param)
{

    TRACE_MSG(TRACE_NWK1, ">>zb_nlme_route_discovery_confirm %hd", (FMT__H, param));

    if (param != 0U)
    {
        zb_nlme_route_discovery_confirm_t *confirm = (zb_nlme_route_discovery_confirm_t *)zb_buf_begin(param);
        ZVUNUSED(confirm);
    }

    TRACE_MSG(TRACE_NWK1, "<<zb_nlme_route_discovery_confirm", (FMT__0));
}
#endif


/*! @} */
