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
/* PURPOSE: APS subsystem. Binding.
*/

#define ZB_TRACE_FILE_ID 100
#include "zb_common.h"
#include "zb_aps.h"
#include "aps_internal.h"
#include "zb_scheduler.h"
#include "zb_hash.h"
#include "zb_address.h"
#include "zb_zdo.h"


/*! \addtogroup ZB_APS */
/*! @{ */


#if defined ZB_NSNG
#define PRINT_GROUP_TABLE()                                                         \
{                                                                                   \
  zb_ushort_t ii, jj;                                                               \
  char str[100];                                                                    \
  for (ii = 0 ; ii < ZG->aps.group.n_groups; ++ii)                                  \
  {                                                                                 \
    char *ptr = str;                                                                \
    ptr += sprintf(ptr, "%i. Group[0x%x]:", ii, ZG->aps.group.groups[ii].group_addr);   \
    for (jj = 0 ; jj < ZG->aps.group.groups[ii].n_endpoints; ++jj)                  \
    {                                                                               \
       ptr += sprintf(ptr, " %i;", ZG->aps.group.groups[ii].endpoints[jj]);         \
    }                                                                               \
    TRACE_MSG(TRACE_APS3, str, (FMT__0));                                           \
  }                                                                                 \
}
#else
#define PRINT_GROUP_TABLE() do { } while(ZB_FALSE)
#endif

zb_uint8_t aps_find_src_ref(zb_address_ieee_ref_t src_addr_ref, zb_uint8_t src_end, zb_uint16_t cluster_id)
{
  zb_uint8_t l = 0;

  TRACE_MSG(TRACE_APS1, "+aps_find_src_ref src_addr_ref %hd src ep %hd, cluster %d", (FMT__H_H_D, src_addr_ref, src_end, cluster_id));

  while (l < ZG->aps.binding.src_n_elements)
  {
    zb_ieee_addr_t a1, a2;
    /* compare not refs but addresses to handle redirects */
    zb_address_ieee_by_ref(a1, src_addr_ref);
    zb_address_ieee_by_ref(a2, ZG->aps.binding.src_table[l].src_addr);

    TRACE_MSG(TRACE_APS3, "ind %hd, src_addr_ref %hd src_end %hd cluster_id %d",
              (FMT__H_H_H_D, l, ZG->aps.binding.src_table[l].src_addr, ZG->aps.binding.src_table[l].src_end, ZG->aps.binding.src_table[l].cluster_id));

    /* Unicast bind req is sent using IEEE address, so compare long addersses */
    if (ZB_IEEE_ADDR_CMP(a1, a2)
        && (ZG->aps.binding.src_table[l].src_end == src_end)
        && (ZG->aps.binding.src_table[l].cluster_id == cluster_id))
    {
      return l;
    }
    l++;
  }

  return (zb_uint8_t)(-1);
}


void zb_aps_check_binding_request(zb_bufid_t param)
{
  zb_aps_check_binding_req_t req_param;
  zb_aps_check_binding_resp_t* resp_param_ptr;

  zb_uindex_t i;
  zb_bool_t bind_entries_found;

  TRACE_MSG(TRACE_APS2, ">> zb_aps_check_binding_request, param", (FMT__D));

  ZB_MEMCPY(&req_param,
            ZB_BUF_GET_PARAM(param, zb_aps_check_binding_req_t),
            sizeof(req_param));
  resp_param_ptr = ZB_BUF_GET_PARAM(param, zb_aps_check_binding_resp_t);

  TRACE_MSG(TRACE_APS2, "src ep %hd, cluster %d, cb %p",
            (FMT__H_D_P, req_param.src_endpoint, req_param.cluster_id, req_param.response_cb));

  for (i = 0; i < ZG->aps.binding.src_n_elements; i++ )
  {
    zb_aps_bind_src_table_t* table_entry = &ZG->aps.binding.src_table[i];
    if (table_entry->cluster_id == req_param.cluster_id
        && (table_entry->src_end == req_param.src_endpoint
            || req_param.src_endpoint == ZB_ZCL_BROADCAST_ENDPOINT))
    {
      break;
    }
  }
  bind_entries_found = (i < ZG->aps.binding.src_n_elements) ? ZB_TRUE : ZB_FALSE;
  TRACE_MSG(TRACE_APS2, "bind_entries_found %d", (FMT__D, bind_entries_found));

  resp_param_ptr->src_endpoint = req_param.src_endpoint;
  resp_param_ptr->cluster_id = req_param.cluster_id;
  resp_param_ptr->exists = bind_entries_found;

  if (req_param.response_cb != NULL)
  {
    ZB_SCHEDULE_CALLBACK(req_param.response_cb, param);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "a callback for zb_aps_check_binding_request is not set!", (FMT__0));
  }

  TRACE_MSG(TRACE_APS2, "<< zb_aps_check_binding_request", (FMT__0));
}


static zb_uint8_t aps_find_dst_ref_ieee(zb_address_ieee_ref_t dst_addr_ref, zb_uint8_t dst_end, zb_uint8_t src)
{
  zb_uint8_t l = 0;

  TRACE_MSG(TRACE_APS1, "+aps_find_dst_ref_ieee dst_addr_ref %hd dst_end %hd, src %d", (FMT__H_H_D, dst_addr_ref, dst_end, src));

  do
  {
    if ( ZG->aps.binding.dst_table[l].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
    {
      zb_ieee_addr_t a1, a2;
      /* compare not refs but addresses to handle redirects */
      zb_address_ieee_by_ref(a1, dst_addr_ref);
      zb_address_ieee_by_ref(a2, ZG->aps.binding.dst_table[l].u.long_addr.dst_addr);

      TRACE_MSG(TRACE_APS3, "ind %hd, dst_end %hd, src %d",
              (FMT__H_H_H_D, l, ZG->aps.binding.dst_table[l].u.long_addr.dst_end, ZG->aps.binding.dst_table[l].src_table_index));

      /* Unicast bind req is sent using IEEE address, so compare long addersses */
      if (ZB_IEEE_ADDR_CMP(a1, a2)
          && (ZG->aps.binding.dst_table[l].u.long_addr.dst_end == dst_end)
          && (ZG->aps.binding.dst_table[l].src_table_index == src))
      {
        return l;
      }
    }
    l++;
  } while (l < ZG->aps.binding.dst_n_elements);

  return (zb_uint8_t)(-1);
}

static zb_uint8_t aps_find_dst_ref_group(zb_uint16_t addr_short, zb_uint8_t src)
{
  zb_uint8_t l = 0;
  do
  {
    if ( (ZG->aps.binding.dst_table[l].dst_addr_mode == ZB_APS_BIND_DST_ADDR_GROUP)
        && (ZG->aps.binding.dst_table[l].u.group_addr == addr_short)
        && (ZG->aps.binding.dst_table[l].src_table_index == src))
    {
      return l;
    }
    l++;
  } while (l < ZG->aps.binding.dst_n_elements);
  return (zb_uint8_t)-1;
}

#ifdef SNCP_MODE
static zb_bool_t zb_assign_available_bind_id(zb_uint8_t param, zb_bool_t remote)
{
  zb_uindex_t i;
  zb_uint8_t id_min;
  zb_uint8_t id_max;
  zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);

  /* define bounds for the id that we need based
     on params remote/local and offset_set/offset_not_set */
  if ( ZG->aps.binding.remote_bind_offset != 0xffU)
  {
    if (ZB_TRUE == remote)
    {
      id_min = ZG->aps.binding.remote_bind_offset;

#ifndef ZB_CERTIFICATION_HACKS
      id_max = ZB_APS_DST_BINDING_TABLE_SIZE - 1U;
#else
      id_max = ZB_CERT_HACKS().dst_binding_table_size - 1U;
#endif
    }
    else
    {
      id_min = 0;
      id_max = ZG->aps.binding.remote_bind_offset - 1U;
    }
  }
  else
  {
    id_min = 0;
#ifndef ZB_CERTIFICATION_HACKS
      id_max = ZB_APS_DST_BINDING_TABLE_SIZE - 1U;
#else
      id_max = ZB_CERT_HACKS().dst_binding_table_size - 1U;
#endif
  }

  /* search the lowest id between range id_min to
      id_max, return 0xff if not available */
  i = 0;
  while (i < ZG->aps.binding.dst_n_elements)
  {
    if (ZG->aps.binding.dst_table[i].id == id_min)
    {
      id_min++;
      if (id_min > id_max)
      {
        return ZB_FALSE;
      }
      i = 0;
    }
    else
    {
      i++;
    }
  }

  apsreq->id = id_min;

  return ZB_TRUE;
}

static zb_bool_t zb_check_if_bind_id_exists(zb_uint8_t id)
{
  zb_uindex_t i;

  for (i = 0; i < ZG->aps.binding.dst_n_elements; i++)
  {
    if (ZG->aps.binding.dst_table[i].id == id)
    {
      return ZB_TRUE;
    }
  }

  return ZB_FALSE;
}
#endif /* SNCP_MODE */

void zb_apsme_bind_request(zb_uint8_t param)
{
  zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
  zb_address_ieee_ref_t src_addr_ref;
  zb_address_ieee_ref_t dst_addr_ref = {0};
  zb_uint8_t s;
  zb_uint8_t d;
  zb_bool_t add_new_src = ZB_FALSE;
  zb_bool_t add_new_dst = ZB_FALSE;
  zb_ret_t status;

  TRACE_MSG(TRACE_APS1, "+zb_apsme_bind_request %hd", (FMT__H, param));

  status = zb_address_by_ieee(apsreq->src_addr, ZB_TRUE, ZB_FALSE, &src_addr_ref);

#ifdef SNCP_MODE
  if (RET_OK == status)
  {
    if (apsreq->remote_bind == 0U)
    {
      if (zb_check_if_bind_id_exists(apsreq->id) == ZB_TRUE)
      {
        TRACE_MSG(TRACE_ERROR, "bind id (%d) requested is already in use", (FMT__D, apsreq->id));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER);
      }
      /* check if remote_bind_offset is set */
      else if (ZG->aps.binding.remote_bind_offset != ((zb_uint8_t)-1))
      {
        if (apsreq->id >= ZG->aps.binding.remote_bind_offset)
        {
          TRACE_MSG(TRACE_ERROR, "bind id (%d) requested is not within range 0 to remote_bind_offset", (FMT__D, apsreq->id));
          status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER);
        }
      }
#ifndef ZB_CERTIFICATION_HACKS
      else if (apsreq->id >= ZB_APS_DST_BINDING_TABLE_SIZE )
#else
      else if (apsreq->id >= ZB_CERT_HACKS().dst_binding_table_size)
#endif
      {
        TRACE_MSG(TRACE_ERROR, "bind id (%d) requested is bigger than the binding table size", (FMT__D, apsreq->id));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER);
      }
      else
      {
        // apsreq->id for local bind is fine and can be inserted
      }

    }
    else if (apsreq->remote_bind == 1U)
    {
      /* remote bindings ids ar only assigned here and are not expected
          to have an valid id untill here */

      if (zb_assign_available_bind_id(param, ZB_U2B(apsreq->remote_bind)) == 0U)
      {
        /* if remote_bind_offset is set its possible that only one part of table reserved for local or
            remote bindings is full and not the whole table */
        TRACE_MSG(TRACE_ERROR, "bind table reserved for remote bind requests is full", (FMT__0));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL);
      }
    }
    else
    {
      //apsreq->remote_bind can only be either ZB_TRUE or ZB_FALSE
    }
  }
#endif

  if (status == RET_OK)
  {
    s = aps_find_src_ref(src_addr_ref, apsreq->src_endpoint, apsreq->clusterid);
    s = ( s == (zb_uint8_t)-1 ) ? ZG->aps.binding.src_n_elements : s;
    /*cstat !MISRAC2012-Rule-2.2_c */
    /* d variable is used in the following `if` independent of ZB_CERTIFICATION_HACKS is defined or not*/
    d = ZG->aps.binding.dst_n_elements;

#ifndef ZB_CERTIFICATION_HACKS
    if ( s < ZB_APS_SRC_BINDING_TABLE_SIZE
         && d < ZB_APS_DST_BINDING_TABLE_SIZE )
#else
    /* Binding table sizes can be limeted in test purposes. */
    if ( s < ZB_CERT_HACKS().src_binding_table_size
         && d < ZB_CERT_HACKS().dst_binding_table_size )
#endif
    {
      if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT )
      {
        status = zb_address_by_ieee(apsreq->dst_addr.addr_long, ZB_TRUE, ZB_FALSE, &dst_addr_ref);
      }
      else if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT )
      {
        /* do nothing */
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "invalid DstAddrMode %hd", (FMT__H, apsreq->addr_mode));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING);
      }

      if (status == RET_OK)
      {
        if ( s == ZG->aps.binding.src_n_elements )
        {
          /* Add new source binding */
          ZG->aps.binding.src_table[s].src_addr = src_addr_ref;
          ZG->aps.binding.src_table[s].src_end = apsreq->src_endpoint;
          ZG->aps.binding.src_table[s].cluster_id = apsreq->clusterid;
          ZG->aps.binding.src_n_elements++;
          add_new_src = ZB_TRUE;
        }

        if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT )
        {
          d = aps_find_dst_ref_ieee(dst_addr_ref, apsreq->dst_endpoint, s);
        }
        else
        {
          d = aps_find_dst_ref_group(apsreq->dst_addr.addr_short, s);
        }
        d = ( d == (zb_uint8_t)-1 ) ? ZG->aps.binding.dst_n_elements : d;

        if (d == ZG->aps.binding.dst_n_elements)
        {
          if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT )
          {
            ZG->aps.binding.dst_table[d].dst_addr_mode = ZB_APS_BIND_DST_ADDR_LONG;
            ZG->aps.binding.dst_table[d].u.long_addr.dst_addr = dst_addr_ref;
            ZG->aps.binding.dst_table[d].u.long_addr.dst_end = apsreq->dst_endpoint;
          }
          else
          {
            ZG->aps.binding.dst_table[d].dst_addr_mode = ZB_APS_BIND_DST_ADDR_GROUP;
            ZG->aps.binding.dst_table[d].u.group_addr = apsreq->dst_addr.addr_short;
          }

          ZG->aps.binding.dst_table[d].src_table_index = s;
#ifdef SNCP_MODE
          ZG->aps.binding.dst_table[d].id = apsreq->id;
#endif
          ZG->aps.binding.dst_n_elements++;
          add_new_dst = ZB_TRUE;
        }
      }
    }
    else
    {
      status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL);
    }
  }

  zb_buf_set_status(param, status);

  /* NK:WARNING: This lock will be cleared on:
     - zb_apsme_unbind_request() - 1 unlock for 1 request
     - leave without rejoin flag - full unlock (for parent)
     In other cases these locks will accumulate and may result to addr lock overflow (assert).
     Maybe need to modify this somehow to reduce locks count for same src-dst-cluster_id (or
     when we reuse old binding entry instead of adding new).
  */
  if (status == RET_OK)
  {
    /* FIXME: We definitely need to ignore same src+dst+cluster+endpoint bind case. Do we need to
     * ignore some other cases? */
    if (add_new_src || add_new_dst)
    {
      /* Lock src and dst addresses only for successful binding */
      status = zb_address_lock(src_addr_ref);
      ZB_ASSERT(status == RET_OK);

      if (apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
      {
        status = zb_address_lock(dst_addr_ref);
        ZB_ASSERT(status == RET_OK);
      }
    }
    /* FIXME: For BDB need save NVRAM atomically after F&B finished. */
#ifdef ZB_USE_NVRAM
    zb_nvram_transaction_start();
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
    zb_nvram_transaction_commit();
#endif /* ZB_USE_NVRAM */
    if (APS_SELECTOR().new_binding_handler != NULL)
    {
      APS_SELECTOR().new_binding_handler(d);
    }
  }

  if (apsreq->confirm_cb != NULL)
  {
    ZB_SCHEDULE_CALLBACK(apsreq->confirm_cb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APS1, "-zb_apsme_bind_request status %d", (FMT__D, status));
}

void apsme_forget_device()
{
  TRACE_MSG(TRACE_APS1, "> apsme_forget_device", (FMT__0));
  /* Unbind all entries */
  zb_apsme_unbind_all(0);
  /* Remove all groups */
  zb_apsme_remove_all_groups_internal();
  TRACE_MSG(TRACE_APS1, "< apsme_forget_device", (FMT__0));
}

/*This is not a Zigbee specified function. Added for SmartPlug Cleans APS binding table.*/
void zb_apsme_unbind_all(zb_uint8_t param)
{
  zb_uint8_t i, j;
  zb_uindex_t retrans_index;

  TRACE_MSG(TRACE_APS1, "> zb_apsme_unbind_all", (FMT__0));

  ZVUNUSED(param);

  for (i = 0 ; i < ZG->aps.binding.src_n_elements ; ++i)
  {
    for (j = 0 ; j < ZG->aps.binding.dst_n_elements ; ++j)
    {
      if (ZG->aps.binding.dst_table[j].src_table_index == i
          && ZG->aps.binding.dst_table[j].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
      {
        zb_address_unlock(ZG->aps.binding.dst_table[j].u.long_addr.dst_addr);
        zb_address_unlock(ZG->aps.binding.src_table[i].src_addr);
      }
    }
  }

  for (retrans_index = 0; retrans_index < ZB_N_APS_RETRANS_ENTRIES; retrans_index++)
  {
    if (ZG->aps.retrans.hash[retrans_index].state != ZB_APS_RETRANS_ENT_FREE)
    {
      zb_address_ieee_ref_t addr_ref;

      if (zb_address_by_short(ZG->aps.retrans.hash[retrans_index].addr, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
      {
        zb_address_unlock(addr_ref);
      }
    }
  }

#ifdef SNCP_MODE
  /* remote_bind_offset must be kept when all bindings are cleared */
  {
    zb_uint8_t tmp_remote_bind_offset = ZG->aps.binding.remote_bind_offset;
    ZB_BZERO(&ZG->aps.binding, sizeof(ZG->aps.binding));
    ZG->aps.binding.remote_bind_offset = tmp_remote_bind_offset;
  }
#else
  ZB_BZERO(&ZG->aps.binding, sizeof(ZG->aps.binding));
#endif
  ZB_RESYNC_CFG_MEM();

#ifdef ZB_USE_NVRAM
  zb_nvram_transaction_start();
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
  zb_nvram_transaction_commit();
#endif /* ZB_USE_NVRAM */

  TRACE_MSG(TRACE_APS1, "< zb_apsme_unbind_all", (FMT__0));
}

#ifdef ZB_CONFIGURABLE_MEM
void zb_apsme_move_dst_bind_table(zb_uint8_t dst_idx, zb_uint8_t src_idx, zb_uint8_t cnt)
{
  zb_uint8_t *trans_index_ptr;

  TRACE_MSG(TRACE_APS1, "zb_apsme_move_dst_bind_table: src_idx %hd dst_idx %hd cnt %hd", (FMT__H_H_H, src_idx, dst_idx, cnt));

  while (cnt)
  {
    TRACE_MSG(TRACE_APS1, "mv src %hd dst %hd",
                (FMT__H_H, src_idx, dst_idx));
    /* Store dst trans_index pointer */
    trans_index_ptr = ZG->aps.binding.dst_table[dst_idx].trans_index;
    /* Copy dst_table element from src to dst */
    ZB_MEMCPY(&ZG->aps.binding.dst_table[dst_idx],
              &ZG->aps.binding.dst_table[src_idx],
              sizeof(zb_aps_bind_dst_table_t));
    /* Restore dst trans_index pointer */
    ZG->aps.binding.dst_table[dst_idx].trans_index = trans_index_ptr;
    /* Copy dst_table.trans_index data from src to dst */
    ZB_MEMCPY(ZG->aps.binding.dst_table[dst_idx].trans_index,
              ZG->aps.binding.dst_table[src_idx].trans_index,
              sizeof(zb_uint8_t) * ZB_SINGLE_TRANS_INDEX_SIZE);
    /* Switch to the next element */
    ++src_idx;
    ++dst_idx;
    --cnt;
  }
}
#endif

void zb_apsme_unbind_by_ref(zb_address_ieee_ref_t addr_ref)
{
  zb_uindex_t i, j;

  TRACE_MSG(TRACE_APS1, "> zb_apsme_unbind_by_ref ref %hd src_n_elements %hd dst_n_elements %hd",
            (FMT__H_H_H, addr_ref, ZG->aps.binding.src_n_elements, ZG->aps.binding.dst_n_elements));

  /* Remove all dst entries */
  i = 0;
  while (i < ZG->aps.binding.dst_n_elements)
  {
    if (ZG->aps.binding.dst_table[i].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
    {
      zb_ieee_addr_t a1, a2;
      /* compare not refs but addresses to handle redirects */
      zb_address_ieee_by_ref(a1, addr_ref);
      zb_address_ieee_by_ref(a2, ZG->aps.binding.dst_table[i].u.long_addr.dst_addr);

      /* Unicast bind req is sent using IEEE address, so compare long addersses */
      if (ZB_IEEE_ADDR_CMP(a1, a2))
      {
        zb_uint8_t src_bind_idx = ZG->aps.binding.dst_table[i].src_table_index;

        TRACE_MSG(TRACE_APS1, "found dst %hd, remove", (FMT__H, i));
        zb_address_unlock(ZG->aps.binding.dst_table[i].u.long_addr.dst_addr);
        zb_address_unlock(ZG->aps.binding.src_table[src_bind_idx].src_addr);
        --ZG->aps.binding.dst_n_elements;
#ifdef ZB_CONFIGURABLE_MEM
        zb_apsme_move_dst_bind_table(i, i+1, ZG->aps.binding.dst_n_elements - i);
#else
        ZB_MEMMOVE(&ZG->aps.binding.dst_table[i],
                   &ZG->aps.binding.dst_table[i+1U],
                   sizeof(zb_aps_bind_dst_table_t)*(ZG->aps.binding.dst_n_elements - i));
#endif
      }
      else
      {
        ++i;
      }
    }
    else
    {
      ++i;
    }
  }

  /* Remove all src entries */
  i = 0;
  while (i < ZG->aps.binding.src_n_elements)
  {
    zb_ieee_addr_t a1, a2;
    /* compare not refs but addresses to handle redirects */
    zb_address_ieee_by_ref(a1, addr_ref);
    zb_address_ieee_by_ref(a2, ZG->aps.binding.src_table[i].src_addr);

    /* Unicast bind req is sent using IEEE address, so compare long addersses */
    if (ZB_IEEE_ADDR_CMP(a1, a2))
    {
      TRACE_MSG(TRACE_APS1, "found src %hd, remove", (FMT__H, i));
      --ZG->aps.binding.src_n_elements;
      ZB_MEMMOVE(&ZG->aps.binding.src_table[i],
                 &ZG->aps.binding.src_table[i+1U],
                 sizeof(zb_aps_bind_src_table_t)*(ZG->aps.binding.src_n_elements - i));

      /* correct dst table indexes */
      j = 0;
      do
      {
        /* remove all corresponding dst-s */
        if ( ZG->aps.binding.dst_table[j].src_table_index == i)
        {
          TRACE_MSG(TRACE_APS1, "found corresponding dst %hd, remove", (FMT__H, i));
          zb_address_unlock(ZG->aps.binding.dst_table[j].u.long_addr.dst_addr);
          zb_address_unlock(ZG->aps.binding.src_table[i].src_addr);
          --ZG->aps.binding.dst_n_elements;
#ifdef ZB_CONFIGURABLE_MEM
          zb_apsme_move_dst_bind_table(j, j+1, ZG->aps.binding.dst_n_elements - j);
#else
          ZB_MEMMOVE(&ZG->aps.binding.dst_table[j],
                     &ZG->aps.binding.dst_table[j+1U],
                     sizeof(zb_aps_bind_dst_table_t)*(ZG->aps.binding.dst_n_elements - j));
#endif
        }
        else
        {
          if ( ZG->aps.binding.dst_table[j].src_table_index > i )
          {
            ZG->aps.binding.dst_table[j].src_table_index--;
          }
          j++;
        }
      }
      while ( j < ZG->aps.binding.dst_n_elements );
    }
    else
    {
      ++i;
    }
  }

  /* Remove unused src entries */
  i = 0;
  while (i < ZG->aps.binding.src_n_elements)
  {
    zb_bool_t found = ZB_FALSE;

    j = 0;
    while (j < ZG->aps.binding.dst_n_elements && !found)
    {
      if ( ZG->aps.binding.dst_table[j].src_table_index == i )
      {
        found = ZB_TRUE;
      }
      j++;
    }

    if (!found)
    {
      TRACE_MSG(TRACE_APS1, "found unused src %hd, remove", (FMT__H, i));
      --ZG->aps.binding.src_n_elements;
      ZB_MEMMOVE(&ZG->aps.binding.src_table[i],
                 &ZG->aps.binding.src_table[i+1U],
                 sizeof(zb_aps_bind_src_table_t)*(ZG->aps.binding.src_n_elements - i));
    }
    else
    {
      ++i;
    }
  }

#ifdef ZB_USE_NVRAM
  zb_nvram_transaction_start();
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
  zb_nvram_transaction_commit();
#endif /* ZB_USE_NVRAM */

  /* FIXME: Do we need to do smth with ZG->aps.binding.trans_table? Maybe do memset(0)? */

  TRACE_MSG(TRACE_APS1, "< zb_apsme_unbind_by_ref src_n_elements %hd dst_n_elements %hd",
            (FMT__H_H, ZG->aps.binding.src_n_elements, ZG->aps.binding.dst_n_elements));
}


void zb_apsme_unbind_request(zb_uint8_t param)
{
  zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
  zb_address_ieee_ref_t src_addr_ref = 0;
  zb_address_ieee_ref_t dst_addr_ref = 0;
  zb_uindex_t s, d = 0;
  zb_uint8_t found = 0;
  zb_uint8_t deleted = 0;
  zb_ret_t status;

  TRACE_MSG(TRACE_APS1, "+zb_apsme_unbind_request %hd", (FMT__H, param));

  status = zb_address_by_ieee(apsreq->src_addr, ZB_FALSE, ZB_FALSE, &src_addr_ref);
  if (status == RET_OK)
  {
    s = aps_find_src_ref(src_addr_ref, apsreq->src_endpoint, apsreq->clusterid);
    if (s == (zb_uint8_t)-1)
    {
      status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING);
    }
    else
    {
      if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT )
      {
        status = zb_address_by_ieee(apsreq->dst_addr.addr_long, ZB_FALSE, ZB_FALSE, &dst_addr_ref);
/*  R21 spec: If the Remote Device is the ZigBee Coordinator or SrcAddress but does not have a Binding Table entry corresponding to the parameters received in the request, a Status of NO_ENTRY is returned. */
        if (status == RET_NOT_FOUND)
        {
          status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING);
        }
      }
      else if ( apsreq->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT )
      {
        /* do nothing */
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "invalid DstAddrMode %hd", (FMT__H, apsreq->addr_mode));
        status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING);
      }

      if (status == RET_OK)
      {
        /* remove all bindings with this dst and src */
        do
        {
          if ( ZG->aps.binding.dst_table[d].src_table_index == s
               && ( (apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT
                     && ZG->aps.binding.dst_table[d].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG
                     && ZG->aps.binding.dst_table[d].u.long_addr.dst_addr == dst_addr_ref
                     && ZG->aps.binding.dst_table[d].u.long_addr.dst_end == apsreq->dst_endpoint)
                    || (apsreq->addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
                        && ZG->aps.binding.dst_table[d].dst_addr_mode == ZB_APS_BIND_DST_ADDR_GROUP
                        && ZG->aps.binding.dst_table[d].u.group_addr == apsreq->dst_addr.addr_short) )
            )
          {

#ifdef SNCP_MODE
            apsreq->id = ZG->aps.binding.dst_table[d].id;
#endif

            /* move all records left */
            ZG->aps.binding.dst_n_elements--;
#ifdef ZB_CONFIGURABLE_MEM
            zb_apsme_move_dst_bind_table(d, d+1, ZG->aps.binding.dst_n_elements - d);
#else
            ZB_MEMMOVE(&ZG->aps.binding.dst_table[d],
                       &ZG->aps.binding.dst_table[d+1U],
                       sizeof(zb_aps_bind_dst_table_t)*(ZG->aps.binding.dst_n_elements - d));
#endif
            deleted = 1;
          }
          else
          {
            if ( ZG->aps.binding.dst_table[d].src_table_index == s )
            {
              found = 1;
            }
            d++;
          }
        }
        while ( d < ZG->aps.binding.dst_n_elements );

        if ( found == 0U )
        {
          /* remove from src table useless binding record */
          ZG->aps.binding.src_n_elements--;
          ZB_MEMMOVE(&ZG->aps.binding.src_table[s],
                     &ZG->aps.binding.src_table[s+1U],
                     sizeof(zb_aps_bind_src_table_t)*(ZG->aps.binding.src_n_elements - s));

          /* correct dst table indexes */
          d = 0;
          do
          {
            if ( ZG->aps.binding.dst_table[d].src_table_index > s )
            {
              ZG->aps.binding.dst_table[d].src_table_index--;
            }
            d++;
          }
          while ( d < ZG->aps.binding.dst_n_elements );
        }
        else
        {
          /* check that binding for dst_table has been deleted */
          status = (deleted == 1U)? RET_OK: ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_BINDING);
        }
      }
    }
  }
/* R21 Spec: If the Remote Device is not the ZigBee Coordinator or the SrcAddress, a Status of NOT_SUPPORTED is returned*/
  else if (status != RET_NOT_FOUND)
  {
    status = RET_ILLEGAL_REQUEST;
  }

  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }

  zb_buf_set_status(param, status);

  if (status == RET_OK)
  {
    /* Lock src and dst addresses only for successful binding */
    zb_address_unlock(src_addr_ref);

    if (apsreq->addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      zb_address_unlock(dst_addr_ref);
    }

#ifdef ZB_USE_NVRAM
    zb_nvram_transaction_start();
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
    (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
    zb_nvram_transaction_commit();
#endif /* ZB_USE_NVRAM */
  }

  if (apsreq->confirm_cb != NULL)
  {
    ZB_SCHEDULE_CALLBACK(apsreq->confirm_cb, param);
  }
  else
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_APS1, "-zb_apsme_unbind_request status %d", (FMT__D, status));
}


/* See 2.2.4.5.3.3; 2.2.4.5.5.3; */
static zb_bool_t zb_check_ep(zb_uint8_t _ep)
{
  zb_bool_t res;

  if (_ep > EP_RESERVED_START_ID || _ep == EP_DEVICE_EP)
  {
    res = ZB_FALSE;
  }
  else
  {
#ifdef ZB_ENABLE_ZCL
    if (!ZB_AF_IS_EP_REGISTERED(_ep)
#ifdef ZB_CERTIFICATION_HACKS
        && ZB_CERT_HACKS().allow_entry_for_unregistered_ep == 0U
#endif /* ZB_CERTIFICATION_HACKS */
      )
    {
      res = ZB_FALSE;
    }
    else
#endif /* ZB_ENABLE_ZCL */
    {
      res = ZB_TRUE;
    }
  }
  return res;
}

static zb_bool_t zb_check_group_addr(zb_uint16_t _gr)
{
  if (_gr < 0xFFF7U)
  {
    return ZB_TRUE;
  }
  return ZB_FALSE;
}

zb_ret_t zb_apsme_add_group_internal(zb_uint16_t group, zb_uint8_t ep)
{
  return zb_aps_group_table_add(&ZG->aps.group, group, ep);
}

zb_ret_t zb_apsme_remove_group_internal(zb_uint16_t group, zb_uint8_t ep)
{
  return zb_aps_group_table_remove(&ZG->aps.group, group, ep);
}

void zb_apsme_remove_all_groups_internal()
{
#ifndef ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF
  zb_uint8_t param;

  while (!ZB_RING_BUFFER_IS_EMPTY(&ZG->aps.group.local_dup_q))
  {
    param = *ZB_RING_BUFFER_GET(&ZG->aps.group.local_dup_q);
    ZB_RING_BUFFER_FLUSH_GET(&ZG->aps.group.local_dup_q);
    zb_buf_free(param);
  }
#endif  /* ZB_LITE_APS_DONT_TX_PACKET_TO_MYSELF */

  zb_aps_group_table_remove_all(&ZG->aps.group);
}

void zb_apsme_add_group_request(zb_uint8_t param)
{
  zb_ret_t status = 0;
  zb_apsme_add_group_req_t req;

  ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(param, zb_apsme_add_group_req_t), sizeof(req));
  TRACE_MSG(TRACE_APS3, "zb_apsme_add_group_request group_addr 0x%x endpoint %hd", (FMT__D_H, req.group_address, req.endpoint));

  if (!zb_check_ep(req.endpoint) || !zb_check_group_addr(req.group_address))
  {
    status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER);
  }
  else
  {
    if( zb_apsme_add_group_internal(req.group_address, req.endpoint) == ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL))
    {
      status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL);
    }
#ifdef ZB_USE_NVRAM
    else
    {
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    }
#endif
  }
  {
    zb_apsme_add_group_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_apsme_add_group_conf_t);
    conf->group_address = req.group_address;
    conf->endpoint = req.endpoint;
    conf->status = status;

    /* Run user confirm function - APSME-ADD-GROUP.confirm via callback */
    if (req.confirm_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(req.confirm_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    /* Print group table */
    PRINT_GROUP_TABLE();
  }
}

void zb_apsme_remove_group_request(zb_uint8_t param)
{
  zb_ret_t status;
  zb_apsme_remove_group_req_t req;

  ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(param, zb_apsme_remove_group_req_t), sizeof(req));
  TRACE_MSG(TRACE_APS3, "zb_apsme_remove_group_request group_addr 0x%x endpoint %hd", (FMT__D_H, req.group_address, req.endpoint));

  {
    /* see 2.2.4.5.3.3 */
    if (!zb_check_ep(req.endpoint) || !zb_check_group_addr(req.group_address))
    {
      status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_PARAMETER);
      goto done;
    }

    status = zb_apsme_remove_group_internal(req.group_address, req.endpoint);
#ifdef ZB_USE_NVRAM
    if (status == (zb_ret_t)ZB_APS_STATUS_SUCCESS)
    {
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    }
#endif
  }

  done:
  {
    zb_apsme_remove_group_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_apsme_remove_group_conf_t);
    conf->group_address = req.group_address;
    conf->endpoint = req.endpoint;
    conf->status = status;

    /* Run user confirm function - APSME-REMOVE-GROUP.confirm via callback */
    if (req.confirm_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(req.confirm_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }

    /* Print group table */
    PRINT_GROUP_TABLE();
  }
}


void zb_apsme_remove_all_groups_request(zb_uint8_t param)
{
  zb_ret_t status = RET_OK;
  zb_apsme_remove_all_groups_req_t req;

  ZB_MEMCPY(&req, ZB_BUF_GET_PARAM(param, zb_apsme_remove_all_groups_req_t), sizeof(req));
  TRACE_MSG(TRACE_APS3, "zb_apsme_remove_all_groups_request endpoint %hd", (FMT__H, req.endpoint));

  {
    zb_uindex_t  i,j;
    zb_bool_t found = ZB_FALSE;

    /* see 2.2.4.5.5.3 */
    /* TODO: Rule 14.3 - ZB_PRO_TESTING_MODE forces zb_check_ep() always returns ZB_TRUE, ZOI-497. */
    if ( !zb_check_ep(req.endpoint) )
    {
      status = RET_INVALID_PARAMETER;
      goto done;
    }

    i = ZG->aps.group.n_groups;
    while (i != 0U)
    {
      i--;
      for (j = 0 ; j < ZG->aps.group.groups[i].n_endpoints ; ++j)
      {
        if (ZG->aps.group.groups[i].endpoints[j] == req.endpoint)
        {
          TRACE_MSG(TRACE_ERROR, "remove entry from table [%hd][%hd]", (FMT__H_H, i, j));
          found = ZB_TRUE;
          /* Remove this entry from table, shift rest of the elements to the left */
          if (i < (zb_uindex_t)ZG->aps.group.n_groups - 1U)
          {
            ZB_MEMMOVE( &(ZG->aps.group.groups[i]),
                        &(ZG->aps.group.groups[i+1U]),
                        ((zb_uindex_t)ZG->aps.group.n_groups - 1U - i)*sizeof(zb_aps_group_table_ent_t));
          }
          --ZG->aps.group.n_groups;
          break;
        }
      }
    }

    if (!found)
    {
      TRACE_MSG(TRACE_ERROR, "no such entries in the group table", (FMT__0));
      status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_GROUP);
    }
#ifdef ZB_USE_NVRAM
    else
    {
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    }
#endif
  }

  done:
  {
    zb_apsme_remove_all_groups_conf_t *conf = ZB_BUF_GET_PARAM(param, zb_apsme_remove_all_groups_conf_t);
    conf->endpoint = req.endpoint;
    conf->status = status;

    /* Run user confirm function - APSME-REMOVE-ALL-GROUPS.confirm via callback */
    if (req.confirm_cb != NULL)
    {
      ZB_SCHEDULE_CALLBACK(req.confirm_cb, param);
    }
    else
    {
      zb_buf_free(param);
    }
    /* Print group table */
    PRINT_GROUP_TABLE();
  }
}

void zb_apsme_get_group_membership_request(zb_uint8_t param)
{
  /* Print group table */
  PRINT_GROUP_TABLE();

  zb_apsme_internal_get_group_membership_request(&ZG->aps.group, param);
}

zb_bool_t zb_aps_is_in_group(zb_uint16_t grp_id)
{
  zb_bool_t res = ZB_FALSE;
  zb_ushort_t i;

  TRACE_MSG(TRACE_APS1, "> zb_aps_is_in_group, grp_id 0x%x groups in base:%d", (FMT__D_D, grp_id, ZG->aps.group.n_groups));
  for (i = 0; i < ZG->aps.group.n_groups; ++i)
  {
    TRACE_MSG(TRACE_APS1, "ChkGroup: 0x%04x", (FMT__D, ZG->aps.group.groups[i].group_addr));
    if (ZG->aps.group.groups[i].group_addr == grp_id)
    {
      res = ZB_TRUE;
      break;
    }
  }

  TRACE_MSG(TRACE_APS1, "< zb_aps_is_in_group %hd", (FMT__H, (zb_uint8_t)res));
  return res;
}

zb_aps_group_table_ent_t* zb_aps_get_group_table_entry(zb_uint16_t group_addr)
{
  zb_uint8_t entry_idx;
  zb_aps_group_table_ent_t* result;

  TRACE_MSG(TRACE_APS1, "> zb_aps_get_group_table_entry group_addr 0x%04x", (FMT__D, group_addr));

  for (entry_idx = 0; entry_idx < ZG->aps.group.n_groups; ++entry_idx)
  {
    if (ZG->aps.group.groups[entry_idx].group_addr == group_addr)
    {
      break;
    }
  }

  result = ((entry_idx < ZG->aps.group.n_groups) ? &(ZG->aps.group.groups[entry_idx]) : NULL);

  TRACE_MSG(TRACE_APS1, "< zb_aps_get_group_table_entry result %p", (FMT__P, result));

  return result;
}/* zb_aps_group_table_ent_t* zb_aps_get_group_table_e... */

#if 0  /* APS Group management not supported */
void zb_apsme_add_group_request(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;
  zb_apsme_group_req_t *apsgrp = ZB_GET_BUF_TAIL(grp, sizeof(zb_apsme_group_req_t));
  zb_address_ieee_ref_t addr_ref;
  zb_ushort_t l = 0;

  TRACE_MSG(TRACE_APS1, ">>apsme_add_group_req %p", (FMT__P, v_grp));
  grp->u.hdr.status = RET_OK;
  if ((apsgrp->group_addr == 0) || (apsgrp->endpoint == 0))
  {
    grp->u.hdr.status = RET_INVALID_PARAMETER;
  }
  else
  {
    grp->u.hdr.status = zb_address_by_short( apsgrp->group_addr, ZB_FALSE, &addr_ref);
    if (grp->u.hdr.status == RET_OK)
    {
      l = ZG->aps.group.grp_n_elements;
      if (l < ZB_APS_GROUP_TABLE_SIZE)
      {
        ZG->aps.group.group_addr[l] = addr_ref;
        ZG->aps.group.endpoint[l] = apsgrp->endpoint;
        ZG->aps.group.grp_n_elements++;
        zb_address_lock( ZG->aps.group.group_addr[l]);
      }
      else
      {
        grp->u.hdr.status = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_TABLE_FULL);
      }
    }
    else
    {
      grp->u.hdr.status = RET_INVALID_PARAMETER;
    }
  }
  TRACE_MSG(TRACE_APS1, "<<apsme_add_group_req status %d", (FMT__D, grp->u.hdr.status));
}

void zb_apsme_remove_group_request(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;
  zb_apsme_group_req_t *apsgrp = ZB_GET_BUF_TAIL(grp, sizeof(zb_apsme_group_req_t));
  zb_address_ieee_ref_t addr_ref;
  zb_ushort_t l = 0;
  zb_short_t  found = 0;

  TRACE_MSG(TRACE_APS1, ">>apsme_remove_group_req %p", (FMT__P, v_grp));
  grp->u.hdr.status = RET_OK;
  if ((apsgrp->group_addr == 0) || (apsgrp->endpoint == 0))
  {
    grp->u.hdr.status = RET_INVALID_PARAMETER;
  }
  else
  {
    grp->u.hdr.status = zb_address_by_short( apsgrp->group_addr, ZB_FALSE, &addr_ref);
    if (grp->u.hdr.status == RET_OK)
    {
      l = 0;
      do
      {
        if ((ZG->aps.group.group_addr[l] == addr_ref)
            && (ZG->aps.group.endpoint[l] == apsgrp->endpoint))
        {
          if (found == 0)
          {
            found = 1;
          }
          do
          {
            ZG->aps.group.group_addr[l] = ZG->aps.group.group_addr[l+1];
            ZG->aps.group.endpoint[l] = ZG->aps.group.endpoint[l+1];
            l++;
          } while (l < ZG->aps.group.grp_n_elements-1);
          if (found)
          {
            zb_address_unlock( addr_ref);
          }
          ZG->aps.group.grp_n_elements--;
          l++;
        }
      } while (l < ZG->aps.group.grp_n_elements);
    }
    else
    {
      grp->u.hdr.status = RET_INVALID_PARAMETER;
    }
    if (found == 0)
    {
      grp->u.hdr.status = RET_INVALID_GROUP;
    }
  }
  TRACE_MSG(TRACE_APS1, "<<apsme_remove_group_req status %d", (FMT__D, grp->u.hdr.status));
}

void zb_apsme_remove_all_group_request(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;
  zb_apsme_group_req_t *apsgrp = ZB_GET_BUF_TAIL(grp, sizeof(zb_apsme_group_req_t));
  zb_ushort_t l, s = 0;
  zb_short_t  found, unlock = 0;

  TRACE_MSG(TRACE_APS1, ">>apsme_remove_all_group_req %p", (FMT__P, v_grp));
  grp->u.hdr.status = RET_OK;
  if (apsgrp->endpoint == 0)
  {
    grp->u.hdr.status = RET_INVALID_PARAMETER;
  }
  else
  {
    if (grp->u.hdr.status == RET_OK)
    {
      l = 0;
      do
      {
        if (s != 0)
        {
          l = s;
          s = 0;
        }
        if (ZG->aps.group.endpoint[l] == apsgrp->endpoint)
        {
          if (found == 0)
          {
            found = 1;
            unlock = 1;
          }
          s = l;
          do
          {
            ZG->aps.group.group_addr[l] = ZG->aps.group.group_addr[l+1];
            ZG->aps.group.endpoint[l] = ZG->aps.group.endpoint[l+1];
          } while (l < ZG->aps.group.grp_n_elements-1);
          if (unlock)
          {
            zb_address_unlock( ZG->aps.group.group_addr[s]);
            unlock = 0;
          }
          ZG->aps.group.grp_n_elements--;
        }
      } while ((l < ZG->aps.group.grp_n_elements) && (s != 0));
    }
    else
    {
      grp->u.hdr.status = RET_INVALID_PARAMETER;
    }
    if (found == 0)
    {
      grp->u.hdr.status = RET_INVALID_PARAMETER;
    }
  }
  TRACE_MSG(TRACE_APS1, "<<apsme_remove_all_group_req status %d", (FMT__D, grp->u.hdr.status));
}

void zb_apsme_add_group_confirm(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;

  TRACE_MSG(TRACE_APS2, "+add_group_confirm %p status %d", (FMT__P_D, grp, grp->u.hdr.status));
  zb_free_buf( grp);
}

void zb_apsme_remove_group_confirm(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;

  TRACE_MSG(TRACE_APS2, "+remove_group_confirm %p status %d", (FMT__P_D, grp,grp->u.hdr.status));
  zb_free_buf( grp);
}

void zb_apsme_remove_all_group_confirm(ZPAR void *v_grp)
{
  zb_bufid_t grp = (zb_bufid_t )v_grp;

  TRACE_MSG(TRACE_APS2, "+remove_all_group_confirm %p status %d", (FMT__P_D, grp, grp->u.hdr.status));
  zb_free_buf( grp);
}
#endif /* APS Group management not supported */
/*! @} */
