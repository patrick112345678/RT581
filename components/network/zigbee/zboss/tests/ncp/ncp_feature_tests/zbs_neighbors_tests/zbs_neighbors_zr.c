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
/* PURPOSE: ZR joins to ZC, then sends APS packet.
*/

#define ZB_TRACE_FILE_ID 6
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef NCP_SDK
static zb_uint8_t g_ic1[16+2] = TEST_IC;
#endif

static zb_ieee_addr_t g_ieee_addr = TEST_ZR_ADDR;


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  ZB_SET_TRACE_ON();
  //ZB_SET_TRACE_MASK(-1);
  ZB_SET_TRACE_MASK(0xDFFF);
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_neighbors_zr");

  zb_set_long_address(g_ieee_addr);

  /* Subgig is impossible with ZR! */
  /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
  zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_network_router_role(1L << CHANNEL);

  zb_set_max_children(1);

  ZB_SET_TRAF_DUMP_ON();

// ZB_BDB().bdb_join_uses_install_code_key = 1;
#ifdef NCP_SDK
  ZB_TCPOL().require_installcodes = ZB_TRUE;
  zb_secur_ic_set(ZB_IC_TYPE_128, g_ic1);
#endif

  /* Uncomment to test linkage with distributed security feature on */
  //zb_enable_distributed();

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

//! [trace_64]
  TRACE_MSG(TRACE_ERROR, "aps ext pan id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
//! [trace_64]

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        break;

      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    zb_buf_free(param);
  }
}


/*! @} */
