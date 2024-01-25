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
/* PURPOSE: Simple GPCB for GP device
*/

#define ZB_TRACE_FILE_ID 41555
#include "zb_common.h"

#if defined ZB_ENABLE_HA

#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"

#include "zb_ha.h"

#include "test_config.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

zb_ieee_addr_t g_zc_addr = DUT_GPP_IEEE_ADDR;

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("dut_gpp");


    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);

    ZB_PIBCACHE_PAN_ID() = 0x1aaa;

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zcl_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }
    TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include <stdio.h>
MAIN()
{
    ARGV_UNUSED;

    printf("HA profile and GPCB role should be enabled in zb_config.h\n");

    MAIN_RETURN(1);
}

#endif
