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
/*  PURPOSE: poll control
*/

#define ZB_TRACE_FILE_ID 2093
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_nwk_nib.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_nvram.h"
#include "zb_ncp.h"

#ifdef ZB_ENABLE_ZCL
#include "zcl/zb_zcl_poll_control.h"
#include "zcl/zb_zcl_common.h"
#endif
#include "zdo_wwah_stubs.h"

#if defined ZB_ED_FUNC && !defined ZB_LITE_NO_ZDO_POLL

static void zb_zdo_poll_parent(zb_uint8_t param);
static void zb_zdo_pim_restart_poll(zb_uint8_t param);
static void zb_zdo_pim_turbo_poll_adaptation(zb_uint8_t got_data);
static void zb_zdo_pim_update_max_poll_interval(zb_uint8_t param);


/* NK: Relevance of was_in_turbo_poll value is guaranteed by nwk queue. */
void zb_zdo_pim_continue_polling_for_pkt(void)
{
  if (ZB_IS_DEVICE_ZED())
  {
    if (ZDO_CTX().pim.was_in_turbo_poll)
    {
      ZDO_CTX().pim.was_in_turbo_poll = ZB_FALSE;
      TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
      zb_zdo_pim_start_turbo_poll_packets(1);
    }
  }
}

/**
   Say to Poll Management logic that we got data packet
 */
void zb_zdo_pim_got_data(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.was_in_turbo_poll = ZB_U2B(ZDO_CTX().pim.turbo_poll_n_packets);
    zb_zdo_pim_turbo_poll_adaptation(1);
  }
}


void zb_zdo_pim_init_defaults(void)
{
  zb_bool_t turbo_prohibit = ZDO_CTX().pim.turbo_prohibit;
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_init_defaults", (FMT__0));
    ZB_BZERO(&ZDO_CTX().pim, sizeof(ZDO_CTX().pim));
    ZDO_CTX().pim.fast_poll_timeout = ZB_PIM_DEFAULT_FAST_POLL_TIMEOUT;
    ZDO_CTX().pim.fast_poll_interval = ZB_PIM_DEFAULT_FAST_POLL_INTERVAL;
    ZDO_CTX().pim.long_poll_interval = ZB_PIM_DEFAULT_LONG_POLL_INTERVAL;
    ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.long_poll_interval;
    ZDO_CTX().pim.turbo_poll_interval = ZB_PIM_DEFAULT_TURBO_POLL_INTERVAL;
    ZDO_CTX().pim.turbo_poll_min = ZB_PIM_DEFAULT_MIN_TURBO_POLL_INTERVAL;
    ZDO_CTX().pim.turbo_poll_max = ZB_PIM_DEFAULT_MAX_TURBO_POLL_INTERVAL;
    ZDO_CTX().pim.work_poll_max = ZDO_CTX().pim.turbo_poll_max;
    ZDO_CTX().pim.was_in_turbo_poll = ZB_FALSE;
    zb_zdo_pim_stop_poll(0);
    ZDO_CTX().pim.turbo_prohibit = turbo_prohibit;
    if(turbo_prohibit)
    {
      TRACE_MSG(TRACE_ZDO1, "turbo poll prohibited, enable fast poll", (FMT__0));
      ZDO_CTX().pim.fast_poll_on = ZB_TRUE;
    }
  }
}


void zb_zdo_pim_start_fast_poll(zb_uint8_t param)
{
  zb_bool_t restart_poll;
  zb_bool_t fast_poll_off;

  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    restart_poll = !ZB_U2B(ZDO_CTX().pim.turbo_poll_n_packets);
    fast_poll_off = !ZDO_CTX().pim.fast_poll_on;

    ZDO_CTX().pim.fast_poll_on = ZB_TRUE;
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_start_fast_poll : fast_poll_on %d restart_poll %d",
              (FMT__D_D, ZDO_CTX().pim.fast_poll_on, restart_poll));

    if (fast_poll_off)
    {
      zb_zdo_pim_update_max_poll_interval(0);
      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_fast_poll_leave, 0);
      ZB_SCHEDULE_ALARM(zb_zdo_fast_poll_leave, 0, ZDO_CTX().pim.fast_poll_timeout);
      if (restart_poll)
      {
        zb_zdo_pim_restart_poll(0);
      }
    }
  }
}


void zb_zdo_fast_poll_leave(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_fast_poll_leave", (FMT__0));
    zb_zdo_pim_stop_fast_poll(0);
  }
}


void zb_zdo_pim_stop_fast_poll(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (ZB_IS_DEVICE_ZED())
  {
    if (ZDO_CTX().pim.fast_poll_on)
    {
      ZDO_CTX().pim.fast_poll_on = ZB_FALSE;
      TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_stop_fast_poll fast_poll_on %d turbo_poll_n_packets %d",
                (FMT__D_D, ZDO_CTX().pim.fast_poll_on, ZDO_CTX().pim.turbo_poll_n_packets));
      if (!ZDO_CTX().pim.fast_poll_on)
      {
        zb_zdo_pim_update_max_poll_interval(0);

        if(!ZB_U2B(ZDO_CTX().pim.turbo_poll_n_packets))
        {
          zb_zdo_pim_restart_poll(0);
        }
      }
    }
  }
}

static void zb_zdo_pim_work_poll_interval_update(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (ZB_IS_DEVICE_ZED())
  {
    if (ZDO_CTX().pim.turbo_poll_n_packets == 0U &&
        ZDO_CTX().pim.turbo_poll_timeout == 0U)
    {
      if (ZDO_CTX().pim.fast_poll_on)
      {
        TRACE_MSG(TRACE_ZDO3, "in fast poll, interval %ld", (FMT__L, ZDO_CTX().pim.fast_poll_interval));
        ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.fast_poll_interval;
      }
      else
      {
        TRACE_MSG(TRACE_ZDO3, "in long poll, interval %ld", (FMT__L, ZDO_CTX().pim.long_poll_interval));
        ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.long_poll_interval;
      }
    }
    else
    {
      if (ZDO_CTX().pim.turbo_poll_interval < ZDO_CTX().pim.turbo_poll_min)
      {
        ZDO_CTX().pim.turbo_poll_interval = ZDO_CTX().pim.turbo_poll_min;
      }
      ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.turbo_poll_interval;
      TRACE_MSG(TRACE_ZDO3, "in turbo poll, interval %ld", (FMT__L, ZDO_CTX().pim.work_poll_interval));
    }
  }
}

static void zb_zdo_pim_restart_poll(zb_uint8_t param)
{
  ZVUNUSED(param);
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_restart_poll poll_in_progress %d poll_stop %d",
              (FMT__D_D, ZDO_CTX().pim.poll_in_progress, ZDO_CTX().pim.poll_stop));

#if 0
    /* DL: NCP now manages automatic turbo poll via
     * sncp_auto_turbo_poll_aps_[rx | tx] from the kernel.
     * turbo_poll_n_packets can become to 0 if arrived, for example,
     * a data packet instead of the expected APS ACK */
#ifdef SNCP_MODE
    /* if turbo poll is over, try to disable poll at all if it was enabled by NCP */
    if (ZDO_CTX().pim.turbo_poll_n_packets == 0U &&
        ZDO_CTX().pim.turbo_poll_timeout == 0U)
    {
      sncp_auto_turbo_poll_stop();
    }
#endif
#endif

    if (ZDO_CTX().pim.poll_in_progress == 0U && ZDO_CTX().pim.poll_stop == 0U)
    {
      zb_time_t t = 0;
      zb_bool_t success;

      success = (zb_schedule_get_alarm_time(zb_zdo_poll_parent, 0, &t) != RET_OK);

      if (!success)
      {
        success = ZB_TIME_GE(t, ZB_TIMER_GET() + ZDO_CTX().pim.work_poll_interval);
      }

      if (success)
      {
        zb_zdo_pim_work_poll_interval_update(0);
        ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_poll_parent, 0);
        ZB_SCHEDULE_ALARM(zb_zdo_poll_parent, 0, ZDO_CTX().pim.work_poll_interval);
      }
    }
  }
}


void zb_zdo_turbo_poll_packets_leave(zb_uint8_t param)
{
  zb_bool_t in_turbo_pkt;
  zb_bool_t in_turbo_cont;
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    in_turbo_pkt = ZB_U2B(ZDO_CTX().pim.turbo_poll_n_packets);
    in_turbo_cont = ZB_U2B(ZDO_CTX().pim.turbo_poll_timeout);

    ZDO_CTX().pim.turbo_poll_n_packets = 0;
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_turbo_poll_packets_leave in_turbo_pkt %hd in_turbo_cont %hd",
              (FMT__H_H, in_turbo_pkt, in_turbo_cont));
    ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_turbo_poll_packets_leave, 0);

    /* Restart only in two cases:
       - Packet turbo polling has been finished
       - Packet polling finished but continuous is in progress */
    if (in_turbo_pkt || in_turbo_cont)
    {
      zb_zdo_pim_update_max_poll_interval(0);
      ZDO_CTX().pim.turbo_poll_interval = ZDO_CTX().pim.turbo_poll_max;
      zb_zdo_pim_restart_poll(0);
    }
  }
}


void zb_zdo_pim_permit_turbo_poll(zb_bool_t permit)
{

#ifndef ZB_TH_ENABLED
  if (ZB_IS_DEVICE_ZED())
#endif
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_permit_turbo_poll permit %d in turbo %d",
              (FMT__D_D, permit, ZDO_CTX().pim.turbo_poll_n_packets));
    ZDO_CTX().pim.turbo_prohibit = !permit;
    if (!permit)
    {
      /* if not permitted, stop immediately */
      zb_zdo_pim_turbo_poll_continuous_leave(0);
      zb_zdo_turbo_poll_packets_leave(0);
    }
  }
}


void zb_zdo_pim_set_turbo_poll_min(zb_time_t turbo_poll_min_ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.turbo_poll_min = ZB_MILLISECONDS_TO_BEACON_INTERVAL(turbo_poll_min_ms);
  }
}


void zb_zdo_pim_reset_turbo_poll_min(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.turbo_poll_min = ZB_PIM_DEFAULT_MIN_TURBO_POLL_INTERVAL;
  }
}

void zb_zdo_pim_set_turbo_poll_max(zb_time_t turbo_poll_max_ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.turbo_poll_max = ZB_MILLISECONDS_TO_BEACON_INTERVAL(turbo_poll_max_ms);
    if (ZDO_CTX().pim.turbo_poll_max < ZB_PIM_DEFAULT_MIN_TURBO_POLL_INTERVAL)
    {
      ZDO_CTX().pim.turbo_poll_max = ZB_PIM_DEFAULT_MIN_TURBO_POLL_INTERVAL;
    }
    zb_zdo_pim_update_max_poll_interval(0);
  }
}

void zb_zdo_pim_reset_turbo_poll_max(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.turbo_poll_max = ZB_PIM_DEFAULT_MAX_TURBO_POLL_INTERVAL;
    zb_zdo_pim_update_max_poll_interval(0);
  }
}

void zb_zdo_pim_start_turbo_poll_continuous(zb_time_t turbo_poll_timeout_ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    /* Considers that continious turbo polling is used once per
     * time. Multiple calling is not allowed - firstly need to stop
     * continuous trubo polling from app and then trigger start once moar */
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
        && !ZDO_CTX().pim.turbo_prohibit
        && ZDO_CTX().pim.turbo_poll_timeout == 0U
        && turbo_poll_timeout_ms != 0U)
    {
      TRACE_MSG(TRACE_APS2, "zb_zdo_pim_start_turbo_poll_continuous", (FMT__0));
      ZDO_CTX().pim.turbo_poll_timeout = ZB_MILLISECONDS_TO_BEACON_INTERVAL(turbo_poll_timeout_ms);

      /* Function cancelling is not needed, because it can't be
       * scheduled in place other than that. */
      /* ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_pim_turbo_poll_continuous_leave, 0); */
      ZB_SCHEDULE_ALARM(zb_zdo_pim_turbo_poll_continuous_leave, 0, ZDO_CTX().pim.turbo_poll_timeout);

      if (ZDO_CTX().pim.turbo_poll_n_packets == 0U)
      {
        ZDO_CTX().pim.turbo_poll_interval = ZDO_CTX().pim.turbo_poll_max;
        zb_zdo_pim_update_max_poll_interval(0);
        zb_zdo_pim_restart_poll(0);
      }
    }
  }
}

void zb_zdo_pim_turbo_poll_continuous_leave(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()) && (ZDO_CTX().pim.turbo_poll_timeout != 0U))
    {
      TRACE_MSG(TRACE_APS2, "zb_zdo_pim_turbo_poll_continuous_leave", (FMT__0));
      /* In case of cancelling by user */
      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_pim_turbo_poll_continuous_leave, 0);
      ZDO_CTX().pim.turbo_poll_timeout = 0;

      /* Check if turbo poll is in progress */
      if (ZDO_CTX().pim.turbo_poll_n_packets == 0U)
      {
        zb_zdo_pim_update_max_poll_interval(0);
        zb_zdo_pim_restart_poll(0);
      }
    }
  }
}

static void zb_zdo_pim_update_max_poll_interval(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    if (zb_zdo_pim_in_fast_poll())
    {
      /* Choose the minimal interval between two */
      ZDO_CTX().pim.work_poll_max = (ZDO_CTX().pim.turbo_poll_max < ZDO_CTX().pim.fast_poll_interval)
        ? (ZDO_CTX().pim.turbo_poll_max)
        : (ZDO_CTX().pim.fast_poll_interval);
    }
    else
    {
      ZDO_CTX().pim.work_poll_max = ZDO_CTX().pim.turbo_poll_max;
    }

    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_update_max_poll_interval %ld",
              (FMT__L, ZDO_CTX().pim.work_poll_max));
  }
}


/* If turbo-poll is active, extend its finish time but do not touch #
   of packets. If turbo-poll is not active, start it with number of
   packets 1.

   This logic is used on APS retransmitting.
 */
void zb_zdo_pim_continue_turbo_poll()
{
  zb_uint8_t n_packets = ZDO_CTX().pim.turbo_poll_n_packets == 0U ? 1U : 0U;
  TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets %d", (FMT__D, ZDO_CTX().pim.turbo_poll_n_packets));
  zb_zdo_pim_start_turbo_poll_packets(n_packets);
}

void zb_zdo_pim_start_turbo_poll_packets(zb_uint8_t n_packets)
{
  if (ZB_IS_DEVICE_ZED())
  {
    zb_time_t alarm_time;

    /* Clean number of waiting packets automatically */
    if (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
        && !ZDO_CTX().pim.turbo_prohibit)
    {
      /* allow turbo poll until receiving turbo_poll_n_packets data packets from
       * the parent. Zero n_packets value can be passed, move timer in
       * this case */
      ZDO_CTX().pim.turbo_poll_n_packets += n_packets;

      TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_start_turbo_poll_packets_poll n_packets %d turbo_poll_n_packets %d work_poll_interval %ld n_polls_per_data %d",
                (FMT__D_D_L_D, n_packets, ZDO_CTX().pim.turbo_poll_n_packets, ZDO_CTX().pim.work_poll_interval,
                 ZDO_CTX().pim.n_polls_per_data));

      if (zb_schedule_get_alarm_time(zb_zdo_turbo_poll_packets_leave, 0, &alarm_time) == RET_OK)
      {
        ZDO_CTX().pim.turbo_leave_alarm_time = alarm_time;
        TRACE_MSG(TRACE_ZDO1, "alarm_time %u", (FMT__D, alarm_time));
      }
      /* Use this only for expected number of packets */
      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_turbo_poll_packets_leave, 0);
      ZB_SCHEDULE_ALARM(zb_zdo_turbo_poll_packets_leave, 0, ZB_PIM_TURBO_POLL_PACKETS_TIMEOUT);

      ZDO_CTX().pim.n_polls_per_data = 0;
      ZDO_CTX().pim.turbo_poll_interval = ZDO_CTX().pim.turbo_poll_min;
      ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.turbo_poll_interval;

      zb_zdo_pim_restart_poll(0);
    }
  }
}


void zb_zdo_pim_turbo_poll_cancel_packet(void)
{
  /*
    Not idea; solution, but...
    Handle ZCL Reporting which is started when no binding exists - see some SE tests.
    In such case ZCL adds 1 more turbo packet, but APS fails to transmit without any polls.
    As a result, never leave turbo poll.
   */
  TRACE_MSG(TRACE_ZDO3, "zb_zdo_pim_turbo_poll_cancel_packet turbo_poll_n_packets %d",
            (FMT__D, ZDO_CTX().pim.turbo_poll_n_packets));
  if (ZDO_CTX().pim.turbo_poll_n_packets > 0U)
  {
    zb_time_t t = ZB_TIMER_GET();
    ZDO_CTX().pim.turbo_poll_n_packets--;
    TRACE_MSG(TRACE_ZDO1, "alarm_time %u t %u", (FMT__D_D, ZDO_CTX().pim.turbo_leave_alarm_time, t));
    if (ZDO_CTX().pim.turbo_leave_alarm_time != 0U
        && ZB_TIME_GE(ZDO_CTX().pim.turbo_leave_alarm_time, t))
    {
      TRACE_MSG(TRACE_ZDO1, "restore turbo cancel alarm at %u", (FMT__D, ZDO_CTX().pim.turbo_leave_alarm_time));
      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_turbo_poll_packets_leave, 0);
      ZB_SCHEDULE_ALARM(zb_zdo_turbo_poll_packets_leave, 0, ZDO_CTX().pim.turbo_leave_alarm_time - t);
    }
    else
    {
      ZDO_CTX().pim.turbo_leave_alarm_time = 0;
    }
    zb_zdo_pim_turbo_poll_adaptation(0);
  }
}


static void zb_zdo_pim_turbo_poll_adaptation(zb_uint8_t got_data)
{
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZDO1, "zb_zdo_pim_turbo_poll_adaptation got_data %d turbo_poll_n_packets %d",
              (FMT__D_D, got_data, ZDO_CTX().pim.turbo_poll_n_packets));
    if (ZDO_CTX().pim.turbo_poll_n_packets != 0U)
    {
      if (!ZB_U2B(got_data))
      {
        ZDO_CTX().pim.n_polls_per_data++;
        if (ZDO_CTX().pim.n_polls_per_data > 1U)
        {
          /* required more than 1 poll to get data - increase poll timeout */
          ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.work_poll_interval * 3U / 2U;
        }
        TRACE_MSG(TRACE_ZDO2, "n_polls_per_data %d work_poll_interval %ld",
                  (FMT__D_L, ZDO_CTX().pim.n_polls_per_data, ZDO_CTX().pim.work_poll_interval));
      }
      else
      {
        if (ZDO_CTX().pim.n_polls_per_data > 1U)
        {
          /* NK: It is not very correct -  first_poll_time could be related to another packet.
             Lets use same work_poll_interval in this case? */
          /* ZDO_CTX().pim.work_poll_interval = ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZDO_CTX().pim.first_poll_time); */
        }
        else
        {
          /* If got data in 1 poll, try to decrease poll timeout */
          ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.work_poll_interval * 4U / 5U;
        }
        TRACE_MSG(TRACE_ZDO2, "n_polls_per_data %d-->0 work_poll_interval %ld",
                  (FMT__D_L, ZDO_CTX().pim.n_polls_per_data, ZDO_CTX().pim.work_poll_interval));
        ZDO_CTX().pim.n_polls_per_data = 0;
        if (ZDO_CTX().pim.turbo_poll_n_packets != 0U)
        {
          ZDO_CTX().pim.turbo_poll_n_packets--;
        }
      }
      /* just check what we got */
      if (ZDO_CTX().pim.work_poll_interval < ZDO_CTX().pim.turbo_poll_min)
      {
        ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.turbo_poll_min;
      }
      else if (ZDO_CTX().pim.work_poll_interval > ZDO_CTX().pim.work_poll_max)
      {
        ZDO_CTX().pim.work_poll_interval = ZDO_CTX().pim.work_poll_max;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }

      ZDO_CTX().pim.turbo_poll_interval = ZDO_CTX().pim.work_poll_interval;
      TRACE_MSG(TRACE_ZDO2, "set turbo_poll_interval %ld turbo_poll_n_packets %d",
                (FMT__L_D, ZDO_CTX().pim.turbo_poll_interval, ZDO_CTX().pim.turbo_poll_n_packets));
      if (ZB_U2B(got_data) && (ZDO_CTX().pim.turbo_poll_n_packets == 0U))
      {
        TRACE_MSG(TRACE_ZDO2, "leaving turbo poll after %ld", (FMT__L, ZB_PIM_TURBO_POLL_LEAVE_TIMEOUT));
        /*  */
        ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_turbo_poll_packets_leave, 0);
        ZB_SCHEDULE_ALARM(zb_zdo_turbo_poll_packets_leave, 0, ZB_PIM_TURBO_POLL_LEAVE_TIMEOUT);
      }
    }
    else
    {
      zb_zdo_pim_work_poll_interval_update(0);
    }
  }
}


void zb_zdo_pim_set_fast_poll_interval(zb_time_t ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.fast_poll_interval = ZB_MILLISECONDS_TO_BEACON_INTERVAL(ms);
    zb_zdo_pim_update_max_poll_interval(0);
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_set_fast_poll_interval %ld ms %ld interval",
              (FMT__L_L, ms, ZDO_CTX().pim.fast_poll_interval));
  }
}


void zb_zdo_pim_set_fast_poll_timeout(zb_time_t ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.fast_poll_timeout = ZB_MILLISECONDS_TO_BEACON_INTERVAL(ms);
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_set_fast_poll_timeout %ld ms %ld",
              (FMT__L_L, ms, ZDO_CTX().pim.fast_poll_timeout));
    if (ZDO_CTX().pim.fast_poll_on)
    {
      ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_fast_poll_leave, 0);
      ZB_SCHEDULE_ALARM(zb_zdo_fast_poll_leave, 0, ZDO_CTX().pim.fast_poll_timeout);
    }
  }
}

void zb_zdo_pim_set_long_poll_interval(zb_time_t ms)
{
  if (ZB_IS_DEVICE_ZED())
  {
    ZDO_CTX().pim.long_poll_interval = ZB_MILLISECONDS_TO_BEACON_INTERVAL(ms);
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_set_long_poll_interval %ld ms %ld",
              (FMT__L_L, ms, ZDO_CTX().pim.long_poll_interval));
  }
}


void zb_zdo_pim_get_long_poll_interval_req(zb_uint8_t param, zb_callback_t cb)
{
  zb_zdo_pim_get_long_poll_interval_resp_t *resp =
    ZB_BUF_GET_PARAM(param, zb_zdo_pim_get_long_poll_interval_resp_t);

  resp->interval = zb_zdo_pim_get_long_poll_ms_interval();

  ZB_SCHEDULE_CALLBACK(cb, param);
}


void zb_zdo_pim_get_in_fast_poll_flag_req(zb_uint8_t param, zb_callback_t cb)
{
  zb_zdo_pim_get_in_fast_poll_flag_resp_t *resp =
    ZB_BUF_GET_PARAM(param, zb_zdo_pim_get_in_fast_poll_flag_resp_t);

  resp->in_fast_poll = zb_zdo_pim_in_fast_poll();

  ZB_SCHEDULE_CALLBACK(cb, param);
}


void zb_zdo_pim_stop_fast_poll_extended_req(zb_uint8_t param, zb_callback_t cb)
{
  zb_zdo_pim_stop_fast_poll_extended_resp_t *resp =
    ZB_BUF_GET_PARAM(param, zb_zdo_pim_stop_fast_poll_extended_resp_t);

  zb_bool_t is_in_fast_poll_before_stop;
  zb_bool_t is_in_fast_poll_after_stop;

  is_in_fast_poll_before_stop = zb_zdo_pim_in_fast_poll();
  zb_zdo_pim_stop_fast_poll(0);
  is_in_fast_poll_after_stop = zb_zdo_pim_in_fast_poll();

  if (!is_in_fast_poll_before_stop)
  {
    resp->stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_NOT_STARTED;
  }
  else if (is_in_fast_poll_after_stop)
  {
    resp->stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_NOT_STOPPED;
  }
  else
  {
    resp->stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_STOPPED;
  }

  ZB_SCHEDULE_CALLBACK(cb, param);
}


zb_poll_mode_t zb_zdo_pim_get_mode()
{
  zb_poll_mode_t pim_mode =
  #ifdef SNCP_MODE
    ZDO_CTX().pim.poll_stop ? ZB_ZDO_PIM_STOP :
  #endif
    (
      ((ZDO_CTX().pim.turbo_poll_n_packets != 0U) || (ZDO_CTX().pim.turbo_poll_timeout != 0U)) ? ZB_ZDO_PIM_TURBO :
      (ZDO_CTX().pim.fast_poll_on ? ZB_ZDO_PIM_FAST : ZB_ZDO_PIM_LONG)
    );
  TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_pim_get_mode pim_mode %hu", (FMT__H, pim_mode));
#if 0
/* The old method for determining ZB_ZDO_PIM_STOP mode without the flag ZDO_CTX().pim.poll_stop */
#ifdef SNCP_MODE
  if (pim_mode != ZB_ZDO_PIM_TURBO)
  {
    /* In SNCP_MODE there is an additional status ZB_ZDO_PIM_STOP. Need to check it. */
    zb_time_t t;
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_get_mode poll_in_progress %hu", (FMT__H, ZDO_CTX().pim.poll_in_progress));
    if (ZDO_CTX().pim.poll_in_progress == 0U)
    {
      zb_ret_t ret;
      ret = zb_schedule_get_alarm_time(zb_zdo_poll_parent, ZB_ALARM_ANY_PARAM, &t);
      TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_pim_get_mode get alarm_time ret %d", (FMT__D, ret));
      if (ret != RET_OK)
      {
        pim_mode = ZB_ZDO_PIM_STOP;
      }
    }
  }
#endif
  TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_pim_get_mode pim_mode %hu", (FMT__H, pim_mode));
#endif
  return pim_mode;
}


zb_bool_t zb_zdo_pim_in_fast_poll()
{
  return (zb_bool_t)ZDO_CTX().pim.fast_poll_on;
}


zb_time_t zb_zdo_get_pim_fast_poll_interval_ms()
{
  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZDO_CTX().pim.fast_poll_interval);
}


zb_time_t zb_zdo_get_pim_fast_poll_timeout_ms()
{
  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZDO_CTX().pim.fast_poll_timeout);
}

zb_time_t zb_zdo_pim_get_long_poll_ms_interval()
{
  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZDO_CTX().pim.long_poll_interval);
}

zb_time_t zb_zdo_get_pim_turbo_poll_min_ms()
{
  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZDO_CTX().pim.turbo_poll_min);
}

zb_time_t zb_zdo_get_poll_interval_ms(void)
{
  return ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZDO_CTX().pim.work_poll_interval);
}


void zb_zdo_pim_start_poll(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    zb_bool_t is_joined = ZB_JOINED();

    ZDO_CTX().pim.poll_stop = ZB_FALSE;
    TRACE_MSG(TRACE_ZDO2, ">> zb_zdo_pim_start_poll", (FMT__0));
    if (!ZDO_CTX().pim.poll_in_progress
        && is_joined
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of Rule 13.5 seems to be a false
         * positive. There are no side effects to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()'. This violation
         * seems to be caused by the fact that this function is an external function, which cannot
         * be analyzed by C-STAT. */
        && (!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
            || (ZB_GET_KEEPALIVE_MODE() == MAC_DATA_POLL_KEEPALIVE)))
    {
      TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_restart_poll", (FMT__0));
      zb_zdo_pim_restart_poll(0);
    }
    TRACE_MSG(TRACE_ZDO2, "<< zb_zdo_pim_start_poll", (FMT__0));
  }
}

void zb_zdo_pim_stop_poll(zb_uint8_t param)
{
  (void)param;
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZDO2, "zb_zdo_pim_stop_poll", (FMT__0));
    zb_zdo_turbo_poll_packets_leave(0);
    zb_zdo_pim_turbo_poll_continuous_leave(0);
    ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_poll_parent, 0);
    ZDO_CTX().parent_threshold_retry = 0;
    ZDO_CTX().pim.poll_stop = ZB_TRUE;
  }
}


#ifdef SNCP_MODE
void zb_zdo_poll_parent_single(zb_uint8_t param, zb_callback_t cb)
{
  ZB_ASSERT(param);
  ZDO_CTX().pim.single_poll_cb = cb;
  /* ok to call zb_zdo_poll_parent: it checks for poll in progress */
  zb_zdo_poll_parent(param);
}
#endif
/**
   Poll parent.
   This function periodically polls parent to check if it has data for us.
   It runs only when mac_rx_on_when_idle is false.

   @param param - buffer.
*/
static void zb_zdo_poll_parent(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO1, ">>zb_zdo_poll_parent pim %hd joined %d poll_in_progress %d", (FMT__H_D_D, param, ZB_JOINED(), ZDO_CTX().pim.poll_in_progress));
  if (ZB_IS_DEVICE_ZED())
  {
    zb_bool_t is_joined = ZB_JOINED();
    TRACE_MSG(TRACE_ZDO1, ">>zb_zdo_poll_parent pim %hd", (FMT__H, param));

    if (ZDO_CTX().pim.poll_in_progress
        || !is_joined)
    {
      TRACE_MSG(TRACE_ZDO1, "OOPS! poll_in_progress %hd or not_joined %hd. Silently ret.",
                (FMT__H_H, ZDO_CTX().pim.poll_in_progress, !ZB_JOINED()));
      if (param != 0U)
      {
        zb_buf_free(param);
      }
    }
    else
    {
      if (param == 0U)
      {
        if (RET_OK != zb_buf_get_out_delayed(zb_zdo_poll_parent))
        {
          /* We can't just stop poll if have no slot in allocation queue
           * now. Retrying infinitely. */
          TRACE_MSG(TRACE_ZDO1, "No memory for poll, retry after %ld", (FMT__L, ZB_PIM_POLL_ALLOC_TIMEOUT));
          ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_poll_parent, 0);
          ZB_SCHEDULE_ALARM(zb_zdo_poll_parent, 0, ZB_PIM_POLL_ALLOC_TIMEOUT);
        }
      }
      else
      {
        zb_nlme_sync_request_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_sync_request_t);
        request->track = ZB_FALSE; /* not used really.. */
        request->poll_rate = ZDO_CTX().pim.work_poll_interval;

        ZDO_CTX().pim.poll_in_progress = ZB_TRUE;
        if (ZDO_CTX().pim.n_polls_per_data == 0U)
        {
          ZDO_CTX().pim.first_poll_time = ZB_TIMER_GET();
          TRACE_MSG(TRACE_ZDO2, "first poll - remember time %ld", (FMT__L, ZDO_CTX().pim.first_poll_time));
        }
        TRACE_MSG(TRACE_ZDO2, "scheduling nlme sync", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zb_nlme_sync_request, param);
      }
    }
    TRACE_MSG(TRACE_ZDO1, "<<zb_zdo_poll_parent", (FMT__0));
  }
}

/**
   NWK calls this callback when it got poll_confirm from MAC
 */
void zb_nlme_sync_confirm(zb_uint8_t param)
{
  zb_uint16_t max_parent_threshold_retry;
  zb_time_t wait_time;
  zb_ret_t status;
  zb_uint8_t *rejoin_reason= ZB_BUF_GET_PARAM(param, zb_uint8_t);

  TRACE_MSG(TRACE_ZDO1, ">> zb_nlme_sync_confirm pim %hd, status 0x%x", (FMT__H_D, param, zb_buf_get_status(param)));

  if (ZB_IS_DEVICE_ZED())
  {
    zb_bool_t is_joined;

    ZDO_CTX().pim.poll_in_progress = ZB_FALSE;
    /* Just in case. */
    ZB_SCHEDULE_ALARM_CANCEL(zb_zdo_poll_parent, 0);
    TRACE_MSG(TRACE_ZDO3, "call zb_zdo_pim_turbo_poll_adaptation(0)", (FMT__0));
    zb_zdo_pim_turbo_poll_adaptation(0);

    status = zb_buf_get_status(param);

#ifdef SNCP_MODE
    if (ZDO_CTX().pim.single_poll_cb != NULL)
    {
      TRACE_MSG(TRACE_ZDO1, "zb_nlme_sync_confirm shedule callback %p", (FMT__P, ZDO_CTX().pim.single_poll_cb));
      ZB_SCHEDULE_CALLBACK(ZDO_CTX().pim.single_poll_cb, (zb_uint8_t)status);
    }
#endif

    if (status == (zb_ret_t)MAC_SUCCESS
        || status == (zb_ret_t)MAC_NO_DATA )
    {
      /* nothing to do */
      TRACE_MSG(TRACE_ZDO1, "sync status %d - do nothing", (FMT__D, status));
      /* Clear counter on success: do not need to rejoin */
      ZDO_CTX().parent_threshold_retry = 0;
    }
    else
    {
      ZDO_CTX().parent_threshold_retry++;
      max_parent_threshold_retry = ZB_ZDO_GET_MAX_PARENT_THRESHOLD_RETRY();
      /* WWAH Requirements W-2: The device shall perform the wait/retry 2 times for each regularly
       * scheduled data poll that fails.
       * ZB_ZCL_WWAH_MAC_POLL_RETRY_COUNT holds number of retries */
      /* FIXME: 11.2 Determining Parent Connectivity:
         End device utilize the MAC layer to verify Parent Connectivity.
         After 3 consecutive MAC transaction failures (not including those caused by CCA failure), the
         device shall initiate Parent Connectivity Recovery Procedure.
      */
      TRACE_MSG(TRACE_ZDO1, "sync status %d parent_threshold_retry %hd max %d",
                (FMT__D_H_H, status, ZDO_CTX().parent_threshold_retry, max_parent_threshold_retry));

      if (ZDO_CTX().parent_threshold_retry >= max_parent_threshold_retry)
      {
        TRACE_MSG(TRACE_ZDO1, "parent_threshold_retry %hd - rejoin!", (FMT__H, ZDO_CTX().parent_threshold_retry));
        /* Stop ZDO poll. NWK polls during rejoin itself. */
        zb_zdo_pim_stop_poll(0);
        *rejoin_reason = ZB_REJOIN_REASON_PARENT_LOST;
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_initiate_rejoin, param);
        zb_buf_get_out_delayed_ext(zb_zdo_device_zed_start_rejoin, RET_OK, 0);
        param = 0;
      }
    }

    is_joined = ZB_JOINED();
    if ((!ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE())
#ifndef ZB_ROUTER_ROLE
         || (ZB_GET_KEEPALIVE_MODE() == MAC_DATA_POLL_KEEPALIVE)
#endif
          )
        && is_joined
        && !ZDO_CTX().pim.poll_stop
#ifdef SNCP_MODE
        /* if single_poll_cb set, it disable shedule next poll - disable auto-poll */
        && (ZDO_CTX().pim.single_poll_cb == NULL)
#endif
       )
    {
      /* Start polling function if necessary */
      ZB_ZDO_GET_CURENT_POLL_ITERATION_INTERVAL(wait_time);
      TRACE_MSG(TRACE_ZDO1, "scheduling next poll, timeout %ld", (FMT__L, wait_time));
      ZB_SCHEDULE_ALARM(zb_zdo_poll_parent, 0, wait_time);
    }

#ifdef SNCP_MODE
    if (ZDO_CTX().pim.single_poll_cb != NULL)
    {
      ZDO_CTX().pim.single_poll_cb = NULL;
    }
#endif

    if (param != 0U)
    {
      zb_buf_free(param);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zb_nlme_sync_confirm", (FMT__0));
  }
}


#ifdef ZB_ENABLE_ZCL
void zb_zdo_update_long_poll_int(zb_uint8_t param)
{
  zb_zcl_attr_t *attr_desc;
  zb_uint8_t endpoint;

  ZVUNUSED(param);
  if (ZB_IS_DEVICE_ZED())
  {
    TRACE_MSG(TRACE_ZCL1, ">> zb_zdo_update_long_poll_int pim", (FMT__0));

    endpoint = get_endpoint_by_cluster(ZB_ZCL_CLUSTER_ID_POLL_CONTROL, ZB_ZCL_CLUSTER_SERVER_ROLE);

    attr_desc = zb_zcl_get_attr_desc_a(endpoint,
                                       ZB_ZCL_CLUSTER_ID_POLL_CONTROL,
                                       ZB_ZCL_CLUSTER_SERVER_ROLE,
                                       ZB_ZCL_ATTR_POLL_CONTROL_LONG_POLL_INTERVAL_ID);
    ZB_ASSERT(attr_desc);

    zb_zdo_pim_set_long_poll_interval(ZB_QUARTERECONDS_TO_MSEC(ZB_ZCL_GET_ATTRIBUTE_VAL_32(attr_desc)));

    TRACE_MSG(TRACE_ZCL1, "<< zb_zdo_update_long_poll_int", (FMT__0));
  }
}
#endif

#endif  /* ZB_ED_FUNC */
