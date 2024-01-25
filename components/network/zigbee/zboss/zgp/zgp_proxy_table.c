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
/*  PURPOSE: Implementation of Proxy table
*/

#define ZB_TRACE_FILE_ID 2110

#include "zb_common.h"
#ifdef ZB_ENABLE_ZGP

#include "zb_magic_macros.h"
#include "zgp/zgp_internal.h"

/*
  Implemented universal API for Proxy table and Sink table.
  Both tables are partially stored in nvram.

  Proxy table API:
  - Seek by zgpd id and return Proxy table entry + index
  - Update Proxy table entry

  Divide proxy table storage to resident part and nvram-only part.
  Use logic similar to Installcodes storage (see zb_secur_ic_get_from_tc_storage).
  Have single proxy table entry cache in RAM to speedup continues dups detection.
  Remember index of last accessed proxy table entry.


  GPD security frame counter need not be saved in nvram

  Resident part of Proxy table:
  - nvram_offset 32 bits. if == 0, entry is empty.
  - security frame counter 32bit
  Separate array (te be better aligned)
  - security counter expire timer (in 1/8 seconds) - 4bit
 */


/* Pack 8 4-bits "timeout expire" entries into single 32-bit word */
#define ZGP_PROXY_GET_TIMEOUT(arr, i) (((arr)[(i) / 8] >> (((i) % 8) * 4)) & 0xf)
#define ZGP_PROXY_CLR_TIMEOUT(arr, i)                               \
  ((arr)[(i) / 8] &= ((arr)[(i) / 8] & (~(0xf << (((i) % 8) * 4)))))
#define ZGP_PROXY_SET_TIMEOUT(arr, i, to)                               \
  ((arr)[(i) / 8] = (((to) & 0xf) << (((i) % 8) * 4)) | ((arr)[(i) / 8] & (~(0xf << (((i) % 8) * 4)))))

#define ZGP_MAX_DUP_TO 15
#define ZGP_SECURITY_COUNTER_ALARM_INTERVAL (ZB_ZGP_DUPLICATE_TIMEOUT / ZGP_MAX_DUP_TO)

static void zgp_tbls_aging_alarm(zb_uint8_t param);

static zb_uint8_t tbl_alarm_running;

void zgp_tbl_init()
{
  TRACE_MSG(TRACE_ZGP1, "zgp_tbl_init", (FMT__0));
  /* Suppose memory is already zeroed. */
#ifdef ZB_ENABLE_ZGP_SINK
  ZGP_CTXC().sink_table.base.tbl_size = ZB_ZGP_SINK_TBL_SIZE;
  ZGP_CTXC().sink_table.base.entry_size = sizeof(zgp_tbl_ent_t);
#ifdef ZB_USE_NVRAM
  ZGP_CTXC().sink_table.base.nvram_dataset = ZB_NVRAM_DATASET_GP_SINKT;
#endif  /* ZB_USE_NVRAM */
  ZGP_CTXC().sink_table.base.cached_i = (-1);
  ZB_BZERO(ZGP_CTXC().app_table, sizeof(ZGP_CTXC().app_table));
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
  ZGP_CTXC().proxy_table.base.tbl_size = ZB_ZGP_PROXY_TBL_SIZE;
  ZGP_CTXC().proxy_table.base.entry_size = sizeof(zgp_tbl_ent_t);
#ifdef ZB_USE_NVRAM
  ZGP_CTXC().proxy_table.base.nvram_dataset = ZB_NVRAM_DATASET_GP_PRPOXYT;
#endif  /* ZB_USE_NVRAM */
  ZGP_CTXC().proxy_table.base.cached_i = (zb_uint_t)(-1);
#endif  /* defined ZB_ENABLE_ZGP_PROXY */

  ZB_SCHEDULE_ALARM(zgp_tbls_aging_alarm, 0, ZGP_SECURITY_COUNTER_ALARM_INTERVAL);
}

void zgp_tbl_clear()
{
  ZB_SCHEDULE_ALARM_CANCEL(zgp_tbls_aging_alarm, 0);

#ifdef ZB_ENABLE_ZGP_SINK
  ZB_BZERO(&ZGP_CTXC().sink_table, sizeof(ZGP_CTXC().sink_table));
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
  ZB_BZERO(&ZGP_CTXC().proxy_table, sizeof(ZGP_CTXC().proxy_table));
#endif  /* ZB_ENABLE_ZGP_PROXY */

  zgp_tbl_init();

#ifdef ZB_USE_NVRAM
#ifdef ZB_ENABLE_ZGP_SINK
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_DATASET_GP_SINKT);
#endif /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_DATASET_GP_PRPOXYT);
#endif  /* ZB_ENABLE_ZGP_PROXY */
#endif  /* ZB_USE_NVRAM */
}

static zb_bool_t age_tbl(zb_zgp_tbl_t *tbl)
{
  zb_uint_t i;
  zb_uint8_t to;
  zb_bool_t found = ZB_FALSE;

  TRACE_MSG(TRACE_ZGP3, "age_tbl %p, size=%d", (FMT__P_D, tbl, tbl->tbl_size));

  for (i = 0 ; i < (tbl->tbl_size) ; ++i)
  {
    /* skip all-zero words (8 4-bit entries each) */
    /*if (tbl->security_counter_timeouts[MAGIC_TRUNC_TO_8(i)] == 0)
    {
      i += 8;
      continue;
    }*/
    if ((to = ZGP_PROXY_GET_TIMEOUT(tbl->security_counter_timeouts, i)) != 0)
    {
      found = ZB_TRUE;
      to--;
      TRACE_MSG(TRACE_ZGP3, "age_tbl: update timeout [%d] %hd", (FMT__D_H, i, to));
      ZGP_PROXY_SET_TIMEOUT(tbl->security_counter_timeouts, i, to);
    }
  }
  return found;
}

static void zgp_tbls_aging_alarm(zb_uint8_t param)
{
  zb_bool_t found = ZB_FALSE;

  ZVUNUSED(param);

#ifdef ZB_ENABLE_ZGP_SINK
  found = age_tbl((zb_zgp_tbl_t*)&ZGP_CTXC().sink_table);
#endif  /* ZB_ENABLE_ZGP_SINK */

#ifdef ZB_ENABLE_ZGP_PROXY
  found = (zb_bool_t)(found || age_tbl((zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table));
#endif  /* ZB_ENABLE_ZGP_PROXY */

  if (found)
  {
    ZB_SCHEDULE_ALARM(zgp_tbls_aging_alarm, 0, ZGP_SECURITY_COUNTER_ALARM_INTERVAL);
    tbl_alarm_running = 1;
  }
  else
  {
    tbl_alarm_running = 0;
  }
}

static zb_int_t zgp_tbl_addr_hash(zb_zgpd_addr_t *zgpd_id, zb_uint_t id_size)
{
  zb_zgpd_addr_t addr;
  zb_int_t       hash;
  /* Fill address having trailing zeroes for 4-byte zgpd id */
  ZB_BZERO(&addr, sizeof(addr));
  ZB_MEMCPY(&addr, zgpd_id, id_size);
  hash = zb_64bit_hash((zb_uint8_t *)&addr);
  ZB_DUMP_IEEE_ADDR(addr.ieee_addr);
  TRACE_MSG(TRACE_ZGP3, "hash: %d", (FMT__D, hash));
  return hash;
}

static zb_bool_t zgp_is_equal_zgpd_id_for_table_entry(zgp_tbl_ent_t *ent, zb_zgpd_id_t *id)
{
  zb_bool_t ret = ZB_FALSE;

  ret = (zb_bool_t)(ZGP_TBL_GET_APP_ID((ent)) == (id)->app_id);

  if (ret == ZB_TRUE)
  {
    switch ((id)->app_id)
    {
      case ZB_ZGP_APP_ID_0000:
        ret &= (zb_bool_t)((ent)->zgpd_id.src_id == (id)->addr.src_id ||
                           (ent)->zgpd_id.src_id == ZB_ZGP_SRC_ID_ALL);
        break;
      case ZB_ZGP_APP_ID_0010:
        ret &= (zb_bool_t)(!ZB_MEMCMP(&(id)->addr, &(ent)->zgpd_id, sizeof(zb_ieee_addr_t)) ||
                           ZB_IEEE_ADDR_IS_UNKNOWN((ent)->zgpd_id.ieee_addr));
        if (ret == ZB_TRUE)
        {
          ret &= (zb_bool_t)((id)->endpoint == (ent)->endpoint ||
                             (id)->endpoint == ZB_ZGP_ALL_ENDPOINTS ||
                             (ent)->endpoint == ZB_ZGP_ALL_ENDPOINTS ||
                             (id)->endpoint == ZB_ZGP_COMMUNICATION_ENDPOINT);
        }
        break;
      default:
        ret = ZB_FALSE;
        break;
    }
  }

  return ret;
}

static zb_ret_t zgp_tbl_load(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZGP3, ">>zgp_tbl_load zgpd_id %p tbl %p", (FMT__P_P, zgpd_id, tbl));

  ZB_DUMP_ZGPD_ID(*zgpd_id);

  if ((tbl->cached_i == (zb_uint_t)(-1)) ||
      ((tbl->cached_i != (zb_uint_t)(-1)) && !zgp_is_equal_zgpd_id_for_table_entry(&tbl->cached, zgpd_id)))
  {
    zb_int_t idx = zgp_tbl_addr_hash(&zgpd_id->addr, ZGPD_ID_SIZE(zgpd_id)) % tbl->tbl_size;
    zb_uint_t n = 0;
    ret = RET_NOT_FOUND;
    /* Utilize open address hash */

    while (n < tbl->tbl_size)
    {
      if (tbl->array[idx].nvram_offset != 0)
      {
        zgp_tbl_ent_t rec;
#ifdef ZB_USE_NVRAM
        if (zb_osif_nvram_read(tbl->nvram_page, tbl->array[idx].nvram_offset, (zb_uint8_t*)&rec, tbl->entry_size) == RET_OK
            && zgp_is_equal_zgpd_id_for_table_entry(&rec, zgpd_id))
#endif
        {
          tbl->cached_i = idx;
          tbl->cached = rec;
          tbl->cached.security_counter = tbl->array[idx].security_counter;
          TRACE_MSG(TRACE_ZGP4, "zgp_tbl_load: cached record %d", (FMT__D, idx));
          ret = RET_OK;
          break;
        }
      }
      idx = (idx + 1) % tbl->tbl_size;
      n++;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP4, "zgp_tbl_load: already cached", (FMT__0));
  }
  TRACE_MSG(TRACE_ZGP3, "<<zgp_tbl_load %d", (FMT__D, ret));
  return ret;
}

/**
 * @brief Load table entry by index
 *
 * @param idx   [in]  Index of table entry which needed load
 * @param tbl   [in]  Pointer to the table for search entry
 *
 */
static zb_ret_t zgp_tbl_load_by_idx(zb_uint_t idx, zb_zgp_tbl_t *tbl)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZGP3, ">>zgp_tbl_load_by_idx idx %d tbl %p", (FMT__D_P, idx, tbl));

  if (tbl->cached_i != idx)
  {
    ret = RET_NOT_FOUND;

    if (tbl->array[idx].nvram_offset != 0)
    {
      zgp_tbl_ent_t rec;
#ifdef ZB_USE_NVRAM
      if (zb_osif_nvram_read(tbl->nvram_page, tbl->array[idx].nvram_offset, (zb_uint8_t*)&rec, tbl->entry_size) == RET_OK)
#endif
      {
        tbl->cached_i = idx;
        tbl->cached = rec;
        tbl->cached.security_counter = tbl->array[idx].security_counter;
        TRACE_MSG(TRACE_ZGP4, "zgp_tbl_load_by_idx: cached record %d", (FMT__D, idx));
        ret = RET_OK;
      }
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP4, "zgp_tbl_load_by_idx: already cached", (FMT__0));
  }
  TRACE_MSG(TRACE_ZGP3, "<<zgp_tbl_load_by_idx %d", (FMT__D, ret));
  return ret;
}

/**
   returns security counter for that zgpd.

   @return counter value of ~0 if no entry
 */
static zb_uint32_t zgp_tbl_get_security_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_uint32_t counter = ~0;

  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    counter = tbl->array[tbl->cached_i].security_counter;
  }
  TRACE_MSG(TRACE_ZGP3, "zgp_tbl_get_security_counter ret %ld", (FMT__L, counter));
  return counter;
}

/**
   returns duplicate counter for that zgpd.

   @return counter value of ~0 if no entry or expired.
 */
static zb_uint32_t zgp_tbl_get_dup_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_uint32_t counter = ~0;

  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    if(ZGP_PROXY_GET_TIMEOUT(tbl->security_counter_timeouts, tbl->cached_i) != 0)
    {
      counter = tbl->array[tbl->cached_i].security_counter;
    }
  }
  TRACE_MSG(TRACE_ZGP3, "zgp_tbl_get_dup_counter ret %ld", (FMT__L, counter));
  return counter;
}

static zb_ret_t zgp_tbl_restore_security_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    tbl->array[tbl->cached_i].security_counter = tbl->array[tbl->cached_i].back_sec_counter;
    tbl->cached.security_counter = tbl->array[tbl->cached_i].security_counter;
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_restore_security_counter %ld", (FMT__L, tbl->cached.security_counter));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_restore_security_counter: not found", (FMT__0));
    return RET_NOT_FOUND;
  }
}

static zb_ret_t zgp_tbl_set_security_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zb_uint32_t counter)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    tbl->cached.security_counter = counter;
    tbl->array[tbl->cached_i].back_sec_counter = tbl->array[tbl->cached_i].security_counter;
    tbl->array[tbl->cached_i].security_counter = counter;
    ZGP_PROXY_SET_TIMEOUT(tbl->security_counter_timeouts, tbl->cached_i, ZGP_MAX_DUP_TO);

    if (!tbl_alarm_running)
    {
      ZB_SCHEDULE_ALARM(zgp_tbls_aging_alarm, 0, ZGP_SECURITY_COUNTER_ALARM_INTERVAL);
    }
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_set_security_counter %ld", (FMT__L, counter));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_set_security_counter: not found", (FMT__0));
    return RET_NOT_FOUND;
  }
}

#ifdef ZB_ENABLE_ZGP_PROXY
/**
   returns search counter for that zgpd.

   @return counter value of ~0 if no entry
 */
static zb_uint8_t zgp_tbl_get_search_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_uint8_t counter;

  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    counter = tbl->array[tbl->cached_i].search_counter;
  }
  else
  {
    counter = ~0;
  }
  TRACE_MSG(TRACE_ZGP3, "zgp_tbl_get_search_counter ret %hd", (FMT__H, counter));
  return counter;
}

static zb_ret_t zgp_tbl_set_search_counter(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zb_uint8_t counter)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    tbl->array[tbl->cached_i].search_counter = counter;
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_set_search_counter %hd", (FMT__H, counter));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_set_search_counter: not found", (FMT__0));
    return RET_NOT_FOUND;
  }
}
#endif  /* ZB_ENABLE_ZGP_PROXY */

/**
   returns security counter for that zgpd.

   @return counter value of ~0 if no entry or expired.
 */
static void zgp_tbl_get_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zb_uint8_t *lqi_p, zb_int8_t *rssi_p)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    *lqi_p = tbl->array[tbl->cached_i].lqi;
    *rssi_p = tbl->array[tbl->cached_i].rssi;
  }
  else
  {
    *lqi_p = ZB_MAC_LQI_UNDEFINED;
    *rssi_p = ZB_MAC_RSSI_UNDEFINED;
  }
}

static void zgp_tbl_set_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zb_uint8_t lqi, zb_int8_t rssi)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    tbl->array[tbl->cached_i].lqi = lqi;
    tbl->array[tbl->cached_i].rssi = rssi;
  }
  else
  {
    /* is it ever possible?? */
    ZB_ASSERT(0);
  }
}

static zb_ret_t zgp_tbl_read(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zgp_tbl_ent_t *ent)
{
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    *ent = tbl->cached;
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_read: ok", (FMT__0));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_read: not found", (FMT__0));
    return RET_NOT_FOUND;
  }
}

static zb_ret_t zgp_tbl_del_internal(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_ret_t ret;

  ret = zgp_tbl_load(zgpd_id, tbl);

  if (ret == RET_OK)
  {
    zgp_tbl_ent_t ent;

    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_del_internal", (FMT__0));

    zgp_tbl_read(zgpd_id, tbl, &ent);
    ALIEN_STUB_TBL_ENTRY_REMOVE(&ent);

#ifdef ZB_ENABLE_ZGP_SINK
    if (tbl->cached.is_sink)
    {
      zb_zgps_unbind_aps_group_for_aliasing(&tbl->cached);
    }
#endif  /* ZB_ENABLE_ZGP_SINK */

    tbl->array[tbl->cached_i].nvram_offset = 0;
    ZB_BZERO(&tbl->cached.zgpd_id, sizeof(tbl->cached.zgpd_id));
    tbl->cached_i = (zb_uint_t)(-1);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_del_internal: not found", (FMT__0));
  }
  return ret;
}

static zb_ret_t zgp_tbl_del_all_if_endpoint(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl)
{
  zb_bool_t update = ZB_FALSE;
  zb_ret_t  ret;

  /* A.3.5.2.5 sink checks if it has a Sink Table entry for this GPD (and Endpoint, matching or 0x00 or 0xff,
   * if Appli-cationID = 0b010)
   */
  if (zgpd_id->app_id == ZB_ZGP_APP_ID_0010 &&
      (zgpd_id->endpoint == ZB_ZGP_COMMUNICATION_ENDPOINT ||
       zgpd_id->endpoint == ZB_ZGP_ALL_ENDPOINTS))
  {
    do
    {
      ret = zgp_tbl_del_internal(zgpd_id, tbl);

      if (ret == RET_OK)
      {
        update = ZB_TRUE;
      }
    } while (ret == RET_OK);
  }
  else
  {
    ret = zgp_tbl_del_internal(zgpd_id, tbl);

    if (ret == RET_OK)
    {
      update = ZB_TRUE;
    }
  }

  if (update)
  {
#ifdef ZB_USE_NVRAM
    ZB_SCHEDULE_CALLBACK(zb_zgp_write_dataset, tbl->nvram_dataset);
#endif
  }

  return (update == ZB_TRUE) ? RET_OK : RET_NOT_FOUND;

  }

/**
 * @brief Get table entry by index
 *
 * @param idx   [in]   Index of table entry which needed
 * @param tbl   [in]   Pointer to the table for search entry
 * @param ent   [out]  Pointer to allocated memory space for table entry
 *
 */
static zb_ret_t zgp_tbl_read_by_idx(zb_uint_t idx, zb_zgp_tbl_t *tbl, zgp_tbl_ent_t *ent)
{
  if (zgp_tbl_load_by_idx(idx, tbl) == RET_OK)
  {
    *ent = tbl->cached;
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_read_by_idx: ok", (FMT__0));
    return RET_OK;
  }
  else
  {
    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_read_by_idx: not found", (FMT__0));
    return RET_NOT_FOUND;
  }
}

static void zgp_tbl_check(zb_zgp_tbl_t *tbl)
{
  zb_uindex_t i;

  for (i = 0; i > tbl->tbl_size; i++)
  {
    ZB_ASSERT(tbl->array[i].nvram_offset != (zb_uint32_t)-1);
  }
}

static zb_ret_t zgp_tbl_write(zb_zgpd_id_t *zgpd_id, zb_zgp_tbl_t *tbl, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret = RET_OK;

  zgp_tbl_check(tbl);

  /* either update or create new entry */
  if (zgp_tbl_load(zgpd_id, tbl) == RET_OK)
  {
    tbl->cached = *ent;
    tbl->array[tbl->cached_i].nvram_offset = (zb_uint32_t)-1;

    /* TODO: add handle ret code */
    ALIEN_STUB_TBL_ENTRY_ADD(ent);

    TRACE_MSG(TRACE_ZGP3, "zgp_tbl_write: modified cached", (FMT__0));
  }
  else
  {
    /* Utilize open address hash */
    zb_int_t idx = zgp_tbl_addr_hash(&zgpd_id->addr, ZGPD_ID_SIZE(zgpd_id)) % tbl->tbl_size;
    zb_uint_t n = 0;

    /* seek for free slot */
    while (n < tbl->tbl_size
           && tbl->array[idx].nvram_offset != 0)
    {
      idx = (idx + 1) % tbl->tbl_size;
      n++;
    }

    if (n < tbl->tbl_size)
    {
      tbl->array[idx].security_counter = ent->security_counter;
      tbl->cached_i = idx;
      tbl->cached = *ent;
      /* Mark as used */
      tbl->array[idx].nvram_offset = (zb_uint32_t)-1;

      /* TODO: add handle ret code */
      ALIEN_STUB_TBL_ENTRY_ADD(ent);

      TRACE_MSG(TRACE_ZGP3, "zgp_tbl_write: ok, cached idx %d", (FMT__D, idx));
    }
    else
    {
      /* no free space! */
      TRACE_MSG(TRACE_ERROR, "No free space in a table!", (FMT__0));
      ret = RET_TABLE_FULL;
    }
  }

#ifdef ZB_USE_NVRAM
  if (ret == RET_OK)
  {
    ZB_SCHEDULE_CALLBACK(zb_zgp_write_dataset, tbl->nvram_dataset);
  }
#endif  /* ZB_USE_NVRAM */
  return ret;
}


#ifdef ZB_USE_NVRAM
/**
   Read ZGP sink/proxy dataset
*/
static void zb_nvram_read_zgp_tbl_dataset(zb_zgp_tbl_t *tbl, zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t ver)
{
  zgp_tbl_ent_t rec;
  zb_uint_t i, count, idx, n;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZGP3, "> zb_nvram_read_zgp_tbl_dataset %d pos %ld length %d ver %d",
            (FMT__H_L_D_D, page, pos, length, ver));

  ZVUNUSED(ver);

  ZB_BZERO(tbl->array, sizeof(tbl->array[0]) * tbl->tbl_size);
  ZB_BZERO(&tbl->cached, sizeof(tbl->cached));
  count = length / tbl->entry_size;

  TRACE_MSG(TRACE_ZGP4, "%d records", (FMT__D, count));

  for (i = 0 ; i < count && ret == RET_OK ; i++)
  {
    /* now use fixed-size entries. It is not optimal. */
    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&rec, tbl->entry_size);

    if (ret == RET_OK)
    {
      idx = zgp_tbl_addr_hash(&rec.zgpd_id, SIZE_BY_APP_ID(ZGP_TBL_GET_APP_ID(&rec))) % tbl->tbl_size;
      TRACE_MSG(TRACE_ZGP4, "page %d pos %d rx addr hash idx %d", (FMT__D_D_D, page, pos, idx));
      /* Seek for free slot */
      n = 0;
      while (n < tbl->tbl_size
             && tbl->array[idx].nvram_offset != 0)
      {
        idx = (idx + 1) % tbl->tbl_size;
        n++;
      }

      if (tbl->array[idx].nvram_offset != 0)
      {
        /* no free space */
        TRACE_MSG(TRACE_ERROR, "No free space for zp tbl load", (FMT__0));
        break;
      }
      else
      {
        tbl->array[idx].nvram_offset = pos;
        tbl->nvram_page = page;
        tbl->array[idx].security_counter = rec.security_counter;
        ZGP_PROXY_CLR_TIMEOUT(tbl->security_counter_timeouts, idx);
        TRACE_MSG(TRACE_ZGP4, "for zgp tbl ent hash idx %d nvram_offset %d nvram_page %d",
                  (FMT__D_D_D, idx, pos, tbl->nvram_page));
      }
    } /* if ok */
    pos += tbl->entry_size;
  } /* for */

  TRACE_MSG(TRACE_ZGP3, "< zb_nvram_read_zgp_tbl_dataset", (FMT__0));
}

static zb_ret_t zb_nvram_write_zgp_tbl_dataset(zb_zgp_tbl_t *tbl, zb_uint8_t page, zb_uint32_t pos)
{
  zgp_tbl_ent_t rec;
  zb_ret_t ret = RET_OK;
  zb_uint_t i;

  TRACE_MSG(TRACE_ZGP3, "> zb_nvram_write_zgp_tbl_dataset %d pos %ld",
      (FMT__D_L, page, pos));

  for (i = 0; i < tbl->tbl_size && ret == RET_OK ; ++i)
  {
    if (tbl->array[i].nvram_offset == (zb_uint32_t)-1
        && i == tbl->cached_i
        && !ZB_IEEE_ADDR_IS_ZERO(&tbl->cached.zgpd_id))
    {
      rec = tbl->cached;
    }
    else if (tbl->array[i].nvram_offset != 0)
    {
      /* Read record from nvram using address stored in installcodes hash
         Hope, record migration will not produce any problems because old page
         still exists during migration.
       */
      ZB_ASSERT(tbl->array[i].nvram_offset != (zb_uint32_t)-1);
      if (RET_OK != zb_osif_nvram_read(tbl->nvram_page, tbl->array[i].nvram_offset, (zb_uint8_t*)&rec, tbl->entry_size))
      {
        TRACE_MSG(TRACE_ERROR, "ZGP table idx %d skipped: unable to fetch entry before writing", (FMT__D, i));
        continue;
      }
    }
    else
    {
      continue;
    }
    rec.security_counter = tbl->array[i].security_counter;
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&rec, tbl->entry_size);
    tbl->array[i].nvram_offset = pos;
    TRACE_MSG(TRACE_ZGP4, "idx %d: update as nvram_page %d nvram_offset %d", (FMT__D_D_D, i, page, tbl->array[i].nvram_offset));
    pos += tbl->entry_size;
  } /* for */
  tbl->nvram_page = page;

  TRACE_MSG(TRACE_ZGP3, "< zb_nvram_write_zgp_tbl_dataset %d", (FMT__D, ret));

  return ret;
}

static void zb_nvram_update_zgp_tbl_offset(zb_zgp_tbl_t *tbl, zb_uint8_t page,
                                           zb_uint32_t dataset_pos, zb_uint32_t pos)
{
  zb_uint8_t i;

  tbl->nvram_page = page;

  for (i=0; i < tbl->tbl_size; i++)
  {
    if (tbl->array[i].nvram_offset != 0 &&
        tbl->array[i].nvram_offset != ((zb_uint32_t)-1))
    {
      TRACE_MSG(TRACE_ZGP4, "nvram_offset: was %ld, move to %ld, offset %ld",
                (FMT__L_L_L, tbl->array[i].nvram_offset,
                 ((tbl->array[i].nvram_offset - dataset_pos) + pos),
                 (tbl->array[i].nvram_offset - dataset_pos)));

      tbl->array[i].nvram_offset =
        (tbl->array[i].nvram_offset - dataset_pos) + pos;
    }
  }
}
#endif  /* ZB_USE_NVRAM */

static zb_uint8_t zb_zgp_tbl_entry_count(zb_zgp_tbl_t *tbl)
{
  zb_uint8_t n = 0;
  zb_uindex_t i;

  for (i = 0 ; i < tbl->tbl_size ; ++i)
  {
    n += (tbl->array[i].nvram_offset != 0);
  }
  return n;
}

static zb_uint16_t zb_nvram_zgp_tbl_length(zb_zgp_tbl_t *tbl)
{
  zb_uint8_t n;

  n = zb_zgp_tbl_entry_count(tbl);

  TRACE_MSG(TRACE_ZGP3, "zb_nvram_zgp_tbl_length n %d ret %d", (FMT__D_D, n, n * tbl->entry_size));
  return n * tbl->entry_size;
}

#ifdef ZB_ENABLE_ZGP_PROXY
static void zgp_proxy_entry_setup_runtime_fields(zb_zgp_proxy_tbl_ent_t *ent)
{
  TRACE_MSG(TRACE_ZGP3, ">> zgp_proxy_entry_setup_runtime_fields", (FMT__0));

  /* update runtime fields */
  if (ZGP_TBL_GET_VALID(ent))
  {
    TRACE_MSG(TRACE_ZGP4, "Set Entry Valid", (FMT__0));
    ZGP_TBL_RUNTIME_SET_VALID(ent);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP4, "Clear Entry Valid", (FMT__0));
    ZGP_TBL_RUNTIME_CLR_VALID(ent);
  }

  if (ZGP_TBL_GET_FIRST_TO_FORWARD(ent))
  {
    TRACE_MSG(TRACE_ZGP4, "Set Entry FirstToForward", (FMT__0));
    ZGP_TBL_RUNTIME_SET_FIRST_TO_FORWARD(ent);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP4, "Clear Entry FirstToForward", (FMT__0));
    ZGP_TBL_RUNTIME_CLR_FIRST_TO_FORWARD(ent);
  }

  if (ZGP_TBL_GET_HAS_ALL_UNICAST_ROUTES(ent))
  {
    TRACE_MSG(TRACE_ZGP4, "Set Entry AllUnicastRoutes", (FMT__0));
    ZGP_TBL_RUNTIME_SET_HAS_ALL_UNICAST_ROUTES(ent);
  }
  else
  {
    TRACE_MSG(TRACE_ZGP4, "Clear Entry AllUnicastRoutes", (FMT__0));
    ZGP_TBL_RUNTIME_CLR_HAS_ALL_UNICAST_ROUTES(ent);
  }

  TRACE_MSG(TRACE_ZGP3, "<< zgp_proxy_entry_setup_runtime_fields", (FMT__0));
}

zb_ret_t zgp_proxy_table_write(zb_zgpd_id_t *zgpd_id, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret;

  ent->is_sink = 0;
  ret = zgp_tbl_write(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table, ent);

  if (ret == RET_OK)
  {
    zgp_proxy_entry_setup_runtime_fields(ent);
  }
  return ret;
}

zb_ret_t zgp_proxy_table_read(zb_zgpd_id_t *zgpd_id, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret = zgp_tbl_read(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table, ent);
  ent->is_sink = 0;

  if (ret == RET_OK)
  {
    zgp_proxy_entry_setup_runtime_fields(ent);
  }
  return ret;
}

zb_ret_t zgp_proxy_table_read_by_idx(zb_uint_t idx, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret;

  ret = zgp_tbl_read_by_idx(idx, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table, ent);
  ent->is_sink = 0;

  if (ret == RET_OK)
  {
    zgp_proxy_entry_setup_runtime_fields(ent);
  }

  return ret;
}

zb_ret_t zgp_proxy_table_idx(struct zb_zgpd_id_s *zgpd_id, zb_uint_t *idx)
{
  zb_ret_t ret;
  zgp_tbl_ent_t ent;

  ret = zgp_proxy_table_read(zgpd_id, &ent);

  if (ret == RET_OK)
  {
    *idx = ZGP_CTXC().proxy_table.base.cached_i;
  }
  return ret;
}

zb_ret_t zgp_proxy_table_del(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_del_all_if_endpoint(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
}

zb_uint32_t zgp_proxy_table_get_security_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_get_security_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
}

zb_uint32_t zgp_proxy_table_get_dup_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_get_dup_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
}

zb_ret_t zgp_proxy_table_restore_security_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_restore_security_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
}

zb_ret_t zgp_proxy_table_set_security_counter(zb_zgpd_id_t *zgpd_id, zb_uint32_t counter)
{
  return zgp_tbl_set_security_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table, counter);
}

zb_uint8_t zb_zgp_proxy_table_entry_get_runtime_field(zgp_tbl_ent_t *ent, zb_uint8_t field)
{
  zb_zgpd_id_t zgpd_id;

  zgpd_id.app_id = ZGP_TBL_GET_APP_ID(ent);
  zgpd_id.addr = ent->zgpd_id;
  zgpd_id.endpoint = ent->endpoint;

  if (zgp_tbl_load(&zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table) == RET_OK)
  {
    return ((ZGP_CTXC().proxy_table.array[ZGP_CTXC().proxy_table.base.cached_i].runtime_options >> (field)) & 1);
  }
  return 0;
}

void zb_zgp_proxy_table_entry_set_runtime_field(zgp_tbl_ent_t *ent, zb_uint8_t field)
{
  zb_zgpd_id_t zgpd_id;

  zgpd_id.app_id = ZGP_TBL_GET_APP_ID(ent);
  zgpd_id.addr = ent->zgpd_id;
  zgpd_id.endpoint = ent->endpoint;

  if (zgp_tbl_load(&zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table) == RET_OK)
  {
    ZGP_CTXC().proxy_table.array[ZGP_CTXC().proxy_table.base.cached_i].runtime_options |= (1<<(field));
  }
}

void zb_zgp_proxy_table_entry_clr_runtime_field(zgp_tbl_ent_t *ent, zb_uint8_t field)
{
  zb_zgpd_id_t zgpd_id;

  zgpd_id.app_id = ZGP_TBL_GET_APP_ID(ent);
  zgpd_id.addr = ent->zgpd_id;
  zgpd_id.endpoint = ent->endpoint;

  if (zgp_tbl_load(&zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table) == RET_OK)
  {
    ZGP_CTXC().proxy_table.array[ZGP_CTXC().proxy_table.base.cached_i].runtime_options &= ~(1<<(field));
  }
}

zb_uint8_t zgp_proxy_table_get_search_counter(struct zb_zgpd_id_s *zgpd_id)
{
  return zgp_tbl_get_search_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
}

zb_ret_t zgp_proxy_table_set_search_counter(struct zb_zgpd_id_s *zgpd_id, zb_uint8_t counter)
{
  return zgp_tbl_set_search_counter(zgpd_id, (zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table, counter);
}

void zgp_proxy_table_get_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_uint8_t *lqi_p, zb_int8_t *rssi_p)
{
  zgp_tbl_get_lqi_rssi(zgpd_id, &ZGP_CTXC().proxy_table.base, lqi_p, rssi_p);
}

void zgp_proxy_table_set_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_uint8_t lqi, zb_int8_t rssi)
{
  zgp_tbl_set_lqi_rssi(zgpd_id, &ZGP_CTXC().proxy_table.base, lqi, rssi);
}

zb_bool_t zb_zgp_is_proxy_table_empty()
{
  return (zb_bool_t)(zb_zgp_tbl_entry_count((zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table) == 0);
}

zb_uint8_t zb_zgp_proxy_table_non_empty_entries_count()
{
  zb_uint8_t count;

  count = zb_zgp_tbl_entry_count((zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
  TRACE_MSG(TRACE_ZGP2, "proxy_table_non_empty_entries_count: %hd", (FMT__H, count));
  return count;
}

zb_bool_t zb_zgp_proxy_table_get_entry_by_non_empty_list_index(zb_uint8_t index, zgp_tbl_ent_t *ent)
{
  zb_uint8_t cur = 0;
  zb_uint8_t    i;
  zb_zgp_tbl_t *tbl = (zb_zgp_tbl_t *)&ZGP_CTXC().proxy_table;

  for (i = 0; i < tbl->tbl_size; i++)
  {
    if (tbl->array[i].nvram_offset != 0)
    {
      if (cur == index)
      {
        if (zgp_proxy_table_read_by_idx(i, ent) == RET_OK)
        {
          return ZB_TRUE;
        }
      }
      cur++;
    }
  }
  return ZB_FALSE;
}

zb_uint8_t zb_zgp_proxy_table_entry_get_search_counter(zgp_tbl_ent_t *ent)
{
  zb_zgpd_id_t zgpd_id;

  zgpd_id.app_id = ZGP_TBL_GET_APP_ID(ent);
  zgpd_id.addr = ent->zgpd_id;

  return zgp_proxy_table_get_search_counter(&zgpd_id);
}

zb_ret_t zb_zgp_proxy_table_entry_set_search_counter(zgp_tbl_ent_t *ent, zb_uint8_t counter)
{
  zb_zgpd_id_t zgpd_id;

  zgpd_id.app_id = ZGP_TBL_GET_APP_ID(ent);
  zgpd_id.addr = ent->zgpd_id;

  return zgp_proxy_table_set_search_counter(&zgpd_id, counter);
}

#endif  /* ZB_ENABLE_ZGP_PROXY */

#ifdef ZB_ENABLE_ZGP_SINK
zb_ret_t zgp_sink_table_write(zb_zgpd_id_t *zgpd_id, zgp_tbl_ent_t *ent)
{
  ent->is_sink = 1;
  return zgp_tbl_write(zgpd_id, &ZGP_CTXC().sink_table.base, ent);
}

zb_ret_t zgp_sink_table_read(zb_zgpd_id_t *zgpd_id, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret = zgp_tbl_read(zgpd_id, &ZGP_CTXC().sink_table.base, ent);
  ent->is_sink = 1;
  return ret;
}

zb_ret_t zgp_sink_table_read_by_idx(zb_uint_t idx, zgp_tbl_ent_t *ent)
{
  return zgp_tbl_read_by_idx(idx, &ZGP_CTXC().sink_table.base, ent);
}

zb_ret_t zgp_sink_table_idx(struct zb_zgpd_id_s *zgpd_id, zb_uint_t *idx)
{
  zb_ret_t ret;
  zgp_tbl_ent_t ent;

  ret = zgp_sink_table_read(zgpd_id, &ent);

  if (ret == RET_OK)
  {
    *idx = ZGP_CTXC().sink_table.base.cached_i;
  }
  return ret;
}

zb_ret_t zgp_sink_table_del(zb_zgpd_id_t *zgpd_id)
{
  /* delete app table entry if exist */
  zb_zgp_erase_app_table_ent_by_id(zgpd_id);

  return zgp_tbl_del_all_if_endpoint(zgpd_id, &ZGP_CTXC().sink_table.base);
}

zb_uint32_t zgp_sink_table_get_security_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_get_security_counter(zgpd_id, &ZGP_CTXC().sink_table.base);
}

zb_uint32_t zgp_sink_table_get_dup_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_get_dup_counter(zgpd_id, &ZGP_CTXC().sink_table.base);
}

zb_ret_t zgp_sink_table_restore_security_counter(zb_zgpd_id_t *zgpd_id)
{
  return zgp_tbl_restore_security_counter(zgpd_id, &ZGP_CTXC().sink_table.base);
}

zb_ret_t zgp_sink_table_set_security_counter(zb_zgpd_id_t *zgpd_id, zb_uint32_t counter)
{
  return zgp_tbl_set_security_counter(zgpd_id, &ZGP_CTXC().sink_table.base, counter);
}

void zgp_sink_get_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_uint8_t *lqi_p, zb_int8_t *rssi_p)
{
  zgp_tbl_get_lqi_rssi(zgpd_id, &ZGP_CTXC().sink_table.base, lqi_p, rssi_p);
}

void zgp_sink_set_lqi_rssi(zb_zgpd_id_t *zgpd_id, zb_uint8_t lqi, zb_int8_t rssi)
{
  zgp_tbl_set_lqi_rssi(zgpd_id, &ZGP_CTXC().sink_table.base, lqi, rssi);
}

zb_bool_t zb_zgp_is_sink_table_empty()
{
  return (zb_bool_t)(zb_zgp_tbl_entry_count((zb_zgp_tbl_t*)&ZGP_CTXC().sink_table) == 0);
}

zb_uint8_t zb_zgp_sink_table_non_empty_entries_count()
{
  zb_uint8_t count;

  count = zb_zgp_tbl_entry_count((zb_zgp_tbl_t*)&ZGP_CTXC().sink_table);
  TRACE_MSG(TRACE_ZGP2, "sink_table_non_empty_entries_count: %hd", (FMT__H, count));
  return count;
}

zb_bool_t zb_zgp_sink_table_get_entry_by_non_empty_list_index(zb_uint8_t index, zgp_tbl_ent_t *ent)
{
  zb_uint8_t cur = 0;
  zb_uindex_t i;
  zb_zgp_tbl_t *tbl = (zb_zgp_tbl_t *)&ZGP_CTXC().sink_table;

  for (i = 0; i < tbl->tbl_size; i++)
  {
    if (tbl->array[i].nvram_offset != 0)
    {
      if (cur == index)
      {
        if (zgp_sink_table_read_by_idx(i, ent) == RET_OK)
        {
          return ZB_TRUE;
        }
      }
      cur++;
    }
  }
  return ZB_FALSE;
}
#endif  /* ZB_ENABLE_ZGP_SINK */

zb_ret_t zgp_any_table_read(zb_zgpd_id_t *zgpd_id, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret = RET_NOT_FOUND;

  ZVUNUSED(zgpd_id);
  ZVUNUSED(ent);

#ifdef ZB_ENABLE_ZGP_SINK
  ret = zgp_sink_table_read(zgpd_id, ent);
#endif  /* ZB_ENABLE_ZGP_SINK */
#ifdef ZB_ENABLE_ZGP_PROXY
  if (ret != RET_OK)
  {
    ret = zgp_proxy_table_read(zgpd_id, ent);
  }
#endif  /* ZB_ENABLE_ZGP_PROXY */
  return ret;
}

zb_ret_t zgp_proxy_table_enumerate(zb_zgp_ent_enumerate_ctx_t *ctx, zgp_tbl_ent_t *ent)
{
  zb_ret_t ret = RET_NOT_FOUND;

  ZVUNUSED(ctx);
  ZVUNUSED(ent);

#ifdef ZB_ENABLE_ZGP_PROXY
  if (ctx->idx == ZB_ZGP_ENT_ENUMERATE_CTX_START_IDX)
  {
    ctx->entries_count = zb_zgp_proxy_table_non_empty_entries_count();
    ctx->idx = 0;
  }

  if (ctx->idx < ctx->entries_count)
    {
    if (zb_zgp_proxy_table_get_entry_by_non_empty_list_index(ctx->idx, ent) == ZB_TRUE)
    {
      ctx->idx++;
      ret = RET_OK;
    }
  }
#endif  /* ZB_ENABLE_ZGP_PROXY */

  return ret;
}

zb_ret_t zgp_sink_table_enumerate(zb_zgp_ent_enumerate_ctx_t *ctx, zgp_tbl_ent_t *ent)
    {
  zb_ret_t ret = RET_NOT_FOUND;

  ZVUNUSED(ctx);
  ZVUNUSED(ent);

#ifdef ZB_ENABLE_ZGP_SINK
  if (ctx->idx == ZB_ZGP_ENT_ENUMERATE_CTX_START_IDX)
      {
    ctx->entries_count = zb_zgp_sink_table_non_empty_entries_count();
    ctx->idx = 0;
      }

  if (ctx->idx < ctx->entries_count)
        {
    if (zb_zgp_sink_table_get_entry_by_non_empty_list_index(ctx->idx, ent) == ZB_TRUE)
    {
      ctx->idx++;
      ret = RET_OK;
        }
      }
#endif  /* ZB_ENABLE_ZGP_SINK */

  return ret;
    }

#ifdef ZB_USE_NVRAM
zb_uint16_t zb_nvram_zgp_sink_table_length()
{
#ifdef ZB_ENABLE_ZGP_SINK
  return zb_nvram_zgp_tbl_length(&ZGP_CTXC().sink_table.base);
#else
  return 0;
#endif
}

zb_uint16_t zb_nvram_zgp_proxy_table_length()
  {
#ifdef ZB_ENABLE_ZGP_PROXY
  return zb_nvram_zgp_tbl_length((zb_zgp_tbl_t*)&ZGP_CTXC().proxy_table);
#else
  return 0;
#endif
  }

zb_uint16_t zb_nvram_zgp_cluster_length()
  {
#ifdef ZB_ENABLE_ZGP_CLUSTER
  return sizeof(zb_zgp_cluster_t);
#else
  return 0;
#endif
}

void zb_nvram_update_zgp_sink_tbl_offset(zb_uint8_t page, zb_uint32_t dataset_pos, zb_uint32_t pos)
{
  ZVUNUSED(page);
  ZVUNUSED(dataset_pos);
  ZVUNUSED(pos);
#ifdef ZB_ENABLE_ZGP_SINK
  {
    zb_zgp_tbl_t *tbl = &ZGP_CTXC().sink_table.base;

    zb_nvram_update_zgp_tbl_offset(tbl, page, dataset_pos, pos);
  }
#endif  /* ZB_ENABLE_ZGP_SINK */
}

void zb_nvram_update_zgp_proxy_tbl_offset(zb_uint8_t page, zb_uint32_t dataset_pos, zb_uint32_t pos)
{
  ZVUNUSED(page);
  ZVUNUSED(dataset_pos);
  ZVUNUSED(pos);
#ifdef ZB_ENABLE_ZGP_PROXY
  {
    zb_zgp_tbl_t *tbl = &ZGP_CTXC().proxy_table.base;

    zb_nvram_update_zgp_tbl_offset(tbl, page, dataset_pos, pos);
  }
#endif  /* ZB_ENABLE_ZGP_PROXY */
}

void zb_nvram_read_zgp_sink_table_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  ZVUNUSED(page);
  ZVUNUSED(pos);
  ZVUNUSED(length);
  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);
#ifdef ZB_ENABLE_ZGP_SINK
  zb_nvram_read_zgp_tbl_dataset(&ZGP_CTXC().sink_table.base, page, pos, length, nvram_ver);
#endif  /* ZB_ENABLE_ZGP_SINK */
  }

zb_ret_t zb_nvram_write_zgp_sink_table_dataset(zb_uint8_t page, zb_uint32_t pos)
  {
  zb_ret_t ret = RET_OK;

  ZVUNUSED(page);
  ZVUNUSED(pos);
#ifdef ZB_ENABLE_ZGP_SINK
  ret = zb_nvram_write_zgp_tbl_dataset(&ZGP_CTXC().sink_table.base, page, pos);
#endif  /* ZB_ENABLE_ZGP_SINK */
  return ret;
  }

void zb_nvram_read_zgp_proxy_table_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  ZVUNUSED(page);
  ZVUNUSED(pos);
  ZVUNUSED(length);
  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);
#ifdef ZB_ENABLE_ZGP_PROXY
  zb_nvram_read_zgp_tbl_dataset(&ZGP_CTXC().proxy_table.base, page, pos, length, nvram_ver);
#endif  /* ZB_ENABLE_ZGP_PROXY */
}

zb_ret_t zb_nvram_write_zgp_proxy_table_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret = RET_OK;

  ZVUNUSED(page);
  ZVUNUSED(pos);
#ifdef ZB_ENABLE_ZGP_PROXY
  ret = zb_nvram_write_zgp_tbl_dataset(&ZGP_CTXC().proxy_table.base, page, pos);
#endif  /* ZB_ENABLE_ZGP_PROXY */
  return ret;
}

void zb_nvram_read_zgp_cluster_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  TRACE_MSG(TRACE_ZGP1, "> zb_nvram_read_zgp_cluster_dataset %d pos %ld length %d nvram_ver %d",
            (FMT__H_L_D_D, page, pos, length, nvram_ver));

  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  if (length == sizeof(zb_zgp_cluster_t))
  {
    zb_ret_t ret;

    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&ZGP_CTXC().cluster, sizeof(zb_zgp_cluster_t));

    if (ret != RET_OK)
    {
      ZB_ASSERT(0);
    }
  }
  else
  {
    ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_ZGP1, "< zb_nvram_read_zgp_cluster_dataset", (FMT__0));
}

zb_ret_t zb_nvram_write_zgp_cluster_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZGP1, "> zb_nvram_write_zgp_cluster_dataset %d pos %ld",
      (FMT__H_L, page, pos));

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ZGP_CTXC().cluster, sizeof(zb_zgp_cluster_t));

  TRACE_MSG(TRACE_ZGP1, "< zb_nvram_write_zgp_cluster_dataset %hd", (FMT__H, ret));

  return ret;
}
#endif  /* ZB_USE_NVRAM */

#endif  /* #ifdef ZB_ENABLE_ZGP */
