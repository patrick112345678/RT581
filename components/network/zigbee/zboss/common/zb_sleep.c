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
/* PURPOSE: Sleep mode routines
*/

#define ZB_TRACE_FILE_ID 44

#include "zb_common.h"

#ifdef ZB_USE_SLEEP
#include "zb_secur.h"
#include "zb_nwk.h"
#include "zb_zdo_globals.h"

void zb_sleep_init()
{
  ZG->sleep.threshold = ZB_SCHED_SLEEP_THRESHOLD_MS;
  ZG->sleep.permit_sleep_signal = ZB_TRUE;
  ZG->sleep.last_timestamp = ZB_TIMER_GET();

  /* By default - keep the timer always active for routers. */
#ifdef ZB_ED_ROLE
  zb_timer_enable_stop();
#endif /* ZB_ED_ROLE */
}

zb_ret_t zb_sleep_set_threshold(zb_uint32_t threshold_ms)
{
  if (threshold_ms <= ZB_MAXIMUM_SLEEP_THRESHOLD_MS && threshold_ms >= ZB_SCHED_SLEEP_THRESHOLD_MS)
  {
    ZG->sleep.threshold = threshold_ms;
    return RET_OK;
  }
  else
  {
    return RET_ERROR;
  }
}

zb_uint32_t zb_get_sleep_threshold()
{
  return ZG->sleep.threshold;
}

zb_uint32_t zb_sleep_calc_sleep_tmo()
{
  zb_uint32_t sleep_tmo = 0;

  /* Can sleep if no immediate callbacks scheduled. */
  if (ZB_RING_BUFFER_IS_EMPTY(ZB_CB_Q))
  {
    /* If delayed callbacks queue is not empty, calculate sleep_tmo */
    if (!ZB_POOLED_LIST8_IS_EMPTY(ZG->sched.tm_queue))
    {
      zb_time_t t = ZB_TIMER_GET();
      zb_tm_q_ent_t *ent = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next)
        + ZG->sched.tm_buffer;

      sleep_tmo = ZB_TIME_SUBTRACT(ent->run_time, t);
#if 0
      /* TODO: some extra sleep/awake pairs on hardware will be debugged and reduced in the future */
      TRACE_MSG(TRACE_APP4, "zb_sleep_calc_sleep_tmo tmo %d func %p t %u run_time %u (my addr %p)", (FMT__D_P_D_D_P, sleep_tmo, ent->func, t, ent->run_time, &zb_sleep_calc_sleep_tmo));
#endif
    }
    else
    {
      sleep_tmo = ZB_SCHED_SLEEP_NO_TM_BI;
    }
  }

  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(sleep_tmo);
}

void zb_sleep_can_sleep(zb_uint32_t sleep_tmo)
{
  zb_bufid_t sig_buf = zb_buf_get_out();

  if (sig_buf != 0U)
  {
    zb_zdo_signal_can_sleep_params_t *can_sleep_params;
    TRACE_MSG(TRACE_MACLL1, "zb_sleep_can_sleep param %hd", (FMT__H, sig_buf));
    can_sleep_params = (zb_zdo_signal_can_sleep_params_t *)zb_app_signal_pack(sig_buf,
      ZB_COMMON_SIGNAL_CAN_SLEEP, RET_OK, (zb_uint8_t)sizeof(zb_zdo_signal_can_sleep_params_t));
    can_sleep_params->sleep_tmo = sleep_tmo;
    ZB_SCHEDULE_CALLBACK(zboss_signal_handler, sig_buf);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "zb_sleep_can_sleep: dont have a buffer for sig!", (FMT__0));
  }
}

void zb_sleep_now(void)
{
  zb_bool_t timer_was_on = (zb_bool_t)ZB_CHECK_TIMER_IS_ON();
  zb_uint32_t timer_orig_tmo;
  zb_uint32_t slept_time;
  zb_uint32_t sleep_tmo;

  ZB_DISABLE_ALL_INTER();
  timer_orig_tmo = ZB_TIMER_CTX().timer_stop - ZB_TIMER_CTX().timer;
  sleep_tmo = zb_sleep_calc_sleep_tmo();
  

  ZB_ENABLE_ALL_INTER();


  if (sleep_tmo > ZG->sleep.threshold)
  {
    //zb_timer_stop();
    slept_time = zb_osif_sleep(sleep_tmo);

    /* zzzzZZZZZzzzz */

    if (slept_time == ZB_SLEEP_INVALID_VALUE)
    {
      slept_time = 0;
    }
    else
    {
      /* slept_time is correct, permit sleep signal */
      ZG->sleep.permit_sleep_signal = ZB_TRUE;
    }

    if (timer_was_on)
    {
#ifndef ZB_NSNG
      /* Continue original timer, but divide time which we already slept. */
      /* In case of NSNG nsng_move_time() already perform correction, so this step is not needed */
      if (ZB_TIMER_CTX().canstop)
      {
        /* Move timer if we stopped the timer */
        ZB_TIMER_CTX().timer += ZB_MILLISECONDS_TO_BEACON_INTERVAL(slept_time);
      }
#endif
      if (timer_orig_tmo > ZB_MILLISECONDS_TO_BEACON_INTERVAL(slept_time))
      {
        zb_timer_start(timer_orig_tmo - ZB_MILLISECONDS_TO_BEACON_INTERVAL(slept_time));
      }
    }
    zb_osif_wake_up();

    if (slept_time != 0U)
    {
      TRACE_MSG(TRACE_MACLL1, "slept_time %ld ms", (FMT__L, slept_time));
    }
  }
}

#endif /* ZB_USE_SLEEP */

