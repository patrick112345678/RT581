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
/* PURPOSE: time functions implementation for 8051 for the Common bank
*/


#define ZB_TRACE_FILE_ID 128
#include "zboss_api_core.h"

#include "zb_common.h"
#include "zb_time.h"

#ifndef ZB_ALIEN_TIMER

/*! \addtogroup ZB_BASE */
/*! @{ */


void zb_timer_start(zb_time_t timeout)
{
    zb_time_t t_cur = ZB_TIMER_GET();
    zb_time_t t = ZB_TIME_ADD(t_cur, timeout);
    zb_time_t timer_stop;

    if (ZB_TIMER_CTX().canstop)
    {
        /* ZB_TIMER_CTX can be accessed from Timer ISR in this case */
        ZB_OSIF_GLOBAL_LOCK();
    }

    /* Use separate variable to avoid false-positive MISRAC2012-Rule-13.5 violation */
    timer_stop = ZB_TIMER_CTX().timer_stop;
    if (!ZB_TIMER_CTX().started
            /* must wake earlier then scheduled before */
            || ZB_TIME_GE(timer_stop, t)
            /* already missed our stop time */
            || ZB_TIME_GE(t_cur, timer_stop)
       )
    {
        ZB_TIMER_CTX().timer_stop = t;
        ZB_TIMER_CTX().started = ZB_TRUE;
    }

    if (ZB_TIMER_CTX().canstop)
    {
        /* ZB_TIMER_CTX can be accessed from Timer ISR in this case */
        ZB_OSIF_GLOBAL_UNLOCK();
    }

    if (!ZB_U2B(ZB_CHECK_TIMER_IS_ON()))
    {
        /* timer is stopped - start it */
        ZB_START_HW_TIMER();
    }
    /*
    #ifndef ZB8051
      TRACE_MSG(TRACE_OSIF3, "t_cur %d tmo %d, timer_stop %d", (FMT__D_D_D, t_cur, (int)timeout, (int)ZB_TIMER_CTX().timer_stop));
    #endif
    */
}

void zb_timer_enable_stop(void)
{
    ZB_TIMER_CTX().canstop = ZB_TRUE;
}

void zb_timer_disable_stop(void)
{
    ZB_TIMER_CTX().canstop = ZB_FALSE;
}

void zb_timer_stop()
{
    if (ZB_TIMER_CTX().canstop)
    {
        ZB_TIMER_CTX().started = ZB_FALSE;
        ZB_STOP_HW_TIMER();
    }
}

/*! @} */

#endif                          /* ZB_ALIEN_TIMER */


zb_time_t zb_timer_get()
{
#ifdef ZB_TOOL
    return osif_current_time_to_be();
#else
    return ZB_TIMER_CTX().timer;
#endif  /* ZB_TOOL */
}


/*
 platform-specific zb_timer_get_precise_time() (used only by application/sniffer/zboss_sniffer.c). Moved out of there.
 Temporary keep in commented out form.
 */
#ifdef KILL_WHEN_FINALLY_RESOLVED

void zb_timer_get_precise_time(zb_time_t *bi_num, zb_uint16_t *bi_reminder_us)
{
    zb_time_t bi;
    zb_time_t bi2;
    zb_time_t reminder;

    ZB_ASSERT(bi_num);
    ZB_ASSERT(bi_reminder_us);

    bi = ZB_TIMER_CTX().timer;
    reminder = zb_osif_get_timer_reminder();
    bi2 = ZB_TIMER_CTX().timer;

    /* If timer moved, refine reminder */
    if (bi != bi2)
    {
        reminder = zb_osif_get_timer_reminder();
    }

    *bi_num = bi2;
    *bi_reminder_us = reminder;
}

#endif
