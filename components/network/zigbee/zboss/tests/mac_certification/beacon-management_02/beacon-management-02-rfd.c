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
/* PURPOSE: FFD1 for beacon-management_02 test.
*/

#define ZB_TRACE_FILE_ID 41722
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"


#define ZB_TEST_ADDR                  0x1122
#define TEST_PAN_ID                   0x1AAA
#define ZB_TEST_CHANNEL               0x14
#define TEST_CHANEL_MASK              (1l << ZB_TEST_CHANNEL)


static zb_ieee_addr_t g_zc_addr = {0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xde, 0xac};

#define LOG_FILE      "mac_bm_02"



/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("mac_bm_02");
  ZG -> nwk.nib.security_level = 0;
  ZB_IEEE_ADDR_COPY(ZB_PIB_EXTENDED_ADDRESS(), &g_zc_addr);
  MAC_PIB().mac_pan_id = TEST_PAN_ID;

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask           = TEST_CHANEL_MASK;

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
  TRACE_MSG(TRACE_APS3, ">>zb_zdo_startup_complete status %d", (FMT__D, (int)buf -> u.hdr.status));
  if (buf -> u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK. Update short addr to 0x%x", (FMT__D, ZB_TEST_ADDR));
    MAC_PIB().mac_short_address = ZB_TEST_ADDR;
    ZB_UPDATE_SHORT_ADDR();

    zb_beacon_request_command();
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, (int)zb_buf_get_status(param)));
  }
  zb_buf_free(param);
}


/*! @} */
