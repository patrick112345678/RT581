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
/*  PURPOSE: NCP host SoC state cache
*/

#define ZB_TRACE_FILE_ID 60

#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_soc_state.h"
#include "ncp_hl_proto.h"

#define MAC_MAX_INVISIBLE_ADDRESSES_COUNT 5
#define MAC_MAX_VISIBLE_ADDRESSES_COUNT 5

/* These values are parameters of startup procedure.
   They are sent to the SoC.
   The only exception is pan_id: it could be retrieved from the SoC during startup.

   All parameters are considered to be unset if their values are equal to zero
   unless otherwise specified.
*/
typedef struct ncp_start_params_s
{
  zb_ieee_addr_t long_addr;
  zb_secur_material_set_t secur_material_set[ZB_SECUR_N_SECUR_MATERIAL];
  zb_bool_t secur_material_specified[ZB_SECUR_N_SECUR_MATERIAL];

  /* If application sets this field:
       ZC creates a network with that Extended PAN ID,
       ZR or ZED can join only the network with the specified Extended PAN ID.
     For getting current PAN ID use zb_get_extended_pan_id() */
  zb_ext_pan_id_t use_extended_pan_id;
  zb_uint16_t pan_id;
  zb_nwk_device_type_t device_type;
  zb_channel_list_t channel_list;
  zb_uint8_t max_children;
  zb_uint8_t rx_on_when_idle; /* 0xff means unset, if unset,
                                 value will be determined based on network role */
  zb_bool_t permit_control4_network;
  zb_bool_t nvram_erase;
  zb_bool_t is_distributed_security;

  nwk_keepalive_supported_method_t keepalive_mode;
  zb_bool_t keepalive_mode_is_set;

  zb_uint_t keepalive_timeout;
  zb_bool_t keepalive_timeout_is_set;

  zb_uint_t ed_timeout;
  zb_bool_t ed_timeout_is_set;

  zb_uint8_t installcode[ZB_CCM_KEY_SIZE+ZB_CCM_KEY_CRC_SIZE];
  zb_uint8_t installcode_type;
  zb_bool_t allow_ic_only;

  zb_bool_t tc_policy_tc_link_keys_required;

#ifdef ZB_LIMIT_VISIBILITY
  zb_uint16_t mac_invisible_shorts[MAC_MAX_INVISIBLE_ADDRESSES_COUNT];
  zb_uint8_t mac_free_invisible_short_entry;

  zb_ieee_addr_t mac_visible_longs[MAC_MAX_VISIBLE_ADDRESSES_COUNT];
  zb_uint8_t mac_free_visible_long_entry;
#endif /* ZB_LIMIT_VISIBILITY */
} ncp_start_params_t;

typedef struct soc_state_s
{
  /* PAN ID is also a part of the SoC state but stored in ncp_start_params_t as it is a
     start parameter as well */
  zb_aps_group_table_t group_table;
  zb_ext_pan_id_t extended_pan_id;
  zb_ieee_addr_t trust_center_address;
  zb_uint16_t short_addr;
  zb_bool_t authenticated;
  zb_bool_t joined;
  zb_bool_t tclk_valid; /* could be a computed value actually if we have APS keys cache */
  zb_bool_t waiting_for_tclk;
  zb_uint8_t current_page;
  zb_uint8_t current_channel;
  zb_uint16_t parent_short_address;
  zb_uint8_t coordinator_version;
  zb_uint32_t fw_version;
  zb_uint32_t stack_version;
  zb_uint32_t ncp_protocol_version;
} soc_state_t;


typedef struct ncp_host_ctx_s
{
  ncp_start_params_t start_params;
  soc_state_t soc_state;
  zb_bool_t zboss_started;
  zb_bool_t is_rejoin_active;
} ncp_host_ctx_t;

static ncp_host_ctx_t host_ctx;


void ncp_host_state_init(void)
{
  ZB_BZERO(&host_ctx, sizeof(host_ctx));
  host_ctx.start_params.rx_on_when_idle = 0xFF;
  host_ctx.start_params.device_type = ZB_NWK_DEVICE_TYPE_NONE;
  host_ctx.start_params.max_children = ZB_DEFAULT_MAX_CHILDREN;
  host_ctx.start_params.installcode_type = ZB_IC_TYPE_MAX;
  host_ctx.start_params.tc_policy_tc_link_keys_required = ZB_TRUE;
}


zb_ret_t ncp_host_state_validate_start_state(void)
{
  zb_bool_t ret = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO2, ">> ncp_host_state_validate_start_state", (FMT__0));

  do
  {
    if (!ZB_IEEE_ADDR_IS_VALID(host_ctx.start_params.long_addr))
    {
      TRACE_MSG(TRACE_ERROR, "long address is not set or invalid: " TRACE_FORMAT_64,
                (FMT__A, host_ctx.start_params.long_addr));
      break;
    }

    if (host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_NONE)
    {
      TRACE_MSG(TRACE_ERROR, "device role is not set", (FMT__0));
      break;
    }

    if (host_ctx.start_params.rx_on_when_idle == 0xFF)
    {
      if (host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_ED)
      {
        host_ctx.start_params.rx_on_when_idle = ZB_FALSE;
      }
      else
      {
        host_ctx.start_params.rx_on_when_idle = ZB_TRUE;
      }
      TRACE_MSG(TRACE_ZDO2, "rx-on-when-idle is not set, default value will be used: %d",
                (FMT__D, host_ctx.start_params.rx_on_when_idle));
    }

    ret = RET_OK;
  } while(0);

  TRACE_MSG(TRACE_ZDO2, "<< ncp_host_state_validate_start_state, ret %d", (FMT__D, ret));

  return ret;
}


/* ncp_host_* getters and setters are intended for internal use only */
zb_bool_t ncp_host_state_get_nvram_erase(void)
{
  return host_ctx.start_params.nvram_erase;
}


void ncp_host_state_get_long_addr(zb_ieee_addr_t address)
{
  ZB_IEEE_ADDR_COPY(address, host_ctx.start_params.long_addr);
}


void ncp_host_state_set_long_addr(const zb_ieee_addr_t address)
{
  ZB_IEEE_ADDR_COPY(host_ctx.start_params.long_addr, address);
}


zb_bool_t ncp_host_state_get_nwk_key(zb_uint8_t* key, zb_uint8_t key_index)
{
  if (host_ctx.start_params.secur_material_specified[key_index])
  {
    ZB_MEMCPY(key, host_ctx.start_params.secur_material_set[key_index].key, ZB_CCM_KEY_SIZE);

    return ZB_TRUE;
  }
  else
  {
    return ZB_FALSE;
  }
}


void ncp_host_state_get_use_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
  ZB_EXTPANID_COPY(ext_pan_id, host_ctx.start_params.use_extended_pan_id);
}


zb_uint16_t ncp_host_state_get_pan_id(void)
{
  return host_ctx.start_params.pan_id;
}


zb_nwk_device_type_t ncp_host_state_get_device_type(void)
{
  return host_ctx.start_params.device_type;
}


zb_uint8_t ncp_host_state_get_max_children(void)
{
  return host_ctx.start_params.max_children;
}


void ncp_host_state_get_channel_list(zb_channel_list_t channel_list)
{
  zb_channel_page_list_copy(channel_list, host_ctx.start_params.channel_list);
}


void ncp_host_state_set_channel_list(zb_channel_list_t channel_list)
{
  zb_channel_page_list_copy(host_ctx.start_params.channel_list, channel_list);
}


zb_bool_t ncp_host_state_get_rx_on_when_idle(void)
{
  return host_ctx.start_params.rx_on_when_idle;
}


zb_bool_t ncp_host_state_get_permit_control4_network(void)
{
  return host_ctx.start_params.permit_control4_network;
}


zb_bool_t ncp_host_state_get_zboss_started(void)
{
  return host_ctx.zboss_started;
}


zb_bool_t ncp_host_state_get_joined(void)
{
  return host_ctx.soc_state.joined;
}


void ncp_host_state_set_zboss_started(zb_bool_t started)
{
  host_ctx.zboss_started = started;
}


void ncp_host_state_set_joined(zb_bool_t joined)
{
  host_ctx.soc_state.joined = joined;
}


void ncp_host_state_set_authenticated(zb_bool_t authenticated)
{
  host_ctx.soc_state.authenticated = authenticated;
}


void ncp_host_state_set_tclk_valid(zb_bool_t tclk_valid)
{
  host_ctx.soc_state.tclk_valid = tclk_valid;
}


void ncp_host_state_set_waiting_for_tclk(zb_bool_t waiting_for_tclk)
{
  host_ctx.soc_state.waiting_for_tclk = waiting_for_tclk;
}


void ncp_host_state_set_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_state_set_extended_pan_id, addr " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ext_pan_id)));

  ZB_EXTPANID_COPY(host_ctx.soc_state.extended_pan_id, ext_pan_id);
}


void ncp_host_state_set_current_channel(zb_uint8_t channel)
{
  host_ctx.soc_state.current_channel = channel;
}


void ncp_host_state_set_current_page(zb_uint8_t page)
{
  host_ctx.soc_state.current_page = page;
}


void ncp_host_state_set_short_address(zb_uint16_t address)
{
  host_ctx.soc_state.short_addr = address;
}


void ncp_host_state_set_parent_short_address(zb_uint16_t address)
{
  host_ctx.soc_state.parent_short_address = address;
}


void ncp_host_state_set_pan_id(zb_uint16_t pan_id)
{
  host_ctx.start_params.pan_id = pan_id;
}


void ncp_host_state_set_coordinator_version(zb_uint8_t coordinator_version)
{
  host_ctx.soc_state.coordinator_version = coordinator_version;
}


zb_aps_group_table_t* ncp_host_state_get_group_table(void)
{
  return &host_ctx.soc_state.group_table;
}

void ncp_host_state_set_rejoin_active(zb_bool_t is_active)
{
  host_ctx.is_rejoin_active = is_active;
}

/* Set Local IEEE address */
void zb_set_long_address(const zb_ieee_addr_t addr)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_long_address, addr " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(addr)));

  /* DD: we don't support setting params after ZBOSS starts now.
     Rationale: it needs commands serialization and we don't want overcomplicate things right now.*/
  ZB_ASSERT(!host_ctx.zboss_started);

  ZB_IEEE_ADDR_COPY(host_ctx.start_params.long_addr, addr);

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_long_address", (FMT__0));
}


/* Set Trust Center Link Keys Required flag */
void zb_aib_tcpol_set_update_trust_center_link_keys_required(zb_bool_t enable)
{
  zb_ret_t ret = RET_BUSY;

  TRACE_MSG(TRACE_ZDO2, "zb_aib_tcpol_set_update_trust_center_link_keys_required, enable %d",
            (FMT__D, enable));

  if (host_ctx.zboss_started)
  {
    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_TC_LINK_KEYS_REQUIRED, enable);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    host_ctx.start_params.tc_policy_tc_link_keys_required = enable;
  }
}

zb_bool_t zb_aib_tcpol_get_allow_unsecure_tc_rejoins(void)
{
  /* TODO */
  return ZB_FALSE;
}


/* Set NWK key */
void zb_secur_setup_nwk_key(zb_uint8_t *key, zb_uint8_t i)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_secur_setup_nwk_key, index %hd", (FMT__H, i));

  /* DD: we don't support setting params after ZBOSS starts now.
     Rationale: it needs commands serialization and we don't want overcomplicate things right now.*/
  ZB_ASSERT(!host_ctx.zboss_started);

  ZB_MEMCPY(host_ctx.start_params.secur_material_set[i].key, key, ZB_CCM_KEY_SIZE);

  TRACE_MSG(TRACE_ZDO2, "<< zb_secur_setup_nwk_key", (FMT__0));
}


#ifdef ZB_SECURITY_INSTALLCODES

zb_ret_t zb_secur_ic_set(zb_uint8_t ic_type, zb_uint8_t *ic)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_SECUR1, ">> zb_secur_ic_set " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));

  /* NP: security layer is located on SoC, so install code validation will be performed there */
  ZB_MEMCPY(host_ctx.start_params.installcode, ic, ZB_IC_SIZE_BY_TYPE(ic_type)+ZB_CCM_KEY_CRC_SIZE);
  host_ctx.start_params.installcode_type = ic_type;

  TRACE_MSG(TRACE_SECUR1, "<< zb_secur_ic_set, ret %d", (FMT__D, ret));

  return ret;
}


void zb_set_installcode_policy(zb_bool_t allow_ic_only)
{
  zb_ret_t ret = RET_BUSY;

  TRACE_MSG(TRACE_ZDO2, "zb_set_installcode_policy, allow_ic_only %hd",
            (FMT__H, allow_ic_only));

  if (host_ctx.zboss_started)
  {
    ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_IC_REQUIRED, allow_ic_only);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    host_ctx.start_params.allow_ic_only = allow_ic_only;
  }
}

#endif /* ZB_SECURITY_INSTALLCODES */



void zb_get_long_address(zb_ieee_addr_t addr)
{
  ZB_IEEE_ADDR_COPY(addr, host_ctx.start_params.long_addr);

  TRACE_MSG(TRACE_ZDO2, "zb_get_long_address: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(addr)));
}


void zb_set_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_extended_pan_id, ext_pan_id " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ext_pan_id)));

  /* DD: we don't support setting params after ZBOSS starts now.
     Rationale: it needs commands serialization and we don't want overcomplicate things right now.*/
  ZB_ASSERT(!host_ctx.zboss_started);

  ZB_EXTPANID_COPY(host_ctx.start_params.use_extended_pan_id, ext_pan_id);

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_extended_pan_id", (FMT__0));
}


void zb_get_use_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
  ZB_EXTPANID_COPY(ext_pan_id, host_ctx.start_params.use_extended_pan_id);

  TRACE_MSG(TRACE_ZDO2, "zb_get_use_extended_pan_id: " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ext_pan_id)));
}


void zb_set_channel_mask(zb_uint32_t channel_mask)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_channel_mask, channel_mask 0x%x", (FMT__D, channel_mask));

  ZB_ASSERT(!host_ctx.zboss_started);

  zb_channel_list_init(host_ctx.start_params.channel_list);
  zb_channel_page_list_set_2_4GHz_mask(host_ctx.start_params.channel_list, channel_mask);

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_channel_mask", (FMT__0));
}


zb_uint32_t zb_get_channel_mask(void)
{
  zb_uint32_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_get_channel_mask", (FMT__0));

  ret = zb_channel_page_list_get_2_4GHz_mask(host_ctx.start_params.channel_list);

  TRACE_MSG(TRACE_ZDO2, "<< zb_get_channel_mask, ret 0x%x", (FMT__D, ret));
  return ret;
}


void zb_aib_get_channel_page_list(zb_channel_list_t list)
{
  zb_channel_page_list_copy(list, host_ctx.start_params.channel_list);
}


#ifndef ZB_COORDINATOR_ONLY
zb_uint8_t *zb_secur_ic_get_from_client_storage(zb_uint8_t* ic_type)
{
  zb_uint8_t *ic;

  if (host_ctx.start_params.installcode_type != ZB_IC_TYPE_MAX)
  {
    *ic_type = host_ctx.start_params.installcode_type;

    ic = host_ctx.start_params.installcode;
  }
  else
  {
    *ic_type = ZB_IC_TYPE_MAX;

    ic = NULL;
  }

  return ic;
}
#endif /* ZB_COORDINATOR_ONLY */


void zb_set_nwk_role_mode_common(zb_nwk_device_type_t device_type,
                                          zb_uint32_t channel_mask,
                                          zb_commissioning_type_t mode)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_nwk_role_mode_common", (FMT__0));
  TRACE_MSG(TRACE_ZDO2, "device_type %d, channel_mask 0x%x, mode %d",
            (FMT__D_D_D, device_type, channel_mask, mode));

  ZB_ASSERT(!host_ctx.zboss_started);

  /* Legacy commissioning is not supported for NCP! */
  ZB_ASSERT(mode != ZB_COMMISSIONING_CLASSIC);

  host_ctx.start_params.device_type = device_type;
  COMM_CTX().commissioning_type = mode;

  zb_set_channel_mask(channel_mask);
  if (mode == ZB_COMMISSIONING_BDB)
  {
    zb_set_bdb_primary_channel_set(channel_mask);
    zb_set_bdb_secondary_channel_set(channel_mask);
  }

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_nwk_role_mode_common", (FMT__0));
}


void zb_set_rx_on_when_idle(zb_bool_t rx_on)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_rx_on_when_idle, rx_on %d", (FMT__D, rx_on));

  ZB_ASSERT(!host_ctx.zboss_started);

  host_ctx.start_params.rx_on_when_idle = rx_on;

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_rx_on_when_idle", (FMT__0));
}


zb_bool_t zb_get_rx_on_when_idle(void)
{
  zb_bool_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_get_rx_on_when_idle", (FMT__0));

  ret = host_ctx.start_params.rx_on_when_idle;

  TRACE_MSG(TRACE_ZDO2, "<< zb_get_rx_on_when_idle, ret %d", (FMT__D, ret));
  return ret;
}


void zb_set_pan_id(zb_uint16_t pan_id)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_pan_id, pan_id 0x%x", (FMT__D, pan_id));

  ZB_ASSERT(!host_ctx.zboss_started);

  host_ctx.start_params.pan_id = pan_id;

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_pan_id", (FMT__0));
}


zb_uint16_t zb_get_pan_id(void)
{
  zb_uint16_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_get_pan_id", (FMT__0));

  ZB_ASSERT(host_ctx.zboss_started);

  ret = host_ctx.start_params.pan_id;

  TRACE_MSG(TRACE_ZDO2, "<< zb_get_pan_id, ret 0x%d", (FMT__D, ret));
  return ret;
}


void zb_permit_control4_network(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_permit_control4_network", (FMT__0));

  ZB_ASSERT(!host_ctx.zboss_started);

  host_ctx.start_params.permit_control4_network = 1;

  TRACE_MSG(TRACE_ZDO2, "<< zb_permit_control4_network", (FMT__0));
}


zb_bool_t zb_control4_network_permitted(void)
{
  zb_bool_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_control4_network_permitted", (FMT__0));

  ret = host_ctx.start_params.permit_control4_network;

  TRACE_MSG(TRACE_ZDO2, "<< zb_control4_network_permitted, ret %d", (FMT__D, ret));

  return ret;
}


void zb_set_nvram_erase_at_start(zb_bool_t erase)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_nvram_erase_at_start, erase %d", (FMT__D, erase));

  ZB_ASSERT(!host_ctx.zboss_started);

  host_ctx.start_params.nvram_erase = erase;

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_nvram_erase_at_start", (FMT__0));
}


zb_bool_t zb_aib_tcpol_get_is_distributed_security(void)
{
  zb_bool_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_aib_tcpol_get_is_distributed_security", (FMT__0));

  ret = host_ctx.start_params.is_distributed_security;

  TRACE_MSG(TRACE_ZDO2, "<< zb_aib_tcpol_get_is_distributed_security, ret %d", (FMT__D, ret));

  return ret;
}


void zb_aib_tcpol_set_is_distributed_security(zb_bool_t enable)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_aib_tcpol_set_is_distributed_security, enable: %d", (FMT__D, enable));

  host_ctx.start_params.is_distributed_security = enable;

  TRACE_MSG(TRACE_ZDO2, "<< zb_aib_tcpol_get_is_distributed_security", (FMT__0));
}


zb_uint8_t zb_aib_get_coordinator_version(void)
{
  zb_uint8_t ret;
  TRACE_MSG(TRACE_ZDO2, ">> zb_aib_get_coordinator_version", (FMT__0));

  ZB_ASSERT(host_ctx.zboss_started);

  ret = host_ctx.soc_state.coordinator_version;

  TRACE_MSG(TRACE_ZDO2, "<< zb_aib_get_coordinator_version, ret %d", (FMT__D, ret));
  return ret;
}


zb_uint32_t zb_aib_channel_page_list_get_2_4GHz_mask()
{
  return zb_channel_page_list_get_2_4GHz_mask(host_ctx.start_params.channel_list);
}


zb_uint8_t zb_aib_channel_page_list_get_first_filled_page(void)
{
  return zb_channel_page_list_get_first_filled_page(host_ctx.start_params.channel_list);
}


zb_uint32_t zb_aib_channel_page_list_get_mask(zb_uint8_t idx)
{
  return zb_channel_page_list_get_mask(host_ctx.start_params.channel_list, idx);
}


void zb_aib_set_trust_center_address(const zb_ieee_addr_t address)
{
  ZB_IEEE_ADDR_COPY(host_ctx.soc_state.trust_center_address, address);
}


void zb_aib_get_trust_center_address(zb_ieee_addr_t address)
{
  ZB_IEEE_ADDR_COPY(address, host_ctx.soc_state.trust_center_address);
}


zb_uint16_t zb_aib_get_trust_center_short_address(void)
{
#ifndef ZB_COORDINATOR_ONLY
  return zb_address_short_by_ieee(host_ctx.soc_state.trust_center_address);
#else
  return 0x0000;
#endif /* ZB_COORDINATOR_ONLY */
}


zb_bool_t zb_aib_trust_center_address_zero(void)
{
  return ZB_IEEE_ADDR_IS_ZERO(host_ctx.soc_state.trust_center_address);
}


zb_bool_t zb_aib_trust_center_address_unknown(void)
{
  return ZB_IEEE_ADDR_IS_UNKNOWN(host_ctx.soc_state.trust_center_address);
}


zb_bool_t zb_aib_trust_center_address_cmp(const zb_ieee_addr_t address)
{
  return ZB_IEEE_ADDR_CMP(host_ctx.soc_state.trust_center_address, address);
}


zb_nwk_device_type_t zb_get_device_type(void)
{
  return host_ctx.start_params.device_type;
}


zb_bool_t zb_is_device_zc(void)
{
  return (zb_bool_t)(host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR);
}


zb_bool_t zb_is_device_zr(void)
{
  return (zb_bool_t)(host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_ROUTER);
}


zb_bool_t zb_is_device_zed(void)
{
  return (zb_bool_t)(host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_ED);
}


zb_bool_t zb_is_device_zc_or_zr(void)
{
  return (zb_bool_t)(host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR
                     || host_ctx.start_params.device_type == ZB_NWK_DEVICE_TYPE_ROUTER);
}


void zb_set_max_children(zb_uint8_t max_children)
{
  TRACE_MSG(TRACE_ZDO2, ">> zb_set_max_children: %d", (FMT__D, max_children));

  ZB_ASSERT(!host_ctx.zboss_started);

  host_ctx.start_params.max_children = max_children;

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_max_children", (FMT__0));
}


zb_uint8_t zb_get_max_children(void)
{
  zb_uint8_t ret = 0;
  TRACE_MSG(TRACE_ZDO2, ">> zb_get_max_children", (FMT__0));

  ret = host_ctx.start_params.max_children;

  TRACE_MSG(TRACE_ZDO2, "<< zb_set_max_children, ret: %d", (FMT__D, ret));
  return ret;
}


zb_uint8_t* ncp_host_state_get_ic(void)
{
  return host_ctx.start_params.installcode;
}


zb_uint8_t ncp_host_state_get_ic_type(void)
{
  return host_ctx.start_params.installcode_type;
}


void ncp_host_state_set_ic(zb_uint8_t ic_type, const zb_uint8_t *ic)
{
  ZB_MEMCPY(host_ctx.start_params.installcode, ic, ZB_IC_SIZE_BY_TYPE(ic_type)+ZB_CCM_KEY_CRC_SIZE);
  host_ctx.start_params.installcode_type = ic_type;
}


zb_bool_t ncp_host_state_get_allow_ic_only(void)
{
  return host_ctx.start_params.allow_ic_only;
}


zb_bool_t ncp_host_state_get_tc_policy_tc_link_keys_required(void)
{
  return host_ctx.start_params.tc_policy_tc_link_keys_required;
}


void ncp_host_state_set_keepalive_timeout(zb_uint_t timeout)
{
  host_ctx.start_params.keepalive_timeout = timeout;
  host_ctx.start_params.keepalive_timeout_is_set = ZB_TRUE;
}


zb_uint_t ncp_host_state_get_keepalive_timeout(void)
{
  return host_ctx.start_params.keepalive_timeout;
}


zb_bool_t ncp_host_state_keepalive_timeout_is_set(void)
{
  return host_ctx.start_params.keepalive_timeout_is_set;
}


void ncp_host_state_set_ed_timeout(zb_uint_t timeout)
{
  host_ctx.start_params.ed_timeout = timeout;
  host_ctx.start_params.ed_timeout_is_set = ZB_TRUE;
}


zb_uint_t ncp_host_state_get_ed_timeout(void)
{
  return host_ctx.start_params.ed_timeout;
}


zb_bool_t ncp_host_state_ed_timeout_is_set(void)
{
  return host_ctx.start_params.ed_timeout_is_set;
}


void ncp_host_state_set_keepalive_mode(nwk_keepalive_supported_method_t mode)
{
  host_ctx.start_params.keepalive_mode = mode;
  host_ctx.start_params.keepalive_mode_is_set = ZB_TRUE;
}


nwk_keepalive_supported_method_t ncp_host_state_get_keepalive_mode(void)
{
  return host_ctx.start_params.keepalive_mode;
}

zb_bool_t ncp_host_state_keepalive_mode_is_set(void)
{
  return host_ctx.start_params.keepalive_mode_is_set;
}

/**
   Check that device is joined.

   It must be a function to exclude exposing of ZDO globals via user's API.
 */
zb_bool_t zb_zdo_joined(void)
{
  ZB_ASSERT(host_ctx.zboss_started);
  /* NOTE: it should be set on startup:
      In case of first start: ZB_FALSE by default and ZB_TRUE after successful formation/join.
      in case of NFN start: get this value from SoC somehow.
  */
  return host_ctx.soc_state.joined;
}


zb_bool_t zb_zdo_is_rejoin_active(void)
{
  return host_ctx.is_rejoin_active;
}


zb_bool_t zb_zdo_authenticated(void)
{
  ZB_ASSERT(host_ctx.zboss_started);
  /* NOTE: it should be set on startup:
      In case of first start: ZB_FALSE by default and ZB_TRUE after successful formation/join.
      in case of NFN start: get this value from SoC somehow.
  */
  return host_ctx.soc_state.authenticated;
}


zb_bool_t zb_zdo_tclk_valid(void)
{
  ZB_ASSERT(host_ctx.zboss_started);
  return host_ctx.soc_state.tclk_valid;
}


zb_uint16_t zb_get_short_address(void)
{
  zb_uint16_t ret;
  ZB_ASSERT(host_ctx.zboss_started);
  ret = host_ctx.soc_state.short_addr;
  TRACE_MSG(TRACE_ZDO2, "zb_get_short_address: 0x%x", (FMT__D, ret));
  return ret;
}


void zb_get_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
  ZB_ASSERT(host_ctx.zboss_started);
  ZB_EXTPANID_COPY(ext_pan_id, host_ctx.soc_state.extended_pan_id);

  TRACE_MSG(TRACE_ZDO2, "zb_get_extended_pan_id: " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ext_pan_id)));
}


zb_uint8_t zb_get_current_page(void)
{
  zb_uint8_t ret;
  ZB_ASSERT(host_ctx.zboss_started);
  ret = host_ctx.soc_state.current_page;
  TRACE_MSG(TRACE_ZDO2, "zb_get_current_page: %d", (FMT__D, ret));
  return ret;
}


zb_uint8_t zb_get_current_channel(void)
{
  zb_uint8_t ret;
  ZB_ASSERT(host_ctx.zboss_started);
  ret = host_ctx.soc_state.current_channel;
  TRACE_MSG(TRACE_ZDO2, "zb_get_current_channel: %d", (FMT__D, ret));
  return ret;
}


zb_uint16_t zb_nwk_get_parent(void)
{
  zb_uint16_t ret;
  ZB_ASSERT(host_ctx.zboss_started);
  ret = host_ctx.soc_state.parent_short_address;
  TRACE_MSG(TRACE_ZDO2, "zb_nwk_get_parent: 0x%x", (FMT__D, ret));
  return ret;
}

void ncp_host_state_set_module_version(zb_uint32_t fw_version,
                                       zb_uint32_t stack_version,
                                       zb_uint32_t ncp_version)
{
  host_ctx.soc_state.fw_version = fw_version;
  host_ctx.soc_state.stack_version = stack_version;
  host_ctx.soc_state.ncp_protocol_version = ncp_version;
}

zb_uint32_t ncp_host_get_firmware_version(void)
{
  return host_ctx.soc_state.fw_version;
}

zb_uint32_t ncp_host_state_get_stack_version(void)
{
  return host_ctx.soc_state.stack_version;
}

zb_uint32_t ncp_host_state_get_ncp_protocol_version(void)
{
  return host_ctx.soc_state.ncp_protocol_version;
}


#ifdef ZB_JOIN_CLIENT
zb_bool_t zdo_secur_waiting_for_tclk_update(void)
{
  ZB_ASSERT(host_ctx.zboss_started);
  return host_ctx.soc_state.waiting_for_tclk;
}
#endif  /* ZB_JOIN_CLIENT */


#ifdef ZB_LIMIT_VISIBILITY
void mac_add_invisible_short(zb_uint16_t addr)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> mac_add_invisible_short", (FMT__0));

  if (ncp_host_state_get_zboss_started())
  {
    ret = ncp_host_mac_add_invisible_short(addr);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_ASSERT(host_ctx.start_params.mac_free_invisible_short_entry < MAC_MAX_INVISIBLE_ADDRESSES_COUNT);

    host_ctx.start_params.mac_invisible_shorts[host_ctx.start_params.mac_free_invisible_short_entry] = addr;
    host_ctx.start_params.mac_free_invisible_short_entry++;
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< mac_add_invisible_short, ret %d", (FMT__D, ret));
}


void mac_add_visible_device(zb_ieee_addr_t long_addr)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT2, ">> mac_add_visible_device", (FMT__0));

  if (ncp_host_state_get_zboss_started())
  {
    ret = ncp_host_mac_add_visible_long(long_addr);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_ASSERT(host_ctx.start_params.mac_free_visible_long_entry < MAC_MAX_VISIBLE_ADDRESSES_COUNT);

    ZB_IEEE_ADDR_COPY(host_ctx.start_params.mac_visible_longs[host_ctx.start_params.mac_free_visible_long_entry],
      long_addr);
    host_ctx.start_params.mac_free_visible_long_entry++;
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< mac_add_visible_device, ret %d", (FMT__D, ret));
}


zb_uint16_t *ncp_host_state_get_mac_invisible_shorts(zb_uint8_t *addresses_count)
{
  *addresses_count = host_ctx.start_params.mac_free_invisible_short_entry;

  return host_ctx.start_params.mac_invisible_shorts;
}


zb_ieee_addr_t *ncp_host_state_get_mac_visible_longs(zb_uint8_t *addresses_count)
{
  *addresses_count = host_ctx.start_params.mac_free_visible_long_entry;

  return host_ctx.start_params.mac_visible_longs;
}
#endif /* ZB_LIMIT_VISIBILITY */
