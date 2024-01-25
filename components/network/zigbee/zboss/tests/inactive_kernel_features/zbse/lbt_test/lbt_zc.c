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
/* PURPOSE: test for ZC application written using ZDO.
*/


#define ZB_TRACE_FILE_ID 63697
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_trace.h"

#include "lbt_test.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_SUB_GHZ_LBT
#error LBT is not compiled!
#endif  /* ZB_SUB_GHZ_LBT */

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#ifdef ZB_NS_BUILD
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif
#endif

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("lbt_zc");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;

  /* Set to 1 to force entire nvram erase. */
  ZB_AIB().aps_nvram_erase_at_start = 1;
  /* set to 1 to enable nvram usage. */
  ZB_AIB().aps_use_nvram = 1;
  ZDO_CTX().conf_attr.permit_join_duration = ZB_DEFAULT_PERMIT_JOINING_DURATION;
#ifdef CHANNEL
  {
    zb_channel_list_t channel_list;

    zb_channel_list_init(channel_list);
    //zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE0_2_4_GHZ, (1L<<11) | (1L<<15) | (1L<<20) | (1L<<26));

    zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE28_SUB_GHZ, 1L<<CHANNEL | 1L<<(CHANNEL + 1));

    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);

    /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
    /* zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL); */
  }
#endif

  ZB_SET_TRAF_DUMP_ON();

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

  TRACE_MSG(TRACE_APS3, ">>zb_zdo_startup_complete %hd status %d", (FMT__H_D, param, (int)buf->u.hdr.status));
  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
  }

  zb_free_buf(buf);
}
