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
/* PURPOSE: General purpose software watchdog for ZB apps
*/


#define ZB_TRACE_FILE_ID 50
#include "zb_common.h"

#ifdef USE_ZB_WATCHDOG

#include "zb_time.h"
#include "zb_scheduler.h"
#include "zb_error_indication.h" /* ZB_CRITICAL_ERROR_WATCHDOG */
#include "zb_watchdog.h"

void zb_add_watchdog(zb_uint8_t wd_number, zb_time_t timeout)
{
  if (wd_number < ZB_N_WATCHDOG)
  {
    ZG->watchdog[wd_number].timeout = timeout;
    ZG->watchdog[wd_number].last_kick = ZB_TIMER_GET();
    ZG->watchdog[wd_number].state = ZB_WATCHDOG_ENABLED;
  }
  ZB_SCHEDULE_ALARM_CANCEL(zb_watchdog_scheduler, 0);
  ZB_SCHEDULE_ALARM(zb_watchdog_scheduler, 0, ZB_WATCHDOG_SCHED_QUANT);
}


void zb_kick_watchdog(zb_uint8_t wd_number)
{
  if (wd_number < ZB_N_WATCHDOG
      && ZG->watchdog[wd_number].timeout != 0)
  {
    ZG->watchdog[wd_number].last_kick = ZB_TIMER_GET();
  }
}


void zb_stop_watchdog(zb_uint8_t wd_number)
{
  zb_add_watchdog(wd_number, 0);
}

void zb_watchdog_scheduler(zb_uint8_t param)
{
  zb_int_t i;

  (void)param;

  for (i = 0 ; i < ZB_N_WATCHDOG ; ++i)
  {
    if (ZG->watchdog[i].state == ZB_WATCHDOG_ENABLED &&
        ZG->watchdog[i].timeout != 0)
    {
      zb_time_t cur_t = ZB_TIMER_GET();
      zb_time_t last_t = ZB_TIME_ADD(ZG->watchdog[i].last_kick, ZG->watchdog[i].timeout);

      if (ZB_TIME_GE(cur_t, last_t) && cur_t != last_t)
      {
        TRACE_MSG(TRACE_ERROR, "ERROR: Watchdog (id %hd) timeout", (FMT__H, i));

        ZB_ERROR_RAISE(ZB_ERROR_SEVERITY_FATAL,
                       ERROR_CODE(ERROR_CATEGORY_WATCHDOG, ZB_ERROR_WATCHDOG_TRIGGERED),
                       (void*)i);
      }
    }
  }

  ZB_SCHEDULE_ALARM(zb_watchdog_scheduler, 0, ZB_WATCHDOG_SCHED_QUANT);
}

void zb_enable_watchdog(zb_uint8_t wd_number)
{
  if (wd_number < ZB_N_WATCHDOG)
  {
    ZG->watchdog[wd_number].state = ZB_WATCHDOG_ENABLED;
    /* INIT wd timer */
    zb_kick_watchdog(wd_number);
  }
}

void zb_disable_watchdog(zb_uint8_t wd_number)
{
  if (wd_number < ZB_N_WATCHDOG)
  {
    ZG->watchdog[wd_number].state = ZB_WATCHDOG_DISABLED;
  }
}

#endif  /* USE_ZB_WATCHDOG */
