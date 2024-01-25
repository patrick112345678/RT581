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
/* PURPOSE: ZBOSS initialization
*/

#define ZB_TRACE_FILE_ID 8
#include "zb_common.h"

#include "zb_bufpool.h"
#include "zb_ringbuffer.h"
#include "zb_scheduler.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_bdb_internal.h"

#include "ncp/ncp_host_api.h"

/*! \addtogroup ZB_BASE */
/*! @{ */


#ifdef ZB_ENABLE_ZGP
void zb_zgp_init(void);
zb_uint16_t zb_zgp_ctx_size(void);
#endif /* ZB_ENABLE_ZGP */

static void trace_context_sizes(void);

#ifdef ZB_INIT_HAS_ARGS
void zb_init(zb_char_t *trace_comment)
#else
void zb_init(void)
#endif
{
#ifdef ZB_INIT_HAS_ARGS
  ZVUNUSED(trace_comment);
#endif
/*Init ZBOSS global context*/

  zb_globals_init();

#ifdef ZB_CHECK_OOM_STATUS
  ZG->oom_check_enabled = 1;
#endif

  /* FIXME: remove all that shit and use ZB_PLATFORM_INIT everywhere! EE. */
  /* [MM]: 07/24/2015 Commented in scope of ZB PRO'15 certification,
   * should be used ZB_PLATFORM_INIT() instead */

#if defined(ZB_PLATFORM_CM4_SUB_GHZ)
  SystemCoreClockUpdate();
  zb_cortexm4_init();
  EXTI_RFINT2_Init();
  ZB_cm4_sub_ghz_SysTick_start();
#endif

  ZB_PLATFORM_INIT();

  /* Note: for trace over GP hal must call trace init later, in Application_Init() */
#ifdef ZB_INIT_HAS_ARGS
  TRACE_INIT(trace_comment);
#else
  TRACE_INIT("");
#endif

  ZB_ENABLE_ALL_INTER();

  zb_init_buffers();

  zb_sched_init();

#ifdef ZB_USE_SLEEP
  zb_sleep_init();
#endif

#ifdef ZB_ENABLE_ZCL
  zb_zcl_init();
#endif

#if defined ZB_ENABLE_ZLL
  zll_init();
#endif /* defined ZB_ENABLE_ZLL */

#ifdef ZB_ENABLE_ZGP
  zb_zgp_init();
#endif /* ZB_ENABLE_ZGP */

  zdo_commissioning_init();

  ncp_host_init();

  trace_context_sizes();
}

static void trace_context_sizes(void)
{
  /* Note: data size is not informatiove for "configurable mem" builds. */
#ifdef ZB_ED_ROLE
  TRACE_MSG(TRACE_INFO1, "ED build", (FMT__0));
#else
  TRACE_MSG(TRACE_INFO1, "FFD build", (FMT__0));
#endif
  TRACE_MSG(TRACE_INFO1, "sizes: g_zb %d sched %d bpool %d addr %d",
            (FMT__D_D_D_D_D_D_D, (zb_uint16_t)sizeof(g_zb), (zb_uint16_t)sizeof(g_zb.sched),
             (zb_uint16_t)sizeof(g_zb.bpool), (zb_uint16_t)sizeof(g_zb.addr)));
#ifdef ZB_ENABLE_ZCL
  TRACE_MSG(TRACE_INFO1, "zcl %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.zcl)));
#endif
#ifdef ZB_ENABLE_ZLL
  TRACE_MSG(TRACE_INFO1, "zll %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.zll)));
#endif
#ifdef ZB_ENABLE_ZGP
  TRACE_MSG(TRACE_INFO1, "zgp %d", (FMT__D, zb_zgp_ctx_size()));
#endif
#ifdef ZB_USE_NVRAM
  TRACE_MSG(TRACE_INFO1, "nvram %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.nvram)));
#endif
#ifdef ZB_USE_BUTTONS
  TRACE_MSG(TRACE_INFO1, "buttons %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.button)));
#endif
#ifdef ZB_USE_ERROR_INDICATION
  TRACE_MSG(TRACE_INFO1, "err_ind %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.err_ind)));
#endif
#ifdef USE_ZB_WATCHDOG
  TRACE_MSG(TRACE_INFO1, "watchdog %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.watchdog)));
#endif

  TRACE_MSG(TRACE_INFO1, "scheduler q size %d",
            (FMT__D, (zb_uint16_t)ZB_SCHEDULER_Q_SIZE));

#ifdef ZB_CONFIGURABLE_MEM
  TRACE_MSG(TRACE_INFO1, "Configurable mem build, use ZBOSS lib defaults = %d", (FMT__D, gc_use_defaults));
#endif

  TRACE_MSG(TRACE_INFO1, "ZB_IOBUF_POOL_SIZE %d", (FMT__D, ZB_IOBUF_POOL_SIZE));

  /* TODO: trace sizes of all configurable components (ok, ok, we can just look into .map file) */
}

/*! @} */
