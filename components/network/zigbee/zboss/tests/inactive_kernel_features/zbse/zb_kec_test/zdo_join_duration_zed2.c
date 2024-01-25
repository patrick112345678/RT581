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
/* PURPOSE: ZED2
*/

#define ZB_TRACE_FILE_ID 40952
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

zb_ieee_addr_t g_ieee_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
    ARGV_UNUSED;

#ifndef KEIL
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_zed2");


    /* set ieee addr */
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);
#ifndef ZB_NS_BUILD
#endif


#ifdef ZB_SECURITY
    /* turn off security */
    ZB_SET_NIB_SECURITY_LEVEL(0);
#endif

    /* become an ED */
    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

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


void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)buf->u.hdr.status));
    }

    zb_free_buf(buf);
}
