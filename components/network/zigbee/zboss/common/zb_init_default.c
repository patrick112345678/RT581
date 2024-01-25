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

#define ZB_TRACE_FILE_ID 119
#include "zb_common.h"

#include "zb_bufpool.h"
#include "zb_ringbuffer.h"
#include "zb_scheduler.h"
#include "zb_mac_transport.h"
#ifndef ZB_MACSPLIT_DEVICE
#include "zb_aps.h"
#endif
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_bdb_internal.h"

/*! \addtogroup ZB_BASE */
/*! @{ */

#ifdef RELEASE
zb_uint32_t zboss_library_revision_get(void);
zb_uint32_t zboss_library_revision_modified(void);
zb_uint32_t zboss_library_revision_mixed(void);
zb_uint8_t * zboss_library_revision_range(void);
#endif



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

#if (!MODULE_ENABLE(SUPPORT_FREERTOS_PORT))
  ZB_PLATFORM_INIT();
#endif

  /* Note: for trace over GP hal must call trace init later, in Application_Init() */
#ifdef ZB_INIT_HAS_ARGS
  TRACE_INIT(trace_comment);
#else
  TRACE_INIT("");
#endif
#if defined ZB_USE_NVRAM
#ifdef ZB_INIT_HAS_ARGS
  zb_osif_nvram_init(trace_comment);
#else
  zb_osif_nvram_init("");
#endif
#endif

#ifdef ZB_USE_NVRAM
  ZB_AIB().aps_use_nvram = 1;
#endif

  zb_init_buffers();

  zb_ib_set_defaults((char*)"");

  zb_sched_init();
  /* some init of 8051 HW moved to zb_low_level_init() */
  /* This is preliminary MAC init. Mode init done in MLME-START.request */
  zb_mac_init();

  zb_mac_transport_init();
#ifndef ZB_MACSPLIT_DEVICE
  zb_nwk_init();
#endif

#ifdef ZB_USE_SLEEP
  zb_sleep_init();
#endif

#if defined ZB_NVRAM_WRITE_CFG && defined ZB_USE_NVRAM && defined C8051F120
/* Write config to nvram. Think there's no any reason to invoke this second time*/
/*
  zb_uint8_t aps_designated_coord
  zb_uint8_t aps_use_insecure_join
  zb_uint8_t aps_use_extended_pan_id
  zb_ieee_addr_t mac_extended_address
*/
  {
    zb_uint8_t addr[8]={0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x0B};
    zb_write_nvram_config(0, 1, 1, hvb7tn.tfmaddr);
  }
#endif

#ifdef ZB_USE_NVRAM
/*  zb_config_from_nvram();
  zb_read_up_counter();
  zb_rexoad_security_key();
  zb_read_formdesc_data();*/
#endif

#ifndef ZB_MACSPLIT_DEVICE
  zb_aps_init();
  zb_zdo_init();
  zdo_secur_init();
#endif

#ifdef ZB_ENABLE_ZCL
  zb_zcl_init();
#endif

#if defined ZB_ENABLE_ZLL
  zll_init();
#endif /* defined ZB_ENABLE_ZLL */

  SEC_CTX().encryption_buf = zb_buf_get_any();
  TRACE_MSG(TRACE_MAC2, "encryption_buf %hd", (FMT__H, SEC_CTX().encryption_buf));
#ifdef SNCP_MODE
  SEC_CTX().accept_partner_lk = ZB_TRUE;
#endif
#if defined ZB_ALIEN_MAC || defined ZB_ENABLE_ZGP_SECUR
  SEC_CTX().encryption_buf2 = zb_buf_get(ZB_TRUE, 0);
#endif

#ifdef ZB_ENABLE_ZGP
  zb_zgp_init();
#endif /* ZB_ENABLE_ZGP */

  trace_context_sizes();
}

static void trace_context_sizes(void)
{
#ifdef ZB_CONFIGURABLE_MEM
  extern zb_uint_t gc_use_defaults;
#endif
  /* Note: data size is not informatiove for "configurable mem" builds. */
#ifdef ZB_ED_ROLE
  TRACE_MSG(TRACE_INFO1, "ED build", (FMT__0));
#else
  TRACE_MSG(TRACE_INFO1, "FFD build", (FMT__0));
#endif

#ifdef RELEASE
  TRACE_MSG(TRACE_INFO1, "rt58x zboss library revision %hd %s & %s", 
      (FMT__D_P_P, zboss_library_revision_get(), (zboss_library_revision_modified()?"modified":"not modified"),
                (zboss_library_revision_mixed()? zboss_library_revision_range():"not mixed")));
#endif
	
#ifndef ZB_MACSPLIT_DEVICE
  TRACE_MSG(TRACE_INFO1, "sizes: g_zb %d sched %d bpool %d nwk %d aps %d addr %d zdo %d",
            (FMT__D_D_D_D_D_D_D, (zb_uint16_t)sizeof(g_zb), (zb_uint16_t)sizeof(g_zb.sched),
             (zb_uint16_t)sizeof(g_zb.bpool), (zb_uint16_t)sizeof(g_zb.nwk),
             (zb_uint16_t)sizeof(g_zb.aps), (zb_uint16_t)sizeof(g_zb.addr),
             (zb_uint16_t)sizeof(g_zb.zdo)));
#else
  TRACE_MSG(TRACE_INFO1, "sizes: g_zb %d sched %d bpool %d",
            (FMT__D_D_D, (zb_uint16_t)sizeof(g_zb), (zb_uint16_t)sizeof(g_zb.sched),
             (zb_uint16_t)sizeof(g_zb.bpool)));
#endif /* !ZB_MACSPLIT_DEVICE */
  TRACE_MSG(TRACE_INFO1, "sec %d", (FMT__D, (zb_uint16_t)sizeof(g_zb.sec)));
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

#ifndef ZB_ALIEN_MAC
  TRACE_MSG(TRACE_INFO1, "g_mac %d g_imac %d", (FMT__D_D, sizeof(g_mac), sizeof(g_imac)));
#endif

#ifdef ZB_CONFIGURABLE_MEM
  TRACE_MSG(TRACE_INFO1, "Configurable mem build, use ZBOSS lib defaults = %d", (FMT__D, gc_use_defaults));
#endif

#ifndef ZB_MACSPLIT_DEVICE
#ifndef ZB_ALIEN_MAC
  TRACE_MSG(TRACE_INFO1, "ZB_IOBUF_POOL_SIZE %d ZB_NWK_IN_Q_SIZE %d ZB_MAC_PENDING_QUEUE_SIZE %d ZB_APS_DST_BINDING_TABLE_SIZE %d ZB_APS_BIND_TRANS_TABLE_SIZE %d ZB_N_APS_RETRANS_ENTRIES %d",
            (FMT__D_D_D_D_D_D,
             ZB_IOBUF_POOL_SIZE,
             ZB_NWK_IN_Q_SIZE,
             ZB_MAC_PENDING_QUEUE_SIZE,
             ZB_APS_DST_BINDING_TABLE_SIZE,
             ZB_APS_BIND_TRANS_TABLE_SIZE,
             ZB_N_APS_RETRANS_ENTRIES));
#else
    TRACE_MSG(TRACE_INFO1, "ZB_IOBUF_POOL_SIZE %d ZB_NWK_IN_Q_SIZE %d ZB_APS_DST_BINDING_TABLE_SIZE %d ZB_APS_BIND_TRANS_TABLE_SIZE %d ZB_N_APS_RETRANS_ENTRIES %d",
            (FMT__D_D_D_D_D,
             ZB_IOBUF_POOL_SIZE,
             ZB_NWK_IN_Q_SIZE,
             ZB_APS_DST_BINDING_TABLE_SIZE,
             ZB_APS_BIND_TRANS_TABLE_SIZE,
             ZB_N_APS_RETRANS_ENTRIES));
#endif
#else
  TRACE_MSG(TRACE_INFO1, "ZB_IOBUF_POOL_SIZE %d ZB_MAC_PENDING_QUEUE_SIZE %d",
            (FMT__D_D,
             ZB_IOBUF_POOL_SIZE,
             ZB_MAC_PENDING_QUEUE_SIZE));
#endif  /* ZB_MACSPLIT_DEVICE */

#ifdef ZB_ROUTER_ROLE
#define ROUTING_TABLE_SIZE ZB_NWK_ROUTING_TABLE_SIZE
#else
#define ROUTING_TABLE_SIZE 0
#endif

  TRACE_MSG(TRACE_INFO1, "ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE %d ZB_IEEE_ADDR_TABLE_SIZE %d ZB_NEIGHBOR_TABLE_SIZE %d ZB_NWK_ROUTING_TABLE_SIZE %d ZB_APS_DUPS_TABLE_SIZE %d",
            (FMT__D_D_D_D_D,
             ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE, ZB_IEEE_ADDR_TABLE_SIZE, ZB_NEIGHBOR_TABLE_SIZE,
             ROUTING_TABLE_SIZE, ZB_APS_DUPS_TABLE_SIZE));

  /* TODO: trace sizes of all configurable components (ok, ok, we can just look into .map file) */
}

/*! @} */
