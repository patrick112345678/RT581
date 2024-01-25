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
/* PURPOSE: Generic Coordinator
*/

#define ZB_TRACE_FILE_ID 40114

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "generic_zc.h"
#include "generic_zc_hal.h"

zb_ieee_addr_t g_zc_addr = GENERIC_ZC_ADDRESS;

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

    generic_zc_hal_init();

    /* Init device, load IB values from nvram or set it to default */


    ZB_INIT("zdo_zc");

    ZB_SET_NIB_SECURITY_LEVEL(5);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
    ZB_PIBCACHE_PAN_ID() = 0x1aaa;

    /* let's always be coordinator */
    ZB_AIB().aps_designated_coordinator = 1;

    /* Set to 1 to force entire nvram erase. */
    ZB_AIB().aps_nvram_erase_at_start = 0;
    /* set to 1 to enable nvram usage. */
    ZB_AIB().aps_use_nvram = 1;
    ZB_AIB().aps_channel_mask = GENERIC_ZC_CHANNEL_MASK;

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void zb_zdo_startup_complete(zb_uint8_t param) ZB_CALLBACK
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    TRACE_MSG(TRACE_APS3, ">>zb_zdo_startup_complete status %d", (FMT__D, (int)buf->u.hdr.status));
    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ZDO1, "Device STARTED OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
    }
    zb_free_buf(buf);
}
