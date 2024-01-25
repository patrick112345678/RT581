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
/* PURPOSE: Neighbor table
*/

#define ZB_TRACE_FILE_ID 328
#include "zb_common.h"
#include "zb_nwk.h"
#include "zb_magic_macros.h"

/*! \addtogroup ZB_NWK */
/*! @{ */

static zb_ret_t alloc_new_extneiboard(zb_address_pan_id_ref_t panid_ref, zb_uint16_t short_addr, zb_ext_neighbor_tbl_ent_t **enbt);

void zb_nwk_neighbor_init(void)
{
  TRACE_MSG(TRACE_NWK1, "nb_init", (FMT__0));
  ZB_MEMSET(&ZG->nwk.neighbor.addr_to_neighbor[0], -1, ZB_IEEE_ADDR_TABLE_SIZE);

  ZG->nwk.neighbor.transmit_failure_threshold = ZB_ZDO_NEIGHBOR_FAILURE_PKT_THRESHOLD;
  ZG->nwk.neighbor.transmit_failure_timeout = ZB_ZDO_NEIGHBOR_UNREACHABLE_TIMEOUT;
}


void zb_nwk_exneighbor_stop(zb_uint16_t parent_short_addr)
{
  zb_uint_t i;
  zb_bool_t parent_found = ZB_FALSE;
  zb_address_ieee_ref_t ref;
  zb_bool_t need_save = ZB_FALSE;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_exneighbor_stop parent_short_addr 0x%x", (FMT__D, parent_short_addr));

  ZVUNUSED(parent_short_addr);

  if (ZB_JOINED())
  {
    /* If joined, move all records from my PAN to the base neighbor table */
    zb_address_pan_id_ref_t my_panid_ref;

    if (zb_address_get_pan_id_ref(ZB_NIB_EXT_PAN_ID(), &my_panid_ref) != RET_OK)
    {
      /* Must extsts already!! */
      TRACE_MSG(TRACE_NWK1, "PAN id " TRACE_FORMAT_64 " not in panids arr - ?", (FMT__A,
                TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));
    }

    /* Check that our parent is already in the neighbor table. If so,
     * do not create a dup converting it from ext neighbor. */
    if (parent_short_addr != (zb_uint16_t)-1
        /*cstat !MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violation of the Rule 13.5 seems to be a false
         * positive. The only way the function 'zb_address_by_short()' could have side effects is if
         * it is called with the second and third parameters ('create' and 'lock', respectively)
         * equal to ZB_TRUE, which is not the case. */
        && zb_address_by_short(parent_short_addr, ZB_FALSE, ZB_FALSE, &ref) == RET_OK
        && ZG->nwk.neighbor.addr_to_neighbor[ref] != 0xffU)
    {
      parent_found = ZB_TRUE;
    }

    /* Conver ext neighbor to base neighbor on place */
    for (i = EXN_START_I ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
    {
      zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[i];

      if (ZB_U2B(ent->used) && ZB_U2B(ent->ext_neighbor))
      {
        if (ent->u.ext.panid_ref == my_panid_ref
            /* ZED keeps only parent in its neighbor */
            /*cstat !MISRAC2012-Rule-13.5 */
            /* After some investigation, the following violation of Rule 13.5 seems to be
             * a false positive. There are no side effect to 'ZB_IS_DEVICE_ZC_OR_ZR()'. This
             * violation seems to be caused by the fact that 'ZB_IS_DEVICE_ZC_OR_ZR()' is an
             * external macro, which cannot be analyzed by C-STAT. */
            && (ZB_IS_DEVICE_ZC_OR_ZR()
                || (ent->u.ext.short_addr == parent_short_addr && parent_found == ZB_FALSE)))
        {
          zb_ret_t ret;

          TRACE_MSG(TRACE_NWK3, "i %d - my pan", (FMT__D, (int)i));
          ret = zb_nwk_neighbor_ext_to_base(ent, ZB_FALSE);
          if (ret == RET_OK)
          {
            need_save = ZB_TRUE;
          }
          else
          {
            TRACE_MSG(TRACE_ERROR, "zb_nwk_neighbor_ext_to_base failed [%d]", (FMT__D, ret));
            ZB_ASSERT(0);
          }
        }
        else
        {
          TRACE_MSG(TRACE_NWK3, "i %d - not my panid - drop", (FMT__D, i));
          ent->used = ent->ext_neighbor = ZB_FALSE_U;
        }
      } /* if used & ext */
    }   /* for */
#ifdef ZB_USE_NVRAM
    if (need_save)
    {
      zb_nvram_store_addr_n_nbt();
    }
#endif
  }     /* if joined */
  else
  {
    /* If not joined, just clear ext neighbor */
    for (i = EXN_START_I ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
    {
      zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[i];

      if (ZB_U2B(ent->used) && ZB_U2B(ent->ext_neighbor))
      {
        ent->used = ent->ext_neighbor = ZB_FALSE_U;
      }
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_exneighbor_stop", (FMT__0));
}


zb_ret_t zb_nwk_exneighbor_by_short(zb_address_pan_id_ref_t panid_ref, zb_uint16_t short_addr,
                                    zb_ext_neighbor_tbl_ent_t **enbt)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_ushort_t i;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_exneighbor_by_short pan %d, addr 0x%x, enbt %p", (FMT__D_D_P,
            (int)panid_ref, short_addr, enbt));

  *enbt = NULL;
  for (i = EXN_START_I ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
  {
    zb_neighbor_tbl_ent_t *en = &ZG->nwk.neighbor.neighbor[i];
    if (ZB_U2B(en->used) && ZB_U2B(en->ext_neighbor)
        && en->u.ext.short_addr == short_addr
        && en->u.ext.panid_ref == panid_ref)
    {
      *enbt = en;
      TRACE_MSG(TRACE_NWK3, "found ent # %d", (FMT__D, i));
      ret = RET_OK;
      break;
    }
  }
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_NWK3, "try alloc", (FMT__0));
    ret = alloc_new_extneiboard(panid_ref, short_addr, enbt);
  }
  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_exneighbor_by_short %d", (FMT__D, ret));

  return ret;
}


zb_ret_t zb_nwk_exneighbor_by_ieee(zb_address_pan_id_ref_t panid_ref, zb_ieee_addr_t long_addr, zb_ext_neighbor_tbl_ent_t **enbt)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_ushort_t i;
  zb_ieee_addr_compressed_t compressed_addr;

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_exneighbor_by_ieee pan %d addr %p enbt %p", (FMT__D_P_P,
            panid_ref, long_addr, enbt));
  zb_ieee_addr_compress(long_addr, &compressed_addr);

  for (i = EXN_START_I ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
  {
    zb_neighbor_tbl_ent_t *en = &ZG->nwk.neighbor.neighbor[i];
    if (ZB_U2B(en->used)
        && ZB_U2B(en->ext_neighbor)
        && en->u.ext.panid_ref == panid_ref
        /*cstat !MISRAC2012-Rule-13.5 */
           /* After some investigation, the following violation of Rule 13.5 seems to be
            * a false positive. There are no side effect to 'ZB_ADDRESS_COMPRESSED_CMP()'. This
            * violation seems to be caused by the fact that 'ZB_ADDRESS_COMPRESSED_CMP()' is an
            * external macro, which cannot be analyzed by C-STAT. */
        && ZB_ADDRESS_COMPRESSED_CMP(en->u.ext.long_addr, compressed_addr))
    {
      TRACE_MSG(TRACE_NWK3, "found ent # %d", (FMT__D, i));
      *enbt = &ZG->nwk.neighbor.neighbor[i];
      ret = RET_OK;
    }
  }
  if (ret == RET_NOT_FOUND)
  {
    TRACE_MSG(TRACE_NWK3, "try alloc new ent", (FMT__0));
    ret = alloc_new_extneiboard(panid_ref, (zb_uint16_t)~0U, enbt);
    if (ret == RET_OK)
    {
      zb_ieee_addr_compress(long_addr, &(*enbt)->u.ext.long_addr);
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_exneighbor_by_ieee %d", (FMT__D, ret));

  return ret;
}

/**
   Allocate new extended neighbor table entry

   @return RET_OK if ok, RET_NO_MEMORY if no more free entries.
 */
static zb_ret_t alloc_new_extneiboard(zb_address_pan_id_ref_t panid_ref, zb_uint16_t short_addr, zb_ext_neighbor_tbl_ent_t **enbt)
{
  zb_ret_t ret = RET_NO_MEMORY;
  zb_uint_t i;

  TRACE_MSG(TRACE_NWK1, ">>alloc_new_extnb pan %d addr %d enbt %p", (FMT__D_D_P,
            panid_ref, short_addr, enbt));

  for (i = EXN_START_I ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
  {
    zb_neighbor_tbl_ent_t *en = &ZG->nwk.neighbor.neighbor[i];
    if (!ZB_U2B(en->used))
    {
      TRACE_MSG(TRACE_NWK3, "allocated ext ent %p #%d", (FMT__P_D, en, i));
      ZB_BZERO(en, sizeof(zb_ext_neighbor_tbl_ent_t));
      en->used = en->ext_neighbor = ZB_TRUE_U;
      en->u.ext.short_addr = short_addr;
      ZB_ADDRESS_COMPRESS_UNKNOWN(en->u.ext.long_addr);
      en->u.ext.panid_ref = panid_ref;
      *enbt = en;
      ret = RET_OK;
      ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NEIGHBOR_ADDED_ID);
      break;
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<alloc_new_extnb %d", (FMT__D, ret));
  return ret;
}

#if defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
static void nwk_neighbor_delete_ed(zb_uint8_t param, zb_uint16_t ieee_ref)
{
  zb_address_ieee_ref_t ref;
  zb_uint8_t nbt_idx;
  ZB_ASSERT(ieee_ref <= ZB_UINT8_MAX);
  ref = (zb_address_ieee_ref_t)ieee_ref;
  nbt_idx = ZG->nwk.neighbor.addr_to_neighbor[ref];
  TRACE_MSG(TRACE_NWK2, ">>nwk_neighbor_delete_ed param %d, ieee_ref %d", (FMT__D_D, param, ref));
  if ( nbt_idx != 0xFFU )
  {
    zb_nwk_src_match_delete(param, ref);
    zb_nwk_neighbor_complete_deletion(ref, nbt_idx);
    ZVUNUSED(param);
  }
  else
  {
    TRACE_MSG(TRACE_NWK2, "nbt already deleted, do nothing", (FMT__0));
    zb_buf_free(param);
  }
}
#endif

void zb_nwk_neighbor_complete_deletion(zb_address_ieee_ref_t ieee_ref, zb_uint8_t neighbor_index)
{
  TRACE_MSG(TRACE_NWK2, ">>zb_nwk_neighbor_complete_deletion ieee_ref %hd neighbor_index %hd",
            (FMT__H_H, ieee_ref, neighbor_index));
  ZG->nwk.neighbor.addr_to_neighbor[ieee_ref] = (zb_uint8_t)-1;
  ZG->nwk.neighbor.neighbor[neighbor_index].used = ZB_FALSE_U;
  TRACE_MSG(TRACE_NWK1, "nbt = %p deleted", (FMT__P, &ZG->nwk.neighbor.neighbor[neighbor_index]));

  zb_address_unlock(ieee_ref);
  /* WTH!!111 Commented the line above
   *  Please do not delete address here because entity of address and
   *  entity of neighbor haven't a direct relationship to each other
   */

  //zb_address_delete(ieee_ref);

  ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NEIGHBOR_REMOVED_ID);

#ifdef ZB_USE_NVRAM
  zb_nvram_transaction_start();
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
  (void)zb_nvram_write_dataset(ZB_NVRAM_NEIGHBOUR_TBL);
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
  zb_nvram_transaction_commit();
#endif

#ifdef ZB_ROUTER_ROLE
  {
    zb_nwk_restart_aging();
  }
#endif
  TRACE_MSG(TRACE_NWK2, "<<zb_nwk_neighbor_complete_deletion", (FMT__0));
}


zb_ret_t zb_nwk_neighbor_delete(zb_address_ieee_ref_t ieee_ref)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uint8_t n = ZG->nwk.neighbor.addr_to_neighbor[ieee_ref];

  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_neighbor_delete ref %hd n %hd", (FMT__H_H, ieee_ref, n));

  if ( n != 0xFFU )
  {
#if defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
    if (ZG->nwk.neighbor.neighbor[n].device_type == ZB_NWK_DEVICE_TYPE_ED)
    {
      ret = zb_buf_get_out_delayed_ext(nwk_neighbor_delete_ed, (zb_uint16_t)ieee_ref, 0);
    }
  /* Deletion process compliting will be executed:
   * a) in scheduled zb_nwk_src_match_delete() for ZED devices.
   * b) immediately for other devices */
    else
#endif  /* defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING */
    {
      zb_nwk_neighbor_complete_deletion(ieee_ref, n);
      ret = RET_OK;
    }
  }

  TRACE_MSG(TRACE_NWK1, "<<nb_del %d", (FMT__D, ret));
  return ret;
}

zb_bool_t zb_nwk_neighbor_exists(zb_address_ieee_ref_t addr_ref)
{
  zb_bool_t ret;

  TRACE_MSG(TRACE_NWK2, ">>zb_nwk_neighor_exists addr_ref %hd", (FMT__H, addr_ref));

  if (addr_ref >= ZB_IEEE_ADDR_TABLE_SIZE)
  {
    ZB_ASSERT(0);
    ret = ZB_FALSE;
  }
  else if (ZG->nwk.neighbor.addr_to_neighbor[addr_ref] == (zb_uint8_t)-1)
  {
    ret = ZB_FALSE;
  }
  else
  {
    ret = ZB_TRUE;
  }

  TRACE_MSG(TRACE_NWK2, "<<zb_nwk_neighor_exists %hd", (FMT__H, ret));

  return ret;
}

zb_ret_t zb_nwk_neighbor_get(zb_address_ieee_ref_t addr_ref, zb_bool_t create_if_absent, zb_neighbor_tbl_ent_t **nbt)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t n = ZG->nwk.neighbor.addr_to_neighbor[addr_ref];
/*
#ifdef ZB_USE_NVRAM
  zb_uint8_t create_new_record = ZB_FALSE;
#endif
*/
  TRACE_MSG(TRACE_NWK1, ">>zb_nwk_neighbor_get addr_ref %hd cr %hd nbt %p", (FMT__H_H_P,
                                                                             addr_ref, create_if_absent, nbt));

  if (n == (zb_uint8_t)-1)
  {
    if (create_if_absent)
    {
/*
#ifdef ZB_USE_NVRAM
      create_new_record = ZB_TRUE;
#endif
*/
      ret = RET_NO_MEMORY;
      for(n = 0; n < ZB_NEIGHBOR_TABLE_SIZE ; n++)
      {
        if (!ZB_U2B(ZG->nwk.neighbor.neighbor[n].used))
        {
          ret = RET_OK;
          break;
        }
      }
      if (ret == RET_OK)
      {
        /* zb_address_lock won't fail, as ZG->nwk.neighbor.neighbor[n].used was already checked before */
        (void)zb_address_lock(addr_ref);
        ZG->nwk.neighbor.addr_to_neighbor[addr_ref] = n;

        {
          zb_neighbor_tbl_ent_t *bn = &ZG->nwk.neighbor.neighbor[n];
          ZB_BZERO(bn, sizeof(zb_neighbor_tbl_ent_t));
          bn->used = ZB_TRUE_U;
          /* Set rx_on to 1 seems better because rx_on device (smart plug) send packets more often
             then ed.
          */
          bn->rx_on_when_idle = 1;
          /* Update device_type on the next dev_annce */
          bn->device_type = ZB_NWK_DEVICE_TYPE_NONE;
          bn->u.base.addr_ref = addr_ref;
          bn->relationship = ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE;
          /* was unknown (2) - see table 2.126. Seems we do not correctly use
           * it anyway */
          bn->permit_joining = 0;
          bn->send_via_routing = 0;

          /* set our depth by default (not sure it is always right) */
#if defined ZB_NWK_DISTRIBUTED_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE
          bn->depth = ZB_NIB().depth;
#endif
#if defined ZB_PRO_STACK && defined ZB_ROUTER_ROLE
          /* initialize cost, so this entry will not be expired before it send
           * first link status */
          bn->u.base.outgoing_cost = ZB_NWK_STATIC_PATH_COST;
#endif
          bn->rssi = (zb_int8_t)ZB_MAC_RSSI_UNDEFINED;
#if !defined ZB_ED_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING
          bn->subghz_ep = ZB_ZCL_BROADCAST_ENDPOINT;
          bn->is_subghz = ZB_PIBCACHE_CURRENT_PAGE() != ZB_CHANNEL_PAGE0_2_4_GHZ;
          TRACE_MSG(TRACE_NWK1, "is_subghz %hd", (FMT__H, bn->is_subghz));
#endif
          *nbt = bn;
          TRACE_MSG(TRACE_NWK1, "nbt = %p rel %hd", (FMT__P_H, *nbt, bn->relationship));
        }

        ZDO_DIAGNOSTICS_INC(ZDO_DIAGNOSTICS_NEIGHBOR_ADDED_ID);

        /* It does not seem reasonable to store new record here - either it will be updated (and
         * stored after update) or it is temporary record (for example incoming pkt from nbt). Let's
         * assume user will store this record manually - if it is really needed.
#ifdef ZB_USE_NVRAM
         if (create_new_record)
         {
           zb_nvram_store_addr_n_nbt();
         }
 #endif */
      }
    }
    else
    {
      ret = RET_NOT_FOUND;
    }
  }
  else
  {
    *nbt = &ZG->nwk.neighbor.neighbor[n];

    TRACE_MSG(TRACE_NWK1, "nbt = %p rel %hd", (FMT__P_H, *nbt, (*nbt)->relationship));
  }

  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_neighbor_get %d", (FMT__D, ret));
  return ret;
}


zb_uint8_t zb_nwk_neighbor_get_ed_cnt()
{
  zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[0];
  zb_uindex_t i;
  zb_uint8_t cnt = 0;

  for ( i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
  {
    if ( ZB_U2B(ent->used) && (ent->device_type == ZB_NWK_DEVICE_TYPE_ED))
    {
      ++cnt;
    }
    ent++;
  }
  TRACE_MSG(TRACE_NWK1, "zb_nwk_neighbor_get_ed_cnt %hd", (FMT__P, cnt));
  return cnt;
}


zb_uint8_t zb_nwk_neighbor_get_zc_zr_cnt(void)
{
  zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[0];
  zb_uindex_t i;
  zb_uint8_t cnt = 0;

  for ( i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++ )
  {
    if ( ZB_U2B(ent->used) &&
        (ent->device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR ||
         ent->device_type == ZB_NWK_DEVICE_TYPE_ROUTER))
    {
      ++cnt;
    }
    ent++;
  }
  TRACE_MSG(TRACE_NWK1, "zb_nwk_neighbor_get_zc_zr_cnt %hd", (FMT__P, cnt));
  return cnt;
}


zb_uint8_t zb_nwk_neighbor_get_ed_short_list(zb_uint8_t start_index, zb_uint8_t max_records, zb_uint8_t** ed_list)
{
  zb_uint16_t addr = 0;
  zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[0];
  zb_uindex_t i;
  zb_uint8_t cnt = 0;
  zb_uint8_t found = 0;

  for ( i = 0; (i < ZB_NEIGHBOR_TABLE_SIZE) && (found < max_records); i++ )
  {
    if ( ZB_U2B(ent->used) && (ent->device_type == ZB_NWK_DEVICE_TYPE_ED))
    {
      /* Skip childs before start_index */
      ++cnt;
      if (cnt >= start_index)
      {
        zb_address_short_by_ref(&addr, ent->u.base.addr_ref);
        ZB_PUT_NEXT_HTOLE16(*ed_list, addr);
        TRACE_MSG(TRACE_NWK3, "associated device addr: %d", (FMT__D, addr));
        ++found;
      }
    }
    ent++;
  }

  return found;
}


/*
 * Get neighbor, sorted by short address
 * Neighbors for Link Status request must be sorted - see 3.6.3.4.2
 */
zb_ret_t zb_nwk_get_sorted_neighbor(zb_uint8_t *index, zb_neighbor_tbl_ent_t **p_nbt)
{
  zb_address_ieee_ref_t ref_p;

  while(zb_address_by_sorted_table_index(*index, &ref_p)==RET_OK)
  {
    if(zb_nwk_neighbor_get(ref_p, ZB_FALSE, p_nbt)==RET_OK)
    {
      return RET_OK;
    }
    (*index)++;
  }
  return RET_NOT_FOUND;
}


zb_ret_t zb_nwk_neighbor_ext_to_base(zb_ext_neighbor_tbl_ent_t *ent, zb_bool_t do_cp)
{
  zb_ret_t ret;
  zb_ieee_addr_t long_address;
  zb_address_ieee_ref_t addr_ref;

  ZB_ASSERT(ZB_U2B(ent->used) && ZB_U2B(ent->ext_neighbor));

  TRACE_MSG(TRACE_NWK1, ">> zb_nwk_neighbor_ext_to_base do_cp %hd", (FMT__H, do_cp));

  if (do_cp)
  {
    zb_uint_t n;
    for(n = 0; n < ZB_NEIGHBOR_TABLE_SIZE ; n++)
    {
      if (!ZB_U2B(ZG->nwk.neighbor.neighbor[n].used))
      {
        ZB_MEMCPY(&ZG->nwk.neighbor.neighbor[n], ent, sizeof(*ent));
        /* Release old entry */
        /* FIXME: do we ever need that logic? It moves ext neighbor entry instead of convert it on-place. For what reason?
         * Try to comment it out. Dup will be fixed when converting exteighbor to neighbor.
         */
        ent->used = ZB_FALSE_U;
        ent = &ZG->nwk.neighbor.neighbor[n];
        break;
      }
    }
  }
  if (ent->u.ext.short_addr != (zb_uint16_t)~0U)
  {
    if (!ZB_ADDRESS_COMPRESSED_IS_UNKNOWN(ent->u.ext.long_addr))
    {
      /* both addresses are known */
      zb_ieee_addr_decompress(long_address, &ent->u.ext.long_addr);
      ret = zb_address_update(long_address, ent->u.ext.short_addr,
                              ZB_FALSE, &addr_ref); /* don't lock address: keep locked */
      TRACE_MSG(TRACE_NWK3, "both addr, ret %d, ref %d", (FMT__D_D, ret, addr_ref));
    }
    else
    {
      /* have only short */
      ret = zb_address_by_short(ent->u.ext.short_addr,
                                ZB_TRUE, ZB_FALSE, &addr_ref);
      TRACE_MSG(TRACE_NWK3, "short addr, ret %d, ref %d", (FMT__D_D, ret, addr_ref));
    }
  }
  else
  {
    /* have only long */
    zb_ieee_addr_decompress(long_address, &ent->u.ext.long_addr);
    ret = zb_address_by_ieee(long_address, ZB_TRUE, ZB_FALSE, &addr_ref);
    TRACE_MSG(TRACE_NWK3, "long addr, ret %d, ref %d", (FMT__D_D, ret, addr_ref));
  }
  /* Now have addr_ref. Can get base neighbor by addr */

  if (ret == RET_OK && ZG->nwk.neighbor.addr_to_neighbor[addr_ref] == 0xffU)
  {
#if defined ZB_COORDINATOR_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING
    zb_uint8_t is_subghz = (ent->u.ext.channel_page != ZB_CHANNEL_PAGE0_2_4_GHZ);
#endif

    ret = zb_address_lock(addr_ref);
    if (ret == RET_OK)
    {
#if (defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
      /* For ZER let's always keep base entry at index 0 */
      if (ent != &ZG->nwk.neighbor.neighbor[0])
      {
        ZB_MEMCPY(&ZG->nwk.neighbor.neighbor[0], ent, sizeof(*ent));
        ent->used = ent->ext_neighbor = ZB_FALSE_U;
        ent = &ZG->nwk.neighbor.neighbor[0];
      }
#endif

      ZB_BZERO(&ent->u.base, sizeof(ent->u.base));
      /* No base neighbor for that address */
      ZG->nwk.neighbor.addr_to_neighbor[addr_ref] = (zb_uint8_t)(ent - &ZG->nwk.neighbor.neighbor[0]);
      /* TODO:MM It seems it is not correct to set rx_on_when_idle based on device_type (example - not sleepy ED smart plug). Check it */
      ent->rx_on_when_idle = ZB_B2U(ent->device_type != ZB_NWK_DEVICE_TYPE_ED);
      /* relationship is unknown. */
      ent->relationship = ZB_NWK_RELATIONSHIP_NONE_OF_THE_ABOVE;
#if defined ZB_PRO_STACK && defined ZB_ROUTER_ROLE
      /* initialize cost, so this entry will not be expired before it send
      * first link status */
      ent->u.base.outgoing_cost = ZB_NWK_STATIC_PATH_COST;
#endif
      ent->u.base.addr_ref = addr_ref;
#if defined ZB_COORDINATOR_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING
      ent->subghz_ep = ZB_ZCL_BROADCAST_ENDPOINT;
      ent->is_subghz = is_subghz;
      TRACE_MSG(TRACE_NWK3, "is_subghz %hd", (FMT__H, ent->is_subghz));
#endif
      ent->ext_neighbor = 0;
    }
  }
  else
  {
    /* Oops, we actually had neighbor table entry for that address! Do not lock it. Drop just created nbt entry. */
    zb_neighbor_tbl_ent_t *e = &ZG->nwk.neighbor.neighbor[ZG->nwk.neighbor.addr_to_neighbor[addr_ref]];

    if (e != ent)
    {
      if (e->device_type == ZB_NWK_DEVICE_TYPE_NONE)
      {
        e->device_type = ent->device_type;
      }
      ent->used = ent->ext_neighbor = ZB_FALSE_U;
      TRACE_MSG(TRACE_NWK1, "Already had nbt entry %p for addr ref %d", (FMT__P_D, e, addr_ref));
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "Some mess in nbt for addr %d", (FMT__D, addr_ref));
    }
  }
  TRACE_MSG(TRACE_NWK1, "<<zb_nwk_neighbor_ext_to_base %d", (FMT__D, ret));
  return ret;
}


void zb_nwk_neighbor_clear()
{
  zb_uint8_t i;

  TRACE_MSG(TRACE_NWK1, "zb_nwk_neighbor_clear", (FMT__0));
  /* Clear "neighbor" locks */
  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; ++i)
  {
    zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[i];

    if (ZB_U2B(ent->used) && !ZB_U2B(ent->ext_neighbor))
    {
      zb_address_unlock(ent->u.base.addr_ref);
    }
  }
  /* ZB_MEMSET(&ZG->nwk.neighbor, 0, sizeof(ZG->nwk.neighbor)); */
  ZB_BZERO(&ZG->nwk.neighbor.neighbor[0], ZB_NEIGHBOR_TABLE_SIZE * sizeof(zb_neighbor_tbl_ent_t));
  ZB_BZERO(&ZG->nwk.neighbor.addr_to_neighbor[0], ZB_IEEE_ADDR_TABLE_SIZE * sizeof(zb_uint8_t));
  ZG->nwk.neighbor.incoming_frame_counter_clock = 0;
#ifdef ZB_ROUTER_ROLE
  ZG->nwk.neighbor.parent_annce_position = 0;
  zb_stop_ed_aging();
#endif
#if defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING
  {
    zb_ret_t ret = zb_buf_get_out_delayed(zb_nwk_src_match_drop);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
    }
  }
#endif  /* defined ZB_ROUTER_ROLE && defined ZB_MAC_PENDING_BIT_SOURCE_MATCHING */
  zb_nwk_neighbor_init();
}

zb_ret_t zb_nwk_neighbor_get_by_idx(zb_uint8_t idx, zb_neighbor_tbl_ent_t **nbt)
{
  if (idx < ZB_NEIGHBOR_TABLE_SIZE &&
      ZB_U2B(ZG->nwk.neighbor.neighbor[idx].used))
  {
    *nbt = &ZG->nwk.neighbor.neighbor[idx];
    return RET_OK;
  }

  return RET_NOT_FOUND;
}

zb_ret_t zb_nwk_neighbor_get_by_short(zb_uint16_t short_addr, zb_neighbor_tbl_ent_t **nbt)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_address_ieee_ref_t addr_ref;

  if (zb_address_by_short(short_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
    if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, nbt) == RET_OK)
#else
    if (ZG->nwk.neighbor.neighbor[0].u.base.addr_ref == addr_ref)
#endif
    {
#if (defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
      *nbt = &ZG->nwk.neighbor.neighbor[0];
#endif
      ret = RET_OK;
      ZB_ASSERT(*nbt);
    }
  }
  /* If successfully found the pointer nbt must be valid */
  //ZB_ASSERT_IF(ret == RET_OK, *nbt != NULL);
  return ret;
}

zb_ret_t zb_nwk_neighbor_get_by_ieee(const zb_ieee_addr_t long_addr, zb_neighbor_tbl_ent_t **nbt)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_address_ieee_ref_t addr_ref;

  if (zb_address_by_ieee(long_addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
    if (zb_nwk_neighbor_get(addr_ref, ZB_FALSE, nbt) == RET_OK)
#else
    if (ZG->nwk.neighbor.neighbor[0].u.base.addr_ref == addr_ref)
#endif
    {
#if (defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
      *nbt = &ZG->nwk.neighbor.neighbor[0];
#endif
      ret = RET_OK;
      ZB_ASSERT(*nbt);
    }
  }
  /* If successfully found the pointer nbt must be valid */
  //ZB_ASSERT_IF(ret == RET_OK, *nbt != NULL);
  return ret;
}


void zb_nwk_neighbor_incoming_frame_counter_clock(zb_uint8_t key_seq_number)
{
#if !(defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)
  zb_ushort_t i = ZG->nwk.neighbor.incoming_frame_counter_clock =
    (ZG->nwk.neighbor.incoming_frame_counter_clock + 1U) % ZB_NEIGHBOR_TABLE_SIZE;
#else
  zb_ushort_t i = 0;
#endif
  zb_int16_t diff =(zb_int16_t)key_seq_number - (zb_int16_t)ZG->nwk.neighbor.neighbor[i].u.base.key_seq_number;

  if (diff < 0)
  {
    diff = 256 + diff;
  }
  if (diff > 3)
  {
    ZG->nwk.neighbor.neighbor[i].u.base.incoming_frame_counter = 0;
    ZG->nwk.neighbor.neighbor[i].u.base.key_seq_number = key_seq_number;
  }
}

#ifndef ZB_ED_ROLE
zb_ushort_t zb_nwk_neighbor_next_rx_on_i(zb_ushort_t i)
{
  while (i < ZB_NEIGHBOR_TABLE_SIZE
         && !(ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
              && ZB_U2B(ZG->nwk.neighbor.neighbor[i].rx_on_when_idle)))
  {
    TRACE_MSG(TRACE_SECUR2, "skip nb ent %d neighbor %p addr_ref %d used %d rel %d dev_t %d rx_on %d",
              (FMT__H_P_H_H_H_H,
               (zb_uint8_t)i, &ZG->nwk.neighbor.neighbor[i],
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].u.base.addr_ref,
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].used,
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].relationship,
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].device_type,
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].rx_on_when_idle));
    i++;
  }
  if ( i >= ZB_NEIGHBOR_TABLE_SIZE)
  {
    i = (zb_ushort_t)~0U;
  }
  TRACE_MSG(TRACE_SECUR2, "<< zb_nwk_nbr_next_ze_children_i next ze %hd", (FMT__H, i));
  return i;
}


zb_ushort_t zb_nwk_nbr_next_ze_children_i(zb_uint16_t addr, zb_ushort_t i)
{
  while (i < ZB_NEIGHBOR_TABLE_SIZE
         && !(ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
              && (ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_CHILD
                  || ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
              && ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ED
              && (addr == ZB_NWK_BROADCAST_ALL_DEVICES && !ZB_U2B(ZG->nwk.neighbor.neighbor[i].rx_on_when_idle))))
  {
//    TRACE_MSG(TRACE_NWK3, "type %hd, addr 0x%x, rwi %hd", (FMT__H_D_H,(zb_uint8_t)ZG->nwk.neighbor.neighbor[i].device_type, (zb_uint16_t) addr,       (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].rx_on_when_idle));
    i++;
  }

  if ( i >= ZB_NEIGHBOR_TABLE_SIZE)
  {
    i = (zb_ushort_t)~0U;
  }
  else
  {
    TRACE_MSG(TRACE_NWK2, "RET typee %hd, addr 0x%x, rwi %hd",
              (FMT__H_D_H,(zb_uint8_t)ZG->nwk.neighbor.neighbor[i].device_type, (zb_uint16_t) addr,
               (zb_uint8_t)ZG->nwk.neighbor.neighbor[i].rx_on_when_idle));
  }

  TRACE_MSG(TRACE_SECUR3, "next ze %hd", (FMT__H, i));
  return i;
}

zb_ushort_t zb_nwk_nbr_next_ze_children_rx_off_i(zb_ushort_t i)
{
  while (i < ZB_NEIGHBOR_TABLE_SIZE
         && !(ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
              && (ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_CHILD
                  || ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD)
              && !ZB_U2B(ZG->nwk.neighbor.neighbor[i].rx_on_when_idle)) )
  {
    i++;
  }
  if ( i >= ZB_NEIGHBOR_TABLE_SIZE)
  {
    i = (zb_ushort_t)~0U;
  }
  TRACE_MSG(TRACE_SECUR3, "next ze %hd", (FMT__H, i));
  return i;
}
#endif

#if defined ZB_NWK_STOCHASTIC_ADDRESS_ASSIGN && defined ZB_ROUTER_ROLE     /* Zigbee pro */

zb_ret_t zb_nwk_neighbor_with_need_rejoin(zb_neighbor_tbl_ent_t **nbt)
{
  zb_uindex_t i;
  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
  {
    if( ZB_U2B(ZG->nwk.neighbor.neighbor[i].used) &&
        ZB_U2B(ZG->nwk.neighbor.neighbor[i].need_rejoin) &&
        ZG->nwk.neighbor.neighbor[i].device_type == ZB_NWK_DEVICE_TYPE_ED &&
        ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_CHILD)
    {
      *nbt = &ZG->nwk.neighbor.neighbor[i];
      return RET_OK;
    }
  }

  return RET_NOT_FOUND;
}

#endif  /* ZB_ED_ROLE */


#if (defined ZB_ED_ROLE && defined ZB_ED_RX_OFF_WHEN_IDLE)

zb_uint8_t zb_nwk_neighbor_table_size()
{
  return 1;
}

#else

zb_uint8_t zb_nwk_neighbor_table_size()
{
  zb_uindex_t i;
  zb_uint8_t cnt = 0;
  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
  {
    cnt += ZG->nwk.neighbor.neighbor[i].used;
  }

  return cnt;
}

#endif      /* ZB_ED_RX_OFF_WHEN_IDLE */

zb_ret_t zb_nwk_exneighbor_remove_by_panid(zb_address_pan_id_ref_t panid_ref)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uint_t i;

  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
        && ZB_U2B(ZG->nwk.neighbor.neighbor[i].used)
        && ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref == panid_ref)
    {
      ZG->nwk.neighbor.neighbor[i].used = ZG->nwk.neighbor.neighbor[i].ext_neighbor = ZB_FALSE_U;;
      ret = RET_OK;
    }
  }

  return ret;
}

zb_ret_t zb_nwk_exneigbor_sort_by_lqi()
{
  zb_uint8_t i;
  zb_uint8_t j;
  zb_ext_neighbor_tbl_ent_t swap;

  for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE - 1U; i++)
  {
    for (j = 0 ; j < ZB_NEIGHBOR_TABLE_SIZE - i - 1U; j++)
    {
      if (ZB_U2B(ZG->nwk.neighbor.neighbor[j].ext_neighbor)
          && ZB_U2B(ZG->nwk.neighbor.neighbor[j].used)
          && ZB_U2B(ZG->nwk.neighbor.neighbor[j+1U].ext_neighbor)
          && ZB_U2B(ZG->nwk.neighbor.neighbor[j+1U].used)
          && ZG->nwk.neighbor.neighbor[j].lqi < ZG->nwk.neighbor.neighbor[j+1U].lqi)
      {
        ZB_MEMCPY(&swap, &ZG->nwk.neighbor.neighbor[j],
                  sizeof(zb_ext_neighbor_tbl_ent_t));
        ZB_MEMCPY(&ZG->nwk.neighbor.neighbor[j], &ZG->nwk.neighbor.neighbor[j+1U],
                  sizeof(zb_ext_neighbor_tbl_ent_t));
        ZB_MEMCPY(&ZG->nwk.neighbor.neighbor[j+1U], &swap,
                  sizeof(zb_ext_neighbor_tbl_ent_t));
      }
    }
  }

  for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor)
        && ZB_U2B(ZG->nwk.neighbor.neighbor[i].used))
    {
      TRACE_MSG(TRACE_NWK1, "ext_neigb[%hd] panid_ref %hd lqi %hd",
                (FMT__H_D_H, i, ZG->nwk.neighbor.neighbor[i].u.ext.panid_ref,
                 ZG->nwk.neighbor.neighbor[i].lqi));
    }
  }

  return RET_OK;
}

#ifdef ZB_ROUTER_ROLE

#define ZB_NWK_RSSI_FILTER_COEF   85
#define ZB_NWK_RSSI_FILTER_DIVIDER 256

void zb_nwk_neighbour_rssi_store(zb_neighbor_tbl_ent_t *nbt, zb_int8_t rssi)
{
  if (nbt->rssi == (zb_int8_t)ZB_MAC_RSSI_UNDEFINED)
  {
    nbt->rssi = rssi;
  }
  else
  {
    /* Apply simple digital filter to incoming rssi
       Y(n) = A*Y(n-1) + B*X(n),
       where Y(n) - resulting RSSI, Y(n-1) - previous RSSI, X(n) - incoming rssi,
       A & B - filter coefficients: A + B = 256

       Actually, the improved formula is used (B = 85)
       Y(n) = Y(n-1) + (B*(X(n) - Y(n-1)) >> 8); */
    zb_int16_t rssi_16 = (ZB_NWK_RSSI_FILTER_COEF * ((zb_int16_t)rssi - (zb_int16_t)nbt->rssi) / ZB_NWK_RSSI_FILTER_DIVIDER);
    nbt->rssi += (zb_int8_t)rssi_16;
  }
}


#ifdef ZB_NWK_NEIGHBOUR_PATH_COST_RSSI_BASED
/*
  Path cost that was used initially in ZBOSS caused high path cost (>= 5)
  for a devices with quite good RSSI (~-70..-80) and no packets loss.
  As a result, some devices (INNR bulb) started to search for another route
  even if all packets are delivered successfully.

  Based on our reseacrh, if RSSI is about -80..-100, nearly 20% of packets are lost,
  which is acceptable.
  If RSSI is less then -110, then a lot of packets are lost => set high pass cost.

  NOTE: our research was limited, it was done for one particular radio coordinator
  and results may vary if there are different environmental conditions.
*/
zb_uint8_t zb_nwk_neighbour_get_path_cost(zb_neighbor_tbl_ent_t *nbt)
{
  zb_uint8_t path_cost;

  if (nbt->rssi == ZB_MAC_RSSI_UNDEFINED)
  {
    path_cost = 7;
  }
  else if (nbt->rssi >= -60)
  {
    path_cost = 1;
  }
  else if (nbt->rssi >= -80)
  {
    path_cost = 2;
  }
  else if (nbt->rssi >= -100)
  {
    path_cost = 3;
  }
  else if (nbt->rssi >= -110)
  {
    path_cost = 4;
  }
  else
  {
    path_cost = 5;
  }

  return path_cost;
}
#endif /* ZB_NWK_NEIGHBOUR_PATH_COST_RSSI_BASED */

#endif  /* ZB_ROUTER_ROLE */

#if !defined ZB_ED_ROLE && defined ZB_MAC_DUTY_CYCLE_MONITORING
zb_uint8_t zb_nwk_neighbor_get_subghz_list(zb_address_ieee_ref_t *ref_list, zb_uint8_t max_records)
{
  zb_neighbor_tbl_ent_t *ent = &ZG->nwk.neighbor.neighbor[0];
  zb_uint8_t count = 0;
  zb_uint8_t i, j, k, t;

  /* get full list of sub-ghz devices */
  j = 0;
  for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE && count < max_records; i++)
  {
    if (ZB_U2B(ent->used) && ent->is_subghz)
    {
      ref_list[j] = i;
      count++;
      j++;
    }
    ent++;
  }

  /* sort list by pkt_count desc */
  for (k = count / 2U; k > 0U; k /= 2U)
  {
    for (i = k; i < count; i++)
    {
      t = ref_list[i];

      for (j = i; j >= k; j -= k)
      {
        if (ZG->nwk.neighbor.neighbor[t].pkt_count > ZG->nwk.neighbor.neighbor[ref_list[j - k]].pkt_count)
        {
          ref_list[j] = ref_list[j - k];
        }
        else
        {
          break;
        }
      }

      ref_list[j] = t;
    }
  }

  /* substitute indices in 'ref_list' with true addr refs */
  for (i = 0; i < count; i++)
  {
    ref_list[i] = ZG->nwk.neighbor.neighbor[ref_list[i]].u.base.addr_ref;
  }

  return count;
}

#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ

static void nbt_pkt_count_normalize_alarm(zb_uint8_t param)
{
  zb_uint_t i;
  zb_uint_t too_big_cnt;
  zb_uint_t non_zero_cnt;
  zb_uint_t normalize_tmo;

  ZVUNUSED(param);

  do
  {
    too_big_cnt = non_zero_cnt = 0;
    for (i = 0 ; i < ZB_NEIGHBOR_TABLE_SIZE ; i++)
    {
      zb_neighbor_tbl_ent_t *nbt = &ZG->nwk.neighbor.neighbor[i];
      if (ZB_U2B(nbt->used) && nbt->is_subghz)
      {
        nbt->pkt_count = ((zb_uint32_t)nbt->pkt_count) * 3U / 4U;
        too_big_cnt += ZB_B2U(nbt->pkt_count > MAX_NBT_PKT_COUNT/2U);
        non_zero_cnt += ZB_B2U(nbt->pkt_count != 0U);
      }
    }
  } while (too_big_cnt > ZB_NEIGHBOR_TABLE_SIZE / 2U);

  if (non_zero_cnt != 0U)
  {
    /* Normailze counters 2 times per duty cycle period */
    normalize_tmo = zb_mac_duty_cycle_get_time_period_sec() / 2U;
    ZB_SCHEDULE_ALARM(nbt_pkt_count_normalize_alarm, 0, normalize_tmo * ZB_TIME_ONE_SECOND);
  }
}


void nbt_inc_in_pkt_count(zb_neighbor_tbl_ent_t *nbt)
{
  zb_time_t dummy;
  zb_uint_t normalize_tmo;

  nbt->pkt_count++;
  /* Deal with overflow: set back to max value */
  if (nbt->pkt_count == 0U)
  {
    nbt->pkt_count--;
  }

  /* When have non-zero packet cnt, start normalize alarm, if not run it already */
  if (zb_schedule_get_alarm_time(nbt_pkt_count_normalize_alarm, 0, &dummy) != RET_OK)
  {
    /* Normailze counters 2 times per duty cycle period */
    normalize_tmo = zb_mac_duty_cycle_get_time_period_sec() / 2U;
    ZB_SCHEDULE_ALARM(nbt_pkt_count_normalize_alarm, 0, normalize_tmo * ZB_TIME_ONE_SECOND);
  }
}

#endif  /* #if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ */

#endif  /* ZB_ROUTER_ROLE */

void zb_set_nbt_transmit_failure_threshold(zb_uint8_t transmit_failure_cnt)
{
  if (transmit_failure_cnt != 0U)
  {
    ZG->nwk.neighbor.transmit_failure_threshold = transmit_failure_cnt;
  }
}

void zb_set_nbt_transmit_failure_timeout(zb_uint8_t transmit_failure_timeout)
{
  ZG->nwk.neighbor.transmit_failure_timeout = transmit_failure_timeout;
}

#ifdef ZB_ROUTER_ROLE
zb_bool_t zb_nwk_ed_is_our_child(zb_uint16_t addr)
{
  zb_address_ieee_ref_t addr_ref;
  zb_neighbor_tbl_ent_t *nbh = NULL;
  zb_ret_t ret;

  ret = zb_address_by_short(addr, ZB_FALSE, ZB_FALSE, &addr_ref);

  if (ret == RET_OK)
  {
    ret = zb_nwk_neighbor_get(addr_ref, ZB_FALSE, &nbh);
  }

  if (ret == RET_OK)
  {
    if (nbh->device_type == ZB_NWK_DEVICE_TYPE_ED
        && nbh->relationship == ZB_NWK_RELATIONSHIP_CHILD)
    {
      ret = RET_OK;
    }
    else
    {
      ret = RET_ERROR;
    }
  }

  return (ret == RET_OK) ? ZB_TRUE : ZB_FALSE;
}
#endif /* ZB_ROUTER_ROLE */

/*! @} */
