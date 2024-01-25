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
/* PURPOSE: ZBOSS common init definitions
*/

/*
  Define it here, before any includes, so stuff in zb_vendor.h and
  zb_mem_config_context.h implements default memory configurations
  using weak symbols.

  If no memory configuration feature compiled it, it will be just ignored.
 */
#define ZB_CONFIG_DEFAULT_KERNEL_DEFINITION

#define ZB_TRACE_FILE_ID 7
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

#include "zb_mem_config_max.h"

#if defined ZB_CONFIGURABLE_MEM || defined DOXYGEN

#include "zb_mem_config_common.h"
#include "zb_mem_config_context.h"

/**
   @addtogroup configurable_mem_internals
   @{
*/

#ifdef KEIL
void zb_keil_safe_bzero(zb_uint8_t *a, zb_uindex_t sz)
{
  zb_uindex_t indx;
  for(indx=0; indx<sz; indx++)
  {
    a[indx]=0;
  }
}
#endif

/**
   Assignment and initialization of buffers and its sizes for configurable memory feature.
 */
void zb_init_configurable_mem(int clear)
{
  zb_uindex_t i;
  zb_uint16_t size = ZB_OFFSETOF(zb_byte_array_t, ring_buf);
  zb_uint8_t *ptr;
#ifdef KEIL
#define ZB_CM_CLEAR(a,b) if (clear) zb_keil_safe_bzero((zb_uint8_t *)(a), (zb_uindex_t)(b))
#else
#define ZB_CM_CLEAR(a,b) if (clear) ZB_BZERO((a),(b))
#endif

  ZG->bpool.pool = gc_iobuf_pool;
  ZB_CM_CLEAR(gc_iobuf_pool, sizeof(*ZG->bpool.pool) * ZB_IOBUF_POOL_SIZE);

  ZG->bpool.busy_bufs_bitmap = gc_bufs_busy_bitmap;
  ZB_CM_CLEAR(gc_bufs_busy_bitmap, (ZB_IOBUF_POOL_SIZE + 7) / 8);
#ifdef ZB_BUF_SHIELD
  ZG->bpool.buf_in_use = gc_iobuf_buf_in_use;
  ZB_CM_CLEAR(gc_iobuf_buf_in_use, (ZB_IOBUF_POOL_SIZE + 7) / 8);
#endif

  ZG->nwk.handle.input_q = (zb_byte_array_t *)&gc_nwk_in_q;
  /* input_q is a ring buffer of bytes. Declare single-byte ring buffer to
   * define its header.
   *
   * Regression tests notes:
   * Do not use sizeof to calculate zb_byte_array_t size as it is not packed.
   */
  ZB_CM_CLEAR(&gc_nwk_in_q, size + ZB_NWK_IN_Q_SIZE);
  ZG->zdo.nwk_addr_req_pending_tsns = gc_nwk_addr_req_pending_tsns;
  ZB_CM_CLEAR(gc_nwk_addr_req_pending_tsns, ZB_N_BUF_IDS);
  ZG->zdo.nwk_addr_req_pending_mask = gc_nwk_addr_req_pending_mask;
  ZB_CM_CLEAR(gc_nwk_addr_req_pending_mask, (ZB_N_BUF_IDS + 7) / 8);
#ifdef APS_FRAGMENTATION
  ZG->zdo.node_desc_req_pending_mask = gc_node_desc_req_pending_mask;
  ZB_CM_CLEAR(gc_node_desc_req_pending_mask, (ZB_N_BUF_IDS + 7) / 8);
#endif
#ifdef ZB_ROUTER_ROLE
  MAC_CTX().pending_data_queue = gc_mac_pending_data_queue;
  ZB_CM_CLEAR(gc_mac_pending_data_queue, ZB_MAC_PENDING_QUEUE_SIZE);
#endif
#ifdef ZB_MAC_SOFTWARE_PB_MATCHING
  /* ZB_MAC_SOFTWARE_PB_MATCHING implies a router role */
  MAC_CTX().child_hash_table = gc_child_hash_table;
  ZB_CM_CLEAR(gc_child_hash_table, ZB_CHILD_HASH_TABLE_SIZE * sizeof(zb_uint16_t));
  MAC_CTX().pending_bitmap = gc_pending_bitmap;
  ZB_CM_CLEAR(gc_pending_bitmap, ZB_PENDING_BITMAP_SIZE * sizeof(zb_uint32_t));
#ifdef ZB_MAC_POLL_INDICATION_CALLS_REDUCED
  MAC_CTX().poll_timestamp_table = gc_poll_timestamp_table;
  ZB_CM_CLEAR(gc_poll_timestamp_table, ZB_CHILD_HASH_TABLE_SIZE * sizeof(zb_time_t));
  MAC_CTX().poll_timeout_table = gc_poll_timeout_table;
  ZB_CM_CLEAR(gc_poll_timeout_table, ZB_CHILD_HASH_TABLE_SIZE * sizeof(zb_uint16_t));
#endif /* ZB_MAC_POLL_INDICATION_CALLS_REDUCED */
#endif  /* ZB_MAC_SOFTWARE_PB_MATCHING */
  ZG->aps.binding.src_table = gc_aps_bind_src_table;
  ZG->aps.binding.dst_table = gc_aps_bind_dst_table;
  ZB_CM_CLEAR(gc_aps_bind_dst_table, sizeof(*gc_aps_bind_dst_table) * ZB_APS_DST_BINDING_TABLE_SIZE);
  ZG->aps.binding.trans_table = gc_trans_table;
  ptr = (zb_uint8_t *)(gc_trans_index_buf);
  for (i = 0 ; i < ZB_APS_DST_BINDING_TABLE_SIZE ; ++i)
  {
    ZG->aps.binding.dst_table[i].trans_index = (zb_uint8_t *)(ptr + ZB_SINGLE_TRANS_INDEX_SIZE*i);
  }

  ZB_CM_CLEAR(gc_trans_table, ZB_APS_BIND_TRANS_TABLE_SIZE);
  ZB_CM_CLEAR(gc_trans_index_buf, ZB_APS_DST_BINDING_TABLE_SIZE * ZB_SINGLE_TRANS_INDEX_SIZE);
  ZB_CM_CLEAR(gc_aps_bind_src_table, sizeof(*gc_aps_bind_src_table) * ZB_APS_SRC_BINDING_TABLE_SIZE);

  ZG->aps.retrans.hash = gc_aps_retrans;
  ZB_CM_CLEAR(gc_aps_retrans, ZB_N_APS_RETRANS_ENTRIES * sizeof(*gc_aps_retrans));

  /* Regression tests notes:
   * Do not use sizeof to calculate zb_byte_array_t size as it is not packed.
   */
  ZG->sched.cb_q = (zb_cb_q_t *)&gc_cb_q;
  ZB_CM_CLEAR(&gc_cb_q, size + ZB_SCHEDULER_Q_SIZE * sizeof(zb_cb_q_ent_t));
  ZG->sched.tm_buffer = gc_tm_buf;
  ZB_CM_CLEAR(gc_tm_buf, ZB_SCHEDULER_Q_SIZE * sizeof(*gc_tm_buf));
  ZG->sched.cb_flag_bm = gc_cb_flag_bm;
  ZB_CM_CLEAR(gc_cb_flag_bm, (ZB_SCHEDULER_Q_SIZE + 31) / 32 * sizeof(zb_uint32_t));

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
  ZG->aps.aib.installcodes_table = gc_installcodes_table;
  ZB_CM_CLEAR(gc_installcodes_table, sizeof(*gc_installcodes_table) * ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE);
#endif
  ZG->aps.aib.aps_device_key_pair_storage.key_pair_set = gc_key_pair_set;
  ZB_CM_CLEAR(gc_key_pair_set, sizeof(*gc_key_pair_set) * ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE);

  ZG->addr.addr_map = gc_addr_map;
  ZB_CM_CLEAR(gc_addr_map, sizeof(*gc_addr_map) * ZB_IEEE_ADDR_TABLE_SIZE);
  ZG->addr.short_sorted = gc_short_sorted;
  ZB_CM_CLEAR(gc_short_sorted, ZB_IEEE_ADDR_TABLE_SIZE);
  ZG->nwk.neighbor.addr_to_neighbor = gc_addr_to_neighbor;
  ZB_CM_CLEAR(gc_addr_to_neighbor, ZB_IEEE_ADDR_TABLE_SIZE);

  ZG->nwk.neighbor.neighbor = gc_neighbor;
  ZB_CM_CLEAR(gc_neighbor, ZB_NEIGHBOR_TABLE_SIZE * sizeof(*gc_neighbor));

#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && !defined ZB_ED_ROLE
  ZG->zse.subghz.srv.dev_list = gc_subghz_dev;
  ZB_CM_CLEAR(gc_subghz_dev, ZB_NEIGHBOR_TABLE_SIZE * sizeof(*gc_subghz_dev));
#endif

#ifdef ZB_ROUTER_ROLE
  ptr = (zb_uint8_t *)(gc_passive_ack);
  for (i = 0 ; i < ZB_NWK_BRR_TABLE_SIZE ; ++i)
  {
    ZG->nwk.handle.brrt[i].passive_ack = (zb_uint8_t *)(ptr + ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE*i);
  }
  ZB_CM_CLEAR(gc_passive_ack, ZB_NWK_BRCST_PASSIVE_ACK_ARRAY_SIZE * i);
#endif

#ifdef ZB_ROUTER_ROLE
  ZB_NIB().routing_table = gc_routing_table;
  ZB_CM_CLEAR(gc_routing_table, ZB_NWK_ROUTING_TABLE_SIZE * sizeof(*gc_routing_table));
  ZB_NIB().nwk_src_route_tbl = gc_src_routing_table;
  ZB_CM_CLEAR(gc_src_routing_table, ZB_NWK_MAX_SRC_ROUTES * sizeof(*gc_src_routing_table));
#endif

  ZG->aps.dups.dups_table = gc_dups_table;
  ZB_CM_CLEAR(gc_dups_table, sizeof(*gc_dups_table) * ZB_APS_DUPS_TABLE_SIZE);

#if defined NCP_MODE && !defined NCP_MODE_HOST
  ZB_NCP_CTX_CALLS() = gc_ncp_pending_calls;
  ZB_CM_CLEAR(gc_ncp_pending_calls, sizeof(*gc_ncp_pending_calls) * ZB_N_BUF_IDS);
#endif /* NCP_MODE && !NCP_MODE_HOST */
}


/**
   @}
*/

#endif  /* ZB_CONFIGURABLE_MEM */

const zb_char_t ZB_IAR_CODE *zb_get_version(void)
{
  return ZB_VERSION;
}


/**
   Globals data structure implementation - let it be here.

   Maybe, put it into separate .c file?
 */
#ifndef ZB_CONTEXTS_IN_SEPARATE_FILE
zb_globals_t g_zb;
#endif
ZB_CODE ZB_CONST zb_64bit_addr_t g_zero_addr={0,0,0,0,0,0,0,0};
ZB_CODE ZB_CONST zb_64bit_addr_t g_unknown_ieee_addr={0xFF, 0xFF, 0xFF, 0xFF,
                                                      0xFF, 0xFF, 0xFF, 0xFF};


void zb_globals_init(void)
{
  ZB_MEMSET(&g_zb, 0, sizeof(zb_globals_t));
  ZB_MEMSET((void*)&g_izb, 0, sizeof(zb_intr_globals_t));
#if !defined ZB_ALIEN_MAC && !defined NCP_MODE_HOST
  /* Zero MAC ctx here, not in MAC: zb_init_configurable_mem uses it. */
  ZB_MEMSET(&g_mac, 0, sizeof(g_mac));
  g_mac.mac_ctx.current_tx_power = ZB_MAC_TX_POWER_INVALID_DBM;
#endif
#ifdef ZB_CONFIGURABLE_MEM
  zb_init_configurable_mem(1);
#endif
}

#ifdef ZB_ED_ROLE
void since_you_got_that_symbol_unresolved_you_probably_use_ZB_ED_ROLE_preprocessor_define_while_linking_with_zc_library(void)
{
}
#else
void since_you_got_that_symbol_unresolved_you_probably_forget_use_ZB_ED_ROLE_preprocessor_define_while_linking_with_zed_library(void)
{
}
#endif /* ZB_ED_ROLE */

/*! @} */
