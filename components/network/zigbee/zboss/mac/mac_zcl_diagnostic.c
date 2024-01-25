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
/* PURPOSE: Roitines specific mac data transfer
*/

#define ZB_TRACE_FILE_ID 296
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_secur.h"
#ifndef ZB_MACSPLIT_DEVICE
#include "zb_zdo.h"
#endif

/*! \addtogroup ZB_MAC */
/*! @{ */

#if defined ZB_MAC_DIAGNOSTICS

void zb_mac_update_rx_zcl_diagnostic(zb_mac_mhr_t *mhr, zb_bufid_t buf)
{
  (void)mhr;
  (void)buf;

  MAC_CTX().diagnostic_ctx.last_msg_lqi = ZB_MAC_GET_LQI(buf);
  MAC_CTX().diagnostic_ctx.last_msg_rssi = ZB_MAC_GET_RSSI(buf);

  if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control)
      == ZB_ADDR_16BIT_DEV_OR_BROADCAST)
  {
    if (ZB_MAC_IS_ADDRESS_BROADCAST(mhr->dst_addr.addr_short))
    {
      MAC_CTX().diagnostic_ctx.mac_rx_bcast++;
    }
    else
    {
      MAC_CTX().diagnostic_ctx.mac_rx_ucast++;
    }
  }
  else if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_NO_ADDR)
  {
    /* Beacons has ZB_ADDR_NO_ADDR in dst addressing mode field */
    MAC_CTX().diagnostic_ctx.mac_rx_bcast++;
  }
  else if (ZB_FCF_GET_DST_ADDRESSING_MODE(mhr->frame_control) == ZB_ADDR_64BIT_DEV)
  {
    /* FIXME: Is address 0xffffffffffffffff broadcast with 64 bit addressing mode? */
    if (ZB_IS_64BIT_ADDR_UNKNOWN(mhr->dst_addr.addr_long))
    {
      MAC_CTX().diagnostic_ctx.mac_rx_bcast++;
    }
    else
    {
      MAC_CTX().diagnostic_ctx.mac_rx_ucast++;
    }
  }
  else
  {
  }
}

void zb_mac_diagnostics_init(void)
{
  ZB_SCHEDULE_ALARM_CANCEL(zb_mac_diagnostics_periodic_handler, ZB_ALARM_ANY_PARAM);
  ZB_BZERO(&MAC_CTX().diagnostic_ctx, sizeof(zb_mac_diagnostic_ctx_t));
  MAC_CTX().diagnostic_ctx.written = 1;
  ZB_SCHEDULE_ALARM(zb_mac_diagnostics_periodic_handler, 0,
                    ZB_MAC_DIAGNOSTICS_TIME_PERIOD_SEC);
}

void zb_mac_diagnostics_periodic_handler(zb_uint8_t unused)
{
  if (MAC_CTX().diagnostic_ctx.written < ZB_MAC_DIAGNOSTICS_FILTER_SIZE)
  {
    MAC_CTX().diagnostic_ctx.written++;
  }

  MAC_CTX().diagnostic_ctx.write_idx = (MAC_CTX().diagnostic_ctx.write_idx + 1U) %
    ZB_MAC_DIAGNOSTICS_FILTER_SIZE;

  ZB_BZERO(&MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx],
           sizeof(zb_mac_diagnostic_ent_t));

  ZB_SCHEDULE_ALARM(zb_mac_diagnostics_periodic_handler, unused,
                    ZB_MAC_DIAGNOSTICS_TIME_PERIOD_SEC);
}

void zb_mac_diagnostics_get_info(zb_mac_diagnostic_ex_info_t *diag_info_ex)
{
  zb_uint8_t i;

  if (diag_info_ex != NULL)
  {
    zb_mac_diagnostic_info_t *diag_info = &diag_info_ex->mac_diag_info;

    ZB_BZERO(diag_info_ex, sizeof(zb_mac_diagnostic_ex_info_t));
    diag_info->period_of_time = ZB_MAC_DIAGNOSTICS_TIME_PERIOD_MIN;

    diag_info->mac_rx_bcast = MAC_CTX().diagnostic_ctx.mac_rx_bcast;
    diag_info->mac_tx_bcast = MAC_CTX().diagnostic_ctx.mac_tx_bcast;
    diag_info->mac_rx_ucast = MAC_CTX().diagnostic_ctx.mac_rx_ucast;

    diag_info->phy_to_mac_que_lim_reached = MAC_CTX().diagnostic_ctx.phy_to_mac_que_lim_reached;
    diag_info->mac_validate_drop_cnt = MAC_CTX().diagnostic_ctx.mac_validate_drop_cnt;
    diag_info->phy_cca_fail_count = MAC_CTX().diagnostic_ctx.phy_cca_fail_count;

    /* Note that there were counters which normalized so not suitable
       for ZCL. It used in zdo channel manager now, so let's keep that as is
       but introduce a parallel set of unicast counters which can be either
       incremented or zeroed.
       Increment in zb_mac_diagnostics_inc_tx_total() and friends.
    */
    diag_info->mac_tx_ucast_total_zcl = MAC_CTX().diagnostic_ctx.mac_tx_ucast_total_zcl;
    diag_info->mac_tx_ucast_failures_zcl = MAC_CTX().diagnostic_ctx.mac_tx_ucast_failures_zcl;
    diag_info->mac_tx_ucast_retries_zcl = MAC_CTX().diagnostic_ctx.mac_tx_ucast_retries_zcl;

    diag_info->last_msg_lqi = MAC_CTX().diagnostic_ctx.last_msg_lqi;
    diag_info->last_msg_rssi = MAC_CTX().diagnostic_ctx.last_msg_rssi;

    if (MAC_CTX().diagnostic_ctx.written != 0U)
    {
      zb_uint32_t tx_total = 0;
      zb_uint32_t tx_failed = 0;
      zb_uint32_t tx_retries = 0;

      for (i = 0; i < MAC_CTX().diagnostic_ctx.written; i++)
      {
        tx_total += MAC_CTX().diagnostic_ctx.filter[i].mac_tx_ucast_total;
        tx_failed += MAC_CTX().diagnostic_ctx.filter[i].mac_tx_ucast_failures;
        tx_retries += MAC_CTX().diagnostic_ctx.filter[i].mac_tx_ucast_retries;
      }

      ZB_ASSERT(tx_total / MAC_CTX().diagnostic_ctx.written <= ZB_UINT16_MAX);
      ZB_ASSERT(tx_failed / MAC_CTX().diagnostic_ctx.written <= ZB_UINT16_MAX);
      ZB_ASSERT(tx_retries / MAC_CTX().diagnostic_ctx.written <= ZB_UINT16_MAX);
      diag_info->mac_tx_ucast_total = (zb_uint16_t)(tx_total / MAC_CTX().diagnostic_ctx.written);
      diag_info->mac_tx_ucast_failures = (zb_uint16_t)(tx_failed / MAC_CTX().diagnostic_ctx.written);
      diag_info->mac_tx_ucast_retries = (zb_uint16_t)(tx_retries / MAC_CTX().diagnostic_ctx.written);
    }

    /* Extended data */
    diag_info_ex->mac_tx_for_aps_messages = MAC_CTX().diagnostic_ctx.mac_tx_for_aps_messages;
  }
}

void zb_mac_diagnostics_cleanup_info(void)
{
  MAC_CTX().diagnostic_ctx.mac_rx_bcast = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_bcast = 0;
  MAC_CTX().diagnostic_ctx.mac_rx_ucast = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_total_zcl = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_failures_zcl = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_retries_zcl = 0;
  MAC_CTX().diagnostic_ctx.phy_to_mac_que_lim_reached = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_for_aps_messages = 0;
  MAC_CTX().diagnostic_ctx.mac_validate_drop_cnt = 0;
  MAC_CTX().diagnostic_ctx.phy_cca_fail_count = 0;
  MAC_CTX().diagnostic_ctx.mac_tx_for_aps_messages = 0;
}

void zb_mac_diagnostics_inc_tx_total(void)
{
  MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx].mac_tx_ucast_total++;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_total_zcl++;
}

void zb_mac_diagnostics_inc_tx_failed(void)
{
  MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx].mac_tx_ucast_failures++;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_failures_zcl++;
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_tx_failed filter[%hu] %hu zcl %hu",
            (FMT__H_H_H, MAC_CTX().diagnostic_ctx.write_idx,
            MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx].mac_tx_ucast_failures,
            MAC_CTX().diagnostic_ctx.mac_tx_ucast_failures_zcl));
}

void zb_mac_diagnostics_inc_tx_retry(void)
{
  MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx].mac_tx_ucast_retries++;
  MAC_CTX().diagnostic_ctx.mac_tx_ucast_retries_zcl++;
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_tx_retry filter[%hu] %hu zcl %hu",
            (FMT__H_H_H, MAC_CTX().diagnostic_ctx.write_idx,
            MAC_CTX().diagnostic_ctx.filter[MAC_CTX().diagnostic_ctx.write_idx].mac_tx_ucast_retries,
            MAC_CTX().diagnostic_ctx.mac_tx_ucast_retries_zcl));
}

void zb_mac_diagnostics_inc_tx_bcast(void)
{
  MAC_CTX().diagnostic_ctx.mac_tx_bcast++;
}

void zb_mac_diagnostics_inc_rx_que_full(zb_uint8_t counts)
{
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_rx_que_full phy_to_mac_que_lim_reached %u counts %hu", (FMT__D_H, MAC_CTX().diagnostic_ctx.phy_to_mac_que_lim_reached, counts));
  MAC_CTX().diagnostic_ctx.phy_to_mac_que_lim_reached += counts;
}

void zb_mac_diagnostics_inc_validate_drop_cnt(void)
{
  MAC_CTX().diagnostic_ctx.mac_validate_drop_cnt++;
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_validate_drop_cnt %hu", (FMT__H, MAC_CTX().diagnostic_ctx.mac_validate_drop_cnt));
}

void zb_mac_diagnostics_inc_phy_cca_fail(void)
{
  MAC_CTX().diagnostic_ctx.phy_cca_fail_count++;
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_phy_cca_fail %hu", (FMT__H, MAC_CTX().diagnostic_ctx.phy_cca_fail_count));
}

void zb_mac_diagnostics_inc_tx_for_aps_messages(void)
{
  MAC_CTX().diagnostic_ctx.mac_tx_for_aps_messages++;
  TRACE_MSG(TRACE_MAC2, "zb_mac_diagnostics_inc_tx_for_aps_messages %u", (FMT__D, MAC_CTX().diagnostic_ctx.mac_tx_for_aps_messages));
}

#endif /* ZB_MAC_DIAGNOSTICS */

/*! @} */

#endif  /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */
