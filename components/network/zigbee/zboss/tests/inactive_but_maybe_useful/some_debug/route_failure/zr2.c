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
/* PURPOSE: ZED
*/

#define ZB_TRACE_FILE_ID 40085
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "test_common.h"


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_3_zr2");

  ZB_PIBCACHE_PAN_ID() = 0x1AAA;

  {
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_list_add(channel_list, TEST_PAGE, (1L << TEST_CHANNEL));
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
  }

  ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ROUTER;

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr_r2);

  MAC_ADD_INVISIBLE_SHORT(0x0000);

  MAC_ADD_VISIBLE_LONG(g_ieee_addr_r1);
  MAC_ADD_VISIBLE_LONG(g_ieee_addr_ed1);

  zb_set_max_children(1);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static zb_uint16_t addr_assign_cb(zb_ieee_addr_t ieee_addr)
{
  zb_uint16_t addr = (zb_uint16_t)0x1111;
  (void)ieee_addr;

  TRACE_MSG(TRACE_APS3, ">>addr_assignmnet_cb", (FMT__0));

  TRACE_MSG(TRACE_APS3, "<<addr_assignmnet_cb: res = 0x%x;", (FMT__H, addr));

  return addr;
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        zb_nwk_set_address_assignment_cb(addr_assign_cb);
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device START FAILED", (FMT__0));
  }

  zb_free_buf(ZB_BUF_FROM_REF(param));
}

