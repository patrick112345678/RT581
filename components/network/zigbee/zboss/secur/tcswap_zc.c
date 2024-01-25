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
/* PURPOSE: TC swapout - TC side
*/


/*! \addtogroup ZB_SECUR */
/*! @{ */

#define ZB_TRACE_FILE_ID 4
#include "zb_common.h"
#include "zb_aps.h"
#include "zb_secur.h"
#include "zboss_tcswap.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif

#if defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE

/* TODO:

   TC Swapout Save/load API now supposes that whole DB must fit into single RAM chunk.
   That is enough for testing, but not production.
   TODO: change API to either callbacks or interface similar to Unix write(): xxx(buf, len)
 */

/*
TODO: implement save/load of IC.
That is also ZC-only feature.
 */

static void send_backup_required_sig(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed_ext(zdo_send_signal_no_args, ZB_TCSWAP_DB_BACKUP_REQUIRED_SIGNAL, 0);
}

static void zb_tcsw_init_first_state(void)
{
  SEC_CTX().tcswap.state = TCSW_STATE_JUST_STARTED;
  SEC_CTX().tcswap.size = 0;
}

static void zb_tcsw_set_next_state(void)
{
  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_set_next_state, current %hd", (FMT__H, SEC_CTX().tcswap.state));

  switch (SEC_CTX().tcswap.state) /* current state */
  {
    case TCSW_STATE_JUST_STARTED:
      SEC_CTX().tcswap.state = TCSW_STATE_GLOBAL_SECTION;
      SEC_CTX().tcswap.size  = sizeof(SEC_CTX().tcswap.load_u.global);
    break;

    case TCSW_STATE_GLOBAL_SECTION:
    case TCSW_STATE_DEVICE_INFO:
      if (SEC_CTX().tcswap.device_count)
      {
        SEC_CTX().tcswap.state = TCSW_STATE_DEVICE_INFO;
        SEC_CTX().tcswap.size  = sizeof(SEC_CTX().tcswap.load_u.device);
      }
      else
      {
        /* todo: bindings */
        SEC_CTX().tcswap.state = TCSW_STATE_DONE;
        SEC_CTX().tcswap.size  = 0;
      }
    break;

    default:
      SEC_CTX().tcswap.state = TCSW_STATE_DONE;
      SEC_CTX().tcswap.size  = 0;
    break;
  }

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_set_next_state, next %hd", (FMT__H, SEC_CTX().tcswap.state));
}

static void zb_tcsw_fill_global_part(zb_tcsw_global_t *global)
{
  zb_uint_t i;
  zb_aps_device_key_pair_set_t *aps_key;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_fill_global_part", (FMT__0));

  ZB_EXTPANID_COPY(global->tc_panid, ZB_NIB_EXT_PAN_ID());
  ZB_IEEE_ADDR_COPY(global->tc_addr, ZB_PIB_EXTENDED_ADDRESS());
  global->db_version = 0;

  /* todo: fix this */
  global->flags = 0;

  global->device_count = 0;
  for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; i++)
  {
    if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != 0)
    {
      aps_key = zb_aps_keypair_get_ent_by_idx(i);
      if (aps_key != NULL && aps_key->key_attributes == ZB_SECUR_VERIFIED_KEY)
      {
        global->device_count += 1;
      }
    }
  }

  /* todo: global->db_mic; */

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_fill_global_part, dev_cnt %hd", (FMT__H, global->device_count));
}

static void zb_tcsw_fill_device(zb_tcsw_device_t *dev, zb_uint_t *idx)
{
  zb_aps_device_key_pair_set_t *aps_key = NULL;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_fill_device, start idx %hd", (FMT__H, *idx));

  while (*idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
  {
    if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[*idx].nvram_offset != 0)
    {
      aps_key = zb_aps_keypair_get_ent_by_idx(*idx);
      if (aps_key != NULL && aps_key->key_attributes == ZB_SECUR_VERIFIED_KEY)
      {
        break;
      }
    }
    *idx += 1;
  }

  TRACE_MSG(TRACE_SECUR4, "aps_key %p idx %u", (FMT__P_D, aps_key, *idx));

  if (*idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
  {
    ZB_ASSERT(aps_key != NULL);
    dev->flags = 0;
    ZB_IEEE_ADDR_COPY(dev->addr, aps_key->device_address);
    dev->key_attr = aps_key->key_attributes;
    (void)zb_sec_b6_hash(aps_key->link_key, ZB_CCM_KEY_SIZE, dev->hashed_key);
    dev->flags |= TCSWAP_HASHED_KEY;
    TRACE_MSG(TRACE_SECUR2, "device_ieee = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dev->addr)));
    *idx += 1;
  }

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_fill_device, idx %u", (FMT__D, *idx));
}

static void zb_tcsw_backup_db(zb_uint8_t *buf)
{
  zb_uint_t offset = 0;
  zb_bool_t need_save;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_backup_db", (FMT__0));

  while (SEC_CTX().tcswap.state != TCSW_STATE_DONE)
  {
    need_save = ZB_TRUE;

    switch (SEC_CTX().tcswap.state)
    {
      case TCSW_STATE_GLOBAL_SECTION:
        zb_tcsw_fill_global_part(&SEC_CTX().tcswap.load_u.global);
        SEC_CTX().tcswap.device_count = SEC_CTX().tcswap.load_u.global.device_count;
        SEC_CTX().tcswap.dev_idx = 0;
      break;

      case TCSW_STATE_DEVICE_INFO:
        zb_tcsw_fill_device(&SEC_CTX().tcswap.load_u.device, &SEC_CTX().tcswap.dev_idx);
        SEC_CTX().tcswap.device_count -= 1;
      break;

      /* todo: TCSW_STATE_BINDINGS */

      default:
        need_save = ZB_FALSE;
      break;
    }

    if (need_save)
    {
      ZB_MEMCPY(buf + offset, SEC_CTX().tcswap.load_u.buf, SEC_CTX().tcswap.size);
      offset += SEC_CTX().tcswap.size;
    }

    zb_tcsw_set_next_state();
  }

  /* Backup complete */
  SEC_CTX().tcswap.flags.dirty_bitmask = 0;

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_backup_db", (FMT__0));
}

static zb_ret_t zb_tcsw_restore_db(zb_uint8_t *buf, zb_uint_t len)
{
  zb_ret_t  ret = RET_OK;
  zb_uint_t offset = 0;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_restore_db", (FMT__0));

  while (ret == RET_OK && SEC_CTX().tcswap.state != TCSW_STATE_DONE)
  {
    /* Let's load next part */
    if (SEC_CTX().tcswap.size > 0)
    {
      if (offset + SEC_CTX().tcswap.size > len)
      {
        /* buf is too small */
        ret = RET_ERROR;
      }

      if (ret == RET_OK)
      {
        ZB_MEMCPY(SEC_CTX().tcswap.load_u.buf, buf + offset, SEC_CTX().tcswap.size);
        offset += SEC_CTX().tcswap.size;
      }
    }

    if (ret == RET_OK)
    {
      switch (SEC_CTX().tcswap.state)
      {
        case TCSW_STATE_GLOBAL_SECTION:
          /* save important global info */
          SEC_CTX().tcswap.device_count = SEC_CTX().tcswap.load_u.global.device_count;
          SEC_CTX().tcswap.flags.have_bindings = (zb_bool_t)(SEC_CTX().tcswap.load_u.global.flags & ZB_TCSW_DB_TCINFO_HAS_BINDING);
          ZB_IEEE_ADDR_COPY(SEC_CTX().tcswap.tc_addr, SEC_CTX().tcswap.load_u.global.tc_addr);

          /* todo: we need do it now or when we are returning RET_OK? */
          /* set aps_use_extended_pan_id so zdo_start_formation() will use it. */
          zb_set_use_extended_pan_id(SEC_CTX().tcswap.load_u.global.tc_panid);
        break;

        case TCSW_STATE_DEVICE_INFO:
        {
          /* Simplify the parser - use fixed-size records */
          zb_tcsw_device_t *dev = &SEC_CTX().tcswap.load_u.device;

          TRACE_MSG(TRACE_SECUR2, "device_ieee " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dev->addr)));
          TRACE_MSG(TRACE_SECUR2, "device_flags %hd tcswap.device_count %hu", (FMT__H_H, dev->flags, SEC_CTX().tcswap.device_count));

          if (dev->flags & TCSWAP_INSTALLCODE)
          {
            zb_secur_ic_add(dev->addr, dev->ic_type, dev->ic, NULL);
          }

          if (dev->flags & TCSWAP_HASHED_KEY)
          {
            zb_aps_device_key_pair_set_t *aps_key;

            TRACE_MSG(TRACE_SECUR3, "Update key " TRACE_FORMAT_128 " key_type ZB_SECUR_UNIQUE_KEY key_attr ZB_SECUR_PROVISIONAL_KEY",
                      (FMT__B, TRACE_ARG_128(dev->hashed_key)));
            aps_key = zb_secur_update_key_pair(dev->addr, dev->hashed_key, ZB_SECUR_UNIQUE_KEY,
                                               ZB_SECUR_PROVISIONAL_KEY, ZB_SECUR_KEY_SRC_UNKNOWN);
            ZVUNUSED(aps_key);
            /* TODO: update key flags? Not sure need it. */
          }
          /* TODO: handle prev key (what to do with it?), passphrase (r23) */

          SEC_CTX().tcswap.device_count -= 1;
        }
        break;

        /* todo: TCSW_LOAD_BINDINGS */
      }

      zb_tcsw_set_next_state();
    }
  } /* while */

  if (ret != RET_OK)
  {
    SEC_CTX().tcswap.state = TCSW_STATE_DONE;
  }

  ZB_SECUR_TRACE_ALL_KEY_PAIRS();
  
  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_restore_db, ret %hd", (FMT__H, ret));

  return ret;
}

void zb_tcsw_key_added(void)
{
  SEC_CTX().tcswap.flags.dirty_bitmask |= ZB_TCSWAP_SECUR;

  ZB_SCHEDULE_ALARM_CANCEL(send_backup_required_sig, 0);
  ZB_SCHEDULE_CALLBACK(send_backup_required_sig, 0);
}

void zb_tcsw_binding_added(void)
{
  SEC_CTX().tcswap.flags.dirty_bitmask |= ZB_TCSWAP_BINDINGS;

  ZB_SCHEDULE_ALARM_CANCEL(send_backup_required_sig, 0);
  ZB_SCHEDULE_ALARM(send_backup_required_sig, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
}

zb_uint_t zb_tcsw_need_backup(void)
{
  return (zb_uint_t)SEC_CTX().tcswap.flags.dirty_bitmask;
}

zb_bool_t zb_tcsw_is_busy(void)
{
  return (SEC_CTX().tcswap.state != TCSW_STATE_DONE);
}

zb_uint_t zb_tcsw_calculate_db_size(void)
{
  zb_uint_t   i = 0;
  zb_uint8_t num = 0;

  for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; i++)
  {
    if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != 0)
    {
      num += 1;
    }
  }

  return sizeof(SEC_CTX().tcswap.load_u.global) + num * sizeof(SEC_CTX().tcswap.load_u.device);
}

zb_bool_t zb_tcsw_check_buffer_size(zb_uint_t size)
{
  return (size >= zb_tcsw_calculate_db_size());
}

zb_ret_t zb_tcsw_start_backup_db(zb_uint8_t *backup_buf, zb_uint_t buf_size)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_start_backup_db", (FMT__0));

  if (!zb_tcsw_is_busy()
       && zb_tcsw_check_buffer_size(buf_size)
       /* && zb_tcsw_need_backup() ? */
     )
  {
    zb_tcsw_init_first_state();
    zb_tcsw_backup_db(backup_buf);
    ret = RET_OK;
  }

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_start_backup_db, ret %hd", (FMT__H, ret));

  return ret;
}

zb_ret_t zb_tcsw_start_restore_db(zb_uint8_t *restore_buf, zb_uint_t buf_size, zb_bool_t change_tc_addr)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_SECUR1, ">> zb_tcsw_start_restore_db", (FMT__0));

  zb_tcsw_init_first_state();
  ret = zb_tcsw_restore_db(restore_buf, buf_size);

  if (ret == RET_OK)
  {
    ZB_TCPOL().tc_swapped = ZB_TRUE;
    if (change_tc_addr)
    {
      ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), SEC_CTX().tcswap.tc_addr);
    }
  }

  TRACE_MSG(TRACE_SECUR1, "<< zb_tcsw_start_restore_db, ret %hd", (FMT__H, ret));

  return ret;
}

#endif  /* defined TC_SWAPOUT && defined ZB_COORDINATOR_ROLE */

