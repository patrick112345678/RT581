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
/* PURPOSE: Network address assign
*/
#define ZB_TRACE_FILE_ID 2230
#include <stdlib.h>

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_mac.h"
#include "zb_nwk.h"
#include "zb_secur.h"
#include "zb_aps.h"
#include "nwk_internal.h"
#include "zb_zdo.h"
#include "zb_ncp.h"

/*! \addtogroup ZB_NWK */
/*! @{ */


/* See 3.6.1.9 Address Conflict */



#if defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE     /* Zigbee pro */

/* Start processing of received address conflict - schedule Network Status sending
 * Check on neighborship for device with conflicting address
 * Change own address if it is in conflict with other devices
 */
void zb_nwk_address_conflict_start_process(zb_uint8_t addr_ref_param)
{
  zb_ret_t ret;
  zb_address_ieee_ref_t addr_ref = (zb_address_ieee_ref_t) addr_ref_param;
  zb_uint16_t addr;

  zb_address_short_by_ref(&addr, addr_ref);

  if (addr != ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    zb_neighbor_tbl_ent_t *nbt;

    ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbt);

    if (ret == RET_OK)
    {
      /* If device with conflicting address is our neighbor - check that it is our ED children.
        * If it is so - change it's address.
        */
      if (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD
          && nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)
      {
        TRACE_MSG(TRACE_NWK1, "child ED conflict", (FMT__0));
        nbt->need_rejoin = 1;
        ret = zb_buf_get_out_delayed(zb_nwk_change_my_child_addr_process);

        if (ret != RET_OK)
        {
          TRACE_MSG(TRACE_ERROR, "zb_nwk_change_my_child_addr_process is not scheduled, ret [%d]", (FMT__D, ret));
        }
      }
    }
    else
    {
      TRACE_MSG(TRACE_NWK1, "Device is not our neighbor, addr_ref %hd", (FMT__H, addr_ref_param));
    }

    ret = zb_buf_get_out_delayed_ext(zb_nwk_address_conflict_send_report,
                                      (zb_uint16_t)addr_ref_param, 0);

    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "zb_nwk_address_conflict_send_report is not scheduled, ret [%d]", (FMT__D, ret));
    }
  }
#ifdef ZB_JOIN_CLIENT
  else
  {
    TRACE_MSG(TRACE_NWK1, "Address conflict for me", (FMT__0));
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00012,0} */
    if (!ZB_IS_DEVICE_ZC())
    {
      ret = zb_buf_get_out_delayed(zb_nwk_change_me_addr);
      if (ret == RET_OK)
      {
        ZG->nwk.handle.addr_conflict_ctx_send_nwk_status = ZB_TRUE;
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
      }
    }
  }
#endif  /* ZB_JOIN_CLIENT */
}

/*
 * Send request Address Conflict and start schedule alarm
 */
static void zb_nwk_address_conflict_schedule_report(zb_uint8_t param, zb_uint16_t addr)
{
  zb_nlme_send_status_t *request = ZB_BUF_GET_PARAM(param, zb_nlme_send_status_t);

  TRACE_MSG(TRACE_NWK1, "zb_nwk_store_address_conflict_report 0x%x", (FMT__D, addr));

  request->dest_addr = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
  request->status.status = ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT;
  request->status.network_addr = addr;
  request->ndsu_handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
  ZB_SCHEDULE_CALLBACK(zb_nlme_send_status, param);
}

/*
 * Send request Address Conflict
 */
void zb_nwk_address_conflict_send_report(zb_uint8_t param, zb_uint16_t addr_ref_param)
{
  zb_address_ieee_ref_t addr_ref = (zb_address_ieee_ref_t) addr_ref_param;
  zb_uint16_t addr;

  TRACE_MSG(TRACE_NWK1, "zb_nwk_address_conflict_send_report", (FMT__0));

  zb_address_short_by_ref(&addr, addr_ref);
  ZG->addr.addr_map[addr_ref].has_address_conflict = ZB_FALSE_U;

  TRACE_MSG(TRACE_NWK1, "address_conflict 0x%x", (FMT__D, addr));

  zb_nwk_address_conflict_schedule_report(param, addr);
}

#ifdef ZB_JOIN_CLIENT
void zb_nwk_change_my_addr_conf(zb_uint8_t param);

/*
 * Change my network address
 */
void zb_nwk_change_me_addr(zb_uint8_t param)
{
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00012,11} */
  if (!ZB_IS_DEVICE_ZC())
  {
    // change addr
    ZB_PIBCACHE_NETWORK_ADDRESS() = zb_nwk_get_stoch_addr();

    /* clear broadcast retransmission table:
     * if we retransmit broadcast with our old address
     * and it is rebroadcasted by our neighbour
     * and we receive that rebroadcasted packet
     * there will be another address conflict */
#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
    nwk_internals_clear();
#else
    zb_nwk_broadcasting_clear();
#endif

    zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_SHORT_ADDRESS, &ZB_PIBCACHE_NETWORK_ADDRESS(), 2, zb_nwk_change_my_addr_conf);
  }
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00012,12} */
  else
  {
    zb_buf_free(param);
  }
}

void zb_nwk_change_my_addr_conf(zb_uint8_t param)
{
  zb_ret_t ret;
  zb_address_ieee_ref_t addr_ref;
  zb_uint16_t old_addr = 0;

  /* Upon receipt packet with address conflicting with own NWK creates new neighbor
   * using addre_ref with conflicting short_addr and device own long address.
   * To avoid presence this neighbor in Link Status messages remove neigbor
   * refernced by addr_ref (do not include itself into neighbors).
   * Also update addr_ref - it must contain new resolved short address.
   */

  ret = zb_address_by_ieee(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
  ZB_ASSERT(ret == RET_OK);

	/* We shouldn't set has_address_conflict to 0 now, because address_update & device_annce are not done yet.
	 * This may trigger meaningless conflict, so move it down below.
	 */
  //ZG->addr.addr_map[addr_ref].has_address_conflict = ZB_FALSE_U;

  if (ZG->nwk.handle.addr_conflict_ctx_send_nwk_status)
  {
    /* We can get our previous short address from the address table - at this point
     * ieee address points to the conflicting address */

    zb_address_short_by_ref(&old_addr, addr_ref);
  }

  ret = zb_address_update(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_PIBCACHE_NETWORK_ADDRESS(),
                          ZB_FALSE, &addr_ref);
  ZB_ASSERT(ret == RET_OK);
  (void)zb_nwk_neighbor_delete(addr_ref);

  if (ZG->nwk.handle.addr_conflict_ctx_send_nwk_status)
  {
    /* Broadcast address conflict and clear the flag */
    zb_ret_t ret_conflict_sch_rep = zb_buf_get_out_delayed_ext(zb_nwk_address_conflict_schedule_report, old_addr, 0);
    if (ret_conflict_sch_rep == RET_OK)
    {
      ZG->nwk.handle.addr_conflict_ctx_send_nwk_status = ZB_FALSE;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed_ext [%d]", (FMT__D, ret));
    }
  }

  /* send dev_annc() */
  TRACE_MSG(TRACE_ZDO3, "scheduling device_annce %hd", (FMT__H, param));
  ZB_SCHEDULE_CALLBACK(zdo_send_device_annce, param);

	/* Move has_address_conflict setting here. */
  ZG->addr.addr_map[addr_ref].has_address_conflict = ZB_FALSE_U;

#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
  ZG->nwk.handle.stop_detect_confilct = ZB_FALSE_U;
#endif

#ifdef NCP_MODE
  ncp_address_update_ind(ZB_PIBCACHE_NETWORK_ADDRESS());
#endif
}
#endif  /* ZB_JOIN_CLIENT */

/*
 * Change my child network address
 * spec 3.6.1.9.3
 */

void zb_nwk_change_my_child_addr(zb_uint8_t param, zb_neighbor_tbl_ent_t *nbt)
{
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t short_addr;
  zb_uint16_t short_addr_new;
//  zb_address_ieee_ref_t ieee_ref;
  zb_nwk_rejoin_response_t *rejoin_response;
  zb_ushort_t i;

  zb_bool_t secure = (zb_bool_t)(ZB_NIB_SECURITY_LEVEL());

  /* First check: maybe, we just sent rejoin to that device. Need not repeat */
  for (i = 0 ; i < ZG->nwk.handle.rejoin_req_table_cnt ; ++i)
  {
    if (ZG->nwk.handle.rejoin_req_table[i].addr_ref == nbt->u.base.addr_ref)
    {
      zb_buf_free(param);
      TRACE_MSG(TRACE_NWK1, "<< zb_nwk_change_my_child_addr: early ret: rejoin just sent to ref %d in tbl %d ",
                (FMT__H_H, nbt->u.base.addr_ref, i));
      return;
    }
  }
  short_addr_new = ZB_NWK_ED_ADDRESS_ASSIGN();

  zb_address_ieee_by_ref(ieee_addr, nbt->u.base.addr_ref);
  zb_address_short_by_ref(&short_addr, nbt->u.base.addr_ref);

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_change_my_child_addr  0x%x = " TRACE_FORMAT_64,
    (FMT__D_A, short_addr, TRACE_ARG_64(ieee_addr)));

  TRACE_MSG(TRACE_NWK1, "new short addr  0x%x", (FMT__D, short_addr_new));

  // change address
  /* allocate address translation entry */

  // send rejoin
  (void)nwk_alloc_and_fill_hdr(param,
                               ZB_PIBCACHE_NETWORK_ADDRESS(), short_addr,
                               ZB_FALSE, secure, ZB_TRUE, ZB_TRUE);
  if ( secure )
  {
    nwk_mark_nwk_encr(param);
    TRACE_MSG(TRACE_NWK1, "rejoin req was secured, reply secured also", (FMT__0));
  }
  rejoin_response = (zb_nwk_rejoin_response_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_REJOIN_RESPONSE, (zb_uint8_t)sizeof(zb_nwk_rejoin_response_t));
  TRACE_MSG(TRACE_ATM1, "Z< send rejoin response to 0x%04x, new address = 0x%04x", (FMT__D_D, short_addr, short_addr_new));

  rejoin_response->network_addr = short_addr_new;

  ZB_NWK_ADDR_TO_LE16(rejoin_response->network_addr);
  rejoin_response->rejoin_status = MAC_SUCCESS;

  /* transmit rejoin packet */
  (void)zb_nwk_init_apsde_data_ind_params(param, ZB_NWK_INTERNAL_REJOIN_CMD_RESPONSE);
  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);


  /* For remeber short address for rejoin used rejoin_req_table.
   * Add record to rejoin_req_table with old short address.
   */
  {
    /* Save network address */
    ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].addr = short_addr_new; //short_addr;
    ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].addr_ref = nbt->u.base.addr_ref;
    (void)zb_address_lock(nbt->u.base.addr_ref);
    TRACE_MSG(TRACE_NWK1, "rejoin_req_table[%hd] = 0x%x", (FMT__H_D, ZG->nwk.handle.rejoin_req_table_cnt, short_addr_new));
    ZG->nwk.handle.rejoin_req_table[ZG->nwk.handle.rejoin_req_table_cnt].secure_rejoin = ZB_B2U(secure);
    ZG->nwk.handle.rejoin_req_table_cnt++;
  }

  TRACE_MSG(TRACE_NWK1, "<< zb_nwk_change_my_child_addr", (FMT__0));
}

/*
 * Send request Address Conflict and start schedule alarm
 */
void zb_nwk_change_my_child_addr_process(zb_uint8_t param)
{
  zb_neighbor_tbl_ent_t *nbt;

  if (zb_nwk_neighbor_with_need_rejoin(&nbt)==RET_OK)
  {
    nbt->need_rejoin = 0;
    zb_nwk_change_my_child_addr(param, nbt);
  }
  else
  {
    zb_buf_free(param);
  }
}

/*
 * Address Conflict broadcast handle
 */
zb_ret_t zb_nwk_address_conflict_resolve(zb_uint8_t param, zb_uint16_t addr)
{
  zb_neighbor_tbl_ent_t *nbt;
  zb_bool_t forward_pkt = ZB_TRUE;
  zb_ret_t ret = RET_NOT_FOUND;
  zb_address_ieee_ref_t addr_ref;

  TRACE_MSG(TRACE_NWK1, "zb_nwk_address_conflict_resolve %hd", (FMT__H, param));

  /* R21, 3.6.1.9 Address Conflicts: */
  /* If a device receives a broadcast data frame and discovers an address conflict as a result of the receipt, as */
  /* discussed below in section 3.6.1.9.2, it should not retransmit the frame as usual but shall discard it before */
  /* taking the resolution actions described below in section 3.6.1.9.3. */

  /* TP_PRO_BV-14: ZC should ignore address conflicts for its short address and should not
   * retransmit it over the air. */

  if (zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK
      && ZB_U2B(ZG->addr.addr_map[addr_ref].has_address_conflict))
  {
    ZB_SCHEDULE_ALARM_CANCEL(zb_nwk_address_conflict_start_process, addr_ref);
    ZG->addr.addr_map[addr_ref].has_address_conflict = ZB_FALSE_U;
  }

  if (addr == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    if (ZB_IS_DEVICE_ZC())
    /*cstat !MISRAC2012-Rule-2.1_b */
    /** @mdr{00012,13} */
    {
      /* Trust me, I'm a coordinator! */
      zb_buf_free(param);
      return RET_OK;
    }

#ifdef ZB_JOIN_CLIENT
    forward_pkt = ZB_FALSE;
    /* Do not attempt to silently change the addr! According to 3.6.1.9.3:
     * - if conflict is DETECTED by ED or nwkAddrAlloc is not stochastic, device shall perform a
     * rejoin
     * - if PARENT DETECTS of IS INFORMED of a conflict for its child, it shall pick new addr and
     * send rejoin response
     * So if it is nwk status with conflict for us, just do not retransmit it (according to the
     * 3.6.1.9).
     */

    if (ZB_IS_DEVICE_ZR())
    {
      ZB_SCHEDULE_CALLBACK(zb_nwk_change_me_addr, param);
    }
    else
    {
      zb_buf_free(param);
    }

    ret = RET_OK;
#endif  /* ZB_JOIN_CLIENT */
  }
  else
  {
    if (zb_nwk_neighbor_get_by_short(addr, &nbt) == RET_OK)
    {
      /* In case of device is our ED-child - it's address_conflict_table already clear;
       * just send rejoin response */
      nbt->need_rejoin = 0;
      if (nbt->relationship == ZB_NWK_RELATIONSHIP_CHILD
          && nbt->device_type == ZB_NWK_DEVICE_TYPE_ED)
      {
        zb_nwk_change_my_child_addr(param, nbt);
        ret = RET_OK;
      }
    }
  }

  if (ret != RET_OK)
  {
    if (forward_pkt)
    {
      /* this packet is not for us and not for our child, forward */
      zb_nwk_hdr_t *nwk_hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);

      if (ZB_NWK_IS_ADDRESS_BROADCAST(nwk_hdr->dst_addr))
      {
        zb_apsde_data_ind_params_t *pkt_params = ZB_BUF_GET_PARAM(param, zb_apsde_data_ind_params_t);
        pkt_params->handle = ZB_NWK_INTERNAL_NSDU_HANDLE;
        ZB_ASSERT(param);
        ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
      }
      else
      {
        zb_buf_free(param);
      }
    }
    else
    {
      zb_buf_free(param);
    }
  }
  return ret;
}

#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
static void zb_nwk_address_conflict_detect_on(zb_uint8_t param)
{

  ZVUNUSED(param);
  if(ZG->nwk.handle.stop_detect_confilct == ZB_TRUE_U)
  {
    ZG->nwk.handle.stop_detect_confilct = ZB_FALSE_U;
    TRACE_MSG(TRACE_NWK1, "<< turn on detect conflict func", (FMT__0));
  }

}
#endif

static void schedule_address_conflict(zb_address_ieee_ref_t addr_ref)
{
#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
  if (!ZB_U2B(ZG->addr.addr_map[addr_ref].has_address_conflict)
    && (ZG->nwk.handle.stop_detect_confilct == ZB_FALSE_U))
#else
  if (!ZB_U2B(ZG->addr.addr_map[addr_ref].has_address_conflict))
#endif
  {
    ZG->addr.addr_map[addr_ref].has_address_conflict = ZB_TRUE_U;

#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
    ZG->nwk.handle.stop_detect_confilct = ZB_TRUE_U;
    ZB_SCHEDULE_ALARM(zb_nwk_address_conflict_detect_on, 0, (ZB_BTR_AGING_CYCLE * ZB_TIME_ONE_SECOND));
#endif

    /* use delay for sending network status */
    ZB_SCHEDULE_ALARM(zb_nwk_address_conflict_start_process,
                      (zb_uint8_t) addr_ref,
                      ZB_NWK_OCTETS_TO_BI(ZB_RANDOM_JTR(ZB_NWKC_MAX_BROADCAST_JITTER_OCTETS)));
  }
}

/*
 * Test pair (short addr, ieee addr) on Address Conflict and
 * resolve if need
 * See 3.6.1.9.2, 3.6.1.9.3
 */
zb_ret_t zb_nwk_is_conflict_addr(zb_uint16_t addr, zb_ieee_addr_t ieee_addr)
{
  zb_ret_t ret = RET_OK;
  zb_address_ieee_ref_t addr_ref = {0};
  zb_bool_t schedule_addr_conflict = ZB_FALSE;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_is_conflict_addr 0x%x = " TRACE_FORMAT_64,
            (FMT__D_A, addr, TRACE_ARG_64(ieee_addr)));


#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
  if (ZG->nwk.handle.stop_detect_confilct == ZB_TRUE_U)
  {
    TRACE_MSG(TRACE_NWK1, ">>STOP detect conflict!", (FMT__0));
    return ret;
  }
#endif
  /*
   * Test own address
   */
  if (addr!=0x0000U)
  {
    if( (ZB_64BIT_ADDR_CMP(ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS()) && addr != ZB_PIBCACHE_NETWORK_ADDRESS())
         || (!ZB_64BIT_ADDR_CMP(ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS()) && addr == ZB_PIBCACHE_NETWORK_ADDRESS()) )
    {
      TRACE_MSG(TRACE_NWK1, "zb_nwk_is_conflict_addr with me: incoming 0x%x ieee " TRACE_FORMAT_64 " my 0x%x ieee " TRACE_FORMAT_64,
                (FMT__D_A_D_A, addr, TRACE_ARG_64(ieee_addr),
                 ZB_PIBCACHE_NETWORK_ADDRESS(),
                 TRACE_ARG_64(ZB_PIBCACHE_EXTENDED_ADDRESS())));

      /* To prevent missing target entry, search with extended address. */
      //ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
      ret = zb_address_by_ieee(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);

      ZB_ASSERT(ret == RET_OK);
      /* Schedule an address conflict only if the address was actually found */
      schedule_addr_conflict = (ret == RET_OK) ? ZB_TRUE : ZB_FALSE;
    }
  }
  if (ret == RET_OK
      && addr != ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    if (zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
    {
      zb_uint16_t addr_by_long_ref;
      zb_ieee_addr_t ieee_addr_by_short_ref;
      zb_address_ieee_ref_t addr_ref_long;

      zb_address_ieee_by_ref(ieee_addr_by_short_ref, addr_ref);

      TRACE_MSG(TRACE_NWK1, "found addr_ref 0x%x " TRACE_FORMAT_64,
                (FMT__D_A, addr_ref, TRACE_ARG_64(ieee_addr_by_short_ref)));

      if (!ZB_64BIT_ADDR_CMP(ieee_addr, ieee_addr_by_short_ref))
      {
        TRACE_MSG(TRACE_NWK1, "zb_nwk_is_conflict_addr ieee = " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(ieee_addr)));

        /* If ieee address is unknown (filled by 0xff) */
        if (ZB_IS_64BIT_ADDR_UNKNOWN(ieee_addr_by_short_ref))
        {
            TRACE_MSG(TRACE_NWK3, "wrong direction", (FMT__0));


          /* Before updating ieee address check if the same ieee address is already
           * stored in addr map with another short address */
          ret = zb_address_by_ieee(ieee_addr, ZB_FALSE, ZB_FALSE, &addr_ref_long);

          if (ret == RET_OK)
          {
            /* Found ieee address stored in addr map. Searching for its short address */
            zb_address_short_by_ref(&addr_by_long_ref, addr_ref_long);

            if (addr_by_long_ref != ZB_UNKNOWN_SHORT_ADDR)
            {
              /* Found the same ieee address stored in addr map with another short addr */
              TRACE_MSG(TRACE_NWK1, "Detected address conflict in addr map!", (FMT__0));
              schedule_addr_conflict = ZB_TRUE;
            }
          }
          else
          {
            /* Not found another addr map entry with the same ieee addr  */
            ret = RET_OK;
          }

          if (!schedule_addr_conflict)
          {
            /* Update ieee address. This call also merges 2 address entries: "short only" and "long
             * only". */
            TRACE_MSG(TRACE_NWK3, "update ieee", (FMT__0));
            ret = zb_address_update(ieee_addr, addr, ZB_FALSE, &addr_ref);
          }
        }
        else
        {
            TRACE_MSG(TRACE_NWK3, "schedule_addr_conflict", (FMT__0));
          schedule_addr_conflict = ZB_TRUE;
        }
      }
    }
  }

  if (schedule_addr_conflict == ZB_TRUE)
  {
    /* use delay for sending network status */
    ret = RET_CONFLICT;
    /* Check that addr_ref entry has_address_conflict bit set to 0.
     * If this bit has non-zero value - it means that we are already scehedule this alarm
     * Just drop frame
     */
    schedule_address_conflict(addr_ref);
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_is_conflict_addr %hd", (FMT__D, ret));
  return ret;
}

/*
 * Test message on Address Conflict and
 * resolve if need
 */
zb_ret_t zb_nwk_test_addresses(zb_uint8_t param)
{
  zb_nwk_hdr_t *hdr = (zb_nwk_hdr_t *)zb_buf_begin(param);
  zb_nwk_btr_t *ent;
  zb_ret_t ret = RET_OK;
  zb_ieee_addr_t src_ieee;
  zb_bool_t has_src_ieee = ZB_FALSE;

  if (ZB_U2B(ZB_NIB().uniq_addr))
  {
    return ret;
  }

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_test_addresses %hd nwk_hdr %p src_addr 0x%x",
            (FMT__H_P_D, param, hdr, hdr->src_addr));

  if (ZB_U2B(ZB_NWK_FRAMECTL_GET_SOURCE_IEEE(hdr->frame_control)))
  {
    zb_ieee_addr_t *ieee_addr;

    has_src_ieee = ZB_TRUE;
    ieee_addr = zb_nwk_get_src_long_from_hdr(hdr);

    ZB_IEEE_ADDR_COPY(src_ieee, *ieee_addr);
  }

  if (ZB_NWK_IS_ADDRESS_BROADCAST(hdr->dst_addr)
      && hdr->src_addr == ZB_PIBCACHE_NETWORK_ADDRESS()
      && ZB_U2B(hdr->src_addr)
      && has_src_ieee
      && ZB_IEEE_ADDR_CMP(src_ieee, ZB_PIBCACHE_EXTENDED_ADDRESS()))
  {
    zb_address_ieee_ref_t addr_ref;

#ifdef ZB_RAF_NWK_ADDRESS_CONFLICT_PREVENT
    ret = zb_address_by_ieee(ZB_PIBCACHE_EXTENDED_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
#else
    ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_FALSE, ZB_FALSE, &addr_ref);
#endif

    if (ret != RET_OK)
    {
      ZB_ASSERT(0);
      return ret;
    }

    /**
     * 3.6.1.9.2
     * <... >
     *
     * When a broadcast frame is received that creates a new BTR, if the Source Address
     * field in the NWK Header is equal to the nwkNetworkAddress attribute of the NIB then
     * a local address conflict has been detected on nwkNetworkAddress.
     */

/* 01/15/2019 EE CR:MAJOR Add a comment why there must be address, not a ref, according to spec (or TC?) */
    NWK_ARRAY_FIND_ENT(ZG->nwk.handle.btt, ZB_NWK_BTR_TABLE_SIZE,
                        ent,
                        ((ent->source_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
                          && (ent->sequence_number == hdr->seq_num))
       );

     if (ent == NULL)
     {

       TRACE_MSG(TRACE_NWK1, "addr conflict by btt: %d %d",
                 (FMT__D_D, addr_ref, ZB_PIBCACHE_NETWORK_ADDRESS()));
       schedule_address_conflict(addr_ref);

       ret = RET_CONFLICT;
     }
  }

  if (ZB_U2B(hdr->src_addr) && has_src_ieee)
  {
    ret = zb_nwk_is_conflict_addr(hdr->src_addr, src_ieee) == RET_OK ? ret : RET_CONFLICT;
  }

  if (ZB_U2B(hdr->dst_addr) && ZB_U2B(ZB_NWK_FRAMECTL_GET_DESTINATION_IEEE(hdr->frame_control)) && !ZB_NWK_IS_ADDRESS_BROADCAST(hdr->dst_addr))
  {
    ret = zb_nwk_is_conflict_addr(hdr->dst_addr, hdr->dst_ieee_addr) == RET_OK ? ret : RET_CONFLICT;
  }
  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_test_addresses", (FMT__0));
  return ret;
}

/*
 * Test dev_annce on Address Conflict and
 * resolve if need
 */
zb_ret_t zb_nwk_test_dev_annce(zb_uint16_t addr, zb_ieee_addr_t ieee_addr)
{
  zb_ieee_addr_t long_addr;

#if !defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN
  if (ZB_NIB().uniq_addr == ZB_TRUE)
  {
    TRACE_MSG(TRACE_NWK1, "unique addr, no test", (FMT__0));
    return RET_OK;
  }
#endif

#ifndef ZB_COORDINATOR_ONLY
  if (!ZB_IS_DEVICE_ZC() && addr == ZB_PIBCACHE_NETWORK_ADDRESS())
  {
    TRACE_MSG(TRACE_NWK1, "addr == our", (FMT__0));
    if (zb_nwk_is_conflict_addr(addr, ieee_addr) == RET_CONFLICT)
    {
      return RET_ERROR;
    }
  }
#endif

  /* The Remote Device shall also use the NWKAddr in the message to
   * find a match with any other 16-bit NWK address held in the Remote
   * Device, even if the IEEEAddr field in the message carries the
   * value of 0xffffffffffffffff. If a match is detected for a device
   * with an IEEE address other than that indicated in the IEEEAddr
   * field received, then this entry shall be marked as not having a
   * known valid 16-bit NWK address */

  if (zb_address_ieee_by_short(addr, long_addr) == RET_OK)
  {
    if (!ZB_64BIT_ADDR_CMP(long_addr, ieee_addr))
    {
      TRACE_MSG(TRACE_NWK1, "addr64 != from our table", (FMT__0));
      if (zb_nwk_is_conflict_addr(addr, ieee_addr) == RET_CONFLICT)
      {
        return RET_ERROR;
      }
    }
  }
  return RET_OK;
}

#endif

/*! @} */
