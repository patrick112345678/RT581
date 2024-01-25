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
/* PURPOSE: ZCL Diagnostics cluster specific commands handling
*/

#define ZB_TRACE_FILE_ID 3

#include "zb_common.h"

/** @internal
    @{
*/

#if defined(ZDO_DIAGNOSTICS)

#include "zdo_diagnostics.h"

static void zdo_diagnostics_get_mac_stats_cb(zb_bufid_t bufid)
{
  zb_mac_diagnostic_info_t mac_stats;
  zb_mac_diagnostic_ex_info_t *mac_stats_ptr_ex;
  zdo_diagnostics_full_stats_t *full_stats;
  zb_mac_status_t status;
  zb_uint32_t mac_tx_for_aps_messages;
  zb_uint8_t pib_attr;

  TRACE_MSG(TRACE_ZDO4, ">>zdo_diagnostics_get_mac_stats_cb(), bufid %hd",
            (FMT__H, bufid));

  /* Reduce scope for conf */
  {
    zb_mlme_get_confirm_t *conf = (zb_mlme_get_confirm_t *)zb_buf_begin(bufid);
    status = conf->status;
    pib_attr = conf->pib_attr;
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,2} */
    mac_stats_ptr_ex = (zb_mac_diagnostic_ex_info_t *)(conf + 1);
  }
  ZB_MEMCPY(&mac_stats, &mac_stats_ptr_ex->mac_diag_info, sizeof(zb_mac_diagnostic_info_t));
  mac_tx_for_aps_messages = mac_stats_ptr_ex->mac_tx_for_aps_messages;

  full_stats = zb_buf_initial_alloc(bufid, sizeof(zdo_diagnostics_full_stats_t));
  full_stats->status = (zb_uint8_t)status;

  if (status == MAC_SUCCESS)
  {
    zb_uint16_t aps_counters_sum =
      /* DL: MAC does ZB_NWK_MAX_BROADCAST_RETRIES retries for one APS broadcast
         and increments the internal counter 'mac_tx_for_aps_messages' each time */
      ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_bcast * ZB_NWK_MAX_BROADCAST_RETRIES
      + ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_ucast_success
      + ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_ucast_fail;

#if 0
    zb_uint32_t mac_counters_sum =
      mac_stats.mac_tx_bcast
      + mac_stats.mac_tx_ucast_total;

    /* TODO: add a cooment from the spec about calcuation logic */
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.average_mac_retry_per_aps_message_sent =
      (aps_counters_sum) ? (mac_counters_sum / aps_counters_sum) : 1;
#endif

    /* HA 9.2.2.2.2.27 "A counter that is equal to the average number
       of MAC retries needed to send an APS message."
       There is no more detailed explanation in the spec. */
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.average_mac_retry_per_aps_message_sent =
      ZB_U2B(aps_counters_sum) ? ((zb_uint16_t)mac_tx_for_aps_messages / aps_counters_sum) : 1U;

    TRACE_MSG(TRACE_ZDO4, "average_mac_retry_per_aps_message_sent = %hu (%u / %u)",
              (FMT__H_D_D,
              ZDO_DIAGNOSTICS_CTX().diagnostics_info.average_mac_retry_per_aps_message_sent,
              mac_tx_for_aps_messages,
              aps_counters_sum));

    ZB_MEMCPY(&(full_stats->zdo_stats),
              &ZDO_DIAGNOSTICS_CTX().diagnostics_info,
              sizeof(zdo_diagnostics_info_t));

    ZB_MEMCPY(&(full_stats->mac_stats),
              &mac_stats,
              sizeof(zb_mac_diagnostic_info_t));

#ifndef ZB_ENABLE_NWK_RETRANSMIT
    full_stats->zdo_stats.nwk_retry_overflow = 0;
#endif /* !ZB_ENABLE_NWK_RETRANSMIT */

    if (pib_attr == ZB_PIB_ATTRIBUTE_GET_AND_CLEANUP_DIAG_INFO)
    {
      ZB_BZERO(&(ZDO_DIAGNOSTICS_CTX().diagnostics_info), sizeof(zdo_diagnostics_info_t));
    }
  }
  else
  {
    ZB_BZERO(&(full_stats->zdo_stats), sizeof(zdo_diagnostics_info_t));
    ZB_BZERO(&(full_stats->mac_stats), sizeof(zb_mac_diagnostic_info_t));

    TRACE_MSG(TRACE_ZDO4, "could not get mac statistics!, zb_mlme_get_confirm->status 0x%hx",
              (FMT__H, full_stats->status));
  }

  if (ZDO_DIAGNOSTICS_CTX().get_stats_cb != NULL)
  {
    ZB_SCHEDULE_CALLBACK(ZDO_DIAGNOSTICS_CTX().get_stats_cb, bufid);
  }
  else
  {
    zb_buf_free(bufid);
  }

  ZDO_DIAGNOSTICS_CTX().get_stats_cb = NULL;

  TRACE_MSG(TRACE_ZDO4, "<<zdo_diagnostics_get_mac_stats_cb()", (FMT__0));
}

static void zdo_diagnostics_get_mac_stats(zb_uint8_t param, zb_uint16_t pib_attr)
{
  TRACE_MSG(TRACE_ZDO4, ">>zdo_diagnostics_get_mac_stats(), param %hd, pib_attr 0x%x",
            (FMT__H_D, param, pib_attr));
  zb_nwk_pib_get(param, (zb_uint8_t)pib_attr, zdo_diagnostics_get_mac_stats_cb);
  TRACE_MSG(TRACE_ZDO4, "<<zdo_diagnostics_get_mac_stats()", (FMT__0));
}

zb_ret_t zdo_diagnostics_get_stats(zb_callback_t cb, zb_uint8_t pib_attr)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZDO4, ">>zdo_diagnostics_get_stats(), cb 0x%p, pib_attr 0x%hx",
            (FMT__P_H_H, cb, pib_attr));

  if (ZDO_DIAGNOSTICS_CTX().get_stats_cb == NULL)
  {
    ZDO_DIAGNOSTICS_CTX().get_stats_cb = cb;
    ret = zb_buf_get_out_delayed_ext(zdo_diagnostics_get_mac_stats, (zb_uint16_t)pib_attr, 0);
  }
  else
  {
    ret = RET_BUSY;
    TRACE_MSG(TRACE_ZDO4, "another request is already scheduled! cb 0x%p",
              (FMT__P_H, ZDO_DIAGNOSTICS_CTX().get_stats_cb));
  }

  TRACE_MSG(TRACE_ZDO4, "<<zdo_diagnostics_get_stats()", (FMT__0));

  return ret;
}

/**
 *  @} internal
 */

#ifdef ZDO_DIAGNOSTICS_DEBUG_TRACE
void zdo_diagnostics_inc(zdo_diagnostics_counter_id_t counter_id, zb_uint_t trace_file_id, zb_uint_t line)
{
  zb_bool_t write_nvram = ZB_FALSE;
  TRACE_MSG(TRACE_ERROR, ">>zdo_diagnostics_inc(), counter_id %hd file %u line %u", (FMT__H_D_D, counter_id, trace_file_id, line));
#else
void zdo_diagnostics_inc(zdo_diagnostics_counter_id_t counter_id)
{
  zb_bool_t write_nvram = ZB_FALSE;
  TRACE_MSG(TRACE_ZDO4, ">>zdo_diagnostics_inc(), counter_id %hd", (FMT__H, counter_id));
#endif

  switch (counter_id)
  {
    case ZDO_DIAGNOSTICS_NUMBER_OF_RESETS_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.number_of_resets);
    break;

    case ZDO_DIAGNOSTICS_PACKET_BUFFER_ALLOCATE_FAILURES_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.packet_buffer_allocate_failures);
    break;

    case ZDO_DIAGNOSTICS_JOIN_INDICATION_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.join_indication);
    break;

    case ZDO_DIAGNOSTICS_APS_TX_BCAST_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_bcast);
    break;

    case ZDO_DIAGNOSTICS_APS_TX_UCAST_SUCCESS_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_ucast_success);
    break;

    case ZDO_DIAGNOSTICS_APS_TX_UCAST_RETRY_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_ucast_retry);
    break;

    case ZDO_DIAGNOSTICS_ROUTE_DISC_INITIATED_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.route_disc_initiated);
    break;

    case ZDO_DIAGNOSTICS_APS_TX_UCAST_FAIL_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_tx_ucast_fail);
    break;

    case ZDO_DIAGNOSTICS_CHILD_MOVED_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.childs_removed);
    break;

    case ZDO_DIAGNOSTICS_NWKFC_FAILURE_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_fc_failure);
    break;

    case ZDO_DIAGNOSTICS_APSFC_FAILURE_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_fc_failure);
    break;

    case ZDO_DIAGNOSTICS_APS_UNAUTHORIZED_KEY_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_unauthorized_key);
    break;

    case ZDO_DIAGNOSTICS_NWK_DECRYPT_FAILURES_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_decrypt_failure);
    break;

    case ZDO_DIAGNOSTICS_APS_DECRYPT_FAILURES_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.aps_decrypt_failure);
    break;

#ifdef ZB_ENABLE_NWK_RETRANSMIT
    case ZDO_DIAGNOSTICS_NWK_RETRY_OVERFLOW_ID:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_retry_overflow);
    break;
#endif /* ZB_ENABLE_NWK_RETRANSMIT */

    case ZDO_DIAGNOSTICS_NEIGHBOR_ADDED_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_added);
    break;

    case ZDO_DIAGNOSTICS_NEIGHBOR_REMOVED_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_removed);
    break;

    case ZDO_DIAGNOSTICS_NEIGHBOR_STALE_ID:
      write_nvram = ZB_TRUE;
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_stale);
    break;

    case ZDO_DIAGNOSTICS_NWK_BCAST_TABLE_FULL:
      ++(ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_bcast_table_full);
    break;

    default:
      TRACE_MSG(TRACE_ZDO4, "counter_id %hd is not implemented!", (FMT__H, counter_id));
    break;
  }

  if (write_nvram)
#if defined ZB_USE_NVRAM
  {
    TRACE_MSG(TRACE_ZDO4, "Write ZDO Diagnostics dataset", (FMT__0));
    (void)zb_nvram_write_dataset(ZB_NVRAM_ZDO_DIAGNOSTICS_DATA);
  }
#else
  {
    TRACE_MSG(TRACE_ZDO4, "ZB_USE_NVRAM isn't set! Skip diagnostics dataset writing", (FMT__0));
  }
#endif /* ZB_USE_NVRAM */

  TRACE_MSG(TRACE_ZDO4, "<<zdo_diagnostics_inc()", (FMT__0));
}

void zdo_diagnostics_init(void)
{
  ZB_BZERO(&(ZDO_DIAGNOSTICS_CTX().diagnostics_info), sizeof(zdo_diagnostics_info_t));
  ZDO_DIAGNOSTICS_CTX().get_stats_cb = NULL;
}

void zdo_diagnostics_route_req_inc(zb_uint8_t param)
{
#ifdef ZB_PRO_STACK
  zb_uint8_t cmd_id;
  zb_nwk_hdr_t *nwhdr = zb_buf_begin(param);

  if (ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control) == ZB_NWK_FRAME_TYPE_COMMAND)
  {
    cmd_id = ZB_NWK_CMD_FRAME_GET_CMD_ID(param, ZB_NWK_HDR_SIZE(nwhdr));
    if (cmd_id == ZB_NWK_CMD_ROUTE_REQUEST)
    {
      ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_ROUTE_DISC_INITIATED_ID);
    }
    TRACE_MSG(TRACE_NWK4, "zdo_diagnostics_route_req_inc param %hu fc 0x%hx cmd_id %hu",
              (FMT__H_H_H, param, ZB_NWK_FRAMECTL_GET_FRAME_TYPE(nwhdr->frame_control), cmd_id));
  }
#endif
  ZVUNUSED(param);
}

#ifdef ZB_USE_NVRAM

zb_uint16_t zb_nvram_diagnostics_dataset_length(void)
{
  zb_uint16_t len;

#if (ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER == ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER_1)
    len = (zb_uint16_t)sizeof(zb_nvram_dataset_diagnostics_v1_t);
#else
    #error zb_nvram_diagnostics_dataset_length() handler for given dataset version is not implemented (see ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER)
#endif

  return len;
}

static void zb_nvram_read_diagnostics_dataset_v1(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_dataset_diagnostics_v1_t ds;
  zb_ret_t ret;

  ZB_BZERO(&ds, sizeof(ds));
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds,  (zb_uint16_t)sizeof(ds));

  if (ret == RET_OK)
  {
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.number_of_resets = ds.number_of_resets;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_added = ds.nwk_neighbor_added;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_removed = ds.nwk_neighbor_removed;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_stale = ds.nwk_neighbor_stale;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.join_indication = ds.join_indication;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.childs_removed = ds.childs_removed;
    ZDO_DIAGNOSTICS_CTX().diagnostics_info.packet_buffer_allocate_failures = ds.packet_buffer_allocate_failures;

#ifdef ZB_ZCL_SUPPORT_CLUSTER_DIAGNOSTICS
    TRACE_MSG(TRACE_ZDO4, "call zb_zcl_diagnostics_sync_counters()", (FMT__0));
    zb_zcl_diagnostics_sync_counters(0, NULL);
#endif /* ZB_ZCL_SUPPORT_CLUSTER_DIAGNOSTICS */
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Could not read the Diagnostics dataset v1! ret 0x%lx", (FMT__L, ret));
  }
}

void zb_nvram_read_diagnostics_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  TRACE_MSG(TRACE_ZDO4, ">>zb_nvram_read_diagnostics_dataset() page %hd, pos 0x%lx, len %d, nvram_ver %d, ds_ver %d",
            (FMT__H_L_D_D_D, page, pos, len, nvram_ver, ds_ver));

  ZVUNUSED(len);
  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  if (ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER_1 == ds_ver)
  {
    TRACE_MSG(TRACE_ZDO4, "Read the Diagnostics dataset v1", (FMT__0));
    zb_nvram_read_diagnostics_dataset_v1(page, pos);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Reading the Diagnostics dataset v%d is not implemented!", (FMT__D, ds_ver));
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_ZDO4, "<<zb_nvram_read_diagnostics_dataset()", (FMT__0));
}

static zb_ret_t zb_nvram_write_diagnostics_dataset_v1(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_dataset_diagnostics_v1_t ds;

  ZB_BZERO(&ds, sizeof(ds));

  ds.number_of_resets = ZDO_DIAGNOSTICS_CTX().diagnostics_info.number_of_resets;
  ds.nwk_neighbor_added = ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_added;
  ds.nwk_neighbor_removed = ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_removed;
  ds.nwk_neighbor_stale = ZDO_DIAGNOSTICS_CTX().diagnostics_info.nwk_neighbor_stale;
  ds.join_indication = ZDO_DIAGNOSTICS_CTX().diagnostics_info.join_indication;
  ds.childs_removed = ZDO_DIAGNOSTICS_CTX().diagnostics_info.childs_removed;
  ds.packet_buffer_allocate_failures = ZDO_DIAGNOSTICS_CTX().diagnostics_info.packet_buffer_allocate_failures;

  return zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));
}

zb_ret_t zb_nvram_write_diagnostics_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZDO4, ">>zb_nvram_write_diagnostics_dataset(), page %hd, pos 0x%lx",
            (FMT__H_L, page, pos));

#if (ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER == ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER_1)
    TRACE_MSG(TRACE_ZDO4, "Write Diagnostics dataset v1", (FMT__0));
    ret = zb_nvram_write_diagnostics_dataset_v1(page, pos);
#else
    #error zb_nvram_write_diagnostics_dataset() handler for given dataset version is not implemented (see ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER)
#endif

  TRACE_MSG(TRACE_ZDO4, "<<zb_nvram_write_diagnostics_dataset(), ret 0x%lx", (FMT__L, ret));

  return ret;
}

#endif /* defined(ZB_USE_NVRAM) */

#endif /* defined(ZDO_DIAGNOSTICS) */
