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


#define ZB_TRACE_FILE_ID 40939
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
  zb_buf_t   *buf = ZB_GET_OUT_BUF();
  zb_uint8_t *ptr;
  zb_uint8_t  str[] = "test1";

  ZVUNUSED(param);

  ZB_BUF_INITIAL_ALLOC(buf, sizeof(str), ptr);
  ZB_MEMCPY(ptr, str, sizeof(str));

  zb_macsplit_transport_send_data(ZB_REF_FROM_BUF(buf));
}

static void recv_data(zb_uint8_t ref)
{
#ifdef ZB_MACSPLIT_HOST
  zb_buf_t   *buf;
  zb_uint8_t *ptr;
  zb_uint16_t i;

  buf = ZB_BUF_FROM_REF(ref);
  ptr = ZB_BUF_BEGIN(buf);

  for (i = 0; i < ZB_BUF_LEN(buf); i++)
  {
    putchar(ptr[i]);
  }
  putchar('\n');
#endif /* ZB_MACSPLIT_HOST */

  ZB_FREE_BUF(ZB_BUF_FROM_REF(ref));

  /* send data again */
   send_data(0);
}

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("macsplit_transport_test_writer");

  zb_macsplit_set_cb_recv_data(recv_data);

  ZB_SCHEDULE_APP_CALLBACK(send_data, 0);

  while (ZB_TRUE)
  {
    zb_sched_loop_iteration();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}
