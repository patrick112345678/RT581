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
/*  PURPOSE: MAC split transport test
*/

#include <stdio.h>
#include <stdlib.h>

#define ZB_TRACE_FILE_ID 40938
#include "zb_common.h"

void zboss_signal_handler(zb_uint8_t param)
{
  ZVUNUSED(param);
}

#ifdef ZB_MACSPLIT_DEVICE
void zb_nwk_init()
{
}

void zb_check_oom_status(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_zcl_diagnostics_inc(zb_uint16_t attr_id, zb_uint8_t value)
{
  ZVUNUSED(attr_id);
  ZVUNUSED(value);
}


void zb_aps_init()
{
}

void zb_zdo_init()
{
}

void zb_zcl_init()
{
}

void zll_init()
{
}

void zb_nwk_unlock_in(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_sleep_can_sleep(zb_uint32_t sleep_tmo)
{
  ZVUNUSED(sleep_tmo);
}

void zb_sleep_init()
{
}

zb_uint32_t zb_sleep_calc_sleep_tmo()
{
  return 0;
}
#endif /* ZB_MACSPLIT_DEVICE */

static void send_data(zb_uint8_t param)
{
  zb_macsplit_transport_send_data(param);
}

static void recv_data(zb_uint8_t ref)
{
  send_data(ref);
}

MAIN()
{
  ARGV_UNUSED;

#if defined ZB_TRACE_LEVEL
  /* choose trace transport if it need */
  ZB_TRACE_SET_TRANSPORT(ZB_TRACE_TRANSPORT_UART);
#endif

  ZB_INIT("macsplit_transport_test_reader");

  zb_macsplit_set_cb_recv_data(recv_data);

  while (ZB_TRUE)
  {
    zb_sched_loop_iteration();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}
