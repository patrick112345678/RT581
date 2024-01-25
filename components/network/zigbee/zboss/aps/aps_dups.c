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
/* PURPOSE: APS subsystem. Dups detection.
*/

#define ZB_TRACE_FILE_ID 2140
#include "zb_common.h"
#include "zb_aps.h"
#include "aps_internal.h"
#include "zb_scheduler.h"
#include "zb_hash.h"


/*! \addtogroup ZB_APS */
/*! @{ */

static zb_uint8_t aps_get_age_step(void);
static void zb_aps_check_timer_cb(zb_uint8_t param);
static void age_dups(zb_uint8_t step);

#define ZB_APS_DUPS_HASH(aps_counter) (ZB_1INT_HASH_FUNC(aps_counter) % ZB_APS_DUPS_TABLE_SIZE)

zb_aps_dup_tbl_ent_t *aps_check_dups(zb_uint16_t src_addr, zb_uint8_t aps_counter, zb_bool_t is_unicast)
{
  zb_address_ieee_ref_t addr_ref;
  zb_aps_dup_tbl_ent_t *ent = NULL;

  TRACE_MSG(TRACE_APS2, "+aps_check_dups:  addr 0x%x aps_counter %hd is_unicast %hd",
            (FMT__D_H_H, src_addr, aps_counter, is_unicast));

  if (zb_address_by_short(src_addr,
                          ZB_TRUE, /* create if absent (is it ever possible?) */
                          ZB_FALSE, /* do not lock. If entry will be expired,
                                     * that is its fatum. */
                          &addr_ref) == RET_OK)
  {
    zb_ushort_t h_i;
    zb_ushort_t i;
    zb_ushort_t free_i = ZB_APS_DUPS_TABLE_SIZE;
    zb_bool_t found = ZB_FALSE;
#ifdef DEBUG
    zb_uint32_t retries = 0;
#endif
    zb_uint8_t age_step = aps_get_age_step();

    if (age_step != 0U)
    {
      age_dups(age_step);
    }

    /* Loop retrying to find free space in dups table. */
    while (ZB_TRUE)
    {
      h_i = ZB_APS_DUPS_HASH(addr_ref);
      i = h_i;

      TRACE_MSG(TRACE_APS3, "add_ref %hd, h_i %hd", (FMT__H_H, addr_ref, h_i));

      do
      {
        ent = &ZB_APS_DUPS().dups_table[i];

#ifdef APS_DUPS_DEBUG
        TRACE_MSG(TRACE_APS3, "aps_check_dups: ref %hd aps_counter %hd clock %hd is_unicast %hd",
                  (FMT__H_H_H_H, ent->addr_ref, ent->counter, ent->clock, ent->is_unicast));
#endif
        /* Remember the first free entry */
        if (ent->clock == 0U &&
            free_i == ZB_APS_DUPS_TABLE_SIZE)
        {
          free_i = i;
        }
        /* First, find any actual entry */
        else if (ent->clock != 0U
                 && ent->addr_ref == addr_ref
                 && ent->counter == aps_counter
                 && ZB_U2B(ent->is_unicast) == is_unicast)
        {
          TRACE_MSG(TRACE_APS3, "found entry in use %hd, clock %hd",
                    (FMT__H_H, i, ZB_APS_DUPS().dups_table[i].clock));
          found = ZB_TRUE;
        }
        else
        {
          /* MISRA rule 15.7 requires empty 'else' branch. */
        }

        i = (i + 1U) % ZB_APS_DUPS_TABLE_SIZE;
      } while (i != h_i);

      if (!found)
      {
        /* Didn't find any used entry, allocate new */
        if (free_i != ZB_APS_DUPS_TABLE_SIZE)
        {
          TRACE_MSG(TRACE_APS3, "use new entry %hd", (FMT__H, free_i));
#ifdef DEBUG
          if (ZB_U2B(retries))
          {
            TRACE_MSG(TRACE_APS3, "found use new entry %hd after %d retries", (FMT__H_D, free_i, retries));
          }
#endif
          ent = &ZB_APS_DUPS().dups_table[free_i];
        }
        else
        {
          TRACE_MSG(TRACE_APS1, "DUPS table is full", (FMT__0));
          /* Try to prevent trivial case: other device sends us continuous data
           * stream (OTA for instance) and fills entire dups table.
           * But sometimes there can be APS retransmit of the last packet - which we must catch.
           * Force dups table aging to find free space. */
          age_dups(1);
#ifdef DEBUG
          retries++;
#endif
          /* Retry to find a place in dups table */
          continue;
        }
      }
      break;
    } /* retries loop */

    /* ent is either found or free entry */
    if (0U == ent->clock)
    {
      ent->addr_ref = addr_ref;
      ent->counter = aps_counter;
      ent->is_unicast = ZB_B2U(is_unicast);
    }
  }

  TRACE_MSG(TRACE_APS2, "-aps_check_dups ret %p, duplicate=%hd", (FMT__P_H, ent, ((ent ? ent->clock : 0) > 0)));

  /* if packet is not a duplicate, return NULL or return ent with clock is equal to 0 */
  return ent;
}


void aps_clear_dups(zb_uint16_t src_addr)
{
  zb_address_ieee_ref_t addr_ref;
  TRACE_MSG(TRACE_APS2, "aps_clear_dups: addr 0x%x", (FMT__D, src_addr));

  if (zb_address_by_short(src_addr,
                          ZB_TRUE, /* create if absent (is it ever possible?) */
                          ZB_FALSE, /* do not lock. If entry will be expired,
                                     * that is its fatum. */
                          &addr_ref) == RET_OK)
  {
    zb_uindex_t i;
    /* Can be called at LEAVE only, so no meaning to optimize search.. */
    for (i = 0 ; i < ZB_APS_DUPS_TABLE_SIZE ; ++i)
    {
      zb_aps_dup_tbl_ent_t *ent = &ZB_APS_DUPS().dups_table[i];
      if (ent->addr_ref == addr_ref)
      {
        TRACE_MSG(TRACE_APS3, "aps_clear_dups: i %d", (FMT__D, i));
        ent->addr_ref = (zb_uint8_t)~0U;
        ent->clock = 0U;
      }
    }
  }
}


void aps_clear_all_dups(void)
{
  zb_uindex_t i;

  TRACE_MSG(TRACE_APS2, "aps_clear_all_dups", (FMT__0));

  for (i = 0 ; i < ZB_APS_DUPS_TABLE_SIZE ; ++i)
  {
    zb_aps_dup_tbl_ent_t *ent = &ZB_APS_DUPS().dups_table[i];
    ent->addr_ref = (zb_uint8_t)~0U;
    ent->clock = 0U;
  }
  ZB_SCHEDULE_ALARM_CANCEL(zb_aps_check_timer_cb, 0);
  ZG->aps.dups_alarm_running = ZB_FALSE;
}


static void aps_start_dups_alarm(void)
{
  TRACE_MSG(TRACE_APS3, "aps_start_dups_alarm", (FMT__0));
  ZG->aps.dups_alarm_running = ZB_TRUE;
  ZB_SCHEDULE_ALARM_CANCEL(zb_aps_check_timer_cb, 0);
  ZB_SCHEDULE_ALARM(zb_aps_check_timer_cb, 0, ZB_APS_DUP_INITIAL_CLOCK * ZB_APS_DUP_CHECK_TIMEOUT);
  ZG->aps.dups_alarm_start = ZB_TIMER_GET();
}


void aps_update_entry_clock_and_start_aging(zb_aps_dup_tbl_ent_t *ent)
{
  TRACE_MSG(TRACE_APS2, ">>aps_update_entry_clock_and_start_aging %p dups_alarm_running %d",
            (FMT__P_D, ent, ZG->aps.dups_alarm_running));

  ZB_ASSERT(ent);
  ent->clock = ZB_APS_DUP_INITIAL_CLOCK;
  if (!ZG->aps.dups_alarm_running)
  {
    aps_start_dups_alarm();
  }

  TRACE_MSG(TRACE_APS2, "<<aps_update_entry_clock_and_start_aging", (FMT__0));
}


/**
   Alarm callback for the APS dups detection.
 */
static void zb_aps_check_timer_cb(zb_uint8_t param)
{
  ZVUNUSED(param);
  age_dups(ZB_APS_DUP_INITIAL_CLOCK);
  TRACE_MSG(TRACE_APS2, "zb_aps_check_timer_cb: dups_alarm_running %d", (FMT__D, ZG->aps.dups_alarm_running));
}


static zb_uint8_t aps_get_age_step(void)
{
  zb_uint_t step = 0;

  /* If not rx-off-when-idle, do not do alarms optimization.. */
  if (ZG->aps.dups_alarm_running)
  {
    step = ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZG->aps.dups_alarm_start);
    step /= ZB_APS_DUP_CHECK_TIMEOUT;
  }
  TRACE_MSG(TRACE_APS3, "aps_get_age_step ret %d", (FMT__D, step));
  return (zb_uint8_t)step;
}


static zb_bool_t age_entry(zb_aps_dup_tbl_ent_t *ent, zb_uint8_t step)
{
  if (ent->clock != 0U)
  {
    if (ent->clock > step)
    {
      ent->clock -= step;
      TRACE_MSG(TRACE_APS3, "age_entry ent %p step %d set clock %d", (FMT__D_D_D, ent, step, ent->clock));
    }
    else
    {
      ent->clock = 0U;
      TRACE_MSG(TRACE_APS3, "age_entry ent %p step %d set clock 0", (FMT__D_D, ent, step));
    }
  }
  return (ent->clock != 0U);
}


static void age_dups(zb_uint8_t step)
{
  zb_uindex_t i;
  zb_bool_t non_empty = ZB_FALSE;

  for (i = 0; i < ZB_APS_DUPS_TABLE_SIZE; i++)
  {
    zb_aps_dup_tbl_ent_t *ent = &ZB_APS_DUPS().dups_table[i];
    /* Attention: age_entry() has side effects, do not swap conditions or break the loop. */
    non_empty = age_entry(ent, step) || non_empty;
  }
  TRACE_MSG(TRACE_APS3, "age_dups step %d non_empty %d", (FMT__D_D, step, non_empty));
  if (non_empty)
  {
    aps_start_dups_alarm();
  }
  else
  {
    ZG->aps.dups_alarm_running = ZB_FALSE;
    ZB_SCHEDULE_ALARM_CANCEL(zb_aps_check_timer_cb, 0);
  }
}

/*! @} */
