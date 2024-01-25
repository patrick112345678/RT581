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
/* PURPOSE: IB save/load/set defaults
*/

#define ZB_TRACE_FILE_ID 122
#include "zboss_api_core.h"

#include "zb_common.h"
#if defined ZB_USE_NVRAM

#include "zb_nvram.h"
#include "zb_error_indication.h"

#ifdef ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION
#include "zb_mac_globals.h"
#endif /* ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION */

#include "zb_zdo.h"
#include "zb_bdb_internal.h"
#include "zb_nwk_ed_aging.h"

#ifdef APP_ONLY_NVRAM
#define ZB_NO_NVRAM_VER_MIGRATION
#endif

#ifdef ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION
#define ZB_NVRAM_TRANSACTION_RADIO_OFF() zb_nvram_transaction_radio_off()
#define ZB_NVRAM_TRANSACTION_RADIO_ON() zb_nvram_transaction_radio_on()
#else
#define ZB_NVRAM_TRANSACTION_RADIO_OFF()
#define ZB_NVRAM_TRANSACTION_RADIO_ON()
#endif /* ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION */

static zb_bool_t zb_nvram_dataset_is_supported(zb_nvram_dataset_types_t ds);
#ifdef ZB_NVRAM_ENABLE_DIRECT_API
static void nvram_update_dataset_position(zb_nvram_dataset_types_t type,
                                          zb_uint8_t page,
                                          zb_uint32_t pos);
#endif /* ZB_NVRAM_ENABLE_DIRECT_API */

typedef struct zb_nvram_load_data_s
{
  zb_nvram_position_t datasets[ZB_NVRAM_DATASET_NUMBER];
  zb_nvram_tl_t time_label;
  zb_uint32_t pos;
  zb_uint8_t page;
} zb_nvram_load_data_t;

static void zb_nvram_erase_if_not_erased(zb_uint8_t page);
static void zb_nvram_read_app_data(zb_uint8_t i, zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);

static void nvram_restore_datasets(zb_nvram_position_t *datasets);
static zb_ret_t nvram_read_pages_headers(void);
static zb_ret_t find_datasets_on_page(zb_uint8_t page, zb_nvram_load_data_t *load_data);
static zb_bool_t nvram_can_load_dataset(zb_uint8_t ds_type,
                                        zb_nvram_ver_t nvram_ver);
static void nvram_handle_datasets_pages(zb_nvram_load_data_t *load_data);
#ifndef ZB_NO_NVRAM_VER_MIGRATION
static zb_bool_t nvram_can_load_dataset_from_nvram_ver_6(zb_uint8_t ds_type);
static void migrate_curr_page_to_new_version(void);
static void nvram_restore_obsolete_datasets(zb_nvram_position_t *datasets);
static zb_uint32_t nvram_migrate_obsolete_datasets(zb_nvram_position_t *datasets,
                                                   zb_uint8_t next_page,
                                                   zb_uint32_t pos);
static zb_ret_t nvram_migrate_aps_secure_dataset(zb_nvram_migration_info_t *migration_info,
                                                 zb_nvram_dataset_hdr_t *hdr);
#endif  /* #ifndef ZB_NO_NVRAM_VER_MIGRATIO */

#ifndef APP_ONLY_NVRAM

#ifdef ZB_SET_MAC_ADDRESS
void zb_set_default_mac_addr(zb_char_t *rx_pipe);
#endif

void zb_nvram_read_aps_keypair_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);
zb_ret_t zb_nvram_write_aps_keypair_dataset(zb_uint8_t page, zb_uint32_t pos);
zb_uint16_t zb_nvram_aps_keypair_length(void);

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
void zb_nvram_read_installcodes_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);
zb_ret_t zb_nvram_write_installcodes_dataset(zb_uint8_t page, zb_uint32_t pos);
zb_uint16_t zb_nvram_installcodes_length(void);
#endif

zb_ret_t zb_nvram_write_aps_binding_dataset(zb_uint8_t page, zb_uint32_t pos);

void zb_nvram_read_aps_binding_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);

static zb_uint16_t zb_nvram_aps_binding_tables_length(void);

zb_ret_t zb_nvram_write_aps_groups_dataset(zb_uint8_t page, zb_uint32_t pos);

void zb_nvram_read_aps_groups_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);

zb_uint16_t zb_nvram_aps_groups_length(void);

#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER && !defined ZB_ED_FUNC
#undef ZB_HA_ENABLE_POLL_CONTROL_SERVER
#endif

#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN
zb_uint16_t zb_nvram_wwah_dataset_size(void);

void zb_nvram_read_wwah_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);

zb_ret_t zb_nvram_write_wwah_dataset(zb_uint8_t page, zb_uint32_t pos);
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN */

#if defined ZB_STORE_COUNTERS
void zb_nvram_read_counters_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);

zb_ret_t zb_nvram_write_counters_dataset(zb_uint8_t page, zb_uint32_t pos);
#endif  /* defined ZB_STORE_COUNTERS */

static zb_ret_t zb_nvram_read_addr_map_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);
static zb_ret_t zb_nvram_write_addr_map_dataset(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t zb_nvram_addr_map_dataset_length(void);

static zb_ret_t zb_nvram_read_neighbour_tbl_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver);
static zb_ret_t zb_nvram_write_neighbour_tbl_dataset(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t zb_nvram_neighbour_tbl_dataset_length(void);

#endif /* APP_ONLY_NVRAM */


#ifdef APP_ONLY_NVRAM
/* Define nvram context here, so non-zb application can use it without linking to ZB libraries */

static zb_nvram_globals_t gs_nvram;
#ifdef ZB_NVRAM
#undef ZB_NVRAM
#endif
#define ZB_NVRAM() (gs_nvram)

#undef ZB_STORE_NEIGHBOR_TBL
#undef ZB_STORE_ADDR_MAP

#ifndef ZB_NO_NVRAM_VER_MIGRATION
#define ZB_NO_NVRAM_VER_MIGRATION
#endif
#endif /* APP_ONLY_NVRAM */

#endif /* USE_NVRAM */
/*! \addtogroup ZB_BASE */
/*! @{ */

#ifndef APP_ONLY_NVRAM
/* Note: still need that functions to compile build without nvram support. it
 * will not work correct, but can compile. */

zb_ret_t zb_aps_keypair_load_by_idx(zb_uint_t idx)
{
  zb_ret_t ret = RET_NOT_FOUND;

  if (idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
  {
    if (ZB_AIB().aps_device_key_pair_storage.cached_i == idx)
    {
      TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_load_by_idx: already cached", (FMT__0));
      ret = RET_OK;
    }
    else
    {
      if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset != 0U)
      {
        zb_aps_device_key_pair_nvram_t rec;
#ifdef ZB_USE_NVRAM
        if (zb_osif_nvram_read(ZB_AIB().aps_device_key_pair_storage.nvram_page,
                               ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset,
                               (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(zb_aps_device_key_pair_nvram_t)) == RET_OK)
#else
          /* Just to be able to compile at platform without nvram */
        ZB_BZERO(&rec, sizeof(rec));
#endif
        {
          ZB_AIB().aps_device_key_pair_storage.cached_i = (zb_uint8_t)idx;
          ZB_MEMCPY(&ZB_AIB().aps_device_key_pair_storage.cached, &rec, sizeof(rec));

          TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_load_by_idx: cached record %d", (FMT__D, idx));
          ret = RET_OK;
        }
      }
    }
  }

  return ret;
}


zb_ret_t zb_aps_keypair_read_by_idx(zb_uint_t idx, zb_aps_device_key_pair_nvram_t *ent)
{
  if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
  {
    ZB_MEMCPY(ent, &ZB_AIB().aps_device_key_pair_storage.cached, sizeof(*ent));
    TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_read_by_idx %d: ok", (FMT__D, idx));
    return RET_OK;
  }
  else
  {
    return RET_NOT_FOUND;
  }
}

zb_aps_device_key_pair_set_t* zb_aps_keypair_get_ent_by_idx(zb_uint_t idx)
{
  if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
  {
    return &ZB_AIB().aps_device_key_pair_storage.cached;
  }
  return NULL;
}

void zb_aps_keypair_get_addr_by_idx(zb_uint_t idx, zb_uint8_t *dev_addr)
{
  if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
  {
    ZB_IEEE_ADDR_COPY(dev_addr, ZB_AIB().aps_device_key_pair_storage.cached.device_address);
  }
  else
  {
    ZB_IEEE_ADDR_ZERO(dev_addr);
  }
}


zb_ret_t zb_aps_keypair_write(zb_aps_device_key_pair_set_t *ent, zb_uint32_t idx)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_write", (FMT__0));

  /* either update or create new entry */
  /* If correct idx is provided, then try to load record by idx */
  if (idx != (zb_uint16_t)-1)
  {
    if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
    {
      ZB_AIB().aps_device_key_pair_storage.cached_i = (zb_uint8_t)idx;
      ZB_MEMCPY(&ZB_AIB().aps_device_key_pair_storage.cached, ent, sizeof(*ent));
      /* Mark as used */
      ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset = ZB_APS_DEVICE_KEY_PAIR_CACHED;
      TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_write: update cached record", (FMT__0));
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "idx provided, but record not found", (FMT__0));
      ZB_ASSERT(0);
    }
  }
  else
  {
    zb_uint_t n = 0;

    /* Utilize open address hash */
    idx = zb_64bit_hash(ent->device_address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;

    /* seek for free slot */
    while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE
           && ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset != 0U)
    {
      idx = (idx + 1U) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
      n++;
    }
    if (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
      /* stor->array[idx].security_counter = ent->security_counter; */
      ZB_AIB().aps_device_key_pair_storage.cached_i = (zb_uint8_t)idx;
      ZB_MEMCPY(&ZB_AIB().aps_device_key_pair_storage.cached, ent, sizeof(*ent));
      /* Mark as used */
      ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset = ZB_APS_DEVICE_KEY_PAIR_CACHED;
      TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_write: ok, cached idx %d", (FMT__D, idx));
    }
    else
    {
      /* no free space! */
      TRACE_MSG(TRACE_ERROR, "No free space in a table!", (FMT__0));
      ret = RET_ERROR;
    }
  }
#ifdef ZB_USE_NVRAM
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_APS_SECURE_DATA);
#endif
  return ret;
}

#endif  /* #ifdef APP_ONLY_NVRAM */

#if defined ZB_USE_NVRAM || defined doxygen

static zb_uint16_t zb_nvram_get_length_dataset(const zb_nvram_dataset_hdr_t *hdr)
{
  zb_size_t len = 0;
  zb_ret_t ret;

  switch(hdr->data_set_type)
  {
#ifndef APP_ONLY_NVRAM
  case ZB_NVRAM_COMMON_DATA:
    len = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_common_t);
    break;
#if defined ZB_ENABLE_HA || defined DOXYGEN
  case ZB_NVRAM_HA_DATA:
    len = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_ha_t);
    break;
#endif /*defined ZB_ENABLE_HA*/
#ifdef ZB_ENABLE_ZGP
  case ZB_NVRAM_DATASET_GP_SINKT:
    len = SIZE_DS_HEADER + zb_nvram_zgp_sink_table_length();
    break;
  case ZB_NVRAM_DATASET_GP_APP_TBL:
    len = SIZE_DS_HEADER + zb_zgp_nvram_app_tbl_length();
    break;
  case ZB_NVRAM_DATASET_GP_PRPOXYT:
    len = SIZE_DS_HEADER + zb_nvram_zgp_proxy_table_length();
    break;
  case ZB_NVRAM_DATASET_GP_CLUSTER:
    len = SIZE_DS_HEADER + zb_nvram_zgp_cluster_length();
    break;
#endif /* ZB_ENABLE_ZGP */
#if (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) || defined(DOXYGEN)
  case ZB_NVRAM_ZCL_REPORTING_DATA:
    len = hdr->data_len ? hdr->data_len : SIZE_DS_HEADER + zb_nvram_zcl_reporting_dataset_length();
    break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */
  case ZB_NVRAM_APS_SECURE_DATA:
    len = (hdr->data_len != 0U) ? hdr->data_len : SIZE_DS_HEADER + zb_nvram_aps_keypair_length();
    break;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
  case ZB_NVRAM_INSTALLCODES:
    len = hdr->data_len ? hdr->data_len : SIZE_DS_HEADER + zb_nvram_installcodes_length();
    break;
#endif
  case ZB_NVRAM_APS_BINDING_DATA:
    len = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_binding_v2_t) + zb_nvram_aps_binding_tables_length();
    break;
  case ZB_NVRAM_APS_GROUPS_DATA:
    len = SIZE_DS_HEADER + zb_nvram_aps_groups_length();
    break;
#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN
  case ZB_NVRAM_HA_POLL_CONTROL_DATA:
    len = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_poll_control_t);
    break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN
  case ZB_NVRAM_ZCL_WWAH_DATA:
    len = SIZE_DS_HEADER + zb_nvram_wwah_dataset_size();
    break;
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN */
#if defined ZB_STORE_COUNTERS || defined DOXYGEN
  case ZB_IB_COUNTERS:
    len = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_counters_t);
    break;
#endif
#endif  /* #ifndef APP_ONLY_NVRAM */
  case ZB_NVRAM_APP_DATA1:
    if(ZB_NVRAM().get_app_data_size_cb[0] != NULL)
    {
      len = SIZE_DS_HEADER + ZB_NVRAM().get_app_data_size_cb[0]();
    }
    break;
  case ZB_NVRAM_APP_DATA2:
    if(ZB_NVRAM().get_app_data_size_cb[1] != NULL)
    {
      len = SIZE_DS_HEADER + ZB_NVRAM().get_app_data_size_cb[1]();
    }
    break;
  case ZB_NVRAM_APP_DATA3:
    if(ZB_NVRAM().get_app_data_size_cb[2] != NULL)
    {
      len = SIZE_DS_HEADER + ZB_NVRAM().get_app_data_size_cb[2]();
    }
    break;
  case ZB_NVRAM_APP_DATA4:
    if(ZB_NVRAM().get_app_data_size_cb[3] != NULL)
    {
      len = SIZE_DS_HEADER + ZB_NVRAM().get_app_data_size_cb[3]();
    }
    break;

#ifndef APP_ONLY_NVRAM

  case ZB_NVRAM_ADDR_MAP:
    len = SIZE_DS_HEADER + zb_nvram_addr_map_dataset_length();
    break;
  case ZB_NVRAM_NEIGHBOUR_TBL:
    len = SIZE_DS_HEADER + zb_nvram_neighbour_tbl_dataset_length();
    break;

#ifdef ZDO_DIAGNOSTICS
  case ZB_NVRAM_ZDO_DIAGNOSTICS_DATA:
    len = SIZE_DS_HEADER + zb_nvram_diagnostics_dataset_length();
    break;
#endif /* ZDO_DIAGNOSTICS */

#endif  /* #ifndef APP_ONLY_NVRAM */
  default:
    ret = zb_nvram_custom_ds_try_get_length(hdr->data_set_type, &len);
    if (ret != RET_OK)
    {
      len = 0;
    }
    len += SIZE_DS_HEADER;
    break;
  }
  len += sizeof(zb_nvram_dataset_tail_t);
  return (zb_uint16_t)len;
}


static zb_uint16_t zb_nvram_get_dataset_version(zb_uint16_t ds_type)
{
  zb_uint16_t ds_version = ZB_NVRAM_DATA_SET_VERSION_NOT_AVAILABLE;

  switch(ds_type)
  {
#ifndef APP_ONLY_NVRAM
  case ZB_NVRAM_COMMON_DATA:
    ds_version = ZB_NVRAM_COMMON_DATA_DS_VER;
    break;
#if defined ZB_ENABLE_HA || defined DOXYGEN
  case ZB_NVRAM_HA_DATA:
    ds_version = ZB_NVRAM_HA_DATA_DS_VER;
    break;
#endif /*defined ZB_ENABLE_HA*/
#ifdef ZB_ENABLE_ZGP
  case ZB_NVRAM_DATASET_GP_SINKT:
    ds_version = ZB_NVRAM_DATASET_GP_SINKT_DS_VER;
    break;
  case ZB_NVRAM_DATASET_GP_APP_TBL:
    ds_version = ZB_NVRAM_DATASET_GP_APP_TBL_DS_VER;
    break;
  case ZB_NVRAM_DATASET_GP_PRPOXYT:
    ds_version = ZB_NVRAM_DATASET_GP_PRPOXYT_DS_VER;
    break;
  case ZB_NVRAM_DATASET_GP_CLUSTER:
    ds_version = ZB_NVRAM_DATASET_GP_CLUSTER_DS_VER;
    break;
#endif  /* #ifdef ZB_ENABLE_ZGP */
#if (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) || defined(DOXYGEN)
  case ZB_NVRAM_ZCL_REPORTING_DATA:
    ds_version = ZB_NVRAM_ZCL_REPORTING_DATA_DS_VER;
    break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */
  case ZB_NVRAM_APS_SECURE_DATA:
    ds_version = ZB_NVRAM_APS_SECURE_DATA_DS_VER;
    break;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
    case ZB_NVRAM_INSTALLCODES:
    ds_version = ZB_NVRAM_INSTALLCODES_DS_VER;
    break;
#endif
  case ZB_NVRAM_APS_BINDING_DATA:
    ds_version = ZB_NVRAM_APS_BINDING_DATA_DS_VER;
    break;
  case ZB_NVRAM_APS_GROUPS_DATA:
    ds_version = ZB_NVRAM_APS_GROUPS_DATA_DS_VER;
    break;
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN
  case ZB_NVRAM_ZCL_WWAH_DATA:
    ds_version = ZB_NVRAM_ZCL_WWAH_DATA_DS_VER;
    break;
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN */
#if defined ZB_STORE_COUNTERS || defined DOXYGEN
  case ZB_IB_COUNTERS:
    ds_version = ZB_IB_COUNTERS_DS_VER;
    break;
#endif
#endif  /* #ifndef APP_ONLY_NVRAM */
  case ZB_NVRAM_APP_DATA1:
    /* Application controls the version of application dataset by its own */
    break;
  case ZB_NVRAM_APP_DATA2:
    /* Application controls the version of application dataset by its own */
    break;
  case ZB_NVRAM_APP_DATA3:
    /* Application controls the version of application dataset by its own */
    break;
  case ZB_NVRAM_APP_DATA4:
    /* Application controls the version of application dataset by its own */
    break;

#ifndef APP_ONLY_NVRAM

  case ZB_NVRAM_ADDR_MAP:
    ds_version = ZB_NVRAM_ADDR_MAP_DS_VER;
    break;
  case ZB_NVRAM_NEIGHBOUR_TBL:
    ds_version = ZB_NVRAM_NEIGHBOUR_TBL_DS_VER;
    break;

#ifdef ZDO_DIAGNOSTICS
  case ZB_NVRAM_ZDO_DIAGNOSTICS_DATA:
    ds_version = ZB_NVRAM_DIAGNOSTICS_DATA_DS_VER;
    break;
#endif /* ZDO_DIAGNOSTICS */

#endif  /* #ifndef APP_ONLY_NVRAM */
  default:
    TRACE_MSG(TRACE_ERROR, "Unknown dataset type", (FMT__0));
    break;
  }

  return ds_version;
}


static zb_bool_t nvram_dataset_tail_is_ok(
  const zb_nvram_dataset_hdr_t *hdr,
  zb_uint8_t page,
  zb_uint32_t pos,
  zb_nvram_ver_t nvram_ver)
{
  zb_bool_t ret = ZB_TRUE;
  if (nvram_ver >= ZB_MIN_NVRAM_VER_WITH_DS_TRAILERS)
  {
    zb_nvram_dataset_tail_t tail;
    ret = ZB_FALSE;
    if (zb_osif_nvram_read(page, pos + hdr->data_len - sizeof(tail),
                           (zb_uint8_t*)&tail, (zb_uint16_t)sizeof(tail)) == RET_OK
        && tail.time_label != ZB_NVRAM_TL_RESERVED
        && tail.time_label == hdr->time_label)
    {
      ret = ZB_TRUE;
    }
  }
  return ret;
}


static zb_ret_t nvram_write_dataset_tail(
  zb_uint8_t page,
  zb_uint32_t pos,
  zb_uint32_t data_len,
  zb_nvram_tl_t time_label)
{
  zb_nvram_dataset_tail_t tail;
  tail.time_label = time_label;
  return zb_osif_nvram_write(page,
                             pos + data_len - sizeof(tail),
                             (zb_uint8_t*)&tail,
                             (zb_uint16_t)sizeof(tail));
}


static zb_ret_t zb_nvram_check_and_read_page_hdr_ds(
    zb_uint8_t page,
    zb_nvram_dataset_hdr_t *hdr,
    zb_nvram_page_hdr_dataset_t *page_hdr_ds)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_check_and_read_page_hdr_ds page %hd, hdr %p, len %d",
            (FMT__H_P_D, page, hdr, hdr->data_len));

  if (hdr->data_set_type == (zb_uint16_t)ZB_NVRAM_DATA_SET_TYPE_PAGE_HDR)
  {
    if (hdr->data_len == SIZE_DS_HEADER_1_0)
    {
      TRACE_MSG(TRACE_COMMON2, "page %hd has no version, use the first", (FMT__H, page));

      ret = RET_OBSOLETE;

      if (page_hdr_ds != NULL)
      {
        page_hdr_ds->version = ZB_NVRAM_VER_1_0;
      }
    }
    else if (hdr->data_len == PAGE_HEADER_SIZE_1_0)
    {
      ret = RET_OBSOLETE;

      if (page_hdr_ds != NULL)
      {
        ret = zb_osif_nvram_read(
           page, SIZE_DS_HEADER_1_0, (zb_uint8_t*)page_hdr_ds, (zb_uint16_t)sizeof(*page_hdr_ds));
      }

      if (ret == RET_OK)
      {
        ret = RET_OBSOLETE;
      }
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    if (hdr->data_len == SIZE_DS_HEADER)
    {
      TRACE_MSG(TRACE_COMMON2, "page %hd has no version, use the first", (FMT__H, page));

      if (page_hdr_ds != NULL)
      {
        page_hdr_ds->version = ZB_NVRAM_VER_1_0;
      }

      ret = RET_OK;
    }
    else if (hdr->data_len == PAGE_HEADER_SIZE)
    {
      ret = RET_OK;

      if (page_hdr_ds != NULL)
      {
        ret = zb_osif_nvram_read(page, SIZE_DS_HEADER, (zb_uint8_t*)page_hdr_ds, (zb_uint16_t)sizeof(*page_hdr_ds));
      }
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
  }

  if (ret == RET_OK)
  {
    ret = nvram_dataset_tail_is_ok(hdr, page, 0, page_hdr_ds->version) == ZB_TRUE ? RET_OK : RET_ERROR;
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_check_and_read_page_hdr_ds ret %d", (FMT__D, ret));

  return ret;
}


#ifndef APP_ONLY_NVRAM

static zb_ret_t zb_nvram_write_common_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret = RET_OK;
  zb_nvram_dataset_common_t ds;
  zb_channel_list_t aps_channel_mask_list;

  ZB_BZERO(&ds, sizeof(ds));
  ds.aps_designated_coordinator = ZB_AIB().aps_designated_coordinator;

  zb_channel_page_list_copy(aps_channel_mask_list, ZB_AIB().aps_channel_mask_list);
  ZB_MEMCPY((zb_uint8_t *)ds.aps_channel_mask_list, (zb_uint8_t *)aps_channel_mask_list, sizeof(zb_channel_list_t));

  ds.device_type = ZB_NIB().device_type;
  ds.rx_on = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
  ds.tc_swapped = ZB_B2U(ZB_TCPOL().tc_swapped);
  ZB_EXTPANID_COPY(ds.aps_use_extended_pan_id, ZB_AIB().aps_use_extended_pan_id);
  ds.aps_insecure_join = ZB_AIB().aps_insecure_join;
  ZB_EXTPANID_COPY(ds.nwk_extended_pan_id, ZB_NIB_EXT_PAN_ID());
  ds.panId = ZB_PIBCACHE_PAN_ID();
  ds.network_address = ZB_PIBCACHE_NETWORK_ADDRESS();
  if (ZG->nwk.handle.parent != (zb_uint8_t)-1)
  {
  zb_address_ieee_by_ref(ds.parent_address, ZG->nwk.handle.parent);
  }
  ds.stack_profile = ZB_STACK_PROFILE;
  ds.depth = ZB_NIB().depth;
  ds.nwk_manager_addr = ZB_NIB().nwk_manager_addr;
#ifndef ZB_COORDINATOR_ONLY

  ZB_IEEE_ADDR_COPY(ds.trust_center_address, ZB_AIB().trust_center_address);
#endif
  {
    zb_uint8_t *key = secur_nwk_key_by_seq(ZB_NIB().active_key_seq_number);
    if (key != NULL)
    {
      ZB_MEMCPY(&ds.nwk_key, key, ZB_CCM_KEY_SIZE);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "could not find a key", (FMT__0));
      ret = RET_ERROR;
    }
  }
  if (ret == RET_OK)
  {
    ds.nwk_key_seq = ZB_NIB().active_key_seq_number;
    ZB_MEMCPY(&ds.tc_standard_key, &ZB_AIB().tc_standard_key, ZB_CCM_KEY_SIZE);

  // custom field
  ds.channel = ZB_PIBCACHE_CURRENT_CHANNEL();
  ds.page = ZB_PIBCACHE_CURRENT_PAGE();

  ZB_MEMCPY(ds.mac_iface_tbl, ZB_NIB().mac_iface_tbl,
            ZB_NWK_MAC_IFACE_TBL_SIZE * sizeof(zb_nwk_mac_iface_tbl_ent_t));

  // trace
  TRACE_MSG(TRACE_COMMON1,
            "nvram wr designated_coord %hd use_extended_pan_id " TRACE_FORMAT_64 " channel_mask_list:",
            (FMT__H_A, ZB_AIB().aps_designated_coordinator, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
/* 10/03/2019 EE CR:MINOR Why ZB_MAC_TESTING_MODE is here?? If you
 * need trace from MAC testing, but strongly dislike that trace
 * (why?), you can move MAC testing trace to some other subsystem!
 * Here and below.
 */
#ifndef ZB_MAC_TESTING_MODE
#if TRACE_ENABLED(TRACE_COMMON1)
  {
    zb_uint8_t i;
    for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
    {
      TRACE_MSG(TRACE_COMMON1,
                "page %hd, mask 0x%lx",
                (FMT__H_L, zb_aib_channel_page_list_get_page(i), zb_aib_channel_page_list_get_mask(i)));
    }
  }
#endif /* TRACE_ENABLED(TRACE_COMMON1) */
#endif /* ZB_MAC_TESTING_MODE */
  ds.hub_connectivity = ZB_NIB().nwk_hub_connectivity;
  TRACE_MSG(TRACE_COMMON1,
            "nvram wr insecure_join %hd panid 0x%x nwk_addr 0x%x depth %hd nwk_manager_addr 0x%x",
            (FMT__H_D_D_H_D, ZB_AIB().aps_insecure_join, ZB_PIBCACHE_PAN_ID(),
             ZB_PIBCACHE_NETWORK_ADDRESS(),
             ZB_NIB().depth, ZB_NIB().nwk_manager_addr));
#ifndef ZB_COORDINATOR_ONLY
  TRACE_MSG(TRACE_COMMON1,
            "nvram wr tc_addr " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ZB_AIB().trust_center_address)));
#endif
  TRACE_MSG(TRACE_COMMON1,
            "nvram wr nwk key 0 " TRACE_FORMAT_128,
            (FMT__AA, TRACE_ARG_128(ZB_NIB().secur_material_set[0].key)));
  TRACE_MSG(TRACE_COMMON1,
            "nvram wr tc std key " TRACE_FORMAT_128,
            (FMT__AA, TRACE_ARG_128(ZB_AIB().tc_standard_key)));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));
  }
  return ret;
}

static void zb_nvram_read_common_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_uint16_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_nvram_dataset_common_t ds;
  zb_address_pan_id_ref_t panid_ref;
  zb_ret_t ret;
  zb_channel_list_t aps_channel_mask_list;

  ZVUNUSED(len);
  ZVUNUSED(nvram_ver);
  /* Currently dataset version is not used */
  ZVUNUSED(ds_ver);

#ifndef ZB_NO_NVRAM_VER_MIGRATION
  if (nvram_ver < ZB_NVRAM_VER_7_0)
  {
    /* Note: migration to zb_nvram_dataset_common_ver_3_0_t (channel mask list etc) is not implemented!  */
    ZB_ASSERT(0);
    ret = RET_ERROR;
#if 0
    zb_nvram_dataset_common_ver_1_0_t old_ds;

    ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&old_ds, sizeof(old_ds));
    /* Convert to new version: tc_alternative_key was removed, so do memcpy and then copy channel. */
    ZB_MEMCPY(&ds, &old_ds, sizeof(ds));
    ds.channel = old_ds.channel;
#endif
  }
  else
#endif
  {
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,0} */
  if (ret == RET_OK)
  {
    ZB_AIB().aps_designated_coordinator = ds.aps_designated_coordinator;

    ZB_MEMCPY((zb_uint8_t *)aps_channel_mask_list, (zb_uint8_t *)ds.aps_channel_mask_list, sizeof(zb_channel_list_t));
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, aps_channel_mask_list);

    ZB_EXTPANID_COPY(ZB_AIB().aps_use_extended_pan_id, ds.aps_use_extended_pan_id);

    ZB_AIB().aps_insecure_join = ds.aps_insecure_join;
    /* EE: uncommented. Really need to restore current ext panid */
    ZB_EXTPANID_COPY(ZB_NIB_EXT_PAN_ID(), ds.nwk_extended_pan_id);
    ZB_PIBCACHE_PAN_ID() = ds.panId;                      //
    ZB_PIBCACHE_NETWORK_ADDRESS() = ds.network_address;   //
    /* handle.parent will be filled when loading neighbor table */
    ZG->nwk.handle.parent = (zb_uint8_t)-1;
    TRACE_MSG(TRACE_NWK3, "Set handle.parent -1", (FMT__0));
    //zb_address_ieee_by_ref(parent_addr, ZG->nwk.handle.parent); // wisa versa
    ZB_NIB().depth = ds.depth;
    ZB_NIB().nwk_manager_addr = ds.nwk_manager_addr;
    if (ZB_IS_DEVICE_ZC())
    {
      /* Save ZC's long address. It can be necessary for TC swapout. */
      ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, ZB_PIBCACHE_EXTENDED_ADDRESS());
    }
    {
      ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, ds.trust_center_address);
    }
    ZB_NIB().active_key_seq_number = ds.nwk_key_seq;
    ZB_MEMCPY(&ZB_NIB().secur_material_set[0].key, &ds.nwk_key, ZB_CCM_KEY_SIZE);
    ZB_NIB().secur_material_set[0].key_seq_number = ZB_NIB().active_key_seq_number;
    ZB_MEMCPY(&ZB_AIB().tc_standard_key, &ds.tc_standard_key, ZB_CCM_KEY_SIZE);
    ZG->nwk.handle.joined_restart = ZB_TRUE;

    // custom field
    ZB_PIBCACHE_CURRENT_CHANNEL() = ds.channel;
    ZB_PIBCACHE_CURRENT_PAGE() = ds.page;

#ifdef NCP_MODE
    ZB_NIB().device_type = ds.device_type;
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ds.rx_on;
#endif
#endif  /* NCP_MODE */
    ZB_TCPOL().tc_swapped = ZB_U2B(ds.tc_swapped);

    TRACE_MSG(TRACE_NWK3, "device_type %d is_rx_on %d", (FMT__D_D, ZB_NIB().device_type, ZB_PIBCACHE_RX_ON_WHEN_IDLE()));

    ZB_MEMCPY(ZB_NIB().mac_iface_tbl, ds.mac_iface_tbl,
              ZB_NWK_MAC_IFACE_TBL_SIZE * sizeof(zb_nwk_mac_iface_tbl_ent_t));

    // for security: sender check frame counter when make unscript message

    ZB_NIB().nwk_hub_connectivity = ds.hub_connectivity;
#ifndef ZB_STORE_COUNTERS
    ZB_NIB().outgoing_frame_counter = (zb_uint32_t)~0 - 50; /* AD: fixme, probably need to store it */
#endif
    if ((ZB_PIBCACHE_PAN_ID()== ZB_BROADCAST_PAN_ID) || (ZB_PIBCACHE_NETWORK_ADDRESS() == ZB_NWK_BROADCAST_ALL_DEVICES))
    {
      ZG->aps.authenticated = ZB_FALSE;
    }
    else
    {
      ZG->aps.authenticated = ZB_TRUE;
    }

    /* ZC's long address is saved in TC address field to keep Common dataset same.
       That address is required after TC swapout if we keep old TC's address.
       IN that case need to overwrite old TCs IEEE from prod config.
     */
    if (ZB_IS_DEVICE_ZC()
      /*cstat !MISRAC2012-Rule-2.1_b */
      /** @mdr{00012,21} */
        && !ZB_IEEE_ADDR_IS_ZERO(ds.trust_center_address)
      /*cstat !MISRAC2012-Rule-14.3_b */
      /** @mdr{00012,22} */
        && !ZB_IEEE_ADDR_IS_UNKNOWN(ds.trust_center_address))
    {
      ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), ds.trust_center_address);
    }

  /* restore our pan_id in pan_id translation table after reboot */
  if (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()) && ZB_PIBCACHE_PAN_ID() != ZB_BROADCAST_PAN_ID)
  {
    ret = zb_address_set_pan_id(ZB_PIBCACHE_PAN_ID(), ZB_NIB_EXT_PAN_ID(), &panid_ref);
    ZB_ASSERT(ret == RET_OK || ret == RET_ALREADY_EXISTS);
  }

    // trace
    TRACE_MSG(TRACE_COMMON1,
              "nvram wr designated_coord %hd use_extended_pan_id " TRACE_FORMAT_64 " channel_mask_list:",
              (FMT__H_A, ZB_AIB().aps_designated_coordinator, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
#ifndef ZB_MAC_TESTING_MODE
#if TRACE_ENABLED(TRACE_COMMON1)
    {
      zb_uint8_t i;
      for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
      {
        TRACE_MSG(TRACE_COMMON1,
                  "page %hd, mask 0x%lx",
                  (FMT__H_L, zb_aib_channel_page_list_get_page(i), zb_aib_channel_page_list_get_mask(i)));
      }
    }
#endif /* TRACE_ENABLED(TRACE_COMMON1) */
#endif /* ZB_MAC_TESTING_MODE */
    TRACE_MSG(TRACE_COMMON1,
              "nvram rx insecure_join %hd panid 0x%x nwk_addr 0x%x depth %hd nwk_manager_addr 0x%x",
              (FMT__H_D_D_H_D, ZB_AIB().aps_insecure_join, ZB_PIBCACHE_PAN_ID(),
               ZB_PIBCACHE_NETWORK_ADDRESS(),
               ZB_NIB().depth, ZB_NIB().nwk_manager_addr));
#ifndef ZB_COORDINATOR_ONLY
    TRACE_MSG(TRACE_COMMON1,
              "nvram rx tc_addr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ZB_AIB().trust_center_address)));
#endif
    TRACE_MSG(TRACE_COMMON1,
            "nvram rx nwk key 0 " TRACE_FORMAT_128,
            (FMT__AA, TRACE_ARG_128(ZB_NIB().secur_material_set[0].key)));
    TRACE_MSG(TRACE_COMMON1,
            "nvram rx tc std key " TRACE_FORMAT_128,
            (FMT__AA, TRACE_ARG_128(ZB_AIB().tc_standard_key)));
  }
}
#endif  /* #ifndef APP_ONLY_NVRAM */


static zb_ret_t write_dataset_body_by_type(zb_uint8_t ds_type, zb_nvram_position_t *pos_info)
{
  zb_ret_t ret;
  zb_uint8_t info_page = (zb_uint8_t)pos_info->page;

  /* Checked within zb_nvram_write_data() */
  ZB_NVRAM().gs_nvram_write_checker.page = pos_info->page;
  ZB_NVRAM().gs_nvram_write_checker.pos = pos_info->pos;
  ZB_NVRAM().gs_nvram_write_checker.payload_length = pos_info->payload_length;

  switch(ds_type)
  {
#ifndef APP_ONLY_NVRAM
    case ZB_NVRAM_COMMON_DATA:
      ret = zb_nvram_write_common_dataset(info_page, pos_info->pos);
      break;
#if defined ZB_ENABLE_HA || defined DOXYGEN
    case ZB_NVRAM_HA_DATA:
      ret = zb_nvram_write_ha_dataset(info_page, pos_info->pos);
      break;
#endif /* defined ZB_ENABLE_HA */
#ifdef ZB_ENABLE_ZGP
    case ZB_NVRAM_DATASET_GP_SINKT:
      ret = zb_nvram_write_zgp_sink_table_dataset(info_page, pos_info->pos);
      break;
    case ZB_NVRAM_DATASET_GP_APP_TBL:
      ret = zb_zgp_nvram_write_app_tbl_dataset(info_page, pos_info->pos);
      break;
    case ZB_NVRAM_DATASET_GP_PRPOXYT:
      ret = zb_nvram_write_zgp_proxy_table_dataset(info_page, pos_info->pos);
      break;
    case ZB_NVRAM_DATASET_GP_CLUSTER:
      ret = zb_nvram_write_zgp_cluster_dataset(info_page, pos_info->pos);
      break;
#endif
#if (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) || defined(DOXYGEN)
    case ZB_NVRAM_ZCL_REPORTING_DATA:
      ret = zb_nvram_write_zcl_reporting_dataset(info_page, pos_info->pos);
      break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */
    case ZB_NVRAM_APS_SECURE_DATA:
      ret = zb_nvram_write_aps_keypair_dataset(info_page, pos_info->pos);
      break;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
    case ZB_NVRAM_INSTALLCODES:
      ret = zb_nvram_write_installcodes_dataset(info_page, pos_info->pos);
      break;
#endif
    case ZB_NVRAM_APS_BINDING_DATA:
      ret = zb_nvram_write_aps_binding_dataset(info_page, pos_info->pos);
      break;
    case ZB_NVRAM_APS_GROUPS_DATA:
      ret = zb_nvram_write_aps_groups_dataset(info_page, pos_info->pos);
      break;
#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
      ret = zb_nvram_write_poll_control_dataset(info_page, pos_info->pos);
      break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN
    case ZB_NVRAM_ZCL_WWAH_DATA:
      ret = zb_nvram_write_wwah_dataset(info_page, pos_info->pos);
      break;
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN */
#if defined ZB_STORE_COUNTERS || defined DOXYGEN
    case ZB_IB_COUNTERS:
      ret = zb_nvram_write_counters_dataset(info_page, pos_info->pos);
      break;
#endif
#endif  /* #ifndef APP_ONLY_NVRAM */
    case ZB_NVRAM_APP_DATA1:
      if (ZB_NVRAM().write_app_data_cb[0] != NULL)
      {
        ret = ZB_NVRAM().write_app_data_cb[0](info_page, pos_info->pos);
      }
      else
      {
        ret = RET_ERROR;
      }
      break;
    case ZB_NVRAM_APP_DATA2:
      if (ZB_NVRAM().write_app_data_cb[1] != NULL)
      {
        ret = ZB_NVRAM().write_app_data_cb[1](info_page, pos_info->pos);
      }
      else
      {
        ret = RET_ERROR;
      }
      break;
    case ZB_NVRAM_APP_DATA3:
      if (ZB_NVRAM().write_app_data_cb[2] != NULL)
      {
        ret = ZB_NVRAM().write_app_data_cb[2](info_page, pos_info->pos);
      }
      else
      {
        ret = RET_ERROR;
      }
      break;
    case ZB_NVRAM_APP_DATA4:
      if (ZB_NVRAM().write_app_data_cb[3] != NULL)
      {
        ret = ZB_NVRAM().write_app_data_cb[3](info_page, pos_info->pos);
      }
      else
      {
        ret = RET_ERROR;
      }
      break;

#ifndef APP_ONLY_NVRAM

    case ZB_NVRAM_ADDR_MAP:
      ret = zb_nvram_write_addr_map_dataset(info_page, pos_info->pos);
      break;
    case ZB_NVRAM_NEIGHBOUR_TBL:
      ret = zb_nvram_write_neighbour_tbl_dataset(info_page, pos_info->pos);
      break;

#ifdef ZDO_DIAGNOSTICS
    case ZB_NVRAM_ZDO_DIAGNOSTICS_DATA:
      ret = zb_nvram_write_diagnostics_dataset(info_page, pos_info->pos);
      break;
#endif /* ZDO_DIAGNOSTICS */

#endif  /* #ifndef APP_ONLY_NVRAM */
    default:
      ret = zb_nvram_custom_ds_try_write(ds_type, info_page, pos_info->pos);
      break;
  }

#ifdef ZB_NVRAM_ENABLE_DIRECT_API
  if (ret == RET_OK)
  {
    nvram_update_dataset_position(ds_type, info_page, pos_info->pos);
  }
#endif /* ZB_NVRAM_ENABLE_DIRECT_API */

  /* Operation finished. Reset checkers. */
  ZB_NVRAM().gs_nvram_write_checker.page = 0;
  ZB_NVRAM().gs_nvram_write_checker.pos = 0;
  ZB_NVRAM().gs_nvram_write_checker.payload_length = 0;

  return ret;
}

#ifdef APP_ONLY_NVRAM
void zb_nvram_local_init(void *id)
{
  ZB_BZERO(&gs_nvram, sizeof(gs_nvram));
  zb_osif_nvram_init(id);
}
#endif

void zb_nvram_load(void)
{
  zb_nvram_position_t *datasets;
  zb_nvram_load_data_t load_data;
  zb_uint8_t i;
  zb_ret_t status;
#ifndef ZB_NO_NVRAM_VER_MIGRATION
  zb_bool_t migrate_version;
#endif
  zb_uint8_t nvram_page_count;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_load", (FMT__0));

#ifndef APP_ONLY_NVRAM
  if (ZB_U2B(ZB_AIB().aps_nvram_erase_at_start))
  {
    TRACE_MSG(TRACE_COMMON1, "force nvram erase", (FMT__0));
    zb_nvram_erase();
    ZB_NVRAM().inited = 1;
    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_load", (FMT__0));
    return;
  }

  if (!ZB_U2B(ZB_AIB().aps_use_nvram))
  {
    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_load: do not use nvram", (FMT__0));
    return;
  }
#endif  /* #ifdef APP_ONLY_NVRAM */

  TRACE_MSG(TRACE_COMMON1, ">> nvram load", (FMT__0));

  /* clean all except app cbs */
  ZB_BZERO(&ZB_NVRAM().current_time_label,
           sizeof(ZB_NVRAM()) - ZB_OFFSETOF(zb_nvram_globals_t, current_time_label));
  ZB_NVRAM().current_time_label = ZB_NVRAM_TL_RESERVED;

  ZB_BZERO(&load_data, sizeof(load_data));
  load_data.time_label = ZB_NVRAM_TL_RESERVED;

  status = nvram_read_pages_headers();
#ifndef ZB_NO_NVRAM_VER_MIGRATION
  migrate_version = (status == RET_OBSOLETE) ? (ZB_TRUE) : (ZB_FALSE);
#else
  ZVUNUSED(status);
#endif

  /* find poses */
  nvram_page_count = zb_get_nvram_page_count();
  for(i = 0; i < nvram_page_count; i++)
  {
    /* At this point we aleady have an information about NVRAM pages' headers
     * and their versions. Search for datasets only in valid NVRAM pages */
    if (ZB_NVRAM().pages[i].nvram_ver != ZB_NVRAM_VER_COUNT)
    {
      status = find_datasets_on_page(i, &load_data);

    if (status == RET_OK)
    {
      /* Last actual time label */
      ZB_NVRAM().current_time_label = load_data.time_label;

      /* Stay on the existing full page even if it is the end of the
       * page. Migrate to the next one upon writing attempt*/
      ZB_NVRAM().current_page = load_data.page;
      ZB_NVRAM().current_pos = load_data.pos;

      TRACE_MSG(TRACE_COMMON2, "Page %d has been read successfully. current_time_label %d current_page %hd current_pos %ld",
                (FMT__H_D_H_L, i, ZB_NVRAM().current_time_label, ZB_NVRAM().current_page, ZB_NVRAM().current_pos));
    }
    else
    {
      /* Mark page as "dirty" and erase it */
      ZB_NVRAM().pages[i].erased = 0;
        ZB_NVRAM().pages[i].nvram_ver = ZB_NVRAM_VER_COUNT;
      zb_nvram_erase_if_not_erased(i);

      /* Initialise iterating variable */
      ZB_BZERO(&load_data, sizeof(load_data));
      load_data.time_label = ZB_NVRAM().current_time_label;

      if (status == RET_ERROR)
      {
        ZB_ERROR_RAISE(ZB_ERROR_SEVERITY_FATAL,
                       ERROR_CODE(ERROR_CATEGORY_NVRAM, ZB_ERROR_NVRAM_READ_FAILURE),
                       NULL);
      }
    }
    }   /* if valid page */
  }   /* for pages */

  /* Perfect place for checking previous nvram migration */
  /*
    Have following possibilities except correct case (one page is erased -
    migration completed):

    a) Complete migration p0 - p1, but crached before p1 erase.
    In such case all datasets will be loaded from p0.
    All datasets are from p0.
    p1 is to be erased and migrated at next write attempt.

    b) Incomplete migration p0 - p1.
    Also, all datasets are loaded from p0.

    c) Complete migration p1 - p0, crash before p1 erase.
    All data to be loaded from p0, p1 to be erased.

    d) Partial migration p1 - p0.
    Only in that case we can have different pages for different datasets in
    load_data.
    Load all from p1, repeat p1 - p0 migration at write attempt.

    nvram_handle_datasets_pages() is doing that logic (while, a bit complicated,
    for me - but let's keep it as is).
   */
  nvram_handle_datasets_pages(&load_data);
  datasets = load_data.datasets;

  nvram_restore_datasets(datasets);
#ifndef ZB_NO_NVRAM_VER_MIGRATION
  if (migrate_version)
  {
    migrate_curr_page_to_new_version();
  }
#endif
  TRACE_MSG(TRACE_COMMON1, "nvram current: page %hd pos %ld time index %d",
            (FMT__H_L_D, ZB_NVRAM().current_page, ZB_NVRAM().current_pos, ZB_NVRAM().current_time_label));

  /* If found nothing */
  if (ZB_NVRAM().current_pos == 0U)
  {
    TRACE_MSG(TRACE_COMMON1,
        "Found no records - go to beginning of page 0", (FMT__0));

    zb_nvram_erase_if_not_erased(0);
    ZB_NVRAM().current_page = 0;
    ZB_NVRAM().current_pos = PAGE_HEADER_SIZE;

    if (ZB_NVRAM().current_time_label == ZB_NVRAM_TL_RESERVED)
    {
      ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INITIAL;
    }
  }

  ZB_NVRAM().inited = 1;

  TRACE_MSG(TRACE_COMMON1, "<< nvram load", (FMT__0));
}


static zb_ret_t zb_nvram_write_page_hdr_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  zb_nvram_page_hdr_dataset_t page_hdr_ds;

  ZB_BZERO(&page_hdr_ds, sizeof(page_hdr_ds));

  page_hdr_ds.version = ZB_NVRAM_VERSION;

  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&page_hdr_ds, (zb_uint16_t)sizeof(page_hdr_ds));

  return ret;
}


static void zb_nvram_erase_if_not_erased(zb_uint8_t page)
{
  if (!ZB_U2B(ZB_NVRAM().pages[page].erased))
  {
    if (!ZB_U2B(ZB_NVRAM().pages[page].erase_in_progress))
    {
      ZB_NVRAM().pages[page].erase_in_progress = 1;
      (void)zb_osif_nvram_erase_async(page);
    }
#ifdef ZB_PLATFORM_CORTEX_M3
    zb_osif_nvram_wait_for_last_op();
#endif
  }
}


static zb_ret_t find_datasets_on_page(zb_uint8_t page, zb_nvram_load_data_t *load_data)
{
  zb_ret_t ret = RET_ERROR;
  zb_nvram_dataset_hdr_t hdr;
  zb_uint32_t page_length = zb_get_nvram_page_length();
  zb_nvram_ver_t nvram_ver = ZB_NVRAM().pages[page].nvram_ver;
  zb_bool_t cur_page_is_actual = ZB_FALSE;
  zb_uint32_t hdr_size;

  TRACE_MSG(TRACE_COMMON2, ">> find_datasets_on_page page %hd, load_data %p",
            (FMT__H_P, page, load_data));

  ZB_ASSERT(load_data);
  ZB_ASSERT(page < ZB_NVRAM_MAX_N_PAGES);

  if (nvram_ver != ZB_NVRAM_VER_COUNT)
  {
    zb_uint32_t pos = 0;
    ret = RET_OK;

    if (nvram_ver < ZB_NVRAM_VER_7_0)
    {
      hdr_size = SIZE_DS_HEADER_1_0;
    }
    else
    {
      hdr_size = SIZE_DS_HEADER;
    }

    /* Start from the beginning - even including page header, restore timelabel*/
    while (pos+hdr_size < page_length && ret == RET_OK)
    {
      ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));
      TRACE_MSG(TRACE_COMMON1, "page %hd pos %ld hdr status %d type 0x%x len %d",
                (FMT__H_L_H_D_D, page, pos, ret, hdr.data_set_type, hdr.data_len));

      /*cstat !MISRAC2012-Rule-14.3_a */
      /** @mdr{00007,1} */
      if (ret == RET_OK)
      {
        if (pos == 0U)
        {
          zb_nvram_page_hdr_dataset_t page_hdr_ds;

          ret = zb_nvram_check_and_read_page_hdr_ds(page, &hdr, &page_hdr_ds);

          if (ret == RET_OBSOLETE)
          {
            ret = RET_OK;
          }

          if (ret == RET_OK)
          {
            if (page_hdr_ds.version <= ZB_NVRAM_MAX_VERSION)
            {
              TRACE_MSG(TRACE_COMMON2, "page %hd has correct erase header", (FMT__H, page));
              ZB_NVRAM().pages[page].erased = 1;
            }
            else
            {
              TRACE_MSG(TRACE_ERROR, "Incorrect NVRAM version %d", (FMT__D, page_hdr_ds.version));
              ZB_NVRAM().pages[page].nvram_ver = ZB_NVRAM_VER_COUNT;
              ret = RET_ERROR;
              break;
            }
          }
          else
          {
            TRACE_MSG(TRACE_COMMON1, "Some dust at page begin", (FMT__0));
            ZB_NVRAM().pages[page].nvram_ver = ZB_NVRAM_VER_COUNT;
            ret = RET_INVALID_STATE;
            break;
          }
        }
        /* End of page: hdr fields after page erase */
        else if (
          (nvram_ver < ZB_NVRAM_VER_7_0 && ZB_NVRAM_DATA_SET_HDR_1_0_IS_EMPTY(&hdr)) ||
          ZB_NVRAM_DATA_SET_HDR_IS_EMPTY(&hdr))
        {
          TRACE_MSG(TRACE_COMMON2, "end of page %hd at pos %ld", (FMT__H_L, page, pos));
          break;
        }
        else if (hdr.data_len == 0U || !ZB_NVRAM_DATA_SET_HDR_IS_VALID(&hdr))
        {
          TRACE_MSG(TRACE_ERROR, "error: data_len == 0 and not end of page!", (FMT__0));
          ret = RET_ERROR;
          break;
        }
        /* Normal record */
        else
        {
          /* Have some data - page is not "just erased". */
          ZB_NVRAM().pages[page].erased = 0;

          /* If time label != reserved, sure data and other heared fields are
           * written successfully.
           * Else, can have valid type and length in the header but corrupted
           * data - skip this dataset.
           */
          if (hdr.time_label != ZB_NVRAM_TL_RESERVED
              /* Additional check: time label at dataset trailer */

              /*cstat !MISRAC2012-Rule-13.5 */
              /* After some investigation, the following violation of Rule 13.5 seems to be a false
               * positive. There are no side effects to 'nvram_dataset_tail_is_ok()'. None of the
               * variables passed to the function as parameters are changed. */
              && nvram_dataset_tail_is_ok(&hdr, page, pos, nvram_ver)
              /* additional validness check */
              && hdr.data_set_type < (zb_uint16_t)ZB_NVRAM_DATASET_NUMBER
              && (
                  /* first record of that type */
                  load_data->datasets[hdr.data_set_type].pos == 0U
                  /* this is newer record */
                  || ZB_NVRAM_TL_GT(hdr.time_label,
                                    load_data->datasets[hdr.data_set_type].time_label)))
          {
            load_data->datasets[hdr.data_set_type].pos = pos + hdr_size;
            load_data->datasets[hdr.data_set_type].payload_length = hdr.data_len - (zb_uint16_t)hdr_size;
            if (nvram_ver >= ZB_NVRAM_VER_9_0)
            {
              load_data->datasets[hdr.data_set_type].payload_length -= (zb_uint16_t)sizeof(zb_nvram_dataset_tail_t);
            }

            load_data->datasets[hdr.data_set_type].version = ((nvram_ver < ZB_NVRAM_VER_7_0)
                                                              ? (ZB_NVRAM_DATA_SET_VERSION_NOT_AVAILABLE)
                                                              : (hdr.data_set_version));

            load_data->datasets[hdr.data_set_type].page = page;
            load_data->datasets[hdr.data_set_type].time_label = hdr.time_label;

            /*
              Switch current page to the page we got last record.
              In general, every page must have all types of records.
              There is one exception: when we died during migration. In such
              case some page have not all record types. There are two scenarios:

              a) page 0 is old, page 1 is new (incomplete). When migrating, we do not change
              time labels, so nothing came from page 1. Page 1 is not empty, so
              it will be erased at next write attempt.

              b) page1 1 is old, page 0 is new (incomplete). Some records got
              from page 0 (new), other from page1. It is not a problem because
              records are identical. Page 1 is after page 0, so
              current page is page 1 (old). Next write attempt will erase page 0
              and migrate to it.

            */

            /* Mark page as actual */
            cur_page_is_actual = ZB_TRUE;
            load_data->page = page;
            load_data->pos = pos + hdr.data_len;

            TRACE_MSG(TRACE_COMMON2, "current dataset[%hd] pos %ld page %hd time_label %d",
                      (FMT__H_L_H_D, hdr.data_set_type, load_data->datasets[hdr.data_set_type].pos,
                       load_data->datasets[hdr.data_set_type].page,
                       load_data->datasets[hdr.data_set_type].time_label));
          }
          else if (cur_page_is_actual)
          {
            /* Current page is actual, we should skip corrupted dataset */
            load_data->pos = pos + hdr.data_len;
          }
          else
          {
            /* MISRA rule 15.7 requires empty 'else' branch. */
          }
        }

        if (hdr.time_label != ZB_NVRAM_TL_RESERVED
            && (hdr.data_set_type < (zb_uint16_t)ZB_NVRAM_DATASET_NUMBER
                || hdr.data_set_type == (zb_uint16_t)ZB_NVRAM_DATA_SET_TYPE_PAGE_HDR)
            && (load_data->time_label == ZB_NVRAM_TL_RESERVED
                || ZB_NVRAM_TL_GT(hdr.time_label, load_data->time_label)))
        {
          load_data->time_label = hdr.time_label;
          /* Place next write position just after that fresh record. */
          TRACE_MSG(TRACE_COMMON2, "current_time_label %d current_page %hd curent_pos %ld",
                    (FMT__D_H_L, load_data->time_label,
                     load_data->page, load_data->pos));
        }

        pos += hdr.data_len;
      } /* if (ret == RET_OK) */
    } /* while() */
  } /* if (nvram_ver != ZB_NVRAM_VER_COUNT) */

  TRACE_MSG(TRACE_COMMON2, "<< find_datasets_on_page ret %d", (FMT__D, ret));

  return ret;
}

#ifndef ZB_NO_NVRAM_VER_MIGRATION
static void migrate_curr_page_to_new_version(void)
{
  zb_nvram_load_data_t load_data;
  zb_uint8_t j;
  zb_uint32_t pos;
  zb_nvram_dataset_hdr_t hdr = { 0 };
  zb_ret_t status;
  zb_uint8_t page = ZB_NVRAM().current_page;
  zb_uint8_t next_page = (ZB_NVRAM().current_page+1) % zb_get_nvram_page_count();

  TRACE_MSG(TRACE_COMMON1, ">> migrate_curr_page_to_new_version", (FMT__0));

  /* Force next page erase */
  ZB_NVRAM().pages[next_page].erased = 0;
  zb_nvram_erase_if_not_erased(next_page);

  ZB_BZERO(&load_data, sizeof(zb_nvram_load_data_t));
  status = find_datasets_on_page(page, &load_data);

  TRACE_MSG(TRACE_COMMON1, "nvram current: page %d pos %ld time index %d",
            (FMT__H_L_D, ZB_NVRAM().current_page, ZB_NVRAM().current_pos, ZB_NVRAM().current_time_label));

  /* just after next page hdr */
  pos = PAGE_HEADER_SIZE;

  if (status == RET_OK)
  {
    /* read and move */
    for(j=0; j<ZB_NVRAM_DATASET_NUMBER; j++)
    {
      if (load_data.datasets[j].pos &&
          nvram_can_load_dataset(j, ZB_NVRAM().pages[page].nvram_ver))
      {
        zb_nvram_position_t  pos_info;

        /* All that data sets has header and a body. */
        TRACE_MSG(TRACE_COMMON1, "migration to new ver: ds type %d page %d",
                  (FMT__D_D, j, page));

        hdr.data_set_type = j;
        hdr.data_len = zb_nvram_get_length_dataset(&hdr);
        ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INC(ZB_NVRAM().current_time_label);
        hdr.time_label = ZB_NVRAM().current_time_label;

        hdr.data_set_version = zb_nvram_get_dataset_version(hdr.data_set_type);

        zb_osif_nvram_write(next_page,
                            pos,
                            (zb_uint8_t*)&hdr,
                            sizeof(hdr));

        pos_info.page = next_page;
        pos_info.pos = pos + sizeof(hdr);
        pos_info.payload_length = hdr.data_len - sizeof(hdr);

        if (write_dataset_body_by_type(j, &pos_info) == RET_OK)
        {
          /* Commit record migration by writing time label at ds tail. */
          nvram_write_dataset_tail(pos_info.page, pos_info.pos - sizeof(hdr), hdr.data_len, hdr.time_label);
        }

        pos += hdr.data_len;
      } /* if tl != reserved */
    }   /* for datasets */
    zb_osif_nvram_flush();
  }     /* if source page is not empty */
  else
  {
    TRACE_MSG(TRACE_COMMON1, "Error reading datasets on curr page %d", (FMT__D, status));
  }

  pos = nvram_migrate_obsolete_datasets(load_data.datasets, next_page, pos);

  zb_osif_nvram_flush();

  /* Erase all obsolete pages to support new version */
  for (j=0; j<ZB_NVRAM_MAX_N_PAGES; j++)
  {
    if (ZB_NVRAM().pages[j].nvram_ver < ZB_NVRAM_VERSION)
    {
      ZB_NVRAM().pages[j].erased = 0;
      zb_nvram_erase_if_not_erased(j);
    }
  }

  ZB_NVRAM().current_page = next_page;
  ZB_NVRAM().current_pos = pos;
  TRACE_MSG(TRACE_COMMON1, "New page %hd pos %ld",
            (FMT__H_L, ZB_NVRAM().current_page, ZB_NVRAM().current_pos));

  TRACE_MSG(TRACE_COMMON1, "<< migrate_curr_page_to_new_version", (FMT__0));
}
#endif  /* ZB_NO_NVRAM_VER_MIGRATION */

static void zb_nvram_migrate_current_page(void)
{
  zb_nvram_load_data_t load_data;
  zb_uint8_t j;
  zb_uint32_t pos = 0;
  zb_nvram_dataset_hdr_t hdr;
  zb_ret_t status;
  zb_uint8_t page = ZB_NVRAM().current_page;
  zb_uint8_t next_page = (ZB_NVRAM().current_page+1U) % zb_get_nvram_page_count();

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_migrate_current_page", (FMT__0));
  /* Flash in case migrating inside a transaction.
     Transaction start/commit functions should be called by higher logic
   */
  zb_osif_nvram_flush();

#ifndef ZB_NO_NVRAM_VER_MIGRATION
  /* Erase next page preventively, even if it is already erased - it
   * may be erased, but contains old version page header. */
  ZB_NVRAM().pages[next_page].erased = 0;
#endif
  zb_nvram_erase_if_not_erased(next_page);

  TRACE_MSG(TRACE_COMMON1, "nvram current: page %d pos %ld time index %d",
            (FMT__H_L_D, ZB_NVRAM().current_page, ZB_NVRAM().current_pos, ZB_NVRAM().current_time_label));

  /* Scan current page to find recent data record positions. */
  ZB_BZERO(&load_data, sizeof(zb_nvram_load_data_t));
  status = find_datasets_on_page(page, &load_data);

  if (status == RET_OK)
  {
    /* just after next page hdr */
    pos = PAGE_HEADER_SIZE;

    /* read and move */
    for(j=0; j<(zb_uint8_t)ZB_NVRAM_DATASET_NUMBER; j++)
    {
      if (load_data.datasets[j].pos != 0U)
      {
        /* All that data sets has header and a body. */
        zb_uint8_t buf[64];
        unsigned wr_size, off;

        TRACE_MSG(TRACE_COMMON1, "migration: ds type %hd page %hd dataset_off %ld",
                  (FMT__H_H_L, j, page, load_data.datasets[j].pos));
        TRACE_MSG(TRACE_COMMON1, "migration: pos %ld", (FMT__L, pos));

        (void)zb_osif_nvram_read(page, load_data.datasets[j].pos-sizeof(hdr), (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));
        /* Write hdr  */
        (void)zb_osif_nvram_write(next_page,
                            pos,
                            (zb_uint8_t*)&hdr,
                            (zb_uint16_t)sizeof(hdr));

        pos += sizeof(hdr);
        hdr.data_len -= (zb_uint16_t)sizeof(hdr);
        off = 0;
        while (off < hdr.data_len)
        {
          wr_size = hdr.data_len - off;
          if (wr_size > sizeof(buf))
          {
            wr_size = sizeof(buf);
          }
          (void)zb_osif_nvram_read(page, load_data.datasets[j].pos + off, buf, (zb_uint16_t)wr_size);
          (void)zb_osif_nvram_write(next_page, pos + off, buf, (zb_uint16_t)wr_size);
          off += wr_size;
        }
        /* Note: dataset length includes dataset trailer, so we already wrote dataset trailer inside a loop above. */

        /* TODO: move to a function! */
#ifndef APP_ONLY_NVRAM
        /* Move offsets for APS_SECURE_DATA */
        if (j == (zb_uint8_t)ZB_NVRAM_APS_SECURE_DATA)
        {
          zb_uindex_t i;

          TRACE_MSG(TRACE_COMMON1, "migrate ZB_NVRAM_APS_SECURE_DATA", (FMT__0));
          ZB_AIB().aps_device_key_pair_storage.nvram_page = next_page;
          for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ++i)
          {
            if ((ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != 0U) &&
                /* Do not touch cached record */
                (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != ZB_APS_DEVICE_KEY_PAIR_CACHED))
            {
              TRACE_MSG(TRACE_COMMON1, "nvram_offset: was %ld, move to %ld, offset %ld",
                        (FMT__L_L_L, ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset,
                         ((ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset - load_data.datasets[j].pos) + pos),
                         ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset - load_data.datasets[j].pos));
              ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset =
                (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset - load_data.datasets[j].pos) + pos;
            }
          }
        }
#ifdef ZB_ENABLE_ZGP
        /* Move offsets for ZGP sink/proxy tables */
        else if (j == ZB_NVRAM_DATASET_GP_SINKT)
        {
          zb_nvram_update_zgp_sink_tbl_offset(next_page, load_data.datasets[j].pos, pos);
        }
        else if (j == ZB_NVRAM_DATASET_GP_PRPOXYT)
        {
          zb_nvram_update_zgp_proxy_tbl_offset(next_page, load_data.datasets[j].pos, pos);
        }
#endif /* ZB_ENABLE_ZGP */
#endif  /* APP_ONLY_NVRAM */

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
        if (j == ZB_NVRAM_INSTALLCODES)
        {
          zb_uindex_t i;
          zb_uindex_t n = 0;

          TRACE_MSG(TRACE_COMMON1, "migrate ZB_NVRAM_INSTALLCODES", (FMT__0));

          for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; i++)
          {
            if (ZB_AIB().installcodes_table[i].nvram_offset != 0)
            {
              ZB_AIB().installcodes_table[i].nvram_page = next_page;
              ZB_AIB().installcodes_table[i].nvram_offset = pos + n * sizeof(zb_aps_installcode_nvram_t);
              n++;
            }
          }
        }
#endif /* ZB_COORDINATOR_ROLE && ZB_SECURITY_INSTALLCODES */

        pos += hdr.data_len;
      } /* if tl != reserved */
    }   /* for datasets */
  }     /* if source page is not empty */
  else
  {
    TRACE_MSG(TRACE_COMMON1, "Error reading datasets on curr page %d", (FMT__D, status));
  }

  zb_osif_nvram_flush();

  ZB_OSIF_GLOBAL_LOCK();
  ZB_NVRAM().current_page = next_page;
  ZB_NVRAM().current_pos = pos;
  ZB_OSIF_GLOBAL_UNLOCK();
  TRACE_MSG(TRACE_COMMON1, "New page %hd pos %ld",
            (FMT__H_L, ZB_NVRAM().current_page, ZB_NVRAM().current_pos));

  /* erase old page */
  zb_nvram_erase_if_not_erased(page);

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_migrate_current_page", (FMT__0));
  return;
}

zb_ret_t zb_nvram_write_start(zb_nvram_dataset_hdr_t *hdr)
{
  zb_ret_t ret;

  if (ZB_NVRAM().current_pos + hdr->data_len + sizeof(zb_nvram_dataset_tail_t) >= zb_get_nvram_page_length())
  {
    zb_nvram_migrate_current_page();
  }
#ifndef ZB_NO_NVRAM_VER_MIGRATION
  else if (ZB_NVRAM().pages[ZB_NVRAM().current_page].nvram_ver != ZB_NVRAM_VERSION)
  {
    migrate_curr_page_to_new_version();
  }
#endif

  ZB_NVRAM().pages[ZB_NVRAM().current_page].erased = 0;
  ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INC(ZB_NVRAM().current_time_label);
  hdr->time_label = ZB_NVRAM().current_time_label;

  TRACE_MSG(TRACE_COMMON1, "dataset header type %d time %d length %d",
            (FMT__H_D_D, hdr->data_set_type, hdr->time_label, hdr->data_len));

  /* write entire ds hdr now. Successful DS write will be marked by TL at its tail. */
  ret = zb_osif_nvram_write(ZB_NVRAM().current_page,
                            ZB_NVRAM().current_pos,
                            (zb_uint8_t*)hdr,
                            (zb_uint16_t)sizeof(zb_nvram_dataset_hdr_t));

  if (hdr->data_set_type != (zb_uint16_t)ZB_NVRAM_RESERVED &&
      hdr->data_set_type != (zb_uint16_t)ZB_IB_COUNTERS &&
      hdr->data_set_type != (zb_uint16_t)ZB_NVRAM_APP_DATA1 &&
      hdr->data_set_type != (zb_uint16_t)ZB_NVRAM_APP_DATA2 &&
      hdr->data_set_type != (zb_uint16_t)ZB_NVRAM_APP_DATA3 &&
      hdr->data_set_type != (zb_uint16_t)ZB_NVRAM_APP_DATA4)
  {
    ZB_NVRAM().empty = 0;
  }
  return ret;
}


zb_ret_t zb_nvram_write_end(zb_nvram_dataset_hdr_t *hdr)
{
  zb_ret_t ret;

  /*
    Finished data set body write. Write time label to mark data set ready.
    If we crashed somewhere in the middle, this data set will have non-empty
    length but reserved timestamp, so it will be skipped during load.
  */
  ret = nvram_write_dataset_tail(ZB_NVRAM().current_page,
                                 ZB_NVRAM().current_pos, hdr->data_len, hdr->time_label);
  ZB_ASSERT(ret == RET_OK);
  if (ret == RET_OK)
  {
    ZB_NVRAM().current_pos += hdr->data_len;

    /* In the current (inefficient) migration implementation we are sure current
     * page has data set types info, so it is safe to erase next page. */
    {
      /* Start erase immediatly but asynchronously, so page will be ready to
       * write when we need it next time. At some platforms it will be
       * synchronous anyway... */
      zb_uint8_t next_page = (ZB_NVRAM().current_page+1U) % zb_get_nvram_page_count();
      if (!ZB_U2B(ZB_NVRAM().pages[next_page].erased)
          && !ZB_U2B(ZB_NVRAM().pages[next_page].erase_in_progress))
      {
        /* Initiate async erase operation for the next page and mark page immediatly as erased.
         * Suppose any nvram operation wait for the previous operation complete,
         * so nvram read/write will wait for erase complete if necessary.
         */
        ZB_NVRAM().pages[next_page].erase_in_progress = 1;
        ret = zb_osif_nvram_erase_async(next_page);
#ifdef ZB_PLATFORM_CORTEX_M3
        zb_osif_nvram_wait_for_last_op();
#endif
      }
    }

    TRACE_MSG(TRACE_COMMON1, "new page %hd pos %ld", (FMT__H_L, ZB_NVRAM().current_page, ZB_NVRAM().current_pos));

    if (ZB_NVRAM().transaction_ongoing != ZB_NVRAM_TRANSACTION_ONGOING)
    {
      /* Flush if required on this platform */
      zb_osif_nvram_flush();
    }
  }

  return ret;
}


#ifdef ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION
void zb_nvram_transaction_radio_off(void)
{
  if (ZB_TRANSCEIVER_GET_RX_ON_OFF() == ZB_TRUE)
  {
    /* Enter sleep mode, so flash operations will not be interrupted by radio RX. */
    ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_FALSE);
  }
}

void zb_nvram_transaction_radio_on(void)
{
  /* Restore RX state for non-sleepy devices. */
  if (ZB_TRANSCEIVER_RADIO_ON_OFF() == ZB_FALSE)
  {
    ZB_TRANSCEIVER_SET_RX_ON_OFF(ZB_PIB_RX_ON_WHEN_IDLE());
  }
}
#endif /* ZB_NVRAM_RADIO_OFF_DURING_TRANSACTION */


void zb_nvram_transaction_start(void)
{
  ZB_NVRAM_TRANSACTION_RADIO_OFF();
  ZB_NVRAM().transaction_ongoing = ZB_NVRAM_TRANSACTION_ONGOING;
}


void zb_nvram_transaction_commit(void)
{
  zb_osif_nvram_flush();
  ZB_NVRAM().transaction_ongoing = ZB_NVRAM_TRANSACTION_AUTOCOMMIT;
  ZB_NVRAM_TRANSACTION_RADIO_ON();
}


zb_ret_t zb_nvram_write_dataset(zb_nvram_dataset_types_t t)
{
  zb_ret_t ret = RET_ERROR;
  zb_nvram_dataset_hdr_t hdr = {0};
  zb_uint16_t dataset_length;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_dataset %hd", (FMT__H, (zb_uint8_t)t));
  ZB_ASSERT(t <= ZB_NVRAM_DATASET_NUMBER);
  do
  {

    if (!zb_nvram_dataset_is_supported(t))
    {
      TRACE_MSG(TRACE_ERROR, "invalid dataset type: %d", (FMT__D, t));
      ret = RET_INVALID_PARAMETER;
      break;
    }

    /* Can't write before initial load */
#ifndef APP_ONLY_NVRAM
    if (!ZB_U2B(ZB_AIB().aps_use_nvram))
    {
      TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_dataset: do not use nvram", (FMT__0));
      break;
    }
    ZB_ASSERT(ZB_NVRAM().inited);
#endif /* #ifndef APP_ONLY_NVRAM */

    /* init the dataset header */
    hdr.data_len = 0;
    hdr.data_set_type = (zb_uint16_t)t;
    /* update length by real situation */
    /* MISRA Rule 1.3i - Unspecified order of evaluation. Added 'dataset_length' variable. */
    dataset_length = zb_nvram_get_length_dataset(&hdr);
    hdr.data_len = dataset_length;

    hdr.data_set_version = 0;
    hdr.data_set_version = zb_nvram_get_dataset_version(hdr.data_set_type);

    ZB_OSIF_GLOBAL_LOCK();
    ret = zb_nvram_write_start(&hdr);

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00008,0} */
    if (ret == RET_OK)
    {
      zb_nvram_position_t pos_info;

      pos_info.page = ZB_NVRAM().current_page;
      pos_info.pos = ZB_NVRAM().current_pos + SIZE_DS_HEADER;
      pos_info.payload_length = hdr.data_len - (zb_uint16_t)sizeof(hdr);
      ret = write_dataset_body_by_type((zb_uint8_t)t, &pos_info);
      ret = zb_nvram_write_end(&hdr);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "nvram_write_start error %hd", (FMT__H, ret));
      break;
    }

  } while (0);
  ZB_OSIF_GLOBAL_UNLOCK();

  if (ret != RET_OK)
  {
    ZB_NVRAM().current_pos += (hdr.data_len);
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_dataset, ret %d", (FMT__H, ret));

  /* TODO: handle the error code in the place where this function is called
           or in some other error handler */
  ZB_ASSERT(ret == RET_OK);

  return ret;
}


void zb_nvram_write_empty_dataset(zb_nvram_dataset_types_t t)
{
  zb_ret_t ret;
  zb_nvram_dataset_hdr_t hdr;
  zb_size_t hdr_datalen;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_empty_dataset %d", (FMT__H, t));

#ifndef APP_ONLY_NVRAM
  if (!ZB_U2B(ZB_AIB().aps_use_nvram))
  {
    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_empty_dataset: do not use nvram", (FMT__0));
    return;
  }
#endif

  hdr_datalen = SIZE_DS_HEADER + sizeof(zb_nvram_dataset_tail_t);
  hdr.data_len = (zb_uint16_t)hdr_datalen;
  hdr.data_set_type = (zb_uint16_t)t;

  ret = zb_nvram_write_start(&hdr);
  TRACE_MSG(TRACE_COMMON1, "nvram_write_start ret %hd", (FMT__H, ret));

  if (ret == RET_OK)
  {
    ret = zb_nvram_write_end(&hdr);
  }

  if (ret != RET_OK)
  {
    ZB_NVRAM().current_pos += hdr.data_len;
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_empty_dataset", (FMT__0));
}

static zb_bool_t zb_nvram_dataset_is_supported(zb_nvram_dataset_types_t ds)
{
  zb_bool_t ret;

  switch(ds)
  {
    case ZB_NVRAM_COMMON_DATA:
    case ZB_NVRAM_INSTALLCODES:
#if defined ZB_ENABLE_HA
    case ZB_NVRAM_HA_DATA:
#endif /*defined ZB_ENABLE_HA*/
    case ZB_NVRAM_DATASET_GP_SINKT:
    case ZB_NVRAM_DATASET_GP_APP_TBL:
    case ZB_NVRAM_DATASET_GP_PRPOXYT:
    case ZB_NVRAM_DATASET_GP_CLUSTER:
#if defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)
    case ZB_NVRAM_ZCL_REPORTING_DATA:
#endif /* defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING) */
#if defined ZB_ENABLE_ZCL
    case ZB_NVRAM_APS_SECURE_DATA:
#endif /* defined ZB_ENABLE_ZCL */
    case ZB_NVRAM_APS_BINDING_DATA:
    case ZB_NVRAM_APS_GROUPS_DATA:
#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER)
    case ZB_NVRAM_ZCL_WWAH_DATA:
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) */
#if defined ZB_STORE_COUNTERS
    case ZB_IB_COUNTERS:
#endif  /* defined ZB_STORE_COUNTERS */
    case ZB_NVRAM_APP_DATA1:
    case ZB_NVRAM_APP_DATA2:
    case ZB_NVRAM_APP_DATA3:
    case ZB_NVRAM_APP_DATA4:
    case ZB_NVRAM_ADDR_MAP:
    case ZB_NVRAM_NEIGHBOUR_TBL:
#ifdef ZDO_DIAGNOSTICS
    case ZB_NVRAM_ZDO_DIAGNOSTICS_DATA:
#endif /* ZDO_DIAGNOSTICS */
      ret = ZB_TRUE;
      break;

    default:
      ret = zb_nvram_custom_ds_is_supported((zb_uint16_t)ds);
      break;
  }

  TRACE_MSG(TRACE_COMMON2, "zb_nvram_dataset_is_supported ds %hd ret %hd", (FMT__H_H, ds, ret));

  return ret;
}

void zb_nvram_clear(void)
{
  zb_uindex_t i;
  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_clear", (FMT__0));

  zb_nvram_transaction_start();

  for (i=0; i<(zb_uindex_t)ZB_NVRAM_DATASET_NUMBER; i++)
  {
    /* Looks like a bit spooky, but the main idea is to save all empty
     * datasets except RESERVED, _IB_COUNTERS (it contains NWK outgoing
     * counter), APP_DATAx and unsupported ones */
    if (zb_nvram_dataset_is_supported((zb_nvram_dataset_types_t)i)
        && i != (zb_uindex_t)ZB_NVRAM_RESERVED
        && i != (zb_uindex_t)ZB_IB_COUNTERS
        && i != (zb_uindex_t)ZB_NVRAM_APP_DATA1
        && i != (zb_uindex_t)ZB_NVRAM_APP_DATA2
        && i != (zb_uindex_t)ZB_NVRAM_APP_DATA3
        && i != (zb_uindex_t)ZB_NVRAM_APP_DATA4
      )
    {
      TRACE_MSG(TRACE_COMMON1, "save empty dataset %hd", (FMT__H, i));
      zb_nvram_write_empty_dataset((zb_nvram_dataset_types_t)i);

#ifndef APP_ONLY_NVRAM
      /* If dataset is ZB_NVRAM_APS_SECURE_DATA, reset corresponding structure in RAM */
      if (i == (zb_uindex_t)ZB_NVRAM_APS_SECURE_DATA)
      {
#ifdef ZB_CERTIFICATION_HACKS
        if(ZB_CERT_HACKS().aps_device_key_pair_storage_donotclear == 1)
        {
          continue;
        }
#endif
        ZB_BZERO(&ZB_AIB().aps_device_key_pair_storage, sizeof(ZB_AIB().aps_device_key_pair_storage));
        ZB_RESYNC_CFG_MEM();
        ZB_AIB().aps_device_key_pair_storage.cached_i = (zb_uint8_t)-1;
      }
#endif
    }
  }
  ZB_NVRAM().empty = 1;

  zb_nvram_transaction_commit();

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_clear", (FMT__0));
}

/** @internal
 *  NWRAM command
    @{
*/

#ifndef APP_ONLY_NVRAM


#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
/**
   Read installcodes dataset and create installcodes hash data structure
 */
void zb_nvram_read_installcodes_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_aps_installcode_nvram_t rec;
  zb_uint_t i, count, idx, n;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_SECUR1, "> zb_nvram_read_installcodes_dataset %d pos %ld length %d nvram_ver %d",
      (FMT__H_L_D_D, page, pos, length, nvram_ver));

  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  count = length / sizeof(rec);

  TRACE_MSG(TRACE_SECUR3, "%d records", (FMT__D, count));

  for (i = 0 ; i < count && ret == RET_OK ; i++)
  {
    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&rec, sizeof(rec));
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00007,2} */
    if (ret == RET_OK)
    {
      idx = zb_64bit_hash(rec.device_address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
      TRACE_MSG(TRACE_SECUR3, "page %d pos %d rx addr " TRACE_FORMAT_64 ", hash idx %d", (FMT__D_D_A_D, page, pos, TRACE_ARG_64(rec.device_address), idx));
      n = 0;
      /* Seek for free slot */
      while (ZB_AIB().installcodes_table[idx].nvram_offset != 0 && n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
      {
        idx = (idx + 1) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
      }
      if (ZB_AIB().installcodes_table[idx].nvram_offset != 0)
      {
        /* no free space */
        TRACE_MSG(TRACE_ERROR, "No free space for installcode load", (FMT__0));
        ret = RET_NO_MEMORY;
      }
      else
      {
        ZB_AIB().installcodes_table[idx].nvram_offset = pos;
        ZB_AIB().installcodes_table[idx].nvram_page = page;
        TRACE_MSG(TRACE_SECUR3, "for i.c. hash idx %d nvram_offset %d nvram_page %d; addr " TRACE_FORMAT_64 " i.c. " TRACE_FORMAT_128,
                  (FMT__D_D_D_A_B, idx, ZB_AIB().installcodes_table[idx].nvram_offset, ZB_AIB().installcodes_table[idx].nvram_page,
                   TRACE_ARG_64(rec.device_address), TRACE_ARG_128(rec.installcode)));
      }
    }
    pos += sizeof(rec);
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_nvram_read_installcodes_dataset", (FMT__0));
}

zb_ret_t zb_nvram_write_installcodes_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_aps_installcode_nvram_t rec = {0};
  zb_ret_t ret = RET_OK;
  zb_int_t idx;
  zb_uint_t i, n;

  TRACE_MSG(TRACE_SECUR1, "> zb_nvram_write_installcodes_dataset %d pos %ld",
      (FMT__H_L, page, pos));

  /* -1 to do not match with any offset in the hash */
  idx = -1;
  if (ZB_AIB().installcode_to_add
      && !ZB_AIB().installcode_to_add->do_update)
  {
    idx = zb_64bit_hash(ZB_AIB().installcode_to_add->address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
    n = 0;
    /* Seek for free slot */
    while (ZB_AIB().installcodes_table[idx].nvram_offset != 0 && n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
    {
      idx = (idx + 1) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
      n++;
    }
    if (ZB_AIB().installcodes_table[idx].nvram_offset != 0)
    {
      TRACE_MSG(TRACE_SECUR1, "No free hash slot!", (FMT__0));
      idx = -1;
    }
    else
    {
      TRACE_MSG(TRACE_SECUR3, "got free hash slot %d", (FMT__D, idx));
    }
  }

  for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE && ret == RET_OK ; ++i)
  {
    if ((zb_int_t)i == idx)
    {
      /* This is the record we want to add, so fill it from the user's data */
      ZB_IEEE_ADDR_COPY(rec.device_address, ZB_AIB().installcode_to_add->address);
      ZB_MEMCPY(rec.installcode, ZB_AIB().installcode_to_add->ic, sizeof(rec.installcode));
      rec.options = ZB_AIB().installcode_to_add->type & 0x3;
      TRACE_MSG(TRACE_SECUR1, "idx %d add entry : installcode "  TRACE_FORMAT_128 " for address " TRACE_FORMAT_64, (FMT__D_B_A, idx, TRACE_ARG_128(rec.installcode), TRACE_ARG_64(rec.device_address)));
    }
    else if (ZB_AIB().installcodes_table[i].nvram_offset != 0)
    {
      /* Read record from nvram using address stored in installcodes hash
         Seems, record migration will not produce any problems because old page
         still exists during migration.
       */
      ret = zb_osif_nvram_read(ZB_AIB().installcodes_table[i].nvram_page,
                               ZB_AIB().installcodes_table[i].nvram_offset,
                               (zb_uint8_t *)&rec,
                               sizeof(rec));
      ZB_ASSERT(ret == RET_OK);
      if (ZB_AIB().installcode_to_add
          && ZB_AIB().installcode_to_add->do_update
          && ZB_IEEE_ADDR_CMP(rec.device_address, ZB_AIB().installcode_to_add->address))
      {
        ZB_MEMCPY(rec.installcode, ZB_AIB().installcode_to_add->ic, sizeof(rec.installcode));
        rec.options = ZB_AIB().installcode_to_add->type & 0x3;
        TRACE_MSG(TRACE_SECUR1, "update installcode i %d to installcode "  TRACE_FORMAT_128 " for address " TRACE_FORMAT_64, (FMT__D_B_A, i, TRACE_ARG_128(rec.installcode), TRACE_ARG_64(rec.device_address)));
      }
      else
      {
        TRACE_MSG(TRACE_SECUR1, "read by i %d : installcode "  TRACE_FORMAT_128 " for address " TRACE_FORMAT_64, (FMT__D_B_A, i, TRACE_ARG_128(rec.installcode), TRACE_ARG_64(rec.device_address)));
      }
    }
    else
    {
      continue;
    }
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, pos, (zb_uint8_t *)&rec, sizeof(rec));
    /* Update installcodes hash */
    ZB_AIB().installcodes_table[i].nvram_page = page;
    ZB_AIB().installcodes_table[i].nvram_offset = pos;
    TRACE_MSG(TRACE_SECUR1, "idx %d: update as nvram_page %d nvram_offset %d", (FMT__D_D_D, i, ZB_AIB().installcodes_table[i].nvram_page, ZB_AIB().installcodes_table[i].nvram_offset));
    pos += sizeof(rec);
  }

  TRACE_MSG(TRACE_SECUR1, "< zb_nvram_write_installcodes_dataset %hd", (FMT__H, ret));
  return ret;
}


zb_uint16_t zb_nvram_installcodes_length(void)
{
  zb_uint_t n = 0;
  zb_uint_t n_free = 0;
  zb_uint_t i;
  for (i = 0 ; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE ; ++i)
  {
    n += (ZB_AIB().installcodes_table[i].nvram_offset != 0);
    /* Check for free slots: if no free slots, dataset write will skip installcode */
    n_free += (ZB_AIB().installcodes_table[i].nvram_offset == 0);
  }

  return (n + (ZB_AIB().installcode_to_add != NULL && !ZB_AIB().installcode_to_add->do_update && n_free != 0)) * sizeof(zb_aps_installcode_nvram_t);
}

#endif /* ZB_COORDINATOR_ROLE && ZB_SECURITY_INSTALLCODES*/


#ifdef ZB_USE_NVRAM
/**
   Read aps_keypair dataset and create hash data structure
 */
void zb_nvram_read_aps_keypair_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t length, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_aps_device_key_pair_nvram_t rec;
  zb_aps_secur_common_data_t common_data;
  zb_uint_t i, count, idx, n;
  zb_ret_t ret;

  TRACE_MSG(TRACE_COMMON1, "> zb_nvram_read_aps_keypair_dataset %d pos %ld length %d nvram_ver %d",
            (FMT__H_L_D_D, page, pos, length, nvram_ver));

  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  ZB_BZERO(&common_data, sizeof(zb_aps_secur_common_data_t));

  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&common_data, (zb_uint16_t)sizeof(common_data));
  pos += sizeof(common_data);

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,3} */
  if (ret == RET_OK)
  {
    ZB_AIB().coordinator_version = common_data.coordinator_version;
  }

  ZB_BZERO(&ZB_AIB().aps_device_key_pair_storage, sizeof(ZB_AIB().aps_device_key_pair_storage));
  ZB_RESYNC_CFG_MEM();

  count = length / sizeof(zb_aps_device_key_pair_nvram_t);

  TRACE_MSG(TRACE_COMMON1, "%d records", (FMT__D, count));

  for (i = 0 ; i < count && ret == RET_OK ; i++)
  {
    /* now use fixed-size entries. It is not optimal. */
    n = 0;
    ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(zb_aps_device_key_pair_nvram_t));

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00007,4} */
    if (ret == RET_OK)
    {
      idx = zb_64bit_hash(rec.device_address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
      TRACE_MSG(TRACE_COMMON1, "page %d pos %d rx addr hash idx %d", (FMT__D_D_D, page, pos, idx));

      while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE
             && ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset != 0U)
      {
        idx = (idx + 1U) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
      }

      if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset != 0U)
      {
        /* no free space */
        TRACE_MSG(TRACE_ERROR, "No free space for zp tbl load", (FMT__0));
        break;
      }
      else
      {
        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset = pos;
        TRACE_MSG(TRACE_COMMON1, "for aps keypair ent hash idx %d nvram_offset %d page %d",
                  (FMT__D_D_D, idx, pos, page));
      }
    } /* if ok */
    pos += sizeof(zb_aps_device_key_pair_nvram_t);
  } /* for */
  ZB_AIB().aps_device_key_pair_storage.nvram_page = page;

  TRACE_MSG(TRACE_COMMON1, "< zb_nvram_read_aps_keypair_dataset", (FMT__0));
}


zb_ret_t zb_nvram_write_aps_keypair_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_aps_device_key_pair_nvram_t rec;
  zb_aps_secur_common_data_t common_data = {0};
  zb_ret_t ret;
  zb_uint_t i;

  TRACE_MSG(TRACE_COMMON1, "> zb_nvram_write_aps_keypair_dataset %d pos %ld",
      (FMT__H_L, page, pos));

  common_data.coordinator_version = ZB_AIB().coordinator_version;
  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&common_data, (zb_uint16_t)sizeof(common_data));
  pos += sizeof(common_data);

  for (i = 0; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE && ret == RET_OK ; ++i)
  {
    if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset == ZB_APS_DEVICE_KEY_PAIR_CACHED
      )
    {
      ZB_MEMCPY(&rec, &ZB_AIB().aps_device_key_pair_storage.cached, sizeof(rec));
    }
    else if (ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != 0U)
    {
      /* Read record from nvram using address stored in installcodes hash
         Hope, record migration will not produce any problems because old page
         still exists during migration.
       */
      ret = zb_osif_nvram_read(ZB_AIB().aps_device_key_pair_storage.nvram_page, ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset,
                               (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(rec));
    }
    else
    {
      continue;
    }

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00007,5} */
    if (ret == RET_OK)
    {
      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(rec));
      ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset = pos;
      TRACE_MSG(TRACE_COMMON1, "idx %d: update as nvram_page %d nvram_offset %d", (FMT__D_D_D, i, page, ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset));
      pos += sizeof(rec);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "idx %d: nvram read error!", (FMT__D, i));
      break;
    }
  } /* for */
  ZB_AIB().aps_device_key_pair_storage.nvram_page = page;

  TRACE_MSG(TRACE_COMMON1, "< zb_nvram_write_aps_keypair_dataset %hd", (FMT__H, ret));
  return ret;
}

zb_uint16_t zb_nvram_aps_keypair_length()
{
  zb_size_t ret;
  zb_uint_t n = 0;
  zb_uint_t i;

  for (i = 0 ; i < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE ; ++i)
  {
    n += ZB_B2U(ZB_AIB().aps_device_key_pair_storage.key_pair_set[i].nvram_offset != 0U);
  }
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_aps_keypair_length n %d ret %d", (FMT__D_D, n, (n * sizeof(zb_aps_device_key_pair_nvram_t))+ sizeof(zb_aps_secur_common_data_t)));

  ret = n;
  ret *= sizeof(zb_aps_device_key_pair_nvram_t);
  ret += sizeof(zb_aps_secur_common_data_t);

  return (zb_uint16_t)ret;
}

static zb_ret_t zb_nvram_read_aps_binding_dataset_v1(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len)
{
  zb_ret_t ret = RET_ERROR;
  zb_nvram_dataset_binding_v1_t ds = { 0 };

  ZVUNUSED(len);
  TRACE_MSG(TRACE_ZCL2, "> zb_nvram_read_aps_binding_dataset_v1", (FMT__0));
  TRACE_MSG(TRACE_ZCL1, "old_aps_binding_ds_size == %d", (FMT__D, sizeof(zb_nvram_dataset_binding_v1_t)));

  /* calculate APS binding table size according old version:
   * /branches/nordic_r21/devel -r42825 (table format is changed in -r42826);
   * SDK version 3.1.0.59;
   */
  if (sizeof(zb_nvram_dataset_binding_v1_t) == len)
  {
    ret = zb_nvram_read_data(page,
                             pos,
                             (zb_uint8_t*)&ds,
                             (zb_uint16_t)sizeof(zb_nvram_dataset_binding_v1_t));

    if (RET_OK != ret
        || ds.src_n_elements > ZB_APS_SRC_BINDING_TABLE_SIZE_OLD
        || ds.dst_n_elements > ZB_APS_DST_BINDING_TABLE_SIZE_OLD)
    {
      ret = RET_ERROR;
      TRACE_MSG(TRACE_ERROR, "invalid APS binding table sizes from ds_v1!", (FMT__0));
    }

    if (RET_OK == ret)
    {
      zb_uint8_t i;

      ZG->aps.binding.src_n_elements = ds.src_n_elements;
      ZG->aps.binding.dst_n_elements = ds.dst_n_elements;
#ifdef SNCP_MODE
      ZG->aps.binding.remote_bind_offset = ds.remote_bind_offset;
#endif

      ZB_MEMCPY(ZG->aps.binding.src_table, ds.src_table, ds.src_n_elements * sizeof(zb_aps_bind_src_table_t));

      for (i = 0; i < ds.dst_n_elements; ++i)
      {
        ZG->aps.binding.dst_table[i].u.group_addr = ds.dst_table[i].u.group_addr;
        ZG->aps.binding.dst_table[i].dst_addr_mode = ds.dst_table[i].dst_addr_mode;
        ZG->aps.binding.dst_table[i].src_table_index = ds.dst_table[i].src_table_index;
      }
    }
  }
#ifdef ZB_NVRAM_APS_BINDING_TABLE_SPECIFIC_MIGRATION
  else
  {
    ret = RET_NOT_FOUND;
    TRACE_MSG(TRACE_ERROR, "invalid ds_v1 size! maybe it's ds_v2 with old DS version", (FMT__0));
  }
#endif /* ZB_NVRAM_APS_BINDING_TABLE_SPECIFIC_MIGRATION */

  TRACE_MSG(TRACE_ZCL2, "< zb_nvram_read_aps_binding_dataset_v1", (FMT__0));
  return ret;
}

static zb_ret_t zb_nvram_read_aps_binding_dataset_v2(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint32_t offset = 0;
  zb_nvram_dataset_binding_v2_t ds = { 0 };

  TRACE_MSG(TRACE_ZCL2, "> zb_nvram_read_aps_binding_dataset_v2", (FMT__0));
  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,6} */
  if (RET_OK == ret)
  {
    zb_uint16_t nvram_aps_binding_tables_size;
    zb_uint16_t expected_aps_binding_tables_size;

    offset += sizeof(ds);
    ZG->aps.binding.src_n_elements = ds.src_n_elements;
    ZG->aps.binding.dst_n_elements = ds.dst_n_elements;
#ifdef SNCP_MODE
    ZG->aps.binding.remote_bind_offset = ds.remote_bind_offset;
#endif
    expected_aps_binding_tables_size = zb_nvram_aps_binding_tables_length();

    /* because 'len' contains zb_nvram_dataset_binding_v2_t */
    nvram_aps_binding_tables_size = len - (zb_uint16_t)sizeof(ds);

  /* Protect ourselves: emergency return if expected and actual
     * length are not equals - this means that tables sizes
   * (ZB_APS_SRC_BINDING_TABLE_SIZE, ZB_APS_DST_BINDING_TABLE_SIZE,
   * ZB_APS_BIND_TRANS_TABLE_SIZE) or binding table structure (with
   * nested structures) have been changed.
   * In at this point, data is supposed to be corrupted and no chance
   * to restore it correctly.
   */
    if (nvram_aps_binding_tables_size != expected_aps_binding_tables_size
        || ds.src_n_elements > ZB_APS_SRC_BINDING_TABLE_SIZE
        || ds.dst_n_elements > ZB_APS_DST_BINDING_TABLE_SIZE)
  {
      TRACE_MSG(TRACE_ERROR,
                "ERROR: unexpected binding dataset lenght; size from nvram %d, expected size %d",
                (FMT__D_D, nvram_aps_binding_tables_size, expected_aps_binding_tables_size));

      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret && ds.src_n_elements != 0U)
  {
    zb_uint16_t size_src_n_elements = ds.src_n_elements * (zb_uint16_t)sizeof(zb_aps_bind_src_table_t);
    ret = zb_nvram_read_data(page,
                             pos + offset,
                             (zb_uint8_t*)&ZG->aps.binding.src_table[0],
                             size_src_n_elements);
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,7} */
  if (RET_OK == ret)
  {
    zb_uint32_t i;
    zb_aps_bind_dst_table_t dst_table_entry = { 0 };

    offset += (ds.src_n_elements * sizeof(zb_aps_bind_src_table_t));

    for (i = 0; i < ds.dst_n_elements; ++i)
    {
    ret = zb_nvram_read_data(page,
                             pos + offset,
                               (zb_uint8_t*)&dst_table_entry,
                               (zb_uint16_t)sizeof(zb_aps_bind_dst_table_t));

      if (RET_OK == ret)
      {
        ZG->aps.binding.dst_table[i].u.group_addr = dst_table_entry.u.group_addr;
        ZG->aps.binding.dst_table[i].dst_addr_mode = dst_table_entry.dst_addr_mode;
        ZG->aps.binding.dst_table[i].src_table_index = dst_table_entry.src_table_index;
#ifdef SNCP_MODE
        ZG->aps.binding.dst_table[i].id = dst_table_entry.id;
#endif
        offset += sizeof(zb_aps_bind_dst_table_t);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "could not read APS dst-binding table", (FMT__0));
        break;
      }
    }
  }

  if (RET_OK != ret)
  {
    ZG->aps.binding.src_n_elements = 0;
    ZG->aps.binding.dst_n_elements = 0;
#ifdef SNCP_MODE
    ZG->aps.binding.remote_bind_offset = 0xff; /*0xff means not configured*/
#endif
  }

  TRACE_MSG(TRACE_ZCL2, "< zb_nvram_read_aps_binding_dataset_v2", (FMT__0));
  return ret;
}

static void zb_nvram_read_aps_binding_dataset_continue(void)
{
  zb_uindex_t i,j;

  TRACE_MSG(TRACE_ZCL3, "> zb_nvram_read_aps_binding_dataset_continue", (FMT__0));

  /* In case of configurable mem this code zeroes &ZG->aps.binding.dst_table[i].trans_index, so run it before ZB_RESYNC_CFG_MEM() call. */
	/* Randy modify to lock src address correctly*/
  for (i = 0; i < ZG->aps.binding.src_n_elements; ++i)
  {
    for (j = 0; j < ZG->aps.binding.dst_n_elements; ++j)
    {
      if (zb_address_lock(ZG->aps.binding.src_table[i].src_addr) != RET_OK)
      {
        /* Looks like addr_table is corrupted or there is mismatch in addr_table and
        * binding_table. Skip this entry and try to continue... */
        TRACE_MSG(TRACE_ERROR, "WARNING: clear bind src connected with addr_map %hd", (FMT__H, ZG->aps.binding.src_table[i].src_addr));
        ZB_BZERO(&ZG->aps.binding.src_table[i], sizeof(zb_aps_bind_src_table_t));
      }
      if (ZG->aps.binding.dst_table[j].dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
      {
        if (zb_address_lock(ZG->aps.binding.dst_table[j].u.long_addr.dst_addr) != RET_OK)
        {
          /* Looks like addr_table is corrupted or there is mismatch in addr_table and
          * binding_table. Skip this entry and try to continue... */
          TRACE_MSG(TRACE_ERROR, "WARNING: clear bind dst connected with addr_map %hd", (FMT__H, ZG->aps.binding.dst_table[j]));
          ZB_BZERO(&ZG->aps.binding.dst_table[j], sizeof(zb_aps_bind_dst_table_t));
        }
      }
    }
  }

  /* re-assign trans_table in zb_aps_binding_table_t and trans_index in zb_aps_bind_dst_table_t */
  ZB_RESYNC_CFG_MEM();
  /* Seems like we can not have binding transactions now, at start. */
  ZB_BZERO(ZG->aps.binding.trans_table, ZB_APS_BIND_TRANS_TABLE_SIZE);

  /* Lock address references */
  /* NK: This will not work if addr_map is not stored (wrong addr_ref addressing).
     Put addr_map and aps_binding storing under one define.
     Maybe it is also needed for neighbor_table storing. */
//  for (i = 0; i < ZG->aps.binding.src_n_elements; ++i)
//  {
//    if (zb_address_lock(ZG->aps.binding.src_table[i].src_addr) != RET_OK)
//    {
//      /* Looks like addr_table is corrupted or there is mismatch in addr_table and
//       * binding_table. Skip this entry and try to continue... */
//      TRACE_MSG(TRACE_ERROR, "WARNING: clear bind src connected with addr_map %hd", (FMT__H, ZG->aps.binding.src_table[i].src_addr));
//      ZB_BZERO(&ZG->aps.binding.src_table[i], sizeof(zb_aps_bind_src_table_t));
//    }
//  }

  /* Clear trans_index in a way compatible with both configurable mem and static builds */
  for (i = 0 ; i < ZB_APS_DST_BINDING_TABLE_SIZE ; ++i)
  {
    ZB_BZERO(ZG->aps.binding.dst_table[i].trans_index, ZB_SINGLE_TRANS_INDEX_SIZE);
  }

  TRACE_MSG(TRACE_ZCL3, "< zb_nvram_read_aps_binding_dataset_continue", (FMT__0));
}

void zb_nvram_read_aps_binding_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZCL1,
            "> zb_nvram_read_aps_binding_dataset %hd pos %ld len %d nvram_ver %d",
            (FMT__H_L_D_D, page, pos, len, nvram_ver));
  TRACE_MSG(TRACE_INFO1, "ds_ver == %d", (FMT__D, ds_ver));

  ZVUNUSED(nvram_ver);

  if (ZB_NVRAM_APS_BINDING_DATA_DS_VER_1 == ds_ver)
  {
    ret = zb_nvram_read_aps_binding_dataset_v1(page, pos, len);
  }

  if (ZB_NVRAM_APS_BINDING_DATA_DS_VER_2 == ds_ver
      || RET_NOT_FOUND == ret)
  {
    ret = zb_nvram_read_aps_binding_dataset_v2(page, pos, len);
  }

  if (RET_OK == ret)
  {
    zb_nvram_read_aps_binding_dataset_continue();
  }
  else
  {
    TRACE_MSG(TRACE_ERROR,
              "ERROR: could not read the APS binding table from NVRAM, page %d pos %ld, ds_ver %d",
              (FMT__H_L_D, page, pos, ds_ver));
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_nvram_read_aps_binding_dataset", (FMT__0));
}

zb_ret_t zb_nvram_write_aps_binding_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret;
  zb_uint32_t offset = 0;
  zb_nvram_dataset_binding_v2_t ds;

  TRACE_MSG(TRACE_ZCL1, "> zb_nvram_write_aps_binding_dataset page %d pos %ld",
      (FMT__H_L, page, pos));

  ZB_BZERO(&ds, sizeof(ds));
  ds.src_n_elements = ZG->aps.binding.src_n_elements;
  ds.dst_n_elements = ZG->aps.binding.dst_n_elements;
#ifdef SNCP_MODE
  ds.remote_bind_offset = ZG->aps.binding.remote_bind_offset;
#endif
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,1} */
  if (RET_OK == ret)
  {
    zb_uint16_t size_zb_aps_bind_src_table = ds.src_n_elements * (zb_uint16_t)sizeof(zb_aps_bind_src_table_t);

    offset += sizeof(ds);
    if (size_zb_aps_bind_src_table > 0U)
    {
      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page,
                                pos + offset,
                                (zb_uint8_t*)&ZG->aps.binding.src_table[0],
                                size_zb_aps_bind_src_table);
    }
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,2} */
  if (RET_OK == ret)
  {
    zb_uint16_t size_zb_aps_bind_dst_table = ds.dst_n_elements * (zb_uint16_t)sizeof(zb_aps_bind_dst_table_t);

    offset += ds.src_n_elements * sizeof(zb_aps_bind_src_table_t);
    if (size_zb_aps_bind_dst_table > 0U)
    {
      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page,
                                pos + offset,
                                (zb_uint8_t*)&ZG->aps.binding.dst_table[0],
                                size_zb_aps_bind_dst_table);
    }
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,3} */
  if (RET_OK != ret)
  {
    TRACE_MSG(TRACE_ERROR,
              "ERROR: could not write the APS binding table to the NVRAM, page %d pos %ld",
              (FMT__H_L, page, pos));
  }
  /* uncomment this line if need to write something else from nvram */
  /* else { offset += (ds.dst_n_elements * sizeof(zb_aps_bind_dst_table_t)); } */

  TRACE_MSG(TRACE_ZCL1, "< zb_nvram_write_aps_binding_dataset", (FMT__0));
  /* Note: in case on configurable mem this code saves ptrs into
   * nvram. This is ok since pending aps transmissions are impossible
   * after reboot, so it cleared at nvram load. */
  return ret;
}

static zb_uint16_t zb_nvram_aps_binding_tables_length(void)
{
  zb_uint16_t ret;

  ret =  ZG->aps.binding.src_n_elements * (zb_uint16_t)sizeof(zb_aps_bind_src_table_t);
  ret += ZG->aps.binding.dst_n_elements * (zb_uint16_t)sizeof(zb_aps_bind_dst_table_t);

  return ret;
}

void zb_nvram_read_aps_groups_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_ret_t ret;
  zb_nvram_dataset_groups_hdr_t g_hdr;
  zb_uindex_t i = 0;

  TRACE_MSG(TRACE_ZCL1, "> zb_nvram_read_aps_groups_dataset %hd pos %ld len %d nvram_ver %d",
      (FMT__H_L_D_D, page, pos, len, nvram_ver));

  ZVUNUSED(len);
  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  ZB_BZERO(&g_hdr, sizeof(g_hdr));
  ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&g_hdr, (zb_uint16_t)sizeof(g_hdr));
  pos += sizeof(g_hdr);

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,8} */
  if (ret == RET_OK)
  {
    ZG->aps.group.n_groups = g_hdr.n_groups;

    while (i < ZG->aps.group.n_groups && ret == RET_OK)
    {
      ret = zb_osif_nvram_read(page, pos,
       (zb_uint8_t*)&ZG->aps.group.groups[i], (zb_uint16_t)sizeof(zb_aps_group_table_ent_t));
      ++i;
      pos += sizeof(zb_aps_group_table_ent_t);
    }
  }

  TRACE_MSG(TRACE_ZCL1, "< zb_nvram_read_aps_groups_dataset", (FMT__0));
}

zb_ret_t zb_nvram_write_aps_groups_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_dataset_groups_hdr_t g_hdr;
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZCL1, "> zb_nvram_write_aps_groups_dataset page %d pos %ld",
      (FMT__H_L, page, pos));

  g_hdr.n_groups = ZG->aps.group.n_groups;

  /* If we fail, trace is given and assertion is triggered */
  ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&g_hdr, (zb_uint16_t)sizeof(g_hdr));
  pos += sizeof(g_hdr);

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,4} */
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_ZCL1, "< zb_nvram_write_aps_groups_dataset", (FMT__0));
    if (g_hdr.n_groups > 0U)
    {
      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page, pos,
                                (zb_uint8_t*)ZG->aps.group.groups,
                                g_hdr.n_groups * (zb_uint16_t)sizeof(zb_aps_group_table_ent_t));
    }
  }
  return ret;
}

zb_uint16_t zb_nvram_aps_groups_length(void)
{
  zb_uint16_t ret;

  TRACE_MSG(TRACE_ZCL1, "zb_nvram_aps_groups_length %hd",
            (FMT__H, sizeof(zb_nvram_dataset_groups_hdr_t) +
          ZG->aps.group.n_groups * sizeof(zb_aps_group_table_ent_t)));

  ret =  (zb_uint16_t)sizeof(zb_nvram_dataset_groups_hdr_t);
  ret += ZG->aps.group.n_groups * (zb_uint16_t)sizeof(zb_aps_group_table_ent_t);

  return ret;
}

#endif /* ZB_USE_NVRAM */

#if defined ZB_STORE_COUNTERS
void zb_nvram_read_counters_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_nvram_dataset_counters_t ds;
  zb_ret_t ret;

  TRACE_MSG(TRACE_APS3, ">> zb_nvram_read_counters_dataset %hd pos %ld len %d nvram_ver %d",
      (FMT__H_L_D_D, page, pos, len, nvram_ver));

  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  /* [AV] in second version aib_cnt was simply added to the end of struct
     that is why it's enough to read "len" bytes to dataset
   */
  ZB_BZERO(&ds, sizeof(ds));

  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&ds, len);

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,9} */
  if  (ret == RET_OK)
  {
    TRACE_MSG(TRACE_APS1, "counter loaded", (FMT__0));
    ZB_NIB().outgoing_frame_counter = ds.nib_counter;
    ZB_AIB().outgoing_frame_counter = ds.aib_counter;
  }
  TRACE_MSG(TRACE_APS3, "<< zb_nvram_read_counters_dataset", (FMT__0));
}

/** @brief Write NIB Security counters to NVRAM */

zb_ret_t zb_nvram_write_counters_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_dataset_counters_t ds;

  TRACE_MSG(TRACE_APS3, ">> zb_nvram_write_counters_dataset %hd pos %ld",
      (FMT__H_L, page, pos));

  ZB_BZERO(&ds, sizeof(ds));

  ds.nib_counter = (ZB_NIB().outgoing_frame_counter / ZB_LAZY_COUNTER_INTERVAL + 1U) * ZB_LAZY_COUNTER_INTERVAL;
  ds.aib_counter = (ZB_AIB().outgoing_frame_counter / ZB_LAZY_COUNTER_INTERVAL + 1U) * ZB_LAZY_COUNTER_INTERVAL;

  /* If we fail, trace is given and assertion is triggered */
  return zb_nvram_write_data(page, pos, (zb_uint8_t*)&ds, (zb_uint16_t)sizeof(ds));
}
#endif


static zb_ret_t zb_nvram_read_addr_map_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_ret_t ret;
  zb_nvram_addr_map_hdr_t hdr;
  zb_uindex_t i;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_read_addr_map_dataset page %hd pos %ld len %d nvram_ver %d",
            (FMT__H_L_D_D, page, pos, len, nvram_ver));

  ZVUNUSED(len);
  ZVUNUSED(nvram_ver);
  ZVUNUSED(ds_ver);

  ZB_BZERO(&hdr, sizeof(zb_nvram_addr_map_hdr_t));

  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));
  if (ret == RET_OK && nvram_ver < ZB_NVRAM_VER_7_0)
  {
    pos += sizeof(zb_nvram_addr_map_hdr_v0_t);
  }
  else
  {
    pos += sizeof(zb_nvram_addr_map_hdr_t);
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,10} */
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_COMMON2, "Read header: number of recs %hd ver %hd",
              (FMT__H_H, hdr.addr_map_num, hdr.version));

    ZB_ASSERT(hdr.addr_map_num <= ZB_IEEE_ADDR_TABLE_SIZE);

#ifndef ZB_NO_NVRAM_VER_MIGRATION
    if (hdr.version == ZB_NVRAM_ADDR_MAP_DS_VER_0)
    {
      /* Check if the migration from v0 to v1 is needed and
       * implement it. Probably, both are obsolete. */
      zb_nvram_addr_map_rec_v0_t addr_map_rec;

      ZB_BZERO(&addr_map_rec, sizeof(zb_nvram_addr_map_rec_v0_t));

      for (i = 0; i < hdr.addr_map_num && ret == RET_OK; ++i)
      {
        ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&addr_map_rec,
                                 sizeof(zb_nvram_addr_map_rec_v0_t));
        pos += sizeof(zb_nvram_addr_map_rec_v0_t);

        TRACE_MSG(TRACE_COMMON3, "Read rec: idx %hd addr %d ieee" TRACE_FORMAT_64,
                  (FMT__H_D_A, addr_map_rec.index, addr_map_rec.addr, TRACE_ARG_64(addr_map_rec.ieee_addr)));

        ZB_ASSERT(!ZB_U2B(ZG->addr.addr_map[addr_map_rec.index].used));

        ZG->addr.addr_map[addr_map_rec.index].used = ZB_TRUE_U;
        ZG->addr.addr_map[addr_map_rec.index].addr = addr_map_rec.addr;
        zb_ieee_addr_compress(addr_map_rec.ieee_addr, &ZG->addr.addr_map[addr_map_rec.index].ieee_addr);
        ZG->addr.addr_map[addr_map_rec.index].redirect_ref = (zb_address_ieee_ref_t)(-1);

        /* lock addresses while restoring binding and neighbor tables
         * this tables. Other references are filled and used in
         * run-time, no need to handle them */
        ZG->addr.addr_map[addr_map_rec.index].lock_cnt = 0;
        ZG->addr.addr_map[addr_map_rec.index].clock = ZB_TRUE_U;
        ZG->addr.n_elements++;

        zb_add_short_addr_sorted(addr_map_rec.index, addr_map_rec.addr);
      }
    }
    else if ((hdr.version == ZB_NVRAM_ADDR_MAP_DS_VER_1) || (hdr.version == ZB_NVRAM_ADDR_MAP_DS_VER_2))
#endif /* #ifndef ZB_NO_NVRAM_VER_MIGRATION */
    {
      zb_nvram_addr_map_rec_v2_t addr_map_rec;

      ZB_BZERO(&addr_map_rec, sizeof(zb_nvram_addr_map_rec_v2_t));

      for (i = 0; i < hdr.addr_map_num && ret == RET_OK; ++i)
      {
        if (hdr.version == ZB_NVRAM_ADDR_MAP_DS_VER_1)
        {
          ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&addr_map_rec,
                                   (zb_uint16_t)sizeof(zb_nvram_addr_map_rec_v1_t));
          pos += sizeof(zb_nvram_addr_map_rec_v1_t);
        }
        else
        {
          ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&addr_map_rec,
                                   (zb_uint16_t)sizeof(zb_nvram_addr_map_rec_v2_t));
          pos += sizeof(zb_nvram_addr_map_rec_v2_t);
        }

        TRACE_MSG(TRACE_COMMON3, "Read rec: idx %hd addr 0x%x ieee" TRACE_FORMAT_64,
                  (FMT__H_D_A, addr_map_rec.index, addr_map_rec.addr, TRACE_ARG_64(addr_map_rec.ieee_addr)));
        TRACE_MSG(TRACE_COMMON3, "Redirect type: %d", (FMT__D, addr_map_rec.redirect_type));

//        ZB_ASSERT(ZG->addr.addr_map[addr_map_rec.index].used == 0);
        if (ZB_U2B(ZG->addr.addr_map[addr_map_rec.index].used))
        {
          /* Strange, entry is already in use! It means that NVRAM is corrupted!
             Assert on NVRAM read operation is not the best idea. Try to skip this bad entry and
             continue...  */
          TRACE_MSG(TRACE_ERROR, "WARNING: bad addr map entry %hd, skip", (FMT__H, addr_map_rec.index));
          continue;
        }

        ZG->addr.addr_map[addr_map_rec.index].redirect_type = addr_map_rec.redirect_type;
        if (ZG->addr.addr_map[addr_map_rec.index].redirect_type == ZB_ADDR_REDIRECT_NONE)
        {
          zb_ieee_addr_compress(addr_map_rec.ieee_addr,
                                &ZG->addr.addr_map[addr_map_rec.index].ieee_addr);
          ZG->addr.addr_map[addr_map_rec.index].addr = addr_map_rec.addr;
          ZG->addr.addr_map[addr_map_rec.index].redirect_ref = addr_map_rec.redirect_ref;

          zb_add_short_addr_sorted(addr_map_rec.index, addr_map_rec.addr);
        }
        else if (addr_map_rec.redirect_type == ZB_ADDR_REDIRECT_SHORT)
        {
          ZG->addr.addr_map[addr_map_rec.index].addr = addr_map_rec.addr;
          ZG->addr.addr_map[addr_map_rec.index].redirect_ref = addr_map_rec.redirect_ref;
        }
        else
        {
          zb_ieee_addr_compress(addr_map_rec.ieee_addr,
                                &ZG->addr.addr_map[addr_map_rec.index].ieee_addr);
          ZG->addr.addr_map[addr_map_rec.index].redirect_ref = addr_map_rec.redirect_ref;
        }

        ZG->addr.addr_map[addr_map_rec.index].used = ZB_TRUE_U;

        /* lock addresses while restoring binding and neighbor tables
         * this tables. Other references are filled and used in
         * run-time, no need to handle them */				
        ZG->addr.addr_map[addr_map_rec.index].lock_cnt = 0;

        ZG->addr.addr_map[addr_map_rec.index].clock = 1U;
        ZG->addr.n_elements++;
      }
    }
#ifndef ZB_NO_NVRAM_VER_MIGRATION
    else
    {
      TRACE_MSG(TRACE_COMMON2, "Unknown ADDR dataset version %hd", (FMT__H, hdr.version));
      ZB_ASSERT(0);
    }
#endif
  }

#ifndef ZB_COORDINATOR_ONLY
  if (ZB_IS_DEVICE_ZR()
#ifdef ZB_DISTRIBUTED_SECURITY_ON
      && zb_tc_is_distributed()
#endif /* ZB_DISTRIBUTED_SECURITY_ON */
    )
  {
    zb_address_ieee_ref_t addr_ref;
    /* For End device lock will be restored after successful rejoin. */
    ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_TRUE, ZB_TRUE, &addr_ref);
  }

  if (!ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address)
#ifdef ZB_DISTRIBUTED_SECURITY_ON
      && !zb_tc_is_distributed()
#endif /* ZB_DISTRIBUTED_SECURITY_ON */
    )
  {
#ifdef ZB_TC_AT_ZC
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00012,23} */
    if (!ZB_IS_DEVICE_ZC())
    {
      zb_address_ieee_ref_t addr_ref;

      /* Lock address - it should be stored in NVRAM. */
      (void)zb_address_update(ZB_AIB().trust_center_address, 0, ZB_TRUE, &addr_ref);

      /* For End device lock will be restored after successful rejoin. */
      if (ZB_IS_DEVICE_ZR())
      {
        /* If we have a trust_center, seems like we are joined - restore "association" lock.
           WARNING: We can have no parent, but we may be joined! */
        ret = zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_TRUE, ZB_TRUE, &addr_ref);
      }
    }
#else
    /*
      Can try to additionally check that ths packet is from device which we
      know (both long and short) and device is TC. It can be device other
      then ZC (short 0).
    */
#error Dont know what to do here!
#endif /* !ZB_TC_AT_ZC */
  }
#endif /* !ZB_COORDINATOR_ONLY */

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_read_addr_map_dataset ret %d",
            (FMT__D, ret));

  return ret;
}

static zb_ret_t zb_nvram_write_addr_map_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_addr_map_hdr_t hdr = {0};
  zb_nvram_addr_map_rec_v2_t addr_map_rec = {0};
  zb_ret_t ret = RET_OK;
  zb_uint32_t hdr_pos;
  zb_uint8_t i;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_addr_map_dataset page %hd pos %ld",
            (FMT__H_L, page, pos));

  ZB_BZERO(&hdr, sizeof(hdr));

  hdr_pos = pos;
  pos += sizeof(zb_nvram_addr_map_hdr_t);

  /* Write addr_map entries */
  for (i = 0; i < ZB_IEEE_ADDR_TABLE_SIZE && ret == RET_OK; i++)
  {
    if (ZB_U2B(ZG->addr.addr_map[i].used) && ZG->addr.addr_map[i].lock_cnt > 0U)
    {
      ZB_BZERO(&addr_map_rec, sizeof(addr_map_rec));

      if (ZG->addr.addr_map[i].redirect_type == ZB_ADDR_REDIRECT_NONE)
      {
        zb_ieee_addr_decompress(addr_map_rec.ieee_addr, &ZG->addr.addr_map[i].ieee_addr);
        addr_map_rec.addr = ZG->addr.addr_map[i].addr;
        addr_map_rec.redirect_ref = ZG->addr.addr_map[i].redirect_ref;
      }
      else if (ZG->addr.addr_map[i].redirect_type == ZB_ADDR_REDIRECT_SHORT)
      {
        addr_map_rec.addr = ZG->addr.addr_map[i].addr;
        addr_map_rec.redirect_ref = ZG->addr.addr_map[i].redirect_ref;
      }
      else
      {
        zb_ieee_addr_decompress(addr_map_rec.ieee_addr, &ZG->addr.addr_map[i].ieee_addr);
        addr_map_rec.redirect_ref = ZG->addr.addr_map[i].redirect_ref;
      }

      /* Common code */
      addr_map_rec.index = i;
      addr_map_rec.redirect_type = ZG->addr.addr_map[i].redirect_type;

      TRACE_MSG(TRACE_COMMON3,
                "Write addr_map record: index %hd short_addr %d ieee " TRACE_FORMAT_64,
                (FMT__H_D_A, addr_map_rec.index, addr_map_rec.addr,
                 TRACE_ARG_64(addr_map_rec.ieee_addr)));
      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&addr_map_rec,
                                (zb_uint16_t)sizeof(zb_nvram_addr_map_rec_v2_t));
      pos += sizeof(addr_map_rec);
      hdr.addr_map_num++;
    }
  }

  /* Write header */
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,5} */
  if (ret == RET_OK)
  {
    hdr.version = ZB_NVRAM_ADDR_MAP_DS_VER_2;
    TRACE_MSG(TRACE_COMMON2, "Write header: addr_map_num %hd ver %hd",
              (FMT__H_H, hdr.addr_map_num, hdr.version));

    ret = zb_nvram_write_data(page, hdr_pos, (zb_uint8_t*)&hdr,
                              (zb_uint16_t)sizeof(zb_nvram_addr_map_hdr_t));
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_addr_map_dataset ret %d",
            (FMT__D, ret));

  return ret;
}

static zb_uint16_t zb_nvram_addr_map_dataset_length(void)
{
  zb_uindex_t i;
  zb_uint32_t len;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_addr_map_dataset_length", (FMT__0));

  len = sizeof(zb_nvram_addr_map_hdr_t);

  for (i = 0; i < ZB_IEEE_ADDR_TABLE_SIZE; i++)
  {
    if (ZB_U2B(ZG->addr.addr_map[i].used) && ZG->addr.addr_map[i].lock_cnt > 0U)
    {
      len += sizeof(zb_nvram_addr_map_rec_v2_t);
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_addr_map_dataset_length %d", (FMT__D, len));
  return (zb_uint16_t)len;
}

/* ZB_NVRAM_NEIGHBOUR_TBL */
static zb_ret_t zb_nvram_read_neighbour_tbl_dataset(
  zb_uint8_t page, zb_uint32_t pos, zb_uint16_t len, zb_nvram_ver_t nvram_ver, zb_uint16_t ds_ver)
{
  zb_ret_t ret;
  zb_nvram_neighbour_hdr_t hdr;
  zb_nvram_neighbour_rec_v2_t rec;
  zb_uindex_t i;

  ZVUNUSED(ds_ver);

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_read_neighbour_tbl_dataset page %hd pos %ld len %d nvram_ver %d",
            (FMT__H_L_D_D, page, pos, len, nvram_ver));

  ZB_BZERO(&hdr, sizeof(zb_nvram_neighbour_hdr_t));
  ZB_BZERO(&rec, sizeof(zb_nvram_neighbour_rec_v2_t));

  ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));
  pos += sizeof(zb_nvram_neighbour_hdr_t);

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00007,11} */
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_COMMON2, "Read header: number of recs %hd ver %hd",
              (FMT__H_H, hdr.nbr_rec_num, hdr.version));
    ZB_ASSERT(hdr.nbr_rec_num <= ZB_NEIGHBOR_TABLE_SIZE);
    ZB_ASSERT(hdr.version < ZB_NVRAM_NEIGHBOR_TBL_DS_VER_COUNT);

    if (hdr.version < ZB_NVRAM_NEIGHBOR_TBL_DS_VER_2)
    {
      pos -= 2U;                 /* skip align bytes */
    }

    for (i = 0; i < hdr.nbr_rec_num && ret == RET_OK; ++i)
    {
#ifndef ZB_NO_NVRAM_VER_MIGRATION
      if (hdr.version == ZB_NVRAM_NEIGHBOR_TBL_DS_VER_0)
      {
        zb_nvram_neighbour_rec_v0_t rec0;
        ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&rec0,
                                 sizeof(zb_nvram_neighbour_rec_v0_t));

        /* Convert v0 record to v1 */
        ZB_MEMCPY(&rec, &rec0, sizeof(zb_nvram_neighbour_rec_v0_t));

        pos += sizeof(zb_nvram_neighbour_rec_v0_t);
      }
      else
      if (hdr.version == ZB_NVRAM_NEIGHBOR_TBL_DS_VER_1 ||
               hdr.version == ZB_NVRAM_NEIGHBOR_TBL_DS_VER_2)
      {
        ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&rec,
                                 sizeof(zb_nvram_neighbour_rec_v1_t));
        pos += sizeof(zb_nvram_neighbour_rec_v1_t);
      }
      else
#endif  /* #ifndef ZB_NO_NVRAM_VER_MIGRATION */
      {
        ret = zb_nvram_read_data(page, pos, (zb_uint8_t*)&rec,
                                 (zb_uint16_t)sizeof(zb_nvram_neighbour_rec_v2_t));
        pos += sizeof(zb_nvram_neighbour_rec_v2_t);
      }

#ifndef ZB_NO_NVRAM_VER_MIGRATION
      if (nvram_ver < ZB_NVRAM_VER_7_0)
      {
#define ZB_NWK_DEVICE_TYPE_ED_NVRAM_6_0 ZB_NWK_DEVICE_TYPE_COORDINATOR
#define ZB_NWK_DEVICE_TYPE_COORDINATOR_NVRAM_6_0 ZB_NWK_DEVICE_TYPE_ED

        /* In devices coming with R20 stack version, coord and ed
         * values are reversed */
        if (rec.device_type == ZB_NWK_DEVICE_TYPE_ED_NVRAM_6_0)
        {
          if (hdr.version == ZB_NVRAM_NEIGHBOR_TBL_DS_VER_0)
          {
            /* Allow maximum possible ED age - before it was "infinite". */
            rec.nwk_ed_timeout = ED_AGING_TIMEOUT_16384MIN;
          }

          rec.device_type = ZB_NWK_DEVICE_TYPE_ED;
        }
        else if (rec.device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR_NVRAM_6_0)
        {
          rec.device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
        }
      }
#endif  /* #ifndef ZB_NO_NVRAM_VER_MIGRATION */

      TRACE_MSG(TRACE_COMMON3,
                "Read rec: addr_ref %hd dev_type %hd rx_on %hd relation %hd key_num %hd nwk_ed_timeout %hd",
                (FMT__H_H_H_H_H_H, rec.addr_ref, rec.device_type, rec.rx_on_when_idle,
                 rec.relationship, rec.key_seq_number, rec.nwk_ed_timeout));

      ZG->nwk.neighbor.neighbor[i].used = ZB_TRUE_U;
      ZG->nwk.neighbor.neighbor[i].u.base.addr_ref = rec.addr_ref;
      if (zb_address_lock(rec.addr_ref) != RET_OK)
      {
        /* Looks like addr_table is corrupted or there is mismatch in addr_table and
         * neighbor_table. Skip this entry and try to continue... */
        TRACE_MSG(TRACE_ERROR, "WARNING: clear neighbour connected with addr_map %hd", (FMT__H, ZG->aps.binding.dst_table[i]));
        ZB_BZERO(&ZG->nwk.neighbor.neighbor[i], sizeof(zb_neighbor_tbl_ent_t));
        continue;
      }
      ZG->nwk.neighbor.neighbor[i].depth = rec.depth;
      ZG->nwk.neighbor.neighbor[i].rx_on_when_idle = rec.rx_on_when_idle;
      ZG->nwk.neighbor.neighbor[i].relationship = rec.relationship;
      if (ZG->nwk.neighbor.neighbor[i].relationship == ZB_NWK_RELATIONSHIP_PARENT)
      {
        TRACE_MSG(TRACE_NWK3, "Set handle.parent %hd", (FMT__H, rec.addr_ref));
        ZG->nwk.handle.parent = rec.addr_ref;
      }
      ZG->nwk.neighbor.neighbor[i].device_type = rec.device_type;
      ZG->nwk.neighbor.neighbor[i].u.base.key_seq_number = rec.key_seq_number;
      ZG->nwk.neighbor.neighbor[i].rssi = (zb_int8_t)ZB_MAC_RSSI_UNDEFINED;
      ZG->nwk.neighbor.addr_to_neighbor[rec.addr_ref] = (zb_uint8_t)i;
      ZG->nwk.neighbor.neighbor[i].mac_iface_idx = rec.mac_iface_idx;

      /* Address is already locked due to
       * the addr_ref map. */

#if defined ZB_ROUTER_ROLE
      if ((rec.relationship == ZB_NWK_RELATIONSHIP_CHILD)
          || (rec.relationship == ZB_NWK_RELATIONSHIP_UNAUTHENTICATED_CHILD))
      {
        if (rec.device_type == ZB_NWK_DEVICE_TYPE_ROUTER)
        {
          ZB_NIB().router_child_num++;
        }
        else if (rec.device_type == ZB_NWK_DEVICE_TYPE_ED)
        {
          ZG->nwk.neighbor.neighbor[i].u.base.nwk_ed_timeout = rec.nwk_ed_timeout;
          ZB_NIB().ed_child_num++;
          /* Init but don't run ED aging here. It should be started after NWK reset */
          zb_init_ed_aging(&ZG->nwk.neighbor.neighbor[i], rec.nwk_ed_timeout, ZB_FALSE);
        }
        else
        {
          /* MISRA rule 15.7 requires empty 'else' branch. */
        }
      }
#endif
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_read_neighbour_tbl_dataset ret %d",
            (FMT__D, ret));

  return ret;
}

static zb_ret_t zb_nvram_write_neighbour_tbl_dataset(zb_uint8_t page, zb_uint32_t pos)
{
  zb_nvram_neighbour_hdr_t hdr;
  zb_nvram_neighbour_rec_v2_t rec;
  zb_uint32_t hdr_pos;
  zb_ret_t ret = RET_OK;
  zb_uindex_t i;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_neighbour_tbl_dataset page %hd pos %ld",
            (FMT__H_L, page, pos));

  ZB_BZERO(&hdr, sizeof(hdr));

  /* Remember header position at start - fill it later */
  hdr_pos = pos;
  pos += sizeof(zb_nvram_neighbour_hdr_t);

  /* For each record: copy only actual neighbour values to data set */
  for (i = 0; i < ZB_NEIGHBOR_TABLE_SIZE && ret == RET_OK; ++i)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used) && !ZB_U2B(ZG->nwk.neighbor.neighbor[i].ext_neighbor))
    {
      zb_neighbor_tbl_ent_t *nbr = &ZG->nwk.neighbor.neighbor[i];

      TRACE_MSG(TRACE_COMMON3,
                "Write rec: addr_ref %hd dev_type %hd rx_on %hd relation %hd key_num %hd nwk_ed_timeout %hd",
                (FMT__H_H_H_H_H_H, nbr->u.base.addr_ref, nbr->device_type, nbr->rx_on_when_idle,
                 nbr->relationship, nbr->u.base.key_seq_number, nbr->u.base.nwk_ed_timeout));

      rec.addr_ref = nbr->u.base.addr_ref;
      rec.depth = nbr->depth;
      rec.rx_on_when_idle = nbr->rx_on_when_idle;
      rec.relationship = nbr->relationship;
      rec.device_type = nbr->device_type;
      rec.key_seq_number = nbr->u.base.key_seq_number;
      rec.nwk_ed_timeout = nbr->u.base.nwk_ed_timeout;
      rec.mac_iface_idx = nbr->mac_iface_idx;

      /* If we fail, trace is given and assertion is triggered */
      ret = zb_nvram_write_data(page, pos, (zb_uint8_t*)&rec, (zb_uint16_t)sizeof(rec));
      pos += sizeof(rec);

      hdr.nbr_rec_num++;
    }
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00008,6} */
  if (ret == RET_OK)
  {
    hdr.version = ZB_NVRAM_NEIGHBOR_TBL_DS_VER_3;

    TRACE_MSG(TRACE_COMMON2, "Write header: records num %hd ver %hd",
              (FMT__H_H, hdr.nbr_rec_num, hdr.version));
    /* If we fail, trace is given and assertion is triggered */
    ret = zb_nvram_write_data(page, hdr_pos, (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_neighbour_tbl_dataset ret %d",
            (FMT__D, ret));

  return ret;
}

static zb_uint16_t zb_nvram_neighbour_tbl_dataset_length(void)
{
  zb_uint32_t len;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_addr_map_dataset_length", (FMT__0));

  len = zb_nwk_neighbor_table_size();
  len = sizeof(zb_nvram_neighbour_hdr_t) + len * sizeof(zb_nvram_neighbour_rec_v2_t);
  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_addr_map_dataset_length %d", (FMT__D, len));

  return (zb_uint16_t)len;
}

void zb_nvram_store_addr_n_nbt(void)
{
  zb_nvram_transaction_start();
  /* If we fail, trace is given and assertion is triggered */
  (void)zb_nvram_write_dataset(ZB_NVRAM_ADDR_MAP);
  (void)zb_nvram_write_dataset(ZB_NVRAM_NEIGHBOUR_TBL);
  zb_nvram_transaction_commit();
}

#endif  /* #ifndef APP_ONLY_NVRAM */

void zb_nvram_register_app1_read_cb(zb_nvram_read_app_data_t cb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app1_read_cb %p", (FMT__P, cb));
  ZB_NVRAM().read_app_data_cb[0] = cb;
}


void zb_nvram_register_app2_read_cb(zb_nvram_read_app_data_t cb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app2_read_cb %p", (FMT__P, cb));
  ZB_NVRAM().read_app_data_cb[1] = cb;
}


void zb_nvram_register_app3_read_cb(zb_nvram_read_app_data_t cb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app3_read_cb %p", (FMT__P, cb));
  ZB_NVRAM().read_app_data_cb[2] = cb;
}


void zb_nvram_register_app4_read_cb(zb_nvram_read_app_data_t cb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app4_read_cb %p", (FMT__P, cb));
  ZB_NVRAM().read_app_data_cb[3] = cb;
}


void zb_nvram_register_app1_write_cb(
    zb_nvram_write_app_data_t wcb,
    zb_nvram_get_app_data_size_t gcb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app1_write_cb wcb %p gcb %p", (FMT__P_P, wcb, gcb));
  ZB_NVRAM().write_app_data_cb[0] = wcb;
  ZB_NVRAM().get_app_data_size_cb[0] = gcb;
}


void zb_nvram_register_app2_write_cb(
    zb_nvram_write_app_data_t wcb,
    zb_nvram_get_app_data_size_t gcb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app2_write_cb wcb %p gcb %p", (FMT__P_P, wcb, gcb));
  ZB_NVRAM().write_app_data_cb[1] = wcb;
  ZB_NVRAM().get_app_data_size_cb[1] = gcb;
}


void zb_nvram_register_app3_write_cb(
    zb_nvram_write_app_data_t wcb,
    zb_nvram_get_app_data_size_t gcb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app3_write_cb wcb %p gcb %p", (FMT__P_P, wcb, gcb));
  ZB_NVRAM().write_app_data_cb[2] = wcb;
  ZB_NVRAM().get_app_data_size_cb[2] = gcb;
}


void zb_nvram_register_app4_write_cb(
    zb_nvram_write_app_data_t wcb,
    zb_nvram_get_app_data_size_t gcb)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app4_write_cb wcb %p gcb %p", (FMT__P_P, wcb, gcb));
  ZB_NVRAM().write_app_data_cb[3] = wcb;
  ZB_NVRAM().get_app_data_size_cb[3] = gcb;
}


static void zb_nvram_read_app_data(zb_uint8_t i, zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  TRACE_MSG(TRACE_COMMON1, "zb_nvram_read_app_data1 %hd pos %ld len %d", (FMT__H_L_D, page, pos, payload_length));
  if (ZB_NVRAM().read_app_data_cb[i] != NULL)
  {
    ZB_NVRAM().read_app_data_cb[i](page, pos, payload_length);
  }
}

/**
   Mark nvram erase finished for this page.
   osif layer calls this routine when async page erase done.
 */
void zb_nvram_erase_finished(zb_uint8_t page)
{
  zb_nvram_dataset_hdr_t hdr = {0};

  /* Put special header into page begin to mark it as successfully erased */
  ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INC(ZB_NVRAM().current_time_label);
  hdr.time_label = ZB_NVRAM().current_time_label;
  hdr.data_set_type = (zb_uint16_t)ZB_NVRAM_DATA_SET_TYPE_PAGE_HDR;
  hdr.data_len = (zb_uint16_t)PAGE_HEADER_SIZE;
  hdr.data_set_version = ZB_NVRAM_VERSION;

  (void)zb_osif_nvram_write(page, 0, ((zb_uint8_t*)&hdr), (zb_uint16_t)sizeof(hdr));
  (void)zb_nvram_write_page_hdr_dataset(page, sizeof(hdr));
  (void)nvram_write_dataset_tail(page,
                                 0,
                                 hdr.data_len,
                                 hdr.time_label);
  zb_osif_nvram_flush();

  ZB_NVRAM().pages[page].erased = 1;
  ZB_NVRAM().pages[page].erase_in_progress = 0;
  ZB_NVRAM().pages[page].nvram_ver = ZB_NVRAM_VERSION;

  TRACE_MSG(TRACE_COMMON1, "page %hd erase finished", (FMT__H, page));
}

#endif  /* #if defined ZB_USE_NVRAM || defined doxygen */

void zb_nvram_save_all(void)
{
  /* save to nvram */
#if defined ZB_USE_NVRAM
  int i;

  TRACE_MSG(TRACE_COMMON1, ">> nvram save", (FMT__0));
  zb_nvram_transaction_start();
  for(i=0; i<(int)ZB_NVRAM_DATASET_NUMBER; i++)
  {
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset((zb_nvram_dataset_types_t)i);
  }
  zb_nvram_transaction_commit();
  TRACE_MSG(TRACE_COMMON1, "<< nvram save", (FMT__0));
#endif
}


void zb_nvram_erase(void)
{
#if defined ZB_USE_NVRAM
  zb_uint8_t i;
  zb_uint8_t nvram_page_count = zb_get_nvram_page_count();

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_erase", (FMT__0));

#ifdef ZB_NO_RUNTIME_NVM_ERASE
  ZB_ASSERT(0);
#endif  /* ZB_NO_RUNTIME_NVM_ERASE */
  ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INITIAL;
  for(i = 0; i < nvram_page_count; i++)
  {
    (void)zb_osif_nvram_erase_async(i);
#ifdef ZB_PLATFORM_CORTEX_M3
    zb_osif_nvram_wait_for_last_op();
#endif
  }
  ZB_NVRAM().current_pos = PAGE_HEADER_SIZE;
  ZB_NVRAM().current_page = 0;
  ZB_NVRAM().empty = 1;

  ZB_NVRAM().inited = 1;

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_erase", (FMT__0));
#endif
}

#ifndef ZB_USE_NVRAM
void zb_nvram_load(void)
{
}
#endif


void zb_ib_set_defaults(zb_char_t *rx_pipe)
{
#ifndef APP_ONLY_NVRAM
  zb_uint8_t i;
#endif
  ZVUNUSED(rx_pipe);

#if (defined ZB_USE_NVRAM) && (defined ZB_SET_MAC_ADDRESS) && !defined APP_ONLY_NVRAM
  zb_set_default_mac_addr(rx_pipe);
#endif

#ifndef APP_ONLY_NVRAM
  /* pand id to join or form */
  /*ZB_EXTPANID_ZERO(ZB_AIB().aps_use_extended_pan_id);
  ZB_AIB().aps_use_extended_pan_id[7] = 8;*/

  /* MMDEVQ: right? */
  zb_channel_list_init(ZB_AIB().aps_channel_mask_list);
  TRACE_MSG(TRACE_APS1, "aps_channel_mask_list:", (FMT__0));
  /* Let's set 2.4GHz mask by default. TODO: check which masks are default for each subgig page */

  zb_channel_page_list_set_mask(ZB_AIB().aps_channel_mask_list, ZB_CHANNEL_PAGE0_2_4_GHZ, ZB_DEFAULT_APS_CHANNEL_MASK);
#ifndef ZB_MAC_TESTING_MODE
  for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
  {
    TRACE_MSG(TRACE_APS1,
              "page %hd, mask 0x%lx",
              (FMT__H_L, zb_aib_channel_page_list_get_page(i),zb_aib_channel_page_list_get_mask(i)));
  }
#endif /* ZB_MAC_TESTING_MODE */
  ZB_AIB().aps_insecure_join = 1;

  ZB_SET_NIB_SECURITY_LEVEL(ZB_SECURITY_LEVEL);


#if defined ZB_TC_GENERATES_KEYS && defined ZB_COORDINATOR_ROLE && !defined ZB_MACSPLIT_DEVICE
  secur_nwk_generate_keys();
#endif  /* ZB_TC_GENERATES_KEYS */

#ifdef ZB_ROUTER_ROLE
  /* It is ok that we do not call MLME-SET here: it will be done later, from zdo_dev_start() */
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_TRUE_U;
#endif
  ZB_NIB().max_children = ZB_DEFAULT_MAX_CHILDREN;
#endif
  ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_NONE;
#endif  /* !APP_ONLY_NVRAM */
}


#if defined ZB_SET_MAC_ADDRESS && !defined APP_ONLY_NVRAM
void zb_set_default_mac_addr(zb_char_t *rx_pipe)
{
#if defined ZB_DEFAULTS_GENERATE_RANDOM_EXT_ADDRESS
  ZVUNUSED(rx_pipe);
  ZB_PIBCACHE_EXTENDED_ADDRESS()[7] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[6] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[5] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[4] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[3] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[2] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[1] = 0xAA;
  ZB_PIBCACHE_EXTENDED_ADDRESS()[0] = ZB_RANDOM_U8();
#else
  ZVUNUSED(rx_pipe);
#endif
}
#endif

/* 04.07.2017 CR [DT] end */

#if defined ZB_USE_NVRAM
static zb_ret_t nvram_read_pages_headers(void)
{
  zb_ret_t       ret = RET_OK;
  zb_uint8_t     i;
  zb_nvram_ver_t min_version_value = ZB_NVRAM_VER_COUNT;
  /* Assign 0 to shut gcc varning about uninitialized value
   * use. Really check for min_version_value prevents
   * access to min_version_page min_version_page. */
  zb_ushort_t    min_version_page = 0;
  zb_uint8_t nvram_page_count = zb_get_nvram_page_count();

  TRACE_MSG(TRACE_COMMON1, ">> nvram_read_pages_headers", (FMT__0));

  for (i = 0; i < nvram_page_count; i++)
  {
    zb_nvram_dataset_hdr_t      hdr;
    zb_nvram_page_hdr_dataset_t page_hdr_ds;

    /* Read from the beginning of a page */
    ret = zb_osif_nvram_read(i, 0, (zb_uint8_t*)&hdr, (zb_uint16_t)sizeof(hdr));

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00007,12} */
    if (ret == RET_OK)
    {
      ret = zb_nvram_check_and_read_page_hdr_ds(i, &hdr, &page_hdr_ds);

      if (ret == RET_OBSOLETE)
      {
        ret = RET_OK;
      }

      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_COMMON1, "Some dust at page begin", (FMT__0));
        ZB_NVRAM().pages[i].nvram_ver = ZB_NVRAM_VER_COUNT;
        ret = RET_INVALID_STATE;
      }
    }

    if (ret == RET_OK)
    {
      if (page_hdr_ds.version <= ZB_NVRAM_MAX_VERSION)
      {
        TRACE_MSG(TRACE_COMMON2, "page %hd has correct erase header", (FMT__H, i));
        ZB_NVRAM().pages[i].erased = 1;
        ZB_NVRAM().pages[i].nvram_ver = page_hdr_ds.version;

        /* Special-case condition - find the page with the minimal
         * NVRAM version */
        if (ZB_NVRAM().pages[i].nvram_ver < min_version_value)
        {
          min_version_value = ZB_NVRAM().pages[i].nvram_ver;
          min_version_page = i;
        }
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "Incorrect NVRAM version %d", (FMT__D, page_hdr_ds.version));
        ZB_NVRAM().pages[i].nvram_ver = ZB_NVRAM_VER_COUNT;
        ret = RET_ERROR;
      }
    }

    if (ret != RET_OK)
    {
      /* Mark a bad page as dirty and erase it */
      ZB_NVRAM().pages[i].erased = 0;
      zb_nvram_erase_if_not_erased(i);

      if (ret == RET_ERROR)
      {
        ZB_ERROR_RAISE(ZB_ERROR_SEVERITY_FATAL,
                       ERROR_CODE(ERROR_CATEGORY_NVRAM, ZB_ERROR_NVRAM_READ_FAILURE),
                       NULL);
      }
    }
  }

  /* Minimal NVRAM version is found */
  if (min_version_value != ZB_NVRAM_VER_COUNT)
  {
    for (i = 0; i < nvram_page_count; i++)
    {
      /* Search across all VALID NVRAM pages:
       * if not all VALID pages have the same minimal NVRAM version,
       * then some issues during previous version migration.
       *
       * Simplified but effective algo.
       *
       */
      if (ZB_NVRAM().pages[i].nvram_ver != ZB_NVRAM_VER_COUNT &&
          i != min_version_page &&
          ZB_NVRAM().pages[i].nvram_ver != min_version_value)
      {
        /* Oooopppsss... Found a page with a version, not equal to
         * the minimal - it must be an error during previos migration. */
        ret = RET_ERROR;
        ZB_NVRAM().pages[i].nvram_ver = ZB_NVRAM_VER_COUNT;

        /* Mark a bad page as dirty and erase it */
        ZB_NVRAM().pages[i].erased = 0;
        zb_nvram_erase_if_not_erased(i);
      }
    }

    if (min_version_value != ZB_NVRAM_VERSION)
    {
      /* If not a current version, update needed */
      TRACE_MSG(TRACE_COMMON3, "obsolete NVRAM version %d", (FMT__D, min_version_value));
      ret = RET_OBSOLETE;
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_read_pages_headers ret %d", (FMT__D, ret));

  return ret;
}

static void nvram_restore_datasets(zb_nvram_position_t *datasets)
{
  zb_uint8_t i;
  TRACE_MSG(TRACE_COMMON1, ">> nvram_restore_datasets", (FMT__0));

  ZB_ASSERT(datasets);

  ZB_NVRAM().empty = 1;
  /* read */
  for(i=0; i<(zb_uint8_t)ZB_NVRAM_DATASET_NUMBER; i++)
  {
    /* pos == 0 is impossible: there is a page header */
    if (datasets[i].pos != 0U &&
        datasets[i].payload_length != 0U &&
        nvram_can_load_dataset(i, ZB_NVRAM().pages[datasets[i].page].nvram_ver))
    {
      zb_uint16_t ds_len = datasets[i].payload_length;
#ifndef APP_ONLY_NVRAM
      zb_nvram_ver_t nvram_ver = ZB_NVRAM().pages[datasets[i].page].nvram_ver;
#endif
      zb_uint16_t ds_ver = datasets[i].version;
      ZVUNUSED(ds_ver);

      /* Checked within zb_nvram_read_data() */
      ZB_NVRAM().gs_nvram_read_checker.page = datasets[i].page;
      ZB_NVRAM().gs_nvram_read_checker.pos = datasets[i].pos;
      ZB_NVRAM().gs_nvram_read_checker.payload_length = ds_len;

      TRACE_MSG(TRACE_COMMON1, "nvram load: ds type %hd page %hd pos %ld current_pos %ld",
                (FMT__H_H_L_L, i, datasets[i].page, datasets[i].pos, ZB_NVRAM().current_pos));

      if (i != (zb_uint8_t)ZB_NVRAM_RESERVED &&
          i != (zb_uint8_t)ZB_IB_COUNTERS &&
          i != (zb_uint8_t)ZB_NVRAM_APP_DATA1 &&
          i != (zb_uint8_t)ZB_NVRAM_APP_DATA2 &&
          i != (zb_uint8_t)ZB_NVRAM_APP_DATA3 &&
          i != (zb_uint8_t)ZB_NVRAM_APP_DATA4)
      {
        ZB_NVRAM().empty = 0;
      }

#ifdef ZB_NVRAM_ENABLE_DIRECT_API
      nvram_update_dataset_position(i, datasets[i].page, datasets[i].pos);
#endif /* ZB_NVRAM_ENABLE_DIRECT_API */

      if ((ZB_NVRAM().ds_filter_cb != NULL)
          && (*ZB_NVRAM().ds_filter_cb)(i) == ZB_FALSE)
      {
        TRACE_MSG(TRACE_COMMON1, "filter out dataset %d", (FMT__D, i));
        continue;
      }

      /* Here restore only fresh datasets */
      switch(i)
      {
#ifndef APP_ONLY_NVRAM
        case ZB_NVRAM_COMMON_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read common dataset", (FMT__0));
          zb_nvram_read_common_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;

#if defined ZB_ENABLE_HA  || defined DOXYGEN
        case ZB_NVRAM_HA_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read ha dataset", (FMT__0));
          zb_nvram_read_ha_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif /* defined ZB_ENABLE_HA*/

#ifdef ZB_ENABLE_ZGP
        case ZB_NVRAM_DATASET_GP_SINKT:
          zb_nvram_read_zgp_sink_table_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
        case ZB_NVRAM_DATASET_GP_APP_TBL:
          zb_zgp_nvram_read_app_tbl_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
        case ZB_NVRAM_DATASET_GP_PRPOXYT:
          zb_nvram_read_zgp_proxy_table_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
        case ZB_NVRAM_DATASET_GP_CLUSTER:
          zb_nvram_read_zgp_cluster_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif /* ZB_ENABLE_ZGP */

#if defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)
        case ZB_NVRAM_ZCL_REPORTING_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read zcl reporting dataset", (FMT__0));
          zb_nvram_read_zcl_reporting_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif /* defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING) */
        case ZB_NVRAM_APS_SECURE_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read aps secure dataset", (FMT__0));
          zb_nvram_read_aps_keypair_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
        case ZB_NVRAM_INSTALLCODES:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read installcodes secure dataset", (FMT__0));
          zb_nvram_read_installcodes_dataset(datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif
        case ZB_NVRAM_APS_BINDING_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read aps binding dataset", (FMT__0));
          zb_nvram_read_aps_binding_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
        case ZB_NVRAM_APS_GROUPS_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read aps groups dataset", (FMT__0));
          zb_nvram_read_aps_groups_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN
        case ZB_NVRAM_HA_POLL_CONTROL_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read ha poll control dataset", (FMT__0));
          zb_nvram_read_poll_control_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER || defined DOXYGEN */
#if (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN
        case ZB_NVRAM_ZCL_WWAH_DATA:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read ZCL WWAH dataset", (FMT__0));
          zb_nvram_read_wwah_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#endif /* (defined ZB_ZCL_SUPPORT_CLUSTER_WWAH && defined ZB_ZCL_ENABLE_WWAH_SERVER) || defined DOXYGEN */
#if defined ZB_STORE_COUNTERS || defined DOXYGEN
      case ZB_IB_COUNTERS:
          zb_nvram_read_counters_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          ZB_NVRAM().refresh_flag = ZB_TRUE;
          break;
#endif
#endif  /* #ifndef APP_ONLY_NVRAM */
      case ZB_NVRAM_APP_DATA1:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read app data1", (FMT__0));
          zb_nvram_read_app_data(0, (zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len);
          break;
      case ZB_NVRAM_APP_DATA2:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read app data2", (FMT__0));
          zb_nvram_read_app_data(1, (zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len);
          break;
      case ZB_NVRAM_APP_DATA3:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read app data3", (FMT__0));
          zb_nvram_read_app_data(2, (zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len);
          break;
      case ZB_NVRAM_APP_DATA4:
          TRACE_MSG(TRACE_COMMON1, "nvram load: read app data4", (FMT__0));
          zb_nvram_read_app_data(3, (zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len);
          break;
#ifndef APP_ONLY_NVRAM

      case ZB_NVRAM_ADDR_MAP:
          TRACE_MSG(TRACE_COMMON1, "nvram load: address map", (FMT__0));
          (void)zb_nvram_read_addr_map_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
      case ZB_NVRAM_NEIGHBOUR_TBL:
          TRACE_MSG(TRACE_COMMON1, "nvram load: neighbor tbl", (FMT__0));
          (void)zb_nvram_read_neighbour_tbl_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
          break;
#ifdef ZDO_DIAGNOSTICS
      case ZB_NVRAM_ZDO_DIAGNOSTICS_DATA:
        TRACE_MSG(TRACE_COMMON1, "nvram load: zdo diagnostics dataset", (FMT__0));
        zb_nvram_read_diagnostics_dataset((zb_uint8_t)datasets[i].page, datasets[i].pos, ds_len, nvram_ver, ds_ver);
        break;
#endif /* ZDO_DIAGNOSTICS */
#endif  /* #ifndef APP_ONLY_NVRAM */
      default:
#ifndef APP_ONLY_NVRAM
        (void)zb_nvram_custom_ds_try_read(i,
                                          (zb_uint8_t)datasets[i].page,
                                          datasets[i].pos,
                                          ds_len,
                                          nvram_ver,
                                          ds_ver);
#endif  /* #ifndef APP_ONLY_NVRAM */
        break;
      }
    } /* if pos != 0 */
  } /* for */

#ifndef ZB_NO_NVRAM_VER_MIGRATION
  nvram_restore_obsolete_datasets(datasets);
#endif

  TRACE_MSG(TRACE_COMMON1, "<< nvram_restore_datasets", (FMT__0));

  /* Operation finished. Reset checkers. */
  ZB_NVRAM().gs_nvram_read_checker.page = 0;
  ZB_NVRAM().gs_nvram_read_checker.pos = 0;
  ZB_NVRAM().gs_nvram_read_checker.payload_length = 0;

  return;
}

static void nvram_handle_datasets_pages(zb_nvram_load_data_t *load_data)
{
  zb_ushort_t i;
  zb_ushort_t ref = (zb_ushort_t)ZB_NVRAM_DATASET_NUMBER;
  zb_uint32_t max_page = ZB_NVRAM_MAX_N_PAGES;
  zb_ret_t    status;
  zb_nvram_position_t *datasets = load_data->datasets;

  /* Datasets are ready to be read */
  for (i=0; i<(zb_ushort_t)ZB_NVRAM_DATASET_NUMBER; i++)
  {
    /* Find first actual */
    if(datasets[i].pos != 0U && datasets[i].payload_length != 0U)
    {
      ref = i;
      break;
    }
  }

  if (ref != (zb_ushort_t)ZB_NVRAM_DATASET_NUMBER)
  {
    for (i = ref+1U; i<(zb_ushort_t)ZB_NVRAM_DATASET_NUMBER; i++)
    {
      /* If at least one dataset comes from different page...*/
      if (datasets[i].pos != 0U &&
          datasets[i].page != datasets[ref].page)
      {
        /* Taking into account we first load p0, and time labels are not changed
         * at migration, it could be only 1 case when different pages are possible:
         * Crash at migration p1 - p0. Then choose p1 as a data source and
         * re-read datasets from it.
         */
        max_page = datasets[ref].page;
        break;
      }
    }

    if (max_page != ZB_NVRAM_MAX_N_PAGES)
    {
      for (i = ref+1U; i<(zb_ushort_t)ZB_NVRAM_DATASET_NUMBER; i++)
      {
        if (datasets[i].pos != 0U &&
            datasets[i].page > max_page)
        {
          /* Actually it is 1, but, potentially, we can have >2 pages. */
          max_page = datasets[i].page;
        }
      }

      /* Search once again */
      ZB_BZERO(load_data, sizeof(zb_nvram_load_data_t));
      load_data->time_label = ZB_NVRAM_TL_RESERVED;

      status = find_datasets_on_page((zb_uint8_t)max_page, load_data);

      if (status == RET_OK)
      {
        /* Last actual time label */
        ZB_NVRAM().current_time_label = load_data->time_label;

        /* Stay on the existing full page even if it is the end of the
         * page. Migrate to the next one upon writing attempt*/
        ZB_NVRAM().current_page = load_data->page;
        ZB_NVRAM().current_pos = load_data->pos;

        TRACE_MSG(TRACE_COMMON2, "Page %d has been read successfully. current_time_label %d current_page %hd current_pos %ld",
                  (FMT__H_D_H_L, i, ZB_NVRAM().current_time_label, ZB_NVRAM().current_page, ZB_NVRAM().current_pos));
      }
      else
      {
        ZB_ASSERT(0);
      }
    }
  }
}

static zb_bool_t nvram_can_load_dataset(zb_uint8_t ds_type, zb_nvram_ver_t nvram_ver)
{
  zb_bool_t ret = ZB_FALSE;

  TRACE_MSG(TRACE_COMMON1, ">> nvram_can_load_dataset ds_type %hd, nvram_ver %d",
            (FMT__H_D, ds_type, nvram_ver));

  switch(nvram_ver)
  {
#ifndef ZB_NO_NVRAM_VER_MIGRATION
    case ZB_NVRAM_VER_1_0:
    case ZB_NVRAM_VER_2_0:
    case ZB_NVRAM_VER_3_0:
    case ZB_NVRAM_VER_4_0:
    case ZB_NVRAM_VER_5_0:
    case ZB_NVRAM_VER_6_0:
    /* FALL THROUGH */
      ret = nvram_can_load_dataset_from_nvram_ver_6(ds_type);
      break;
    case ZB_NVRAM_VER_7_0:
    case ZB_NVRAM_VER_8_0:
    case ZB_NVRAM_VER_9_0:
#endif  /* #ifndef ZB_NO_NVRAM_VER_MIGRATION */
    case ZB_NVRAM_VER_10_0:
    /* FALL THROUGH */
      ret = ZB_TRUE;
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown NVRAM version %d", (FMT__D, nvram_ver));
      break;
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_can_load_dataset ret %hd", (FMT__H, ret));

  return ret;
}

#ifndef ZB_NO_NVRAM_VER_MIGRATION
static zb_bool_t nvram_can_load_dataset_from_nvram_ver_6(zb_uint8_t ds_type)
{
  zb_bool_t ret = ZB_FALSE;

  TRACE_MSG(TRACE_COMMON1, ">> nvram_can_load_dataset_ver6 ds_type %hd",
            (FMT__H, ds_type));

  switch(ds_type)
  {
    case ZB_NVRAM_COMMON_DATA:
    case ZB_NVRAM_HA_DATA:
#if defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)
    case ZB_NVRAM_ZCL_REPORTING_DATA:
#endif
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
    case ZB_IB_COUNTERS:
    case ZB_NVRAM_APP_DATA1:
    case ZB_NVRAM_APP_DATA2:
    case ZB_NVRAM_ADDR_MAP:
    case ZB_NVRAM_NEIGHBOUR_TBL:
      /* FALL THROUGH */
      ret = ZB_TRUE;

    case ZB_NVRAM_APS_SECURE_DATA_GAP:
    case ZB_NVRAM_APS_BINDING_DATA_GAP:
    case ZB_NVRAM_DATASET_GRPW_DATA:
    case ZB_NVRAM_APP_DATA3:
    case ZB_NVRAM_APP_DATA4:
      /* FALL THROUGH */
      /* Will return false */
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "Unknown dataset %hd", (FMT__H, ds_type));
      break;
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_can_load_dataset_ver6 ret %hd",
            (FMT__H, ret));

  return ret;
}

static void nvram_restore_obsolete_datasets(zb_nvram_position_t *datasets)
{
  zb_ushort_t i;

  ZB_ASSERT(datasets);

  TRACE_MSG(TRACE_COMMON1, ">> nvram_restore_obsolete_datasets datasets %p",
            (FMT__P, datasets));

  for (i=0; i<ZB_NVRAM_DATASET_NUMBER; i++)
  {
    if (datasets[i].pos != 0 &&
        datasets[i].payload_length != 0 &&
        !nvram_can_load_dataset(i, ZB_NVRAM().pages[datasets[i].page].nvram_ver))
    {
      /* Only for those who wasn't loaded - special cases */
      if (i == ZB_NVRAM_APS_BINDING_DATA_GAP &&
          ZB_NVRAM().pages[datasets[i].page].nvram_ver < ZB_NVRAM_VER_7_0)
      {
#ifndef APP_ONLY_NVRAM
        zb_nvram_read_aps_binding_dataset(datasets[i].page,
                                          datasets[i].pos,
                                          datasets[i].payload_length,
                                          ZB_NVRAM().pages[datasets[i].page].nvram_ver,
                                          datasets[i].version);
#endif /* #ifndef APP_ONLY_NVRAM */
      }
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_restore_obsolete_datasets",
            (FMT__0));

  return;
}

static zb_uint32_t nvram_migrate_obsolete_datasets(zb_nvram_position_t *datasets,
                                                   zb_uint8_t next_page,
                                                   zb_uint32_t pos)
{
  zb_ushort_t i;
  zb_uint8_t ds_type;
  zb_nvram_dataset_hdr_t hdr;
  zb_nvram_migration_info_t migration_info;
  zb_ret_t ret;

  TRACE_MSG(TRACE_COMMON1, ">> nvram_migrate_obsolete_datasets datasets %p, next_page %hd, pos %ld",
            (FMT__P_H_L, datasets, next_page, pos));

  for (i=0; i<ZB_NVRAM_DATASET_NUMBER; i++)
  {
    ds_type = ZB_NVRAM_DATASET_NUMBER;

    if (datasets[i].pos != 0 &&
        datasets[i].payload_length != 0 &&
        !nvram_can_load_dataset(i, ZB_NVRAM().pages[datasets[i].page].nvram_ver))
    {
      if (ZB_NVRAM().pages[datasets[i].page].nvram_ver < ZB_NVRAM_VER_7_0)
      {
        if (i == ZB_NVRAM_APS_BINDING_DATA_GAP)
        {
          ds_type = ZB_NVRAM_APS_BINDING_DATA;
        }
        else if (i == ZB_NVRAM_APS_SECURE_DATA_GAP)
        {
          ds_type = ZB_NVRAM_APS_SECURE_DATA;
        }
      }

      if (ds_type != ZB_NVRAM_DATASET_NUMBER)
      {
        hdr.data_set_type = ds_type;
        ZB_NVRAM().current_time_label = ZB_NVRAM_TL_INC(ZB_NVRAM().current_time_label);
        hdr.time_label = ZB_NVRAM().current_time_label;
        hdr.data_set_version = zb_nvram_get_dataset_version(hdr.data_set_type);

        if (i == ZB_NVRAM_APS_SECURE_DATA_GAP)
        {
          /* Special case - handle differently */
          /* Calculate lenght in run-time */
          hdr.data_len = SIZE_DS_HEADER;

          migration_info.src_page = datasets[i].page;
          migration_info.src_pos = datasets[i].pos;
          migration_info.length = datasets[i].payload_length;

          migration_info.dst_page = next_page;
          migration_info.dst_pos = pos;
          migration_info.nvram_ver = ZB_NVRAM().pages[datasets[i].page].nvram_ver;

          ret = nvram_migrate_aps_secure_dataset(&migration_info, &hdr);
        }
        else
        {
          zb_nvram_position_t pos_info;

          /* Case for "transparent" datasets - which doesn't change
           * internal structures between versions, the only change is
           * dataset numbering */
          hdr.data_len = zb_nvram_get_length_dataset(&hdr);
          zb_osif_nvram_write(next_page,
                              pos,
                              (zb_uint8_t*)&hdr,
                              sizeof(hdr));

          migration_info.dst_page = next_page;
          migration_info.dst_pos = pos;
          pos_info.page = next_page;
          pos_info.pos = pos + sizeof(hdr);
          pos_info.payload_length = hdr.data_len - sizeof(hdr);

          ret = write_dataset_body_by_type(ds_type, &pos_info);
        }

        /* Commit record migration by writing time label */
        if (ret == RET_OK)
        {
          /* Commit record migration by writing time label at ds tail. */
          ret = nvram_write_dataset_tail(migration_info.dst_page,
                                         migration_info.dst_pos,
                                         hdr.data_len,
                                         hdr.time_label);
          ZB_ASSERT(ret == RET_OK);
        }

        pos += hdr.data_len;
      }
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_migrate_obsolete_datasets pos %ld",
            (FMT__L, pos));

  return pos;
}

static zb_ret_t nvram_migrate_aps_secure_dataset(zb_nvram_migration_info_t *migration_info,
                                                 zb_nvram_dataset_hdr_t    *hdr)
{
  zb_ret_t ret;
  zb_uint8_t count;
  zb_uindex_t i;
  zb_aps_device_key_pair_nvram_t rec;
  zb_aps_secur_common_data_t common_data;
  zb_aps_device_key_pair_set_nvram_1_0_t old_rec;

  ZB_ASSERT(migration_info);
  ZB_ASSERT(hdr);

  TRACE_MSG(TRACE_COMMON1, ">> nvram_migrate_aps_secure_dataset hdr %p src_page %hd src_pos %ld dst_page %hd dst_pos %ld",
            (FMT__P_H_L_H_L, hdr, migration_info->src_page, migration_info->src_pos,
             migration_info->dst_page, migration_info->dst_pos));

  count = migration_info->length / sizeof(zb_aps_device_key_pair_set_nvram_1_0_t);
  hdr->data_len += sizeof(common_data) + count * sizeof(zb_aps_device_key_pair_nvram_t);
  hdr->data_len += sizeof(zb_nvram_dataset_tail_t);

  ret = zb_osif_nvram_write(migration_info->dst_page,
                            migration_info->dst_pos,
                            (zb_uint8_t*)hdr,
                            sizeof(zb_nvram_dataset_hdr_t));

  migration_info->dst_pos += sizeof(zb_nvram_dataset_hdr_t);

  if (ret == RET_OK)
  {
    zb_int_t idx;
    zb_uint_t n = 0;

    /* Fill common data part */
    /* Use ZigBee R20 version. It will be overwritten during next
     * joining */
    common_data.coordinator_version = 20;
    ZB_AIB().coordinator_version = common_data.coordinator_version;

    ret = zb_osif_nvram_write(migration_info->dst_page,
                              migration_info->dst_pos,
                              (zb_uint8_t*)&common_data,
                              sizeof(zb_aps_secur_common_data_t));
    migration_info->dst_pos += sizeof(zb_aps_secur_common_data_t);

    ZB_AIB().aps_device_key_pair_storage.nvram_page = migration_info->dst_page;

    for (i = 0; i<count && ret == RET_OK; i++)
    {
      ret = zb_osif_nvram_read(migration_info->src_page,
                               migration_info->src_pos,
                               (zb_uint8_t*)&old_rec,
                               sizeof(zb_aps_device_key_pair_set_nvram_1_0_t));

      migration_info->src_pos += sizeof(zb_aps_device_key_pair_set_nvram_1_0_t);

      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "error during NVRAM read", (FMT__0));
        break;
      }

      ZB_MEMCPY(rec.device_address, old_rec.device_address, sizeof(zb_ieee_addr_t));
      ZB_MEMCPY(rec.link_key, old_rec.master_key, ZB_CCM_KEY_SIZE * sizeof(zb_uint8_t));
      rec.aps_link_key_type = old_rec.global_link_key;
      rec.key_attributes = ZB_SECUR_APPLICATION_KEY;

      idx = zb_64bit_hash(rec.device_address) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;

      /* seek for free slot */
      while (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE
             && ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset != 0)
      {
        idx = (idx + 1) % ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        n++;
      }

      if (n < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE)
      {
        ZB_AIB().aps_device_key_pair_storage.cached_i = idx;
        ZB_MEMCPY(&ZB_AIB().aps_device_key_pair_storage.cached, &rec, sizeof(zb_aps_device_key_pair_nvram_t));

        ret = zb_osif_nvram_write(migration_info->dst_page,
                                  migration_info->dst_pos,
                                  (zb_uint8_t*)&rec,
                                  sizeof(zb_aps_device_key_pair_nvram_t));

        migration_info->dst_pos += sizeof(zb_aps_device_key_pair_nvram_t);

        ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].nvram_offset = migration_info->dst_pos;
        TRACE_MSG(TRACE_COMMON1, "zb_aps_keypair_write: ok, cached idx %d", (FMT__D, idx));
      }
      else
      {
        /* no free space! */
        TRACE_MSG(TRACE_ERROR, "No free space in a table!", (FMT__0));
        ret = RET_ERROR;
      }
    }
  }

  TRACE_MSG(TRACE_COMMON1, "<< nvram_migrate_aps_secure_dataset ret %d",
            (FMT__D, ret));

  return ret;
}
#endif  /* #ifndef ZB_NO_NVRAM_VER_MIGRATION */

#ifdef ZB_NVRAM_ENABLE_DIRECT_API

static void nvram_update_dataset_position(zb_nvram_dataset_types_t type,
                                          zb_uint8_t page,
                                          zb_uint32_t pos)
{
  TRACE_MSG(TRACE_COMMON2, "nvram_update_dataset_position, type %d, page %d, pos %d",
            (FMT__D_D_D, type, page, pos));
  ZB_NVRAM().ds_pos[type].page = page;
  ZB_NVRAM().ds_pos[type].pos = pos - SIZE_DS_HEADER;
}


static zb_ret_t write_dataset_direct_validate_params(zb_nvram_dataset_types_t dataset_type,
                                                     zb_uint16_t dataset_version,
                                                     zb_uint8_t *buf,
                                                     zb_uint16_t data_len)
{
  zb_ret_t ret = RET_OK;

  if (ret == RET_OK)
  {
    if (!(dataset_type > ZB_NVRAM_RESERVED && dataset_type < ZB_NVRAM_DATASET_NUMBER))
    {
      TRACE_MSG(TRACE_ERROR, "invalid dataset type: %d", (FMT__D, dataset_type));
      ret = RET_INVALID_PARAMETER_1;
    }
  }

  if (ret == RET_OK)
  {
    zb_uint16_t expected_version = zb_nvram_get_dataset_version(dataset_type);
    if (expected_version != dataset_version)
    {
      TRACE_MSG(TRACE_ERROR, "unsupported dataset version %d, expected %d",
                (FMT__D_D, dataset_version, expected_version));
      ret = RET_INVALID_PARAMETER_2;
    }
  }

  if (ret == RET_OK)
  {
    if (buf == 0)
    {
      TRACE_MSG(TRACE_ERROR, "invalid buf", (FMT__0));
      ret = RET_INVALID_PARAMETER_3;
    }
  }

  /* Maybe some sanity check could be performed here... */
  ZVUNUSED(data_len);

  return ret;
}


zb_ret_t zb_nvram_write_dataset_direct(zb_nvram_dataset_types_t dataset_type,
                                       zb_uint16_t dataset_version,
                                       zb_uint8_t *buf, zb_uint16_t data_len)
{
  zb_ret_t ret;
  zb_nvram_dataset_hdr_t hdr;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_dataset_direct, type %d, version %d, buf %p len %d",
            (FMT__D_D_P_D, dataset_type, dataset_version, buf, data_len));

  do
  {
    ret = write_dataset_direct_validate_params(dataset_type, dataset_version, buf, data_len);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to parse parameters, ret %d", (FMT__D, ret));
      break;
    }
    else
    {
      ZB_BZERO(&hdr, sizeof(hdr));

      hdr.data_set_type = dataset_type;
      hdr.data_len = data_len + SIZE_DS_HEADER + sizeof(zb_nvram_dataset_tail_t);
    }


    ret = zb_nvram_write_start(&hdr);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to write a dataset header, ret %d", (FMT__D, ret));
      break;
    }

    ret = zb_osif_nvram_write(ZB_NVRAM().current_page,
                              ZB_NVRAM().current_pos + SIZE_DS_HEADER,
                              buf,
                              data_len);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to write a dataset body, ret %d", (FMT__D, ret));
      ZB_NVRAM().current_pos += hdr.data_len;
      break;
    }

    ret = zb_nvram_write_end(&hdr);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to write a dataset tail, ret %d", (FMT__D, ret));
      ZB_NVRAM().current_pos += hdr.data_len;
      break;
    }
  } while(0);

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_dataset_direct, ret %d", (FMT__D, ret));
  return ret;
}


static zb_ret_t read_dataset_direct_validate_params(zb_nvram_dataset_types_t dataset_type,
                                                    zb_uint8_t *buf,
                                                    zb_size_t buf_len)
{
  zb_ret_t ret = RET_OK;

  ZVUNUSED(buf_len);

  if (ret == RET_OK)
  {
    if (!(dataset_type > ZB_NVRAM_RESERVED && dataset_type < ZB_NVRAM_DATASET_NUMBER))
    {
      TRACE_MSG(TRACE_ERROR, "invalid dataset type: %d", (FMT__D, dataset_type));
      ret = RET_INVALID_PARAMETER_1;
    }
  }

  if (ret == RET_OK)
  {
    /* if pos is uninitialized then this dataset is not written yet */
    if (ZB_NVRAM().ds_pos[dataset_type].pos == 0)
    {
      ret = RET_NOT_FOUND;
    }
  }

  if (ret == RET_OK)
  {
    if (buf == 0)
    {
      TRACE_MSG(TRACE_ERROR, "invalid buf", (FMT__0));
      ret = RET_INVALID_PARAMETER_2;
    }
  }

  return ret;
}


zb_ret_t zb_nvram_read_dataset_direct(zb_nvram_dataset_types_t dataset_type,
                                      zb_uint8_t *buf,
                                      zb_size_t buf_len,
                                      zb_uint16_t *dataset_version,
                                      zb_uint16_t *dataset_length,
                                      zb_uint16_t *nvram_version)
{
  zb_ret_t ret;
  zb_nvram_dataset_hdr_t hdr;

  zb_uint32_t curr_pos;
  zb_uint8_t curr_page;
  zb_size_t ds_len;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_read_dataset_direct, type %d, buf %p, buf_len %d",
            (FMT__D_P_D, dataset_type, buf, buf_len));

  do
  {
    ret = read_dataset_direct_validate_params(dataset_type, buf, buf_len);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "invalid params, ret %d", (FMT__D, ret));
      break;
    }

    curr_page = ZB_NVRAM().ds_pos[dataset_type].page;
    curr_pos = ZB_NVRAM().ds_pos[dataset_type].pos;

    ret = zb_osif_nvram_read(curr_page, curr_pos, (zb_uint8_t*)&hdr, sizeof(hdr));
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to read a dataset header, ret %d", (FMT__D, ret));
      break;
    }

    if (!nvram_dataset_tail_is_ok(&hdr,
                                  curr_page, curr_pos,
                                  ZB_NVRAM().pages[curr_page].nvram_ver))
    {
      TRACE_MSG(TRACE_ERROR, "dataset validation failed", (FMT__0));
      ret = RET_ERROR;
      break;
    }
    curr_pos += sizeof(hdr);

    ds_len = hdr.data_len - sizeof(hdr) - sizeof(zb_nvram_dataset_tail_t);
    if (ds_len > buf_len)
    {
      TRACE_MSG(TRACE_ERROR, "buffer len is insufficient, buf size: %d bytes, but need %d bytes",
                (FMT__D_D, buf_len, ds_len));
      ret = RET_ERROR;
      break;
    }

    ret = zb_osif_nvram_read(curr_page, curr_pos, buf, ds_len);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "failed to read a dataset body, ret %d", (FMT__D, ret));
      break;
    }

    if (dataset_version != NULL)
    {
      *dataset_version = hdr.data_set_version;
    }

    if (dataset_length != NULL)
    {
      *dataset_length = ds_len;
    }

    if (nvram_version != NULL)
    {
      *nvram_version = ZB_NVRAM_VERSION;
    }
  } while(0);


  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_read_dataset_direct, ret %d", (FMT__D, ret));
  return ret;
}

#endif /* ZB_NVRAM_ENABLE_DIRECT_API */


zb_ret_t zb_nvram_read_data(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_read_data, page %d, pos %d, buf %p, len %d",
            (FMT__D_D_P_D, page, pos, buf, len));

  do
  {
    if (page != ZB_NVRAM().gs_nvram_read_checker.page)
    {
      TRACE_MSG(TRACE_ERROR, "incorrect page %d, expected %d", (FMT__D, page, ZB_NVRAM().gs_nvram_read_checker.page));
      ret = RET_INVALID_PARAMETER_1;
      break;
    }

    if (pos < ZB_NVRAM().gs_nvram_read_checker.pos
        || pos >= (ZB_NVRAM().gs_nvram_read_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_read_checker.payload_length))
    {
      TRACE_MSG(TRACE_ERROR,
                "<< zb_nvram_read_data: trying to read from [ %d ; %d ], but read area should be "
                "between [ %d ; %d ]",
                (FMT__D_D_D_D, pos, (pos + (zb_uint32_t)len), ZB_NVRAM().gs_nvram_read_checker.pos,
                 (ZB_NVRAM().gs_nvram_read_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_read_checker.payload_length)));
      ret = RET_INVALID_PARAMETER_2;
      break;
    }

    if ((pos + (zb_uint32_t)len) > ZB_NVRAM().gs_nvram_read_checker.pos
                                       + (zb_uint32_t)ZB_NVRAM().gs_nvram_read_checker.payload_length)
    {
      TRACE_MSG(TRACE_ERROR,
                "<< zb_nvram_read_data: trying to read from [ %d ; %d ], but read area should be between [ %d ; %d ]",
                (FMT__D_D_D_D, pos, (pos + (zb_uint32_t)len), ZB_NVRAM().gs_nvram_read_checker.pos,
                 (ZB_NVRAM().gs_nvram_read_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_read_checker.payload_length)));
      ret = RET_INVALID_PARAMETER_4;
      break;
    }

  } while (ZB_FALSE);

  if (ret == RET_OK)
  {
    ret = zb_osif_nvram_read(page, pos, buf, len);
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_read_data, ret %d", (FMT__D, ret));

  ZB_ASSERT(RET_OK == ret);

  return ret;
}

zb_ret_t zb_nvram_write_data(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_data, page %d, pos %d, buf %p, len %d",
            (FMT__D_D_P_D, page, pos, buf, len));

  do
  {
    /* Assure page is correct */
    if (page != ZB_NVRAM().gs_nvram_write_checker.page)
    {
      TRACE_MSG(TRACE_ERROR, "incorrect page %d, expected %d",
                (FMT__D, page, ZB_NVRAM().gs_nvram_write_checker.page));
      //ret = RET_INVALID_PARAMETER_1;
      //break;
    }

    if (pos < ZB_NVRAM().gs_nvram_write_checker.pos
        || pos >= (ZB_NVRAM().gs_nvram_write_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_write_checker.payload_length))
    {
      TRACE_MSG(TRACE_ERROR,
                "<< zb_nvram_write_data: trying to write into [ %d ; %d ], but write area should be "
                "between [ %d ; %d ]",
                (FMT__D_D_D_D, pos, (pos + (zb_uint32_t)len), ZB_NVRAM().gs_nvram_write_checker.pos,
                 (ZB_NVRAM().gs_nvram_write_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_write_checker.payload_length)));
      //ret = RET_INVALID_PARAMETER_2;
      //break;
    }

    if ((pos + (zb_uint32_t)len) > (ZB_NVRAM().gs_nvram_write_checker.pos
                                    + (zb_uint32_t)ZB_NVRAM().gs_nvram_write_checker.payload_length))
    {
      TRACE_MSG(TRACE_ERROR,
                "<< zb_nvram_write_data: trying to write into [ %d ; %d ], but write area should be "
                "between [ %d ; %d ]",
                (FMT__D_D_D_D, pos, (pos + (zb_uint32_t)len), ZB_NVRAM().gs_nvram_write_checker.pos,
                 (ZB_NVRAM().gs_nvram_write_checker.pos
                  + (zb_uint32_t)ZB_NVRAM().gs_nvram_write_checker.payload_length)));
      //ret = RET_INVALID_PARAMETER_4;
      //break;
    }

    ret = zb_osif_nvram_write(page, pos, buf, len);

  } while (ZB_FALSE);

  TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_data, ret %d", (FMT__D, ret));

  ZB_ASSERT(RET_OK == ret);

  return ret;
}

#endif /* ZB_USE_NVRAM */
/*! @} */
