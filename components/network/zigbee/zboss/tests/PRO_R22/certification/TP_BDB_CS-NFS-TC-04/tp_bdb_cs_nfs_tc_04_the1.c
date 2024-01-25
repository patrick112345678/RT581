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
/* PURPOSE: TH ZED1
*/

#define ZB_TEST_NAME TP_BDB_CS_NFS_TC_04_THE1
#define ZB_TRACE_FILE_ID 40895

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

#include "tp_bdb_cs_nfs_tc_04_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ED_ROLE
#error End Device role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_the1 = IEEE_ADDR_THE1;

static zb_bool_t is_first_start = ZB_TRUE;

static void zb_nwk_leave_req_delayed(zb_uint8_t unused);
static void zb_nwk_leave_req(zb_uint8_t param);
static void retry_join(zb_uint8_t unused);

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_the1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

	
  zb_set_long_address(g_ieee_addr_the1);

  zb_set_network_ed_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_set_rx_on_when_idle(ZB_FALSE);
  MAC_ADD_INVISIBLE_SHORT(0);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static void zb_nwk_leave_req_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(zb_nwk_leave_req);
}

static void zb_nwk_leave_req(zb_uint8_t param)
{
  zb_nlme_leave_request_t *req = NULL;

  TRACE_MSG(TRACE_ERROR, "zb_nwk_leave_req", (FMT__0));

  req = ZB_BUF_GET_PARAM(param, zb_nlme_leave_request_t);
  ZB_64BIT_ADDR_ZERO(req->device_address);
  req->remove_children = ZB_FALSE;
  req->rejoin = ZB_FALSE;

  ZB_SCHEDULE_CALLBACK(zb_nlme_leave_request, param);
}

static void retry_join(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">>retry_join", (FMT__0));

  bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APS1, "Device started, status %d", (FMT__D, status));
      if (status == 0)
      {
        if (is_first_start)
        {
          is_first_start = ZB_FALSE;

          ZB_SCHEDULE_ALARM(zb_nwk_leave_req_delayed, 0, THED_SEND_LEAVE_DELAY);
          ZB_SCHEDULE_ALARM(retry_join, 0, THED_JOIN_THR1_DELAY + DO_RETRY_JOIN_DELAY);
        }
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      TRACE_MSG(TRACE_APS1, "Unknown signal, status %d", (FMT__D, status));
      break;
  }

  zb_buf_free(param);
}


/*! @} */
