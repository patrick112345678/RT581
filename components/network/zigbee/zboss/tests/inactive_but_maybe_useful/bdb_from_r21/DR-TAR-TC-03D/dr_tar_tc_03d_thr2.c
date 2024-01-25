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
/* PURPOSE: TH ZR2
*/


#define ZB_TEST_NAME DR_TAR_TC_03D_THR2
#define ZB_TRACE_FILE_ID 40977
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "dr_tar_tc_03d_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static void send_leave_delayed(zb_uint8_t unused);
static void send_leave(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr2");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr2);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_CERT_HACKS().enable_leave_to_router_hack = 1;
  
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_NIB().max_children = 0;

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


static void send_leave_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_GET_OUT_BUF_DELAYED(send_leave);
}


static void send_leave(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_nlme_leave_request_t *req = ZB_GET_BUF_PARAM(buf, zb_nlme_leave_request_t);

  TRACE_MSG(TRACE_ZDO1, ">>send_leave: buf_param = %d", (FMT__D, param));

  req->remove_children = 0;
  req->rejoin = 0;
  ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr_dut);
  zb_nlme_leave_request(param);
  
  TRACE_MSG(TRACE_ZDO1, "<<send_leave", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        ZB_SCHEDULE_ALARM(send_leave_delayed, 0, THR2_SEND_LEAVE_DELAY);
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
