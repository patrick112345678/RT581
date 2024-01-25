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
/* PURPOSE: Zigbee scheduler.
*/

/*! \addtogroup sched */
/*! @{ */
#define ZB_TRACE_FILE_ID 124

#include "zboss_api_core.h"
#include "zboss_api_buf.h"
#ifdef ZB_MINIMAL_CONTEXT

#include "zb_g_context_min.h"
#include "zb_scheduler.h"

#else

#include "zb_common.h"
#include "zb_sleep.h"

#ifndef ZB_ALIEN_MAC
#include "zb_mac.h"
#include "zb_mac_globals.h"
#endif

#endif /* ZB_MINIMAL_CONTEXT */

#ifdef ZB_INTERRUPT_SAFE_ALARMS
#define ZB_ALARM_INT_DISABLE() ZB_OSIF_GLOBAL_LOCK()
#define ZB_ALARM_INT_ENABLE() ZB_OSIF_GLOBAL_UNLOCK()
#else /* ZB_INTERRUPT_SAFE_ALARMS */
#define ZB_ALARM_INT_DISABLE()
#define ZB_ALARM_INT_ENABLE()
#endif /* ZB_INTERRUPT_SAFE_ALARMS */

#ifdef ZB_INTERRUPT_SAFE_CALLBACKS
#define ZB_CB_INT_DISABLE() ZB_OSIF_GLOBAL_LOCK()
#define ZB_CB_INT_ENABLE() ZB_OSIF_GLOBAL_UNLOCK()
#else /* ZB_INTERRUPT_SAFE_CALLBACKS */
#define ZB_CB_INT_DISABLE()
#define ZB_CB_INT_ENABLE()
#endif /* ZB_INTERRUPT_SAFE_CALLBACKS */

#ifndef ZB_MAC_GET_TRANS_INT_FLAG
#define ZB_MAC_GET_TRANS_INT_FLAG() (0U)
#endif

/**
   \par Scheduler and timer management.

   The idea is to be able to stop timer when nobody waits for it.
   That means, current time can't be checked by the application: it does not
   goes forward always.
   The only API is to run callback after some timeout.
   If scheduler have some delayed callback, we must run a timer, so scheduler
   can keep current time which is actual only when we have some delayed
   callbacks.

   Time management has platform-independent and platform-dependent parts.
   Scheduler interface and macros to operate with time are platform-independent.

   Time stored in platform-dependent units (ticks). It can overflow, so operate
   with time only using macros: it handles overflow. Overflow frequency depends
   on time type size and timer resolution and is platform-dependent.

   Timer start and examine are platform-dependent.

   Platform-dependent timer part provides macros to convert between
   platform-dependent time type ('raw' time) and milliseconds.
*/

#if !defined ZB_ALIEN_SCHEDULER

#ifdef MAC_TRANSPORT_USES_SELECT
static zb_ret_t zb_sched_mac_transport_iteration(void);
#endif

#ifdef ZB_LWIP
void zb_sched_register_ethernet_cb(zb_callback_t eth_cb)
{
    MAC_CTX().ethernet_cb = eth_cb;
}
#endif

#if defined ZB_MAC_LOGIC_ITERATION || defined MAC_TRANSPORT_USES_SELECT
static zb_bool_t sched_is_cb_q_empty(void)
{
    zb_bool_t cb_q_is_empty;
    ZB_CB_INT_DISABLE();
    cb_q_is_empty = (ZB_RING_BUFFER_PEEK(ZB_CB_Q) == NULL);
    ZB_CB_INT_ENABLE();
    return cb_q_is_empty;
}
#endif /* ZB_MAC_LOGIC_ITERATION || MAC_TRANSPORT_USES_SELECT */

void zb_sched_loop_iteration(void)
{
    zb_bool_t did_something;
    zb_uint8_t n_quants;

#ifdef ZB_MAC_LOGIC_ITERATION
    zb_ret_t ret_mac;
#endif /* ZB_MAC_LOGIC_ITERATION */

#if defined ZB_PROFILE_STACK_USAGE
    zb_uint16_t stack_usage;
#endif /* ZB_PROFILE_STACK_USAGE */

    /* NK: Debug purpose! */
    /* TRACE_MSG(TRACE_COMMON3, "MAC_CTX().flags.tx_busy %i", (FMT__H, MAC_CTX().flags.tx_busy)); */
    do
    {
        zb_time_t t;
        did_something = ZB_FALSE;
        n_quants = 0;

        ZB_KICK_HW_WATCHDOG();

#if defined( ENABLE_USB_SERIAL_IMITATOR )
        if (ZG->sched.usbc_rx_cb && usbc_has_rx_data())
        {
            (ZG->sched.usbc_rx_cb)(0);
        }
#endif /* defined( ENABLE_USB_SERIAL_IMITATOR ) */
#ifdef ZB_CHECK_OOM_STATUS
        zb_check_oom_status(0);
#endif
#ifdef ZB_LWIP
        if (MAC_CTX().ethernet_cb)
        {
            (*MAC_CTX().ethernet_cb)(0);
        }
#endif

#if defined ZB_PROFILE_STACK_USAGE
        zb_stack_profiler_pre();
#endif  /* ZB_PROFILE_STACK_USAGE */

#if defined ZB_MACSPLIT && defined ZB_MACSPLIT_USE_IO_BUFFERS
        zb_macsplit_transport_recv_bytes();
#endif /* defined ZB_MACSPLIT && defined ZB_MACSPLIT_USE_IO_BUFFERS */
#ifdef ZB_OSIF_SPI_SLAVE
        zb_osif_spi_slave_loop_iteration();
#endif /* ZB_OSIF_SPI_SLAVE */
#ifdef ZB_MAC_LOGIC_ITERATION
        (void)ZB_MAC_LOGIC_ITERATION();
#endif /* ZB_MAC_LOGIC_ITERATION */

#if defined ZB_PROFILE_STACK_USAGE
        stack_usage = zb_stack_profiler_usage();

        TRACE_MSG(TRACE_ERROR, "PROFILER: function 0x%p uses 0x%x bytes",
                  (FMT__P_D, zb_mac_logic_iteration, stack_usage));
#endif  /* ZB_PROFILE_STACK_USAGE */

        /* First execute regular (immediate) callbacks */
        {
            zb_cb_q_ent_t *entp;

            /* Synchroize with zb_schedule_callback_from_alien() */
            ZB_CB_INT_DISABLE();
            while ((entp = ZB_RING_BUFFER_PEEK(ZB_CB_Q)) != NULL)
            {
                zb_bool_t is_2_param = ZB_U2B(ZB_SCHEDULER_IS_2PARAM_CB(ZB_CB_Q->read_i));
                zb_cb_q_ent_t ent = *entp;

                /* For the Linux we need to exit from the loop when, for instance, TERM is received */
                if (ZB_SCHEDULER_IS_STOP())
                {
                    break;
                }

                ZB_CB_INT_ENABLE();
#ifdef ZB_BUF_SHIELD
                if (!is_2_param)
                {
                    /* Use second parameter for an additional paranoidal check. */
                    ZB_ASSERT(ent.user_param == ent.param);
                }
#endif
                TRACE_MSG(TRACE_SPECIAL3, "cb_q written %hd, read_i %hd, write_i %hd  trans flag %hd mac_rx_need_buf %hd",
                          (FMT__H_H_H_H_H, ZB_CB_Q->written, ZB_CB_Q->read_i, ZB_CB_Q->write_i,
                           ZB_MAC_GET_TRANS_INT_FLAG(), zb_buf_get_mac_rx_need()));

                if (
                    /* There is incoming MAC packet pending and it is enough space in scheduler queue to
                     * handle it... */
                    ((ZB_MAC_GET_TRANS_INT_FLAG() != 0U
                      /*cstat !MISRAC2012-Rule-13.5 */
                      /* After some investigation, the following violation of Rule 13.5 seems to be
                       * a false positive. There are no side effect to 'zb_buf_get_mac_rx_need()' or
                       * 'zb_buf_have_rx_bufs()'. This violation seems to be caused by the fact that
                       * 'zb_buf_get_mac_rx_need()' and 'zb_buf_have_rx_bufs()' are external functions,
                       * which cannot be analyzed by C-STAT. */
                      || (zb_buf_get_mac_rx_need() && zb_buf_have_rx_bufs()))
                     && ZB_RING_BUFFER_FREE_SPACE(ZB_CB_Q) > ZB_SCHEDULER_Q_SIZE / 3U)
                    /* ... or maximum number of regular callbacks per iteration is reached */
                    || n_quants >= ZB_CB_QUANT)
                {
                    TRACE_MSG(TRACE_COMMON3, "break", (FMT__0));
                    /* Interrupts are disabled when we are leaving ths loop from
                     * while, so disable it here to keep disable/enable balance
                     * (significant on some platforms where int disable/enable is a mutex really) */
                    ZB_CB_INT_DISABLE();
                    break;
                }
                did_something = ZB_TRUE;
                n_quants++;
                /* Note: there was trace of zb_buf_len(param). Not always
                 * param is a buf ref. It can be addr ref for instance. If buf
                 * is bot busy, buffer pool asserts. */
                TRACE_MSG(TRACE_SPECIAL1, "calling cb %p param %hd",
                          (FMT__P_H, ZB_CB_QENT_FPTR(&ent, is_2_param), ent.param));

                /* NK: Flush cb_q before calling cb.
                   Rationale: we can put another cb to the head inside, so read+flush
                   operation should be atomic.

                   Let's be interrupt-safe: need it at some platforms.
                */
                ZB_CB_INT_DISABLE();
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
                if (!is_2_param)
                {
                    add_buf_schedule_table(ent.param, 1, *ent.u.func_ptr, 0);
                }
                else
                {
                    add_buf_schedule_table(ent.param, 1, 0, *ent.u.func2_ptr);
                }
#endif
                ZB_BYTE_ARRAY_FLUSH_GET(ZB_CB_Q, ZB_SCHEDULER_Q_SIZE);
                ZB_CB_INT_ENABLE();

#if defined ZB_PROFILE_STACK_USAGE
                zb_stack_profiler_pre();
#endif /* ZB_PROFILE_STACK_USAGE */
                if (is_2_param)
                {
                    (*ent.u.func2_ptr)(ent.param, ent.user_param);
                }
                else
                {
                    (*ent.u.func_ptr)(ent.param);
                }
#if defined ZB_PROFILE_STACK_USAGE
                stack_usage = zb_stack_profiler_usage();

                TRACE_MSG(TRACE_ERROR, "PROFILER: function 0x%p uses 0x%x bytes",
                          (FMT__P_D, ZB_CB_QENT_FPTR(&ent, is_2_param), stack_usage));
#endif  /* ZB_PROFILE_STACK_USAGE */

                ZB_CB_INT_DISABLE();
            }
            ZB_CB_INT_ENABLE();
        }

        /* Handle delayed execution */
        {
            zb_uint8_t i =
                ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next);
            zb_tm_q_ent_t *entp = ZG->sched.tm_buffer + i;
            t = ZB_TIMER_GET();
            n_quants = 0;

            ZB_ALARM_INT_DISABLE();
            /* execute all callbacks scheduled to run before current time */
            while ( i != ZP_NULL8
                    && (entp != NULL)
                    && ZB_TIME_GE(t, entp->run_time)
                    && n_quants < ZB_CB_QUANT)
            {
                zb_tm_q_ent_t ent = *entp;
                did_something = ZB_TRUE;
                n_quants++;
                ZB_POOLED_LIST8_CUT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next, i);
                ZB_ALARM_INT_ENABLE();
                /* call the callback */
                TRACE_MSG(TRACE_SPECIAL1, "%p calling alarm cb %p param %hd", (FMT__P_P_H, entp, ent.func, ent.param));

#if defined ZB_PROFILE_STACK_USAGE
                zb_stack_profiler_pre();
#endif  /* ZB_PROFILE_STACK_USAGE */

                ent.func(ent.param);

#if defined ZB_PROFILE_STACK_USAGE
                stack_usage = zb_stack_profiler_usage();

                TRACE_MSG(TRACE_ERROR, "PROFILER: function 0x%p uses 0x%x bytes",
                          (FMT__P_D, ent.func, stack_usage));
#endif /* ZB_PROFILE_STACK_USAGE */

                /* put to the freelist */
                ZB_ASSERT(ZG->sched.tm_buffer_usage > 0U);
                ZB_ALARM_INT_DISABLE();
                ZG->sched.tm_buffer_usage--;
                ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_freelist, next, i);
                i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next);
                entp = ZG->sched.tm_buffer + i;
            }
            ZB_ALARM_INT_ENABLE();

            /*
             * If have something to run later, restart hw
             * timer.
             */
        }
        {
            zb_bool_t list_is_empty;
            ZB_CB_INT_DISABLE();
            list_is_empty = ZB_POOLED_LIST8_IS_EMPTY(ZG->sched.tm_queue);
            ZB_CB_INT_ENABLE();
            if (!list_is_empty)
            {
                zb_time_t new_timeout;

                ZB_ALARM_INT_DISABLE();
                new_timeout = ZB_TIME_SUBTRACT(
                                  (ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next)
                                   + ZG->sched.tm_buffer)->run_time,
                                  t);
                ZB_ALARM_INT_ENABLE();
                zb_timer_start(new_timeout);
            }
            else
            {
#ifndef ZB_NEVER_STOP_TIMER
                zb_timer_stop();
#endif
            }
        }


    } while (did_something);

    /*
      Q: can it work as busy-wait loop waiting for transiever interrupt?

      A: Sure, but then RF circle of transceiver should not
      be disabled, and I'm not sure, that it's a good idea in terms of power
      saving
    */

    /*  TRACE_MSG(TRACE_SPECIAL1, "!MAC_CTX().tx_wait_cb %hd",
        (FMT__H, !MAC_CTX().tx_wait_cb));
    */

#ifdef MAC_TRANSPORT_USES_SELECT
    ret_mac = zb_sched_mac_transport_iteration();
#else
    /* STACK PROFILER: */
#if defined ZB_PROFILE_STACK_USAGE
    zb_stack_profiler_pre();
#endif  /* ZB_PROFILE_STACK_USAGE */
#ifdef ZB_MAC_LOGIC_ITERATION
    ret_mac = ZB_MAC_LOGIC_ITERATION();
#endif /* ZB_MAC_LOGIC_ITERATION */

#if defined ZB_PROFILE_STACK_USAGE
    stack_usage = zb_stack_profiler_usage();

    TRACE_MSG(TRACE_ERROR, "PROFILER: function 0x%p uses 0x%x bytes",
              (FMT__P_D, zb_mac_logic_iteration, stack_usage));
#endif  /* ZB_PROFILE_STACK_USAGE */
#endif

#if defined ZB_SERIAL_FOR_TRACE
    /* TODO: To be moved out to application control etc. */
    ZB_OSIF_SERIAL_FLUSH();
#endif /* ZB_SERIAL_FOR_TRACE */



#ifdef ZB_MAC_LOGIC_ITERATION
    if (ret_mac == RET_BLOCKED)
    {
        /* If MAC is idle.. */
        if (sched_is_cb_q_empty())
        {
            /* Try any kind of sleep only if no callbacks in the queue */
#ifdef ZB_USE_SLEEP
            /* If we can put MCU asleep keeping radio working, send "can sleep" signal to every device, even non-sleepy. */
#ifdef ZB_MAC_RADIO_CANT_WAKEUP_MCU
            if ((ZB_IS_DEVICE_ZED()
                    /*cstat -MISRAC2012-Rule-13.5 */
                    /* After some investigation, the following violation of Rule 13.5 seems to be a false
                     * positive. There are no side effect to 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()'. This violation
                     * seems to be caused by the fact that 'ZB_PIBCACHE_RX_ON_WHEN_IDLE()' is an external
                     * macro, which cannot be analyzed by C-STAT. */
                    && !ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
#ifdef SNCP_MODE
                    || (ZB_NWK_DEVICE_TYPE_NONE == ZB_NIB().device_type)
#endif
               )
#endif
            {
                zb_uint32_t sleep_tmo = zb_sleep_calc_sleep_tmo();

#ifdef NO_THESHOLD_ON_REPEATED_SLEEP
                /* TODO: attempt to reduce extra awake/sleep on the NCP hardware */
                if (ZB_TIME_SUBTRACT(ZB_TIMER_GET(), ZG->sleep.last_timestamp) >= ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZG->sleep.threshold))
#endif
                {
                    ZG->sleep.permit_sleep_signal = ZB_TRUE;
                }

                if (sleep_tmo > ZG->sleep.threshold && ZG->sleep.permit_sleep_signal == ZB_TRUE)
                {
                    ZG->sleep.permit_sleep_signal = ZB_FALSE;
                    ZG->sleep.last_timestamp = ZB_TIMER_GET();
                    /* Note that sleep != blocking waiting for i/o for nsng or macsplit host. Sleep means putting MCU asleep with or without switching radio off.
                      MAC decides about radio switch off itself.
                      Can put ZR/ZC to sleep only if radio can be ON and can generate an interrupt waking up MCP when it is asleep.
                      Builds waiting for Host-side i/o. like nsng or mac split can't generate an interrupt, so only sleepy ZED can slip.
                    */
                    zb_sleep_can_sleep(sleep_tmo);
                }
                else
                {
                    /* Not sure why we put MCU asleep unconditionally here. Because of too short sleep period? */
                    ZB_GO_IDLE();
                }
            }

#endif /* ZB_USE_SLEEP */
        }
    }
#endif /* ZB_MAC_LOGIC_ITERATION */
}

#ifdef MAC_TRANSPORT_USES_SELECT
static zb_ret_t zb_sched_mac_transport_iteration(void)
{
    if (!ZB_MAC_ALLOWS_TRANSPORT_ITERATION() || !sched_is_cb_q_empty())
    {
        /* if have some callbacks to run or MAC does not allow to perform blocking iterations
         * (has some ready packet to send and radio is not already busy because of sending any packet),
         * can't perform blocking iteration. But still need to run MAC transport
         * to have a chance to receive something from nsng/macsplit/etc */
        ZB_TRANSPORT_NONBLOCK_ITERATION();
    }
    else
    {
        ZB_TRANSPORT_BLOCK();
    }

    if (ZB_SCHEDULER_IS_STOP())
    {
        return RET_EXIT;
    }

    return zb_mac_logic_iteration();
}
#endif


void zb_sched_register_usbc_rx_cb(zb_callback_t usbc_rx_cb)
{
#if ! defined( ENABLE_USB_SERIAL_IMITATOR )
    ZVUNUSED(usbc_rx_cb);
#else
    TRACE_MSG(TRACE_COMMON1, "> zb_sched_register_usbc_rx_cb %p", (FMT__P, usbc_rx_cb));

    ZG->sched.usbc_rx_cb = usbc_rx_cb;

    TRACE_MSG(TRACE_COMMON1, "< zb_sched_register_usbc_rx_cb", (FMT__0));
#endif /* ! defined( ENABLE_USB_SERIAL_IMITATOR ) */
}

#ifndef ZB_MINIMAL_CONTEXT
zb_ret_t zb_schedule_tx_cb(zb_callback_t func, zb_uint8_t param, zb_uint8_t prior)
{
    zb_ret_t ret = RET_OK;
    zb_mac_cb_ent_t *ent;
    TRACE_MSG(TRACE_COMMON1, "> zb_schedule_tx_cb func %p param %hd", (FMT__P_H, func, param));

    ZB_CB_INT_DISABLE();
    ent = ZB_RING_BUFFER_PUT_RESERVE(&ZG->sched.mac_tx_q);
    if (ent != NULL)
    {
        ent->func = func;
        ent->param = param;
        if ((prior > 0U) || (ZB_RING_BUFFER_FREE_SPACE(&ZG->sched.mac_tx_q) > ZB_SCHED_TX_CB_HIGH_PRIOR_RESERVE))
        {
            ZB_RING_BUFFER_FLUSH_PUT(&ZG->sched.mac_tx_q);
            TRACE_MSG(TRACE_COMMON1, "%p scheduled mac cb %p param %hd",
                      (FMT__P_P_H, ent, ent->func, ent->param));
        }
        else
        {
            TRACE_MSG(TRACE_COMMON1, "no free space for schedule", (FMT__0));
            ret = RET_OVERFLOW;
        }
    }
    ZB_CB_INT_ENABLE();

    if (ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "MAC callbacks rb overflow! param %hd", (FMT__H, param));
        ret = RET_OVERFLOW;
    }

    TRACE_MSG(TRACE_COMMON1, "< zb_schedule_tx_cb", (FMT__0));
    return ret;
}
#endif /* ZB_MINIMAL_CONTEXT */


#endif  /* !defined ALIEN_SCHEDULER */

#ifdef ZB_STACK_REGRESSION_TESTING_API
/* Used by regression test RTP_INT_01 */
#define REGRESSION_TESTS_API_SCHED_INT_CB()                    \
do                                                             \
{                                                              \
  if(ZB_REGRESSION_TESTS_API().sched_int_cb != NULL)           \
  {                                                            \
    ZB_REGRESSION_TESTS_API().sched_int_cb(0);                 \
  }                                                            \
} while(ZB_FALSE)

/* Used by regression test RTP_INT_01 */
#define REGRESSION_TESTS_API_SCHED_CB_INT_CALLED()             \
do                                                             \
{                                                              \
  if(ZB_REGRESSION_TESTS_API().sched_cb_int_called)    \
  {                                                            \
    ZB_REGRESSION_TESTS_API().sched_cb_int_called = ZB_FALSE;  \
  }                                                            \
} while(ZB_FALSE)

#else

#define REGRESSION_TESTS_API_SCHED_INT_CB()
#define REGRESSION_TESTS_API_SCHED_CB_INT_CALLED()

#endif /* ZB_STACK_REGRESSION_TESTING_API */

zb_ret_t zb_schedule_callback(zb_callback_t func, zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_cb_q_ent_t *ent = NULL;

    ZB_CB_INT_DISABLE();
    if (ZB_RING_BUFFER_USED_SPACE(ZB_CB_Q) < ZB_SCHEDULER_Q_SIZE)
    {
        /* Low priotity callback */
        ent = ZB_RING_BUFFER_GETW(ZB_CB_Q);
    }

    REGRESSION_TESTS_API_SCHED_INT_CB();

    if (ent != NULL)
    {
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
        add_buf_schedule_table(param, 0, func, 0);
#endif
        zb_size_t i = (zb_size_t)(ent - ZB_CB_Q->ring_buf);
        ent->u.func_ptr = func;
        ent->param = param;

        /* Single parameter callback */
        ZB_SCHEDULER_RESET_2PARAM_CB(i);
#ifdef ZB_BUF_SHIELD
        /* Let's duplicate param to verify it just before cb run: catch races
         * with ISR. */
        ent->user_param = param;
#endif

        /* Low priotity callback */
        ZB_BYTE_ARRAY_FLUSH_PUT(ZB_CB_Q, ZB_SCHEDULER_Q_SIZE);

        TRACE_MSG(TRACE_SPECIAL1, "%p scheduled cb %p param %hd",
                  (FMT__P_P_H, ent, ent->u.func_ptr, ent->param));
    }

    REGRESSION_TESTS_API_SCHED_CB_INT_CALLED();

    ZB_CB_INT_ENABLE();

    if (ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "Callbacks rb overflow! param %hd", (FMT__H, param));
        //ZB_ASSERT(0);
        ret = RET_OVERFLOW;
    }

    return ret;
}

zb_ret_t zb_schedule_callback2(zb_callback2_t func, zb_uint8_t param, zb_uint16_t user_param)
{
    zb_ret_t ret = RET_OK;
    zb_cb_q_ent_t *ent = NULL;

    ZB_CB_INT_DISABLE();
    if (ZB_RING_BUFFER_USED_SPACE(ZB_CB_Q) < ZB_SCHEDULER_Q_SIZE)
    {
        /* Low priotity callback */
        ent = ZB_RING_BUFFER_GETW(ZB_CB_Q);
    }

    REGRESSION_TESTS_API_SCHED_INT_CB();

    if (ent != NULL)
    {
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
        add_buf_schedule_table(param, 0, 0, func);
#endif
        zb_size_t i = (zb_size_t)(ent - ZB_CB_Q->ring_buf);
        ent->u.func2_ptr = func;
        ent->param = param;

        /* Two parameter callback */
        ZB_SCHEDULER_SET_2PARAM_CB(i);
        ent->user_param = user_param;

        /* Low priotity callback */
        ZB_BYTE_ARRAY_FLUSH_PUT(ZB_CB_Q, ZB_SCHEDULER_Q_SIZE);
        TRACE_MSG(TRACE_SPECIAL1, "%p scheduled cb %p param %hd",
                  (FMT__P_P_H, ent, ent->u.func2_ptr, ent->param));
        TRACE_MSG(TRACE_SPECIAL3, "extra params user_param %d",
                  (FMT__D, user_param));
    }

    REGRESSION_TESTS_API_SCHED_CB_INT_CALLED();

    ZB_CB_INT_ENABLE();

    if (ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "Callbacks rb overflow! param %hd", (FMT__H, param));
        ZB_ASSERT(0);
        ret = RET_OVERFLOW;
    }

    return ret;
}

zb_ret_t zb_schedule_callback_prior(zb_callback_t func, zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;
    zb_cb_q_ent_t *ent = NULL;

    ZB_CB_INT_DISABLE();
    if (ZB_RING_BUFFER_USED_SPACE(ZB_CB_Q) < ZB_SCHEDULER_Q_SIZE)
    {
        /* High priotity callback */
        ent = ZB_BYTE_ARRAY_PUT_HEAD_RESERVE(ZB_CB_Q, ZB_SCHEDULER_Q_SIZE);
    }

    REGRESSION_TESTS_API_SCHED_INT_CB();

    if (ent != NULL)
    {
#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
        add_buf_schedule_table(param, 0, func, 0);
#endif
        zb_size_t i = (zb_size_t)(ent - ZB_CB_Q->ring_buf);
        ent->u.func_ptr = func;
        ent->param = param;

        /* Single parameter callback */
        ZB_SCHEDULER_RESET_2PARAM_CB(i);
#ifdef ZB_BUF_SHIELD
        /* Let's duplicate param to verify it just before cb run: catch races
         * with ISR. */
        ent->user_param = param;
#endif

        /* High priotity callback */
        ZB_BYTE_ARRAY_FLUSH_PUT_HEAD(ZB_CB_Q, ZB_SCHEDULER_Q_SIZE);

        TRACE_MSG(TRACE_SPECIAL1, "%p scheduled cb %p param %hd",
                  (FMT__P_P_H, ent, ent->u.func_ptr, ent->param));
    }

    REGRESSION_TESTS_API_SCHED_CB_INT_CALLED();

    ZB_CB_INT_ENABLE();

    if (ent == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "Callbacks rb overflow! param %hd", (FMT__H, param));
        ZB_ASSERT(0);
        ret = RET_OVERFLOW;
    }

    return ret;
}

zb_ret_t zb_schedule_app_callback(zb_callback_t func, zb_uint8_t param)
{
    return (ZB_RING_BUFFER_USED_SPACE(ZB_CB_Q) <= ZB_SCHEDULER_Q_SIZE - ZB_SCHEDULER_Q_SIZE_PROTECTED_STACK_POOL ) ?
           zb_schedule_callback(func, param) : RET_OVERFLOW;
}

zb_ret_t zb_schedule_app_callback2(zb_callback2_t func, zb_uint8_t param, zb_uint16_t user_param)
{
    return (ZB_RING_BUFFER_USED_SPACE(ZB_CB_Q) <= ZB_SCHEDULER_Q_SIZE - ZB_SCHEDULER_Q_SIZE_PROTECTED_STACK_POOL ) ?
           zb_schedule_callback2(func, param, user_param) : RET_OVERFLOW;
}

#if defined ZB_TRACE_LEVEL && defined ZB_TRACE_MASK
void zb_scheduler_trace_file_line(zb_uint32_t file_id, zb_uint32_t line_number, zb_callback_t func)
{
    TRACE_MSG(TRACE_ERROR, "schedule alarm %p from ZB_TRACE_FILE_ID %ld line %ld n scheduled %d", (FMT__P_L_L_D, func, file_id, line_number, (zb_uint16_t)(ZG->sched.tm_buffer_usage + 1)));
}
#endif

#ifndef ZB_ALIEN_MAC
/**
   Schedule callback from another thread
 */
void zb_schedule_callback_from_alien(zb_callback_t func, zb_uint8_t param)
{
    /* There were int disable. Now schedule_callback is interrupt-safe. */
    ZB_SCHEDULE_CALLBACK(func, param);
}
#endif

/* run_after time is specified in beacon intervals, convert it to
 * internal time representation: milliseconds for */
zb_ret_t zb_schedule_alarm(zb_callback_t func, zb_uint8_t param, zb_time_t timeout_bi) /* __reentrant for sdcc, to save DSEG space */
{
    zb_ret_t ret = RET_OK;
    zb_tm_q_ent_t *nent;
    zb_uint8_t i;
    zb_time_t t;

    if (timeout_bi == 0U)
    {
        /* Convert alarm with time 0 to conventional callback */
        ret = zb_schedule_callback(func, param);
        if (ret == RET_OK)
        {
            return ret;
        }
    }

    /* allocate entry - get from the freelist */
    ZB_ALARM_INT_DISABLE();
    ZB_POOLED_LIST8_CUT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_freelist, next, i);
    ZB_ALARM_INT_ENABLE();
    if (i != ZP_NULL8)
    {
        nent = ZG->sched.tm_buffer + i;
        TRACE_MSG(TRACE_COMMON3, "tm_freelist, i=%hd get nent %p", (FMT__H_P, i, (void *)nent));
        nent->func = func;
        nent->param = param;

        t = ZB_TIMER_GET();
        nent->run_time = ZB_TIME_ADD(t, timeout_bi);

        TRACE_MSG(TRACE_COMMON3, "%p scheduled alarm %p, timeout_bi %ld, at %ld, param %hd",
                  (FMT__P_P_L_L_H,
                   nent, nent->func, timeout_bi,
                   nent->run_time, (int)nent->param));

        ZB_ALARM_INT_DISABLE();
        ZG->sched.tm_buffer_usage++;
        ZB_ALARM_INT_ENABLE();
        TRACE_MSG(TRACE_COMMON3, "tm_buffer_usage %hd", (FMT__H, ZG->sched.tm_buffer_usage));

#ifdef ZB_TRACE_LEVEL
        /* Note: it will not work when called from interrupt */
        if (ZG->sched.tm_buffer_usage > ZB_SCHEDULER_Q_SIZE - 3)
        {
            zb_uint8_t j;
            zb_tm_q_ent_t *ent;
            TRACE_MSG(TRACE_ERROR, "alarms q of size %d q size %d", (FMT__D_D, ZG->sched.tm_buffer_usage, ZB_SCHEDULER_Q_SIZE));
            ZB_POOLED_LIST8_ITERATE(ZG->sched.tm_buffer, ZG->sched.tm_queue, next, j)
            {
                ent = ZG->sched.tm_buffer + j;
                TRACE_MSG(TRACE_ERROR, "q ent %hd func %p runtime %ld param %hd",
                          (FMT__H_P_L_H, j, ent->func, ent->run_time, ent->param));
            }
        }
#endif

        ZB_ALARM_INT_DISABLE();
        if (ZB_POOLED_LIST8_IS_EMPTY(ZG->sched.tm_queue)
                /* new time is before queue start */
                || ZB_TIME_GE(
                    (ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next)
                     + ZG->sched.tm_buffer)->run_time, nent->run_time))
        {
            TRACE_MSG(TRACE_COMMON3, "insert first", (FMT__0));
            ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next, i);
        }
        else
        {
            zb_uint8_t elem_index;
            i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next);
            while (i != ZP_NULL8
                    && ZB_TIME_GE(nent->run_time, ZG->sched.tm_buffer[i].run_time)) // nent->run_time > ZG->sched.tm_buffer[i].run_time
            {
                i = ZB_POOLED_LIST8_NEXT(ZG->sched.tm_buffer, i, next);
            }

            elem_index = (zb_uint8_t)(nent - ZG->sched.tm_buffer);
            if (i != ZP_NULL8)
            {
                i = ZB_POOLED_LIST8_PREV(ZG->sched.tm_buffer, i, next);

                if (i != ZP_NULL8)
                {
                    /* insert after found entry */
                    ZB_POOLED_LIST8_INSERT_AFTER( ZG->sched.tm_buffer,
                                                  ZG->sched.tm_queue,
                                                  next,
                                                  i,
                                                  elem_index);
                }
                else
                {
                    /* insert head */
                    ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer,
                                                ZG->sched.tm_queue,
                                                next,
                                                elem_index);
                }
            }
            else
            {
                /* insert tail */
                ZB_POOLED_LIST8_INSERT_TAIL(ZG->sched.tm_buffer,
                                            ZG->sched.tm_queue,
                                            next,
                                            elem_index);
            }
        }
        ZB_ALARM_INT_ENABLE();

#if 0
        TRACE_MSG(TRACE_COMMON3, "current time %d", (FMT__D, t));
        ZB_LIST_ITERATE(ZG->sched.tm_queue, next, ent)
        {
            TRACE_MSG(TRACE_COMMON3, "entry %p cb %p run_time %d", (FMT__P_P_D, (void *)ent, ent->func, ent->run_time));
        }
#endif


#ifndef ZB_ALIEN_TIMER
        /*
         * If list head must be executed just now, not need to start
         * a timer: will execute it immediately in zb_sched_loop_iteration()
         */
        ZB_ALARM_INT_DISABLE();
        nent = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next) + ZG->sched.tm_buffer;
        t = ZB_TIMER_GET();
        TRACE_MSG(TRACE_COMMON3, "started %d cur t %d, head t %d", (FMT__D_D_D, ZB_TIMER_CTX().started, t, nent->run_time));
        if (nent->run_time != t
                && ZB_TIME_GE(nent->run_time, t))
        {
            zb_time_t new_timeout = ZB_TIME_SUBTRACT(nent->run_time, t);
            ZB_ALARM_INT_ENABLE();
            zb_timer_start(new_timeout);
        }
        else
        {
            ZB_ALARM_INT_ENABLE();
        }
        TRACE_MSG(TRACE_COMMON3, "cur t %d, head t %d timer_stop %d",
                  (FMT__D_D_D, t, nent->run_time, ZB_TIMER_CTX().timer_stop));

#endif  /* ZB_ALIEN_TIMER */
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "schedule alarm FAILED (no place)", (FMT__0));
        //ZB_ASSERT(0);
        ret = RET_OVERFLOW;
    }
    return ret;
}

zb_ret_t zb_schedule_app_alarm(zb_callback_t func, zb_uint8_t param, zb_time_t run_after)
{
    zb_ret_t ret;

    if (run_after == 0U)
    {
        /* Convert alarm with time 0 to conventional callback */
        ret = zb_schedule_app_callback(func, param);
    }
    else
    {
        ret = (ZG->sched.tm_buffer_usage <= ZB_SCHEDULER_Q_SIZE - ZB_SCHEDULER_Q_SIZE_PROTECTED_STACK_POOL ) ?
              zb_schedule_alarm(func, param, run_after) : RET_OVERFLOW;
    }

    return ret;
}

zb_ret_t zb_schedule_alarm_cancel(zb_callback_t func, zb_uint8_t param, zb_uint8_t *p_param) /* __reentrant for sdcc, to save DSEG space */
{
    zb_ret_t ret = RET_NOT_FOUND;
    zb_tm_q_ent_t *ent;
    zb_uint8_t i;
    zb_uint8_t nexti;

    if (p_param != NULL)
    {
        *p_param = 0;
    }

    ZB_ALARM_INT_DISABLE();
    /* go through alarms list */
    for (i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next) ;
            i != ZP_NULL8 ; i = nexti)
    {
        nexti = ZB_POOLED_LIST8_NEXT(ZG->sched.tm_buffer, i, next);
        ent = ZG->sched.tm_buffer + i;
        if (ent->func == func && (param == ZB_ALARM_ANY_PARAM || param == ZB_ALARM_ALL_CB || param == ent->param))
        {
            TRACE_MSG(TRACE_COMMON3, "%p alarm cancel func %p, param %hd", (FMT__P_P_H, ent, ent->func, ent->param));
            /* remove from scheduled alarms, add to free list */
            if ((p_param != NULL) && (ent->param != 0U))
            {
                *p_param = ent->param;
            }
            //      TRACE_MSG(TRACE_ERROR, "cancel alarm %p scheduled %d", (FMT__P_D, ent->func, (zb_uint16_t)(ZG->sched.tm_buffer_usage-1)));
            ZB_POOLED_LIST8_REMOVE(ZG->sched.tm_buffer, ZG->sched.tm_queue, next, i);
            ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_freelist, next, i);
            ZB_ASSERT(ZG->sched.tm_buffer_usage > 0U);
            ZG->sched.tm_buffer_usage--;
            TRACE_MSG(TRACE_COMMON3, "tm_buffer_usage %hd", (FMT__H, ZG->sched.tm_buffer_usage));
            TRACE_MSG(TRACE_COMMON3, "tm_queue -> tm_freelist, ent %p", (FMT__P, (void *)ent));
            ret = RET_OK;
            if (param != ZB_ALARM_ALL_CB)
            {
                break;
            }
        }
    }
    ZB_ALARM_INT_ENABLE();

    return ret;
}

zb_ret_t zb_schedule_get_alarm_time(zb_callback_t func, zb_uint8_t param, zb_time_t *timeout_bi)
{
    zb_ret_t ret = RET_NOT_FOUND;
    zb_tm_q_ent_t *ent;
    zb_uindex_t i;

    ZB_ALARM_INT_DISABLE();
    /* go through alarms list */
    for (i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next) ;
            i != ZP_NULL8 ; i = ZB_POOLED_LIST8_NEXT(ZG->sched.tm_buffer, i, next))
    {
        ent = ZG->sched.tm_buffer + i;
        if (ent->func == func && (param == ent->param || param == ZB_ALARM_ANY_PARAM))
        {
            TRACE_MSG(TRACE_COMMON3, "zb_schedule_get_alarm_time ent %p func %p, param %hd time %d", (FMT__P_P_H_D, ent, ent->func, ent->param, ent->run_time));
            *timeout_bi = ent->run_time;
            ret = RET_OK;
            break;
        }
    }
    ZB_ALARM_INT_ENABLE();

    return ret;
}
#if defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE     /* Zigbee pro */

#ifndef ZB_MINIMAL_CONTEXT
zb_uint8_t zb_schedule_alarm_cancel_compare(zb_callback_t func, zb_callback_compare_t comp, void *param)
{
    zb_uint8_t ret = 0;
    zb_tm_q_ent_t *ent;
    zb_uint8_t i;
    zb_uint8_t nexti;

    ZB_ALARM_INT_DISABLE();
    /* go through alarms list */
    for (i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next) ;
            i != ZP_NULL8 ; i = nexti)
    {
        nexti = ZB_POOLED_LIST8_NEXT(ZG->sched.tm_buffer, i, next);
        ent = ZG->sched.tm_buffer + i;
        if (ent->func == func)
        {
            zb_bool_t is_equal = comp(ent->param, param);
            if (is_equal)
            {
                TRACE_MSG(TRACE_COMMON3, "cancel alarm %p scheduled %d", (FMT__P_D, ent->func, (zb_uint16_t)(ZG->sched.tm_buffer_usage - 1)));
                TRACE_MSG(TRACE_COMMON3, "%p alarm cancel func %p, param %hd", (FMT__P_P_H, ent, ent->func, ent->param));
                /* remove buffer*/
                ret = ent->param;
                /* remove from scheduled alarms, add to free list */
                ZB_POOLED_LIST8_REMOVE(ZG->sched.tm_buffer, ZG->sched.tm_queue, next, i);
                ZB_POOLED_LIST8_INSERT_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_freelist, next, i);
                ZB_ASSERT(ZG->sched.tm_buffer_usage > 0U);
                ZG->sched.tm_buffer_usage--;
                TRACE_MSG(TRACE_COMMON3, "tm_buffer_usage %hd", (FMT__H, ZG->sched.tm_buffer_usage));
                TRACE_MSG(TRACE_COMMON3, "tm_queue -> tm_freelist, ent %p", (FMT__P, (void *)ent));
                break;
            }
        }
    }
    ZB_ALARM_INT_ENABLE();
    return ret;
}
#endif /* ZB_MINIMAL_CONTEXT */

#endif

#ifdef ZB_DEBUG_BUFFERS_EXT
void zb_schedule_trace_queue()
{
    zb_uint_t i;
    zb_cb_q_ent_t *ent;
    zb_tm_q_ent_t *ent_sch;

    for (i = 0; i < ZB_CB_Q->written; i++)
    {
        ent = ZB_RING_BUFFER_SEARCH_GET(ZB_CB_Q, i);
        TRACE_MSG(TRACE_ERROR, "schedule_queue func %p param %hd",
                  (FMT__P_H, i, ent->u.func_ptr, ent->param));
    }

    i = ZB_POOLED_LIST8_GET_HEAD(ZG->sched.tm_buffer, ZG->sched.tm_queue, next);
    while (i != ZP_NULL8)
    {
        ent_sch = ZG->sched.tm_buffer + i;
        if (ent_sch->param > 0 && ent_sch->param < ZB_N_BUF_IDS)
        {
            TRACE_MSG(TRACE_ERROR, "schedule_alarm_queue ent %hd func %p runtime %ld param %hd",
                      (FMT__H_P_L_H, i, ent_sch->func, ent_sch->run_time, ent_sch->param));
        }
        i = ZB_POOLED_LIST8_NEXT(ZG->sched.tm_buffer, i, next);
    }
}
#endif

#if defined ZB_DEBUG_BUFFERS_EXT_RAF && defined ZB_DEBUG_BUFFERS_RAF_SCHEDULE_RECORD
static void add_buf_schedule_table(zb_uint8_t param, zb_bool_t usage, zb_callback_t func, zb_callback2_t func2)
{
    zb_buf_ent_t   *zbbuf;
    zb_uint8_t      i;

    /* Warning: The input param might not be buffer id! User should ignore the data when this condition happens */
    if (param > 0 && param <= ZB_IOBUF_POOL_SIZE)
    {
        zbbuf = zb_bufpool_storage_bufid_to_buf((param) - 1);

        i = 0;
        for (i = 0; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
        {
            if (zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].used == 0)
            {
                break;
            }
        }

        if (i == ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT)
        {
            for (i = 1; i < ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT; i++)
            {
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].time = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time;
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].func = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].func;
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].func2 = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].func2;
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].usage = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].usage;
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].used = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].used;
                zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i - 1].schedulerid = zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].schedulerid;
            }
            i = ZB_DEBUG_BUFFERS_EXT_RAF_RECORD_COUNT - 1;
        }

        zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].time = ZB_TIMER_GET();
        zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].func = func;
        zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].func2 = func2;
        zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].usage = usage;
        zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].used = 1;
        if (usage == 0)
        {
            zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].schedulerid = (ZB_CB_Q)->write_i;
        }
        else
        {
            zbbuf->buf_schedule_table[ZB_DEBUG_BUFFERS_EXT_RAF_ROUND_COUNT - 1][i].schedulerid = (ZB_CB_Q)->read_i;
        }
    }
}

#endif

/*! @} */
