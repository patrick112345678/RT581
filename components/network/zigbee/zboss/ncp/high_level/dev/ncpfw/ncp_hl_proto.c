/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2021 DSR Corporation, Denver CO, USA.
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
/*     PURPOSE: NCP high-level protocol. NCP side.
*/

#define ZB_TRACE_FILE_ID 14001
#include "zb_config.h"
#include "zb_common.h"
#include "zb_ncp_internal.h"
#include "zb_mac_globals.h"
#include "mac_internal.h"
#include "zb_nwk.h"
#include "ncp_hl_proto.h"
#include "zb_ncp.h"
#include "zb_aps.h"
#include "zb_secur.h"
#include "zb_address.h"
#include "zboss_api.h"
#include "zboss_api_zdo.h"
#include "zb_magic_macros.h"
#include "ncp_manufacture_mode.h"
#include "zdo_wwah_stubs.h"
#include "zdo_diagnostics.h"
#include "zboss_api_aps_interpan.h"

#include "zgp/zgp_internal.h"

#ifdef SNCP_MODE
#include "sncp_version.h"
#endif

#ifndef NCP_MODE
//#error This app is to be compiled as NCP application FW
#endif

/* FIXME: May have more appropriate code than RET_OPERATION_FAILED if key reading is blocked? */
#define NCP_RET_KEYS_LOCKED  RET_OPERATION_FAILED

#define JOIN_RESP_SIZE (2 + 8 + 1 + 1 + 1 + 1)

static inline zb_ret_t ncp_zdo_status_to_retcode(zb_uint8_t status)
{
  zb_ret_t ret = RET_OK;;
  if (status != (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, (zb_ret_t)status);
  }
  return ret;
}

typedef struct ncp_hl_body_s
{
  zb_uint8_t *ptr;
  zb_size_t len;
  zb_size_t pos;
}
ncp_hl_body_t;

static inline ncp_hl_body_t ncp_hl_body(zb_uint8_t *hdr, zb_size_t hsize, zb_size_t len)
{
  ncp_hl_body_t body = { NULL, 0, 0 };
  if (len >= hsize)
  {
    body.ptr = hdr + hsize;
    body.len = len - hsize;
  }
  return body;
}

static inline ncp_hl_body_t ncp_hl_request_body(ncp_hl_request_header_t *hdr, zb_size_t rxlen)
{
  return ncp_hl_body((zb_uint8_t*)hdr, sizeof(*hdr), rxlen);
}

static inline ncp_hl_body_t ncp_hl_response_body(ncp_hl_response_header_t *hdr, zb_size_t txlen)
{
  return ncp_hl_body((zb_uint8_t*)hdr, sizeof(*hdr), txlen);
}

static inline void ncp_hl_body_get_u8(ncp_hl_body_t *body, zb_uint8_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  *val = body->ptr[body->pos];
  body->pos += vsize;
}

static inline void ncp_hl_body_get_u16(ncp_hl_body_t *body, zb_uint16_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_LETOH16(val, &body->ptr[body->pos]);
  body->pos += vsize;
}

static inline void ncp_hl_body_get_u32(ncp_hl_body_t *body, zb_uint32_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_LETOH32(val, &body->ptr[body->pos]);
  body->pos += vsize;
}

static inline void ncp_hl_body_get_u64(ncp_hl_body_t *body, zb_uint64_t *val)
{
  zb_size_t vsize = sizeof(*val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_LETOH64(val, &body->ptr[body->pos]);
  body->pos += vsize;
}

static inline void ncp_hl_body_get_u64addr(ncp_hl_body_t *body, zb_64bit_addr_t addr)
{
  zb_size_t asize = sizeof(zb_64bit_addr_t);

  ZB_ASSERT(body->pos + asize <= body->len);

  ZB_MEMCPY(addr, &body->ptr[body->pos], asize);
  body->pos += asize;
}

static inline void ncp_hl_body_get_ptr(ncp_hl_body_t *body, zb_uint8_t **ptr, zb_size_t size)
{
  ZB_ASSERT(body->pos + size <= body->len);

  *ptr = &body->ptr[body->pos];
  body->pos += size;
}

static inline void ncp_hl_body_put_u8(ncp_hl_body_t *body, zb_uint8_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  body->ptr[body->pos] = val;
  body->pos += vsize;
}

static inline void ncp_hl_body_put_u16(ncp_hl_body_t *body, zb_uint16_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_HTOLE16(&body->ptr[body->pos], &val);
  body->pos += vsize;
}

static inline void ncp_hl_body_put_u32(ncp_hl_body_t *body, zb_uint32_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_HTOLE32(&body->ptr[body->pos], &val);
  body->pos += vsize;
}

static inline void ncp_hl_body_put_u64(ncp_hl_body_t *body, zb_uint64_t val)
{
  zb_size_t vsize = sizeof(val);

  ZB_ASSERT(body->pos + vsize <= body->len);

  ZB_HTOLE64(&body->ptr[body->pos], &val);
  body->pos += vsize;
}

static inline void ncp_hl_body_put_u64addr(ncp_hl_body_t *body, zb_64bit_addr_t addr)
{
  zb_size_t asize = sizeof(zb_64bit_addr_t);

  ZB_ASSERT(body->pos + asize <= body->len);

  ZB_MEMCPY(&body->ptr[body->pos], addr, asize);
  body->pos += asize;
}

static inline void ncp_hl_body_put_array(ncp_hl_body_t *body, zb_uint8_t *array, zb_uint8_t length)
{
  zb_size_t asize = length;

  ZB_ASSERT(body->pos + asize <= body->len);

  ZB_MEMCPY(&body->ptr[body->pos], array, asize);
  body->pos += asize;
}

static inline zb_ret_t ncp_hl_body_check_len(const ncp_hl_body_t *body, zb_size_t expected)
{
  zb_ret_t ret = RET_OK;
  if (body->ptr == NULL || body->len < expected)
  {
    ret = RET_INVALID_FORMAT;
  }
  return ret;
}

static void ncp_hl_packet_received(void *data, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_configuration(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_af(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_zdo(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_aps(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_nwkmgmt(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t handle_ncp_req_secur(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static void handle_parent_lost(zb_uint8_t count);

#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
static zb_uint16_t handle_ncp_req_intrp(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t intrp_data_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */

#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
static zb_uint16_t handle_ncp_req_manuf_test(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */

#ifdef ZB_NCP_ENABLE_OTA_CMD
static zb_uint16_t handle_ncp_req_ota(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_NCP_ENABLE_OTA_CMD */

static void nwk_addr_rsp(zb_uint8_t param);
static void nwk_addr_ext_rsp(zb_uint8_t param);
static void ieee_addr_rsp(zb_uint8_t param);
static void ieee_addr_ext_rsp(zb_uint8_t param);
static void power_desc_rsp(zb_uint8_t param);
static void node_desc_rsp(zb_uint8_t param);
static void simple_desc_rsp(zb_uint8_t param);
static void active_ep_desc_rsp(zb_uint8_t param);
static void match_desc_rsp(zb_uint8_t param);
static void zdo_system_srv_discovery_rsp(zb_uint8_t param);
static void zdo_set_node_desc_manuf_code_rsp(zb_ret_t status);
static void bind_rsp(zb_uint8_t param);
static void unbind_rsp(zb_uint8_t param);
static void ncp_zdo_mgmt_leave_rsp(zb_uint8_t param);
static void ncp_zdo_mgmt_bind_rsp(zb_uint8_t param);
static void ncp_zdo_mgmt_lqi_rsp(zb_uint8_t param);
static void ncp_zdo_mgmt_nwk_update_rsp(zb_uint8_t param);

static zb_ret_t ncp_alloc_buf(zb_uint8_t *buf);
static zb_ret_t ncp_alloc_buf_size(zb_bufid_t *buf, zb_size_t size);
static zb_uint16_t set_channel_mask(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_channel_mask(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_local_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_trace(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_trace(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_current_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_local_ieee_addr(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_ed_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_ed_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len);
/*
static zb_uint16_t get_keepalive_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_keepalive_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len);
*/
static zb_uint16_t get_rx_on_when_idle(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_rx_on_when_idle(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_joined(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_authenticated(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t add_visible_dev(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t add_invisible_short(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t rm_invisible_short(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_nwk_key(ncp_hl_request_header_t *hdr, zb_uint16_t len);

#ifndef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
static zb_uint16_t cfg_get_serial_number(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t cfg_get_vendor_data(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */

static zb_uint16_t get_nwk_keys(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_aps_key_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_parent_address(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_extended_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_extended_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_coordinator_version(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_short_address(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_trust_center_address(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_tx_power(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_tx_power(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_config_parameter(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#ifdef ZB_HAVE_BLOCK_KEYS
static zb_uint16_t get_lock_status(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif
static zb_uint16_t get_nwk_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_nwk_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);

#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
static zb_uint16_t production_config_read(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif

static zb_uint16_t nvram_write(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t nvram_read(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t nvram_erase(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t nvram_clear(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_tc_policy(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_zdo_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_zdo_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#ifdef ZB_ENABLE_ZGP
static zb_uint_t disable_gppb(ncp_hl_request_header_t *hdr, zb_uint8_t len);
static zb_uint_t gp_set_shared_key_type(ncp_hl_request_header_t *hdr, zb_uint8_t len);
static zb_uint_t gp_set_default_link_key(ncp_hl_request_header_t *hdr, zb_uint8_t len);
#endif
static zb_uint16_t set_leave_without_rejoin_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_leave_without_rejoin_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len);


static zb_uint16_t nwk_formation(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t reset_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_zigbee_role(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_zigbee_role(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_max_children(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_max_children(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t nwk_permit_joining(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ncp_hl_nwk_start_without_formation(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t nwk_discovery(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsde_data_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_unbind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_unbind_all_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_binding_req(ncp_hl_request_header_t *hdr, zb_uint16_t len, zb_uint16_t type);
static zb_uint16_t apsme_add_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_rm_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t apsme_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len, zb_uint16_t type);
static zb_uint16_t apsme_rm_all_groups_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t aps_check_binding_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t aps_get_group_table_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);


static zb_uint16_t nwk_join(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static zb_uint16_t nwk_nlme_router_start_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);

#if !defined SNCP_MODE
static void set_pan_id_confirm(zb_uint8_t param);
#endif
static void ncp_hl_send_no_arg_resp(ncp_hl_call_code_t req, zb_ret_t status);
static void ncp_hl_send_join_ok_resp(zb_uint8_t param);
static void store_tsn(zb_uint8_t buf, zb_uint8_t tsn, zb_uint16_t call_id);
static void free_tsn(zb_uint8_t buf);
static void ncp_hl_send_formation_response(zb_ret_t status);

static zb_uint16_t big_pkt_to_ncp(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static zb_uint16_t secur_set_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_ic_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_add_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_del_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_join_uses_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_local_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_ic_list(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_ic_by_idx(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_remove_all_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_key_idx(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_key(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_erase_key(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_clear_key_table(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_ret_t get_ic_type_by_len(zb_uint8_t ic_len, zb_uint8_t* ic_type);

static void ncp_hl_secur_tclk_ind(void);
static void ncp_hl_secur_tclk_exchange_failed_ind(zb_ret_t status);

#ifdef ZB_ENABLE_SE_MIN_CONFIG
static zb_uint16_t secur_add_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_del_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_get_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_start_cbke(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static void secur_cbke_done(zb_ret_t status);
static void secur_tc_cbke_ind(zb_address_ieee_ref_t addr_ref, zb_ret_t status);
static zb_uint16_t secur_start_partner_lk(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_partner_lk_enable(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#ifdef ZB_SE_KE_WHITELIST
static zb_uint16_t secur_add_to_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_del_from_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t secur_del_all_from_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_SE_KE_WHITELIST */
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

static zb_uint16_t secur_initiate_nwk_key_switch_procedure(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static void convert_nbt_to_ncp(const zb_neighbor_tbl_ent_t *nbt, ncp_hl_response_neighbor_by_ieee_t *ncp_nbt);

static zb_uint16_t get_address_ieee_by_short(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_address_short_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_neighbor_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_long_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_fast_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t start_fast_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t stop_fast_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t start_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t stop_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t enable_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t disable_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_first_nbt_entry(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_next_nbt_entry(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_ret_t get_nbt_entry(ncp_hl_response_neighbor_by_ieee_t *ncp_nbt, zb_bool_t reset);
static zb_uint16_t ncp_hl_nwk_pan_id_conflict_resolve(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t enable_pan_id_conflict_resolution(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t enable_auto_pan_id_conflict_resolution(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static zb_uint16_t start_turbo_poll_packets(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t start_turbo_poll_continuous(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t turbo_poll_cancel_packet(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t turbo_poll_continuous_leave(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t turbo_poll_packets_leave(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t permit_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_long_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t get_in_fast_poll_flag(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_keepalive_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t start_concentrator_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t stop_concentrator_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_fast_poll_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static zb_uint16_t nwk_addr_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ieee_addr_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t power_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t node_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t simple_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t active_ep_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t match_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t zdo_system_srv_discovery_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t zdo_set_node_desc_manuf_code_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t unbind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ncp_zdo_mgmt_leave_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ncp_zdo_mgmt_bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ncp_zdo_mgmt_lqi_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t ncp_zdo_mgmt_nwk_update_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t zdo_permit_joining(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t zdo_rejoin_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t zdo_get_stats_req(ncp_hl_request_header_t *hdr, zb_uint16_t len);

static void zdo_permit_joining_rsp(zb_uint8_t param);
static void zdo_permit_joining_rsp_local(zb_uint8_t param);

static zb_uint16_t set_simple_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t del_ep(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_node_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t set_power_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len);

#ifdef SNCP_MODE
static zb_uint16_t single_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif

#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
static zb_uint16_t manuf_mode_start(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_mode_end(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_set_page_and_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_get_page_and_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_set_power(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_get_power(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_start_tone(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_stop_tone(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_start_stream(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_stop_stream(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_send_packet(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_start_rx(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static zb_uint16_t manuf_stop_rx(ncp_hl_request_header_t *hdr, zb_uint16_t len);
static void manuf_send_packet_rsp(zb_uint8_t param);
static void manuf_rx_packet_ind(zb_uint8_t param);
static inline zb_ret_t ncp_hl_manuf_req_check(const ncp_hl_request_header_t *hdr, zb_size_t len, zb_size_t expected);
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */

#ifdef ZB_NCP_ENABLE_OTA_CMD
static zb_uint16_t ota_run_bootloader(ncp_hl_request_header_t *hdr, zb_uint16_t len);
#endif /* ZB_NCP_ENABLE_OTA_CMD */

#if defined ZB_NCP_ENABLE_CUSTOM_COMMANDS && defined SNCP_MODE
static zb_uint16_t handle_ncp_req_custom_commands(ncp_hl_request_header_t *hdr, zb_uint16_t len);
ZB_WEAK_PRE zb_uint16_t read_nvram_reserved(ncp_hl_request_header_t *hdr, zb_uint16_t len) ZB_WEAK;
ZB_WEAK_PRE zb_uint16_t write_nvram_reserved(ncp_hl_request_header_t *hdr, zb_uint16_t len) ZB_WEAK;
#endif /* ZB_NCP_ENABLE_CUSTOM_COMMANDS && SNCP_MODE */

static void schedule_reset(ncp_hl_reset_opt_t opt);
static void make_reset(zb_uint8_t param);

static void ncp_hl_nwk_join_ind(zb_uint8_t param);
static void ncp_hl_nwk_join_failed_ind(zb_ret_t status);
static void ncp_hl_send_rejoin_resp(zb_ret_t status);

static void apply_rx_on_when_idle(zb_bool_t rx_on);
static void send_data_or_zdo_indication(zb_bool_t is_zdo, zb_uint8_t param);

#define ZB_NCP_MAX_PACKET_SIZE 0x2000u
#define NCP_RET_LATER (255U)
#define TSN_RESERVED (0xFFU)
#define ZDO_MAX_ENTRIES  (ZDO_TRAN_TABLE_SIZE)

#define SIMPLE_DESC_ALLOC_FIXED_PART sizeof(zb_af_simple_desc_1_1_t)
#define SIMPLE_DESC_ALLOC_CLUSTERS_PART (20U * sizeof(zb_uint16_t)) /* suppose 10+10 clusiers */
#define SPACE_FOR_SIMPLE_DESCS (SIMPLE_DESC_ALLOC_FIXED_PART + SIMPLE_DESC_ALLOC_CLUSTERS_PART * (ZB_MAX_EP_NUMBER - 1U))

typedef struct zb_ncp_hl_zdo_entries_s
{
  zb_uint8_t zb_tsn;
  zb_uint8_t ncp_tsn;
  zb_bool_t wait_for_timeout;
}
zb_ncp_hl_zdo_entries_t;

static zb_ncp_hl_zdo_entries_t zdo_entries[ZDO_MAX_ENTRIES];

typedef struct zb_ncp_simple_desc_meta_s
{
  zb_uint8_t size;
}
zb_ncp_simple_desc_meta_t;

typedef struct zb_ncp_simple_descs_pool_s
{
  /* uint32 to force alignment */
  zb_uint32_t mem[SPACE_FOR_SIMPLE_DESCS/4U];
  zb_uint_t used;
  zb_ncp_simple_desc_meta_t meta[ZB_MAX_EP_NUMBER];
  zb_uint8_t cnt;
}
zb_ncp_simple_descs_pool_t;

typedef enum zb_ncp_mode_e
{
  ZB_NCP_MODE_DEFAULT,
  ZB_NCP_MODE_MANUFACTURE
}
zb_ncp_mode_t;

typedef struct
{
  zb_uint8_t tx_buf[ZB_NCP_MAX_PACKET_SIZE];
  zb_uint16_t single_cmds[256];
  zb_uint8_t single_tsn;
  zb_uint8_t work_buf;
  zb_uint8_t parnter_lk_in_progress;
  zb_ncp_simple_descs_pool_t simple_descs_pool;
  zb_ncp_mode_t mode;
  char serial_number[ZB_NCP_SERIAL_NUMBER_LENGTH];
  zb_uint8_t vendor_data_size;
  zb_uint8_t vendor_data[ZB_NCP_MAX_VENDOR_DATA_SIZE];
  zb_bool_t just_boot;
  ncp_hl_call_code_t start_command; /* to distinguish between join, rejoin, start w/o formation
                                       and so on requests */
#ifdef SNCP_MODE
  /* Field for providing the functionality: Single poll command from Host
   *   set when host issue PIM_STOP_POLL cmd, and reset when cmds is
   *   PIM_START_POLL, PIM_START_FAST_POLL or PIM_ENABLE_TURBO_POLL */
  zb_uint8_t poll_aps_tx:4;
  zb_bitbool_t poll_aps_rx:1;
  zb_bitbool_t poll_stopped_from_host:1;

  zb_bitbool_t poll_cbke:1;
  zb_bitbool_t poll_ed_timeout:1;
#endif
  zb_bitfield_t parent_lost:1;  /* set by appropriate indication from stack,
                                        reset by ncp_hl_nwk_leave_itself(), handle_join_rejoin_signal()  */
#ifdef ZB_HAVE_CALIBRATION
  /* If there is a crystal calibration function,
   *   1 - calibration is successful,
   *   0 - failed or was not performed
   */
  zb_bitfield_t calibration_status:1;
#endif
#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
  zb_bitbool_t tc_swapped:1;  /* set on ZB_TC_SWAPPED_SIGNAL, clear on rejoin indication */
#endif
}
zb_ncp_fw_ctx_t;

static zb_ncp_fw_ctx_t g_ncp_ctx;
#define NCP_CTX g_ncp_ctx
#define SIMPLE_DESCS_POOL NCP_CTX.simple_descs_pool


/* all these routines for storing and finding ncp_tsn by zb_tsn */
static zb_ncp_hl_zdo_entries_t* ncp_zdo_util_get_entry_by_zb_tsn(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t zb_tsn);
static zb_ret_t ncp_zdo_util_get_new_entry(zb_ncp_hl_zdo_entries_t *entries,
                                           zb_ncp_hl_zdo_entries_t **entry);
static void ncp_zdo_util_init_entry(zb_ncp_hl_zdo_entries_t * entry,
                                    zb_uint8_t zb_tsn,
                                    zb_uint8_t ncp_tsn,
                                    zb_bool_t wait_for_timeout);
static zb_uint8_t ncp_zdo_util_get_idx_by_zb_tsn(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t zb_tsn);
static void ncp_zdo_util_clear_entry_by_idx(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t idx);
static void ncp_zdo_util_clear_entry(zb_ncp_hl_zdo_entries_t *entry);
static void ncp_zdo_util_clear_all_entries(zb_ncp_hl_zdo_entries_t *entries);

static void ncp_hl_fill_join_resp_body(void * b);

static zb_uint16_t ncp_hl_fill_ind_hdr(ncp_hl_ind_header_t **ih,  zb_uint_t ind_code,  zb_uint_t body_size);


ncp_ll_packet_received_cb_t ncp_hl_proto_init(void)
{
  NCP_CTX.mode = ZB_NCP_MODE_DEFAULT;
#ifdef SNCP_MODE
  NCP_CTX.poll_aps_rx = ZB_FALSE;
  NCP_CTX.poll_aps_tx = 0;
  NCP_CTX.poll_cbke = ZB_FALSE;
  NCP_CTX.poll_ed_timeout = ZB_FALSE;
#endif /* SNCP_MODE */
  ncp_zdo_util_clear_all_entries(zdo_entries);
  return ncp_hl_packet_received;
}

#ifdef SNCP_MODE

/* We have customer request to disable autiomatic ZBOSS poll just after join & authentication and after CBKE.
   During join/rejoin/cbke ZBOSS must poll automatically.
*/
#define SET_POLL_STOPPED_FROM_HOST(set_reset) (NCP_CTX.poll_stopped_from_host = (set_reset) ? ZB_TRUE : ZB_FALSE)

#define SNCP_SWITCH_POLL_ON() zb_zdo_pim_start_poll(0)
#define SNCP_SWITCH_POLL_OFF() sncp_auto_poll_off()

static void sncp_auto_poll_off(void)
{
  if ((NCP_CTX.poll_stopped_from_host) && (!NCP_CTX.poll_aps_rx) && (NCP_CTX.poll_aps_tx == 0U) && (!NCP_CTX.poll_cbke) && (!NCP_CTX.poll_ed_timeout))
  {
    zb_zdo_pim_stop_poll(0);
  }
}

void sncp_auto_turbo_poll_ed_timeout(zb_bool_t is_on)
{
#if TRACE_ENABLED(TRACE_ZDO3) || TRACE_ENABLED(TRACE_NWK3)
  TRACE_MSG(TRACE_INFO2, ">> sncp_auto_turbo_poll_ed_timeout param %hd poll_stopped_from_host %hd poll_ed_timeout %hu",
            (FMT__H_H_H, is_on, NCP_CTX.poll_stopped_from_host, NCP_CTX.poll_ed_timeout));
#endif
  /* if auto poll is disabled by Host, enable auto poll for ED Timeout NWK command */
  if (NCP_CTX.poll_stopped_from_host)
  {
    if (is_on)
    {
      NCP_CTX.poll_ed_timeout = ZB_TRUE;
      SNCP_SWITCH_POLL_ON();
#if TRACE_ENABLED(TRACE_ZDO3) || TRACE_ENABLED(TRACE_NWK3)
      TRACE_MSG(TRACE_INFO2, "SNCP_SWITCH_POLL_ON", (FMT__0));
#endif
    }
    else
    {
      NCP_CTX.poll_ed_timeout = ZB_FALSE;
      SNCP_SWITCH_POLL_OFF();
#if TRACE_ENABLED(TRACE_ZDO3) || TRACE_ENABLED(TRACE_NWK3)
      TRACE_MSG(TRACE_INFO2, "SNCP_SWITCH_POLL_OFF", (FMT__0));
#endif
    }
  }
#if TRACE_ENABLED(TRACE_ZDO3) || TRACE_ENABLED(TRACE_NWK3)
  TRACE_MSG(TRACE_INFO2, "<< sncp_auto_turbo_poll_ed_timeout poll_ed_timeout %hu", (FMT__H, NCP_CTX.poll_ed_timeout));
#endif
}

void sncp_auto_turbo_poll_aps_rx(zb_bool_t is_on)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> sncp_auto_turbo_poll_aps_rx param %hd poll_stopped_from_host %hd poll_aps_rx %hu",
            (FMT__H_H_H, is_on, NCP_CTX.poll_stopped_from_host, NCP_CTX.poll_aps_rx));
  /* if auto poll is disabled by Host, enable auto poll for APS fragmented rx */
  if (NCP_CTX.poll_stopped_from_host)
  {
    if (is_on)
    {
      NCP_CTX.poll_aps_rx = ZB_TRUE;
      SNCP_SWITCH_POLL_ON();
      TRACE_MSG(TRACE_TRANSPORT3, "SNCP_SWITCH_POLL_ON", (FMT__0));
    }
    else
    {
      NCP_CTX.poll_aps_rx = ZB_FALSE;
      SNCP_SWITCH_POLL_OFF();
      TRACE_MSG(TRACE_TRANSPORT3, "SNCP_SWITCH_POLL_OFF", (FMT__0));
    }
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<< sncp_auto_turbo_poll_aps_rx poll_aps_rx %hu", (FMT__H, NCP_CTX.poll_aps_rx));
}

void sncp_auto_turbo_poll_aps_tx(zb_bool_t is_on)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> sncp_auto_turbo_poll_aps_tx param %hd poll_stopped_from_host %hd poll_aps_tx %hu",
            (FMT__H_H_H, is_on, NCP_CTX.poll_stopped_from_host, NCP_CTX.poll_aps_tx));
  /* if auto poll is disabled by Host, enable auto poll for APS fragmented tx */
  if (NCP_CTX.poll_stopped_from_host)
  {
    if (is_on)
    {
      if (NCP_CTX.poll_aps_tx == (1U << 4U /* size in bits */) - 1U)
      {
        TRACE_MSG(TRACE_ERROR, "Oops! NCP_CTX.poll_aps_tx should be increased, and already %hu", (FMT__H, NCP_CTX.poll_aps_tx));
      }
      else
      {
        /* NCP_CTX.poll_aps_tx = ZB_TRUE; */
        NCP_CTX.poll_aps_tx++;
        SNCP_SWITCH_POLL_ON();
        TRACE_MSG(TRACE_TRANSPORT3, "SNCP_SWITCH_POLL_ON", (FMT__0));
      }
    }
    else
    {
      /* NCP_CTX.poll_aps_tx = ZB_FALSE; */
      if (NCP_CTX.poll_aps_tx == 0U)
      {
        TRACE_MSG(TRACE_ERROR, "Oops! NCP_CTX.poll_aps_tx should be reduced, and already 0", (FMT__0));
      }
      else
      {
        NCP_CTX.poll_aps_tx--;
        if (NCP_CTX.poll_aps_tx == 0U)
        {
          SNCP_SWITCH_POLL_OFF();
          TRACE_MSG(TRACE_TRANSPORT3, "SNCP_SWITCH_POLL_OFF", (FMT__0));
        }
      }
    }
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<< sncp_auto_turbo_poll_aps_tx poll_aps_tx %hu", (FMT__H, NCP_CTX.poll_aps_tx));
}

void sncp_auto_turbo_poll_stop(void)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> sncp_auto_turbo_poll_stop poll_stopped_from_host %hd poll_aps_rx %hd poll_aps_tx %hd poll_cbke %hd poll_ed_timeout %hd",
            (FMT__H_H_H_H_H, NCP_CTX.poll_stopped_from_host, NCP_CTX.poll_aps_rx, NCP_CTX.poll_aps_tx, NCP_CTX.poll_cbke, NCP_CTX.poll_ed_timeout));
  /* disable poll if turbo poll was finished and poll is controlled by Host and it was enabled by NCP */
  /* to prevent enabled poll after turbo poll is over */
  if ((NCP_CTX.poll_stopped_from_host) &&
      (NCP_CTX.poll_aps_rx || NCP_CTX.poll_aps_tx > 0U || NCP_CTX.poll_cbke || NCP_CTX.poll_ed_timeout)
     )
  {
    NCP_CTX.poll_aps_rx = ZB_FALSE;
    NCP_CTX.poll_aps_tx = 0;
    NCP_CTX.poll_cbke = ZB_FALSE;
    NCP_CTX.poll_ed_timeout = ZB_FALSE;
    SNCP_SWITCH_POLL_OFF();
    TRACE_MSG(TRACE_TRANSPORT3, "SNCP_SWITCH_POLL_OFF", (FMT__0));
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<< sncp_auto_turbo_poll_stop", (FMT__0));
}

#else

#define SET_POLL_STOPPED_FROM_HOST(set_reset)
#define SNCP_SWITCH_POLL_ON()
#define SNCP_SWITCH_POLL_OFF()

#endif

static zb_ret_t start_single_command(ncp_hl_request_header_t *hdr)
{
  zb_ret_t ret = RET_OK;
  if (NCP_CTX.single_cmds[hdr->tsn] != 0U)
  {
    ret = RET_BUSY;
  }
  else
  {
    NCP_CTX.single_cmds[hdr->tsn] = hdr->hdr.call_id;
    /* remember that tsn, so in case of consequent execution (most probable
     * case) stop_single_command can start search from it. */
    NCP_CTX.single_tsn = hdr->tsn;
  }
  return ret;
}

static inline void stop_single_command_by_tsn(zb_uint8_t tsn)
{
  NCP_CTX.single_cmds[tsn] = 0;
}

static zb_uint8_t stop_single_command(zb_uint16_t call_id)
{
  zb_ret_t ret = RET_NOT_FOUND;
  zb_uindex_t i;

  i = NCP_CTX.single_tsn;
  do
  {
    if (NCP_CTX.single_cmds[i] == call_id)
    {
      stop_single_command_by_tsn((zb_uint8_t)i);
      ret = RET_OK;
      break;
    }
    i = (i + 1U) % 256U;
  }
  while (i != NCP_CTX.single_tsn);

  if (ret == RET_OK)
  {
    return ((zb_uint8_t)i);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Unable to stop NCP single command: %d", (FMT__D, call_id));
    return TSN_RESERVED;
  }
}

static void ncp_hl_packet_received(void *data, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;
  ncp_hl_request_header_t *hdr = (ncp_hl_request_header_t*)data;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_packet_received data %p len %hd", (FMT__P_H, data, len));
/* Check for correct header. There must be request from a Host. */
  if (len >= sizeof(ncp_hl_request_header_t)
      && NCP_HL_GET_PKT_TYPE(hdr->hdr.control) == NCP_HL_REQUEST)
  {
    ZB_LETOH16_ONPLACE(hdr->hdr.call_id);
    if (NCP_CTX.mode == ZB_NCP_MODE_DEFAULT)
    {
      switch (NCP_HL_CALL_CATEGORY(hdr->hdr.call_id))
      {
        case NCP_HL_CATEGORY_CONFIGURATION:
          TRACE_MSG(TRACE_TRANSPORT3, "call_id 0x%x - NCP_HL_CATEGORY_CONFIGURATION", (FMT__D, hdr->hdr.call_id));
          txlen = handle_ncp_req_configuration(hdr, len);
          break;
        case NCP_HL_CATEGORY_AF:
          TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_CATEGORY_AF", (FMT__0));
          txlen = handle_ncp_req_af(hdr, len);
          break;
        case NCP_HL_CATEGORY_ZDO:
          txlen = handle_ncp_req_zdo(hdr, len);
          break;
        case NCP_HL_CATEGORY_APS:
          txlen = handle_ncp_req_aps(hdr, len);
          break;
        case NCP_HL_CATEGORY_NWKMGMT:
          txlen = handle_ncp_req_nwkmgmt(hdr, len);
          break;
        case NCP_HL_CATEGORY_SECUR:
          TRACE_MSG(TRACE_TRANSPORT3, "call_id 0x%x - NCP_HL_CATEGORY_SECUR", (FMT__D, hdr->hdr.call_id));
          txlen = handle_ncp_req_secur(hdr, len);
          break;

#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL
        case NCP_HL_CATEGORY_INTRP:
          txlen = handle_ncp_req_intrp(hdr, len);
          break;
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */

#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
        case NCP_HL_CATEGORY_MANUF_TEST:
          txlen = handle_ncp_req_manuf_test(hdr, len);
          break;
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */

#ifdef ZB_NCP_ENABLE_OTA_CMD
        case NCP_HL_CATEGORY_OTA:
          txlen = handle_ncp_req_ota(hdr, len);
          break;
#endif /* ZB_NCP_ENABLE_OTA_CMD */

#if defined ZB_NCP_ENABLE_CUSTOM_COMMANDS && defined SNCP_MODE
        case NCP_HL_CATEGORY_CUSTOM_COMMANDS:
          txlen = handle_ncp_req_custom_commands(hdr, len);
          break;
#endif /* ZB_NCP_ENABLE_CUSTOM_COMMANDS && SNCP_MODE */

        default:
          break;
      }
    }
#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
    else
    {
      ZB_ASSERT(NCP_CTX.mode == ZB_NCP_MODE_MANUFACTURE);
      switch (NCP_HL_CALL_CATEGORY(hdr->hdr.call_id))
      {
        case NCP_HL_CATEGORY_MANUF_TEST:
          txlen = handle_ncp_req_manuf_test(hdr, len);
          break;
        default:
          break;
      }
    }
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */
  }
  if (txlen == 0U)
  {
    TRACE_MSG(TRACE_ERROR, "Unsupported request %d", (FMT__D, hdr->hdr.call_id));
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, RET_ILLEGAL_REQUEST, 0);
  }
  if (txlen != NCP_RET_LATER)
  {
    /* if 0xff, resp will be sent later */
    ncp_send_packet(NCP_CTX.tx_buf, txlen);
  }
  if (NCP_CTX.work_buf == 0U)
  {
    /* Do not use delayed alloc to prevent filling entire queue with alloc requests */
    NCP_CTX.work_buf = zb_buf_get_out();
  }
}


static void buf_free_if_valid(zb_bufid_t buf)
{
  if (buf != 0U)
  {
    zb_buf_free(buf);
  }
}


static void handle_formation_signal(ncp_signal_t signal)
{
  zb_ret_t status = RET_ERROR;

  switch (signal)
  {
  case NCP_SIGNAL_NWK_FORMATION_OK:
    TRACE_MSG(TRACE_TRANSPORT3, "Formation completed ok", (FMT__0));
    status = RET_OK;
    break;
  case NCP_SIGNAL_NWK_FORMATION_FAILED:
    TRACE_MSG(TRACE_ERROR, "Formation failed", (FMT__0));
    status = RET_OPERATION_FAILED;
    break;
  default:
    TRACE_MSG(TRACE_ERROR, "unknown formation signal", (FMT__0));
    ZB_ASSERT(0);
    break;
  }

  if (NCP_CTX.start_command == NCP_HL_NWK_START_WITHOUT_FORMATION)
  {
    NCP_CTX.start_command = NCP_HL_NO_COMMAND;
    ncp_hl_send_no_arg_resp(NCP_HL_NWK_START_WITHOUT_FORMATION, status);
  }
  else if (NCP_CTX.start_command == NCP_HL_NWK_FORMATION)
  {
    NCP_CTX.start_command = NCP_HL_NO_COMMAND;
    ncp_hl_send_formation_response(status);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "formation signal received, but formation should not have started",
              (FMT__0));
    ZB_ASSERT(0);
  }
}


static void handle_join_rejoin_signal(ncp_signal_t signal, zb_bufid_t param)
{
  zb_ret_t status = RET_ERROR;

  switch(signal)
  {
    case NCP_SIGNAL_NWK_JOIN_FAILED:
    {
      status = zb_buf_get_status(param);
      TRACE_MSG(TRACE_TRANSPORT3, "Join failed, status %d", (FMT__D, status));
      break;
    }
    case NCP_SIGNAL_NWK_JOIN_AUTH_FAILED:
    {
      TRACE_MSG(TRACE_TRANSPORT3, "Authentication after join failed", (FMT__0));
      status = ERROR_CODE(ERROR_CATEGORY_ZDO, ZB_ZDP_STATUS_NOT_AUTHORIZED);
      break;
    }
    case NCP_SIGNAL_NWK_JOIN_DONE:
    {
      TRACE_MSG(TRACE_TRANSPORT3, "Join done ok", (FMT__0));
      status = RET_OK;
      break;
    }
    case NCP_SIGNAL_NWK_REJOIN_FAILED:
    {
      status = zb_buf_get_status(param);
      TRACE_MSG(TRACE_TRANSPORT3, "Rejoin failed, status %d", (FMT__D, status));

      /* since we don't use zb_zdo_startup_complete_int, so clear ZG->zdo.handle.rejoin here */
      ZG->zdo.handle.rejoin = ZB_FALSE;
      break;
    }
    case NCP_SIGNAL_NWK_REJOIN_DONE:
    {
      TRACE_MSG(TRACE_TRANSPORT3, "Rejoin done ok", (FMT__0));
      status = RET_OK;

      /* since we don't use zb_zdo_startup_complete_int, so clear ZG->zdo.handle.rejoin here */
      ZG->zdo.handle.rejoin = ZB_FALSE;
      break;
    }
    default:
    {
      TRACE_MSG(TRACE_ERROR, "unknown join signal 0x%x", (FMT__D, signal));
      ZB_ASSERT(0);
      break;
    }
  }

  switch ((zb_uint_t)NCP_CTX.start_command)
  {
    case NCP_HL_NWK_NLME_JOIN:
    {
      SNCP_SWITCH_POLL_OFF();
      if (status == RET_OK)
      {
        if (NCP_CTX.just_boot)
        {
          NCP_CTX.just_boot = ZB_FALSE;
        }
        else
        {
          ncp_hl_send_join_ok_resp(param);
        }
      }
      else
      {
        ncp_hl_send_no_arg_resp(NCP_HL_NWK_NLME_JOIN, status);
      }
      break;
    }
    case NCP_HL_NWK_NLME_ROUTER_START:
    {
      ncp_hl_send_no_arg_resp(NCP_HL_NWK_NLME_ROUTER_START, status);
      break;
    }
    case NCP_HL_ZDO_REJOIN:
    {
      SNCP_SWITCH_POLL_OFF();
      ncp_hl_send_rejoin_resp(status);
      break;
    }
    case 0:
    {
      SNCP_SWITCH_POLL_OFF();
      if (signal == NCP_SIGNAL_NWK_REJOIN_DONE || signal == NCP_SIGNAL_NWK_JOIN_DONE)
      {
        ncp_hl_nwk_join_ind(param);
      }
      else if (signal == NCP_SIGNAL_NWK_REJOIN_FAILED)
      {
        ncp_hl_nwk_join_failed_ind(status);
      }
      else
      {
        TRACE_MSG(TRACE_ERROR, "unexpected signal 0x%x", (FMT__D, signal));
        ZB_ASSERT(0);
      }
      break;
    }
    default:
    {
      TRACE_MSG(TRACE_ERROR, "unexpected start command 0x%x", (FMT__D, NCP_CTX.start_command));
      ZB_ASSERT(0);
      break;
    }
  }
  NCP_CTX.start_command = NCP_HL_NO_COMMAND;
  NCP_CTX.parent_lost = 0;
#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
  NCP_CTX.tc_swapped = ZB_FALSE;
#endif
}


void ncp_signal(ncp_signal_t signal, zb_uint8_t param)
{
  if (ncp_exec_blocked())
  {
    ncp_enqueue_buf(NCP_SIGNAL, (zb_uint8_t)signal, param);
  }
  else
  {
    ncp_signal_exec(signal, param);
  }
}

void ncp_signal_exec(ncp_signal_t signal, zb_uint8_t param)
{
  switch (signal)
  {
    case NCP_SIGNAL_NWK_FORMATION_OK:
    case NCP_SIGNAL_NWK_FORMATION_FAILED:
      handle_formation_signal(signal);
      break;
      /* TODO: it looks redundant. Recheck and remove. */
    case NCP_SIGNAL_NWK_PERMIT_JOINING_COMPLETED:
      TRACE_MSG(TRACE_ERROR, ">> NCP_SIGNAL_NWK_PERMIT_JOINING_COMPLETED", (FMT__0));
      /* TODO: kill hdr.status everywhere and pass status via parameter */
      ncp_hl_send_no_arg_resp(NCP_HL_NWK_PERMIT_JOINING, zb_buf_get_status(param));
      break;
    case NCP_SIGNAL_NWK_JOIN_FAILED:
    case NCP_SIGNAL_NWK_JOIN_AUTH_FAILED:
    case NCP_SIGNAL_NWK_JOIN_DONE:
    case NCP_SIGNAL_NWK_REJOIN_FAILED:
    case NCP_SIGNAL_NWK_REJOIN_DONE:
      handle_join_rejoin_signal(signal, param);
      break;

    case NCP_SIGNAL_SECUR_TCLK_EXCHANGE_COMPLETED:
      ncp_hl_secur_tclk_ind();
      break;

    case NCP_SIGNAL_SECUR_TCLK_EXCHANGE_FAILED:
      ncp_hl_secur_tclk_exchange_failed_ind(RET_ERROR);
      break;

    case NCP_SIGNAL_NLME_PARENT_LOST:
      handle_parent_lost(param);
      break;

    default:
      break;
  }
  buf_free_if_valid(param);
}


#ifdef ZB_ENABLE_SE_MIN_CONFIG
void ncp_se_signal(zse_commissioning_signal_t signal, zb_uint8_t param)
{
  if (ncp_exec_blocked())
  {
    ncp_enqueue_buf(NCP_SE_SIGNAL, (zb_uint8_t)signal, param);
  }
  else
  {
    ncp_se_signal_exec(signal, param);
  }
}

#ifdef ZB_COORDINATOR_ROLE

static void handle_se_comm_joined_device_signals(zse_commissioning_signal_t signal,
                                                 zb_uint8_t param)
{
  zb_address_ieee_ref_t addr_ref = (zb_address_ieee_ref_t)param;
  zb_ret_t status = RET_OK;

  ZB_ASSERT(signal == SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED
            || signal == SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK);

  param = bdb_cancel_link_key_refresh_alarm(ncp_comm_se_link_key_refresh_alarm, addr_ref);

  if (signal == SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED)
  {
    status = ERROR_CODE(ERROR_CATEGORY_CBKE, (zb_ret_t)(se_get_ke_status_code()));
    if (param != 0U)
    {
      ZB_SCHEDULE_CALLBACK(ncp_comm_se_link_key_refresh_alarm, param);
      param = 0;
    }
  }

  secur_tc_cbke_ind(addr_ref, status);
  buf_free_if_valid(param);
}

#endif /* ZB_COORDINATOR_ROLE */


void ncp_se_signal_exec(zse_commissioning_signal_t signal, zb_uint8_t param)
{
  switch (signal)
  {
    case SE_COMM_SIGNAL_CBKE_OK:
      TRACE_MSG(TRACE_TRANSPORT3, "SE_COMM_SIGNAL_CBKE_OK", (FMT__0));
      secur_cbke_done(RET_OK);
      break;
    case SE_COMM_SIGNAL_CBKE_FAILED:
      TRACE_MSG(TRACE_TRANSPORT3, "SE_COMM_SIGNAL_CBKE_FAILED status %d", (FMT__D, se_get_ke_status_code()));
      secur_cbke_done(ERROR_CODE(ERROR_CATEGORY_CBKE, (zb_ret_t)se_get_ke_status_code()));
      break;
#ifdef ZB_COORDINATOR_ROLE
    case SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_FAILED:
    case SE_COMM_SIGNAL_JOINED_DEVICE_CBKE_OK:
      handle_se_comm_joined_device_signals(signal, param);
      param = 0;
      break;
#endif /* ZB_COORDINATOR_ROLE */
    default:
      break;
  }
  buf_free_if_valid(param);
}

#endif /* ZB_ENABLE_SE_MIN_CONFIG */

void ncp_hl_set_prod_config(const ncp_app_production_config_t *prod_cfg)
{
  ZB_MEMCPY(NCP_CTX.serial_number, prod_cfg->serial_number, sizeof(NCP_CTX.serial_number));
  ZB_MEMCPY(NCP_CTX.vendor_data, prod_cfg->vendor_data, sizeof(NCP_CTX.vendor_data));
  NCP_CTX.vendor_data_size = prod_cfg->vendor_data_size;
}


static zb_uint16_t handle_ncp_req_configuration(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen = 0;

  switch (hdr->hdr.call_id)
  {
    case NCP_HL_GET_MODULE_VERSION:
    {
      zb_uint8_t *resp_body;
      zb_uint32_t ver_fw = NCP_FW_VERSION;
      zb_uint32_t ver_stack = NCP_STACK_VERSION;
      zb_uint32_t ver_protocol = NCP_PROTOCOL_VERSION;
      zb_uint_t resp_body_size = sizeof(ver_fw);

      resp_body_size += sizeof(ver_protocol);
      resp_body_size += sizeof(ver_stack);

      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_MODULE_VERSION", (FMT__0));
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_MODULE_VERSION, hdr->tsn, RET_OK, resp_body_size);

      resp_body = (zb_uint8_t*)&rh[1];
      /* Place versions in the response body sequentially in byte numbers:
       * [0..3]  NCP_FW_VERSION,
       * [4..7]  NCP_STACK_VERSION,
       * [8..11] NCP_PROTOCOL_VERSION,
       */
      ZB_HTOLE32(resp_body, &ver_fw);
      resp_body += sizeof(ver_fw);

      ZB_HTOLE32(resp_body, &ver_stack);
      resp_body += sizeof(ver_stack);

      ZB_HTOLE32(resp_body, &ver_protocol);
      break;
    }
    case NCP_HL_NCP_RESET:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NCP_RESET", (FMT__0));
      txlen = reset_req(hdr, len);
      break;
    case NCP_HL_NCP_FACTORY_RESET:
      break;
    case NCP_HL_GET_ZIGBEE_ROLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_ZIGBEE_ROLE", (FMT__0));
      txlen = get_zigbee_role(hdr, len);
      break;
    case NCP_HL_SET_ZIGBEE_ROLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_ZIGBEE_ROLE", (FMT__0));
      txlen = set_zigbee_role(hdr, len);
      break;
    case NCP_HL_SET_MAX_CHILDREN:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_MAX_CHILDREN", (FMT__0));
      txlen = set_max_children(hdr, len);
      break;
    case NCP_HL_GET_MAX_CHILDREN:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_MAX_CHILDREN", (FMT__0));
      txlen = get_max_children(hdr, len);
      break;
    case NCP_HL_GET_ZIGBEE_CHANNEL_MASK:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_ZIGBEE_CHANNEL_MASK", (FMT__0));
      txlen = get_channel_mask(hdr, len);
      break;
    case NCP_HL_SET_ZIGBEE_CHANNEL_MASK:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_ZIGBEE_CHANNEL_MASK", (FMT__0));
      txlen = set_channel_mask(hdr, len);
      break;
    case NCP_HL_GET_ZIGBEE_CHANNEL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_ZIGBEE_CHANNEL", (FMT__0));
      txlen = get_current_channel(hdr, len);
      break;
    case NCP_HL_GET_PAN_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_PAN_ID", (FMT__0));
      txlen = get_pan_id(hdr, len);
      break;
    case NCP_HL_SET_PAN_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_PAN_ID", (FMT__0));
      txlen = set_pan_id(hdr, len);
      break;
    case NCP_HL_SET_EXTENDED_PAN_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_EXTENDED_PAN_ID", (FMT__0));
      txlen = set_extended_pan_id(hdr, len);
      break;
    case NCP_HL_GET_LOCAL_IEEE_ADDR:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_LOCAL_IEEE_ADDR", (FMT__0));
      txlen = get_local_ieee_addr(hdr, len);
      break;
    case NCP_HL_SET_LOCAL_IEEE_ADDR:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_LOCAL_IEEE_ADDR", (FMT__0));
      txlen = set_local_ieee(hdr, len);
      break;
    case NCP_HL_GET_TRACE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_TRACE", (FMT__0));
      txlen = get_trace(hdr, len);
      break;
    case NCP_HL_SET_TRACE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_TRACE", (FMT__0));
      txlen = set_trace(hdr, len);
      break;
    case NCP_HL_GET_TX_POWER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_TX_POWER", (FMT__0));
      txlen = get_tx_power(hdr, len);
      break;
    case NCP_HL_SET_TX_POWER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_TX_POWER", (FMT__0));
      txlen = set_tx_power(hdr, len);
      break;
    case NCP_HL_GET_RX_ON_WHEN_IDLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_RX_ON_WHEN_IDLE", (FMT__0));
      txlen = get_rx_on_when_idle(hdr, len);
      break;
    case NCP_HL_SET_RX_ON_WHEN_IDLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_RX_ON_WHEN_IDLE", (FMT__0));
      txlen = set_rx_on_when_idle(hdr, len);
      break;
    case NCP_HL_GET_JOINED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_JOINED", (FMT__0));
      txlen = get_joined(hdr, len);
      break;
    case NCP_HL_GET_AUTHENTICATED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_AUTHENTICATED", (FMT__0));
      txlen = get_authenticated(hdr, len);
      break;
    case NCP_HL_GET_ED_TIMEOUT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_ED_TIMEOUT", (FMT__0));
      txlen = get_ed_timeout(hdr, len);
      break;
    case NCP_HL_SET_ED_TIMEOUT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_ED_TIMEOUT", (FMT__0));
      txlen = set_ed_timeout(hdr, len);
      break;
    case NCP_HL_ADD_VISIBLE_DEV:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ADD_VISIBLE_DEV", (FMT__0));
      txlen = add_visible_dev(hdr, len);
      break;
    case NCP_HL_ADD_INVISIBLE_SHORT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ADD_INVISIBLE_SHORT", (FMT__0));
      txlen = add_invisible_short(hdr, len);
      break;
    case NCP_HL_RM_INVISIBLE_SHORT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_RM_INVISIBLE_SHORT", (FMT__0));
      txlen = rm_invisible_short(hdr, len);
      break;
    case NCP_HL_SET_NWK_KEY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_NWK_KEY", (FMT__0));
      txlen = set_nwk_key(hdr, len);
      break;
#ifndef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
    case NCP_HL_GET_SERIAL_NUMBER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_SERIAL_NUMBER", (FMT__0));
      txlen = cfg_get_serial_number(hdr, len);
      break;
    case NCP_HL_GET_VENDOR_DATA:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_VENDOR_DATA", (FMT__0));
      txlen = cfg_get_vendor_data(hdr, len);
      break;
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
    case NCP_HL_GET_NWK_KEYS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_NWK_KEYS", (FMT__0));
      txlen = get_nwk_keys(hdr, len);
      break;
    case NCP_HL_GET_APS_KEY_BY_IEEE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_APS_KEY_BY_IEEE", (FMT__0));
      txlen = get_aps_key_by_ieee(hdr, len);
      break;
    case NCP_HL_BIG_PKT_TO_NCP:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_BIG_PKT_TO_NCP", (FMT__0));
      txlen = big_pkt_to_ncp(hdr, len);
      break;
    case NCP_HL_GET_PARENT_ADDRESS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_PARENT_ADDRESS", (FMT__0));
      txlen = get_parent_address(hdr, len);
      break;
    case NCP_HL_GET_EXTENDED_PAN_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_EXTENDED_PAN_ID", (FMT__0));
      txlen = get_extended_pan_id(hdr, len);
      break;
    case NCP_HL_GET_COORDINATOR_VERSION:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_COORDINATOR_VERSION", (FMT__0));
      txlen = get_coordinator_version(hdr, len);
      break;
    case NCP_HL_GET_SHORT_ADDRESS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_SHORT_ADDRESS", (FMT__0));
      txlen = get_short_address(hdr, len);
      break;
    case NCP_HL_GET_TRUST_CENTER_ADDRESS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_TRUST_CENTER_ADDRESS", (FMT__0));
      txlen = get_trust_center_address(hdr, len);
      break;
    case NCP_HL_DEBUG_WRITE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_DEBUG_WRITE", (FMT__0));
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_DEBUG_WRITE, hdr->tsn, RET_OK, 0);
      break;
    case NCP_HL_GET_CONFIG_PARAMETER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_CONFIG_PARAMETER", (FMT__0));
      txlen = get_config_parameter(hdr, len);
      break;
#ifdef ZB_HAVE_BLOCK_KEYS
    case NCP_HL_GET_LOCK_STATUS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_LOCK_STATUS", (FMT__0));
      txlen = get_lock_status(hdr, len);
      break;
#endif
    case NCP_HL_SET_NWK_LEAVE_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_NWK_LEAVE_ALLOWED", (FMT__0));
      txlen = set_nwk_leave_allowed(hdr, len);
      break;
    case NCP_HL_GET_NWK_LEAVE_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_NWK_LEAVE_ALLOWED", (FMT__0));
      txlen = get_nwk_leave_allowed(hdr, len);
      break;
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
    case NCP_HL_PRODUCTION_CONFIG_READ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PRODUCTION_CONFIG_READ", (FMT__0));
      txlen = production_config_read(hdr, len);
      break;
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST  */
    case NCP_HL_NVRAM_WRITE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NVRAM_WRITE", (FMT__0));
      txlen = nvram_write(hdr, len);
      break;
    case NCP_HL_NVRAM_READ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NVRAM_READ", (FMT__0));
      txlen = nvram_read(hdr, len);
      break;
    case NCP_HL_NVRAM_CLEAR:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NVRAM_CLEAR", (FMT__0));
      txlen = nvram_clear(hdr, len);
      break;
    case NCP_HL_NVRAM_ERASE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NVRAM_ERASE", (FMT__0));
      txlen = nvram_erase(hdr, len);
      break;
    case NCP_HL_SET_TC_POLICY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_TC_POLICY", (FMT__0));
      txlen = set_tc_policy(hdr, len);
      break;
    case NCP_HL_SET_ZDO_LEAVE_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_ZDO_LEAVE_ALLOWED", (FMT__0));
      txlen = set_zdo_leave_allowed(hdr, len);
      break;
    case NCP_HL_GET_ZDO_LEAVE_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_ZDO_LEAVE_ALLOWED", (FMT__0));
      txlen = get_zdo_leave_allowed(hdr, len);
      break;
#ifdef ZB_ENABLE_ZGP
    case NCP_HL_DISABLE_GPPB:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_DISABLE_GPPB", (FMT__0));
      txlen = disable_gppb(hdr, len);
      break;
    case NCP_HL_GP_SET_SHARED_KEY_TYPE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GP_SET_SHARED_KEY_TYPE", (FMT__0));
      txlen = gp_set_shared_key_type(hdr, len);
      break;
    case NCP_HL_GP_SET_DEFAULT_LINK_KEY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GP_SET_DEFAULT_LINK_KEY", (FMT__0));
      txlen = gp_set_default_link_key(hdr, len);
      break;
#endif /* ZB_ENABLE_ZGP */
    case NCP_HL_SET_LEAVE_WO_REJOIN_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_LEAVE_WO_REJOIN_ALLOWED", (FMT__0));
      txlen = set_leave_without_rejoin_allowed(hdr, len);
      break;
    case NCP_HL_GET_LEAVE_WO_REJOIN_ALLOWED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_LEAVE_WO_REJOIN_ALLOWED", (FMT__0));
      txlen = get_leave_without_rejoin_allowed(hdr, len);
      break;
    default:
      break;
  }
  return txlen;
}


/* since Host needs to get ACK but NCP hasn't sent it due to reset, postpone reset via alarm to ~1 sec */
static void schedule_reset(ncp_hl_reset_opt_t opt)
{
  (void)ZB_SCHEDULE_APP_ALARM_CANCEL(make_reset, ZB_ALARM_ANY_PARAM);

  (void)ZB_SCHEDULE_APP_ALARM(make_reset, (zb_uint8_t)opt, ZB_TIME_ONE_SECOND);
}

static void make_reset(zb_uint8_t param)
{
  ncp_hl_reset_opt_t options = (ncp_hl_reset_opt_t)param;

  /* DD: I found no simple way to reinit the stack state after nvram erase,
     so for NCP we will use zb_reset() */
  /* ES: actual nvram actions were moved here from NCP_HL_NCP_RESET handler,
     else between NVRAM erase/clear and ZBOSS reset some time were during which stack could fill NVRAM again.
     NCP_HL_NCP_RESET request is asynchronous command and can be received before some scheduled NVRAM operation.
     Now NCP makes reset right after NVRAM erase/clear */
  switch(options)
  {
    case NVRAM_ERASE:
      zb_nvram_erase();
      break;
    case FACTORY_RESET:
      zb_nvram_clear();
      break;
    default:
      break;
  }

#if defined ZB_NSNG && defined SNCP_MODE
  ncp_hl_send_reset_resp(0);
#else
  zb_reset(0);
#endif
}

/*
ID = NCP_HL_NCP_RESET:
  Request:
    header:
      2b: ID
    body:
      1b: reset options (0 - reset only, 1 - NVRAM erase, 2 - factory reset)
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t reset_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t options;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr) + sizeof(options))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    options = *(zb_uint8_t*)(&hdr[1]);
    switch(options)
    {
      case NO_OPTIONS:
      case NVRAM_ERASE:
      case FACTORY_RESET:
        schedule_reset(options);
        break;
#ifdef ZB_HAVE_BLOCK_KEYS
      case BLOCK_READING_KEYS:
        ret = zb_chip_lock();
        if (ret == RET_OK)
        {
          schedule_reset(NO_OPTIONS);
        }
        break;
#endif
      default:
        ret = RET_INVALID_FORMAT;
        break;
    }
  }
  if (RET_OK == ret)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NCP_RESET, hdr->tsn, ret, 0);
  }

  return txlen;
}


static zb_uint16_t set_channel_mask(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t page;
  zb_uint8_t ch;
  zb_uint32_t mask;

  /*
    Arguments:
    1b page
    4b mask
   */
  if (len < sizeof(*hdr) + sizeof(page) + sizeof(mask))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    page = *p;
    ZB_MEMCPY(&mask, &p[1], sizeof(mask));
    ZB_LETOH32_ONPLACE(mask);
    TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_ZIGBEE_CHANNEL_MASK page %hd mask 0x%x",
              (FMT__H_D, page, mask));
    if (zb_channel_page_get_max_channel_number(page, &ch) != RET_OK
        /*cstat -MISRAC2012-Rule-13.5 */
        /* After some investigation, the following violations of Rule 13.5 seem to be
         * false positives. There are no side effect to 'zb_high_bit_number()' or
         * 'zb_channel_page_get_start_channel_number()'. This violations seems to be caused by the
         * fact that both 'zb_high_bit_number()' or 'zb_channel_page_get_start_channel_number()' are
         * external functions, which cannot be analyzed by C-STAT. */
        || zb_high_bit_number(mask) > ch
        || zb_channel_page_get_start_channel_number(page, &ch) != RET_OK
        || zb_low_bit_number(mask) < ch)
        /*cstat +MISRAC2012-Rule-13.5 */
    {
      ret = RET_INVALID_PARAMETER;
    }
#ifdef ZB_SUBGHZ_BAND_ENABLED
#ifdef SNCP_MODE
    else if (ZB_IS_DEVICE_ZR() && ZB_LOGICAL_PAGE_IS_SUB_GHZ(page))
    {
      TRACE_MSG(TRACE_ERROR, "Sub-GHz is not supported for ZR role!", (FMT__0));
      ret = RET_NOT_IMPLEMENTED;
    }
#endif /* SNCP_MODE */
#else
    else if (page != 0U)
    {
      TRACE_MSG(TRACE_ERROR, "Sub-GHz is not implemented!", (FMT__0));
      ret = RET_NOT_IMPLEMENTED;
    }
#endif /* ZB_SUBGHZ_BAND_ENABLED */
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
  }
  if (ret == RET_OK)
  {
#ifndef ZB_SUBGHZ_BAND_ENABLED
    zb_set_channel_mask(mask);
#else
    zb_aib_channel_page_list_set_mask(ZB_CHANNEL_PAGE_TO_IDX(page), mask);
#endif
  }
  /* create a resp */
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_ZIGBEE_CHANNEL_MASK, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_GET_ZIGBEE_CHANNEL_MASK:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: # of page/mask entries
      1b: page
      4b: channel mask
*/
static zb_uint16_t get_channel_mask(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint32_t channel_mask;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;
  zb_uint16_t txlen;
  zb_uint8_t *num_entries;
  zb_uint8_t idx;

  (void)len;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_channel_mask", (FMT__0));

  (void)ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_ZIGBEE_CHANNEL_MASK, hdr->tsn, ret, (sizeof(channel_mask) + 1U) * ZB_CHANNEL_PAGES_NUM/*#all ent*/ + 1U/*pages&masks #*/);
  num_entries = (zb_uint8_t*)(&rh[1]); /* # of entries */
  p = &num_entries[1];

  for (idx = 0; idx < ZB_CHANNEL_PAGES_NUM; idx++)
  {
    channel_mask = zb_aib_channel_page_list_get_mask(idx);

    TRACE_MSG(TRACE_TRANSPORT3, "idx %u channel %hu channel_mask 0x%x", (FMT__D_H_L, idx, ZB_CHANNEL_PAGE_FROM_IDX(idx), channel_mask));
    if (channel_mask != 0U)
    {
      (*num_entries) += 1U;
      *p = (zb_uint8_t)ZB_CHANNEL_PAGE_FROM_IDX(idx);
      p++;
      ZB_HTOLE32(p, &channel_mask);
      p += sizeof(channel_mask);
    }
  }

  txlen =  (zb_uint16_t)sizeof(ncp_hl_response_header_t);
  txlen += ((zb_uint16_t)sizeof(channel_mask) + 1U) * (*num_entries);
  txlen += 1U /*body_size*/;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_channel_mask num_entries %hu", (FMT__H, *num_entries));

  return txlen;
}

/*
ID = NCP_HL_GET_ZIGBEE_CHANNEL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: current channel
*/
static zb_uint16_t get_current_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t current_page;
  zb_uint8_t current_channel;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  (void)len;

  current_page = ZB_PIBCACHE_CURRENT_PAGE();
  current_channel = ZB_PIBCACHE_CURRENT_CHANNEL();

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_ZIGBEE_CHANNEL, hdr->tsn, ret,
    sizeof(current_page) + sizeof(current_channel));

  p = (zb_uint8_t*)(&rh[1]);
  *p = current_page;
  p++;
  *p = current_channel;

  return txlen;
}

/*
ID = NCP_HL_GET_PAN_ID:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      2b: pan id
*/
static zb_uint16_t get_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t pan_id;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  (void)len;

  pan_id = ZB_PIBCACHE_PAN_ID();
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_PAN_ID, hdr->tsn, ret, sizeof(pan_id));
  ZB_HTOLE16(&rh[1], &pan_id);

  return txlen;
}

/*
ID = NCP_HL_SET_PAN_ID:
  Request:
    header:
      2b: ID
    body:
      2b: pan id
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#if !defined SNCP_MODE
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t);
  zb_uint16_t pan_id;
  zb_uint8_t buf;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>set_pan_id", (FMT__0));

  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
  }
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    ncp_hl_body_get_u16(&body, &pan_id);

    TRACE_MSG(TRACE_TRANSPORT3, "  set_pan_id, pan_id %x", (FMT__D, pan_id));

    zb_set_pan_id(pan_id);

    zb_nwk_pib_set(buf, ZB_PIB_ATTRIBUTE_PANID,
        &pan_id, sizeof(pan_id), set_pan_id_confirm);

    txlen = NCP_RET_LATER;
  }
  else
  {
    stop_single_command_by_tsn(hdr->tsn);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<set_pan_id, ret %hu", (FMT__D, ret));

  return txlen;

#else
  ZVUNUSED(len);

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, RET_NOT_IMPLEMENTED, 0);
#endif
}

#if !defined SNCP_MODE
static void set_pan_id_confirm(zb_uint8_t param)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t tsn = stop_single_command(NCP_HL_SET_PAN_ID);
  zb_uint16_t txlen;
  zb_mlme_set_confirm_t *confirm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>set_pan_id_confirm, param %hu, mac status %hu", (FMT__H_H, param, confirm->status));

  if (confirm->status != MAC_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_MAC, confirm->status);
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SET_PAN_ID, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<set_pan_id_confirm", (FMT__0));
}
#endif

/*
ID = NCP_HL_SET_EXTENDED_PAN_ID:
  Request:
    header:
      2b: ID
    body:
      8b: extended pan id
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_extended_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t);
  zb_ext_pan_id_t extended_pan_id;
  zb_uint8_t buf;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> set_extended_pan_id", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected);

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u64addr(&body, extended_pan_id);

    TRACE_MSG(TRACE_TRANSPORT3, "ext_pan_id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(extended_pan_id)));

    zb_set_extended_pan_id(extended_pan_id);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< set_extended_pan_id, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_GET_LOCAL_IEEE_ADDR:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: ieee address
*/
static zb_uint16_t get_local_ieee_addr(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_ieee_addr_t ieee_addr;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  (void)len;

  ZB_IEEE_ADDR_COPY(ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
  ZB_DUMP_IEEE_ADDR(ieee_addr);
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_LOCAL_IEEE_ADDR, hdr->tsn, ret, sizeof(zb_uint8_t) + sizeof(ieee_addr));
  /* MAC if == always 0 */
  p = (zb_uint8_t*)(&rh[1]);
  *p = 0;
  p++;
  ZB_MEMCPY(p, ieee_addr, sizeof(ieee_addr));

  return txlen;
}

static zb_uint16_t set_zigbee_role(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t role;

  /*
    Arguments:
    1b role (zb_nwk_device_type_t)
  */
  if (len < sizeof(*hdr) + sizeof(role))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    role = *p;
    TRACE_MSG(TRACE_TRANSPORT3, "set_zigbee_role %hd", (FMT__H, role));
    if (role > ZB_NWK_DEVICE_TYPE_ED)
    {
      TRACE_MSG(TRACE_ERROR, "invalid role %hd", (FMT__H, role));
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    /* Boe check for compile options */

    switch (role)
    {
      case ZB_NWK_DEVICE_TYPE_COORDINATOR:
#ifdef ZB_COORDINATOR_ROLE
        ZB_AIB().aps_designated_coordinator = ZB_TRUE;
        ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
        zb_set_default_ffd_descriptor_values(ZB_NWK_DEVICE_TYPE_COORDINATOR);
#ifdef ZB_ENABLE_SE_MIN_CONFIG
        ncp_kec_tc_init();
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

#ifdef ZB_ENABLE_ZGP
        ZB_SCHEDULE_CALLBACK(zgp_init_by_scheduler, 0);
#endif

#else
        TRACE_MSG(TRACE_ERROR, "ZC role is not compiled in", (FMT__0));
        ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_COORDINATOR_ROLE */
        break;

      case ZB_NWK_DEVICE_TYPE_ROUTER:
      {
#ifdef ZB_ROUTER_ROLE
#if defined SNCP_MODE && defined ZB_SUBGHZ_BAND_ENABLED
        zb_uint8_t i;
        zb_uint8_t page;
        zb_uint32_t mask;
        zb_channel_list_t channel_list;

        zb_channel_list_init(channel_list);
        zb_aib_get_channel_page_list(channel_list);

        for (i = 0U; i < ZB_CHANNEL_PAGES_NUM; ++i)
        {
          page = ZB_CHANNEL_PAGE_FROM_IDX(i);
          mask = zb_aib_channel_page_list_get_mask(i);

          if (ZB_LOGICAL_PAGE_IS_SUB_GHZ(page) && (mask != 0U))
          {
            TRACE_MSG(TRACE_ERROR, "Sub-GHz is not supported for ZR role! page %hd, mask 0x%lx",
                      (FMT__H_L, page, mask));
            ret = RET_NOT_IMPLEMENTED;
            break;
          }
          else
          {
            /* MISRA rule 15.7 requires empty 'else' branch. */
          }
        }

        if (ret == RET_NOT_IMPLEMENTED)
        {
          break;
        }
        else
        {
          /* MISRA rule 15.7 requires empty 'else' branch. */
        }
#endif /* SNCP_MODE && ZB_SUBGHZ_BAND_ENABLED */

        ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ROUTER;
#ifdef ZB_ED_RX_OFF_WHEN_IDLE
        /* must be called after device_type assigning */
        apply_rx_on_when_idle(ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()));
#endif
        zb_set_default_ffd_descriptor_values(ZB_NWK_DEVICE_TYPE_ROUTER);

#ifdef ZB_ENABLE_ZGP
        ZB_SCHEDULE_CALLBACK(zgp_init_by_scheduler, 0);
#endif
#else
        TRACE_MSG(TRACE_ERROR, "ZR role is not compiled in", (FMT__0));
        ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_ROUTER_ROLE */
        break;
      }

      case ZB_NWK_DEVICE_TYPE_ED:
#ifdef ZB_ED_FUNC
        ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;
#ifdef ZB_ED_RX_OFF_WHEN_IDLE
        /* must be called after device_type assigning */
        apply_rx_on_when_idle(ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()));
#endif
        zb_set_default_ed_descriptor_values();
#else
        TRACE_MSG(TRACE_ERROR, "ZED role is not compiled in", (FMT__0));
        ret = RET_NOT_IMPLEMENTED;
#endif
        break;

      default:
        break;
    }
  }
  if (ret == RET_OK)
  {
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
  }
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_ZIGBEE_ROLE, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_GET_ZIGBEE_ROLE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: zigbee role
*/
static zb_uint16_t get_zigbee_role(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t tmp8;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  (void)len;

  tmp8 = ZB_NIB_DEVICE_TYPE();
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_ZIGBEE_ROLE, hdr->tsn, ret, sizeof(tmp8));
  ZB_MEMCPY(&rh[1], &tmp8, sizeof(tmp8));

  return txlen;
}


static zb_uint16_t set_max_children(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t max_children;

  /*
    Arguments:
    1b max children
  */
  if (len < sizeof(*hdr) + sizeof(max_children))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    max_children = *p;

    zb_set_max_children(max_children);
  }

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_GET_MAX_CHILDREN:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: max_children
*/
static zb_uint16_t get_max_children(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t max_children;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  ZVUNUSED(len);

  max_children = zb_get_max_children();

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(max_children));
  ZB_MEMCPY(&rh[1], &max_children, sizeof(max_children));

  return txlen;
}


static void sync_long_addr(zb_uint8_t param)
{
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS, &ZB_PIBCACHE_EXTENDED_ADDRESS(),
                 sizeof(zb_ieee_addr_t), NULL);
}

static zb_uint16_t set_local_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  /*
    Packet format:
    1b MAC interface # (must be 0)
    8b IEEE address
   */
  if (len < (zb_uint16_t)sizeof(*hdr) + (zb_uint16_t)sizeof(zb_ieee_addr_t) + 1U)
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    zb_uint8_t *p = (zb_uint8_t*)&hdr[1];
    if (*p != 0U)
    {
      ret = RET_INVALID_PARAMETER;
    }
    else
    {
      p++;
      zb_set_long_address(p);

      ret = zb_buf_get_out_delayed(sync_long_addr);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
      }
    }
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_LOCAL_IEEE_ADDR, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_GET_ED_TIMEOUT:
  Request:
    7b: Common request header
  Response:
    5b: Common response header
    1b: ED timeout value
*/
static zb_uint16_t get_ed_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t ed_timeout;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;

  (void)len;

  ed_timeout = zdo_get_aging_timeout();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(ed_timeout));
  ZB_MEMCPY(&rh[1], &ed_timeout, ZB_8BIT_SIZE);

  return txlen;
}

/*
ID = NCP_HL_SET_ED_TIMEOUT:
  Request:
    7b: Common request header
    1b: ED timeout value
  Response:
    5b: Common response header
*/
static zb_uint16_t set_ed_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t ed_timeout;

  if (len < sizeof(*hdr) + sizeof(ed_timeout))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    ZB_MEMCPY(&ed_timeout, &hdr[1], ZB_8BIT_SIZE);
#ifdef SNCP_MODE
    /* according to GBCS 3.2 10.7.3
        Where a Device is an End Device, the Device shall not send an End Device Timeout Request command
        with a Requested Timeout Enumeration Value greater than 10 (so meaning greater than 1,024 minutes)
    */
    if (ed_timeout > ED_AGING_TIMEOUT_1024MIN)
    {
      ret = RET_INVALID_PARAMETER;
    }
    else
#endif
    {
      zdo_set_aging_timeout(ed_timeout);
    }
  }

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GET_RX_ON_WHEN_IDLE:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: value 0-off, 1-on
*/
static zb_uint16_t get_rx_on_when_idle(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_rx_on_when_idle len %hu", (FMT__H, len));

  (void)len;

  val = zb_get_rx_on_when_idle();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  *p = val ? 1U : 0U;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_rx_on_when_idle txlen %u", (FMT__D, txlen));

  return txlen;
}

static void sync_rx_on(zb_bufid_t param)
{
  zb_uint8_t rx_on = (zb_uint8_t)ZB_PIBCACHE_RX_ON_WHEN_IDLE();
  zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE,
                 &rx_on, 1,
                 NULL);
}

static void apply_rx_on_when_idle(zb_bool_t rx_on)
{
  zb_ret_t ret;

  if (rx_on)
  {
    ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(ZB_ZDO_NODE_DESC()->mac_capability_flags, 1U);
    ZB_MAC_CAP_SET_POWER_SOURCE(ZB_ZDO_NODE_DESC()->mac_capability_flags, 1U);
    zb_set_rx_on_when_idle(ZB_TRUE);
  }
  else
  {
    zb_uint8_t mask = 0;
    ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(mask, 1U);
    ZB_MAC_CAP_SET_POWER_SOURCE(mask, 1U);
    ZB_ZDO_NODE_DESC()->mac_capability_flags &= ~mask;
    zb_set_rx_on_when_idle(ZB_FALSE);
  }

  ret = zb_buf_get_out_delayed(sync_rx_on);
  if (ret != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "Failed zb_buf_get_out_delayed [%d]", (FMT__D, ret));
  }
}

/*
ID = NCP_HL_SET_RX_ON_WHEN_IDLE:
  Request:
    header:
      2b: ID
    body:
      1b: value 0-off, 1-on
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_rx_on_when_idle(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
  zb_uint8_t value;
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "set_rx_on_when_idle len %d", (FMT__D, len));

#ifndef ZB_ED_RX_OFF_WHEN_IDLE
  if (len < sizeof(*hdr) + sizeof(value))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    value = *(zb_uint8_t*)&hdr[1];
    apply_rx_on_when_idle(ZB_U2B(value));
  }
  if (ret == RET_OK)
  {
    /* If we fail, trace is given and assertion is triggered */
    (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
  }
#endif

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_RX_ON_WHEN_IDLE, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GET_JOINED:
  Request:
  5b: Common request header
  Response:
  7b: Common response header
  1b: bit 0: joined - false, 1 - true
      bit 1: parent lost, 0 - false, 1 - true
*/
static zb_uint16_t get_joined(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_joined len %hu", (FMT__H, len));

  (void)len;

  val = ZB_JOINED();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  *p = 0;
  if (val != 0)
  {
    *p |= NCP_HL_GET_JOINED_FLAG_JOINED;
  }
  if (NCP_CTX.parent_lost != 0U)
  {
    *p |= NCP_HL_GET_JOINED_FLAG_PARENT_LOST;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_joined txlen %u joined %hd parent lost %hd", (FMT__D_H_H, txlen, val, NCP_CTX.parent_lost));

  return txlen;
}

/*
  ID = NCP_HL_GET_AUTHENTICATED:
  Request:
  5b: Common request header
  Response:
  7b: Common response header
  1b: value 0 - false, 1 - true
*/
static zb_uint16_t get_authenticated(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_authenticated len %hu", (FMT__H, len));

  (void)len;

  val = ZG->aps.authenticated;
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  *p = val ? 1U : 0U;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_authenticated txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
ID = NCP_HL_ADD_VISIBLE_DEV:
  Request:
    7b: Common request header
    8b: IEEE address
  Response:
    5b: Common response header
*/
static zb_uint16_t add_visible_dev(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_LIMIT_VISIBILITY
  zb_ieee_addr_t ieee_addr;
#endif /* ZB_LIMIT_VISIBILITY */

  TRACE_MSG(TRACE_TRANSPORT3, ">>add_visible_dev len %hu", (FMT__H, len));

#ifdef ZB_LIMIT_VISIBILITY
  if (len < sizeof(*hdr) + sizeof(ieee_addr))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    ZB_IEEE_ADDR_COPY(ieee_addr, (zb_uint8_t*)(&hdr[1]));
    ZB_DUMP_IEEE_ADDR(ieee_addr);
    MAC_ADD_VISIBLE_LONG(ieee_addr);
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_LIMIT_VISIBILITY */

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_ADD_INVISIBLE_SHORT:
  Request:
    7b: Common request header
    2b: Short address
  Response:
    5b: Common response header
*/
static zb_uint16_t add_invisible_short(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_LIMIT_VISIBILITY
  zb_uint16_t short_addr = 0;
#endif /* ZB_LIMIT_VISIBILITY */

  TRACE_MSG(TRACE_TRANSPORT3, ">>add_invisible_short len %hu", (FMT__H, len));

#ifdef ZB_LIMIT_VISIBILITY
  if (len < sizeof(*hdr) + sizeof(short_addr))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    ZB_LETOH16(&short_addr, (zb_uint8_t*)(&hdr[1]));
    TRACE_MSG(TRACE_TRANSPORT3, ">>add_invisible_short short_addr 0x%x", (FMT__H, short_addr));
    MAC_ADD_INVISIBLE_SHORT(short_addr);
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_LIMIT_VISIBILITY */

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_RM_INVISIBLE_SHORT:
  Request:
    7b: Common request header
    2b: Short address
  Response:
    5b: Common response header
*/
static zb_uint16_t rm_invisible_short(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_LIMIT_VISIBILITY
  zb_uint16_t short_addr = 0;
#endif /* ZB_LIMIT_VISIBILITY */

  TRACE_MSG(TRACE_TRANSPORT3, ">>rm_invisible_short len %hu", (FMT__H, len));

#ifdef ZB_LIMIT_VISIBILITY
  if (len < sizeof(*hdr) + sizeof(short_addr))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    ZB_LETOH16(&short_addr, (zb_uint8_t*)(&hdr[1]));
    TRACE_MSG(TRACE_TRANSPORT3, ">>rm_invisible_short short_addr 0x%x", (FMT__H, short_addr));
    MAC_REMOVE_INVISIBLE_SHORT(short_addr);
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_LIMIT_VISIBILITY */

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_SET_NWK_KEY:
  Request:
    7b: Common request header
    16b: NWK key
    1b: key number
  Response:
    5b: Common response header
*/
static zb_uint16_t set_nwk_key(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t* nwk_key = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t key_number = nwk_key[ZB_CCM_KEY_SIZE];

  TRACE_MSG(TRACE_TRANSPORT3, ">>set_nwk_key len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + ZB_CCM_KEY_SIZE + sizeof(key_number))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    zb_secur_setup_nwk_key(nwk_key, key_number);
  }

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

#ifndef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
/*
ID = NCP_HL_GET_SERIAL_NUMBER:
  Request:
    7b: Common request header
  Response:
    5b: Common response header
    16b: Serial Number (16 ASCII characters)
*/
static zb_uint16_t cfg_get_serial_number(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;

  (void)len;

  TRACE_MSG(TRACE_TRANSPORT3, ">>cfg_get_serial_number", (FMT__0));

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(NCP_CTX.serial_number));
  ZB_MEMCPY(&rh[1], NCP_CTX.serial_number, sizeof(NCP_CTX.serial_number));

  return txlen;
}

/*
ID = NCP_HL_GET_VENDOR_DATA:
  Request:
    7b: Common request header
  Response:
    5b: Common response header
    1b: Vendor data size
    0-255b: Binary vendor data
*/
static zb_uint16_t cfg_get_vendor_data(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;

  (void)len;

  TRACE_MSG(TRACE_TRANSPORT3, ">>cfg_get_vendor_data", (FMT__0));

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret,
      sizeof(zb_uint8_t) + NCP_CTX.vendor_data_size);

  p = (zb_uint8_t*)&rh[1];
  *p = NCP_CTX.vendor_data_size;
  ++p;
  ZB_MEMCPY(p, NCP_CTX.vendor_data, sizeof(NCP_CTX.vendor_data));

  return txlen;
}
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */

static void add_nwk_key_to_rsp_by_seq_num(zb_uint8_t *p, zb_uint8_t key_seq_num)
{
  zb_uint8_t *nwk_key = secur_nwk_key_by_seq(key_seq_num);
  if (nwk_key != NULL)
  {
    ZB_MEMCPY(p, nwk_key, ZB_CCM_KEY_SIZE);
    p += ZB_CCM_KEY_SIZE;
    *p = key_seq_num;
  }
  else
  {
    /* MISRA rule 15.7 requires empty 'else' branch. */
  }
}

/*
ID = NCP_HL_GET_NWK_KEYS:
  Request:
    7b: Common request header
  Response:
    5b: Common response header
    ZB_SECUR_N_SECUR_MATERIAL*ZB_CCM_KEY_SIZE + ZB_SECUR_N_SECUR_MATERIAL*b: NWK keys
        (now there is a set of 3 NWK keys of 16 bytes each) + key #
*/
static zb_uint16_t get_nwk_keys(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;
  zb_uindex_t i;

  ZVUNUSED(len);

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_nwk_keys", (FMT__0));

#ifdef ZB_HAVE_BLOCK_KEYS
  if (!zb_chip_lock_get())
#endif
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, RET_OK, ZB_SECUR_N_SECUR_MATERIAL*(ZB_CCM_KEY_SIZE + 1U));

    p = (zb_uint8_t*)&rh[1];
    ZB_MEMSET(p, 0, ZB_SECUR_N_SECUR_MATERIAL*(ZB_CCM_KEY_SIZE + 1U));

    add_nwk_key_to_rsp_by_seq_num(p, ZB_NIB().active_key_seq_number);
    p += (ZB_CCM_KEY_SIZE + 1U);

    for (i = 0; i < ZB_SECUR_N_SECUR_MATERIAL; i++)
    {
      if (i != ZB_NIB().active_key_seq_number)
      {
        add_nwk_key_to_rsp_by_seq_num(p, (zb_uint8_t)i);
        p += (ZB_CCM_KEY_SIZE + 1U);
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }
    }
  }
#ifdef ZB_HAVE_BLOCK_KEYS
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, RET_OPERATION_FAILED, 0U);
  }
#endif

  return txlen;
}

/*
ID = NCP_HL_GET_APS_KEY_BY_IEEE:
  Request:
    7b: Common request header
    8b: IEEE address
  Response:
    5b: Common response header
    16b: APS key
*/
static zb_uint16_t get_aps_key_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p = (zb_uint8_t *)&hdr[1];
  zb_ieee_addr_t ieee_addr;
  zb_aps_device_key_pair_set_t *aps_key;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_aps_key_by_ieee len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(ieee_addr))
  {
    ret = RET_INVALID_PARAMETER;
  }
#ifdef ZB_HAVE_BLOCK_KEYS
  else if (zb_chip_lock_get())
  {
    /* May be simple RET_NOT_FOUND */
    ret = NCP_RET_KEYS_LOCKED;
  }
#endif
  else
  {
    ZB_MEMCPY(ieee_addr, p, sizeof(ieee_addr));

    aps_key = zb_secur_get_link_key_by_address(ieee_addr, ZB_SECUR_VERIFIED_KEY);
    if (aps_key == NULL)
    {
      aps_key = zb_secur_get_link_key_by_address(ieee_addr, ZB_SECUR_ANY_KEY_ATTR);
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }

    if (aps_key == NULL)
    {
      ret = RET_NOT_FOUND;
    }
    else
    {
      ret = RET_OK;
    }
  }

  if (RET_OK == ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, ZB_CCM_KEY_SIZE);
    ZB_MEMCPY(&rh[1], aps_key->link_key, ZB_CCM_KEY_SIZE);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_aps_key_by_ieee ret %hu", (FMT__H, len));

  return txlen;
}

/*
  ID = NCP_HL_GET_PARENT_ADDRESS:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    2b: parent short address
*/
static zb_uint16_t get_parent_address(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t parent_address;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_parent_address len %hu", (FMT__H, len));

  (void)len;

  parent_address = zb_nwk_get_parent();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(parent_address));
  ZB_HTOLE16(&rh[1], &parent_address);

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_parent_address txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_EXTENDED_PAN_ID:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    8b: Extended PAN ID
*/
static zb_uint16_t get_extended_pan_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_extended_pan_id len %hu", (FMT__H, len));

  (void)len;

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(zb_ext_pan_id_t));
  p = (zb_uint8_t*)&rh[1];
  zb_get_extended_pan_id(p);

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_extended_pan_id txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_COORDINATOR_VERSION:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: Coordinator version
*/
static zb_uint16_t get_coordinator_version(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t coordinator_version;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_coordinator_version len %hu", (FMT__H, len));

  (void)len;

  coordinator_version = zb_aib_get_coordinator_version();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(coordinator_version));
  p = (zb_uint8_t*)&rh[1];
  *p = coordinator_version;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_coordinator_version txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_SHORT_ADDRESS:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    2b: Current short address
*/
static zb_uint16_t get_short_address(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t short_address;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_short_address len %hu", (FMT__H, len));

  (void)len;

  short_address = zb_get_short_address();
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(short_address));
  ZB_HTOLE16(&rh[1], &short_address);

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_short_address txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_TRUST_CENTER_ADDRESS:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    8b: Trust Center address
*/
static zb_uint16_t get_trust_center_address(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_trust_center_address len %hu", (FMT__H, len));

  (void)len;

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(zb_ieee_addr_t));
  p = (zb_uint8_t*)&rh[1];
  zb_aib_get_trust_center_address(p);

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_trust_center_address txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_CONFIG_PARAMETER:
  Request:
    5b: Common request header
    1b: Request parameter id
  Response:
    7b: Common response header
    1b: Requested parameter id
    0..4b: Requested parameter value(s), 0b - if error
*/

static zb_uint16_t get_config_parameter(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;
  ncp_hl_config_parameter_t prm_id;
  zb_uint8_t prms_len;
  zb_uint16_t uint16;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_config_parameter len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(zb_uint8_t))
  {
    ret = RET_INVALID_PARAMETER;
  }
  else
  {
    prm_id = (ncp_hl_config_parameter_t) *((zb_uint8_t*)&hdr[1]);

    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, 0);
    p = (zb_uint8_t*)&rh[1];
    *p = prm_id;
    p++;
    /* most parameters are one byte in size */
    prms_len = 1;

    switch(prm_id)
    {
      case NCP_CP_IEEE_ADDR_TABLE_SIZE:
        *p = ZB_IEEE_ADDR_TABLE_SIZE;
        break;
      case NCP_CP_NEIGHBOR_TABLE_SIZE:
        *p = ZB_NEIGHBOR_TABLE_SIZE;
        break;
      case NCP_CP_APS_SRC_BINDING_TABLE_SIZE:
        *p = ZB_APS_SRC_BINDING_TABLE_SIZE;
        break;
      case NCP_CP_APS_GROUP_TABLE_SIZE:
        *p = ZB_APS_GROUP_TABLE_SIZE;
        break;
      case NCP_CP_NWK_ROUTING_TABLE_SIZE:
        *p = ZB_NWK_ROUTING_TABLE_SIZE;
        break;
      case NCP_CP_NWK_ROUTE_DISCOVERY_TABLE_SIZE:
        *p = ZB_NWK_ROUTE_DISCOVERY_TABLE_SIZE;
        break;
      case NCP_CP_IOBUF_POOL_SIZE:
        *p = ZB_IOBUF_POOL_SIZE;
        break;
      case NCP_CP_PANID_TABLE_SIZE:
        *p = ZB_PANID_TABLE_SIZE;
        break;
      case NCP_CP_APS_DUPS_TABLE_SIZE:
        *p = ZB_APS_DUPS_TABLE_SIZE;
        break;
      case NCP_CP_APS_BIND_TRANS_TABLE_SIZE:
        *p = ZB_APS_BIND_TRANS_TABLE_SIZE;
        break;
      case NCP_CP_N_APS_RETRANS_ENTRIES:
        *p = ZB_N_APS_RETRANS_ENTRIES;
        break;
      case NCP_CP_NWK_MAX_HOPS:
        *p = ZB_NIB_MAX_DEPTH() * 2U;
        break;
#ifdef ZB_ROUTER_ROLE
      case NCP_CP_NIB_MAX_CHILDREN:
        *p = zb_get_max_children();
        break;
#endif
      case NCP_CP_N_APS_KEY_PAIR_ARR_MAX_SIZE:
        *p = ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE;
        break;
      case NCP_CP_NWK_MAX_SRC_ROUTES:
        *p = ZB_NWK_MAX_SRC_ROUTES;
        break;
      case NCP_CP_APS_MAX_WINDOW_SIZE:
        *p = ZB_APS_MAX_WINDOW_SIZE;
        break;
      case NCP_CP_APS_INTERFRAME_DELAY:
        /* in milliseconds */
        *p = ZB_APS_INTERFRAME_DELAY;
        break;
      case NCP_CP_ZDO_ED_BIND_TIMEOUT:
        /* in seconds */
        *p = ZDO_CTX().conf_attr.enddev_bind_timeout;
        break;
      case NCP_CP_NIB_PASSIVE_ASK_TIMEOUT:
        /* in milliseconds */
        uint16 = (zb_uint16_t)(ZB_NWK_OCTETS_TO_US(ZB_NIB().passive_ack_timeout) / 1000U);
        ZB_HTOLE16(p, &uint16);
        prms_len = (zb_uint8_t)sizeof(zb_uint16_t);
        break;
      case NCP_CP_APS_ACK_TIMEOUTS:
        uint16 = (zb_uint16_t)(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_N_APS_ACK_WAIT_DURATION_FROM_NON_SLEEPY));
        ZB_HTOLE16(p, &uint16);
        p += sizeof(zb_uint16_t);
        uint16 = (zb_uint16_t)(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_N_APS_ACK_WAIT_DURATION_FROM_SLEEPY));
        ZB_HTOLE16(p, &uint16);
        prms_len = (zb_uint8_t)(2U * sizeof(zb_uint16_t));
        break;
      case NCP_CP_MAC_BEACON_JITTER:
        uint16 = (zb_uint16_t)(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_MAC_HANDLE_BEACON_REQ_LOW_TMO));
        ZB_HTOLE16(p, &uint16);
        p += sizeof(zb_uint16_t);
        uint16 = (zb_uint16_t)(ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_MAC_HANDLE_BEACON_REQ_HI_TMO + ZB_MAC_HANDLE_BEACON_REQ_LOW_TMO));
        ZB_HTOLE16(p, &uint16);
        prms_len = (zb_uint8_t)(2U * sizeof(zb_uint16_t));
        break;
#ifdef MAC_CTX
      case NCP_CP_TX_POWER:
        *p = (zb_uint8_t)(MAC_CTX().default_tx_power);
        p += sizeof(zb_uint8_t);
        *p = (zb_uint8_t)(MAC_CTX().current_tx_power);
        prms_len = (zb_uint8_t)(2U * sizeof(zb_uint8_t));
        break;
#endif
#ifdef ZB_ENABLE_ZLL
      case NCP_CP_ZLL_DEFAULT_RSSI_THRESHOLD:
        *p = ZB_ZLL_DEFAULT_RSSI_THRESHOLD;
        break;
#endif
      case NCP_CP_NIB_MTORR:
        *p = (zb_uint8_t)(ZB_NIB_GET_IS_CONCENTRATOR());
        p += sizeof(zb_uint8_t);
        *p = ZB_NIB_GET_CONCENTRATOR_RADIUS();
        p += sizeof(zb_uint8_t);
        /* in seconds */
        uint16 = (zb_uint16_t)ZB_NIB_GET_CONCENTRATOR_DISC_TIME();
        ZB_HTOLE16(p, &uint16);
        prms_len = (zb_uint8_t)(2U * sizeof(zb_uint8_t) + sizeof(zb_uint16_t));
        break;
      default:
        ret = RET_NOT_FOUND;
        break;
    }
    txlen += 1U; /* Requested parameter id */
    txlen += prms_len;
  }

  if (RET_OK != ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_config_parameter txlen %u", (FMT__D, txlen));

  return txlen;
}

#ifdef ZB_HAVE_BLOCK_KEYS
/*
  ID = NCP_HL_GET_LOCK_STATUS:
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: Chip lock status: 0 - not locked, 1 - locked
*/
static zb_uint16_t get_lock_status(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  ZVUNUSED(len);

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, RET_OK, sizeof(zb_uint8_t));

  p = (zb_uint8_t*)&rh[1];
  *p = zb_chip_lock_get() ? 1U : 0U;

  return txlen;
}
#endif


#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
/*
  ID = NCP_HL_PRODUCTION_CONFIG_READ:
  Request:
    5b: Common request header

  Response:
    7b: Common response header
    2b: production config data length
    8b: production config header
      4b: CRC
      2b: length (with application section)
      2b: version
    [production config length] b: production config body

*/
static zb_uint_t production_config_read(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#ifdef ZB_PRODUCTION_CONFIG
  zb_ret_t ret = RET_OK;
  zb_production_config_hdr_t prod_cfg_hdr;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *response_data;
  zb_uint_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> production_config_read len %hu", (FMT__H, len));

  ZVUNUSED(len);

  if (ret == RET_OK)
  {
    if (!zb_osif_prod_cfg_check_presence())
    {
      TRACE_MSG(TRACE_INFO1, "no production config block found", (FMT__0));
      ret = RET_NOT_FOUND;
    }
  }

  if (ret == RET_OK)
  {
    zb_ret_t read_header_ret = zb_osif_prod_cfg_read_header((zb_uint8_t *)&prod_cfg_hdr,
      sizeof(zb_production_config_hdr_t));

    if (read_header_ret != RET_OK)
    {
      TRACE_MSG(TRACE_INFO1, "impossible to read production config header, ret %d", (FMT__D, read_header_ret));
      ret = RET_ERROR;
    }
    else
    {
      TRACE_MSG(TRACE_INFO1, "production config header is read, len %d, version %d",
        (FMT__D_D, prod_cfg_hdr.len, prod_cfg_hdr.version));
    }
  }

  if (ret == RET_OK)
  {
    zb_ret_t read_body_ret;
    zb_uint16_t data_length = prod_cfg_hdr.len;

    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(data_length) + data_length);
    response_data = (zb_uint8_t*)&rh[1];

    ZB_HTOLE16(response_data, &data_length);
    response_data += sizeof(data_length);

    read_body_ret = zb_osif_prod_cfg_read(response_data, prod_cfg_hdr.len, 0);

    if (read_body_ret != RET_OK)
    {
      TRACE_MSG(TRACE_INFO1, "impossible to read production config body, ret %d", (FMT__D, read_body_ret));
      ret = RET_ERROR;
    }
    else
    {
      TRACE_MSG(TRACE_INFO1, "production config body is read", (FMT__0));
    }
  }

#else
  zb_uint_t txlen;
  zb_ret_t ret = RET_NOT_IMPLEMENTED;

  ZVUNUSED(len);

#endif

  if (ret != RET_OK)
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< production_config_read txlen %u", (FMT__D, txlen));

  return txlen;
}
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST  */


#ifdef ZB_NVRAM_ENABLE_DIRECT_API
static zb_ret_t nvram_get_write_req_hdr(ncp_hl_body_t *body, ncp_hl_nvram_write_req_hdr_t *hdr)
{
  ncp_hl_body_get_u8(body, &hdr->dataset_qnt);
  return RET_OK;
}


static zb_ret_t nvram_get_write_req_ds_hdr(ncp_hl_body_t *body,
                                           ncp_hl_nvram_write_req_ds_hdr_t *hdr)
{
  ncp_hl_body_get_u16(body, &hdr->type);
  ncp_hl_body_get_u16(body, &hdr->version);
  ncp_hl_body_get_u16(body, &hdr->len);

  return RET_OK;
}
#endif

/*
  ID = NCP_HL_NVRAM_WRITE:
  Request:
    5b: Common request header
    1b: Dataset Quantity
    variable length * Dataset Quantity: datasets
      Dataset Header:
        1b: type
        2b: version
        2b: length
      Dataset Body:
        length specified in the dataset header

  Response:
    7b: Common response header
*/
static zb_uint16_t nvram_write(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

#ifdef ZB_NVRAM_ENABLE_DIRECT_API
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  ncp_hl_nvram_write_req_hdr_t common_hdr;
  ncp_hl_nvram_write_req_ds_hdr_t ds_hdr;

  ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nvram_write len %hu", (FMT__H, len));

  len -= sizeof(*hdr);

  if (ret == RET_OK)
  {
    if (len < sizeof(common_hdr))
    {
      ret = RET_INVALID_FORMAT;
    }
    else
    {
      nvram_get_write_req_hdr(&body, &common_hdr);
      len -= sizeof(common_hdr);
    }
  }

  if (ret == RET_OK)
  {
    zb_uindex_t curr_ds_index;
    zb_uint8_t *p;

    zb_nvram_transaction_start();

    for (curr_ds_index = 0; curr_ds_index < common_hdr.dataset_qnt; curr_ds_index++)
    {
      if (len < sizeof(ds_hdr))
      {
        ret = RET_INVALID_FORMAT;
        break;
      }
      nvram_get_write_req_ds_hdr(&body, &ds_hdr);
      len -= sizeof(ds_hdr);

      if (len < ds_hdr.len)
      {
        ret = RET_INVALID_FORMAT;
        break;
      }

      ncp_hl_body_get_ptr(&body, &p, ds_hdr.len);

      if (zb_nvram_write_dataset_direct(ds_hdr.type,
                                        ds_hdr.version,
                                        p,
                                        ds_hdr.len) != RET_OK)
      {
        ret = RET_ERROR;
        break;
      }

      len -= ds_hdr.len;
    }

    zb_nvram_transaction_commit();
  }
#else
  ret = RET_NOT_IMPLEMENTED;

  ZVUNUSED(len);
#endif

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NVRAM_WRITE, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<<nvram_write txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_GET_NWK_LEAVE_ALLOWED
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: value 0-denied, 1-allowed
*/
static zb_uint16_t get_nwk_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_nwk_leave_allowed len %hu", (FMT__H, len));

  (void)len;

  val = (ZB_NIB_GET_LEAVE_REQ_ALLOWED() != 0U);
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  *p = val ? 1U : 0U;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_nwk_leave_allowed txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
ID = NCP_HL_SET_NWK_LEAVE_ALLOWED:
  Request:
    header:
      2b: ID
    body:
      1b: value 0-denied, 1-allowed
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_nwk_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_PRO_STACK
  zb_uint8_t value;
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "set_nwk_leave_allowed len %d", (FMT__D, len));

#ifdef ZB_PRO_STACK
  if (len < sizeof(*hdr) + sizeof(value))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    value = *(zb_uint8_t*)&hdr[1];
    if (value != 0U)
    {
      ZB_NIB_SET_LEAVE_REQ_ALLOWED(ZB_TRUE);
    }
    else
    {
      ZB_NIB_SET_LEAVE_REQ_ALLOWED(ZB_FALSE);
    }
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_PRO_STACK */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_NWK_LEAVE_ALLOWED, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GET_ZDO_LEAVE_ALLOWED
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: value 0-denied, 1-allowed
*/
static zb_uint16_t get_zdo_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_zdo_leave_allowed len %hu", (FMT__H, len));

  (void)len;

  val = (ZB_ZDO_CHECK_IF_LEAVE_WITHOUT_REJOIN_ALLOWED() != 0U);
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00015,12} */
  *p = val ? 1U : 0U;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_zdo_leave_allowed txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
ID = NCP_HL_SET_ZDO_LEAVE_ALLOWED:
  Request:
    header:
      2b: ID
    body:
      1b: value 0-denied, 1-allowed
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_zdo_leave_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
#if defined(ZB_ZDO_DENY_LEAVE_CONFIG)
  zb_uint8_t value;
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "set_zdo_leave_allowed len %d", (FMT__D, len));

#if defined(ZB_ZDO_DENY_LEAVE_CONFIG)
  if (len < sizeof(*hdr) + sizeof(value))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = RET_OK;
    value = *(zb_uint8_t*)&hdr[1];
    if (value != 0U)
    {
      ZB_ZDO_SET_LEAVE_REQ_ALLOWED(ZB_TRUE);
    }
    else
    {
      ZB_ZDO_SET_LEAVE_REQ_ALLOWED(ZB_FALSE);
    }
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_PRO_STACK */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_ZDO_LEAVE_ALLOWED, hdr->tsn, ret, 0);
}

#ifdef ZB_ENABLE_ZGP
/*
  ID = NCP_HL_DISABLE_GPPB:
  Request:
    5b: Common request header

  Response:
    7b: Common response header
*/
static zb_uint_t disable_gppb(ncp_hl_request_header_t *hdr, zb_uint8_t len)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, "disable_gppb len %d", (FMT__D, len));

  if (len != sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = zgp_disable();
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_DISABLE_GPPB, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GP_SET_SHARED_KEY_TYPE:
  Request:
    5b: Common request header
    1b: Key type

  Response:
    7b: Common response header
*/
static zb_uint_t gp_set_shared_key_type(ncp_hl_request_header_t *hdr, zb_uint8_t len)
{
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_ret_t ret = RET_OK;
  zb_uint8_t key_type;

  TRACE_MSG(TRACE_TRANSPORT3, "disable_gppb len %d", (FMT__D, len));

  if (len != sizeof(*hdr) + sizeof(key_type))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ncp_hl_body_get_u8(&body, &key_type);

    TRACE_MSG(TRACE_TRANSPORT3, "key type: %d", (FMT__D, key_type));

    zgp_gp_set_shared_security_key_type((enum zb_zgp_security_key_type_e)key_type);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_GP_SET_SHARED_KEY_TYPE, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GP_SET_DEFAULT_LINK_KEY:
  Request:
    5b: Common request header
    16b: Default key

  Response:
    7b: Common response header
*/
static zb_uint_t gp_set_default_link_key(ncp_hl_request_header_t *hdr, zb_uint8_t len)
{
  zb_uint8_t* link_key = (zb_uint8_t*)(&hdr[1]);
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, "gp_set_default_link_key len %d", (FMT__D, len));

  if (len != sizeof(*hdr) + ZB_CCM_KEY_SIZE)
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(link_key)));

    zgp_set_link_key(link_key);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_GP_SET_DEFAULT_LINK_KEY, hdr->tsn, ret, 0);
}

#endif /* ZB_ENABLE_ZGP */

/*
ID = NCP_HL_SET_LEAVE_WO_REJOIN_ALLOWED:
  Request:
    header:
      2b: ID
    body:
      1b: value 0-denied, 1-allowed
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_leave_without_rejoin_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
#ifdef ZB_PRO_STACK
  zb_uint8_t value;
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "set_leave_without_rejoin_allowed len %d", (FMT__D, len));

#ifdef ZB_PRO_STACK

  if (len < sizeof(*hdr) + sizeof(value))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    value = *(zb_uint8_t*)&hdr[1];
    if (value != 0U)
    {
      ZB_NIB_SET_LEAVE_REQ_WITHOUT_REJOIN_ALLOWED(ZB_TRUE);
    }
    else
    {
      ZB_NIB_SET_LEAVE_REQ_WITHOUT_REJOIN_ALLOWED(ZB_FALSE);
    }
  }
#else
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_PRO_STACK */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_LEAVE_WO_REJOIN_ALLOWED, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_GET_LEAVE_WO_REJOIN_ALLOWED
  Request:
    5b: Common request header
  Response:
    7b: Common response header
    1b: value 0-denied, 1-allowed
*/
static zb_uint16_t get_leave_without_rejoin_allowed(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t val;
  zb_uint8_t *p;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>get_leave_without_rejoin_allowed len %hu", (FMT__H, len));

  (void)len;

  val = (ZB_NIB_GET_LEAVE_REQ_WITHOUT_REJOIN_ALLOWED() != 0U);
  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, sizeof(val));
  p = (zb_uint8_t*)(&rh[1]);
  *p = val ? 1U : 0U;

  TRACE_MSG(TRACE_TRANSPORT3, "<<get_leave_without_rejoin_allowed txlen %u", (FMT__D, txlen));

  return txlen;
}

/*
  ID = NCP_HL_NVRAM_READ:
  Request:
    5b: Common request header
    1b: Dataset Type

  Response:
    7b: Common response header
    2b: NVRAM version
    2b: Dataset Type
    2b: Dataset Version
    2b: Data Length
    Data Length bytes: Data
*/
static zb_uint16_t nvram_read(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;
#ifdef ZB_NVRAM_ENABLE_DIRECT_API
  zb_ret_t ret = RET_OK;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;
  ncp_hl_nvram_read_resp_ds_hdr_t *ds_hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nvram_read len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + 2 /* dataset type */)
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    /* TRICKY: we write only the header here, a payload size will be known later,
       and we will have to add it to txlen later */
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NVRAM_READ, hdr->tsn, ret, sizeof(*ds_hdr));
    p = (zb_uint8_t*)(rh + 1);

    ds_hdr = (ncp_hl_nvram_read_resp_ds_hdr_t*)p;
    p += sizeof(*ds_hdr);

    ncp_hl_body_get_u16(&body, &ds_hdr->type);

    ret = zb_nvram_read_dataset_direct((zb_nvram_dataset_types_t)ds_hdr->type,
                                       p, ZB_NCP_MAX_PACKET_SIZE - txlen,
                                       &ds_hdr->version,
                                       &ds_hdr->len,
                                       &ds_hdr->nvram_version);

    if (ret == RET_OK)
    {
      txlen += ds_hdr->len;
    }
    if (ret != RET_OK && ret != RET_NOT_FOUND)
    {
      ret = RET_ERROR;
    }
  }

  if (ret != RET_OK)
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NVRAM_READ, hdr->tsn, ret, 0);
  }
#else
  ZVUNUSED(len);
  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NVRAM_READ, hdr->tsn, RET_NOT_IMPLEMENTED, 0);
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "<<nvram_read txlen %u", (FMT__D, txlen));

  return txlen;
}


static zb_uint16_t nvram_erase(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nvram_erase len %hu", (FMT__H, len));

  if (len != sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    zb_nvram_erase();
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NVRAM_ERASE, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<<nvram_erase txlen %u", (FMT__D, txlen));

  return txlen;
}


static zb_uint16_t nvram_clear(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nvram_clear len %hu", (FMT__H, len));

  if (len != sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    zb_nvram_clear();
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NVRAM_CLEAR, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<<nvram_clear txlen %u", (FMT__D, txlen));

  return txlen;
}


static zb_ret_t set_tc_policy_internal(zb_uint16_t type, zb_uint8_t value)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">> set_tc_policy_internal, type %d, value %d",
            (FMT__D_D, type, value));

  switch(type)
  {
    case NCP_HL_TC_POLICY_TC_LINK_KEYS_REQUIRED:
      zb_aib_tcpol_set_update_trust_center_link_keys_required((zb_bool_t)value);
      break;
    case NCP_HL_TC_POLICY_IC_REQUIRED:
      zb_set_installcode_policy((zb_bool_t)value);
      break;
    case NCP_HL_TC_POLICY_TC_REJOIN_ENABLED:
      zb_secur_set_tc_rejoin_enabled((zb_bool_t)value);
      break;
    case NCP_HL_TC_POLICY_IGNORE_TC_REJOIN:
      zb_secur_set_ignore_tc_rejoin((zb_bool_t)value);
      break;
    case NCP_HL_TC_POLICY_APS_INSECURE_JOIN:
      zb_zdo_set_aps_unsecure_join((zb_bool_t)value);
      break;
    case NCP_HL_TC_POLICY_DISABLE_NWK_MGMT_CHANNEL_UPDATE:
      zb_zdo_disable_network_mgmt_channel_update((zb_bool_t)value);
      break;
    default:
      ret = RET_NOT_FOUND;
      break;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< set_tc_policy_internal, ret %d", (FMT__D, ret));
  return ret;
}

/*
  ID = NCP_HL_SET_TC_POLICY
  Request:
    5b: Common request header
    2b: Policy type
    1b: Policy value
  Response:
    7b: Common response header
*/
static zb_uint16_t set_tc_policy(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint16_t txlen;
  zb_uint16_t policy_type;
  zb_uint8_t policy_value;

  TRACE_MSG(TRACE_TRANSPORT3, ">>set_tc_policy len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(policy_type) + sizeof(policy_value))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u16(&body, &policy_type);
    ncp_hl_body_get_u8(&body, &policy_value);

    TRACE_MSG(TRACE_TRANSPORT3, "policy type: %d, value: %d",
              (FMT__D_D, policy_type, policy_value));

    ret = set_tc_policy_internal(policy_type, policy_value);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, sizeof(zb_ieee_addr_t));

  TRACE_MSG(TRACE_TRANSPORT3, "<<set_tc_policy txlen %u, ret %d", (FMT__D_D, txlen, ret));

  return txlen;
}


static zb_uint16_t handle_ncp_req_af(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;
  (void)hdr;
  (void)len;

  switch(hdr->hdr.call_id)
  {
    case NCP_HL_AF_DEL_SIMPLE_DESC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_AF_DEL_SIMPLE_DESC", (FMT__0));
      txlen = del_ep(hdr, len);
      break;
    case NCP_HL_AF_SET_SIMPLE_DESC:
      /* Add or update simple descriptor */
      txlen = set_simple_desc(hdr, len);
      break;
    case NCP_HL_AF_SET_NODE_DESC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_AF_SET_NODE_DESC", (FMT__0));
      txlen = set_node_desc(hdr, len);
      break;
    case NCP_HL_AF_SET_POWER_DESC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_AF_SET_POWER_DESC", (FMT__0));
      txlen = set_power_desc(hdr, len);
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_zdo: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }
  return txlen;
}


static zb_af_simple_desc_1_1_t * alloc_new_simple_desc(zb_uint_t n_clusters)
{
  /* zb_af_simple_desc_1_1_t already have space for 2 clusters */
  zb_af_simple_desc_1_1_t *res = NULL;
  zb_uint_t size = sizeof(zb_af_simple_desc_1_1_t) + (n_clusters > 2U ? n_clusters - 2U : 0U) * 2U;

  size = MAGIC_ROUND_TO_4(size);
#if 0
  if (sizeof(NCP_CTX.simple_descs_mem) - NCP_CTX.used_simple_descs >= size)
  {
    res = (zb_af_simple_desc_1_1_t *)(((zb_uint8_t*)NCP_CTX.simple_descs_mem) + NCP_CTX.used_simple_descs);
    NCP_CTX.used_simple_descs += size;
    TRACE_MSG(TRACE_TRANSPORT3, "allocated simple desc of size %d. new used_simple_descs %d", (FMT__D_D, size, NCP_CTX.used_simple_descs));
  }
#endif
  if (sizeof(SIMPLE_DESCS_POOL.mem) - SIMPLE_DESCS_POOL.used >= size)
  {
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,4} */
    res = (zb_af_simple_desc_1_1_t *)(((zb_uint8_t*)SIMPLE_DESCS_POOL.mem) + SIMPLE_DESCS_POOL.used);
    SIMPLE_DESCS_POOL.used += size;
    SIMPLE_DESCS_POOL.meta[SIMPLE_DESCS_POOL.cnt].size = (zb_uint8_t)size;
    SIMPLE_DESCS_POOL.cnt++;
    TRACE_MSG(TRACE_TRANSPORT3, "allocated simple desc of size %d. new used_simple_descs %d", (FMT__D_D, size, SIMPLE_DESCS_POOL.used));
  }

  return res;
}

static void delete_simple_desc(zb_af_simple_desc_1_1_t * ptr, zb_uint_t num)
{
  zb_uindex_t intial_index;
  zb_uindex_t i;

  ZB_MEMSET(ptr, 0, SIMPLE_DESCS_POOL.meta[num].size);
  SIMPLE_DESCS_POOL.used -= SIMPLE_DESCS_POOL.meta[num].size;
  SIMPLE_DESCS_POOL.meta[num].size = 0;

  for(i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "before delete_simple_desc ZB_ZDO_SIMPLE_DESC_LIST()[%d] %p", (FMT__D_P, i, ZB_ZDO_SIMPLE_DESC_LIST()[i]));
  }

  intial_index = ((zb_uindex_t)num)+1U;
  for(i = intial_index; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
  {
    /* mem areas may overlap, so memmove instead memcpy */
    ZB_MEMMOVE(ptr, ZB_ZDO_SIMPLE_DESC_LIST()[i], SIMPLE_DESCS_POOL.meta[i].size);
    SIMPLE_DESCS_POOL.meta[i-1U].size = SIMPLE_DESCS_POOL.meta[i].size;
    ZB_ZDO_SIMPLE_DESC_LIST()[i-1U] = ZB_ZDO_SIMPLE_DESC_LIST()[i];
    ptr += SIMPLE_DESCS_POOL.meta[i].size;
  }
  ZB_ZDO_SIMPLE_DESC_NUMBER()--;

  for(i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "after delete_simple_desc ZB_ZDO_SIMPLE_DESC_LIST()[%d] %p", (FMT__D_P, i, ZB_ZDO_SIMPLE_DESC_LIST()[i]));
  }
}

static zb_uint16_t set_simple_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p;
  zb_af_simple_desc_1_1_t *dsc = NULL;
  zb_uint8_t ep;
  zb_uint8_t in_clu_cnt;
  zb_uint8_t out_clu_cnt;
  zb_uint_t in_out_clu_cnt;
  zb_uint_t i;

  TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc len %d", (FMT__D, len));
  /*
    Format:

    ep 1b
    profile_id 2b
    device_id 2b
    device_version 1b
    in_clu_count 1b
    out_clu_count 1b
    clusters[in_clu_count + out_clu_count]
  */
  if (len < sizeof(*hdr) + 8U)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc: len is too small", (FMT__D, ret));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    p = (zb_uint8_t *)(&hdr[1]);
    ep = *p;
    in_clu_cnt = p[6];
    out_clu_cnt = p[7];
    in_out_clu_cnt = (zb_uint_t)in_clu_cnt + (zb_uint_t)out_clu_cnt;
    TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc: ep %hd in_clu_cnt %d out_clu_cnt %d", (FMT__H_D_D, ep, in_clu_cnt, out_clu_cnt));
    if (len != sizeof(*hdr) + (in_out_clu_cnt) * 2U + 8U)
    {
      ret = RET_INVALID_FORMAT;
    }
  }
  if (ret == RET_OK)
  {
    if (*p == 0U)       /* Can't set via that API simple desc for ZDO */
    {
      ret = RET_INVALID_PARAMETER_1;
    }
  }
  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc: ZB_ZDO_SIMPLE_DESC_NUMBER %d", (FMT__D, ZB_ZDO_SIMPLE_DESC_NUMBER()));
    for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
    {
      if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint == ep)
      {
        TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc: found ep i %d", (FMT__D, i));
        if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_input_cluster_count + ZB_ZDO_SIMPLE_DESC_LIST()[i]->app_output_cluster_count <= in_clu_cnt + out_clu_cnt)
        {
          dsc = ZB_ZDO_SIMPLE_DESC_LIST()[i];
          TRACE_MSG(TRACE_TRANSPORT3, "will update simple desc i %d as invalid", (FMT__D, i));
        }
        else
        {
          /* Mark dsc as invalid. */
          TRACE_MSG(TRACE_TRANSPORT3, "mark simple desc i %d as invalid", (FMT__D, i));
          ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint = (zb_uint8_t)-1;
        }
        break;
      }
    }
    if (dsc == NULL)
    {
      if (ZB_ZDO_SIMPLE_DESC_NUMBER() >= 1U/*zdo*/ + ZB_MAX_EP_NUMBER/*zcl*/)
      {
        TRACE_MSG(TRACE_ERROR, "Already set %d endpoints - can't allocate more", (FMT__D, ZB_ZDO_SIMPLE_DESC_NUMBER()));
        ret = RET_OVERFLOW;
      }
      else
      {
        dsc = alloc_new_simple_desc(in_out_clu_cnt);
        if (dsc == NULL)
        {
          ret = RET_NO_MEMORY;
        }
        else
        {
          TRACE_MSG(TRACE_TRANSPORT3, "adding new simple desc %p", (FMT__P, dsc));
          ret = zb_add_simple_descriptor(dsc);
          if (ret != RET_OK)
          {
            TRACE_MSG(TRACE_ERROR, "zb_add_simple_descriptor failed [%d]", (FMT__D, ret));
          }
        }
      }
    }
  }
  if (ret == RET_OK)
  {
    /* Now fill descriptor */

    dsc->endpoint = ep;
    p++;
    ZB_LETOH16(&dsc->app_profile_id, p);
    p++;
    p++;
    ZB_LETOH16(&dsc->app_device_id, p);
    p++;
    p++;
    dsc->app_device_version = *p;
    p++;
    dsc->app_input_cluster_count = *p;
    p++;
    dsc->app_output_cluster_count = *p;
    p++;
    TRACE_MSG(TRACE_TRANSPORT3, "set simple desc ep %hd prof_id 0x%x dev_id 0x%x dev_ver 0x%hx in_clu_c %hd out_clu_c %hd",
              (FMT__H_D_D_D_H_H, dsc->endpoint, dsc->app_profile_id, dsc->app_device_id, dsc->app_device_version, dsc->app_input_cluster_count, dsc->app_output_cluster_count));
    for (i = 0 ; i < in_out_clu_cnt ; ++i)
    {
      ZB_LETOH16(&dsc->app_cluster_list[i], p);
      TRACE_MSG(TRACE_TRANSPORT3, "cluster i %d = 0x%x", (FMT__D_D, i, dsc->app_cluster_list[i]));
      p++;
      p++;
    }

#if defined ZB_ENABLE_SE_MIN_CONFIG || defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
    /* Not sure failing of ncp_sync_kec_subg_for_ep is ever possible: the
     * only reason is "to many descriptors" case, but we must catch it
     * before.. We already allocated and added ep dsc, so it is hard
     * to rollback.  */
    ret = ncp_sync_kec_subg_for_ep(dsc);
#endif /* ZB_ENABLE_SE_MIN_CONFIG */
  }

  TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc: ZB_ZDO_SIMPLE_DESC_NUMBER %d", (FMT__D, ZB_ZDO_SIMPLE_DESC_NUMBER()));
  TRACE_MSG(TRACE_TRANSPORT3, "set_simple_desc ret %d", (FMT__D, ret));

#if TRACE_ENABLED(TRACE_TRANSPORT3)
  {
    for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
    {
      dsc = ZB_ZDO_SIMPLE_DESC_LIST()[i];
      TRACE_MSG(TRACE_TRANSPORT3, "ep %hd prof_id 0x%x dev_id 0x%x dev_ver 0x%hx in_clu_c %hd out_clu_c %hd",
                (FMT__H_D_D_D_H_H, dsc->endpoint, dsc->app_profile_id, dsc->app_device_id, dsc->app_device_version, dsc->app_input_cluster_count, dsc->app_output_cluster_count));
    }
  }
#endif /* TRACE_ENABLED(TRACE_TRANSPORT3) */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_AF_SET_SIMPLE_DESC, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_AF_DEL_SIMPLE_DESC:
  Request:
    header:
      2b: ID
    body:
      1b: endpoint
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t del_ep(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t ep;
  zb_uint8_t i;
  zb_uint8_t desc_num = 0;
  zb_af_simple_desc_1_1_t *desc_for_deleting = NULL;

  TRACE_MSG(TRACE_TRANSPORT3, "del_ep len %d", (FMT__D, len));

  if (len < sizeof(*hdr) +  sizeof(ep))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ep = *(zb_uint8_t*)&hdr[1];
    if ((ep < 1U) || (ep > 254U))
    {
      ret = RET_INVALID_PARAMETER;
    }
    else
    {
      for (i = 0; i < ZB_ZDO_SIMPLE_DESC_NUMBER(); i++)
      {
        TRACE_MSG(TRACE_TRANSPORT3, "del_ep: ep %d = %d", (FMT__D_D, i, ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint));
        if (ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint == ep)
        {
          /* mark found ep as invalid for prevents its usage */
          TRACE_MSG(TRACE_TRANSPORT3, "mark simple desc i %d as invalid", (FMT__D, i));
          ZB_ZDO_SIMPLE_DESC_LIST()[i]->endpoint = (zb_uint8_t)-1;
          desc_for_deleting = ZB_ZDO_SIMPLE_DESC_LIST()[i];
          desc_num = i;
          break;
        }
      }
      if (desc_for_deleting != NULL)
      {
        delete_simple_desc(desc_for_deleting, desc_num);
      }
      else
      {
        ret = RET_NOT_FOUND;
      }
    }
  }

  TRACE_MSG(TRACE_TRANSPORT3, "del_ep ret %d", (FMT__D, ret));

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_AF_DEL_SIMPLE_DESC, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_AF_SET_NODE_DESC:
  Request:
    header:
      2b: ID
    body:
      1b: device type (0-zc, 1-zr, 2-zed)
      1b: mac capabilities: device type, power source (1-mains power, 0-otherwise), rx on idle (1-on, 0-off), allocate address (1/0)
      2b: manufacturer code
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_node_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t device_type;
  zb_uint8_t mac_cap;
  zb_uint16_t manufacturer_code = 0;
  zb_uint8_t *p = (zb_uint8_t*)&hdr[1];

  TRACE_MSG(TRACE_TRANSPORT3, "set_node_desc len %d", (FMT__D, len));

  if (len < sizeof(*hdr) + sizeof(device_type)
                         + sizeof(mac_cap)
                         + sizeof(manufacturer_code))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    device_type = *p;
    p++;
    mac_cap = *p;
    p++;
    ZB_LETOH16(&manufacturer_code, p);
    if (device_type > ZB_NWK_DEVICE_TYPE_ED)
    {
      ret = RET_INVALID_PARAMETER_1;
    }

    if (ret == RET_OK)
    {
      if (ZB_IS_DEVICE_ZC_OR_ZR())
      {
        if ((ZB_MAC_CAP_GET_DEVICE_TYPE(mac_cap) != 0U)
            && (ZB_MAC_CAP_GET_POWER_SOURCE(mac_cap) != 0U)
            && (ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(mac_cap) != 0U))
        {
          ret = RET_OK;
        }
        else
        {
          ret = RET_INVALID_PARAMETER_2;
        }
      }
      else
      {
        if (ZB_MAC_CAP_GET_DEVICE_TYPE(mac_cap) == 0U)
        {
          ret = RET_OK;
        }
        else
        {
          ret = RET_INVALID_PARAMETER_2;
        }
      }
    }

    if (RET_OK == ret)
    {
      zb_set_node_descriptor((zb_logical_type_t)device_type, ZB_U2B(ZB_MAC_CAP_GET_POWER_SOURCE(mac_cap)),
                                          ZB_U2B(ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(mac_cap)),
                                          ZB_U2B(ZB_MAC_CAP_GET_ALLOCATE_ADDRESS(mac_cap)));
      zdo_set_node_descriptor_manufacturer_code(manufacturer_code);
    }
  }

  TRACE_MSG(TRACE_TRANSPORT3, "set_node_desc ret %d", (FMT__D, ret));

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_AF_SET_NODE_DESC, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_AF_SET_POWER_DESC:
  Request:
    header:
      2b: ID
    body:
      1b: current power mode (0 - receiver synchronized with the receiver on when
                                  idle subfield of the node descriptor,
                              1 - receiver comes on periodically as defined by the
                                  node power descriptor,
                              2 - receiver comes on when stimulated, for example,
                                  by a user pressing a button)
      1b: available power sources bitfield (bit #0 - constant power
                                            bit #1 - rechargeable battery
                                            bit #2 - disposable battery)
      1b: current power source - ONLY one source can be chosen from available power sources
                                              (bit #0 - constant power
                                            OR bit #1 - rechargeable battery
                                            OR bit #2 - disposable battery)
      1b: current power source level (0  - critical
                                      4  - 33%
                                      8  - 66%
                                      12 - 100%)
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_power_desc(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t cur_pwr_mode;
  zb_uint8_t available_pwr_srcs;
  zb_uint8_t cur_pwr_src;
  zb_uint8_t cur_pwr_src_lvl;
  zb_uint8_t *p = (zb_uint8_t*)&hdr[1];

  TRACE_MSG(TRACE_TRANSPORT3, "set_power_desc len %d", (FMT__D, len));

  if (len < sizeof(*hdr) + sizeof(cur_pwr_mode)
                         + sizeof(available_pwr_srcs)
                         + sizeof(cur_pwr_src)
                         + sizeof(cur_pwr_src_lvl))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    cur_pwr_mode = *p;
    p++;
    available_pwr_srcs = *p;
    p++;
    cur_pwr_src = *p;
    p++;
    cur_pwr_src_lvl = *p;
    p++;

    TRACE_MSG(TRACE_TRANSPORT3, "set_power_desc cur_pwr_mode %x available_pwr_srcs %x cur_pwr_src %x cur_pwr_src_lvl %x",
                                (FMT__D_D_D_D, cur_pwr_mode, available_pwr_srcs, cur_pwr_src, cur_pwr_src_lvl));

    if (cur_pwr_mode > ZB_POWER_MODE_COME_ON_WHEN_STIMULATED)
    {
      ret = RET_INVALID_PARAMETER_1;
    }

    if ((available_pwr_srcs & (~(ZB_POWER_SRC_CONSTANT
                             |ZB_POWER_SRC_RECHARGEABLE_BATTERY
                             |ZB_POWER_SRC_DISPOSABLE_BATTERY))) != 0U)
    {
      ret = RET_INVALID_PARAMETER_2;
    }

    if ((cur_pwr_src > ZB_POWER_SRC_DISPOSABLE_BATTERY) ||
        (cur_pwr_src == (ZB_POWER_SRC_CONSTANT
                        | ZB_POWER_SRC_RECHARGEABLE_BATTERY)) ||
        (0U == cur_pwr_src))
    {
      ret = RET_INVALID_PARAMETER_3;
    }

    if ( (cur_pwr_src_lvl != ZB_POWER_LEVEL_CRITICAL)
      && (cur_pwr_src_lvl != ZB_POWER_LEVEL_33)
      && (cur_pwr_src_lvl != ZB_POWER_LEVEL_66)
      && (cur_pwr_src_lvl != ZB_POWER_LEVEL_100) )
    {
      ret = RET_INVALID_PARAMETER_4;
    }

    if (RET_OK == ret)
    {
      zb_set_node_power_descriptor((zb_current_power_mode_t)cur_pwr_mode,
                                   available_pwr_srcs, cur_pwr_src, (zb_power_source_level_t)cur_pwr_src_lvl);
      /* 2.3.2.3.6 The power source subfield is one bit in length and shall be set to 1 if the current power source is mains power.
      Otherwise, the power source subfield shall be set to 0. This information is derived from the node current power source
      field of the node power descriptor. */
/* 01/22/2019 EE CR:MINOR Use constant instead of 2 */
      ZB_ZDO_NODE_DESC()->mac_capability_flags &= (~(1U << 2U));
      ZB_MAC_CAP_SET_POWER_SOURCE((ZB_ZDO_NODE_DESC()->mac_capability_flags), ((cur_pwr_src == ZB_POWER_SRC_CONSTANT) ? 1U : 0U));
    }
  }

  TRACE_MSG(TRACE_TRANSPORT3, "set_power_desc ret %d", (FMT__D, ret));

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_AF_SET_POWER_DESC, hdr->tsn, ret, 0);
}

#ifdef SNCP_MODE

static zb_bool_t get_bind_index_by_id(zb_uint8_t id, zb_uint8_t *index)
{
  zb_uint8_t i;

  for (i=0; i < ZG->aps.binding.dst_n_elements; i++)
  {
    if (ZG->aps.binding.dst_table[i].id == id)
    {
      *index = i;
      return ZB_TRUE;
    }
  }
  return ZB_FALSE;
}


/*
 *  Reads binding src and dst table and fills bind_entry based on id
 *  @return ZB_TRUE if "id" is found and bind_entry filled, returns ZB_FALSE otherwise
 */
static zb_bool_t fill_bind_entry(ncp_hl_bind_entry_t *bind_entry, zb_uint8_t id)
{
  zb_aps_bind_src_table_t *src_tbl;
  zb_aps_bind_dst_table_t *dst_tbl;
  zb_uint8_t dst_tbl_index;


  /* Find index from id */
  if (get_bind_index_by_id(id, &dst_tbl_index) == ZB_FALSE)
  {
    return ZB_FALSE;
  }

  dst_tbl = &ZG->aps.binding.dst_table[dst_tbl_index];
  src_tbl = &ZG->aps.binding.src_table[dst_tbl->src_table_index];


  /* Get data from src table */
  bind_entry->src_endpoint =  src_tbl->src_end;

  ZB_HTOLE16(&bind_entry->cluster_id, &src_tbl->cluster_id);

  bind_entry->dst_addr_mode = dst_tbl->dst_addr_mode;

  /* Get that from dst table */
  if(dst_tbl->dst_addr_mode == ZB_APS_BIND_DST_ADDR_GROUP)
  {
    bind_entry->u.short_addr = dst_tbl->u.group_addr;
    ZB_HTOLE16_ONPLACE(bind_entry->u.short_addr);
    /* destination EP not present */
    bind_entry->dst_endpoint = 0;
  }
  else if (dst_tbl->dst_addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
  {
    zb_address_ieee_by_ref(bind_entry->u.long_addr, dst_tbl->u.long_addr.dst_addr);
    bind_entry->dst_endpoint = dst_tbl->u.long_addr.dst_end;
  }
  else
  {
    // dst_addr_mode can either be ZB_APS_BIND_DST_ADDR_LONG or ZB_APS_BIND_DST_ADDR_GROUP
    ZB_ASSERT(0);
  }

  bind_entry->id = dst_tbl->id;
  bind_entry->bind_type = NCP_HL_UNICAST_BINDING;

  return ZB_TRUE;
}

static zb_uint16_t apsme_get_bind_entry_by_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_bind_entry_t bind_entry;
  ncp_hl_response_header_t *rh;
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;


  if (len < (sizeof(*hdr) + sizeof(zb_uint8_t)))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    zb_uint8_t id = *(zb_uint8_t*)(&hdr[1]);

    /* get bind data, returns false if "id" provided is not found */
    if (fill_bind_entry(&bind_entry, id) != ZB_TRUE)
    {
      /* binding does not exist */
      ZB_BZERO(&bind_entry, sizeof(bind_entry));
      bind_entry.bind_type = NCP_HL_UNUSED_BINDING;

      ret = RET_NOT_FOUND;
    }
  }

  if(ret == RET_OK ||
     ret == RET_NOT_FOUND)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_GET_BIND_ENTRY_BY_ID, hdr->tsn, ret, sizeof(bind_entry));
    ZB_MEMCPY(&rh[1], &bind_entry, sizeof(bind_entry));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_GET_BIND_ENTRY_BY_ID, hdr->tsn, ret, 0);
  }

  return txlen;
}

static void handle_rm_bind_entry_by_id(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);


  if (status == RET_OK)
  {
    ncp_hl_response_header_t *rh;
    zb_uint16_t txlen;

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_RM_BIND_ENTRY_BY_ID,
                               stop_single_command(NCP_HL_APSME_RM_BIND_ENTRY_BY_ID),
                               status, 0);

    ncp_send_packet(rh, txlen);
  }
  else
  {
    ncp_hl_send_no_arg_resp(NCP_HL_APSME_RM_BIND_ENTRY_BY_ID, status);
  }

  zb_buf_free(param);
}

static zb_uint16_t apsme_rm_bind_entry_by_id(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_bind_entry_t bind_entry;
  ncp_hl_response_header_t *rh;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_bufid_t buf = 0;


  ret = start_single_command(hdr);

  if (ret == RET_OK &&
      len < (sizeof(*hdr) + sizeof(zb_uint8_t)))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    zb_uint8_t id = *(zb_uint8_t*)(&hdr[1]);

    /* get bind data, returns false if "id" provided is not found */
    if (fill_bind_entry(&bind_entry, id) != ZB_TRUE)
    {
      ret = RET_NOT_FOUND;
    }
    else
    {
      zb_apsme_binding_req_t *req;
      req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);

      /* src addr is allways ours */
      ZB_MEMCPY(&req->src_addr, ZB_PIBCACHE_EXTENDED_ADDRESS(), sizeof(zb_ieee_addr_t));

      req->src_endpoint = bind_entry.src_endpoint;

      req->clusterid = bind_entry.cluster_id;

      req->addr_mode = bind_entry.dst_addr_mode;

      if(req->addr_mode == ZB_APS_BIND_DST_ADDR_GROUP)
      {
        req->dst_addr.addr_short = bind_entry.u.short_addr;
        ZB_HTOLE16_ONPLACE(req->dst_addr.addr_short);

        /* destination EP not present */
        req->dst_endpoint = 0;
      }
      else if (req->addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
      {
        ZB_MEMCPY(&req->dst_addr.addr_long, &bind_entry.u.long_addr, sizeof(zb_ieee_addr_t));
        req->dst_endpoint = bind_entry.dst_endpoint;
      }
      else
      {
        /* MISRA rule 15.7 requires empty 'else' branch. */
      }

      req->confirm_cb = handle_rm_bind_entry_by_id;
      zb_apsme_unbind_request(buf);

    }
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_RM_BIND_ENTRY_BY_ID, hdr->tsn, ret, 0);
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
  }

  return txlen;
}

static zb_uint16_t apsme_clear_bind_table(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;


  if (len < (sizeof(*hdr)))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    zb_apsme_unbind_all(0);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_APSME_CLEAR_BIND_TABLE, hdr->tsn, ret, 0);

  return txlen;
}

static zb_uint16_t apsme_set_remote_bind_offset(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;


  if (len < (sizeof(*hdr) + sizeof(zb_uint8_t)))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    zb_uint8_t req_offset = *(zb_uint8_t*)(&hdr[1]);

    if (
#ifndef ZB_CERTIFICATION_HACKS
        (req_offset > ZB_APS_DST_BINDING_TABLE_SIZE) &&
#else
        (req_offset > ZB_CERT_HACKS().dst_binding_table_size) &&
#endif
        (req_offset != 0xFFU))
    {
      ret = RET_INVALID_PARAMETER;
    }
    else
    {
      ZG->aps.binding.remote_bind_offset = req_offset;
#ifdef ZB_USE_NVRAM
      zb_nvram_transaction_start();
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_BINDING_DATA);
      zb_nvram_transaction_commit();
#endif /* ZB_USE_NVRAM */
    }
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SET_REMOTE_BIND_OFFSET, hdr->tsn, ret, 0);

  return txlen;
}

static zb_uint16_t apsme_get_remote_bind_offset(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_response_header_t *rh;
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;


  if (len < (sizeof(*hdr)))
  {
    ret = RET_INVALID_FORMAT;
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_GET_REMOTE_BIND_OFFSET, hdr->tsn, ret, 0);

  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_REMOTE_BIND_OFFSET,
                               hdr->tsn,
                               ret, sizeof(ZG->aps.binding.remote_bind_offset));

    ZB_MEMCPY(&rh[1], &ZG->aps.binding.remote_bind_offset,
              sizeof(ZG->aps.binding.remote_bind_offset));
  }

  return txlen;
}

void ncp_apsme_remote_bind_unbind_ind( zb_uint8_t param, zb_bool_t bind)
{
  zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
  zb_ret_t ret = zb_buf_get_status(param);
  ncp_hl_bind_entry_t bind_entry;
  ncp_hl_ind_header_t* ih;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ncp_apsme_remote_bind_unbind_ind", (FMT__0));

  /* indication is only sent for NCP on success */
  if (ret == RET_OK)
  {
    zb_uint16_t txlen;

    /* convert to func if reused: fill ncp_h_bind_entry_t with apsme_binding_req_t
      we could get data from current bind table for REMOTE_BIND_ind but for REMOTE_UNBIND_ind data
      is already removed at this point
    */
    bind_entry.dst_endpoint = apsreq->dst_endpoint;
    bind_entry.cluster_id = apsreq->clusterid;
    bind_entry.dst_addr_mode = apsreq->addr_mode;

    bind_entry.src_endpoint = apsreq->src_endpoint;

    if(apsreq->addr_mode == ZB_APS_BIND_DST_ADDR_GROUP)
    {
      bind_entry.u.short_addr = apsreq->dst_addr.addr_short;
      ZB_HTOLE16_ONPLACE(bind_entry.u.short_addr);

      /* destination EP not present */
      bind_entry.dst_endpoint = 0;
    }
    else if (apsreq->addr_mode == ZB_APS_BIND_DST_ADDR_LONG)
    {
      ZB_MEMCPY(&bind_entry.u.long_addr, &apsreq->dst_addr.addr_long, sizeof(zb_ieee_addr_t));
      bind_entry.dst_endpoint = apsreq->dst_endpoint;
    }
    else
    {
      /* apsreq->addr_mode can either be ZB_APS_BIND_DST_ADDR_LONG or ZB_APS_BIND_DST_ADDR_GROUP */
      ZB_ASSERT(0);
    }

    bind_entry.id = apsreq->id;

    if (bind == ZB_TRUE)
    {
      /* bind entry exists */
      bind_entry.bind_type = NCP_HL_UNICAST_BINDING;

      txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_APSME_REMOTE_BIND_IND, sizeof(bind_entry));
    }
    else
    {
      /* bind entry was removed */
      bind_entry.bind_type = NCP_HL_UNUSED_BINDING;

      txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_APSME_REMOTE_UNBIND_IND, sizeof(bind_entry));
    }

    ZB_MEMCPY(&ih[1], &bind_entry, sizeof(bind_entry));

    ncp_send_packet(ih, txlen);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<ncp_apsme_remote_bind_unbind_ind", (FMT__0));
}

#endif /* SNCP_MODE */

static zb_uint16_t handle_ncp_req_zdo(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;
  (void)len;

  switch(hdr->hdr.call_id)
  {
    case NCP_HL_ZDO_NWK_ADDR_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_NWK_ADDR_REQ", (FMT__0));
      txlen = nwk_addr_req(hdr, len);
      break;
    case NCP_HL_ZDO_IEEE_ADDR_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_IEEE_ADDR_REQ", (FMT__0));
      txlen = ieee_addr_req(hdr, len);
      break;
    case NCP_HL_ZDO_POWER_DESC_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_POWER_DESC_REQ", (FMT__0));
      txlen = power_desc_req(hdr, len);
      break;
    case NCP_HL_ZDO_NODE_DESC_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_NODE_DESC_REQ", (FMT__0));
      txlen = node_desc_req(hdr, len);
      break;
    case NCP_HL_ZDO_SIMPLE_DESC_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_SIMPLE_DESC_REQ", (FMT__0));
      txlen = simple_desc_req(hdr, len);
      break;
    case NCP_HL_ZDO_ACTIVE_EP_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_ACTIVE_EP_REQ", (FMT__0));
      txlen = active_ep_desc_req(hdr, len);
      break;
    case NCP_HL_ZDO_MATCH_DESC_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_MATCH_DESC_REQ", (FMT__0));
      txlen = match_desc_req(hdr, len);
      break;
    case NCP_HL_ZDO_BIND_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_BIND_REQ", (FMT__0));
      txlen = bind_req(hdr, len);
      break;
    case NCP_HL_ZDO_UNBIND_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_UNBIND_REQ", (FMT__0));
      txlen = unbind_req(hdr, len);
      break;
    case NCP_HL_ZDO_MGMT_LEAVE_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_MGMT_LEAVE_REQ", (FMT__0));
      txlen = ncp_zdo_mgmt_leave_req(hdr, len);
      break;
    case NCP_HL_ZDO_MGMT_BIND_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_MGMT_BIND_REQ", (FMT__0));
      txlen = ncp_zdo_mgmt_bind_req(hdr, len);
      break;
    case NCP_HL_ZDO_MGMT_LQI_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_MGMT_LQI_REQ", (FMT__0));
      txlen = ncp_zdo_mgmt_lqi_req(hdr, len);
      break;
    case NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ", (FMT__0));
      txlen = ncp_zdo_mgmt_nwk_update_req(hdr, len);
      break;
    case NCP_HL_ZDO_PERMIT_JOINING_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_PERMIT_JOINING_REQ", (FMT__0));
      txlen = zdo_permit_joining(hdr, len);
      break;
    case NCP_HL_ZDO_REJOIN:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_REJOIN", (FMT__0));
      txlen = zdo_rejoin_req(hdr, len);
      break;
    case NCP_HL_ZDO_GET_STATS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_GET_STATS", (FMT__0));
      txlen = zdo_get_stats_req(hdr, len);
      break;
    case NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ", (FMT__0));
      txlen = zdo_system_srv_discovery_req(hdr, len);
      break;
    case NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ", (FMT__0));
      txlen = zdo_set_node_desc_manuf_code_req(hdr, len);
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_zdo: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }

  return txlen;
}


static zb_uint16_t handle_ncp_req_aps(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  switch (hdr->hdr.call_id)
  {
    case NCP_HL_APSDE_DATA_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSDE_DATA_REQ len %d", (FMT__D, len));
      txlen = apsde_data_req(hdr, len);
      break;
    case NCP_HL_APSME_BIND:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_BIND len %d", (FMT__D, len));
      txlen = apsme_bind_req(hdr, len);
      break;
    case NCP_HL_APSME_UNBIND:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_UNBIND len %d", (FMT__D, len));
      txlen = apsme_unbind_req(hdr, len);
      break;
    case NCP_HL_APSME_UNBIND_ALL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_UNBIND_ALL len %d", (FMT__D, len));
      txlen = apsme_unbind_all_req(hdr, len);
      break;
    case NCP_HL_APSME_ADD_GROUP:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_ADD_GROUP len %d", (FMT__D, len));
      txlen = apsme_add_group_req(hdr, len);
      break;
    case NCP_HL_APSME_RM_GROUP:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_RM_GROUP len %d", (FMT__D, len));
      txlen = apsme_rm_group_req(hdr, len);
      break;
    case NCP_HL_APSME_RM_ALL_GROUPS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_RM_ALL_GROUPS len %d", (FMT__D, len));
      txlen = apsme_rm_all_groups_req(hdr, len);
      break;
    case NCP_HL_APS_CHECK_BINDING:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APS_CHECK_BINDING len %d", (FMT__D, len));
      txlen = aps_check_binding_req(hdr, len);
      break;
    case NCP_HL_APS_GET_GROUP_TABLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APS_GET_GROUP_TABLE len %d", (FMT__D, len));
      txlen = aps_get_group_table_req(hdr, len);
      break;
#ifdef SNCP_MODE
    case NCP_HL_APSME_GET_BIND_ENTRY_BY_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_GET_BIND_ENTRY_BY_ID len %d", (FMT__D, len));
      txlen = apsme_get_bind_entry_by_id(hdr, len);
      break;
    case NCP_HL_APSME_RM_BIND_ENTRY_BY_ID:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_RM_BIND_ENTRY_BY_ID len %d", (FMT__D, len));
      txlen = apsme_rm_bind_entry_by_id(hdr, len);
      break;
    case NCP_HL_APSME_CLEAR_BIND_TABLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_APSME_CLEAR_BIND_TABLE len %d", (FMT__D, len));
      txlen = apsme_clear_bind_table(hdr, len);
      break;
    case NCP_HL_SET_REMOTE_BIND_OFFSET:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_REMOTE_BIND_OFFSET len %d", (FMT__D, len));
      txlen = apsme_set_remote_bind_offset(hdr, len);
      break;
    case NCP_HL_GET_REMOTE_BIND_OFFSET:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_GET_REMOTE_BIND_OFFSET len %d", (FMT__D, len));
      txlen = apsme_get_remote_bind_offset(hdr, len);
      break;
#endif /* SNCP_MODE */
    default:
      break;
  }
  return txlen;
}


static zb_uint16_t apsde_data_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t param_len;
  zb_uint16_t data_len;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_apsde_data_req_t req;
  zb_uint8_t buf = 0U;
  zb_uint8_t *datap;

  TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req hdr %p len %d tsn %d", (FMT__P_D_D, hdr, len, hdr->tsn));
#if defined ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ && defined ZB_SUSPEND_APSDE_DATAREQ_BY_SUBGHZ_CLUSTER
  if (ZCL_CTX().subghz_ctx.cli.suspend_zcl_messages)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "Transmission suspended by SubGHz cluster", (FMT__0));
    ret = RET_CANCELLED;
  }
#endif
  if (len < sizeof(*hdr) + sizeof(param_len) + sizeof(data_len))
  {
    TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req len %d wants at least %d", (FMT__D_D, len, sizeof(*hdr) + sizeof(param_len) + sizeof(data_len)));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    param_len = *p;
    p++;
    if (param_len != NCP_APSDE_PARAM_SIZE)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req param_len %d wants %d", (FMT__D_D, param_len, NCP_APSDE_PARAM_SIZE));
      ret = RET_INVALID_FORMAT;
    }
  }
  if (ret == RET_OK)
  {
    ZB_LETOH16(&data_len, p);
    p++;
    p++;

    TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req data_len %d", (FMT__D, data_len));
    if (sizeof(*hdr) + sizeof(param_len) + sizeof(data_len) + param_len + data_len != len)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req sizeof(*hdr) %d + sizeof(param_len) %d + sizeof(data_len) %d + param_len %d + data_len %d = %d len %d",
                (FMT__D_D_D_D_D_D_D, sizeof(*hdr), sizeof(param_len), sizeof(data_len), param_len, data_len,
                 sizeof(*hdr) + sizeof(param_len) + sizeof(data_len) + param_len + data_len,
                 len));
      ret = RET_INVALID_FORMAT;
    }
  }
  if (ret == RET_OK)
  {
    ZB_BZERO(&req, sizeof(req));
    ZB_MEMCPY(&req, p, param_len);
    p = &p[param_len];
    ZB_LETOH16_ONPLACE(req.profileid);
    ZB_LETOH16_ONPLACE(req.clusterid);
    ZB_LETOH16_ONPLACE(req.alias_src_addr);
    if (req.addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
        || req.addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
      ZB_LETOH16_ONPLACE(req.dst_addr.addr_short);
    }


#ifdef SNCP_MODE
    /* update apsde_data_req with destination from bind table if addr_mode == ZB_APS_ADDR_MODE_BIND_TBL_ID */
    if (req.addr_mode == ZB_APS_ADDR_MODE_BIND_TBL_ID)
    {
      zb_uint8_t dst_tbl_index;

      hdr->hdr.call_id = INTERNAL_APSDE_DATA_REQ_BY_BIND_TBL_ID;

      if (get_bind_index_by_id(req.dst_endpoint, &dst_tbl_index) == ZB_FALSE)
      {
        ret = RET_NOT_FOUND;
      }
      else
      {
        /* replace apsde_data_req destination data with bind tbl destination data */
        if (ZG->aps.binding.dst_table[dst_tbl_index].dst_addr_mode ==  ZB_APS_BIND_DST_ADDR_GROUP)
        {
          req.dst_addr.addr_short = ZG->aps.binding.dst_table[dst_tbl_index].u.group_addr;
          ZB_LETOH16_ONPLACE(req.dst_addr.addr_short);
          /* destination EP not present */
          req.dst_endpoint = 0;
        }
        else if (ZG->aps.binding.dst_table[dst_tbl_index].dst_addr_mode ==  ZB_APS_BIND_DST_ADDR_LONG)
        {
          ZB_MEMCPY(&req.dst_addr.addr_long, &ZG->aps.binding.dst_table[dst_tbl_index].u.long_addr.dst_addr,
                    sizeof(zb_ieee_addr_t));
          req.dst_endpoint = ZG->aps.binding.dst_table[dst_tbl_index].u.long_addr.dst_end;
        }
        else
        {
          /* dst_addr_mode can either be ZB_APS_BIND_DST_ADDR_LONG or ZB_APS_BIND_DST_ADDR_GROUP */
          ZB_ASSERT(0);
        }

        /* update addr_mode in apsde_data_req to be interpreted by ZBOSS */
        req.addr_mode = ZG->aps.binding.dst_table[dst_tbl_index].dst_addr_mode;

        ZB_ASSERT(req.addr_mode != ZB_APS_ADDR_MODE_BIND_TBL_ID);
      }
    }

#endif

    if (ret == RET_OK)
    {
      ret = ncp_alloc_buf_size(&buf, data_len + sizeof(req));
    }

    TRACE_MSG(TRACE_TRANSPORT3, "  profile_id %d, cluster_id %d, addr_mode %hd",
      (FMT__D_D_H, req.profileid, req.clusterid, req.addr_mode));
  }

  if (ret == RET_OK)
  {
    datap = zb_buf_initial_alloc(buf, data_len);
    ZB_MEMCPY(datap, p, data_len);
    ZB_MEMCPY(ZB_BUF_GET_PARAM(buf, zb_apsde_data_req_t), &req, sizeof(req));

    store_tsn(buf, hdr->tsn, hdr->hdr.call_id);
    if (ncp_exec_blocked())
    {
      ncp_enqueue_buf(NCP_DATA_REQ, 0, buf);
    }
    else
    {
      (void)ZB_SCHEDULE_APP_CALLBACK(zb_apsde_data_request, buf);
    }
    return NCP_RET_LATER;
  }
  else
  {
    ncp_hl_response_header_t *rh;
    zb_size_t params_len = ZB_OFFSETOF(zb_apsde_data_confirm_t, FIRST_INTERNAL_APSDE_CONF_FIELD);
    zb_uint16_t txlen;
    zb_apsde_data_confirm_t aps_data_conf;

    ZB_BZERO(&aps_data_conf, sizeof(aps_data_conf));

    aps_data_conf.dst_endpoint = req.dst_endpoint;
    aps_data_conf.src_endpoint = req.src_endpoint;
    aps_data_conf.addr_mode = req.addr_mode;

    ZB_MEMCPY(&aps_data_conf.dst_addr, &req.dst_addr, sizeof(aps_data_conf.dst_addr));

    aps_data_conf.tx_time = 0;

    if (aps_data_conf.addr_mode != ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ZB_HTOLE16_ONPLACE(aps_data_conf.dst_addr.addr_short);
    }

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSDE_DATA_REQ, hdr->tsn, ret, params_len);
    ZB_MEMCPY(&rh[1], &aps_data_conf, params_len);

    TRACE_MSG(TRACE_TRANSPORT3, "apsde_data_req status %d %d %d",
              (FMT__D_D_D, ret, ERROR_GET_CATEGORY(ret), ERROR_GET_CODE(ret)));

    return txlen;
  }
}


static void store_tsn(zb_uint8_t buf, zb_uint8_t tsn, zb_uint16_t call_id)
{
  ZB_ASSERT(buf > 0U && buf < ZB_N_BUF_IDS);
  ZB_ASSERT(ZB_NCP_CTX_CALLS()[buf].call_id == 0U);
  ZB_NCP_CTX_CALLS()[buf].tsn = tsn;
  ZB_NCP_CTX_CALLS()[buf].call_id = call_id;
}


static void free_tsn(zb_uint8_t buf)
{
  ZB_NCP_CTX_CALLS()[buf].call_id = 0U;
}


zb_bool_t ncp_catch_aps_data_conf(zb_uint8_t param)
{
  zb_bool_t ret = ZB_FALSE;

  if ((ZB_NCP_CTX_CALLS()[param].call_id == NCP_HL_APSDE_DATA_REQ) ||
      (ZB_NCP_CTX_CALLS()[param].call_id == INTERNAL_APSDE_DATA_REQ_BY_BIND_TBL_ID))
  {
    ncp_hl_response_header_t *rh;
    zb_apsde_data_confirm_t aps_data_conf = *ZB_BUF_GET_PARAM(param, zb_apsde_data_confirm_t);
    zb_ret_t status = aps_data_conf.status;
    zb_size_t params_len = ZB_OFFSETOF(zb_apsde_data_confirm_t, FIRST_INTERNAL_APSDE_CONF_FIELD);
    zb_uint16_t txlen;

    /* 12/14/2018 EE CR:MAJOR Fill tx time in APS. Translate tx_time to ms */
    ZB_HTOLE32_ONPLACE(aps_data_conf.tx_time);

    if(ZB_NCP_CTX_CALLS()[param].call_id == INTERNAL_APSDE_DATA_REQ_BY_BIND_TBL_ID)
    {
      aps_data_conf.addr_mode = ZB_APS_ADDR_MODE_BIND_TBL_ID;
    }

    if (aps_data_conf.addr_mode != ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ZB_HTOLE16_ONPLACE(aps_data_conf.dst_addr.addr_short);
    }
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSDE_DATA_REQ, ZB_NCP_CTX_CALLS()[param].tsn, status, params_len);
    ZB_MEMCPY(&rh[1], &aps_data_conf, params_len);

    TRACE_MSG(TRACE_TRANSPORT3, "ncp_catch_aps_data_conf param %hd status %d %d %d",
              (FMT__H_D_D_D, param, status, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));
    TRACE_MSG(TRACE_INFO1, "ncp_catch_aps_data_conf param %hd status %d %d %d",
              (FMT__H_D_D_D, param, status, ERROR_GET_CATEGORY(status), ERROR_GET_CODE(status)));

    ncp_send_packet(rh, txlen);
    free_tsn(param);
    zb_buf_free(param);
    ret = ZB_TRUE;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_catch_aps_data_conf: not my data conf by param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_INFO1, "ncp_catch_aps_data_conf: not my data conf by param %hd", (FMT__H, param));
  }
  return ret;
}

static void handle_bind_confirm(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);
#ifdef SNCP_MODE
    zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
    ZB_ASSERT_COMPILE_TIME(sizeof(zb_uint8_t) == sizeof(apsreq->id));
#endif /* SNCP_MODE */

  if (status == RET_OK)
  {
    ncp_hl_response_header_t *rh;
    zb_uint16_t txlen;

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_BIND,
                                 stop_single_command(NCP_HL_APSME_BIND),
                                 status, sizeof(zb_uint8_t));

#ifdef SNCP_MODE
    ZB_MEMCPY(&rh[1], &apsreq->id, sizeof(zb_uint8_t));
#else
    *((zb_uint8_t*)&rh[1]) = 0;
#endif /* SNCP_MODE */

    ncp_send_packet(rh, txlen);
  }
  else
  {
    ncp_hl_send_no_arg_resp(NCP_HL_APSME_BIND, status);
  }

  zb_buf_free(param);
}

static void handle_unbind_confirm(zb_uint8_t param)
{
  zb_ret_t status = zb_buf_get_status(param);
#ifdef SNCP_MODE
    zb_apsme_binding_req_t *apsreq = ZB_BUF_GET_PARAM(param, zb_apsme_binding_req_t);
    ZB_ASSERT_COMPILE_TIME(sizeof(zb_uint8_t) == sizeof(apsreq->id));
#else
    ZVUNUSED(param);
#endif /* SNCP_MODE */

  if (status == RET_OK)
  {
    ncp_hl_response_header_t *rh;
    zb_uint16_t txlen;

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APSME_UNBIND,
                               stop_single_command(NCP_HL_APSME_UNBIND),
                               status, sizeof(zb_uint8_t));

#ifdef SNCP_MODE
    ZB_MEMCPY(&rh[1], &apsreq->id, sizeof(zb_uint8_t));
#else
    *((zb_uint8_t*)&rh[1]) = 0;
#endif /* SNCP_MODE */

    ncp_send_packet(rh, txlen);
  }
  else
  {
    ncp_hl_send_no_arg_resp(NCP_HL_APSME_UNBIND, status);
  }

  zb_buf_free(param);
}


static zb_uint16_t apsme_binding_req(ncp_hl_request_header_t *hdr, zb_uint16_t len, zb_uint16_t type)
{
  zb_apsme_binding_req_t *req;
  zb_bufid_t buf = 0;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_ret_t ret;
  zb_uint16_t txlen;

  ret = start_single_command(hdr);
  if (ret == RET_OK &&
      ((NCP_HL_APSME_BIND == type &&
        len < (sizeof(*hdr) +
           sizeof(zb_ieee_addr_t) +
           sizeof(zb_uint8_t) +
           sizeof(zb_uint16_t) +
           sizeof(zb_uint8_t) +
           sizeof(zb_ieee_addr_t) +
           sizeof(zb_uint8_t)
#ifdef SNCP_MODE
           + sizeof(zb_uint8_t)
#endif /* SNCP_MODE */
           )
      ) ||
      (NCP_HL_APSME_UNBIND == type &&
            len < (sizeof(*hdr) +
           sizeof(zb_ieee_addr_t) +
           sizeof(zb_uint8_t) +
           sizeof(zb_uint16_t) +
           sizeof(zb_uint8_t) +
           sizeof(zb_ieee_addr_t) +
           sizeof(zb_uint8_t))
           )))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (RET_OK == ret)
  {
    req = ZB_BUF_GET_PARAM(buf, zb_apsme_binding_req_t);
    ncp_hl_body_get_u64addr(&body, req->src_addr);
    ncp_hl_body_get_u8(&body, &req->src_endpoint);
    ncp_hl_body_get_u16(&body, &req->clusterid);
    ncp_hl_body_get_u8(&body, &req->addr_mode);

    if (ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT == req->addr_mode)
    {
      /* req->dst_addr is packed union inside unpacked structure. It is safe to cast its
       * members discarding 'packed' attribute. */
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00002,5} */
      ncp_hl_body_get_u16(&body, (zb_uint16_t *)&req->dst_addr.addr_short);

      /* this is a union of size(zb_addr_u) == 8 bytes (bigger field is zb_ieee_addr_t)
          we read 2 bytes so we need to pad 6 bytes */
      body.pos += (sizeof(req->dst_addr.addr_long) -  sizeof(req->dst_addr.addr_short));
    }
    else if (ZB_APS_ADDR_MODE_64_ENDP_PRESENT == req->addr_mode)
    {
      ncp_hl_body_get_u64addr(&body, req->dst_addr.addr_long);
    }
    else
    {
      ret = RET_INVALID_PARAMETER;
    }
    ncp_hl_body_get_u8(&body, &req->dst_endpoint);

#ifdef SNCP_MODE
    if (NCP_HL_APSME_BIND == type)
    {
      ncp_hl_body_get_u8(&body, &req->id);
      /* this function only receive local bind_req */
      req->remote_bind = ZB_FALSE;
    }
#endif /* SNCP_MODE */
  }

  if (RET_OK == ret)
  {
    switch (type)
    {
    case NCP_HL_APSME_BIND:
      req->confirm_cb = handle_bind_confirm;
      zb_apsme_bind_request(buf);
      break;
    case NCP_HL_APSME_UNBIND:
      req->confirm_cb = handle_unbind_confirm;
      zb_apsme_unbind_request(buf);
      break;
    default:
      /* type can either be NCP_HL_APSME_UNBIND or NCP_HL_APSME_BIND */
      ZB_ASSERT(0);
      break;
    }
  }

  if (ret != RET_OK)
  {
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
    txlen = ncp_hl_fill_resp_hdr(NULL, type, hdr->tsn, ret, 0);
  }
  else
  {
    txlen = NCP_RET_LATER;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<apsme_binding ret %hu", (FMT__D, ret));

  return txlen;
}

/*
  ID = NCP_HL_APSME_BIND:
  Request:
    5b: Common Request Header
    8b: Src IEEE Address
    1b: Src Endpoint
    2b: Cluster ID
    1b: Dst Address Mode
    8b: Dst IEEE/NWK Address
    1b: Dst Endpoint
*/
static zb_uint16_t apsme_bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_bind_req len %hu", (FMT__H, len));

  txlen = apsme_binding_req(hdr, len, NCP_HL_APSME_BIND);

  return txlen;
}

/*
  ID = NCP_HL_APSME_UNBIND:
  Request:
    5b: Common Request Header
    8b: Src IEEE Address
    1b: Src Endpoint
    2b: Cluster ID
    1b: Dst Address Mode
    8b: Dst IEEE/NWK Address
    1b: Dst Endpoint
*/
static zb_uint16_t apsme_unbind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_unbind_req len %hu", (FMT__H, len));

  txlen = apsme_binding_req(hdr, len, NCP_HL_APSME_UNBIND);

  return txlen;
}


/*
  ID = NCP_HL_APSME_UNBIND_ALL:
  Request:
    5b: Common Request Header
  Response:
    7b: Common Response Header
*/
static zb_uint16_t apsme_unbind_all_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected_len = 0;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> apsme_unbind_all_req", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    zb_apsme_unbind_all(0);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< apsme_unbind_all_req, ret %hu", (FMT__D, ret));

  return txlen;
}


static zb_uint16_t apsme_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len, zb_uint16_t type)
{
  zb_uint16_t txlen;
  zb_ret_t ret = RET_OK;
  zb_uint16_t group = 0;
  zb_uint8_t ep = 0;
  zb_uint8_t *p = (zb_uint8_t *)(&hdr[1]);

  if (len < sizeof(*hdr) + sizeof(group) + sizeof(ep))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ZB_LETOH16(&group, p);
    p += 2;
    ep = *p;

    if (group >= 0xFFF7U)
    {
      ret = RET_INVALID_PARAMETER_1;
    }
    else if ((ep > ZB_MAX_ENDPOINT_NUMBER) || (ep < ZB_MIN_ENDPOINT_NUMBER))
    {
      ret = RET_INVALID_PARAMETER_2;
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
  }
  if (RET_OK == ret)
  {
    switch (type)
    {
    case NCP_HL_APSME_ADD_GROUP:
      ret = zb_apsme_add_group_internal(group, ep);
      break;
    case NCP_HL_APSME_RM_GROUP:
      ret = zb_apsme_remove_group_internal(group, ep);
      break;
    default:
      TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_group_req unknown type %hu", (FMT__H, type));
      ret = RET_INVALID_PARAMETER;
      break;
    }
#ifdef ZB_USE_NVRAM
    if (ret == (zb_ret_t)ZB_APS_STATUS_SUCCESS)
    {
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    }
#endif
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, type, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<<apsme_group_req ret %hu", (FMT__D, ret));

  return txlen;
}

/*
  ID = NCP_HL_APSME_ADD_GROUP:
  Request:
    5b: Common Request Header
    2b: Group
    1b: Endpoint
*/
static zb_uint16_t apsme_add_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_add_group_req len %hu", (FMT__H, len));

  txlen = apsme_group_req(hdr, len, NCP_HL_APSME_ADD_GROUP);

  return txlen;
}

/*
  ID = NCP_HL_APSME_RM_GROUP:
  Request:
    5b: Common Request Header
    2b: Group
    1b: Endpoint
*/
static zb_uint16_t apsme_rm_group_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_rm_group_req len %hu", (FMT__H, len));

  txlen = apsme_group_req(hdr, len, NCP_HL_APSME_RM_GROUP);

  return txlen;
}

/*
  ID = NCP_HL_APSME_RM_ALL_GROUPS:
  Request:
    5b: Common Request Header
    1b: Endpoint
*/
static zb_uint16_t apsme_rm_all_groups_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;
  zb_ret_t ret = RET_OK;
  zb_uint8_t ep = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_rm_all_groups_req len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(ep))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ep = *(zb_uint8_t*)(&hdr[1]);

    if ((ep > ZB_MAX_ENDPOINT_NUMBER) || (ep < ZB_MIN_ENDPOINT_NUMBER))
    {
      ret = RET_INVALID_PARAMETER_1;
    }
  }
  if (RET_OK == ret)
  {
    zb_int_t i, j;
    zb_bool_t found = ZB_FALSE;

    for (i = (zb_int_t)ZG->aps.group.n_groups - 1; i >= 0; --i)
    {
      for (j = 0 ; j < (zb_int_t)ZG->aps.group.groups[i].n_endpoints ; ++j)
      {
        if (ZG->aps.group.groups[i].endpoints[j] == ep)
        {
          TRACE_MSG(TRACE_ERROR, "remove entry from table [%hd][%hd]", (FMT__H_H, i, j));
          found = ZB_TRUE;
          /* Remove this entry from table, shift rest of the elements to the left */
          if (i < (zb_int_t)ZG->aps.group.n_groups - 1)
          {
            zb_size_t n_groups_size = ((zb_size_t)ZG->aps.group.n_groups - 1U - (zb_size_t)i);
            ZB_MEMMOVE( &(ZG->aps.group.groups[i]),
                        &(ZG->aps.group.groups[i+1]),
                        n_groups_size*sizeof(zb_aps_group_table_ent_t));
          }
          --ZG->aps.group.n_groups;
          break;
        }
      }
    }

    if (!found)
    {
      TRACE_MSG(TRACE_ERROR, "no such entries in the group table", (FMT__0));
      ret = ERROR_CODE(ERROR_CATEGORY_APS, ZB_APS_STATUS_INVALID_GROUP);
    }
#ifdef ZB_USE_NVRAM
    else
    {
      /* If we fail, trace is given and assertion is triggered */
      (void)zb_nvram_write_dataset(ZB_NVRAM_APS_GROUPS_DATA);
    }
#endif
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_APSME_RM_ALL_GROUPS, hdr->tsn, ret, 0);

  return txlen;
}

static void handle_check_binding_response(zb_bufid_t buf)
{
  zb_aps_check_binding_resp_t *resp;
  ncp_hl_response_header_t* rh = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, "handle_check_binding_response, buf %d", (FMT__D, buf));

  resp = ZB_BUF_GET_PARAM(buf, zb_aps_check_binding_resp_t);

  TRACE_MSG(TRACE_TRANSPORT3, "src ep %d, cluster id 0x%x, exists %d",
            (FMT__D_D_D, resp->src_endpoint, resp->cluster_id, resp->exists));

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APS_CHECK_BINDING,
                               stop_single_command(NCP_HL_APS_CHECK_BINDING),
                               RET_OK, 1);

  p = (zb_uint8_t*)&rh[1];
  *p = ZB_B2U(resp->exists);

  ncp_send_packet(rh, txlen);
}

/*
  ID = NCP_HL_APS_CHECK_BINDING:
  Request:
    5b: Common Request Header
    1b: Endpoint
    2b: Cluster ID
*/
static zb_uint16_t aps_check_binding_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_aps_check_binding_req_t *req;
  zb_bufid_t buf = 0;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint16_t txlen;
  zb_ret_t ret;

  TRACE_MSG(TRACE_TRANSPORT3, ">>aps_check_binding_req len %hu", (FMT__H, len));

  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    if (len < sizeof(*hdr) + sizeof(req->src_endpoint) + sizeof(req->cluster_id))
    {
      ret = RET_INVALID_FORMAT;
    }
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (RET_OK == ret)
  {
    req = ZB_BUF_GET_PARAM(buf, zb_aps_check_binding_req_t);

    ncp_hl_body_get_u8(&body, &req->src_endpoint);
    ncp_hl_body_get_u16(&body, &req->cluster_id);
    req->response_cb = handle_check_binding_response;
    zb_aps_check_binding_request(buf);
  }

  if (ret != RET_OK)
  {
    buf_free_if_valid(buf);
    if (ret != RET_BUSY)
    {
      stop_single_command_by_tsn(hdr->tsn);
    }
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_APS_CHECK_BINDING, hdr->tsn, ret, 0);
  }
  else
  {
    txlen = NCP_RET_LATER;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<aps_check_binding_req, txlen %d", (FMT__D, txlen));

  return txlen;
}


/*
  ID = NCP_HL_APS_GET_GROUP_TABLE:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t aps_get_group_table_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;
  zb_uint16_t txlen;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">>apsme_get_group_table_req len %hu", (FMT__H, len));

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    zb_size_t table_size = sizeof(ZG->aps.group.groups[0]) * ZG->aps.group.n_groups;
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_APS_GET_GROUP_TABLE, hdr->tsn, ret,
                                 sizeof(ZG->aps.group.n_groups) + table_size);

    p = (zb_uint8_t*)&rh[1];

    *p = ZG->aps.group.n_groups;
    p++;

    ZB_MEMCPY(p, ZG->aps.group.groups, sizeof(table_size));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_APS_GET_GROUP_TABLE, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<apsme_get_group_table_req txlen %d", (FMT__D, txlen));

  return txlen;
}


static zb_uint16_t handle_ncp_req_nwkmgmt(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  switch (hdr->hdr.call_id)
  {
    case NCP_HL_NWK_FORMATION:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_FORMATION len %d", (FMT__D, len));
      txlen = nwk_formation(hdr, len);
      break;
    case NCP_HL_NWK_DISCOVERY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_DISCOVERY len %d", (FMT__D, len));
      txlen = nwk_discovery(hdr, len);
      break;
    case NCP_HL_NWK_NLME_JOIN:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_NLME_JOIN len %d", (FMT__D, len));
      txlen = nwk_join(hdr, len);
      break;
    case NCP_HL_NWK_PERMIT_JOINING:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_PERMIT_JOIN len %d", (FMT__D, len));
      txlen = nwk_permit_joining(hdr, len);
      break;
    case NCP_HL_NWK_GET_IEEE_BY_SHORT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_GET_IEEE_BY_SHORT len %d", (FMT__D, len));
      txlen = get_address_ieee_by_short(hdr, len);
      break;
    case NCP_HL_NWK_GET_SHORT_BY_IEEE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_GET_SHORT_BY_IEEE len %d", (FMT__D, len));
      txlen = get_address_short_by_ieee(hdr, len);
      break;
    case NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE len %d", (FMT__D, len));
      txlen = get_neighbor_by_ieee(hdr, len);
      break;
    case NCP_HL_PIM_SET_FAST_POLL_INTERVAL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_SET_FAST_POLL_INTERVAL len %d", (FMT__D, len));
      txlen = set_fast_poll_interval(hdr, len);
      break;
    case NCP_HL_PIM_SET_LONG_POLL_INTERVAL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_SET_LONG_POLL_INTERVAL len %d", (FMT__D, len));
      txlen = set_long_poll_interval(hdr, len);
      break;
    case NCP_HL_PIM_START_FAST_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_START_FAST_POLL len %d", (FMT__D, len));
      txlen = start_fast_poll(hdr, len);
      break;
    case NCP_HL_PIM_START_LONG_POLL:
      ZB_ASSERT(ZB_FALSE);
      break;
    case NCP_HL_PIM_START_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_START_POLL len %d", (FMT__D, len));
      txlen = start_poll(hdr, len);
      break;
    case NCP_HL_PIM_STOP_FAST_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_STOP_FAST_POLL len %d", (FMT__D, len));
      txlen = stop_fast_poll(hdr, len);
      break;
    case NCP_HL_PIM_STOP_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_STOP_POLL len %d", (FMT__D, len));
      txlen = stop_poll(hdr, len);
      break;
    case NCP_HL_PIM_ENABLE_TURBO_POLL:
    case NCP_HL_PIM_SET_ADAPTIVE_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_ENABLE_TURBO_POLL len %d", (FMT__D, len));
      txlen = enable_turbo_poll(hdr, len);
      break;
    case NCP_HL_PIM_DISABLE_TURBO_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_DISABLE_TURBO_POLL len %d", (FMT__D, len));
      txlen = disable_turbo_poll(hdr, len);
      break;
    case NCP_HL_PIM_START_TURBO_POLL_PACKETS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_START_TURBO_POLL_PACKETS len %d", (FMT__D, len));
      txlen = start_turbo_poll_packets(hdr, len);
      break;
    case NCP_HL_PIM_START_TURBO_POLL_CONTINUOUS:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_START_TURBO_POLL_CONTINUOUS len %d", (FMT__D, len));
      txlen = start_turbo_poll_continuous(hdr, len);
      break;
    case NCP_HL_PIM_TURBO_POLL_CONTINUOUS_LEAVE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_TURBO_POLL_CONTINUOUS_LEAVE len %d", (FMT__D, len));
      txlen = turbo_poll_continuous_leave(hdr, len);
      break;
    case NCP_HL_PIM_TURBO_POLL_PACKETS_LEAVE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_TURBO_POLL_PACKETS_LEAVE len %d", (FMT__D, len));
      txlen = turbo_poll_packets_leave(hdr, len);
      break;
    case NCP_HL_PIM_PERMIT_TURBO_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_PERMIT_TURBO_POLL len %d", (FMT__D, len));
      txlen = permit_turbo_poll(hdr, len);
      break;
    case NCP_HL_PIM_SET_FAST_POLL_TIMEOUT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_SET_FAST_POLL_TIMEOUT len %d", (FMT__D, len));
      txlen = set_fast_poll_timeout(hdr, len);
      break;
    case NCP_HL_PIM_GET_LONG_POLL_INTERVAL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_GET_LONG_POLL_INTERVAL len %d", (FMT__D, len));
      txlen = get_long_poll_interval(hdr, len);
      break;
    case NCP_HL_PIM_GET_IN_FAST_POLL_FLAG:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_GET_IN_FAST_POLL_FLAG len %d", (FMT__D, len));
      txlen = get_in_fast_poll_flag(hdr, len);
      break;
    case NCP_HL_SET_KEEPALIVE_MODE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SET_KEEPALIVE_MODE len %d", (FMT__D, len));
      txlen = set_keepalive_mode(hdr, len);
      break;
    case NCP_HL_START_CONCENTRATOR_MODE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_START_CONCENTRATOR_MODE len %d", (FMT__D, len));
      txlen = start_concentrator_mode(hdr, len);
      break;
    case NCP_HL_STOP_CONCENTRATOR_MODE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_START_CONCENTRATOR_MODE len %d", (FMT__D, len));
      txlen = stop_concentrator_mode(hdr, len);
      break;
    case NCP_HL_NWK_GET_FIRST_NBT_ENTRY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_GET_FIRST_NBT_ENTRY len %d", (FMT__D, len));
      txlen = get_first_nbt_entry(hdr, len);
      break;
    case NCP_HL_NWK_GET_NEXT_NBT_ENTRY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_GET_NEXT_NBT_ENTRY len %d", (FMT__D, len));
      txlen = get_next_nbt_entry(hdr, len);
      break;
    case NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE len %d", (FMT__D, len));
      txlen = ncp_hl_nwk_pan_id_conflict_resolve(hdr, len);
      break;
    case NCP_HL_NWK_ENABLE_PAN_ID_CONFLICT_RESOLUTION:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_ENABLE_PAN_ID_CONFLICT_RESOLUTION len %d", (FMT__D, len));
      txlen = enable_pan_id_conflict_resolution(hdr, len);
      break;
    case NCP_HL_NWK_ENABLE_AUTO_PAN_ID_CONFLICT_RESOLUTION:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_ENABLE_AUTO_PAN_ID_CONFLICT_RESOLUTION len %d", (FMT__D, len));
      txlen = enable_auto_pan_id_conflict_resolution(hdr, len);
      break;
    case NCP_HL_NWK_START_WITHOUT_FORMATION:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_START_WITHOUT_FORMATION len %d", (FMT__D, len));
      txlen = ncp_hl_nwk_start_without_formation(hdr, len);
      break;
    case NCP_HL_NWK_NLME_ROUTER_START:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_NWK_NLME_ROUTER_START len %d", (FMT__D, len));
      txlen = nwk_nlme_router_start_req(hdr, len);
      break;
#ifdef SNCP_MODE
    case NCP_HL_PIM_SINGLE_POLL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_SINGLE_POLL len %d", (FMT__D, len));
      txlen = single_poll(hdr, len);
      break;
#endif
    case NCP_HL_PIM_TURBO_POLL_CANCEL_PACKET:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_PIM_TURBO_POLL_CANCEL_PACKET len %d", (FMT__D, len));
      txlen = turbo_poll_cancel_packet(hdr, len);
      break;
    default:
      break;
  }
  return txlen;
}


static zb_ret_t ncp_alloc_buf(zb_bufid_t *buf)
{
  zb_ret_t ret = RET_OK;
  if (NCP_CTX.work_buf != 0U)
  {
    *buf = NCP_CTX.work_buf;
    TRACE_MSG(TRACE_TRANSPORT3, "ncp_alloc_buf: got from work_buf %hd", (FMT__H, NCP_CTX.work_buf));
    NCP_CTX.work_buf = 0;
  }
  else
  {
    /* second chance: sync alloc */
    zb_bufid_t b = zb_buf_get_out();
    if (b != 0U)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "ncp_alloc_buf: sync alloc %hu", (FMT__H, b));
      *buf = b;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "ncp_alloc_buf: no memory!", (FMT__0));
      ret = RET_NO_MEMORY;
    }
  }
  return ret;
}

static zb_ret_t ncp_alloc_buf_size(zb_bufid_t *buf, zb_size_t size)
{
  zb_ret_t ret = RET_OK;

  if (size > ZB_IO_BUF_SIZE)
  {
    *buf = zb_buf_get(ZB_FALSE, size);
    if (*buf == 0U)
    {
      ret = RET_NO_MEMORY;
    }
  }
  else
  {
    ret = ncp_alloc_buf(buf);
  }

  return ret;
}

static zb_ret_t get_channels_list(
  zb_uint16_t len,
  zb_uint8_t *data_start_ptr,
  zb_channel_list_t channels_list,
  zb_uint8_t *n_ent,
  zb_uint8_t *offset)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p = data_start_ptr;
  zb_size_t len_rest = len;

  *offset = 0;
  /*
    Command format:

    channel list:
    1b n entries
    [n] 1b page 4b mask
   */
  if (len_rest <= 0U)
  {
    TRACE_MSG(TRACE_ERROR, "too small packet len %d", (FMT__D, len));
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    *n_ent = *p;
    p++;
    len_rest--;
  }
  if (*n_ent < 1U)
  {
    ret = RET_INVALID_PARAMETER;
  }
  if (ret == RET_OK)
  {
    zb_uint_t i;
    zb_channel_list_init(channels_list);
    for (i = 0 ; i < *n_ent && ret == RET_OK ; ++i)
    {
      zb_uint8_t page;
      zb_uint32_t mask;
      if (len_rest < sizeof(page) + sizeof(mask))
      {
        TRACE_MSG(TRACE_ERROR, "too small packet len %d", (FMT__D, len));
        ret = RET_INVALID_FORMAT;
      }
      else
      {
        len_rest -= (sizeof(page) + sizeof(mask));
        page = *p;
        p++;
        ZB_MEMCPY(&mask, p, sizeof(mask));
        ZB_LETOH32_ONPLACE(mask);
        p += sizeof(mask);
        ret = zb_channel_list_add(channels_list, page, mask);
        TRACE_MSG(TRACE_TRANSPORT3, "page[%d] %hd mask 0x%x - status %d", (FMT__D_H_D_D, i, page, mask, ret));
#if defined SNCP_MODE && defined ZB_SUBGHZ_BAND_ENABLED
        if (ZB_IS_DEVICE_ZR() && ZB_LOGICAL_PAGE_IS_SUB_GHZ(page))
        {
          TRACE_MSG(TRACE_ERROR, "Sub-GHz is not supported for ZR role!", (FMT__0));
          ret = RET_NOT_IMPLEMENTED;
          break;
        }
        else
        {
          /* MISRA rule 15.7 requires empty 'else' branch. */
        }
#endif /* SNCP_MODE && ZB_SUBGHZ_BAND_ENABLED */
      }
    }
  }
  /* TODO: implement functionality in py-ncp to work with channels lists.
   * Now we have the hardcoded entries number in ncp_host_hl.py -> ncp_hl_channel_list_t
   * Maybe it's better to send only usefull entries from ncp-py.
   */
  p += ((sizeof(zb_uint8_t) + sizeof(zb_uint32_t)) * (ZB_CHANNEL_PAGES_NUM - (zb_uint32_t)(*n_ent)));
  *offset = p - data_start_ptr;
  TRACE_MSG(TRACE_TRANSPORT3, "get_channels_list ret %d offset %d", (FMT__D_D, ret, *offset));
  return ret;
}


static zb_uint16_t nwk_formation(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{

#if defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON)

  zb_ret_t ret = RET_OK;
  zb_uint8_t len_rest = len - sizeof(*hdr);
  zb_nlme_network_formation_request_t *request;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t n_scan_list_ent;
  zb_uint8_t buf = 0;
  /*
    Command format:

    channel list:
    1b n entries
    [n] 1b page 4b mask

    1b scan duration
    1b DistributedNetwork
    2b DistributedNetworkAddress
    8b ExtendedPanID
   */
  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    zb_uint8_t shift;
    request = ZB_BUF_GET_PARAM(buf, zb_nlme_network_formation_request_t);
    ret = get_channels_list(len_rest, p, request->scan_channels_list, &n_scan_list_ent, &shift);
    p += shift;
    len_rest -= shift;
  }
  if (ret == RET_OK
      && len_rest < sizeof(request->scan_duration) + sizeof(request->distributed_network) + sizeof(request->distributed_network_address))
  {
    TRACE_MSG(TRACE_ERROR, "formation: too small packet len %d", (FMT__D, len));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    request->scan_duration = *p;
    p++;
    request->distributed_network = *p;
    p++;
    ZB_MEMCPY(&request->distributed_network_address, p, sizeof(request->distributed_network_address));
    ZB_LETOH16_ONPLACE(request->distributed_network_address);

    p += sizeof(request->distributed_network_address);

    ZB_MEMCPY(request->extpanid, p, sizeof(request->extpanid));

    TRACE_MSG(TRACE_TRANSPORT3, "formation: scan_duration %d distributed %hd distributed_network_address 0x%x",
              (FMT__D_H_D, request->scan_duration, request->distributed_network, request->distributed_network_address));
    TRACE_MSG(TRACE_TRANSPORT3, "formation: extpanid " TRACE_FORMAT_64,
      (FMT__A, TRACE_ARG_64(request->extpanid)));

    (void)ZB_SCHEDULE_APP_CALLBACK(zb_nlme_network_formation_request, buf);
    ZB_ASSERT(NCP_CTX.start_command == 0);
    NCP_CTX.start_command = NCP_HL_NWK_FORMATION;
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
    /* create a resp */
    return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON) */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_FORMATION, hdr->tsn, RET_NOT_IMPLEMENTED, 0);

#endif /* defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON) */
}


static zb_uint16_t nwk_discovery(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t len_rest = len - (zb_uint16_t)sizeof(*hdr);
  zb_nlme_network_discovery_request_t *request;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t n_scan_list_ent;
  zb_uint8_t buf = 0;
  /*
    Command format:

    channel list:
    1b n entries
    [n] 1b page 4b mask

    1b scan duration
   */
  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    zb_uint8_t shift;

    request = ZB_BUF_GET_PARAM(buf, zb_nlme_network_discovery_request_t);
    ret = get_channels_list(len_rest, p, request->scan_channels_list, &n_scan_list_ent, &shift);
    p += shift;
    len_rest -= shift;
  }
  if (ret == RET_OK
      && len_rest < sizeof(request->scan_duration))
  {
    TRACE_MSG(TRACE_ERROR, "formation: too small packet len %d", (FMT__D, len));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    request->scan_duration = *p;
    TRACE_MSG(TRACE_TRANSPORT3, "nwk_discovery: scan_duration %hd",
              (FMT__H, request->scan_duration));
    (void)ZB_SCHEDULE_APP_CALLBACK(zb_nlme_network_discovery_request, buf);
    /* After active scan complete NWK calls zb_nlme_network_discovery_confirm */
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
    /* create a resp */
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_DISCOVERY, hdr->tsn, ret, 0);
  }
}


zb_bool_t ncp_catch_nwk_disc_conf(zb_uint8_t param)
{
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p;
  zb_uint_t i;
  zb_nlme_network_discovery_confirm_t *cnf = (zb_nlme_network_discovery_confirm_t *)zb_buf_begin(param);
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,6} */
  zb_nlme_network_descriptor_t *dsc = (zb_nlme_network_descriptor_t *)(&cnf[1]);

#define NCP_NWK_DSC_ENTRY_SIZE (8U/*extpanid*/ + 2U/*panid*/ + 1U/*updid*/ + 1U/*page*/ + 1U/*chan*/ + 1U/*flags*/ + 1U/*LQI*/ + 1U/*RSSI*/)
  /* 4 bytes entry size internally, 14 bytes externally.
     If max # of nets == 16, body size if 227. Fits into 255 - ok.
     But not extandable!
   */

  /* In case of any error send packet with status and network count only */
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_DISCOVERY,
                               stop_single_command(NCP_HL_NWK_DISCOVERY),
                               cnf->status, 1U + ((zb_uint_t)cnf->network_count) * NCP_NWK_DSC_ENTRY_SIZE);
  p = (zb_uint8_t*)(&rh[1]);
  *p = cnf->network_count;
  p++;
  for (i = 0 ; i < cnf->network_count ; ++i)
  {
    zb_uint16_t panid;
    zb_ext_neighbor_tbl_ent_t *nbt;
    zb_uint8_t lqi;
    zb_int8_t rssi;
    zb_uint8_t capa_info;

    /* exptanid */
    zb_address_get_pan_id(dsc->panid_ref, p);
    p += sizeof(zb_ext_pan_id_t);
    zb_address_get_short_pan_id(dsc->panid_ref, &panid);
    ZB_HTOLE16(p, &panid);
    p = &p[2];
    *p = dsc->nwk_update_id;
    p++;
    *p = dsc->channel_page;
    p++;
    *p = dsc->logical_channel;
    p++;

    capa_info = (dsc->permit_joining
                 | (dsc->router_capacity << 1)
                 | (dsc->end_device_capacity << 2)
                 | (dsc->stack_profile << 4));
    *p = capa_info;
    p++;

    nbt = nwk_choose_parent(dsc->panid_ref, capa_info);
    if (nbt != NULL)
    {
      lqi = nbt->lqi;
      rssi = nbt->rssi;
    }
    else
    {
      lqi = ZB_MAC_LQI_UNDEFINED;
      rssi = (zb_int8_t)ZB_MAC_RSSI_UNDEFINED;
    }

    *p = lqi;
    p++;
    *((zb_int8_t*)p) = rssi;
  }
  ncp_send_packet(rh, txlen);

  zb_buf_free(param);

  return ZB_TRUE;
}


static zb_uint16_t nwk_join(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t len_rest = len - (zb_uint16_t)sizeof(*hdr);
  zb_nlme_join_request_t *request = NULL;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t n_scan_list_ent;
  zb_uint8_t buf = 0;

  /*
    Command format:

    8b extpanid
    1b "rejoin network"
    channel list:
     1b n entries
     [n] 1b page 4b mask
    1b scan duration
    1b CapabilityInformation
    1b SecurityEnable (if rejoin, do secure rejoin)
   */
  ret = start_single_command(hdr);
  if (ret == RET_OK
      /*cstat !MISRAC2012-Rule-14.3_b */
      /** @mdr{00012,9} */
      && ZB_IS_DEVICE_ZC())
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00012,4} */
  {
    ret = RET_INVALID_STATE;
  }
  if (ZB_JOINED())
  {
    ret = RET_INVALID_STATE;
  }
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK
      && len_rest < sizeof(zb_ext_pan_id_t) + 1U /*expanid + rejoin*/)
  {
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    request = ZB_BUF_GET_PARAM(buf, zb_nlme_join_request_t);
    ZB_EXTPANID_COPY(request->extended_pan_id, p);
    p = &p[sizeof(request->extended_pan_id)];
    if (*p > ZB_NLME_REJOIN_METHOD_CHANGE_CHANNEL)
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    zb_uint8_t shift;
    zb_ret_t get_channels_list_ret;

    request->rejoin_network = *p;
    p++;
    len_rest -= ((zb_uint16_t)sizeof(request->extended_pan_id) + 1U);

    get_channels_list_ret = get_channels_list(len_rest, p, request->scan_channels_list, &n_scan_list_ent, &shift);

    if (get_channels_list_ret == RET_OK)
    {
      p += shift;
      len_rest -= shift;
    }
    else
    {
      ret = get_channels_list_ret;
    }
  }
  if (ret == RET_OK
      && len_rest < 3U * 1U /* scan duratiuon + capability + security */)
  {
    TRACE_MSG(TRACE_ERROR, "formation: too small packet len %d", (FMT__D, len));
    ret = RET_INVALID_FORMAT;
  }
  if ((ret == RET_OK) && (request != NULL))
  {
    request->scan_duration = *p;
    p++;
    request->capability_information = *p;
#ifdef ZB_ED_RX_OFF_WHEN_IDLE
    /* enforce rx_on_when_idle bit, wrong value may come from the host */
    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
      /* force set rx_on_when_idle bit */
      request->capability_information |= 1U<<3U;
    }
    else
    {
      /* force clear rx_on_when_idle bit */
      request->capability_information &= ~(1U<<3U);
    }
#endif
    p++;
    request->security_enable = *p;

    if (ZB_IS_DEVICE_ZC_OR_ZR())
    {
      if ((ZB_MAC_CAP_GET_DEVICE_TYPE(request->capability_information) != 0U)
          && (ZB_MAC_CAP_GET_POWER_SOURCE(request->capability_information) != 0U)
          && (ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(request->capability_information) != 0U))
      {
        ret = RET_OK;
      }
      else
      {
        ret = RET_INVALID_PARAMETER_6;
      }
    }
    else
    {
      if (ZB_MAC_CAP_GET_DEVICE_TYPE(request->capability_information) == 0U)
      {
        ret = RET_OK;
      }
      else
      {
        ret = RET_INVALID_PARAMETER_6;
      }
    }
  }

  if (ret == RET_OK)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "nwk_join: scan_duration %hd",
              (FMT__H, request->scan_duration));
    (void)ZB_SCHEDULE_APP_CALLBACK(zb_nlme_join_request, buf);
    ZB_ASSERT(NCP_CTX.start_command == 0UL);
    NCP_CTX.start_command = NCP_HL_NWK_NLME_JOIN;
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
    /* create a resp */
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_NLME_JOIN, hdr->tsn, ret, 0);
  }
}

static void ncp_hl_fill_join_resp_body(void * b)
{
  zb_uint8_t *p = (zb_uint8_t *)b;

  /*
    Packet structure:
    2b short_addr
    8b extpanid
    1b channel page
    1b channel
    1b ench beacon type
    1b MAC interface #

    Note that status is in the HL packet header
   */

  ZB_HTOLE16(p, &ZB_PIBCACHE_NETWORK_ADDRESS());
  p = &p[2];
  ZB_EXTPANID_COPY(p, ZB_NIB_EXT_PAN_ID());
  p = &p[8];
  *p = ZB_PIBCACHE_CURRENT_PAGE();
  p++;
  *p = ZB_PIBCACHE_CURRENT_CHANNEL();
  p++;
  /* in sub-ghz we always use enchanced beacins */
  *p = ZB_B2U(ZB_PIBCACHE_CURRENT_PAGE() != 0U);
  p++;
  /* Interface # is 0 until we implement multi-MAC */
  *p = 0;
}

static void ncp_hl_send_join_ok_resp(zb_uint8_t param)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_send_join_resp", (FMT__0));

  /* Fixme: do we need to analyze anything from param? */
  ZVUNUSED(param);
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_NLME_JOIN,
                               stop_single_command(NCP_HL_NWK_NLME_JOIN),
                               RET_OK, JOIN_RESP_SIZE);
  ncp_hl_fill_join_resp_body(&rh[1]);

  ncp_send_packet(rh, txlen);
}


/* Packet structure:
   2b - short addr,
   2b - PAN ID,
   1b - current page,
   1b - current channel
 */
static void ncp_hl_send_formation_response(zb_ret_t status)
{
  ncp_hl_response_header_t* rh = NULL;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_send_formation_response, status %d", (FMT__D, status));

  if (status == RET_OK)
  {
    zb_uint8_t *p;
    zb_uint16_t txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_FORMATION,
                                 stop_single_command(NCP_HL_NWK_FORMATION),
                                 RET_OK, 6);
    p = (zb_uint8_t*)&rh[1];
    ZB_HTOLE16(p, &ZB_PIBCACHE_NETWORK_ADDRESS());
    p += 2;

    ZB_HTOLE16(p, &ZB_PIBCACHE_PAN_ID());
    p += 2;

    *p = zb_get_current_page();
    p++;

    *p = zb_get_current_channel();
    p++;

    ncp_send_packet(rh, txlen);
  }
  else
  {
    ncp_hl_send_no_arg_resp(NCP_HL_NWK_FORMATION, status);
  }
}

static zb_uint16_t nwk_permit_joining(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#if defined ZB_COORDINATOR_ROLE || defined ZB_ROUTER_ROLE

  zb_ret_t ret;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t duration;
  zb_nlme_permit_joining_request_t *request;
  zb_uint8_t buf = 0;
  /*
    Command format:
    1b permit_duration
   */
  if (len < sizeof(*hdr) + sizeof(duration))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = start_single_command(hdr);
  }
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    request = ZB_BUF_GET_PARAM(buf, zb_nlme_permit_joining_request_t);
    request->permit_duration = *p;
    TRACE_MSG(TRACE_TRANSPORT3, "nwk_permit_joining %hd", (FMT__H, request->permit_duration));
    (void)ZB_SCHEDULE_APP_CALLBACK(zb_nlme_permit_joining_request, buf);
    /* zb_nlme_permit_joining_confirm calls zdo_mgmt_permit_joining_resp_cli */
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    hdr->tsn = stop_single_command(NCP_HL_NWK_PERMIT_JOINING);
    /* create a resp */
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_PERMIT_JOINING, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE || defined ZB_ROUTER_ROLE */

  /* create a resp */
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_PERMIT_JOINING, hdr->tsn, RET_NOT_IMPLEMENTED, 0);

#endif /* defined ZB_COORDINATOR_ROLE || defined ZB_ROUTER_ROLE */
}

/*
  ID = NCP_HL_NWK_START_WITHOUT_FORMATION:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t ncp_hl_nwk_start_without_formation(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#if defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON)

  zb_ret_t ret = RET_OK;
  zb_uint8_t param = 0;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_nwk_start_without_formation len %hd", (FMT__D, len));

  ret = start_single_command(hdr);
  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&param);
  }
  if (RET_OK == ret)
  {
    if (!ZB_EXTPANID_IS_ZERO(ZB_NIB_EXT_PAN_ID()) && ZB_PIBCACHE_CURRENT_CHANNEL() != 0)
    {
      TRACE_MSG(TRACE_ERROR, "Pan ID = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_NIB_EXT_PAN_ID())));
      TRACE_MSG(TRACE_ERROR, "Start already created network on %hd channel",
                (FMT__H, ZB_PIBCACHE_CURRENT_CHANNEL()));

      zb_nwk_cont_without_formation(param);
    }
    else
    {
      ret = RET_OPERATION_FAILED;
    }
  }

  if (RET_OK == ret)
  {
    ZB_ASSERT(NCP_CTX.start_command == 0);
    NCP_CTX.start_command = NCP_HL_NWK_START_WITHOUT_FORMATION;
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(param);
    hdr->tsn = stop_single_command(NCP_HL_NWK_START_WITHOUT_FORMATION);
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_START_WITHOUT_FORMATION, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON) */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_START_WITHOUT_FORMATION, hdr->tsn, RET_NOT_IMPLEMENTED, 0);

#endif /* defined ZB_COORDINATOR_ROLE || (defined ZB_ROUTER_ROLE && defined ZB_DISTRIBUTED_SECURITY_ON) */
}


/*
ID = NCP_HL_NWK_NLME_ROUTER_START:
  Request:
    7b: Common request header
    1b: Beacon Order
    1b: Superframe Order
    1b: Battery Life Extension
  Response:
    5b: Common response header
*/
static zb_uint16_t nwk_nlme_router_start_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#ifdef ZB_ROUTER_ROLE
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint8_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_uint8_t);

  zb_nlme_start_router_request_t *param;
  zb_uint8_t beacon_order;
  zb_uint8_t superframe_order;
  zb_uint8_t battery_life_extension;

  zb_uint8_t buf = 0;

  ret = start_single_command(hdr);

  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &beacon_order);
    ncp_hl_body_get_u8(&body, &superframe_order);
    ncp_hl_body_get_u8(&body, &battery_life_extension);

    param = ZB_BUF_GET_PARAM(buf, zb_nlme_start_router_request_t);
    param->beacon_order = beacon_order;
    param->superframe_order = superframe_order;
    param->battery_life_extension = battery_life_extension;

    TRACE_MSG(TRACE_TRANSPORT3, "nwk_nlme_router_start_req, beacon_order %hd, superframe_order %hd, battery_life %hd",
      (FMT__H_H_H, beacon_order, superframe_order, battery_life_extension));

    (void)ZB_SCHEDULE_APP_CALLBACK(zb_nlme_start_router_request, buf);

    ZB_ASSERT(NCP_CTX.start_command == 0U);
    NCP_CTX.start_command = NCP_HL_NWK_NLME_ROUTER_START;

    /* response will be sent from NCP_SIGNAL_NWK_JOIN_DONE or NCP_SIGNAL_NWK_REJOIN_DONE signals handlers */

    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    stop_single_command_by_tsn(hdr->tsn);
    /* create a resp */
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_NLME_ROUTER_START, hdr->tsn, ret, 0);
  }

#else /* defined ZB_ROUTER_ROLE */

  /* create a resp */
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_NLME_ROUTER_START, hdr->tsn, RET_NOT_IMPLEMENTED, 0);

#endif /* defined defined ZB_ROUTER_ROLE */
}


static zb_uint16_t handle_ncp_req_secur(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  switch(hdr->hdr.call_id)
  {
    case NCP_HL_SECUR_SET_LOCAL_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_SET_LOCAL_IC", (FMT__0));
      txlen = secur_set_ic(hdr, len);
      break;
    case NCP_HL_SECUR_ADD_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_ADD_IC", (FMT__0));
      txlen = secur_add_ic(hdr, len);
      break;
    case NCP_HL_SECUR_DEL_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_DEL_IC", (FMT__0));
      txlen = secur_del_ic(hdr, len);
      break;
    case NCP_HL_SECUR_JOIN_USES_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_JOIN_USES_IC", (FMT__0));
      txlen = secur_join_uses_ic(hdr, len);
      break;
    case NCP_HL_SECUR_GET_IC_BY_IEEE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_IC_BY_IEEE", (FMT__0));
      txlen = secur_get_ic_by_ieee(hdr, len);
      break;
    case NCP_HL_SECUR_GET_LOCAL_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_LOCAL_IC", (FMT__0));
      txlen = secur_get_local_ic(hdr, len);
      break;
    case NCP_HL_SECUR_GET_IC_LIST:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_IC_LIST", (FMT__0));
      txlen = secur_get_ic_list(hdr, len);
      break;
    case NCP_HL_SECUR_GET_IC_BY_IDX:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_IC_BY_IDX", (FMT__0));
      txlen = secur_get_ic_by_idx(hdr, len);
      break;
    case NCP_HL_SECUR_REMOVE_ALL_IC:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_REMOVE_ALL_IC", (FMT__0));
      txlen = secur_remove_all_ic(hdr, len);
      break;
    case NCP_HL_SECUR_GET_KEY_IDX:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_KEY_IDX", (FMT__0));
      txlen = secur_get_key_idx(hdr, len);
      break;
    case NCP_HL_SECUR_GET_KEY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_KEY", (FMT__0));
      txlen = secur_get_key(hdr, len);
      break;
    case NCP_HL_SECUR_ERASE_KEY:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_ERASE_KEY", (FMT__0));
      txlen = secur_erase_key(hdr, len);
      break;
    case NCP_HL_SECUR_CLEAR_KEY_TABLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_CLEAR_KEY_TABLE", (FMT__0));
      txlen = secur_clear_key_table(hdr, len);
      break;
    case NCP_HL_SECUR_NWK_INITIATE_KEY_SWITCH_PROCEDURE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_NWK_INITIATE_KEY_SWITCH_PROCEDURE", (FMT__0));
      txlen = secur_initiate_nwk_key_switch_procedure(hdr, len);
      break;

#ifdef ZB_ENABLE_SE_MIN_CONFIG
    case NCP_HL_SECUR_ADD_CERT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_ADD_CERT", (FMT__0));
      txlen = secur_add_cert(hdr, len);
      break;
    case NCP_HL_SECUR_DEL_CERT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_DEL_CERT", (FMT__0));
      txlen = secur_del_cert(hdr, len);
      break;
    case NCP_HL_SECUR_START_KE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_START_KE", (FMT__0));
      txlen = secur_start_cbke(hdr, len);
      break;
    case NCP_HL_SECUR_START_PARTNER_LK:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_START_PARTNER_LK", (FMT__0));
      txlen = secur_start_partner_lk(hdr, len);
      break;
    case NCP_HL_SECUR_PARTNER_LK_ENABLE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_PARTNER_LK_ENABLE", (FMT__0));
      txlen = secur_partner_lk_enable(hdr, len);
      break;
    case NCP_HL_SECUR_GET_CERT:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_SECUR_GET_CERT", (FMT__0));
      txlen = secur_get_cert(hdr, len);
      break;
#if defined ZB_SE_KE_WHITELIST
    case NCP_HL_SECUR_KE_WHITELIST_ADD:
      txlen = secur_add_to_whitelist(hdr, len);
      break;
    case NCP_HL_SECUR_KE_WHITELIST_DEL:
      txlen = secur_del_from_whitelist(hdr, len);
      break;
    case NCP_HL_SECUR_KE_WHITELIST_DEL_ALL:
      txlen = secur_del_all_from_whitelist(hdr, len);
      break;
#endif /* ZB_SE_KE_WHITELIST */
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_secur: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }
  return txlen;
}


#ifdef ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL

static zb_uint16_t handle_ncp_req_intrp(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  switch (hdr->hdr.call_id)
  {
    case NCP_HL_INTRP_DATA_REQ:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_INTRP_DATA_REQ len %d", (FMT__D, len));
      txlen = intrp_data_req(hdr, len);
      break;
    default:
      break;
  }
  return txlen;
}

static void intrp_data_req_cb(zb_bufid_t buf)
{
  ncp_hl_response_header_t *rh;
  zb_uint8_t * p;
  zb_size_t params_len;
  zb_mchan_intrp_data_confirm_t *conf = ZB_BUF_GET_PARAM(buf, zb_mchan_intrp_data_confirm_t);
  zb_ret_t ret = zb_buf_get_status(buf);
  zb_uint16_t txlen;
  zb_uint8_t tsn;

  /* Check if buffer was already processed and has a valid TSN assigned to it. */
  tsn = stop_single_command(NCP_HL_INTRP_DATA_REQ);
  if (tsn == TSN_RESERVED)
  {
    return;
  }

  /*
    Packet format:
    channel_status_page_mask (zb_uint32_t)
    asdu_handle (zb_uint8_t)
  */
  params_len = sizeof(zb_uint8_t) + sizeof(zb_uint32_t);
  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_INTRP_DATA_REQ, tsn, ret, params_len);
  p = (zb_uint8_t*)(&rh[1]);
  ZB_HTOLE32(p, &conf->channel_status_page_mask);
  p += sizeof(zb_uint32_t);
  *p = conf->asdu_handle;
  p++;

  TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req_cb param %hd status %d (%d::%d) asdu_handle: %hd",
            (FMT__H_L_D_D_H, buf, ret, ERROR_GET_CATEGORY(ret), ERROR_GET_CODE(ret), conf->asdu_handle));

  ncp_send_packet(rh, txlen);
  zb_buf_free(buf);
}

#define NCP_INTRP_DATA_REQ_PARAMS_SIZE (sizeof(zb_intrp_data_req_t) + sizeof(zb_uint32_t) + sizeof(zb_uint32_t))

static zb_uint16_t intrp_data_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t param_len;
  zb_uint16_t data_len;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_intrp_data_req_t ncp_req;
  zb_intrp_data_req_t *req;
  zb_uint32_t channel_page_mask;
  zb_uint32_t chan_wait_ms;
  zb_uint8_t buf = 0U;
  zb_uint8_t *datap;

  TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req hdr %p len %d tsn %d", (FMT__P_D_D, hdr, len, hdr->tsn));

  if (len < sizeof(*hdr) + sizeof(param_len) + sizeof(data_len))
  {
    TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req len %d wants at least %d", (FMT__D_D, len, sizeof(*hdr) + sizeof(param_len) + sizeof(data_len)));
    ret = RET_INVALID_FORMAT;
  }

  /* Parse param_len field. */
  if (ret == RET_OK)
  {
    param_len = *p;
    p++;
    if (param_len != NCP_INTRP_DATA_REQ_PARAMS_SIZE)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req param_len %d wants %d", (FMT__D_D, param_len, NCP_INTRP_DATA_REQ_PARAMS_SIZE));
      ret = RET_INVALID_FORMAT;
    }
  }

  /* Parse data_len field. */
  if (ret == RET_OK)
  {
    ZB_LETOH16(&data_len, p);
    p++;
    p++;

    TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req data_len %d", (FMT__D, data_len));
    if (sizeof(*hdr) + sizeof(param_len) + sizeof(data_len) + param_len + data_len != len)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req sizeof(*hdr) %d + sizeof(param_len) %d + sizeof(data_len) %d + param_len %d + data_len %d = %d len %d",
                (FMT__D_D_D_D_D_D_D, sizeof(*hdr), sizeof(param_len), sizeof(data_len), param_len, data_len,
                 sizeof(*hdr) + sizeof(param_len) + sizeof(data_len) + param_len + data_len,
                 len));
      ret = RET_INVALID_FORMAT;
    }
  }

  /* Parse command params. */
  if (ret == RET_OK)
  {
    ZB_BZERO(&ncp_req, sizeof(ncp_req));
    ZB_MEMCPY(&ncp_req, p, sizeof(ncp_req));
    p += sizeof(ncp_req);
    ZB_MEMCPY(&channel_page_mask, p, sizeof(channel_page_mask));
    p += sizeof(channel_page_mask);
    ZB_MEMCPY(&chan_wait_ms, p, sizeof(chan_wait_ms));
    p += sizeof(chan_wait_ms);

    ZB_LETOH16_ONPLACE(ncp_req.dst_pan_id);
    ZB_LETOH16_ONPLACE(ncp_req.profile_id);
    ZB_LETOH16_ONPLACE(ncp_req.cluster_id);
    ZB_LETOH32_ONPLACE(channel_page_mask);
    ZB_LETOH32_ONPLACE(chan_wait_ms);

    if (ncp_req.dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
        || ncp_req.dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
      ZB_LETOH16_ONPLACE(ncp_req.dst_addr.addr_short);
    }

    /* Allocate ZBOSS buffer for a local request. */
    if (data_len > ZB_IO_BUF_SIZE - sizeof(ncp_req))
    {
      buf = zb_buf_get(ZB_FALSE, (zb_uint_t)data_len + ZB_IO_BUF_SIZE);
      if (buf == 0U)
      {
        ret = RET_NO_MEMORY;
      }
    }
    else
    {
      ret = ncp_alloc_buf(&buf);
    }

    TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req: dst_pan_id: 0x%x profile_id %d, cluster_id %d, dst_short: 0x%x, addr_mode %hd",
      (FMT__D_D_D_D_H, ncp_req.dst_pan_id, ncp_req.profile_id, ncp_req.cluster_id, ncp_req.dst_addr.addr_short, ncp_req.dst_addr_mode));
  }

  /* Store TSN to use in the API call callback. */
  if (ret == RET_OK)
  {
    ret = start_single_command(hdr);
  }

  /* Call ZBOSS API. */
  if (ret == RET_OK)
  {
    datap = zb_buf_initial_alloc(buf, data_len);

    /* Copy command data. */
    ZB_MEMCPY(datap, p, data_len);

    /* Construct API call parameters. */
    req = ZB_BUF_GET_PARAM(buf, zb_intrp_data_req_t);

    req->dst_addr_mode = ncp_req.dst_addr_mode;
    req->dst_pan_id = ncp_req.dst_pan_id;
    if (ncp_req.dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
        || ncp_req.dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
    {
      req->dst_addr.addr_short = ncp_req.dst_addr.addr_short;
    }
    else
    {
      ZB_MEMCPY(req->dst_addr.addr_long, ncp_req.dst_addr.addr_long, sizeof(zb_ieee_addr_t));
    }

    req->profile_id = ncp_req.profile_id;
    req->cluster_id = ncp_req.cluster_id;
    req->asdu_handle = ncp_req.asdu_handle;

    /* Store TSN to use in the API call callback. */
    ret = zb_intrp_data_request_with_chan_change(buf, channel_page_mask, chan_wait_ms, intrp_data_req_cb);
  }

  if (ret == RET_OK)
  {
    return NCP_RET_LATER;
  }
  else
  {
    ncp_hl_response_header_t *rh;
    zb_size_t params_len;
    zb_uint16_t txlen;
    zb_uint32_t channel_status_page_mask = 0;

    stop_single_command_by_tsn(hdr->tsn);

    params_len = sizeof(zb_uint8_t) + sizeof(zb_uint32_t);
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_INTRP_DATA_REQ, hdr->tsn, ret, params_len);
    p = (zb_uint8_t*)(&rh[1]);
    ZB_HTOLE32(p, &channel_status_page_mask);
    p += sizeof(zb_uint32_t);
    *p = ncp_req.asdu_handle;
    p++;

    TRACE_MSG(TRACE_TRANSPORT3, "intrp_data_req status %d %d %d",
              (FMT__D_D_D, ret, ERROR_GET_CATEGORY(ret), ERROR_GET_CODE(ret)));

    return txlen;
  }
}

zb_uint8_t ncp_hl_intrp_cmd_ind(zb_uint8_t param, zb_uint8_t current_page, zb_uint8_t current_channel)
{
  zb_uint8_t *p;
  ncp_hl_ind_header_t *ih;
  /* whole structure will be copied to avoid alignment problems */
  zb_intrp_data_ind_t ind = *ZB_BUF_GET_PARAM(param, zb_intrp_data_ind_t);
  zb_size_t params_len;
  zb_uint16_t data_len;
  zb_uint16_t txlen;
  zb_uint16_t ind_code = NCP_HL_INTRP_DATA_IND;

  params_len = sizeof(zb_intrp_data_ind_t) + sizeof(zb_uint8_t);
  data_len = (zb_uint16_t)zb_buf_len(param);
  txlen = ncp_hl_fill_ind_hdr(&ih, ind_code, sizeof(data_len) + sizeof(params_len) + params_len + data_len);

  /*
    Packet format:
    1b params len
    2b data len
    params (zb_intrp_data_ind_t)
    data
  */
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_intrp_cmd_ind data_len %hd params_len %hd bufid %hd",
            (FMT__H_H_H, data_len, (zb_uint16_t)params_len, (zb_uint16_t)param));

  p = (zb_uint8_t*)(&ih[1]);
  *p = (zb_uint8_t)params_len;
  p++;
  ZB_HTOLE16(p, &data_len);
  p += sizeof(data_len);

  ZB_HTOLE16_ONPLACE(ind.dst_pan_id);
  if (ind.dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
      || ind.dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
  {
    ZB_LETOH16_ONPLACE(ind.dst_addr.addr_short);
  }
  ZB_HTOLE16_ONPLACE(ind.src_pan_id);
  ZB_HTOLE16_ONPLACE(ind.profile_id);
  ZB_HTOLE16_ONPLACE(ind.cluster_id);

  ZB_MEMCPY(p, &ind, sizeof(ind));
  p += sizeof(ind);
  *p = current_page;
  p+= sizeof(current_page);
  *p = current_channel;
  p+= sizeof(current_channel);
  ZB_MEMCPY(p, zb_buf_begin(param), data_len);

  ncp_send_packet(ih, txlen);
  zb_buf_free(param);

  return ZB_B2U(ZB_TRUE);
}
#endif /* ZB_ENABLE_INTER_PAN_NON_DEFAULT_CHANNEL */


#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD
static zb_uint16_t handle_ncp_req_manuf_test(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;
  (void)hdr;
  (void)len;
  switch(hdr->hdr.call_id)
  {
    case NCP_HL_MANUF_MODE_START:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_MODE_START", (FMT__0));
      txlen = manuf_mode_start(hdr, len);
      break;
    case NCP_HL_MANUF_MODE_END:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_MODE_END", (FMT__0));
      txlen = manuf_mode_end(hdr, len);
      break;
    case NCP_HL_MANUF_SET_PAGE_AND_CHANNEL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_SET_PAGE_AND_CHANNEL", (FMT__0));
      txlen = manuf_set_page_and_channel(hdr, len);
      break;
    case NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_GET_PAGE_AND_CHANNEL", (FMT__0));
      txlen = manuf_get_page_and_channel(hdr, len);
      break;
    case NCP_HL_MANUF_SET_POWER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_SET_POWER", (FMT__0));
      txlen = manuf_set_power(hdr, len);
      break;
    case NCP_HL_MANUF_GET_POWER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_GET_POWER", (FMT__0));
      txlen = manuf_get_power(hdr, len);
      break;
    case NCP_HL_MANUF_START_TONE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_START_TONE", (FMT__0));
      txlen = manuf_start_tone(hdr, len);
      break;
    case NCP_HL_MANUF_STOP_TONE:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_STOP_TONE", (FMT__0));
      txlen = manuf_stop_tone(hdr, len);
      break;
    case NCP_HL_MANUF_START_STREAM_RANDOM:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_START_STREAM_RANDOM", (FMT__0));
      txlen = manuf_start_stream(hdr, len);
      break;
    case NCP_HL_MANUF_STOP_STREAM_RANDOM:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_STOP_STREAM_RANDOM", (FMT__0));
      txlen = manuf_stop_stream(hdr, len);
      break;
    case NCP_HL_MANUF_SEND_SINGLE_PACKET:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_SEND_SINGLE_PACKET", (FMT__0));
      txlen = manuf_send_packet(hdr, len);
      break;
    case NCP_HL_MANUF_START_TEST_RX:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_START_TEST_RX", (FMT__0));
      txlen = manuf_start_rx(hdr, len);
      break;
    case NCP_HL_MANUF_STOP_TEST_RX:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_MANUF_STOP_TEST_RX", (FMT__0));
      txlen = manuf_stop_rx(hdr, len);
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_manuf_test: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }
  return txlen;
}
#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */


#ifdef ZB_NCP_ENABLE_OTA_CMD
static zb_uint16_t handle_ncp_req_ota(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  switch(hdr->hdr.call_id)
  {
    case NCP_HL_OTA_RUN_BOOTLOADER:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_OTA_RUN_BOOTLOADER", (FMT__0));
      txlen = ota_run_bootloader(hdr, len);
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_secur: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }
  return txlen;
}
#endif /* ZB_NCP_ENABLE_OTA_CMD */


zb_uint16_t
ncp_hl_fill_resp_hdr(
  ncp_hl_response_header_t **rh,
  zb_uint_t resp_code,
  zb_uint8_t tsn,
  zb_ret_t status,
  zb_uint_t body_size)
{
  zb_ret_t ret;
  ncp_hl_response_header_t *hdr;
  zb_uint_t len = sizeof(ncp_hl_response_header_t) + body_size;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,7} */
  hdr = (ncp_hl_response_header_t *)NCP_CTX.tx_buf;
  if (rh != NULL)
  {
    *rh = hdr;
  }
  hdr->hdr.version = NCP_HL_PROTO_VERSION;
  hdr->hdr.control = NCP_HL_RESPONSE;
  hdr->hdr.call_id = (zb_uint16_t)resp_code;
  ZB_HTOLE16_ONPLACE(hdr->hdr.call_id);
  hdr->tsn = tsn;
  ret = ERROR_GET_CATEGORY(status);
  hdr->status_category = (zb_uint8_t)ret;
  ret = ERROR_GET_CODE(status);
  hdr->status_code = (zb_uint8_t)ret;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_fill_resp_hdr code %hd tsn %d status %d %d %d size %d",
            (FMT__D_H_D_D_D_D, resp_code, tsn, status, hdr->status_category, hdr->status_code, body_size));

  return (zb_uint16_t)len;
}


static zb_uint16_t ncp_hl_fill_ind_hdr(
  ncp_hl_ind_header_t **ih,
  zb_uint_t ind_code,
  zb_uint_t body_size)
{
  ncp_hl_ind_header_t *hdr;
  zb_uint16_t len = (zb_uint16_t)sizeof(ncp_hl_ind_header_t) + (zb_uint16_t)body_size;
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,8} */
  hdr = (ncp_hl_ind_header_t *)NCP_CTX.tx_buf;
  if (ih != NULL)
  {
    *ih = hdr;
  }
  hdr->version = NCP_HL_PROTO_VERSION;
  hdr->control = NCP_HL_INDICATION;
  hdr->call_id = (zb_uint16_t)ind_code;
  ZB_HTOLE16_ONPLACE(hdr->call_id);
  return len;
}


void ncp_hl_mark_ffd_just_boot(void)
{
  NCP_CTX.just_boot = ZB_TRUE;
}


void ncp_hl_send_reset_resp(zb_ret_t status)
{
  /* Customer requested to switch auto poll off by default */
  SET_POLL_STOPPED_FROM_HOST(ZB_TRUE);
  ncp_hl_send_no_arg_resp(NCP_HL_NCP_RESET, status);
}

void ncp_hl_send_reset_ind(zb_uint8_t rst_src)
{
  ncp_hl_ind_header_t *ih;
  zb_uint8_t *p;
  zb_uint16_t txlen;

  /* Customer requested to switch auto poll off by default */
  SET_POLL_STOPPED_FROM_HOST(ZB_TRUE);

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NCP_RESET_IND, sizeof(rst_src));
  p = (zb_uint8_t*)(&ih[1]);
  *p = rst_src;
  ncp_send_packet(ih, txlen);

}

void ncp_hl_reset_notify_ncp(zb_ret_t status)
{
  zb_uint8_t rst_src;
#if defined ZB_NSNG && defined SNCP_MODE
  static zb_bool_t first_time = true;
#endif /* ZB_NSNG && SNCP_MODE */

#ifdef ZB_NSNG
#ifdef SNCP_MODE
  if (first_time ==  true)
  {
    rst_src = ZB_RESET_SRC_RESET_PIN;
    first_time = false;
  }
  else
#endif /* SNCP_MODE */
  {
    rst_src = ZB_RESET_SRC_SW_RESET;
  }
#else
  rst_src = zb_get_reset_source();
#endif /* ZB_NSNG */

  if (rst_src == ZB_RESET_SRC_SW_RESET)
  {
    ncp_hl_send_reset_resp(status);
  }
  else
  {
    ncp_hl_send_reset_ind(rst_src);
  }
}

static void ncp_hl_send_no_arg_resp(ncp_hl_call_code_t req, zb_ret_t status)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_send_no_arg_resp %d", (FMT__D, status));
  txlen = ncp_hl_fill_resp_hdr(&rh, req,
                               stop_single_command((zb_uint16_t)req),
                               status, 0);
  ncp_send_packet(rh, txlen);
}

void ncp_hl_rx_device_annce(zb_zdo_signal_device_annce_params_t *params)
{
  zb_uint8_t *p;
  ncp_hl_ind_header_t *ih;
  zb_uint16_t txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_ZDO_DEV_ANNCE_IND, sizeof(params->device_short_addr) + sizeof(params->ieee_addr) + sizeof(params->capability));
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_zdo_dev_annce_ind dev 0x%x long " TRACE_FORMAT_64 " cap 0x%x",
            (FMT__D_A_D, params->device_short_addr, TRACE_ARG_64(params->ieee_addr), params->capability));
  p = (zb_uint8_t*)&ih[1];
  ZB_HTOLE16(p, &(params->device_short_addr));
  p+=sizeof(params->device_short_addr);
  ZB_HTOLE64(p, &(params->ieee_addr));
  p+=sizeof(params->ieee_addr);
  *p=params->capability;
  ncp_send_packet(ih, txlen);
}

zb_uint8_t ncp_hl_data_or_zdo_cmd_ind(zb_uint8_t param)
{
  /* whole structure will be copied to avoid alignment problems */
  zb_apsde_data_indication_t ind = *ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_bool_t is_zdo = ZB_FALSE;
  /* allow_processing_here variable - to reduce returns */
  zb_bool_t allow_processing_here = ZB_TRUE;

  /* after one of merges af_data_cb from af_rx.c is processed before the stack checks profile ID
     and NCP has to skip all of them except APS data packet
  */
  /* ZDO commands are also interesting as SNCP indication,
     but they must be passed for further processing in stack
  */
  if (
#ifndef SNCP_MODE
    (ind.profileid == ZB_AF_ZDO_PROFILE_ID) ||
    (ind.profileid == ZB_AF_GP_PROFILE_ID) ||
#endif
    ((ind.profileid == ZB_AF_SE_PROFILE_ID)
       && (ind.clusterid == ZB_ZCL_CLUSTER_ID_KEY_ESTABLISHMENT
#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
           || ind.clusterid == ZB_ZCL_CLUSTER_ID_SUB_GHZ
#endif
          )
     ))
  {
    /* Allow further packet processing */
    allow_processing_here = ZB_FALSE;
  }

#ifdef SNCP_MODE
  /* handle packets with profileid == ZB_AF_ZDO_PROFILE_ID and dst_endpoint == 0
   * as ZDO command indication */
  if ((ind.profileid == ZB_AF_ZDO_PROFILE_ID))
  {
    if ((ind.dst_endpoint == 0U) && ZB_ZDO_IS_CLI_CMD(ind.clusterid))
    {
      /* ZDO command from client to server
       * profileid = ZB_AF_ZDO_PROFILE_ID and
       * dst_endpoint = 0 (ZDO) and
       * clusterid < 0x8000 (*_req, not *_rsp) */
      is_zdo = ZB_TRUE;
    }
    else
    {
      /* Allow further packet processing */
      allow_processing_here = ZB_FALSE;
    }
  }
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_data_or_zdo_cmd_ind is_zdo %d ind.profileid 0x%x ind.clusterid 0x%x from 0x%x bufid %hd",
            (FMT__D_H_H_H_H, is_zdo, ind.profileid, ind.clusterid, ind.src_addr, (zb_uint16_t)param));

  if (allow_processing_here)
  {
#ifdef SNCP_MODE
    if (is_zdo)
    {
      if (ncp_exec_blocked())
      {
        /* we must copy the buffer to let it be processed further in stack and
           keep data from it to send an indication later,
           it must be synchronous operation as we are in middle of processing by
           void zb_apsde_data_indication(zb_uint8_t param)
        */
        zb_bufid_t copy_buf = zb_buf_get(ZB_TRUE, 0);

        if (copy_buf != ZB_BUF_INVALID)
        {
          zb_buf_copy(param, copy_buf);

          ncp_enqueue_buf(NCP_DATA_OR_ZDO_IND, 1U/*is_zdo: always true*/, copy_buf);
        }
        else
        {
          /* there are no free buffers to pass ZDO indication to Host, skip it */
        }
      }
      else
      {
        send_data_or_zdo_indication(is_zdo, param);
      }

      /* pass unprocessing ZDO to stack */
      allow_processing_here = ZB_FALSE;
    }
    else
#endif
    {
      ncp_hl_data_or_zdo_ind(is_zdo, param);
    }
  }

  return ZB_B2U(allow_processing_here);
}

void ncp_hl_data_or_zdo_ind(zb_bool_t is_zdo, zb_uint8_t param)
{
  if (ncp_exec_blocked())
  {
    ncp_enqueue_buf(NCP_DATA_OR_ZDO_IND, ZB_B2U(is_zdo), param);
  }
  else
  {
    ncp_hl_data_or_zdo_ind_exec(is_zdo, param);
  }
}

void ncp_hl_data_or_zdo_ind_exec(zb_bool_t is_zdo, zb_uint8_t param)
{
  send_data_or_zdo_indication(is_zdo, param);
  zb_buf_free(param);
}

static void send_data_or_zdo_indication(zb_bool_t is_zdo, zb_uint8_t param)
{
  zb_uint8_t *p;
  ncp_hl_ind_header_t *ih;
  /* whole structure will be copied to avoid alignment problems */
  zb_apsde_data_indication_t ind = *ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);
  zb_size_t params_len = ZB_OFFSETOF(zb_apsde_data_indication_t, FIRST_INTERNAL_APSIND_FIELD);
  /* Just in case make data_len 2 bytes: will we glue fragments here?? */
  zb_uint16_t data_len;
  zb_uint16_t txlen;
  zb_uint16_t ind_code = (is_zdo) ? NCP_HL_ZDO_REMOTE_CMD_IND : NCP_HL_APSDE_DATA_IND;

  data_len = (zb_uint16_t)zb_buf_len(param);
  txlen = ncp_hl_fill_ind_hdr(&ih, ind_code, sizeof(data_len) + sizeof(params_len) + params_len + data_len);

  /*
    Packet format:
    1b params len
    2b data len
    params (begin of zb_apsde_data_indication_t)
    data
  */
  TRACE_MSG(TRACE_TRANSPORT3, "send_data_or_zdo_indication is_zdo %d data_len %hd params_len %hd from 0x%x bufid %hd",
            (FMT__D_H_H_H_H, is_zdo, data_len, (zb_uint16_t)params_len, ind.src_addr, (zb_uint16_t)param));
  p = (zb_uint8_t*)(&ih[1]);
  *p = (zb_uint8_t)params_len;
  p++;
  ZB_HTOLE16(p, &data_len);
  p += sizeof(data_len);

  if (ZB_APS_FC_GET_DELIVERY_MODE(ind.fc) != ZB_APS_DELIVERY_GROUP)
  {
    /* If not group, let it be 0 to exclude confusing */
    ind.group_addr = 0;
  }
  ZB_HTOLE16_ONPLACE(ind.src_addr);
  ZB_HTOLE16_ONPLACE(ind.dst_addr);
  ZB_HTOLE16_ONPLACE(ind.group_addr);
  ZB_HTOLE16_ONPLACE(ind.clusterid);
  ZB_HTOLE16_ONPLACE(ind.profileid);
  ZB_HTOLE16_ONPLACE(ind.mac_src_addr);
  ZB_HTOLE16_ONPLACE(ind.mac_dst_addr);
  ind.reserved = 0;

  ZB_MEMCPY(p, &ind, params_len);
  ZB_MEMCPY(&p[params_len], zb_buf_begin(param), data_len);

  ncp_send_packet(ih, txlen);
}


static void ncp_hl_nwk_join_ind(zb_uint8_t param)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_nwk_join_ind", (FMT__0));

  /* Fixme: do we need to analyze anything from param? */
  ZVUNUSED(param);

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_REJOINED_IND, JOIN_RESP_SIZE);
  ncp_hl_fill_join_resp_body(&ih[1]);
  (void)stop_single_command(NCP_HL_NWK_NLME_JOIN);
  ncp_send_packet(ih, txlen);
}


static void ncp_hl_nwk_join_failed_ind(zb_ret_t status)
{
  zb_ret_t ret;
  ncp_hl_ind_header_t *ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_nwk_join_failed_ind %d", (FMT__D, status));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_REJOIN_FAILED_IND, 2);
  p = (zb_uint8_t*)&ih[1];
  ret = ERROR_GET_CATEGORY(status);
  *p = (zb_uint8_t)ret;
  p++;
  ret = ERROR_GET_CODE(status);
  *p = (zb_uint8_t)ret;
  (void)stop_single_command(NCP_HL_NWK_NLME_JOIN);
  ncp_send_packet(ih, txlen);
}

/*
ID = NCP_HL_SECUR_TCLK_IND
  Indication:
    8b: trust center ieee address
    1b: key type
*/
static void ncp_hl_secur_tclk_ind(void)
{
  ncp_hl_ind_header_t *ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_secur_tclk_ind", (FMT__0));

  txlen = ncp_hl_fill_ind_hdr(&ih,
                              NCP_HL_SECUR_TCLK_IND,
                              sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t));
  p = (zb_uint8_t*)&ih[1];

  zb_aib_get_trust_center_address(p);
  ZB_DUMP_IEEE_ADDR(p);
  p += sizeof(zb_ieee_addr_t);

  /* TODO: set key type when it is supported by the stack.
     0 means key obtained using APS Request Key,
     1 - CBKE.
     See 5.3.15 bdbTCLinkKeyExchangeMethod attribute in the BDB spec */
  *p = 0;

  TRACE_MSG(TRACE_TRANSPORT3, "key type: 0x%x", (FMT__D, *p));

  ncp_send_packet(ih, txlen);
}

/*
ID = NCP_HL_SECUR_TCLK_EXCHANGE_FAILED_IND
  Indication:
    1b: status category
    1b: status code
*/
static void ncp_hl_secur_tclk_exchange_failed_ind(zb_ret_t status)
{
  ncp_hl_ind_header_t *ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_secur_tclk_ind, status %d", (FMT__D, status));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_SECUR_TCLK_EXCHANGE_FAILED_IND, 2);
  p = (zb_uint8_t*)&ih[1];
  *p = ERROR_GET_CATEGORY(status);
  p++;
  *p = ERROR_GET_CODE(status);
  ncp_send_packet(ih, txlen);
}

/*
ID = NCP_HL_NWK_LEAVE_IND:
  Indication:
    8b: ieee address
    1b: rejoin
*/
void ncp_hl_nwk_leave_ind(zb_zdo_signal_leave_indication_params_t *leave_ind_params)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_LEAVE_IND, sizeof(zb_zdo_signal_leave_indication_params_t));
  p = (zb_uint8_t*)(&ih[1]);
  ZB_IEEE_ADDR_COPY(p, (leave_ind_params->device_addr));
  ZB_DUMP_IEEE_ADDR(p);
  p += sizeof(zb_ieee_addr_t);
  *p = leave_ind_params->rejoin;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_nwk_leave_ind rejoin %d", (FMT__D, *p));

  ncp_send_packet(ih, txlen);
}

#ifndef SNCP_MODE
/*
ID = NCP_HL_ZDO_DEV_AUTHORIZED_IND:
  Indication:
    8b: ieee address
    2b: nwk address
    1b: authorization type
    1b: authorization status
*/
void ncp_hl_nwk_device_authorized_ind(zb_zdo_signal_device_authorized_params_t *params)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_hl_nwk_device_authorized_ind", (FMT__0));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_ZDO_DEV_AUTHORIZED_IND,
    sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t));
  p = (zb_uint8_t*)(&ih[1]);
  ZB_IEEE_ADDR_COPY(p, (params->long_addr));
  ZB_DUMP_IEEE_ADDR(p);
  p += sizeof(zb_ieee_addr_t);

  ZB_HTOLE16(p, &params->short_addr);

  p += sizeof(zb_uint16_t);

  *p = params->authorization_type;
  p += sizeof(zb_uint8_t);

  *p = params->authorization_status;

  ncp_send_packet(ih, txlen);
}


/*
ID = NCP_HL_ZDO_DEV_UPDATE_IND:
  Indication:
    8b: ieee address
    2b: nwk address
    1b: status
*/
void ncp_hl_nwk_device_update_ind(zb_zdo_signal_device_update_params_t *params)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_hl_nwk_device_update_ind", (FMT__0));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_ZDO_DEV_UPDATE_IND,
    sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t) + sizeof(zb_uint8_t));
  p = (zb_uint8_t*)(&ih[1]);
  ZB_IEEE_ADDR_COPY(p, (params->long_addr));
  ZB_DUMP_IEEE_ADDR(p);
  p += sizeof(zb_ieee_addr_t);

  ZB_HTOLE16(p, &params->short_addr);

  p += sizeof(zb_uint16_t);

  *p = params->status;

  ncp_send_packet(ih, txlen);
}
#endif

/*
ID = NCP_HL_NWK_PAN_ID_CONFLICT_IND:
  Indication:
    2b: PAN ID count
    [n]2b: array of such PAN ID, only ID from 0 to panid_count will be taken into consideration
*/
void ncp_hl_nwk_pan_id_conflict_ind(zb_pan_id_conflict_info_t *pan_id_params)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;
  zb_uindex_t i;

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_PAN_ID_CONFLICT_IND, sizeof(zb_pan_id_conflict_info_t));
  p = (zb_uint8_t*)(&ih[1]);
  ZB_HTOLE16(p, &pan_id_params->panid_count);
  p+=sizeof(zb_uint16_t);
  for(i = 0; i < ZB_PAN_ID_CONFLICT_INFO_MAX_PANIDS_COUNT; i++)
  {
    ZB_HTOLE16(p, &pan_id_params->panids[i]);
    p+=sizeof(zb_uint16_t);
  }
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_nwk_pan_id_conflict_ind txlen %d", (FMT__D, txlen));

  ncp_send_packet(ih, txlen);
}


/*
  ID = NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE:
  Request:
    5b: Common Request Header
    2b: PAN ID count
    [n]2b: array of such PAN ID, only ID from 0 to panid_count will be taken into consideration
*/
static zb_uint16_t ncp_hl_nwk_pan_id_conflict_resolve(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t panid_count;
  zb_uint8_t* p = (zb_uint8_t*)(&hdr[1]);
  zb_ret_t ret = RET_OK;
  zb_uindex_t i;
  zb_pan_id_conflict_info_t *pan_id_params;
  zb_uint8_t buf = 0;

  ZB_MEMCPY(&panid_count, p, sizeof(panid_count));
  p+=sizeof(zb_uint16_t);
  if (len < sizeof(*hdr) + sizeof(panid_count) + panid_count * sizeof(zb_uint16_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (RET_OK == ret)
  {
    pan_id_params = zb_buf_initial_alloc(buf, sizeof(zb_pan_id_conflict_info_t));
    pan_id_params->panid_count = panid_count;
    for(i = 0; i < panid_count; i++)
    {
      ZB_LETOH16(&pan_id_params->panids[i], p);
      p+=sizeof(zb_uint16_t);
    }
    zb_start_pan_id_conflict_resolution(buf);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_PAN_ID_CONFLICT_RESOLVE, hdr->tsn, ret, 0);
}


/*
  ID = NCP_HL_NWK_ENABLE_PAN_ID_CONFLICT_RESOLUTION:
  Request:
    5b: Common Request Header
    1b: enable flag
*/
static zb_uint16_t enable_pan_id_conflict_resolution(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, (zb_size_t)len);
  zb_uint8_t enable_conflict_resolution;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(enable_conflict_resolution);

  TRACE_MSG(TRACE_TRANSPORT3, ">> enable_pan_id_conflict_resolution", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &enable_conflict_resolution);

    TRACE_MSG(TRACE_TRANSPORT3, "  enable_conflict_resolution %hd", (FMT__H, enable_conflict_resolution));

    zb_enable_panid_conflict_resolution(ZB_U2B(enable_conflict_resolution));
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< enable_pan_id_conflict_resolution, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
  ID = NCP_HL_NWK_ENABLE_AUTO_PAN_ID_CONFLICT_RESOLUTION:
  Request:
    5b: Common Request Header
    1b: enable flag
*/
static zb_uint16_t enable_auto_pan_id_conflict_resolution(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, (zb_size_t)len);
  zb_uint8_t enable_auto_conflict_resolution;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(enable_auto_conflict_resolution);

  TRACE_MSG(TRACE_TRANSPORT3, ">> enable_auto_pan_id_conflict_resolution", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &enable_auto_conflict_resolution);

    TRACE_MSG(TRACE_TRANSPORT3, "  enable_auto_conflict_resolution %hd", (FMT__H, enable_auto_conflict_resolution));

    zb_enable_auto_pan_id_conflict_resolution(ZB_U2B(enable_auto_conflict_resolution));
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< enable_auto_pan_id_conflict_resolution, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_NWK_ADDRESS_UPDATE_IND:
  Indication:
    2b: NWK address (short)
*/
void ncp_address_update_ind(zb_uint16_t short_address)
{
  zb_uint16_t txlen;
  ncp_hl_ind_header_t *ih;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_address_update_ind addr 0x%x", (FMT__D, short_address));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_ADDRESS_UPDATE_IND, sizeof(short_address));
  ZB_HTOLE16(&ih[1], &short_address);

  ncp_send_packet(ih, txlen);
}

#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
/*
ID = NCP_HL_NWK_ROUTE_REPLY_IND:
  Indication:
    7b:  route reply
*/
void ncp_route_reply_ind(zb_nwk_cmd_rrep_t *rrep)
{
  zb_uint16_t txlen;
  ncp_hl_ind_header_t *ih;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_route_reply_ind", (FMT__H_D_D_H, rrep->rreq_id, rrep->originator, rrep->responder, rrep->path_cost));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_ROUTE_REPLY_IND, sizeof(zb_nwk_cmd_rrep_t));
  ZB_MEMCPY(&ih[1], rrep, sizeof(zb_nwk_cmd_rrep_t));
  p = (zb_uint8_t*)(&ih[1]);
  /*originator and responder fields*/
  p = &p[2];
  ZB_HTOLE16_ONPLACE(*p);
  p = &p[4];
  ZB_HTOLE16_ONPLACE(*p);

  ncp_send_packet(ih, txlen);
}

/*
ID = NCP_HL_NWK_ROUTE_REQUEST_SEND_IND
  Indication:
    5b: route request
*/
void ncp_nwk_route_req_send_ind(zb_nwk_cmd_rreq_t *rreq)
{
  ncp_hl_ind_header_t *ih;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_route_request_ind, status %d", (FMT__H_D_H, rreq->rreq_id, rreq->dest_addr, rreq->path_cost));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_ROUTE_REQUEST_SEND_IND, sizeof(zb_nwk_cmd_rreq_t));
  ZB_MEMCPY(&ih[1], rreq, sizeof(zb_nwk_cmd_rreq_t));
  p = (zb_uint8_t*)(&ih[1]);
  /*dest_addr field*/
  p = &p[2];
  ZB_HTOLE16_ONPLACE(*p);

  ncp_send_packet(ih, txlen);
}

/*
ID = NCP_HL_NWK_ROUTE_RECORD_SEND_IND
  Indication:
    3b: route record
*/
void ncp_nwk_route_send_rrec_ind(zb_nwk_cmd_rrec_t *rrec)
{
  ncp_hl_ind_header_t *ih;
  zb_uint16_t txlen;
  zb_uint8_t *p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_route_record_ind, status %d", (FMT__H_D, rrec->relay_cnt, rrec->relay_addr));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_ROUTE_RECORD_SEND_IND, sizeof(zb_nwk_cmd_rrec_t));
  ZB_MEMCPY(&ih[1], rrec, sizeof(zb_nwk_cmd_rrec_t));
  p = (zb_uint8_t*)(&ih[1]);
  /*relay_addr field*/
  p = &p[1];
  ZB_HTOLE16_ONPLACE(*p);

  ncp_send_packet(ih, txlen);
}
#endif
/*
ID = NCP_HL_SECUR_ADD_IC:
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
      [N]: install code, where N can be 8, 10, 14 or 18
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static void secur_add_ic_cb(zb_ret_t status)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_SECUR_ADD_IC);
  zb_uint16_t txlen;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_add_ic_cb", (FMT__0));

  ZB_ASSERT(tsn != TSN_RESERVED);

  if (status != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zb_secur_add_ic: status %d", (FMT__D, status));
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "zb_secur_add_ic successfully", (FMT__0));
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_ADD_IC, tsn, status, 0);

  ncp_send_packet(hdr, txlen);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_add_ic_cb", (FMT__0));
}

static zb_uint16_t secur_add_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES

  zb_uint8_t ic_len = 0;
  zb_uint8_t body_len = 0;
  zb_uint8_t ic_type = 0;
  zb_uint8_t* ic = NULL;
  zb_bool_t cmd_is_started = ZB_FALSE;

  body_len = len - sizeof(ncp_hl_request_header_t);

  if (body_len > (sizeof(zb_ieee_addr_t) + ZB_IC_TYPE_MAX_SIZE + 2) ) /* 2 is CRC16 size*/
  {
    TRACE_MSG(TRACE_ERROR, "secur_add_ic: wrong arguments body_len %d", (FMT__D, body_len));
    ret = RET_INVALID_FORMAT;
  }

  if (body_len <= sizeof(zb_ieee_addr_t))
  {
    TRACE_MSG(TRACE_ERROR, "secur_add_ic: wrong arguments body_len %d", (FMT__D, body_len));
    ret = RET_INVALID_FORMAT;
  }

  ret = start_single_command(hdr);

  if (ret == RET_OK)
  {
    ic_len = body_len - sizeof(zb_ieee_addr_t);
    ret = get_ic_type_by_len(ic_len, &ic_type);
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    ic = (zb_uint8_t*)&hdr[1] + sizeof(zb_ieee_addr_t);
    zb_secur_ic_add((zb_uint8_t*)&hdr[1], ic_type, ic, secur_add_ic_cb);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    if (cmd_is_started)
    {
      stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_ADD_IC, hdr->tsn, ret, 0);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  return txlen;
}

/*
ID = NCP_HL_SECUR_SET_LOCAL_IC:
  Request:
    header:
      2b: ID
    body:
      [N]: install code, where N can be 8, 10, 14 or 18
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t secur_set_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t ic_len;
  zb_uint8_t ic_type;

  ic_len = len - (zb_uint16_t)sizeof(ncp_hl_request_header_t);

  if (ic_len > ZB_IC_TYPE_MAX_SIZE + 2U) /* 2 is CRC16 size*/
  {
    TRACE_MSG(TRACE_ERROR, "secur_set_ic: wrong arguments ic_len %d", (FMT__D, ic_len));
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = RET_OK;
  }

  if (ret == RET_OK)
  {
    ret = get_ic_type_by_len((zb_uint8_t)ic_len, &ic_type);

    if (RET_OK == ret)
    {
      ret = zb_secur_ic_set(ic_type, (zb_uint8_t*)&hdr[1]);
      if (ret != RET_OK)
      {
        TRACE_MSG(TRACE_ERROR, "zb_secur_ic_set: status %d", (FMT__D, ret));
      }
      else
      {
        TRACE_MSG(TRACE_TRANSPORT3, "zb_secur_ic_set successfully", (FMT__0));
      }
    }
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_SET_LOCAL_IC, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_SECUR_GET_IC_BY_IEEE:
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: N - len of IC
      [N]: install code, where N can be 8, 10, 14 or 18
*/
static zb_uint16_t secur_get_ic_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;
  /* Requirement for SNCP is ZR and ZED only */
#ifdef ZB_COORDINATOR_ROLE
  zb_ret_t ret = RET_OK;
  zb_uint8_t ic[ZB_CCM_KEY_SIZE+ZB_CCM_KEY_CRC_SIZE];
  zb_uint8_t ic_type = 0;
  zb_uint8_t ic_len = 0;
  zb_int8_t status = 0;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p = NULL;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_get_ic_by_ieee len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(zb_ieee_addr_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    status = zb_secur_ic_get_from_tc_storage((zb_uint8_t*)&hdr[1], &ic_type, ic);
    if (status < 0)
    {
      ret = RET_NOT_FOUND;
    }
  }
  if (RET_OK == ret)
  {
    ic_len = ZB_IC_SIZE_BY_TYPE(ic_type);
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_GET_IC_BY_IEEE, hdr->tsn, ret, sizeof(ic_type) + ic_len + 2);
    p = (zb_uint8_t*)&rh[1];
    *p = ic_len + 2;
    p++;
    ZB_MEMCPY(p, ic, ic_len + 2);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_IC_BY_IEEE, hdr->tsn, ret, 0);
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<<secur_get_ic_by_ieee, ret %d, txlen %hu", (FMT__D_H, ret, txlen));
#else
 txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_IC_BY_IEEE, hdr->tsn, RET_NOT_IMPLEMENTED, 0);
#endif

  return txlen;
}

static zb_ret_t get_ic_type_by_len(zb_uint8_t ic_len, zb_uint8_t* ic_type)
{
  zb_ret_t ret = RET_OK;

  switch (ic_len)
  {
    case 8:
      *ic_type = ZB_IC_TYPE_48;
      break;
    case 10:
      *ic_type = ZB_IC_TYPE_64;
      break;
    case 14:
      *ic_type = ZB_IC_TYPE_96;
      break;
    case 18:
      *ic_type = ZB_IC_TYPE_128;
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "get_ic_type_by_len: wrong install code len %d", (FMT__D, ic_len));
      ret = RET_INVALID_FORMAT;
      break;
  }

  return ret;
}

/*
ID = NCP_HL_SECUR_DEL_IC:
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static void secur_del_ic_cb(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_SECUR_DEL_IC);
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_secur_ic_remove_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_resp_t);
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_del_ic_cb param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  ret = (zb_ret_t)resp->status;

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_DEL_IC, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_del_ic_cb", (FMT__0));
}


static zb_uint16_t secur_del_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES

  zb_size_t expected = sizeof(zb_ieee_addr_t);
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_bool_t cmd_is_started = ZB_FALSE;
  zb_uint8_t buf = 0;

  ret = start_single_command(hdr);

  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    zb_secur_ic_remove_req_t *param;

    param = ZB_BUF_GET_PARAM(buf, zb_secur_ic_remove_req_t);
    param->response_cb = secur_del_ic_cb;

    ncp_hl_body_get_u64addr(&body, param->device_address);

    zb_secur_ic_remove_req(buf);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);

    if (cmd_is_started)
    {
      stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else  /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  return txlen;
}

/*
ID = NCP_HL_SECUR_JOIN_USES_IC:
  Request:
    header:
      2b: ID
    body:
      1b: enable/disable (0/1) joining uses ic
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t secur_join_uses_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{

#if defined ZB_SECURITY_INSTALLCODES

  zb_ret_t ret = RET_OK;
  zb_uint8_t enable;

  if (len < sizeof(*hdr) + sizeof(enable))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (RET_OK == ret)
  {
    enable = *(zb_uint8_t*)&hdr[1];
    ZB_TCPOL().require_installcodes = ZB_U2B(enable);
  }

#else /* defined ZB_SECURITY_INSTALLCODES */
  zb_ret_t ret = RET_NOT_IMPLEMENTED;
#endif /* defined ZB_SECURITY_INSTALLCODES */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_JOIN_USES_IC, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_SECUR_GET_LOCAL_IC:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: N - len of IC
      [N]: install code, where N can be 8, 10, 14 or 18
*/
static zb_uint16_t secur_get_local_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t *ic = NULL;
  zb_uint8_t ic_type = 0;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_get_local_ic len %hu", (FMT__H, len));

  if (len != sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ic = zb_secur_ic_get_from_client_storage(&ic_type);
    if (ic == NULL)
    {
      ret = RET_NOT_FOUND;
    }
  }
  if (RET_OK == ret)
  {
    zb_uint8_t *p;
    zb_uint8_t ic_len = ZB_IC_SIZE_BY_TYPE(ic_type);

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_GET_LOCAL_IC, hdr->tsn, ret, sizeof(ic_type) + ic_len + 2U);
    p = (zb_uint8_t*)&rh[1];
    *p = ic_len + 2U;
    p++;
    ZB_MEMCPY(p, ic, ((zb_size_t)ic_len) + 2U);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_LOCAL_IC, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<secur_get_local_ic, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
ID = NCP_HL_SECUR_GET_KEY_IDX
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      2b: index of keypair
*/
static zb_uint16_t secur_get_key_idx(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_get_key_idx len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(zb_ieee_addr_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    zb_uint8_t *p = (zb_uint8_t*)&hdr[1];
    zb_uint16_t idx = zb_aps_keypair_get_index_by_addr(p, ZB_SECUR_VERIFIED_KEY);
    if (idx == (zb_uint16_t)-1)
    {
      idx = zb_aps_keypair_get_index_by_addr(p, ZB_SECUR_ANY_KEY_ATTR);
    }
    else
    {
      /* MISRA rule 15.7 requires empty 'else' branch. */
    }
    if (idx != (zb_uint16_t)-1) {
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_GET_KEY_IDX, hdr->tsn, ret, sizeof(idx));
      ZB_HTOLE16(&rh[1], &idx);
    }
    else
    {
      ret = RET_NOT_FOUND;
    }
  }

  if (ret != RET_OK)
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_KEY_IDX, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<secur_get_key_idx, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
ID = NCP_HL_SECUR_GET_KEY
  Request:
    header:
      2b: ID
    body:
      2b: index of the key
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      16b: key
      1b:  aps link key type
      1b:  key source
      1b:  key attributes
      4b:  outgoing frame counter
      4b:  incoming frame counter
      8b:  partner IEEE address
*/
static zb_uint16_t secur_get_key(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  ncp_hl_response_partner_lk_t ncp_key_info;
  zb_aps_device_key_pair_nvram_t zb_key_info;
  zb_uint32_t tmp_frame_counter;
  zb_uint16_t idx;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_get_key len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(zb_uint16_t))
  {
    ret = RET_INVALID_FORMAT;
  }
#ifdef ZB_HAVE_BLOCK_KEYS
  else if (zb_chip_lock_get())
  {
    ret = NCP_RET_KEYS_LOCKED;
  }
#endif
  else
  {
    ZB_MEMCPY(&idx, &hdr[1], sizeof(idx));
    ret = zb_aps_keypair_read_by_idx(idx, &zb_key_info);
  }

  if (ret == RET_OK)
  {
    ZB_MEMCPY(&ncp_key_info.key, &zb_key_info.link_key, sizeof(zb_key_info.link_key));
    ncp_key_info.aps_link_key_type = zb_key_info.aps_link_key_type;
    ncp_key_info.key_source = zb_key_info.key_source;
    ncp_key_info.key_attributes = zb_key_info.key_attributes;
    tmp_frame_counter = ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].outgoing_frame_counter;
    ZB_HTOLE32(&ncp_key_info.outgoing_frame_counter, &tmp_frame_counter);
    tmp_frame_counter = ZB_AIB().aps_device_key_pair_storage.key_pair_set[idx].incoming_frame_counter;
    ZB_HTOLE32(&ncp_key_info.incoming_frame_counter, &tmp_frame_counter);
    ZB_MEMCPY(&ncp_key_info.partner_address, &zb_key_info.device_address, sizeof(zb_key_info.device_address));

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_GET_KEY, hdr->tsn, ret, sizeof(ncp_key_info));
    ZB_MEMCPY(&rh[1], &ncp_key_info, sizeof(ncp_key_info));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_KEY, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<secur_get_key, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
ID = NCP_HL_SECUR_ERASE_KEY
  Request:
    header:
      2b: ID
    body:
      2b: index of the key
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t secur_erase_key(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_erase_key len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(zb_uint16_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    zb_uint16_t idx;

    ZB_LETOH16(&idx, &hdr[1]);
    if (zb_aps_keypair_load_by_idx(idx) == RET_OK)
    {
      zb_secur_delete_link_key_by_idx(idx);
    }
    else
    {
      ret = RET_OPERATION_FAILED;
    }
  }
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_ERASE_KEY, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_SECUR_CLEAR_KEY_TABLE
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t secur_clear_key_table(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t idx;
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_clear_key_table len %hu", (FMT__H, len));

  if (len != sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    for (idx = 0; idx < ZB_N_APS_KEY_PAIR_ARR_MAX_SIZE; ++idx)
    {
      zb_secur_delete_link_key_by_idx(idx);
    }
  }
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_CLEAR_KEY_TABLE, hdr->tsn, ret, 0);
}

#ifdef ZB_ENABLE_SE_MIN_CONFIG
/*
  Request:

  1b suite (KEC_CS1 - 1, KEC_CS2 - 2)
  22 (CS1) or 37 (CS2) b ca_public_key
  48 or 74 b certificate
  21 or 36 b private_key

  Resp:
  status
*/
static zb_uint16_t secur_add_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#ifdef ZB_SE_ENABLE_KEC_CLUSTER
#if !defined ZB_PRODUCTION_CONFIG

  zb_ret_t ret = RET_OK;
  zb_uint8_t body_len = len - sizeof(ncp_hl_request_header_t);
  zb_uint8_t cs_type;
  zb_uint8_t cs_type_code;
  zb_uint8_t *p;
  zb_uint8_t *ca_public_key;
  zb_uint8_t *certificate;
  zb_uint8_t *private_key;

  if (body_len < 1)
  {
    /* First check for 1 byte cs type */
    TRACE_MSG(TRACE_ERROR, "secur_add_cert: too small body_len %hd", (FMT__H, body_len));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    p = (zb_uint8_t*)&hdr[1];
    cs_type_code = *p;
    cs_type = zb_kec_get_suite_num(cs_type_code);
    p++;
    if (cs_type == 0xffU)
    {
      ret = RET_INVALID_PARAMETER_1;
      TRACE_MSG(TRACE_ERROR, "invalid cs type %hd", (FMT__H, cs_type));
    }
  }
  if (ret == RET_OK)
  {
    if (body_len != 1 + ZB_CERT_SIZE[cs_type] + ZB_QE_SIZE[cs_type] + ZB_PR_SIZE[cs_type])
    {
      TRACE_MSG(TRACE_ERROR, "invalid body length %d, wants %d",
                (FMT__D_D, body_len, ZB_CERT_SIZE[cs_type] + ZB_QE_SIZE[cs_type] + ZB_PR_SIZE[cs_type]));
      ret = RET_INVALID_FORMAT;
    }
  }
  if (ret == RET_OK)
  {
    ca_public_key = p;
    certificate = &ca_public_key[ZB_QE_SIZE[cs_type]];
    private_key = &ca_public_key[ZB_QE_SIZE[cs_type] + ZB_CERT_SIZE[cs_type]];

    ret = zb_se_load_ecc_cert(cs_type_code, ca_public_key, certificate, private_key);
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "zb_se_load_ecc_cert error status %d", (FMT__D, ret));
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT3, "zb_se_load_ecc_cert ok", (FMT__0));
    }
  }
#else /* !defined ZB_PRODUCTION_CONFIG */
  zb_ret_t ret = RET_NOT_IMPLEMENTED;

  ZVUNUSED(len);
#endif /* !defined ZB_PRODUCTION_CONFIG */

#else /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  zb_ret_t ret = RET_NOT_IMPLEMENTED;

  ZVUNUSED(len);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_ADD_CERT, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_SECUR_DEL_CERT:
  Request:
    5b: Common Request Header
    1b: Suite #
    8b: Issuer
    8b: IEEE Address (MAC)
*/
static zb_uint16_t secur_del_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
#if !defined ZB_PRODUCTION_CONFIG
  zb_uint8_t suite_no;
  zb_uint8_t issuer[8];
  zb_ieee_addr_t ieee_addr;
  zb_uint8_t *p = (zb_uint8_t *)&hdr[1];
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr)
            + sizeof(suite_no)
            + sizeof(issuer)
            + sizeof(ieee_addr)
     )
  {
    ret = RET_INVALID_FORMAT;
  }

  if (RET_OK == ret)
  {
    suite_no = zb_kec_get_suite_num(*p);
    if (suite_no == 0xffU)
    {
      ret = RET_INVALID_PARAMETER_1;
      TRACE_MSG(TRACE_ERROR, "invalid certificate suite %hd", (FMT__H, *p));
    }
    p++;
    ZB_MEMCPY(issuer, p, sizeof(issuer));
    p += sizeof(issuer);
    ZB_MEMCPY(ieee_addr, p, sizeof(ieee_addr));
    zb_inverse_bytes(ieee_addr, sizeof(ieee_addr));
  }

  if (RET_OK == ret)
  {
    ret = zb_se_erase_ecc_cert(suite_no, issuer, ieee_addr);
  }
#else /* !defined ZB_PRODUCTION_CONFIG */
  zb_ret_t ret = RET_NOT_IMPLEMENTED;

  ZVUNUSED(len);
#endif /* !defined ZB_PRODUCTION_CONFIG */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_DEL_CERT, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_SECUR_GET_CERT:
  Request:
    5b: Common Request Header
    1b: Suite #
    8b: Issuer
    8b: IEEE Address (MAC)
  Response:
    7b: Common Response Header
    1b: Suite # (KEC_CS1 - 1, KEC_CS2 - 2)
    22/37b: ca_public_key
    48/74b: certificate
    21/36b: private_key
*/
static zb_uint16_t secur_get_cert(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint8_t suite_no;
  zb_uint8_t issuer[8];
  zb_ieee_addr_t ieee_addr;
  zb_uint8_t *p = (zb_uint8_t *)&hdr[1];
  zb_ret_t ret = RET_OK;
  zse_cert_nvram_t buf;
  zse_cert_nvram_t *existing_cert;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>secur_get_cert len %hu", (FMT__H, len));

  if (len < sizeof(*hdr)
            + sizeof(suite_no)
            + sizeof(issuer)
            + sizeof(ieee_addr)
     )
  {
    ret = RET_INVALID_FORMAT;
  }

  if (RET_OK == ret)
  {
    suite_no = zb_kec_get_suite_num(*p);
    if (suite_no == 0xffU)
    {
      ret = RET_INVALID_PARAMETER_1;
      TRACE_MSG(TRACE_ERROR, "invalid certificate suite %hd", (FMT__H, *p));
    }
    p++;
    ZB_MEMCPY(issuer, p, sizeof(issuer));
    p += sizeof(issuer);
    ZB_MEMCPY(ieee_addr, p, sizeof(ieee_addr));
  }

  /* since there isn't MULTIMAC */
  if (!ZB_IEEE_ADDR_CMP(ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS()))
  {
    ret = RET_INVALID_PARAMETER_4;
  }
  else
  {
    /* subject stores in certificate in reverse order */
    zb_inverse_bytes(ieee_addr, sizeof(ieee_addr));
  }

  if (RET_OK == ret)
  {
    existing_cert = zse_certdb_get_from_tc_storage(suite_no, issuer, &buf);
    if (existing_cert == NULL)
    {
      ret = RET_NOT_FOUND;
    }
  }

  if (RET_OK == ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_GET_CERT, hdr->tsn, ret, sizeof(suite_no) +
                                 ZB_QE_SIZE[suite_no] + ZB_CERT_SIZE[suite_no] + ZB_PR_SIZE[suite_no]);
    p = (zb_uint8_t*)&rh[1];
    *p = (1U << suite_no);
    p++;
    ZB_MEMCPY(p, existing_cert->ca.u8, ZB_QE_SIZE[suite_no]);
    p += ZB_QE_SIZE[suite_no];
    ZB_MEMCPY(p, existing_cert->cert.u8, ZB_CERT_SIZE[suite_no]);
    p += ZB_CERT_SIZE[suite_no];
    ZB_MEMCPY(p, existing_cert->pr.u8, ZB_PR_SIZE[suite_no]);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_GET_CERT, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<secur_get_cert, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

static zb_uint16_t secur_start_cbke(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;
  zb_ret_t ret;
  zb_uint8_t cs = 0;
  zb_uint16_t short_addr;

  /*
    1b - CS1, CS2 or CS1|CS2
   */

  ret = start_single_command(hdr);
  if (ret == RET_OK
      && len != sizeof(ncp_hl_request_header_t) + sizeof(cs) + sizeof(short_addr))
  {
    ret = RET_INVALID_FORMAT;
  }
#ifdef ZB_CERT_HACKS
  if (!ZB_CERT_HACKS().allow_cbke_from_zc_ncp_dev)
  {
#endif
    if (ZB_IS_DEVICE_ZC())
    /*cstat !MISRAC2012-Rule-2.1_b */
    /** @mdr{00012,5} */
    {
      TRACE_MSG(TRACE_ERROR, "ZC can't start client-side CBKE", (FMT__0));
      ret = RET_INVALID_STATE;
    }
#ifdef ZB_CERT_HACKS
  }
#endif


  if (!ZB_JOINED())
  {
    TRACE_MSG(TRACE_ERROR, "can't start client-side CBKE when not joined", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if (!ZG->aps.authenticated)
  {
    TRACE_MSG(TRACE_ERROR, "can't start client-side CBKE when not authenticated", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if (!ncp_have_kec())
  {
    TRACE_MSG(TRACE_ERROR, "can't start CBKE when KEC is not enabled", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if (ret == RET_OK)
  {
    cs = *(zb_uint8_t*)(&hdr[1]);
    ZB_LETOH16(&short_addr, (zb_uint8_t*)(&hdr[1])+sizeof(cs));

    if ( (cs == 0U) || (cs > ZB_KEC_SUPPORTED_CRYPTO_ATTR))
    {
      ret = RET_INVALID_PARAMETER_1;
    }
  }
  if (ret == RET_OK)
  {
    ret = zb_buf_get_out_delayed_ext(se_send_key_est_match_desc_addr, short_addr, 0 );
  }

  if (ret == RET_OK)
  {
    ncp_set_kec_suite(cs);
#ifdef SNCP_MODE
    NCP_CTX.poll_cbke = ZB_TRUE;
#endif /* SNCP_MODE */
    SNCP_SWITCH_POLL_ON();
    txlen = NCP_RET_LATER;
  }
  else
  {
    hdr->tsn = stop_single_command(NCP_HL_SECUR_START_KE);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_START_KE, hdr->tsn, ret, 0);
  }

  return txlen;
}


static void secur_cbke_done(zb_ret_t status)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_START_KE,
                                          stop_single_command(NCP_HL_SECUR_START_KE),
                                          status, 0);
#ifdef SNCP_MODE
  NCP_CTX.poll_cbke = ZB_FALSE;
#endif /* SNCP_MODE */
  SNCP_SWITCH_POLL_OFF();
  TRACE_MSG(TRACE_TRANSPORT3, "secur_cbke_done status %d", (FMT__D, status));
  ncp_send_packet(rh, txlen);
}


static void secur_tc_cbke_ind(zb_address_ieee_ref_t addr_ref, zb_ret_t status)
{
  zb_ret_t ret;
  zb_ieee_addr_t ieee_address;
  zb_uint16_t short_address;
  zb_uint16_t txlen;
  ncp_hl_ind_header_t *ih;
  zb_uint8_t *p;

  /*
    Format:

    Ind header
    1b status category
    1b status code
    2b short address
    8b long address
  */
  zb_address_by_ref(ieee_address, &short_address, addr_ref);
  TRACE_MSG(TRACE_TRANSPORT3, "secur_tc_cbke_ind addr_ref %hd (addr 0x%x) status %d", (FMT__H_D_D, addr_ref, short_address, status));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_SECUR_CBKE_SRV_FINISHED_IND, 1U + 1U + sizeof(short_address) + sizeof(ieee_address));
  p = (zb_uint8_t*)&ih[1];
  ret = ERROR_GET_CATEGORY(status);
  *p = (zb_uint8_t)ret;
  p++;
  ret = ERROR_GET_CODE(status);
  *p = (zb_uint8_t)ret;
  p++;
  ZB_HTOLE16(p, &short_address);
  p++;
  p++;
  ZB_IEEE_ADDR_COPY(p, ieee_address);
  ncp_send_packet(ih, txlen);
}

#ifdef ZB_SE_KE_WHITELIST
static zb_uint16_t secur_add_to_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_ieee_addr_t addr;

  ret = start_single_command(hdr);

  if ((ret == RET_OK) &&
      (len != (sizeof(ncp_hl_request_header_t) + sizeof(zb_ieee_addr_t))))
  {
    ret = RET_INVALID_FORMAT;
  }

  if ((ret == RET_OK) &&
      (ZSE_CTXC().ke_whitelist.num_entries >= ZB_SE_KE_WHITELIST_MAX_SIZE))
  {
    ret = RET_END_OF_LIST;
  }

  if (ret == RET_OK)
  {
    ZB_MEMCPY(&addr, (zb_uint8_t *)&hdr[1], sizeof(zb_ieee_addr_t));
    zb_secur_ke_whitelist_add(addr);
  }

  hdr->tsn = stop_single_command(NCP_HL_SECUR_KE_WHITELIST_ADD);

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_KE_WHITELIST_ADD, hdr->tsn, ret, 0);
}

static zb_uint16_t secur_del_from_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_ieee_addr_t addr;


  ret = start_single_command(hdr);

  if ((ret == RET_OK) &&
      (len != (sizeof(ncp_hl_request_header_t) + sizeof(zb_ieee_addr_t))))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    ZB_MEMCPY(&addr, (zb_uint8_t *)&hdr[1], sizeof(zb_ieee_addr_t));
    zb_secur_ke_whitelist_del(addr);
  }

  hdr->tsn = stop_single_command(NCP_HL_SECUR_KE_WHITELIST_DEL);

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_KE_WHITELIST_DEL, hdr->tsn, ret, 0);
}

static zb_uint16_t secur_del_all_from_whitelist(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;

  ret = start_single_command(hdr);

  if ((ret == RET_OK) &&
      (len != (sizeof(ncp_hl_request_header_t))))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    zb_secur_ke_whitelist_del_all();
  }

  hdr->tsn = stop_single_command(NCP_HL_SECUR_KE_WHITELIST_DEL_ALL);

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_KE_WHITELIST_DEL_ALL, hdr->tsn, ret, 0);
}
#endif /* ZB_SE_KE_WHITELIST */

static zb_uint16_t secur_start_partner_lk(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t peer_addr;
  /*
    Parameters:
    2b - short address of device to establish a key with
   */
  ret = start_single_command(hdr);
  if (len != sizeof(ncp_hl_request_header_t) + sizeof(peer_addr))
  {
    /* There must be one parameter */
    ret = RET_INVALID_FORMAT;
  }
  if (ZB_IS_DEVICE_ZC())
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00012,6} */
  {
    TRACE_MSG(TRACE_ERROR, "ZC can't do partner LK establishment", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if ((ZB_JOINED() == 0)
      || !ZG->aps.authenticated)
  {
    TRACE_MSG(TRACE_ERROR, "can't start partner LK establishment when not joined & authenticated", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if (!ncp_have_kec())
  {
    TRACE_MSG(TRACE_ERROR, "can't start partner LK establishment KEC is not enabled", (FMT__0));
    ret = RET_INVALID_STATE;
  }
  if (ret == RET_OK)
  {
    ZB_LETOH16(&peer_addr, &hdr[1]);
    if (peer_addr == 0U)
    {
      TRACE_MSG(TRACE_ERROR, "Can't establish a partner LK to TC", (FMT__0));
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = zb_buf_get_out_delayed_ext(zb_se_start_aps_key_establishment, peer_addr, 0);
    if (ret == RET_OK)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "Starting partner LK establishment with 0x%x tsn 0x%x", (FMT__D_D, peer_addr, hdr->tsn));
      NCP_CTX.parnter_lk_in_progress = 1;
      return NCP_RET_LATER;
    }
  }

  stop_single_command_by_tsn(hdr->tsn);
  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_START_PARTNER_LK, hdr->tsn, ret, 0);
}

static zb_uint16_t secur_partner_lk_enable(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t enable;

  if (len != (sizeof(ncp_hl_request_header_t) + sizeof(zb_bool_t)))
  {
    ret = RET_INVALID_FORMAT;
  }

  enable = *(zb_uint8_t*)(&hdr[1]);

#ifdef SNCP_MODE
  if (ZB_U2B(enable))
  {
    SEC_CTX().accept_partner_lk = ZB_TRUE;
  }
  else
  {
    SEC_CTX().accept_partner_lk = ZB_FALSE;
  }
#else
  ZVUNUSED(enable);
#endif /* SNCP_MODE */

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_SECUR_PARTNER_LK_ENABLE, hdr->tsn, ret, 0);
}

zb_bool_t ncp_partner_lk_inprogress(void)
{
  return (zb_bool_t)NCP_CTX.parnter_lk_in_progress;
}


void ncp_hl_partner_lk_rsp(zb_ret_t status)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SECUR_START_PARTNER_LK,
                                          stop_single_command(NCP_HL_SECUR_START_PARTNER_LK),
                                          status, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_partner_lk_rsp status %d", (FMT__D, status));
  ncp_send_packet(rh, txlen);
  NCP_CTX.parnter_lk_in_progress = 0;
}


void ncp_hl_partner_lk_ind(zb_uint8_t *peer_addr)
{
  zb_uint16_t txlen;
  ncp_hl_ind_header_t *ih;

  /*
    Format:

    Ind header
    8b long peer address
  */
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_partner_lk_ind peer " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(peer_addr)));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_SECUR_PARTNER_LK_FINISHED_IND, sizeof(zb_ieee_addr_t));
  ZB_MEMCPY(&ih[1], peer_addr, sizeof(zb_ieee_addr_t));
  ncp_send_packet(ih, txlen);
}
#endif /* ZB_ENABLE_SE_MIN_CONFIG */


/*
ID = NCP_HL_SECUR_NWK_INITIATE_KEY_SWITCH_PROCEDURE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t secur_initiate_nwk_key_switch_procedure(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_initiate_nwk_key_switch_procedure", (FMT__0));

#ifdef ZB_COORDINATOR_ROLE
  if (len != sizeof(ncp_hl_request_header_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = RET_OK;
    zb_buf_get_out_delayed(zb_secur_nwk_key_switch_procedure);
  }
#else /* ZB_COORDINATOR_ROLE */
  ret = RET_NOT_IMPLEMENTED;
#endif /* ZB_COORDINATOR_ROLE */

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_initiate_nwk_key_switch_procedure", (FMT__0));
  return txlen;
}


/*
ID = NCP_HL_SECUR_GET_IC_LIST:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: start IC index
    body:
      1b: IC table size
      1b: start index
      1b: IC count in response
      [IC count]: IC table
        8b: device_address
        1b: IC type
        [8, 10, 14, 18]b: IC
*/
static void secur_get_ic_list_cb(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_SECUR_GET_IC_LIST);
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_secur_ic_get_list_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_list_resp_t);
  ncp_hl_response_header_t *hdr;
  zb_uindex_t ic_index;
  zb_aps_installcode_nvram_t *ic_table = (zb_aps_installcode_nvram_t *)zb_buf_begin(param);

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_get_ic_list_cb param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  ret = (zb_ret_t)resp->status;

  if (ret == RET_OK)
  {
    zb_size_t response_body_len = sizeof(zb_uint8_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t);

    for (ic_index = 0; ic_index < resp->ic_table_list_count; ic_index++)
    {
      zb_aps_installcode_nvram_t *ic_entry = &ic_table[ic_index];

      response_body_len += (sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t) +
            ZB_IC_SIZE_BY_TYPE(ic_entry->options));
    }

    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_GET_IC_LIST, tsn, ret, response_body_len);
  }

  if (ret == RET_OK)
  {
    ncp_hl_body_t body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u8(&body, resp->ic_table_entries);
    ncp_hl_body_put_u8(&body, resp->start_index);
    ncp_hl_body_put_u8(&body, resp->ic_table_list_count);

    for (ic_index = 0; ic_index < resp->ic_table_list_count; ic_index++)
    {
      zb_aps_installcode_nvram_t *ic_entry = &ic_table[ic_index];

      ncp_hl_body_put_u64addr(&body, ic_entry->device_address);
      ncp_hl_body_put_u8(&body, ic_entry->options);
      ncp_hl_body_put_array(&body, ic_entry->installcode, ZB_IC_SIZE_BY_TYPE(ic_entry->options));
    }
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_GET_IC_LIST, tsn, ret, 0);
  }

  ncp_send_packet(hdr, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_get_ic_list_cb", (FMT__0));
}


static zb_uint16_t secur_get_ic_list(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_uint8_t ic_start_index = 0;
  zb_uint_t expected = sizeof(ic_start_index);
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_bool_t cmd_is_started = ZB_FALSE;
  zb_uint8_t buf = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_get_ic_list", (FMT__0));

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
  ret = start_single_command(hdr);

  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    zb_secur_ic_get_list_req_t *param;

    param = ZB_BUF_GET_PARAM(buf, zb_secur_ic_get_list_req_t);
    param->response_cb = secur_get_ic_list_cb;

    ncp_hl_body_get_u8(&body, &param->start_index);

    zb_secur_ic_get_list_req(buf);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);

    if (cmd_is_started)
    {
      stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  ZVUNUSED(ic_start_index);
  ZVUNUSED(expected);
  ZVUNUSED(body);
  ZVUNUSED(cmd_is_started);
  ZVUNUSED(buf);

  ret = RET_NOT_IMPLEMENTED;

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_get_ic_list", (FMT__0));

  return txlen;
}


/*
ID = NCP_HL_SECUR_GET_IC_BY_IDX:
  Request:
    header:
      2b: ID
    body:
      1b: IC index
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: device_address
      1b: IC type
      [8, 10, 14, 18]b: IC
*/
static void secur_get_ic_by_idx_cb(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_SECUR_GET_IC_BY_IDX);
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_secur_ic_get_by_idx_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_get_by_idx_resp_t);
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_get_ic_by_idx_cb param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  ret = (zb_ret_t)resp->status;

  if (ret == RET_OK)
  {
    ncp_hl_body_t body;

    zb_size_t response_body_len = sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t) +
      ZB_IC_SIZE_BY_TYPE(resp->ic_type);

    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_GET_IC_BY_IDX, tsn, ret, response_body_len);
    body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u64addr(&body, resp->device_address);
    ncp_hl_body_put_u8(&body, resp->ic_type);
    ncp_hl_body_put_array(&body, resp->installcode, ZB_IC_SIZE_BY_TYPE(resp->ic_type));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_GET_IC_BY_IDX, tsn, ret, 0);
  }

  ncp_send_packet(hdr, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_get_ic_by_idx_cb", (FMT__0));
}


static zb_uint16_t secur_get_ic_by_idx(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_size_t expected = sizeof(zb_uint8_t);
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_bool_t cmd_is_started = ZB_FALSE;
  zb_uint8_t buf = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_get_ic_by_idx", (FMT__0));

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES

  ret = start_single_command(hdr);

  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    zb_secur_ic_get_by_idx_req_t *param;

    param = ZB_BUF_GET_PARAM(buf, zb_secur_ic_get_by_idx_req_t);
    param->response_cb = secur_get_ic_by_idx_cb;

    ncp_hl_body_get_u8(&body, &param->ic_index);

    zb_secur_ic_get_by_idx_req(buf);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);

    if (cmd_is_started)
    {
      stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  ZVUNUSED(expected);
  ZVUNUSED(body);
  ZVUNUSED(cmd_is_started);
  ZVUNUSED(buf);

  ret = RET_NOT_IMPLEMENTED;

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_get_ic_by_idx", (FMT__0));

  return txlen;
}


/*
ID = NCP_HL_SECUR_REMOVE_ALL_IC:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static void secur_remove_all_ic_cb(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_SECUR_REMOVE_ALL_IC);
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_secur_ic_remove_all_resp_t *resp = ZB_BUF_GET_PARAM(param, zb_secur_ic_remove_all_resp_t);
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_remove_all_ic_cb param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  ret = (zb_ret_t)resp->status;

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_SECUR_REMOVE_ALL_IC, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_remove_all_ic_cb", (FMT__0));
}

static zb_uint16_t secur_remove_all_ic(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_bool_t cmd_is_started = ZB_FALSE;
  zb_bufid_t buf = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> secur_remove_all_ic", (FMT__0));

#if defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES
  ret = start_single_command(hdr);

  if (len != sizeof(ncp_hl_request_header_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    zb_secur_ic_remove_all_req_t *param;

    param = ZB_BUF_GET_PARAM(buf, zb_secur_ic_remove_all_req_t);
    param->response_cb = secur_remove_all_ic_cb;

    zb_secur_ic_remove_all_req(buf);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);

    if (cmd_is_started)
    {
      stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

#else /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */
  ZVUNUSED(cmd_is_started);
  ZVUNUSED(buf);

  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
#endif /* defined ZB_COORDINATOR_ROLE && defined ZB_SECURITY_INSTALLCODES */

  TRACE_MSG(TRACE_TRANSPORT3, "<< secur_remove_all_ic", (FMT__0));

  return txlen;
}


/*
ID = NCP_HL_NWK_GET_IEEE_BY_SHORT:
  Request:
    header:
      2b: ID
    body:
      2b: short address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: ieee address
*/
static zb_uint16_t get_address_ieee_by_short(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_ieee_addr_t ieee_addr;
  zb_uint16_t short_addr;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < (sizeof(*hdr) + sizeof(zb_uint16_t)))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ZB_LETOH16(&short_addr, &hdr[1]);
    TRACE_MSG(TRACE_TRANSPORT3, "short_addr 0x%x", (FMT__D, short_addr));
    ret = zb_address_ieee_by_short(short_addr, ieee_addr);
    ZB_DUMP_IEEE_ADDR(ieee_addr);
  }
  if (RET_OK == ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_GET_IEEE_BY_SHORT, hdr->tsn, ret, sizeof(ieee_addr));
    ZB_MEMCPY(&rh[1], ieee_addr, sizeof(ieee_addr));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_GET_IEEE_BY_SHORT, hdr->tsn, ret, 0);
  }
  return txlen;
}

/*
ID = NCP_HL_NWK_GET_SHORT_BY_IEEE:
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      2b: short address
*/
static zb_uint16_t get_address_short_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t short_addr;
  zb_ieee_addr_t ieee_addr;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr) + sizeof(ieee_addr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ZB_MEMCPY(ieee_addr, &hdr[1], sizeof(ieee_addr));
    ZB_DUMP_IEEE_ADDR(ieee_addr);
    short_addr = zb_address_short_by_ieee(ieee_addr);
    TRACE_MSG(TRACE_TRANSPORT3, "short_addr 0x%x", (FMT__D, short_addr));
    if (ZB_UNKNOWN_SHORT_ADDR == short_addr)
    {
      ret = RET_NOT_FOUND;
    }
  }
  if (RET_OK == ret)
  {
    ZB_HTOLE16_ONPLACE(short_addr);
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_GET_SHORT_BY_IEEE, hdr->tsn, ret, sizeof(short_addr));
    ZB_MEMCPY(&rh[1], &short_addr, sizeof(short_addr));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_GET_SHORT_BY_IEEE, hdr->tsn, ret, 0);
  }
  return txlen;
}

static void convert_nbt_to_ncp(const zb_neighbor_tbl_ent_t *nbt, ncp_hl_response_neighbor_by_ieee_t *ncp_nbt)
{
  zb_uint16_t tmp16;
  zb_uint32_t tmp32;

  zb_address_ieee_by_ref(ncp_nbt->ieee_addr, nbt->u.base.addr_ref);
  ZB_DUMP_IEEE_ADDR(ncp_nbt->ieee_addr);

  tmp16 = zb_address_short_by_ieee(ncp_nbt->ieee_addr);
  ZB_HTOLE16(&ncp_nbt->short_addr, &tmp16);
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->short_addr 0x%x", (FMT__D, ncp_nbt->short_addr));

  ncp_nbt->device_type = nbt->device_type;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->device_type 0x%x", (FMT__D, ncp_nbt->device_type));

  ncp_nbt->rx_on_when_idle = nbt->rx_on_when_idle;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->rx_on_when_idle 0x%x", (FMT__D, ncp_nbt->rx_on_when_idle));

  ncp_nbt->ed_config = 0;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->ed_config 0x%x", (FMT__D, ncp_nbt->ed_config));

  tmp32 = nbt->u.base.time_to_expire;
  ZB_HTOLE32(&ncp_nbt->timeout_counter, &tmp32);
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->timeout_counter %d", (FMT__D, ncp_nbt->timeout_counter));

  tmp32 = zb_convert_timeout_value(nbt->u.base.nwk_ed_timeout);
  ZB_HTOLE32(&ncp_nbt->device_timeout, &tmp32);
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->device_timeout %d", (FMT__D, ncp_nbt->device_timeout));

  ncp_nbt->relationship = nbt->relationship;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->relationship 0x%x", (FMT__D, ncp_nbt->relationship));

  ncp_nbt->transmit_failure_cnt = nbt->transmit_failure_cnt;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->transmit_failure_cnt %d", (FMT__D, ncp_nbt->transmit_failure_cnt));

  ncp_nbt->lqi = nbt->lqi;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->lqi %d", (FMT__D, ncp_nbt->lqi));

#ifdef ZB_ROUTER_ROLE
  ncp_nbt->outgoing_cost = nbt->u.base.outgoing_cost;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->outgoing_cost %d", (FMT__D, ncp_nbt->outgoing_cost));

  ncp_nbt->age = nbt->u.base.age;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->age %d", (FMT__D, ncp_nbt->age));
#endif /* ZB_ROUTER_ROLE */

  ncp_nbt->keepalive_received = nbt->keepalive_received;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->keepalive_received %d", (FMT__D, ncp_nbt->keepalive_received));

  ncp_nbt->mac_iface_idx = nbt->mac_iface_idx;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_nbt->mac_iface_idx %d", (FMT__D, ncp_nbt->mac_iface_idx));
}

/*
ID = NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE:
  Request:
    header:
      2b: ID
    body:
      8b: ieee address
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: ieee address
      2b: short address
      1b: device type (0-zc, 1-zr, 2-zed)
      1b: rx on when idle (TRUE or FALSE)
      2b: end device configuration (bitmask)
      4b: timeout counter
      4b: device timeout
      1b: relationship: 0x00=neighbor is the parent
                        0x01=neighbor is a child
                        0x02=neighbor is a sibling
                        0x03=none of the above
                        0x04=previous child
                        0x05=unauthenticated child
      1b: transmit failure
      1b: lqi
      1b: outgoing cost
      1b: age
      1b: keepalive received
      1b: mac interface index
*/
static zb_uint16_t get_neighbor_by_ieee(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_ieee_addr_t ieee_addr;
  zb_neighbor_tbl_ent_t *nbt;
  ncp_hl_response_neighbor_by_ieee_t ncp_nbt;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr) + sizeof(ieee_addr))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    ZB_MEMCPY(ieee_addr, &hdr[1], sizeof(ieee_addr));
    ZB_DUMP_IEEE_ADDR(ieee_addr);
    ret = zb_nwk_neighbor_get_by_ieee(ieee_addr, &nbt);
  }

  if (RET_OK == ret)
  {
    convert_nbt_to_ncp(nbt, &ncp_nbt);

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE, hdr->tsn, ret, sizeof(ncp_nbt));
    ZB_MEMCPY(&rh[1], &ncp_nbt, sizeof(ncp_nbt));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_GET_NEIGHBOR_BY_IEEE, hdr->tsn, ret, 0);
  }

  return txlen;
}

/*
ID = NCP_HL_PIM_SET_LONG_POLL_INTERVAL:
  Request:
    header:
      2b: ID
    body:
      4b: long poll interval in range 0x04-0x6E0000 in quarterseconds
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_long_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint32_t long_interval = 0;
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr) + sizeof(long_interval))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ZB_LETOH32(&long_interval, &hdr[1]);
    if ((long_interval < ZB_PIM_MINIMUM_LONG_POLL_INTERVAL_IN_QS)
        || (long_interval > ZB_PIM_MAXIMUM_LONG_POLL_INTERVAL_IN_QS))
    {
      ret = RET_INVALID_PARAMETER_1;
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT3, "long_interval 0x%x", (FMT__D, long_interval));
      TRACE_MSG(TRACE_TRANSPORT3, "ZB_QUARTERECONDS_TO_MSEC(long_interval) 0x%x", (FMT__D, ZB_QUARTERECONDS_TO_MSEC(long_interval)));
      zb_zdo_pim_set_long_poll_interval(ZB_QUARTERECONDS_TO_MSEC(long_interval));
    }
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_SET_LONG_POLL_INTERVAL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_SET_FAST_POLL_INTERVAL:
  Request:
    header:
      2b: ID
    body:
      2b: fast poll interval in range 0x01-0xFFFF in quarterseconds
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_fast_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t fast_interval = 0;
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr) + sizeof(fast_interval))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ZB_LETOH16(&fast_interval, &hdr[1]);
    if ((fast_interval < ZB_PIM_MINIMUM_SHORT_POLL_INTERVAL_IN_QS)
#if ZB_PIM_MAXIMUM_SHORT_POLL_INTERVAL_IN_QS < 0xffff
        || (fast_interval > ZB_PIM_MAXIMUM_SHORT_POLL_INTERVAL_IN_QS)
#endif
      )
    {
      ret = RET_INVALID_PARAMETER_1;
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT3, "fast_interval 0x%x", (FMT__D, fast_interval));
      TRACE_MSG(TRACE_TRANSPORT3, "ZB_QUARTERECONDS_TO_MSEC(fast_interval) 0x%x", (FMT__D, ZB_QUARTERECONDS_TO_MSEC(fast_interval)));
      zb_zdo_pim_set_fast_poll_interval(ZB_QUARTERECONDS_TO_MSEC(fast_interval));
    }
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_SET_FAST_POLL_INTERVAL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_START_FAST_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t start_fast_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "start_fast_poll", (FMT__0));
    zb_zdo_pim_start_fast_poll(0);
    SET_POLL_STOPPED_FROM_HOST(ZB_FALSE);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_START_FAST_POLL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_STOP_FAST_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: stop result
*/
static zb_uint16_t stop_fast_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_bool_t is_in_fast_poll_before_stop;
  zb_bool_t is_in_fast_poll_after_stop;
  zb_uint8_t stop_result;

  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "stop_fast_poll", (FMT__0));

    is_in_fast_poll_before_stop = zb_zdo_pim_in_fast_poll();
    zb_zdo_pim_stop_fast_poll(0);
    /* DL: zb_zdo_pim_stop_fast_poll() can start ZDO polling? */
    SET_POLL_STOPPED_FROM_HOST(ZB_FALSE);
    is_in_fast_poll_after_stop = zb_zdo_pim_in_fast_poll();

    if (!is_in_fast_poll_before_stop)
    {
      stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_NOT_STARTED;
    }
    else if (is_in_fast_poll_after_stop)
    {
      stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_NOT_STOPPED;
    }
    else
    {
      stop_result = ZB_ZDO_PIM_STOP_FAST_POLL_RESULT_STOPPED;
    }
  }

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_PIM_STOP_FAST_POLL, hdr->tsn, ret, sizeof(stop_result));
  ZB_MEMCPY(&rh[1], &stop_result, sizeof(stop_result));

  return txlen;
}

/*
ID = NCP_HL_PIM_START_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t start_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "start_poll", (FMT__0));
    zb_zdo_pim_start_poll(0);
    SET_POLL_STOPPED_FROM_HOST(ZB_FALSE);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_START_POLL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_STOP_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t stop_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "stop_poll", (FMT__0));
    zb_zdo_pim_stop_poll(0);
    SET_POLL_STOPPED_FROM_HOST(ZB_TRUE);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_STOP_POLL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_ENABLE_TURBO_POLL:
  Request:
    header:
      2b: ID
    body:
      4b: turbo poll timeout in msec
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t  enable_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint32_t turbo_poll_timeout_ms = 0;
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr) + sizeof(turbo_poll_timeout_ms))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ZB_LETOH32(&turbo_poll_timeout_ms, &hdr[1]);

    TRACE_MSG(TRACE_TRANSPORT3, "enable turbo poll timeout_ms %d", (FMT__D, turbo_poll_timeout_ms));

    zb_zdo_pim_turbo_poll_continuous_leave(0);
    zb_zdo_pim_permit_turbo_poll(ZB_TRUE);
    zb_zdo_pim_start_turbo_poll_continuous(turbo_poll_timeout_ms);
    zb_zdo_pim_start_poll(0);
    SET_POLL_STOPPED_FROM_HOST(ZB_FALSE);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_ENABLE_TURBO_POLL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_DISABLE_TURBO_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t  disable_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "disable turbo poll", (FMT__0));
    zb_zdo_pim_permit_turbo_poll(ZB_FALSE);
    SET_POLL_STOPPED_FROM_HOST(ZB_FALSE);
  }

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_DISABLE_TURBO_POLL, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_PIM_START_TURBO_POLL_PACKETS:
  Request:
    header:
      2b: ID
    body:
      1b: count of packets
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t start_turbo_poll_packets(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint8_t packets_count;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(packets_count);

  TRACE_MSG(TRACE_TRANSPORT3, ">> start_turbo_poll_packets", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &packets_count);

    TRACE_MSG(TRACE_TRANSPORT3, "  start_turbo_poll_packets, packets_count %hd", (FMT__H, packets_count));

    TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets %d", (FMT__D, packets_count));
    zb_zdo_pim_start_turbo_poll_packets(packets_count);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< start_turbo_poll_packets, ret %hu", (FMT__D, ret));

  return txlen;
}

/*
ID = NCP_HL_PIM_TURBO_POLL_CANCEL_PACKET:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t turbo_poll_cancel_packet(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> turbo_poll_cancel_packet", (FMT__0));

  (void)len;

  zb_zdo_pim_turbo_poll_cancel_packet();

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< turbo_poll_cancel_packet, ret %hu", (FMT__D, ret));

  return txlen;
}

/*
ID = NCP_HL_PIM_START_TURBO_POLL_CONTINUOUS:
  Request:
    header:
      2b: ID
    body:
      4b: turbo poll timeout (ms)
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t start_turbo_poll_continuous(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint32_t timeout;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(timeout);

  TRACE_MSG(TRACE_TRANSPORT3, ">> start_turbo_poll_continuous", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u32(&body, &timeout);

    TRACE_MSG(TRACE_TRANSPORT3, "  start_turbo_poll_continuous, timeout %ld", (FMT__L, timeout));

    zb_zdo_pim_start_turbo_poll_continuous(timeout);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< start_turbo_poll_continuous, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_PIM_TURBO_POLL_CONTINUOUS_LEAVE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t turbo_poll_continuous_leave(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint16_t txlen;

  zb_size_t expected_len = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> turbo_poll_continuous_leave", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    zb_zdo_pim_turbo_poll_continuous_leave(0);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< turbo_poll_continuous_leave, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_PIM_TURBO_POLL_PACKETS_LEAVE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t turbo_poll_packets_leave(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint16_t txlen;

  zb_size_t expected_len = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">> turbo_poll_packets_leave", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    zb_zdo_turbo_poll_packets_leave(0);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< turbo_poll_packets_leave, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_PIM_PERMIT_TURBO_POLL:
  Request:
    header:
      2b: ID
    body:
      1b: turbo poll permit status
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t permit_turbo_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint8_t permit;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(permit);

  TRACE_MSG(TRACE_TRANSPORT3, ">> permit_turbo_poll", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &permit);

    TRACE_MSG(TRACE_TRANSPORT3, "  permit_turbo_poll, permit %hd", (FMT__H, permit));

    zb_zdo_pim_permit_turbo_poll(ZB_U2B(permit));
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< permit_turbo_poll, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_PIM_SET_FAST_POLL_TIMEOUT:
  Request:
    header:
      2b: ID
    body:
      4b: fast poll timeout
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_fast_poll_timeout(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint32_t timeout;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(timeout);

  TRACE_MSG(TRACE_TRANSPORT3, ">> set_fast_poll_timeout", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u32(&body, &timeout);

    TRACE_MSG(TRACE_TRANSPORT3, "  set_fast_poll_timeout, timeout %hd", (FMT__H, timeout));

    zb_zdo_pim_set_fast_poll_timeout(timeout);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< set_fast_poll_timeout, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_SET_KEEPALIVE_MODE:
  Request:
    header:
      2b: ID
    body:
      1b: keepalive mode
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_keepalive_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint8_t mode;
  zb_uint16_t txlen;

  zb_size_t expected_len = sizeof(mode);

  TRACE_MSG(TRACE_TRANSPORT3, ">> set_keepalive_mode", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &mode);

    TRACE_MSG(TRACE_TRANSPORT3, "  set_keepalive_mode, mode %hd", (FMT__H, mode));

    zb_set_keepalive_mode(mode);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< set_keepalive_mode, ret %hu", (FMT__D, ret));

  return txlen;
}


/*
ID = NCP_HL_START_CONCENTRATOR_MODE:
  Request:
    header:
      2b: ID
    body:
      1b: radius
      4b: time between discoveries
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t start_concentrator_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
#ifdef ZB_COORDINATOR_ROLE
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_uint8_t radius;
  zb_uint32_t disc_time;

  zb_size_t expected_len = sizeof(radius) + sizeof(disc_time);

  TRACE_MSG(TRACE_TRANSPORT3, ">> start_concentrator_mode", (FMT__0));

  ret = ncp_hl_body_check_len(&body, expected_len);

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &radius);
    ncp_hl_body_get_u32(&body, &disc_time);

    TRACE_MSG(TRACE_TRANSPORT3, "  radius %hd, disc_time %ld", (FMT__H_L, radius, disc_time));

    zb_start_concentrator_mode(radius, disc_time);
  }

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< start_concentrator_mode, ret %hu", (FMT__D, ret));
#else
  ZVUNUSED(len);

  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
#endif

  return txlen;
}


/*
ID = NCP_HL_STOP_CONCENTRATOR_MODE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t stop_concentrator_mode(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
#ifdef ZB_COORDINATOR_ROLE
  ZVUNUSED(len);

  ret = RET_OK;

  TRACE_MSG(TRACE_TRANSPORT3, ">> stop_concentrator_mode", (FMT__0));

  zb_stop_concentrator_mode();

  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

  TRACE_MSG(TRACE_TRANSPORT3, "<< stop_concentrator_mode, ret %hu", (FMT__D, ret));
#else
  ZVUNUSED(len);

  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);

#endif

  return txlen;
}


/*
ID = NCP_HL_PIM_GET_LONG_POLL_INTERVAL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      4b: long poll interval
*/
static zb_uint16_t get_long_poll_interval(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint32_t long_poll_interval;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    long_poll_interval = zb_zdo_pim_get_long_poll_ms_interval();
  }

  if (RET_OK == ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_PIM_GET_LONG_POLL_INTERVAL, hdr->tsn, ret, sizeof(long_poll_interval));
    ZB_MEMCPY(&rh[1], &long_poll_interval, sizeof(long_poll_interval));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_GET_LONG_POLL_INTERVAL, hdr->tsn, ret, 0);
  }

  return txlen;
}


/*
ID = NCP_HL_PIM_GET_IN_FAST_POLL_FLAG:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: in fast poll flag value
*/
static zb_uint16_t get_in_fast_poll_flag(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t in_fast_poll_status;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    in_fast_poll_status = ZB_B2U(zb_zdo_pim_in_fast_poll());
  }

  if (RET_OK == ret)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_PIM_GET_IN_FAST_POLL_FLAG, hdr->tsn, ret, sizeof(in_fast_poll_status));
    ZB_MEMCPY(&rh[1], &in_fast_poll_status, sizeof(in_fast_poll_status));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_PIM_GET_IN_FAST_POLL_FLAG, hdr->tsn, ret, 0);
  }

  return txlen;
}


/*
ID = NCP_HL_NWK_GET_FIRST_NBT_ENTRY:
  Request:
    5b: Common Request Header
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body: (if status code is RET_OK)
      8b: ieee address
      2b: short address
      1b: device type (0-zc, 1-zr, 2-zed)
      1b: rx on when idle (TRUE or FALSE)
      2b: end device configuration (bitmask)
      4b: timeout counter
      4b: device timeout
      1b: relationship: 0x00=neighbor is the parent
                        0x01=neighbor is a child
                        0x02=neighbor is a sibling
                        0x03=none of the above
                        0x04=previous child
                        0x05=unauthenticated child
      1b: transmit failure
      1b: lqi
      1b: outgoing cost
      1b: age
      1b: keepalive received
      1b: mac interface index
*/
static zb_uint16_t get_first_nbt_entry(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_response_neighbor_by_ieee_t ncp_nbt;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, "get_first_nbt_entry len %d", (FMT__D, len));

  if (len >= sizeof(*hdr))
  {
    ret = get_nbt_entry(&ncp_nbt, ZB_TRUE);
    if (RET_OK == ret)
    {
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_GET_FIRST_NBT_ENTRY, hdr->tsn, ret, sizeof(ncp_nbt));
      ZB_MEMCPY(&rh[1], &ncp_nbt, sizeof(ncp_nbt));
    }
    else
    {
      txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_GET_FIRST_NBT_ENTRY, hdr->tsn, ret, 0);
    }
  }
  else
  {
    txlen = 0;
  }

  return txlen;
}

/*
ID = NCP_HL_NWK_GET_NEXT_NBT_ENTRY:
  Request:
    5b: Common Request Header
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body: (if status code is RET_OK)
      8b: ieee address
      2b: short address
      1b: device type (0-zc, 1-zr, 2-zed)
      1b: rx on when idle (TRUE or FALSE)
      2b: end device configuration (bitmask)
      4b: timeout counter
      4b: device timeout
      1b: relationship: 0x00=neighbor is the parent
                        0x01=neighbor is a child
                        0x02=neighbor is a sibling
                        0x03=none of the above
                        0x04=previous child
                        0x05=unauthenticated child
      1b: transmit failure
      1b: lqi
      1b: outgoing cost
      1b: age
      1b: keepalive received
      1b: mac interface index
*/
static zb_uint16_t get_next_nbt_entry(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  ncp_hl_response_neighbor_by_ieee_t ncp_nbt;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, "get_next_nbt_entry len %d", (FMT__D, len));

  if (len >= sizeof(*hdr))
  {
    ret = get_nbt_entry(&ncp_nbt, ZB_FALSE);
    if (RET_OK == ret)
    {
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_NWK_GET_NEXT_NBT_ENTRY, hdr->tsn, ret, sizeof(ncp_nbt));
      ZB_MEMCPY(&rh[1], &ncp_nbt, sizeof(ncp_nbt));
    }
    else
    {
      txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_NWK_GET_NEXT_NBT_ENTRY, hdr->tsn, ret, 0);
    }
  }
  else
  {
    txlen = 0;
  }

  return txlen;
}

static zb_ret_t get_nbt_entry(ncp_hl_response_neighbor_by_ieee_t *ncp_nbt, zb_bool_t reset)
{
  static zb_uindex_t next_entry_n = 0;
  zb_uindex_t i;
  zb_ret_t ret = RET_OK;
  zb_neighbor_tbl_ent_t *nbt;

  if (reset)
  {
    next_entry_n = 0;
  }

  for(i = next_entry_n; i < ZB_NEIGHBOR_TABLE_SIZE; i++)
  {
    if (ZB_U2B(ZG->nwk.neighbor.neighbor[i].used))
    {
      next_entry_n = i + 1U;
      break;
    }
  }

  if (i < ZB_NEIGHBOR_TABLE_SIZE)
  {
    nbt = &ZG->nwk.neighbor.neighbor[i];

    convert_nbt_to_ncp(nbt, ncp_nbt);
  }
  else
  {
    ret = RET_NOT_FOUND;
  }

  return ret;
}

/*********************************ZDO****************************************/

static void ncp_zdo_util_clear_all_entries(zb_ncp_hl_zdo_entries_t *entries)
{
  zb_uint8_t i;

  ZB_ASSERT(entries);

  for(i = 0; i < ZDO_MAX_ENTRIES; i++)
  {
    ncp_zdo_util_clear_entry_by_idx(entries, i);
  }
}

static void ncp_zdo_util_clear_entry(zb_ncp_hl_zdo_entries_t *entry)
{
  ZB_ASSERT(entry != NULL);

  ZB_BZERO(entry, sizeof(*entry));
  entry->zb_tsn = ZB_ZDO_INVALID_TSN;
  entry->ncp_tsn = TSN_RESERVED;
}

static void ncp_zdo_util_clear_entry_by_idx(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t idx)
{
  ZB_ASSERT(entries);
  ZB_ASSERT(idx < ZDO_MAX_ENTRIES);

  ncp_zdo_util_clear_entry(&entries[idx]);
}


static zb_uint8_t ncp_zdo_util_get_idx_by_zb_tsn(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t zb_tsn)
{
  zb_uint8_t i;
  zb_uint8_t idx = ZDO_MAX_ENTRIES;

  ZB_ASSERT(entries);

  for(i = 0; i < ZDO_MAX_ENTRIES; i++)
  {
    if (entries[i].zb_tsn == zb_tsn)
    {
      idx = i;
      break;
    }
  }

  return idx;
}


static void ncp_zdo_util_handle_status_for_entry(zb_ncp_hl_zdo_entries_t *entry,
                                                 zb_zdp_status_t status)
{
  ZB_ASSERT(entry);

  if (!entry->wait_for_timeout
      || status == ZB_ZDP_STATUS_TIMEOUT)
  {
    ncp_zdo_util_clear_entry(entry);
  }
}


static zb_ret_t ncp_zdo_util_get_new_entry(zb_ncp_hl_zdo_entries_t *entries,
                                           zb_ncp_hl_zdo_entries_t **entry)
{
  zb_ret_t ret = RET_NO_MEMORY;
  zb_uint8_t free_elem_idx;

  ZB_ASSERT(entries != NULL);

  free_elem_idx = ncp_zdo_util_get_idx_by_zb_tsn(entries, ZB_ZDO_INVALID_TSN);
  if (ZDO_MAX_ENTRIES != free_elem_idx)
  {
    ret = RET_OK;
    *entry = entries + free_elem_idx;
  }

  return ret;
}

static void ncp_zdo_util_init_entry(zb_ncp_hl_zdo_entries_t *entry,
                                    zb_uint8_t zb_tsn,
                                    zb_uint8_t ncp_tsn,
                                    zb_bool_t wait_for_timeout)
{
  ZB_ASSERT(entry != NULL);

  entry->zb_tsn = zb_tsn;
  entry->ncp_tsn = ncp_tsn;
  entry->wait_for_timeout = wait_for_timeout;
}


static zb_ncp_hl_zdo_entries_t* ncp_zdo_util_get_entry_by_zb_tsn(zb_ncp_hl_zdo_entries_t *entries, zb_uint8_t zb_tsn)
{
  zb_uint8_t idx;
  zb_ncp_hl_zdo_entries_t* entry = NULL;

  ZB_ASSERT(entries);

  idx = ncp_zdo_util_get_idx_by_zb_tsn(entries, zb_tsn);
  if (idx < ZDO_MAX_ENTRIES)
  {
    entry = &entries[idx];
  }

  return entry;
}

/*
ID = NCP_HL_ZDO_NWK_ADDR_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: Dest Address
      8b: IEEE Address of Interest
      1b: Request Type
      1b: Start Index
*/
static zb_uint16_t nwk_addr_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint8_t)
                     + sizeof(zb_ieee_addr_t)
                     + sizeof(zb_uint8_t)
                     + sizeof(zb_uint8_t);
  zb_zdo_nwk_addr_req_param_t *param;
  zb_uint8_t buf;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nwk_addr_req, len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret != RET_OK)
  {
    buf = 0; /* buffer not allocated */
  }
  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf,
                             zb_zdo_nwk_addr_req_param_t);

    ncp_hl_body_get_u16(&body, &param->dst_addr);
    ncp_hl_body_get_u64addr(&body, param->ieee_addr);
    ncp_hl_body_get_u8(&body, &param->request_type);
    ncp_hl_body_get_u8(&body, &param->start_index);

    TRACE_MSG(TRACE_TRANSPORT3, "  nwk_addr_req, dst_addr 0x%x, request_type %hu, start_index %hu",
              (FMT__D_H_H, param->dst_addr, param->request_type, param->start_index));
    ZB_DUMP_IEEE_ADDR(param->ieee_addr);

    /* TODO:OB: Check IEEE addr for broadcast/unicast. */
    if (param->request_type > ZB_ZDO_EXTENDED_DEVICE_RESP)
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    if (param->request_type == ZB_ZDO_SINGLE_DEVICE_RESP)
    {
      tsn = zb_zdo_nwk_addr_req(buf, nwk_addr_rsp);
    }
    else
    {
      tsn = zb_zdo_nwk_addr_req(buf, nwk_addr_ext_rsp);
    }
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn, ZB_FALSE);
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<nwk_addr_req, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
ID = NCP_HL_ZDO_NWK_ADDR_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: IEEE Address of Remote Dev
      2b: NWK Address of Remote Dev
      0b/1b: Num Assoc Dev
      0b/1b: Start Index
      0b/512b: NWK Address Assoc Dev List
*/
static void nwk_addr_rsp_common(zb_uint8_t param, zb_bool_t is_ext)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret = RET_OK;
  zb_uint8_t ret_res_status;
  zb_uint8_t tsn;
  zb_size_t num_assoc_dev = 0U;
  zb_uint_t body_size;
  zb_uint16_t txlen;
  zb_size_t i;
  zb_zdo_nwk_addr_resp_head_t *resp;
  zb_zdo_nwk_addr_resp_ext_t *resp_ext;
  zb_zdo_nwk_addr_resp_ext2_t *resp_ext2;
  zb_uint8_t *assoc_dev_list;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>nwk_addr_rsp, param %hu, is ext %hu", (FMT__H_H, param, is_ext));

  ZB_ASSERT(param);

  resp = (zb_zdo_nwk_addr_resp_head_t*)zb_buf_begin(param);
  ret_res_status = resp->status;

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,9} */
  resp_ext = (zb_zdo_nwk_addr_resp_ext_t*)&resp[1];
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,10} */
  resp_ext2 = (zb_zdo_nwk_addr_resp_ext2_t*)&resp_ext[1];
  assoc_dev_list = (zb_uint8_t*)&resp_ext2[1];

  TRACE_MSG(TRACE_TRANSPORT3, "  nwk_addr_rsp, ZDO status 0x%hx", (FMT__H, resp->status));

  if (ret_res_status == ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_DUMP_IEEE_ADDR(resp->ieee_addr);
    TRACE_MSG(TRACE_TRANSPORT3, "  nwk_addr_rsp, nwk addr 0x%x", (FMT__D, resp->nwk_addr));

    body_size = sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t);
    if (is_ext)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "  nwk_addr_rsp, num assoc dev %hu", (FMT__H, resp_ext->num_assoc_dev));

      num_assoc_dev = resp_ext->num_assoc_dev;
      body_size += sizeof(zb_uint8_t);
      if (num_assoc_dev != 0U)
      {
        /* HACK:OB: Avoid breaking current NCP HL
         * packet body boundaries of 200 bytes. */
        if (num_assoc_dev > 64U)
        {
          num_assoc_dev = 64U;
        }
        body_size += sizeof(zb_uint8_t);
        body_size += num_assoc_dev * sizeof(zb_uint16_t);

      }
    }
  }
  else
  {
    body_size = 0;
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, ret_res_status);
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_NWK_ADDR_REQ, tsn, ret, body_size);

  if (ret == RET_OK)
  {
    ncp_hl_body_t body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u64addr(&body, resp->ieee_addr);
    ncp_hl_body_put_u16(&body, resp->nwk_addr);
    if (is_ext)
    {
      ncp_hl_body_put_u8(&body, resp_ext->num_assoc_dev);
      if (num_assoc_dev != 0U)
      {
        zb_uint16_t assoc_dev;
        ncp_hl_body_put_u8(&body, resp_ext2->start_index);
        for (i = 0; i < num_assoc_dev; ++i)
        {
          ZB_MEMCPY(&assoc_dev, assoc_dev_list, sizeof(assoc_dev));
          ncp_hl_body_put_u16(&body, assoc_dev);
          assoc_dev_list += sizeof(zb_int16_t);
        }
      }
    }
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<nwk_addr_rsp", (FMT__0));
}

static void nwk_addr_rsp(zb_uint8_t param)
{
  nwk_addr_rsp_common(param, ZB_FALSE);
}

static void nwk_addr_ext_rsp(zb_uint8_t param)
{
  nwk_addr_rsp_common(param, ZB_TRUE);
}

/*
ID = NCP_HL_ZDO_IEEE_ADDR_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: Dest Address
      2b: NWK Address of Interest
      1b: Reuqest Type
      1b: Start Index
*/
static zb_uint16_t ieee_addr_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t)
                     + sizeof(zb_uint16_t)
                     + sizeof(zb_uint8_t)
                     + sizeof(zb_uint8_t);
  zb_zdo_ieee_addr_req_param_t *param;
  zb_uint8_t buf = ZB_BUF_INVALID;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ieee_addr_req, len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf,
                             zb_zdo_ieee_addr_req_param_t);

    ncp_hl_body_get_u16(&body, &param->dst_addr);
    ncp_hl_body_get_u16(&body, &param->nwk_addr);
    ncp_hl_body_get_u8(&body, &param->request_type);
    ncp_hl_body_get_u8(&body, &param->start_index);

    TRACE_MSG(TRACE_TRANSPORT3, "  ieee_addr_req, dst_addr 0x%x, nwk_addr 0x%x, request_type %hu, start_index %hu",
              (FMT__D_D_H_H, param->dst_addr, param->nwk_addr, param->request_type, param->start_index));

    if ((ZB_NWK_IS_ADDRESS_BROADCAST(param->nwk_addr) &&
          param->nwk_addr != ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE) ||
        (param->request_type > ZB_ZDO_EXTENDED_DEVICE_RESP))
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    if (param->request_type == ZB_ZDO_SINGLE_DEVICE_RESP)
    {
      tsn = zb_zdo_ieee_addr_req(buf, ieee_addr_rsp);
    }
    else
    {
      tsn = zb_zdo_ieee_addr_req(buf, ieee_addr_ext_rsp);
    }
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn, ZB_FALSE);
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<ieee_addr_req, ret %d, txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
ID = NCP_HL_ZDO_IEEE_ADDR_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      8b: IEEE Address of Remote Dev
      2b: NWK Address of Remote Dev
      0b/1b: Num Assoc Dev
      0b/1b: Start Index
      0b/512b: NWK Address Assoc Dev List
*/
static void ieee_addr_rsp_common(zb_uint8_t param, zb_bool_t is_ext)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret = RET_OK;
  zb_ret_t ret_resp_status;
  zb_uint8_t tsn;
  zb_size_t num_assoc_dev = 0U;
  zb_uint_t body_size;
  zb_uint16_t txlen;
  zb_size_t i;
  zb_zdo_ieee_addr_resp_t *resp;
  zb_zdo_ieee_addr_resp_ext_t *resp_ext;
  zb_zdo_ieee_addr_resp_ext2_t *resp_ext2;
  zb_uint8_t *assoc_dev_list;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ieee_addr_rsp, param %hu, is ext %hu", (FMT__H_H, param, is_ext));

  ZB_ASSERT(param);

  resp = (zb_zdo_ieee_addr_resp_t*)zb_buf_begin(param);
  ret_resp_status = (zb_ret_t)resp->status;

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,11} */
  resp_ext = (zb_zdo_ieee_addr_resp_ext_t*)&resp[1];
  /*cstat !MISRAC2012-Rule-11.3 */
  /** @mdr{00002,12} */
  resp_ext2 = (zb_zdo_ieee_addr_resp_ext2_t*)&resp_ext[1];
  assoc_dev_list = (zb_uint8_t*)&resp_ext2[1];

  TRACE_MSG(TRACE_TRANSPORT3, "  ieee_addr_rsp, ZDO status 0x%hx", (FMT__H, resp->status));

  if (ret_resp_status == (zb_ret_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ZB_DUMP_IEEE_ADDR(resp->ieee_addr_remote_dev);
    TRACE_MSG(TRACE_TRANSPORT3, "  ieee_addr_rsp, nwk addr 0x%x", (FMT__D, resp->nwk_addr_remote_dev));

    body_size = sizeof(zb_ieee_addr_t) + sizeof(zb_uint16_t);
    if (is_ext)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "  ieee_addr_rsp, num assoc dev %hd", (FMT__H, resp_ext->num_assoc_dev));

      num_assoc_dev = resp_ext->num_assoc_dev;
      body_size += sizeof(zb_uint8_t);
      if (num_assoc_dev != 0U)
      {
        /* HACK:OB: Avoid breaking current NCP HL
         * packet body boundaries of 200 bytes. */
        if (num_assoc_dev > 64U)
        {
          num_assoc_dev = 64U;
        }
        body_size += sizeof(zb_uint8_t);
        body_size += num_assoc_dev * sizeof(zb_uint16_t);
      }
    }
  }
  else
  {
    body_size = 0;
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, ret_resp_status);
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_IEEE_ADDR_REQ, tsn, ret, body_size);

  if (ret == RET_OK)
  {
    ncp_hl_body_t body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u64addr(&body, resp->ieee_addr_remote_dev);
    ncp_hl_body_put_u16(&body, resp->nwk_addr_remote_dev);
    if (is_ext)
    {
      ncp_hl_body_put_u8(&body, resp_ext->num_assoc_dev);
      if (num_assoc_dev != 0U)
      {
        zb_uint16_t assoc_dev;
        ncp_hl_body_put_u8(&body, resp_ext2->start_index);
        for (i = 0; i < num_assoc_dev; ++i)
        {
          ZB_MEMCPY(&assoc_dev, assoc_dev_list, sizeof(assoc_dev));
          ncp_hl_body_put_u16(&body, assoc_dev);
          assoc_dev_list += sizeof(zb_int16_t);
        }
      }
    }
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<ieee_addr_rsp", (FMT__0));
}

static void ieee_addr_rsp(zb_uint8_t param)
{
  ieee_addr_rsp_common(param, ZB_FALSE);
}

static void ieee_addr_ext_rsp(zb_uint8_t param)
{
  ieee_addr_rsp_common(param, ZB_TRUE);
}

/*
ID = NCP_HL_ZDO_POWER_DESC_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: address of interest
*/
static zb_uint16_t power_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t);
  zb_zdo_power_desc_req_t *req;
  zb_uint8_t buf = 0;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>power_desc_req, len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    req = zb_buf_initial_alloc(buf,
        sizeof(zb_zdo_power_desc_req_t));

    /* 'nwk_addr' is a field inside a packed structure. 'req' is guaranteed to be aligned by 4 by
     * zb_buf_initial_alloc() implementation. If 'nwk_addr' field offset is aligned by 2 then it
     * is safe to access it. ncp_hl_body_get_u16() for now doesn't require value to be aligned but
     * be cautious in case of future changes.*/
    {
      ZB_ASSERT_VALUE_ALIGNED(ZB_OFFSETOF(zb_zdo_power_desc_req_t, nwk_addr), 2U);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,13} */
    ncp_hl_body_get_u16(&body, (zb_uint16_t *)&req->nwk_addr);

    if (ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr))
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zb_zdo_power_desc_req(buf, power_desc_rsp);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn, ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<power_desc_req, ret %d, txlen %hu", (FMT__D_H, ret, txlen));
  return txlen;
}

/*
ID = NCP_HL_ZDO_POWER_DESC_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      2b: power description flags
*/
static void power_desc_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t status;
  zb_size_t size;
  zb_uint16_t txlen;
  zb_zdo_power_desc_resp_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>power_desc_rsp, param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_power_desc_resp_t*)zb_buf_begin(param);
  TRACE_MSG(TRACE_TRANSPORT3, "  power_desc_rsp, ZDO status %hu", (FMT__H, resp->hdr.status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->hdr.tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  if (resp->hdr.status == ZB_ZDP_STATUS_SUCCESS)
  {
    status = RET_OK;
    size = sizeof(zb_af_node_power_desc_t) + sizeof(resp->hdr.nwk_addr);
  }
  else
  {
    status = ERROR_CODE(ERROR_CATEGORY_ZDO, (zb_ret_t)resp->hdr.status);
    size = 0;
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_POWER_DESC_REQ, tsn, status, size);

  if (status == RET_OK)
  {
    ncp_hl_body_t body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u16(&body, resp->power_desc.power_desc_flags);
    ncp_hl_body_put_u16(&body, resp->hdr.nwk_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "  power_desc_rsp, power_desc_flags 0x%hx", (FMT__H, resp->power_desc.power_desc_flags));
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->hdr.status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<power_desc_rsp", (FMT__0));
}

/*
ID = NCP_HL_ZDO_NODE_DESC_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: address of interest
*/
static zb_uint16_t node_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret = RET_OK;
  zb_uint16_t addr_of_interest;
  zb_uint8_t buf = 0;
  zb_zdo_node_desc_req_t *req;
  zb_uint16_t txlen;
  zb_uint8_t tsn;

  if (len < sizeof(*hdr) + sizeof(addr_of_interest))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (RET_OK == ret)
  {
    /* ZB_LETOH16 can handle unaligned pointers */
    ZB_LETOH16(&addr_of_interest, &hdr[1]);
    TRACE_MSG(TRACE_TRANSPORT3, "addr_of_interest 0x%x", (FMT__D, addr_of_interest));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_node_desc_req_t));
    req->nwk_addr = addr_of_interest;
    tsn = zb_zdo_node_desc_req(buf, node_desc_rsp);
    if (ZB_ZDO_INVALID_TSN == tsn)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn, ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_NODE_DESC_REQ, hdr->tsn, ret, 0);
  }

  return txlen;
}

/*
ID = NCP_HL_ZDO_NODE_DESC_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      2b: node description flags
      1b: MAC capability flags
      2b: Manufacturer code
      1b: Maximum buffer size
      2b: Maximum incoming transfer size
      2b: Server mask
      2b: Maximum outgoing transfer size
      1b: Descriptor capability field
*/
static void node_desc_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  void *resp;
  zb_zdo_node_desc_resp_t *node_desc_resp;
  zb_uint8_t status;
  ncp_hl_response_header_t *rh;
  zb_uint8_t tsn;
  zb_uint16_t txlen;
  zb_uint8_t *body;

  TRACE_MSG(TRACE_TRANSPORT3, ">>node_desc_rsp, param = %d", (FMT__D, (int)param));
  ZB_ASSERT(param);

  resp = zb_buf_begin(param);
  node_desc_resp = (zb_zdo_node_desc_resp_t*)resp;

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, node_desc_resp->hdr.tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  status = node_desc_resp->hdr.status;
  TRACE_MSG(TRACE_TRANSPORT3, "node_desc_rsp, ZDO status = %hd", (FMT__H, status));
  if (status != ZB_ZDP_STATUS_SUCCESS)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_NODE_DESC_REQ, tsn, ERROR_CODE(ERROR_CATEGORY_ZDO, status), 0);
    ZB_MEMCPY(&rh[1], &status, sizeof(status));
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.node_desc_flags %x", (FMT__D, node_desc_resp->node_desc.node_desc_flags));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.mac_capability_flags %x", (FMT__D, node_desc_resp->node_desc.mac_capability_flags));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.manufacturer_code %x", (FMT__D, node_desc_resp->node_desc.manufacturer_code));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.max_buf_size %d", (FMT__D, node_desc_resp->node_desc.max_buf_size));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.max_incoming_transfer_size %d", (FMT__D, node_desc_resp->node_desc.max_incoming_transfer_size));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.server_mask %x", (FMT__D, node_desc_resp->node_desc.server_mask));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.max_outgoing_transfer_size %d", (FMT__D, node_desc_resp->node_desc.max_outgoing_transfer_size));
    TRACE_MSG(TRACE_TRANSPORT3, "node_desc_resp->node_desc.desc_capability_field %x", (FMT__D, node_desc_resp->node_desc.desc_capability_field));

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_NODE_DESC_REQ, tsn, 0, sizeof(zb_af_node_desc_t) +
      sizeof(node_desc_resp->hdr.nwk_addr));
    body = (zb_uint8_t *)&rh[1];

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,14} */
    zb_copy_node_desc((zb_af_node_desc_t*)body, &node_desc_resp->node_desc);
    body += sizeof(zb_af_node_desc_t);

    /* ZB_HTOLE16 can handle unaligned pointers */
    ZB_HTOLE16(body, &node_desc_resp->hdr.nwk_addr);
  }

  ncp_send_packet(rh, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_APP2, "<< node_desc_rsp", (FMT__0));
}

/*
ID = NCP_HL_ZDO_SIMPLE_DESC_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: address_of_interest
      1b: endpoint
*/
static zb_uint16_t simple_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret = RET_OK;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint16_t addr_of_interest;
  zb_uint8_t ep;
  zb_uint8_t buf = 0;
  zb_zdo_simple_desc_req_t *req;
  zb_uint16_t txlen;
  zb_uint8_t zb_tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">>simple_desc_req len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(addr_of_interest) + sizeof(ep))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (RET_OK == ret)
  {
    ep = p[2];
    /* ep shall be 1-254 */
    if ((ep < 1U) || (ep > 254U))
    {
      ret = RET_INVALID_PARAMETER_2;
    }
    else
    {

      /* ZB_LETOH16 can handle unaligned pointers */
      ZB_LETOH16(&addr_of_interest, &hdr[1]);
      TRACE_MSG(TRACE_TRANSPORT3, "addr_of_interest 0x%x", (FMT__D, addr_of_interest));

      req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_simple_desc_req_t));
      req->nwk_addr = addr_of_interest;
      req->endpoint = ep;
      zb_tsn = zb_zdo_simple_desc_req(buf, simple_desc_rsp);
      if (ZB_ZDO_INVALID_TSN == zb_tsn)
      {
        ret = RET_ERROR;
      }
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, zb_tsn, hdr->tsn, ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_SIMPLE_DESC_REQ, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<simple_desc_req ret %d", (FMT__D, ret));

  return txlen;
}

/*
ID = NCP_HL_ZDO_SIMPLE_DESC_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: endpoint
      2b: app_profile_id
      2b: app_device_id
      1b: app_device_version
      1b: app_input_cluster_count
      1b: app_output_cluster_count
      2b*[app_input_cluster_count+app_output_cluster_count]: app_cluster_list
*/
static void simple_desc_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  void *resp;
  zb_zdo_simple_desc_resp_t *simple_desc_resp;
  zb_ret_t status;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">>simple_desc_rsp, param = %d", (FMT__D, (int)param));
  ZB_ASSERT(param);

  resp = zb_buf_begin(param);
  simple_desc_resp = (zb_zdo_simple_desc_resp_t*)resp;

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, simple_desc_resp->hdr.tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  status = (zb_ret_t)simple_desc_resp->hdr.status;
  TRACE_MSG(TRACE_TRANSPORT3, "simple_desc_rsp, ZDO status = %x", (FMT__H, status));
  if (status != ((zb_ret_t)ZB_ZDP_STATUS_SUCCESS))
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_SIMPLE_DESC_REQ, tsn, ERROR_CODE(ERROR_CATEGORY_ZDO, status), 0);
    ZB_MEMCPY(&rh[1], &status, sizeof(status));
  }
  else
  {
    zb_uint8_t n_clusters;
    zb_uindex_t j;
    zb_uint8_t *body;
    zb_af_simple_desc_1_1_t *simple_desc = &simple_desc_resp->simple_desc;
    zb_size_t simple_desc_size = sizeof(simple_desc->endpoint) + sizeof(simple_desc->app_profile_id) +
      sizeof(simple_desc->app_device_id) + sizeof((zb_uint8_t)simple_desc->app_device_version) +
      sizeof(simple_desc->app_input_cluster_count) + sizeof(simple_desc->app_output_cluster_count);

    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->endpoint %x", (FMT__D, simple_desc->endpoint));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->app_profile_id %x", (FMT__D, simple_desc->app_profile_id));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->app_device_id %x", (FMT__D, simple_desc->app_device_id));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->app_device_version %x", (FMT__D, simple_desc->app_device_version));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->app_input_cluster_count %x", (FMT__D, simple_desc->app_input_cluster_count));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc->app_output_cluster_count %x", (FMT__D, simple_desc->app_output_cluster_count));
    TRACE_MSG(TRACE_TRANSPORT3, "simple_desc_resp->hdr.nwk_addr %x", (FMT__D, simple_desc_resp->hdr.nwk_addr));

    n_clusters = simple_desc->app_input_cluster_count + simple_desc->app_output_cluster_count;

    for (j = 0; j < n_clusters; j++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "cluster %hd", (FMT__D, simple_desc->app_cluster_list[j]));
    }

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_SIMPLE_DESC_REQ, tsn, 0,
                                      simple_desc_size +
                                      /* take into account app_cluster_list */
                                      n_clusters * sizeof(zb_uint16_t) +
                                      sizeof(simple_desc_resp->hdr.nwk_addr));

    /* ZDO_CLUSTER_FILTERED_OUT isn't used in current NCP iteration, since
    all clusters, except KE, resides on Host side and implements by customer */
    body = (zb_uint8_t*)&rh[1];

    *body = simple_desc->endpoint;
    body += (zb_uint8_t)sizeof(simple_desc->endpoint);

    ZB_HTOLE16(body, &simple_desc->app_profile_id);
    body += (zb_uint8_t)sizeof(simple_desc->app_profile_id);

    ZB_HTOLE16(body, &simple_desc->app_device_id);
    body += (zb_uint8_t)sizeof(simple_desc->app_device_id);

    *body = simple_desc->app_device_version;
    body += (zb_uint8_t)sizeof((zb_uint8_t)simple_desc->app_device_version);

    *body = simple_desc->app_input_cluster_count;
    body += (zb_uint8_t)sizeof(simple_desc->app_input_cluster_count);

    *body = simple_desc->app_output_cluster_count;
    body += (zb_uint8_t)sizeof(simple_desc->app_output_cluster_count);

    for (j = 0; j < n_clusters; j++)
    {
      ZB_HTOLE16(body, &simple_desc->app_cluster_list[j]);
      body += sizeof(simple_desc->app_cluster_list[0]);
    }

    /* ZB_HTOLE16 can handle unaligned pointers */
    ZB_HTOLE16(body, &simple_desc_resp->hdr.nwk_addr);
  }

  ncp_send_packet(rh, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, (zb_zdp_status_t)status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< simple_desc_rsp", (FMT__0));
}

/*
ID = NCP_HL_ZDO_ACTIVE_EP_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: address of interest
*/
static zb_uint16_t active_ep_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret = RET_OK;
  zb_uint16_t addr_of_interest;
  zb_uint8_t buf = 0;
  zb_zdo_active_ep_req_t *req;
  zb_uint16_t txlen;
  zb_uint8_t zb_tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">>active_ep_desc_req len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(addr_of_interest))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (RET_OK == ret)
  {
    /* ZB_LETOH16 can handle unaligned pointers */
    ZB_LETOH16(&addr_of_interest, &hdr[1]);
    TRACE_MSG(TRACE_TRANSPORT3, "addr_of_interest 0x%x", (FMT__D, addr_of_interest));

    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_active_ep_req_t));
    req->nwk_addr = addr_of_interest;
    zb_tsn = zb_zdo_active_ep_req(buf, active_ep_desc_rsp);
    if (ZB_ZDO_INVALID_TSN == zb_tsn)
    {
      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, zb_tsn, hdr->tsn, ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_ACTIVE_EP_REQ, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<active_ep_desc_req ret %d", (FMT__D, ret));

  return txlen;
}

/*
ID = NCP_HL_ZDO_ACTIVE_EP_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: active ep count
      [n]: active ep list
*/
static void active_ep_desc_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  void *resp;
  zb_zdo_ep_resp_t *active_ep_desc_resp;
  zb_uint8_t *ep_list;
  zb_uint8_t status;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">>active_ep_desc_rsp, param = %d", (FMT__D, (int)param));
  ZB_ASSERT(param);

  resp = zb_buf_begin(param);
  active_ep_desc_resp = (zb_zdo_ep_resp_t*)resp;
  ep_list = (zb_uint8_t*)&active_ep_desc_resp[1];

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, active_ep_desc_resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  status = active_ep_desc_resp->status;
  TRACE_MSG(TRACE_TRANSPORT3, "active_ep_desc_resp, ZDO status = %x", (FMT__H, status));
  if (status != (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_ACTIVE_EP_REQ, tsn, ERROR_CODE(ERROR_CATEGORY_ZDO, (zb_ret_t)status), 0);
    ZB_MEMCPY(&rh[1], &status, sizeof(status));
  }
  else
  {
    zb_uint8_t *p;
    zb_uindex_t j;

    TRACE_MSG(TRACE_TRANSPORT3, "active_ep_desc_resp->ep_count %x", (FMT__D, active_ep_desc_resp->ep_count));
    for (j = 0; j < active_ep_desc_resp->ep_count; j++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "ep %hd", (FMT__D, ep_list[j]));
    }

    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_ACTIVE_EP_REQ, tsn, 0, sizeof(active_ep_desc_resp->ep_count) +
      active_ep_desc_resp->ep_count + sizeof(active_ep_desc_resp->nwk_addr));
    p = (zb_uint8_t*)&rh[1];
    *p = active_ep_desc_resp->ep_count;
    p++;
    for (j = 0; j < active_ep_desc_resp->ep_count; j++)
    {
      *p = ep_list[j];
      p++;
    }
    /* ZB_HTOLE16 can handle unaligned pointers */
    ZB_HTOLE16(p, &active_ep_desc_resp->nwk_addr);
  }

  ncp_send_packet(rh, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_APP2, "<< active_ep_desc_rsp", (FMT__0));
}

/*
ID = NCP_HL_ZDO_MATCH_DESC_REQ:
  Request:
    header:
      2b: ID
    body:
      2b: address of interest
      2b: profile ID
      1b: number of input clusters
      1b: number of output clusters
      2b*[number of input clusters + number of output clusters]: list of clusterIDs
*/
static zb_uint16_t match_desc_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  zb_uint16_t addr_of_interest;
  zb_uint16_t profile_id;
  zb_uint8_t num_in_clusters = 0;
  zb_uint8_t num_out_clusters = 0;
  zb_uint8_t buf = 0;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_zdo_match_desc_param_t *req;
  zb_uint16_t txlen;
  zb_uint8_t zb_tsn;

  if (len < (sizeof(*hdr) +
             sizeof(addr_of_interest) +
             sizeof(profile_id) +
             sizeof(num_in_clusters) +
             sizeof(zb_uint16_t)*num_in_clusters +
             sizeof(num_out_clusters) +
             sizeof(zb_uint16_t)*num_out_clusters))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = ncp_alloc_buf(&buf);
    if (ret == RET_OK)
    {
      ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
    }
    if (RET_OK == ret)
    {
      zb_uint_t num_in_out_clusters;
      zb_uindex_t j;

      /* ZB_LETOH16 can handle unaligned pointers */
      ZB_LETOH16(&addr_of_interest, p);
      TRACE_MSG(TRACE_TRANSPORT3, "addr_of_interest 0x%x", (FMT__D, addr_of_interest));

      p += sizeof(addr_of_interest);
      /* ZB_LETOH16 can handle unaligned pointers */
      ZB_LETOH16(&profile_id, p);
      TRACE_MSG(TRACE_TRANSPORT3, "profile_id 0x%x", (FMT__D, profile_id));

      p += sizeof(profile_id);
      num_in_clusters = *p;
      TRACE_MSG(TRACE_TRANSPORT3, "num_in_clusters %d", (FMT__D, num_in_clusters));

      p += sizeof(num_in_clusters);
      num_out_clusters = *p;
      TRACE_MSG(TRACE_TRANSPORT3, "num_out_clusters %d", (FMT__D, num_out_clusters));

      p += sizeof(num_out_clusters);

      num_in_out_clusters = ((zb_uint_t)num_in_clusters + (zb_uint_t)num_out_clusters);

      if (num_in_out_clusters != 0U)
      {
        /* one cluster already reserved in zb_zdo_match_desc_param_t declaration */
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (num_in_out_clusters - 1U) * sizeof(zb_uint16_t));
      }
      else
      {
        req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t));
      }

      TRACE_MSG(TRACE_TRANSPORT3, "in clusters:", (FMT__0));
      for(j = 0; j < num_in_clusters; j++)
      {
        ZB_MEMCPY(&req->cluster_list[j], p, sizeof(zb_uint16_t));
        p+=sizeof(zb_uint16_t);
        TRACE_MSG(TRACE_TRANSPORT3, "0x%x", (FMT__D, req->cluster_list[j]));
      }

      TRACE_MSG(TRACE_TRANSPORT3, "out clusters:", (FMT__0));
      for(j = num_in_clusters; j < num_in_out_clusters; j++)
      {
        ZB_MEMCPY(&req->cluster_list[j], p, sizeof(zb_uint16_t));
        p+=sizeof(zb_uint16_t);
        TRACE_MSG(TRACE_TRANSPORT3, "0x%x", (FMT__D, req->cluster_list[j]));
      }

      req->nwk_addr = addr_of_interest;
      req->addr_of_interest = addr_of_interest;
      req->profile_id = profile_id;
      req->num_in_clusters = num_in_clusters;
      req->num_out_clusters = num_out_clusters;

      zb_tsn = zb_zdo_match_desc_req(buf, match_desc_rsp);
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, zb_tsn, hdr->tsn, ZB_NWK_IS_ADDRESS_BROADCAST(req->nwk_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_MATCH_DESC_REQ, hdr->tsn, ret, 0);
  }

  return txlen;
}

/*
ID = NCP_HL_ZDO_MATCH_DESC_REQ:
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: match ep count
      [n]: match ep list
      2b: NWK addr of interest
*/
static void match_desc_rsp(zb_uint8_t param)
{
  void *resp;
  zb_zdo_match_desc_resp_t *match_desc_resp;
  zb_uint8_t *match_list;
  zb_ncp_hl_zdo_entries_t *entry;
  ncp_hl_response_header_t *rh;
  zb_ret_t status;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>match_desc_rsp, param = %d", (FMT__D, (int)param));
  ZB_ASSERT(param);

  resp = zb_buf_begin(param);
  match_desc_resp = (zb_zdo_match_desc_resp_t*)resp;
  match_list = (zb_uint8_t*)&match_desc_resp[1];

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, match_desc_resp->tsn);
  ZB_ASSERT(entry != NULL);

  if (NULL == entry)
  {
    TRACE_MSG(TRACE_ERROR, "Unknown zb tsn %d", (FMT__D, match_desc_resp->tsn));
  }
  else
  {
    status = (zb_ret_t)match_desc_resp->status;
    TRACE_MSG(TRACE_TRANSPORT3, "match_desc_resp, ZDO status = %x", (FMT__H, status));
    if (status != (zb_ret_t)ZB_ZDP_STATUS_SUCCESS)
    {
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_MATCH_DESC_REQ, entry->ncp_tsn, ERROR_CODE(ERROR_CATEGORY_ZDO, status), 0);
      ZB_MEMCPY(&rh[1], &status, sizeof(status));
    }
    else
    {
      zb_uint8_t *p;
      zb_uindex_t j;

      TRACE_MSG(TRACE_TRANSPORT3, "match_desc_resp->match_len %x", (FMT__D, match_desc_resp->match_len));
      for (j = 0; j < match_desc_resp->match_len; j++)
      {
        TRACE_MSG(TRACE_TRANSPORT3, "match %hd", (FMT__D, match_list[j]));
      }

      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_MATCH_DESC_REQ, entry->ncp_tsn, 0,
        sizeof(match_desc_resp->match_len) + match_desc_resp->match_len + sizeof(match_desc_resp->nwk_addr));

      p = (zb_uint8_t*)&rh[1];
      *p = match_desc_resp->match_len;
      p++;
      for (j = 0; j < match_desc_resp->match_len; j++)
      {
        *p = match_list[j];
        p++;
      }

      /* ZB_HTOLE16 can handle unaligned pointers */
      ZB_HTOLE16(p, &match_desc_resp->nwk_addr);
    }

    ncp_send_packet(rh, txlen);
    ncp_zdo_util_handle_status_for_entry(entry, (zb_zdp_status_t)status);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_APP2, "<< match_desc_rsp, txlen %d", (FMT__D, txlen));
}


static void zdo_system_srv_discovery_rsp(zb_uint8_t param)
{
  void *resp;
  zb_zdo_system_server_discovery_resp_t *system_srv_discovery_resp;
  zb_ncp_hl_zdo_entries_t *entry;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t status;

  TRACE_MSG(TRACE_TRANSPORT3, ">> zdo_system_srv_discovery_rsp, param = %d", (FMT__D, (int)param));
  ZB_ASSERT(param);

  resp = zb_buf_begin(param);
  system_srv_discovery_resp = (zb_zdo_system_server_discovery_resp_t*)resp;

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, system_srv_discovery_resp->tsn);
  ZB_ASSERT(entry != NULL);
  if (NULL == entry)
  {
    TRACE_MSG(TRACE_ERROR, "Unknown zb tsn %d", (FMT__D, system_srv_discovery_resp->tsn));
  }
  else
  {
    status = system_srv_discovery_resp->status;
    TRACE_MSG(TRACE_TRANSPORT3, "system_srv_discovery_resp, ZDO status = %x", (FMT__H, status));
    if (status != (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
    {
      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ, entry->ncp_tsn,
                                   ERROR_CODE(ERROR_CATEGORY_ZDO, (zb_ret_t)status), 0);
      ZB_MEMCPY(&rh[1], &status, sizeof(status));
    }
    else
    {
      zb_uint8_t *p;

      TRACE_MSG(TRACE_TRANSPORT3, "system_srv_discovery_resp->server_mask %x",
                (FMT__D, system_srv_discovery_resp->server_mask));

      txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ, entry->ncp_tsn, 0,
      sizeof(system_srv_discovery_resp->server_mask));

      p = (zb_uint8_t*)&rh[1];

      /* ZB_HTOLE16 can handle unaligned pointers */
      ZB_HTOLE16(p, &system_srv_discovery_resp->server_mask);
    }

    ncp_send_packet(rh, txlen);
    ncp_zdo_util_handle_status_for_entry(entry, status);
  }

  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< zdo_system_srv_discovery_rsp", (FMT__0));
}


static void zdo_set_node_desc_manuf_code_rsp(zb_ret_t status)
{
  zb_uint16_t txlen;
  zb_uint8_t tsn = stop_single_command(NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ);
  ncp_hl_response_header_t *rh;

  TRACE_MSG(TRACE_TRANSPORT3, ">> zdo_set_node_desc_manuf_code_rsp, status = %d", (FMT__D, status));

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ, tsn, status, 0);
  ncp_send_packet(rh, txlen);

  TRACE_MSG(TRACE_TRANSPORT3, "<< zdo_set_node_desc_manuf_code_rsp", (FMT__0));
}


/*
  ID = NCP_HL_ZDO_BIND_REQ:
  Request:
    5b: Common Request Header
    2b: Target NWK Address
    8b: Src IEEE Address
    1b: Src Endpoint
    2b: Cluster ID
    1b: Dst Address Mode
    8b: Dst IEEE/NWK Address
    1b: Dst Endpoint
*/
static zb_uint16_t bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t) +
                       sizeof(zb_ieee_addr_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_uint16_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_ieee_addr_t) +
                       sizeof(zb_uint8_t);
  zb_zdo_bind_req_param_t *param;
  zb_uint8_t buf;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>bind_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret != RET_OK)
  {
    buf = 0; /* buffer not allocated */
  }
  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);

    ncp_hl_body_get_u16(&body, &param->req_dst_addr);
    ncp_hl_body_get_u64addr(&body, param->src_address);
    ncp_hl_body_get_u8(&body, &param->src_endp);
    ncp_hl_body_get_u16(&body, &param->cluster_id);
    ncp_hl_body_get_u8(&body, &param->dst_addr_mode);
    if (param->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
    {
      /* param->dst_address.addr_short is packed union inside unpacked structure. It is safe to cast its
       * members discarding 'packed' attribute. */
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00002,16} */
      ncp_hl_body_get_u16(&body, (zb_uint16_t *)&param->dst_address.addr_short);
    }
    else if (param->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ncp_hl_body_get_u64addr(&body, param->dst_address.addr_long);
      ncp_hl_body_get_u8(&body, &param->dst_endp);
    }
    else
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zb_zdo_bind_req(buf, bind_rsp);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn,
                            ZB_NWK_IS_ADDRESS_BROADCAST(param->req_dst_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<bind_req ret %d txlen %hu", (FMT__D_H, ret, txlen));
  return txlen;
}

/*
  ID = NCP_HL_ZDO_BIND_REQ:
  Response:
    7b: Common Response Header
*/
static void bind_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret = RET_OK;
  zb_ret_t ret_resp_status;
  zb_uint16_t txlen;
  zb_zdo_bind_resp_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>bind_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);
  ret_resp_status = (zb_ret_t)resp->status;
  TRACE_MSG(TRACE_TRANSPORT3, "  bind_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  if (ret_resp_status != (zb_ret_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, ret_resp_status);
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_BIND_REQ, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<bind_rsp", (FMT__0));
}

/*
  ID = NCP_HL_ZDO_UNBIND_REQ:
  Request:
    5b: Common Request Header
    2b: Target NWK Address
    8b: Src IEEE Address
    1b: Src Endpoint
    2b: Cluster ID
    1b: Dst Address Mode
    8b: Dst IEEE/NWK Address
    1b: Dst Endpoint
*/
static zb_uint16_t unbind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t) +
                       sizeof(zb_ieee_addr_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_uint16_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_ieee_addr_t) +
                       sizeof(zb_uint8_t);
  zb_zdo_bind_req_param_t *param;
  zb_uint8_t buf;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>unbind_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret != RET_OK)
  {
    buf = 0; /* buffer not allocated */
  }
  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);

    ncp_hl_body_get_u16(&body, &param->req_dst_addr);
    ncp_hl_body_get_u64addr(&body, param->src_address);
    ncp_hl_body_get_u8(&body, &param->src_endp);
    ncp_hl_body_get_u16(&body, &param->cluster_id);
    ncp_hl_body_get_u8(&body, &param->dst_addr_mode);
    if (param->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT)
    {
      /* param->dst_address.addr_short is packed union inside unpacked structure. It is safe to cast its
       * members discarding 'packed' attribute. */
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00002,17} */
      ncp_hl_body_get_u16(&body, (zb_uint16_t *)(&param->dst_address.addr_short));
    }
    else if (param->dst_addr_mode == ZB_APS_ADDR_MODE_64_ENDP_PRESENT)
    {
      ncp_hl_body_get_u64addr(&body, param->dst_address.addr_long);
      ncp_hl_body_get_u8(&body, &param->dst_endp);
    }
    else
    {
      ret = RET_INVALID_PARAMETER;
    }
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zb_zdo_unbind_req(buf, unbind_rsp);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn,
                            ZB_NWK_IS_ADDRESS_BROADCAST(param->req_dst_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<unbind_req ret %d txlen %hu", (FMT__D_H, ret, txlen));
  return txlen;
}

/*
  ID = NCP_HL_ZDO_UNBIND_REQ:
  Response:
    7b: Common Response Header
*/
static void unbind_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret = RET_OK;
  zb_ret_t ret_resp_status;
  zb_uint16_t txlen;
  zb_zdo_bind_resp_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>unbind_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_bind_resp_t*)zb_buf_begin(param);
  ret_resp_status = (zb_ret_t)resp->status;
  TRACE_MSG(TRACE_TRANSPORT3, "  unbind_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  if (ret_resp_status != (zb_ret_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, ret_resp_status);
  }

  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_UNBIND_REQ, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<unbind_rsp", (FMT__0));
}

/*
  ID = NCP_HL_ZDO_MGMT_LEAVE_REQ:
  Request:
    5b: Common Request Header
    2b: Target NWK Address
    8b: Remote Dev IEEE Address
    1b: Leave Flags
          Bits 0:5 - Reserved
          Bit 6 - Remove Children
          Bit 7 - Rejoin
*/
static zb_uint16_t ncp_zdo_mgmt_leave_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t) +
                       sizeof(zb_ieee_addr_t) +
                       sizeof(zb_uint8_t);
  zb_uint8_t buf = 0;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ncp_zdo_mgmt_leave_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    zb_zdo_mgmt_leave_param_t *param;
    zb_uint8_t flags;

    param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);

    /* 'dst_addr' is a field inside a packed structure. 'param' is guaranteed to be aligned by 4 by
     * ZB_BUF_GET_PARAM() implementation. If 'dst_addr' field offset is aligned by 2 then it
     * is safe to access it. ncp_hl_body_get_u16() for now doesn't require value to be aligned but
     * be cautious in case of future changes. */
    {
      ZB_ASSERT_VALUE_ALIGNED(ZB_OFFSETOF(zb_zdo_mgmt_leave_param_t, dst_addr), 2U);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,18} */
    ncp_hl_body_get_u16(&body, (zb_uint16_t *)&param->dst_addr);
    ncp_hl_body_get_u64addr(&body, param->device_address);
    ncp_hl_body_get_u8(&body, &flags);

    param->reserved = 0U;
    param->remove_children = (zb_uint8_t)ZB_CHECK_BIT_IN_BIT_VECTOR(&flags, NCP_HL_ZDO_LEAVE_FLAG_REMOVE_CHILDREN);
    param->rejoin = (zb_uint8_t)ZB_CHECK_BIT_IN_BIT_VECTOR(&flags, NCP_HL_ZDO_LEAVE_FLAG_REJOIN);

    TRACE_MSG(TRACE_TRANSPORT3, " dst_addr 0x%x, flags 0x%hx", (FMT__D_H, param->dst_addr, flags));
    TRACE_MSG(TRACE_TRANSPORT3, " remove_children %hd, rejoin %hd", (FMT__H_H, param->remove_children, param->rejoin));
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zdo_mgmt_leave_req(buf, ncp_zdo_mgmt_leave_rsp);
    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }
  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn, ZB_FALSE);
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<ncp_zdo_mgmt_leave_req ret %d txlen %hu", (FMT__D_H, ret, txlen));
  return txlen;
}

/*
  ID = NCP_HL_ZDO_MGMT_LEAVE_REQ:
  Response:
    7b: Common Response Header
*/
static void ncp_zdo_mgmt_leave_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_zdo_mgmt_leave_res_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ncp_zdo_mgmt_leave_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_mgmt_leave_res_t *)zb_buf_begin(param);
  TRACE_MSG(TRACE_TRANSPORT3, "  ncp_zdo_mgmt_leave_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  ret = ncp_zdo_status_to_retcode(resp->status);
  txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_LEAVE_REQ, tsn, ret, 0);

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<ncp_zdo_mgmt_leave_rsp", (FMT__0));
}


/*
  ID = NCP_HL_ZDO_MGMT_BIND_REQ:
  Request:
    5b: Common Request Header
    2b: Target NWK Address
    1b: Start Entry Index
*/
static zb_uint16_t ncp_zdo_mgmt_bind_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_zdo_mgmt_bind_param_t *param;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t) +
                       sizeof(zb_uint8_t);
  zb_uint8_t buf = 0;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_bind_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_bind_param_t);

    ncp_hl_body_get_u16(&body, &param->dst_addr);
    ncp_hl_body_get_u8(&body, &param->start_index);
  }

  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zb_zdo_mgmt_bind_req(buf, ncp_zdo_mgmt_bind_rsp);

    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn,
                            ZB_NWK_IS_ADDRESS_BROADCAST(param->dst_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_bind_req ret %d txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
  ID = NCP_HL_ZDO_MGMT_BIND_REQ:
  Response:
    7b: Common Response Header
  Response:
    5b: Common response header
    1b: Total binding table entries count
    1b: Start entry index in Binding Table Response
    1b: Number of entries in Binding Table Response

    Number of entries * entry size:
      8b: source IEEE address
      1b: source endpoint
      2b: binding cluster id
      1b: destination address mode
      8b: destination address
      1b: destination endpoint
*/
static void ncp_zdo_mgmt_bind_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_zdo_mgmt_bind_resp_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_bind_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_mgmt_bind_resp_t *)zb_buf_begin(param);
  TRACE_MSG(TRACE_TRANSPORT3, "  ncp_zdo_mgmt_bind_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  ret = ncp_zdo_status_to_retcode(resp->status);

  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ncp_hl_body_t body;
    zb_zdo_binding_table_record_t* record;
    zb_uindex_t record_index;

    zb_size_t response_body_len = sizeof(zb_uint8_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t);
    response_body_len += resp->binding_table_list_count * (sizeof(zb_ieee_addr_t) +
      sizeof(zb_uint8_t) + sizeof(zb_uint16_t) + sizeof(zb_uint8_t) + sizeof(zb_ieee_addr_t) + sizeof(zb_uint8_t));

    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_BIND_REQ, tsn, ret, response_body_len);
    body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u8(&body, resp->binding_table_entries);
    ncp_hl_body_put_u8(&body, resp->start_index);
    ncp_hl_body_put_u8(&body, resp->binding_table_list_count);

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,19} */
    record = (zb_zdo_binding_table_record_t*) (resp + 1);

    for (record_index = 0; record_index < resp->binding_table_list_count; record_index++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "    >> binding_table_entry #%hd ", (FMT__H, record_index));

      TRACE_MSG(TRACE_TRANSPORT3, "      src_address: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(record->src_address)));
      TRACE_MSG(TRACE_TRANSPORT3, "      src_endp: %hd", (FMT__H, record->src_endp));
      TRACE_MSG(TRACE_TRANSPORT3, "      cluster_id: %d", (FMT__D, record->cluster_id));
      TRACE_MSG(TRACE_TRANSPORT3, "      dst_addr_mode: %hd", (FMT__H, record->dst_addr_mode));

      if (record->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
          || record->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
      {
        TRACE_MSG(TRACE_TRANSPORT3, "      dst_address: %d", (FMT__D, record->dst_address.addr_short));
      }
      else
      {
        TRACE_MSG(TRACE_TRANSPORT3, "      dst_address: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(record->dst_address.addr_long)));
      }

      TRACE_MSG(TRACE_TRANSPORT3, "      dst_endp: %hd", (FMT__H, record->dst_endp));

      TRACE_MSG(TRACE_TRANSPORT3, "    << binding_table_entry", (FMT__0));

      ncp_hl_body_put_u64addr(&body, record->src_address);
      ncp_hl_body_put_u8(&body, record->src_endp);
      ncp_hl_body_put_u16(&body, record->cluster_id);
      ncp_hl_body_put_u8(&body, record->dst_addr_mode);

      if (record->dst_addr_mode == ZB_APS_ADDR_MODE_16_GROUP_ENDP_NOT_PRESENT
          || record->dst_addr_mode == ZB_APS_ADDR_MODE_16_ENDP_PRESENT)
      {
        ncp_hl_body_put_u16(&body, record->dst_address.addr_short);
        ncp_hl_body_put_u32(&body, 0);
        ncp_hl_body_put_u16(&body, 0);
      }
      else
      {
        ncp_hl_body_put_u64addr(&body, record->dst_address.addr_long);
      }

      ncp_hl_body_put_u8(&body, record->dst_endp);

      record++;
    }

  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_BIND_REQ, tsn, ret, 0);
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_bind_rsp", (FMT__0));
}


/*
  ID = NCP_HL_ZDO_MGMT_LQI_REQ:
  Request:
    5b: Common Request Header
    2b: Target NWK Address
    1b: Start Entry Index
*/
static zb_uint16_t ncp_zdo_mgmt_lqi_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_zdo_mgmt_lqi_param_t *param;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint16_t) +
                       sizeof(zb_uint8_t);
  zb_uint8_t buf = 0;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_lqi_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_lqi_param_t);

    ncp_hl_body_get_u16(&body, &param->dst_addr);
    ncp_hl_body_get_u8(&body, &param->start_index);
  }

  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (ret == RET_OK)
  {
    tsn = zb_zdo_mgmt_lqi_req(buf, ncp_zdo_mgmt_lqi_rsp);

    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn,
                            ZB_NWK_IS_ADDRESS_BROADCAST(param->dst_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_lqi_req ret %d txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
  ID = NCP_HL_ZDO_MGMT_LQI_REQ:
  Response:
    5b: Common response header
    1b: Total neighbors table entries count
    1b: Start entry index in LQI Response
    1b: Number of entries in LQI Response

    Number of entries * entry size:
      8b: extended PAN id
      8b: neighbor IEEE address
      2b: neighbor NWK address
      1b: neighbor flags
      1b: permit join
      1b: depth
      1b: link quality
*/
static void ncp_zdo_mgmt_lqi_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_zdo_mgmt_lqi_resp_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_lqi_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_mgmt_lqi_resp_t *)zb_buf_begin(param);
  TRACE_MSG(TRACE_TRANSPORT3, "  ncp_zdo_mgmt_lqi_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  ret = ncp_zdo_status_to_retcode(resp->status);

  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ncp_hl_body_t body;
    zb_zdo_neighbor_table_record_t* record;
    zb_uindex_t record_index;

    zb_size_t response_body_len = sizeof(zb_uint8_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t);
    response_body_len += resp->neighbor_table_list_count * (sizeof(zb_ext_pan_id_t) + sizeof(zb_ieee_addr_t) +
      sizeof(zb_uint16_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t) + sizeof(zb_uint8_t));

    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_LQI_REQ, tsn, ret, response_body_len);
    body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u8(&body, resp->neighbor_table_entries);
    ncp_hl_body_put_u8(&body, resp->start_index);
    ncp_hl_body_put_u8(&body, resp->neighbor_table_list_count);

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,20} */
    record = (zb_zdo_neighbor_table_record_t*) (resp + 1);

    for (record_index = 0; record_index < resp->neighbor_table_list_count; record_index++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "    >> neighbor_table_entry #%hd ", (FMT__H, record_index));

      TRACE_MSG(TRACE_TRANSPORT3, "      ext_pan_id: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(record->ext_pan_id)));
      TRACE_MSG(TRACE_TRANSPORT3, "      ext_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(record->ext_addr)));
      TRACE_MSG(TRACE_TRANSPORT3, "      network_addr: %d", (FMT__D, record->network_addr));
      TRACE_MSG(TRACE_TRANSPORT3, "      type_flags: %hd", (FMT__H, record->type_flags));
      TRACE_MSG(TRACE_TRANSPORT3, "      permit_join: %hd", (FMT__H, record->permit_join));
      TRACE_MSG(TRACE_TRANSPORT3, "      depth: %hd", (FMT__H, record->depth));
      TRACE_MSG(TRACE_TRANSPORT3, "      lqi: %hd", (FMT__H, record->lqi));

      TRACE_MSG(TRACE_TRANSPORT3, "    << neighbor_table_entry", (FMT__0));

      ncp_hl_body_put_u64addr(&body, record->ext_pan_id);
      ncp_hl_body_put_u64addr(&body, record->ext_addr);
      ncp_hl_body_put_u16(&body, record->network_addr);
      ncp_hl_body_put_u8(&body, record->type_flags);
      ncp_hl_body_put_u8(&body, record->permit_join);
      ncp_hl_body_put_u8(&body, record->depth);
      ncp_hl_body_put_u8(&body, record->lqi);

      record++;
    }

  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_LQI_REQ, tsn, ret, 0);
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_lqi_rsp", (FMT__0));
}


/*
  ID = NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ:
  Request:
    5b: Common Request Header
    4b: scan channels
    1b: scan duration
    1b: scan count
    2b: manager addr
    2b: destination NWK addr
*/
static zb_uint16_t ncp_zdo_mgmt_nwk_update_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_zdo_mgmt_nwk_update_req_t *param;
  zb_ret_t ret;
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);
  zb_size_t expected = sizeof(zb_uint32_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_uint8_t) +
                       sizeof(zb_uint16_t) +
                       sizeof(zb_uint16_t);
  zb_uint8_t buf = 0;
  zb_uint8_t tsn;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_nwk_update_req len %hu", (FMT__H, len));

  ret = ncp_hl_body_check_len(&body, expected);

  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }

  if (ret == RET_OK)
  {
    param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_nwk_update_req_t);

    /* 'scan_channels' is field inside a packed structure (hdr is a field of param and scan_channels is a field of param->hdr).
     * 'param' is guaranteed to be aligned by 4 by * ZB_BUF_GET_PARAM() implementation. If 'hdr.scan_channels'
     * field offset is aligned by 4 then it is safe to access it. ncp_hl_body_get_u32() for now doesn't require value to be aligned but
     * be cautious in case of future changes. */
    {
      ZB_ASSERT_VALUE_ALIGNED(ZB_OFFSETOF(zb_zdo_mgmt_nwk_update_req_hdr_t, scan_channels), 4U);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,21} */
    ncp_hl_body_get_u32(&body, (zb_uint32_t *)&param->hdr.scan_channels);
    ncp_hl_body_get_u8(&body, &param->hdr.scan_duration);

    ncp_hl_body_get_u8(&body, &param->scan_count);

    /* 'manager_addr' is a field inside a packed structure. 'param' is guaranteed to be aligned by 4 by
     * zb_buf_initial_alloc() implementation. If 'manager_addr' field offset is aligned by 2 then it
     * is safe to access it. ncp_hl_body_get_u16() for now doesn't require value to be aligned but
     * be cautious in case of future changes.*/
    {
      ZB_ASSERT_VALUE_ALIGNED(ZB_OFFSETOF(zb_zdo_mgmt_nwk_update_req_t, manager_addr), 2U);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,22} */
    ncp_hl_body_get_u16(&body, (zb_uint16_t *)&param->manager_addr);

    /* 'dst_addr' is a field inside a packed structure. 'param' is guaranteed to be aligned by 4 by
     * zb_buf_initial_alloc() implementation. If 'dst_addr' field offset is aligned by 2 then it
     * is safe to access it. ncp_hl_body_get_u16() for now doesn't require value to be aligned but
     * be cautious in case of future changes.*/
    {
      ZB_ASSERT_VALUE_ALIGNED(ZB_OFFSETOF(zb_zdo_mgmt_nwk_update_req_t, dst_addr), 2U);
    }
    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00002,23} */
    ncp_hl_body_get_u16(&body, (zb_uint16_t *)&param->dst_addr);

    TRACE_MSG(TRACE_TRANSPORT3, "  >> zdo_mgmt_nwk_update_req", (FMT__0));
    TRACE_MSG(TRACE_TRANSPORT3, "      hdr.scan_channels: %ld", (FMT__L, param->hdr.scan_channels));
    TRACE_MSG(TRACE_TRANSPORT3, "      hdr.scan_duration: %hd", (FMT__H, param->hdr.scan_duration));
    TRACE_MSG(TRACE_TRANSPORT3, "      scan_count: %hd", (FMT__H, param->scan_count));
    TRACE_MSG(TRACE_TRANSPORT3, "      manager_addr: %d", (FMT__D, param->manager_addr));
    TRACE_MSG(TRACE_TRANSPORT3, "      dst_addr: %d", (FMT__D, param->dst_addr));
    TRACE_MSG(TRACE_TRANSPORT3, "  << zdo_mgmt_nwk_update_req", (FMT__0));
  }

  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }

  if (ret == RET_OK)
  {
    tsn = zb_zdo_mgmt_nwk_update_req(buf, ncp_zdo_mgmt_nwk_update_rsp);

    if (tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, tsn, hdr->tsn,
                            ZB_NWK_IS_ADDRESS_BROADCAST(param->dst_addr));
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);

    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_nwk_update_req ret %d txlen %hu", (FMT__D_H, ret, txlen));

  return txlen;
}

/*
  ID = NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ:
  Response:
    5b: Common response header
    4b: scanned channels
    2b: total transmissions count
    2b: transmission failures count
    1b: scanned channels list count
    1b * (scanned channels list count): energy values
*/
static void ncp_zdo_mgmt_nwk_update_rsp(zb_uint8_t param)
{
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint8_t tsn;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_zdo_mgmt_nwk_update_notify_hdr_t *resp;
  ncp_hl_response_header_t *hdr;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_zdo_mgmt_nwk_update_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);

  resp = (zb_zdo_mgmt_nwk_update_notify_hdr_t *)zb_buf_begin(param);
  TRACE_MSG(TRACE_TRANSPORT3, "  ncp_zdo_mgmt_nwk_update_rsp ZDO status %hu", (FMT__H, resp->status));

  entry = ncp_zdo_util_get_entry_by_zb_tsn(zdo_entries, resp->tsn);
  ZB_ASSERT(entry != NULL);
  tsn = entry->ncp_tsn;

  ret = ncp_zdo_status_to_retcode(resp->status);

  if (resp->status == (zb_uint8_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ncp_hl_body_t body;
    zb_uint8_t* record;
    zb_uindex_t record_index;

    zb_size_t response_body_len = sizeof(zb_uint32_t) + sizeof(zb_uint16_t) +
      sizeof(zb_uint16_t) + sizeof(zb_uint8_t);

    response_body_len += resp->scanned_channels_list_count * sizeof(zb_uint8_t);

    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ, tsn, ret, response_body_len);
    body = ncp_hl_response_body(hdr, txlen);

    ncp_hl_body_put_u32(&body, resp->scanned_channels);
    ncp_hl_body_put_u16(&body, resp->total_transmissions);
    ncp_hl_body_put_u16(&body, resp->transmission_failures);
    ncp_hl_body_put_u8(&body, resp->scanned_channels_list_count);

    record = (zb_uint8_t*) (resp + 1);

    for (record_index = 0; record_index < resp->scanned_channels_list_count; record_index++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "  ncp_zdo_mgmt_nwk_update_rsp: record - nwk_addr %hd", (FMT__H, *record));
      ncp_hl_body_put_u8(&body, *record);
      record++;
    }
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(&hdr, NCP_HL_ZDO_MGMT_NWK_UPDATE_REQ, tsn, ret, 0);
  }

  ncp_send_packet(hdr, txlen);
  ncp_zdo_util_handle_status_for_entry(entry, resp->status);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_zdo_mgmt_nwk_update_rsp", (FMT__0));
}


/*
  ID = NCP_HL_ZDO_PERMIT_JOINING_REQ:
  Request:
    5b: Common Request Header
    2b: Destination short address
    1b: PermitDuration (0x00-0xFE) in seconds
    1b: TC_Significance
*/
static zb_uint16_t zdo_permit_joining(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);
  zb_uint8_t duration;
  zb_uint16_t short_addr = 0;
  zb_bufid_t buf = 0;
  zb_zdo_mgmt_permit_joining_req_param_t *req_param;
  zb_uint16_t txlen;
  zb_uint8_t tc_significance;

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_permit_joining len %hu", (FMT__H, len));

  ret = start_single_command(hdr);
  if (RET_OK == ret)
  {
    if (len < sizeof(*hdr) + sizeof(short_addr) + sizeof(duration) + sizeof(tc_significance))
    {
      ret = RET_INVALID_FORMAT;
    }
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (RET_OK == ret)
  {
    zb_uint8_t zdo_tsn;
    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_permit_joining_req_param_t);

    ZB_LETOH16(&short_addr, p);
    p += sizeof(short_addr);

    duration = *p;
    p += sizeof(duration);

    tc_significance = *p;

    req_param->dest_addr = short_addr;
    req_param->permit_duration = duration;
    req_param->tc_significance = tc_significance;

    /* KLUDGE: cb is called differently and this depends on dest_addr parameter
               so we need to handle these two cases explicitly for now... */
    if (short_addr == ZB_PIBCACHE_NETWORK_ADDRESS())
    {
      /* zdo_tsn is always 0xFF here */
      (void)zb_zdo_mgmt_permit_joining_req(buf, zdo_permit_joining_rsp_local);
    }
    else
    {
      zdo_tsn = zb_zdo_mgmt_permit_joining_req(buf, zdo_permit_joining_rsp);
      if (zdo_tsn == ZB_ZDO_INVALID_TSN)
      {
        ret = RET_ERROR;
      }
    }
  }

  if (RET_OK == ret)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    hdr->tsn = stop_single_command(NCP_HL_ZDO_PERMIT_JOINING_REQ);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_PERMIT_JOINING_REQ, hdr->tsn, ret, 0);
  }

  return txlen;
}

/*
  ID = NCP_HL_ZDO_PERMIT_JOINING_REQ:
  Response:
    7b: Common Response Header

*/
static void zdo_permit_joining_rsp_local(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_ZDO_PERMIT_JOINING_REQ);
  zb_ret_t ret = RET_OK;
  zb_ret_t ret_resp_status;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_zdo_mgmt_permit_joining_resp_t *resp;

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_permit_joining_rsp_local param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  resp = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_permit_joining_resp_t);
  ret_resp_status = (zb_ret_t)resp->status;
  TRACE_MSG(TRACE_TRANSPORT3, "zdo_permit_joining_rsp ZDO status %hu", (FMT__H, resp->status));

  if (ret_resp_status != (zb_ret_t)ZB_ZDP_STATUS_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_ZDO, ret_resp_status);
  }

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_PERMIT_JOINING_REQ, tsn, ret, 0);

  ncp_send_packet(rh, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<zdo_permit_joining_rsp_local ret %d", (FMT__D, ret));
}

/*
  ID = NCP_HL_ZDO_PERMIT_JOINING_REQ:
  Response:
    7b: Common Response Header

*/
static void zdo_permit_joining_rsp(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_ZDO_PERMIT_JOINING_REQ);
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_permit_joining_rsp param %hu", (FMT__H, param));

  ZB_ASSERT(param);
  ZB_ASSERT(tsn != TSN_RESERVED);

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_PERMIT_JOINING_REQ, tsn, ret, 0);

  ncp_send_packet(rh, txlen);
  zb_buf_free(param);

  TRACE_MSG(TRACE_TRANSPORT3, "<<zdo_permit_joining_rsp ret %d", (FMT__D, ret));
}

/*
  ID = NCP_HL_ZDO_REJOIN:
  Request:
    5b: Common Request Header
    8b: Ext PAN ID
    channel list:
     1b: [n] entries number
     [n] 1b page 4b mask
    1b: secure rejoin option (0/1)
*/
static zb_uint16_t zdo_rejoin_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_ext_pan_id_t extpanid;
  zb_uint16_t len_rest = len - (zb_uint16_t)sizeof(*hdr);
  zb_bool_t secure_rejoin;
  zb_channel_list_t channels_list;
  zb_uint8_t n_scan_list_ent;
  zb_bufid_t buf = 0;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_rejoin_req len %hu", (FMT__H, len));

  ret = start_single_command(hdr);
  if (ret == RET_OK
      /*cstat !MISRAC2012-Rule-14.3_b */
      /** @mdr{00012,9} */
      && ZB_IS_DEVICE_ZC())
  /*cstat !MISRAC2012-Rule-2.1_b */
  /** @mdr{00012,4} */
  {
    ret = RET_INVALID_STATE;
  }
  /* ZB_JOINED is ZB_FALSE for ZED after reboot in SNCP and for all network roles in NCP.
     Is it intended? */
#if 0
  if (!ZB_JOINED())
  {
    ret = RET_INVALID_STATE;
  }
#endif
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK
      && len_rest < sizeof(zb_ext_pan_id_t))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    zb_uint8_t shift;

    ZB_EXTPANID_COPY(extpanid, p);
    p += sizeof(extpanid);

    len_rest -= ((zb_uint16_t)sizeof(extpanid));

    zb_channel_list_init(channels_list);
    ret = get_channels_list(len_rest, p, channels_list, &n_scan_list_ent, &shift);

    if (ret == RET_OK)
    {
      p += shift;
      len_rest -= shift;
    }
  }
  if (ret == RET_OK
      && len_rest < sizeof(secure_rejoin))
  {
    TRACE_MSG(TRACE_ERROR, "formation: too small packet len %d", (FMT__D, len));
    ret = RET_INVALID_FORMAT;
  }
  if (ret == RET_OK)
  {
    secure_rejoin = (*p != 0U) ? ZB_TRUE : ZB_FALSE;
    ret = zdo_initiate_rejoin(buf, extpanid, channels_list, secure_rejoin);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<zdo_rejoin_req ret %d", (FMT__D, ret));

  if (ret == RET_OK)
  {
    ZB_ASSERT(NCP_CTX.start_command == 0U);
    NCP_CTX.start_command = NCP_HL_ZDO_REJOIN;
    return NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    hdr->tsn = stop_single_command(NCP_HL_ZDO_REJOIN);
    return ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_REJOIN, hdr->tsn, ret, 0);
  }
}

#if defined ZDO_DIAGNOSTICS

static void zdo_get_stats_cb(zb_uint8_t buf)
{
  zb_ret_t ret = RET_OK;
  zb_ret_t ret_fullstats_status;
  zb_uint8_t tsn;
  zb_uint16_t txlen;
  zb_uint8_t *resp_body;
  ncp_hl_response_header_t *rh;
  zdo_diagnostics_full_stats_t *full_stats;

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_get_stats_cb(), buf %hd", (FMT__H, buf));

  tsn = stop_single_command(NCP_HL_ZDO_GET_STATS);
  full_stats = (zdo_diagnostics_full_stats_t *)zb_buf_begin(buf);
  ret_fullstats_status = (zb_ret_t)full_stats->status;

  if (ret_fullstats_status != (zb_ret_t)MAC_SUCCESS)
  {
    ret = ERROR_CODE(ERROR_CATEGORY_MAC, (zb_ret_t)full_stats->status);
  }

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_GET_STATS, tsn, ret, sizeof(zdo_diagnostics_full_stats_t));
  resp_body = (zb_uint8_t *)&rh[1];

  TRACE_MSG(TRACE_TRANSPORT3, "tsn %hd, full_stats->status 0x%hx, txlen %ld",
            (FMT__H_H_L, tsn, full_stats->status, txlen));

  if (ret_fullstats_status == (zb_ret_t)MAC_SUCCESS)
  {
    /* mac_stats */
    ZB_HTOLE32_ONPLACE(full_stats->mac_stats.mac_rx_bcast);
    ZB_HTOLE32_ONPLACE(full_stats->mac_stats.mac_tx_bcast);
    ZB_HTOLE32_ONPLACE(full_stats->mac_stats.mac_rx_ucast);

    ZB_HTOLE32_ONPLACE(full_stats->mac_stats.mac_tx_ucast_total_zcl);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_failures_zcl);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_retries_zcl);

    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_total);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_failures);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_tx_ucast_retries);

    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.phy_to_mac_que_lim_reached);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.mac_validate_drop_cnt);
    ZB_HTOLE16_ONPLACE(full_stats->mac_stats.phy_cca_fail_count);

    /* zdo_stats */
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.number_of_resets);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_tx_bcast);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_tx_ucast_success);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_tx_ucast_retry);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_tx_ucast_fail);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.route_disc_initiated);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_neighbor_added);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_neighbor_removed);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_neighbor_stale);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.join_indication);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.average_mac_retry_per_aps_message_sent);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.packet_buffer_allocate_failures);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.childs_removed);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_fc_failure);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_fc_failure);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_unauthorized_key);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_decrypt_failure);
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.aps_decrypt_failure);
#ifdef ZB_ENABLE_NWK_RETRANSMIT
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_retry_overflow);
#endif /* ZB_ENABLE_NWK_RETRANSMIT */
    ZB_HTOLE16_ONPLACE(full_stats->zdo_stats.nwk_bcast_table_full);
  }

  ZB_MEMCPY(resp_body, full_stats, sizeof(zdo_diagnostics_full_stats_t));

  ncp_send_packet(rh, txlen);
  zb_buf_free(buf);

  TRACE_MSG(TRACE_TRANSPORT3, "<<zdo_get_stats_cb()", (FMT__0));
}

static void zdo_get_stats_req_schedule(zb_uint8_t do_cleanup)
{
  zb_ret_t ret;
  zb_uint16_t get_stats_delay_ms = 250;

  if (ZB_U2B(do_cleanup))
  {
    ret = ZDO_DIAGNOSTICS_GET_AND_CLEANUP_STATS(zdo_get_stats_cb);
  }
  else
  {
    ret = ZDO_DIAGNOSTICS_GET_STATS(zdo_get_stats_cb);
  }

  if (ret == RET_BUSY)
  {
    TRACE_MSG(TRACE_TRANSPORT3, "  ZDO diagnostics context is busy, try to get stats after %d ms",
              (FMT__D, get_stats_delay_ms));

    ZB_SCHEDULE_ALARM(zdo_get_stats_req_schedule,
                      do_cleanup,
                      get_stats_delay_ms / 1000UL * ZB_TIME_ONE_SECOND);
  }
}

#endif /* ZDO_DIAGNOSTICS */

static zb_uint16_t zdo_get_stats_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

#if defined ZDO_DIAGNOSTICS
  zb_uint8_t do_cleanup;
  zb_size_t expected = sizeof(zb_uint8_t);
  ncp_hl_body_t body = ncp_hl_request_body(hdr, len);

  TRACE_MSG(TRACE_TRANSPORT3, ">>zdo_get_stats_req(), hdr 0x%p, len %hu",
            (FMT__P_H, hdr, len));

  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    ret = ncp_hl_body_check_len(&body, expected);
  }

  if (ret == RET_OK)
  {
    ncp_hl_body_get_u8(&body, &do_cleanup);
    TRACE_MSG(TRACE_TRANSPORT3, "  do_cleanup %hd", (FMT__H, do_cleanup));

    zdo_get_stats_req_schedule(do_cleanup);

    txlen = NCP_RET_LATER;
  }
  else
  {
    /* Note that buf can't be allocated there, so no need to free it */
    (void)stop_single_command(hdr->hdr.call_id);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<zdo_get_stats_req, txlen %ld", (FMT__L, txlen));
#else
  ret = RET_NOT_IMPLEMENTED;
  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_GET_STATS, hdr->tsn, ret, 0);
#endif /* ZDO_DIAGNOSTICS */

  return txlen;
}

/*
  ID = NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ:
  Request:
     2b: server mask
*/
static zb_uint16_t zdo_system_srv_discovery_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_ncp_hl_zdo_entries_t *entry;
  zb_uint16_t server_mask;
  zb_uint8_t buf = 0;
  zb_zdo_system_server_discovery_param_t *req_param;
  zb_uint16_t txlen;
  zb_uint8_t zdo_tsn;

  TRACE_MSG(TRACE_TRANSPORT3, ">> zdo_system_srv_discovery_req len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(server_mask))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    ret = ncp_zdo_util_get_new_entry(zdo_entries, &entry);
  }
  if (RET_OK == ret)
  {
    /* ZB_LETOH16 can handle unaligned pointers */
    ZB_LETOH16(&server_mask, &hdr[1]);
    TRACE_MSG(TRACE_TRANSPORT3, "server_mask 0x%x", (FMT__D, server_mask));

    req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_system_server_discovery_param_t);
    req_param->server_mask = server_mask;

    zdo_tsn = zb_zdo_system_server_discovery_req(buf, zdo_system_srv_discovery_rsp);
    if (zdo_tsn == ZB_ZDO_INVALID_TSN)
    {
      ret = RET_ERROR;
    }
  }

  if (RET_OK == ret)
  {
    ncp_zdo_util_init_entry(entry, zdo_tsn, hdr->tsn, ZB_FALSE);
    txlen = NCP_RET_LATER;
  }
  else
  {
    buf_free_if_valid(buf);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_SYSTEM_SRV_DISCOVERY_REQ, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< zdo_system_srv_discovery_req ret %d", (FMT__D, ret));

  return txlen;
}


/*
  ID = NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ:
  Request:
     2b: manuf_code
*/
static zb_uint16_t zdo_set_node_desc_manuf_code_req(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint16_t manuf_code;
  zb_uint16_t txlen;
  zb_bool_t cmd_is_started = ZB_FALSE;

  TRACE_MSG(TRACE_TRANSPORT3, ">> zdo_set_node_desc_manuf_code_req len %hu", (FMT__H, len));

  if (len != sizeof(*hdr) + sizeof(manuf_code))
  {
    ret = RET_INVALID_FORMAT;
  }

  if (ret == RET_OK)
  {
    ret = start_single_command(hdr);
    cmd_is_started = ZB_TRUE;
  }

  if (ret == RET_OK)
  {
    zb_uint16_t hdr_16;

    ZB_MEMCPY(&hdr_16, &hdr[1], sizeof(zb_uint16_t));
    ZB_LETOH16(&manuf_code, &hdr_16);
    TRACE_MSG(TRACE_TRANSPORT3, "manuf_code 0x%x", (FMT__D, manuf_code));

    zb_set_node_descriptor_manufacturer_code_req(manuf_code, zdo_set_node_desc_manuf_code_rsp);
  }

  if (ret == RET_OK)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    if (cmd_is_started)
    {
      (void)stop_single_command(hdr->hdr.call_id);
    }

    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_ZDO_SET_NODE_DESC_MANUF_CODE_REQ, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< zdo_set_node_desc_manuf_code_req, ret %d", (FMT__D, ret));

  return txlen;
}

zb_bool_t ncp_hl_is_manufacture_mode(void)
{
  return (NCP_CTX.mode == ZB_NCP_MODE_MANUFACTURE) ? ZB_TRUE : ZB_FALSE;
}

#ifdef ZB_NCP_ENABLE_MANUFACTURE_CMD

void ncp_hl_manuf_init_cont(zb_uint8_t param)
{
  zb_ncp_manuf_init_cont(param);
}

static inline zb_ret_t ncp_hl_manuf_req_check(const ncp_hl_request_header_t *hdr, zb_size_t len, zb_size_t expected)
{
  zb_ret_t ret = RET_OK;

  if (len != sizeof(*hdr) + expected)
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    if (NCP_CTX.mode != ZB_NCP_MODE_MANUFACTURE)
    {
      ret = RET_OPERATION_FAILED;
    }
  }
  if (RET_OK == ret)
  {
    if (!zb_ncp_manuf_check_request(hdr->hdr.call_id))
    {
      ret = RET_ILLEGAL_REQUEST;
    }
  }
  return ret;
}

static zb_ret_t manuf_mode_extract_page_channel(ncp_hl_request_header_t *hdr, zb_uint8_t *page, zb_uint32_t *channel)
{
  zb_ret_t ret;
  zb_uint8_t *body = (zb_uint8_t*)(&hdr[1]);

  *page = *body;
  body += sizeof(*page);

  ZB_LETOH32(channel, body);

  if ((*page == ZB_CHANNEL_PAGE0_2_4_GHZ ||
      ((*page >= ZB_CHANNEL_PAGE28_SUB_GHZ) && (*page <= ZB_CHANNEL_PAGE31_SUB_GHZ))))
  {

    if (((*page == ZB_CHANNEL_PAGE0_2_4_GHZ)
          && (*channel >= ZB_PAGE0_2_4_GHZ_CHANNEL_FROM)
          && (*channel <= ZB_PAGE0_2_4_GHZ_CHANNEL_TO))
        ||
        ((*page == ZB_CHANNEL_PAGE28_SUB_GHZ)
          /* && (*channel >= ZB_PAGE28_SUB_GHZ_CHANNEL_FROM) alway true */
          && (*channel <= ZB_PAGE28_SUB_GHZ_CHANNEL_TO))
        ||
        ((*page == ZB_CHANNEL_PAGE29_SUB_GHZ)
          && (*channel >= ZB_PAGE29_SUB_GHZ_CHANNEL_FROM)
          && (*channel <= ZB_PAGE29_SUB_GHZ_CHANNEL_TO))
        ||
        ((*page == ZB_CHANNEL_PAGE30_SUB_GHZ)
          && (*channel >= ZB_PAGE30_SUB_GHZ_CHANNEL_FROM)
          && (*channel <= ZB_PAGE30_SUB_GHZ_CHANNEL_TO))
        ||
        ((*page == ZB_CHANNEL_PAGE31_SUB_GHZ)
          /* && (*channel >= ZB_PAGE31_SUB_GHZ_CHANNEL_FROM) alway true */
          && (*channel <= ZB_PAGE31_SUB_GHZ_CHANNEL_TO))
       )
    {
      ret = RET_OK;
    }
    else
    {
      ret = RET_INVALID_PARAMETER_1;
    }
  }
  else
  {
    /* pages from 23 to 27 are not supported */
    ret = RET_INVALID_PARAMETER_2;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "manuf_mode_extract_page_channel page %hu channel %u ret %d", (FMT__H_L_D, *page, *channel, ret));
  return ret;
}

/*
  ID = NCP_HL_MANUF_MODE_START:
  Request:
    5b: Common Request Header
    1b: page # 0, 28-31
    4b: channel # depends on a page
  Response:
    7b: Common Response Header
*/
static zb_uint16_t manuf_mode_start(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret = RET_OK;
  zb_uint8_t page = 0U;
  zb_uint32_t channel = 0U;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_mode_start len %hu", (FMT__H, len));

  if (len < sizeof(*hdr) + sizeof(page) + sizeof(channel))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    if (!zb_ncp_manuf_check_request(hdr->hdr.call_id))
    {
      ret = RET_ILLEGAL_REQUEST;
    }
  }
  if (ret == RET_OK)
  {
    ret = manuf_mode_extract_page_channel(hdr, &page, &channel);
  }
  if (RET_OK == ret)
  {
    NCP_CTX.mode = ZB_NCP_MODE_MANUFACTURE;
    zb_ncp_manuf_init(page, (zb_uint8_t)channel);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_mode_start ret %d", (FMT__D, ret));

  return ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_MODE_START, hdr->tsn, ret, 0);
}

/*
  ID = NCP_HL_MANUF_MODE_END:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_mode_end(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_mode_end len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    schedule_reset(NO_OPTIONS);
  }

  if (RET_OK == ret)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_MODE_END, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_mode_end ret %d", (FMT__D, ret));

  return txlen;
}

/*
  ID = NCP_HL_MANUF_SET_PAGE_AND_CHANNEL:
  Request:
    5b: Common Request Header
    1b: page # 0, 28-31
    4b: channel # depends on a page
  Response:
    7b: Common Response Header
*/
static zb_uint16_t manuf_set_page_and_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint8_t page;
  zb_uint32_t channel;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_size_t expected = sizeof(page) + sizeof(channel);

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_set_page_and_channel len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, expected);
  if (RET_OK == ret)
  {
    ret = manuf_mode_extract_page_channel(hdr, &page, &channel);
  }

  if (RET_OK == ret)
  {
    ret = zb_ncp_manuf_set_page_and_channel(page, (zb_uint8_t)channel);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_set_page_and_channel ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_SET_PAGE_AND_CHANNEL, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
  Request:
    5b: Common Request Header
  Response:
    7b: Common Response Header
    1b: page # 0, 28-31
    4b: channel # depends on a page
*/
static zb_uint16_t manuf_get_page_and_channel(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint8_t page;
  zb_uint32_t channel;
  zb_ret_t ret;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *body;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_get_page_and_channel len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    page = zb_ncp_manuf_get_page();
    channel = zb_ncp_manuf_get_channel();
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_MANUF_GET_PAGE_AND_CHANNEL, hdr->tsn, ret, sizeof(page) + sizeof(channel));

    body = (zb_uint8_t*)&rh[1];
    *body = page;
    body += sizeof(page);
    ZB_HTOLE32(body, &channel);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_GET_PAGE_AND_CHANNEL, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_get_page_and_channel ret %d", (FMT__D, ret));

  return txlen;
}

/*
  ID = NCP_HL_MANUF_SET_POWER:
  Request:
    5b: Common Request Header
    1b: power
*/
static zb_uint16_t manuf_set_power(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_int8_t power;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_size_t expected = sizeof(power);

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_set_power len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, expected);
  if (RET_OK == ret)
  {
    power = *(zb_int8_t*)(&hdr[1]);
    zb_ncp_manuf_set_power(power);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_set_power ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_SET_POWER, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_GET_POWER:
  Request:
    5b: Common Request Header
  Response:
    7b: Common Response Header
    1b: power
*/
static zb_uint16_t manuf_get_power(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_get_power len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    zb_int8_t power = zb_ncp_manuf_get_power();
    txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_MANUF_GET_POWER, hdr->tsn, ret, sizeof(power));
    ZB_MEMCPY(&rh[1], &power, sizeof(power));
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_GET_POWER, hdr->tsn, ret, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_get_power ret %d", (FMT__D, ret));

  return txlen;
}

/*
  ID = NCP_HL_MANUF_START_TONE:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_start_tone(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_start_tone len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    ret = zb_ncp_manuf_start_tone();
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_start_tone ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_START_TONE, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_STOP_TONE:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_stop_tone(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_stop_tone len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    zb_ncp_manuf_stop_tone_or_stream();
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_stop_tone ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_STOP_TONE, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_START_STREAM_RANDOM:
  Request:
    5b: Common Request Header
    2b: random seed
*/
static zb_uint16_t manuf_start_stream(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t seed = 0xAAAA;
  zb_ret_t ret;
  zb_uint16_t txlen;
  zb_size_t expected = sizeof(seed);

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_start_stream len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, expected);
  if (RET_OK == ret)
  {
    ZB_LETOH16(&seed, (zb_uint8_t*)(&hdr[1]));
    ret = zb_ncp_manuf_start_stream(seed);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_start_stream ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_START_STREAM_RANDOM, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_STOP_STREAM_RANDOM:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_stop_stream(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_stop_stream len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    zb_ncp_manuf_stop_tone_or_stream();
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_stop_stream ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_STOP_STREAM_RANDOM, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_SEND_SINGLE_PACKET:
  Request:
    5b: Common Request Header
    1b: payload len
    [n]b: payload
*/
static zb_uint16_t manuf_send_packet(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint8_t payload_len = 0;
  zb_ret_t ret;
  zb_bufid_t buf;
  zb_uint16_t txlen;
  zb_uint8_t * p = (zb_uint8_t*)(&hdr[1]);

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_send_packet len %hu", (FMT__H, len));

  ret = start_single_command(hdr);
  if (ret == RET_OK)
  {
    zb_size_t expected;

    payload_len = *p;
    p++;
    expected = payload_len + sizeof(payload_len);
    ret = ncp_hl_manuf_req_check(hdr, (zb_size_t)len, expected);
  }
  if (payload_len > MAX_PHY_FRM_SIZE)
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    ret = ncp_alloc_buf_size(&buf, (zb_size_t)payload_len + 2U);

    if (ret == RET_OK)
    {
      /* Copy data to new location because:
       *   1. When sending packet, the original 'p' might be overwritten
       *   2. Create a two byte gap in front of the data for the SubGhz bands
       */
      zb_uint8_t *datap = zb_buf_initial_alloc(buf, (zb_size_t)payload_len + 2U);
      ZB_MEMCPY(&datap[2], p, payload_len);
      ret = zb_ncp_manuf_send_packet(buf, manuf_send_packet_rsp);
    }
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_send_packet ret %d", (FMT__D, ret));
      (void)stop_single_command(NCP_HL_MANUF_SEND_SINGLE_PACKET);
      txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_SEND_SINGLE_PACKET, hdr->tsn, ret, 0);
      return txlen;
    }
    return NCP_RET_LATER;
  }
  else
  {
    TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_send_packet ret %d", (FMT__D, ret));
    hdr->tsn = stop_single_command(NCP_HL_MANUF_SEND_SINGLE_PACKET);
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_SEND_SINGLE_PACKET, hdr->tsn, ret, 0);
    return txlen;
  }
}

/*
ID = NCP_HL_MANUF_SEND_SINGLE_PACKET:
  Response:
    5b: Common response header
*/
static void manuf_send_packet_rsp(zb_uint8_t param)
{
  zb_uint8_t tsn = stop_single_command(NCP_HL_MANUF_SEND_SINGLE_PACKET);
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  ZB_ASSERT(tsn != TSN_RESERVED);

  TRACE_MSG(TRACE_TRANSPORT3, "manuf_send_packet_rsp param %hu status %d", (FMT__H_D, param, zb_buf_get_status(param)));

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_MANUF_SEND_SINGLE_PACKET, tsn, zb_buf_get_status(param), 0);

  zb_buf_free(param);

  ncp_send_packet(rh, txlen);
}

/*
  ID = NCP_HL_MANUF_START_TEST_RX:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_start_rx(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_start_rx len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    ret = zb_ncp_manuf_start_rx(manuf_rx_packet_ind);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_start_rx ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_START_TEST_RX, hdr->tsn, ret, 0);

  return txlen;
}

/*
  ID = NCP_HL_MANUF_STOP_TEST_RX:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t manuf_stop_rx(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>manuf_stop_rx len %hu", (FMT__H, len));

  ret = ncp_hl_manuf_req_check(hdr, len, 0U);
  if (RET_OK == ret)
  {
    zb_ncp_manuf_stop_rx();
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_stop_rx ret %d", (FMT__D, ret));

  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_MANUF_STOP_TEST_RX, hdr->tsn, ret, 0);

  return txlen;
}

/*
ID = NCP_HL_MANUF_RX_PACKET_IND:
  Indication:
    2b: payload len
    1b: lqi
    1b: rssi
    [n]b: payload
*/
static void manuf_rx_packet_ind(zb_bufid_t buf)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint16_t data_length = (zb_uint16_t)zb_buf_len(buf) - 1U;
  zb_uint8_t *p;
  zb_uint8_t lqi;
  zb_uint8_t rssi;

  TRACE_MSG(TRACE_TRANSPORT3, "manuf_rx_packet_ind", (FMT__0));

  lqi = (zb_uint8_t)(*((zb_uint8_t*)zb_buf_begin(buf) + zb_buf_len(buf) - 2));
  rssi = (zb_uint8_t)(*((zb_int8_t*)zb_buf_begin(buf) + zb_buf_len(buf) - 2 + 1));

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_MANUF_RX_PACKET_IND, sizeof(data_length) + sizeof(lqi) + sizeof(rssi) + data_length);
  p = (zb_uint8_t *)&ih[1];
  ZB_HTOLE16(p, &data_length);
  p += sizeof(data_length);
  *p = lqi;
  p++;
  *p = rssi;
  p++;
  ZB_MEMCPY(p, (zb_uint8_t *)zb_buf_begin(buf), data_length);
  /* packet which received faster than 2 ms after previous may be lost */
  ncp_send_packet(ih, txlen);

  zb_buf_free(buf);
}

#endif /* ZB_NCP_ENABLE_MANUFACTURE_CMD */

#ifdef ZB_NCP_ENABLE_OTA_CMD
/*
  ID = NCP_HL_OTA_RUN_BOOTLOADER:
  Request:
    5b: Common Request Header
*/
static zb_uint16_t ota_run_bootloader(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>ota_run_bootloader len %hu", (FMT__H, len));

  ret = zb_osif_bootloader_run_after_reboot();

  schedule_reset(NO_OPTIONS);

  TRACE_MSG(TRACE_TRANSPORT3, "<<ota_run_bootloader ret %d", (FMT__D, ret));

  if (RET_OK == ret)
  {
    txlen = NCP_RET_LATER;
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_OTA_RUN_BOOTLOADER, hdr->tsn, ret, 0);
  }

  return txlen;
}
#endif /* ZB_NCP_ENABLE_OTA_CMD */

#if defined ZB_NCP_ENABLE_CUSTOM_COMMANDS && defined SNCP_MODE
static zb_uint16_t handle_ncp_req_custom_commands(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen = 0;

  TRACE_MSG(TRACE_TRANSPORT3, ">>handle_ncp_req_custom_commands, len %d", (FMT__D, len));

  switch(hdr->hdr.call_id)
  {
    case NCP_HL_READ_NVRAM_RESERVED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_READ_NVRAM_RESERVED", (FMT__0));
      txlen = read_nvram_reserved(hdr, len);
      break;
    case NCP_HL_WRITE_NVRAM_RESERVED:
      TRACE_MSG(TRACE_TRANSPORT3, "NCP_HL_WRITE_NVRAM_RESERVED", (FMT__0));
      txlen = write_nvram_reserved(hdr, len);;
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "handle_ncp_req_secur: unknown call_id %x", (FMT__H, hdr->hdr.call_id));
      break;
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<<handle_ncp_req_custom_commands, txlen %d", (FMT__D, txlen));
  return txlen;
}

/*
  ID = NCP_HL_READ_NVRAM_RESERVED:
  Request:
    it depends on platform
*/
zb_uint16_t read_nvram_reserved(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>read_nvram_reserved, len %hu", (FMT__H, len));

  TRACE_MSG(TRACE_TRANSPORT3, "command is not implemented! write your handler in platform!", (FMT__0));
  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_READ_NVRAM_RESERVED, hdr->tsn, RET_NOT_IMPLEMENTED, 0U);

  TRACE_MSG(TRACE_TRANSPORT3, "<<read_nvram_reserved", (FMT__0));

  return txlen;
}

/*
  ID = NCP_HL_WRITE_NVRAM_RESERVED:
  Request:
    it depends on platform
*/
zb_uint16_t write_nvram_reserved(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, ">>write_nvram_reserved, len %hu", (FMT__H, len));

  TRACE_MSG(TRACE_TRANSPORT3, "command is not implemented! write your handler in platform!", (FMT__0));
  txlen = ncp_hl_fill_resp_hdr(NULL, NCP_HL_WRITE_NVRAM_RESERVED, hdr->tsn, RET_NOT_IMPLEMENTED, 0U);

  TRACE_MSG(TRACE_TRANSPORT3, "<<write_nvram_reserved, ret 0x%lx", (FMT__L, ret));

  return txlen;
}
#endif /* ZB_NCP_ENABLE_CUSTOM_COMMANDS && SNCP_MODE */

/*
  ID = NCP_HL_BIG_PKT_TO_NCP:
  Request:
    5b: Common Request Header
    2b: pkt len
    [n]b: pkt
*/
static zb_uint16_t big_pkt_to_ncp(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint16_t pkt_len = 0;
  zb_ret_t ret = RET_OK;
  zb_uint16_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t *p = (zb_uint8_t*)(&hdr[1]);

  TRACE_MSG(TRACE_TRANSPORT3, ">>big_pkt_to_ncp len %hu", (FMT__H, len));

  ZB_LETOH16(&pkt_len, p);
  p += sizeof(pkt_len);

  if (len < sizeof(*hdr) + pkt_len + sizeof(pkt_len))
  {
    ret = RET_INVALID_FORMAT;
  }
  if (RET_OK == ret)
  {
    zb_uint8_t *p_rh;

    txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, ret, pkt_len + sizeof(pkt_len));
    p_rh = (zb_uint8_t*)(&rh[1]);
    ZB_HTOLE16(p_rh, &pkt_len);
    p_rh += sizeof(pkt_len);
    ZB_MEMCPY(p_rh, p, pkt_len);
  }
  else
  {
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<<manuf_send_packet ret %d", (FMT__D, ret));

  return txlen;
}

#ifdef SNCP_MODE
/*
ID = NCP_HL_PIM_SINGLE_POLL:
  Sheduled from ZDO zb_nlme_sync_confirm()
     status: MAC data request status: MAC_SUCCESS, MAC_NO_DATA, MAC_NO_ACK, etc
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static void single_poll_cb(zb_uint8_t status)
{
  zb_uint_t txlen;
  ncp_hl_response_header_t *rh;
  zb_uint8_t tsn = stop_single_command(NCP_HL_PIM_SINGLE_POLL);

  TRACE_MSG(TRACE_TRANSPORT3, "single_poll_cb status %hu", (FMT__H, status));

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_PIM_SINGLE_POLL, tsn, ERROR_CODE(ERROR_CATEGORY_MAC, (zb_ret_t)status), 0U);
  ncp_send_packet(rh, (zb_uint16_t)txlen);
}

/*
ID = NCP_HL_PIM_SINGLE_POLL:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t single_poll(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_bufid_t buf;
  zb_uint16_t txlen;
  zb_ret_t ret;

  TRACE_MSG(TRACE_TRANSPORT3, ">>single_poll len %hu", (FMT__H, len));

  if (len < sizeof(*hdr))
  {
    ret = RET_INVALID_FORMAT;
  }
  else if (!NCP_CTX.poll_stopped_from_host)
  {
    /* the request NCP_HL_PIM_STOP_POLL was not fulfilled before */
    ret = RET_INVALID_STATE;
  }
  else if (!ZB_JOINED())
  {
    ret = RET_INVALID_STATE;
  }
  else if (zb_zdo_pim_get_mode() != ZB_ZDO_PIM_STOP)
  {
    /* ZDO polling already running */
    ret = RET_ALREADY_EXISTS;
  }
  else
  {
    ret = start_single_command(hdr);
  }
  if (ret == RET_OK)
  {
    ret = ncp_alloc_buf(&buf);
  }
  if (ret == RET_OK)
  {
    zb_zdo_poll_parent_single(buf, single_poll_cb);
    txlen = NCP_RET_LATER;
  }
  else
  {
    stop_single_command_by_tsn(hdr->tsn);
    txlen = ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
  }
  TRACE_MSG(TRACE_TRANSPORT3, "<<single_poll ret %d", (FMT__D, ret));
  return txlen;
}
#endif

/*
ID = NCP_HL_GET_TX_POWER:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b: tx power in dBm
*/
static zb_uint16_t get_tx_power(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_int8_t tx_power;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_size_t rsp_body_size;

  ZVUNUSED(len);

  tx_power = zb_mac_get_tx_power();
  if (tx_power == ZB_MAC_TX_POWER_INVALID_DBM)
  {
    ret = RET_INVALID_STATE;
    rsp_body_size = 0U;
  }
  else
  {
    ret = RET_OK;
    rsp_body_size = sizeof(tx_power);
  }

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_GET_TX_POWER, hdr->tsn, ret, rsp_body_size);

  if (ret == RET_OK)
  {
    zb_int8_t *rsp_body = (zb_int8_t*)&rh[1];
    *rsp_body = tx_power;
  }

  return txlen;
}

/*
ID = NCP_HL_SET_TX_POWER:
  Request:
    header:
      2b: ID
      1b: tx power
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      1b or 0b: new tx power in dBm, 0b if error
*/
static zb_uint16_t set_tx_power(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_int8_t tx_power;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  if (len < sizeof(*hdr) + sizeof(tx_power))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    tx_power = *(zb_int8_t*)&hdr[1];
    zb_mac_set_tx_power(tx_power);
    ret = RET_OK;
  }

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_SET_TX_POWER, hdr->tsn, ret, ret == RET_OK ? sizeof(zb_int8_t) : 0U);

  if (ret == RET_OK)
  {
    zb_int8_t *rsp_body = (zb_int8_t*)&rh[1];
    *rsp_body = zb_mac_get_tx_power();
  }

  return txlen;
}

/*
ID = NCP_HL_SET_TRACE:
  Request:
    header:
      2b: ID
      4b: new trace bit mask:
        1 - wireless traffic
        2 - reserved (stack trace)
        4 - NCP LL protocol
        8 - interrupt line
        16 - sleep/awake
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
*/
static zb_uint16_t set_trace(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_ret_t ret;
  zb_uint32_t new_trace_mask;

  if (len < sizeof(*hdr) + sizeof(new_trace_mask))
  {
    ret = RET_INVALID_FORMAT;
  }
  else
  {
    ret = RET_OK;
    ZB_HTOLE32(&new_trace_mask, &hdr[1]);
    if ((new_trace_mask & NCP_TRACE_TRAFFIC) != 0U)
    {
      ZB_SET_TRAF_DUMP_ON();
    }
    else
    {
      ZB_SET_TRAF_DUMP_OFF();
    }
#ifdef ZB_HAVE_LL_TRACE
    /* Do not allow to turn off tracing at all, if it is on */
    zb_osif_set_ll_trace(new_trace_mask | (zb_osif_get_ll_trace() & NCP_TRACE_COMMON));
#endif
  }

  return ncp_hl_fill_resp_hdr(NULL, hdr->hdr.call_id, hdr->tsn, ret, 0);
}

/*
ID = NCP_HL_GET_TRACE:
  Request:
    header:
      2b: ID
  Response:
    header:
      2b: ID
      1b: status category
      1b: status code
    body:
      4b: current trace bit mask:
        1 - wireless traffic
        2 - reserved (stack trace)
        4 - NCP LL protocol
        8 - interrupt line
        16 - sleep/awake
*/
static zb_uint16_t get_trace(ncp_hl_request_header_t *hdr, zb_uint16_t len)
{
  zb_uint32_t trace_mask;
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;

  ZVUNUSED(len);

#ifdef ZB_TRAFFIC_DUMP_ON
  trace_mask = (ZB_GET_TRAF_DUMP_STATE() != 0U) ? NCP_TRACE_TRAFFIC : 0U;
#else
  /* When the traffic dump is disabled the ZB_GET_TRAF_DUMP_STATE() macro
   * evaluates to a constant value which leads to a MISRAC2012-Rule-14.3_b
   * violaton. As the macro is used only for NCP right now it is easier
   * to fix it with conditional compilation rather than creating separate
   * deviation record. */
  trace_mask = 0U;
#endif
#ifdef ZB_HAVE_LL_TRACE
  trace_mask |= zb_osif_get_ll_trace();
#endif

  TRACE_MSG(TRACE_TRANSPORT3, "get_trace trace_mask %u", (FMT__D, trace_mask));

  txlen = ncp_hl_fill_resp_hdr(&rh, hdr->hdr.call_id, hdr->tsn, RET_OK, sizeof(trace_mask));
  ZB_HTOLE32(&rh[1], &trace_mask);

  return txlen;
}

/*
   handle Leave signal
*/
void ncp_hl_nwk_leave_itself(zb_zdo_signal_leave_params_t *leave_params)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;
  zb_uint8_t* p;
  zb_bool_t rejoin = ZB_FALSE;

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_hl_nwk_leave_itself leave_type %d", (FMT__D, leave_params->leave_type));
  NCP_CTX.parent_lost = 0;

  switch (leave_params->leave_type)
  {
    case ZB_NWK_LEAVE_TYPE_RESET:
      rejoin = ZB_FALSE;
      break;

    case ZB_NWK_LEAVE_TYPE_REJOIN:
      rejoin = ZB_TRUE;
      break;

    default:
      ZB_ASSERT(ZB_FALSE);
      break;
  }

  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_NWK_LEAVE_IND, sizeof(zb_zdo_signal_leave_indication_params_t));
  p = (zb_uint8_t*)(&ih[1]);

  zb_get_long_address(p);
  ZB_DUMP_IEEE_ADDR(p);

  p += sizeof(zb_ieee_addr_t);
  *p = ZB_B2U(rejoin);
  TRACE_MSG(TRACE_TRANSPORT3, " rejoin %d", (FMT__D, *p));

  ncp_send_packet(ih, txlen);

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_hl_nwk_leave_itself txlen %hd", (FMT__H, txlen));
}

static void handle_parent_lost(zb_uint8_t param)
{
  ncp_hl_ind_header_t* ih = NULL;
  zb_uint16_t txlen;

  ZVUNUSED(param);

  TRACE_MSG(TRACE_TRANSPORT3, "handle_parent_lost", (FMT__0));

  NCP_CTX.parent_lost = 1;

  /* and send inication to Host */
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_PARENT_LOST_IND, 0);
  ncp_send_packet(ih, txlen);
}

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
void ncp_hl_tc_swapped_signal(zb_uint8_t param)
{
  ZVUNUSED(param);

  NCP_CTX.tc_swapped = ZB_TRUE;
}
#endif

static void ncp_hl_send_rejoin_resp(zb_ret_t status)
{
  ncp_hl_response_header_t *rh;
  zb_uint16_t txlen;
  zb_uint8_t* p;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_send_rejoin_resp status %d", (FMT__D, status));

  txlen = ncp_hl_fill_resp_hdr(&rh, NCP_HL_ZDO_REJOIN,
                               stop_single_command(NCP_HL_ZDO_REJOIN),
                               status, sizeof(zb_uint8_t) * 1U);
  p = (zb_uint8_t*)(&rh[1]);

#if defined TC_SWAPOUT && !defined ZB_COORDINATOR_ONLY
  *p = ZB_B2U(NCP_CTX.tc_swapped);
#else
  *p = 0U;
#endif

  ncp_send_packet(rh, txlen);
}

#ifdef ZB_HAVE_CALIBRATION
void ncp_perform_calibration(void)
{
  zb_ret_t ret;
  zb_uint32_t cal_value = 0U;

  ret = ncp_res_flash_read_by_id(ZBS_NCP_RESERV_CRYSTAL_CAL_ID, &cal_value);
  if (ret == RET_OK)
  {
    ret = zb_ti13xx_ajust_xosc_cap_array((zb_uint8_t)cal_value);
  }
  NCP_CTX.calibration_status = ret == RET_OK ? 1U : 0U;
  TRACE_MSG(TRACE_TRANSPORT3, "ncp_perform_calibration calibration_status %hd", (FMT__H, NCP_CTX.calibration_status));
}
#endif

#ifdef ZB_ZCL_SUPPORT_CLUSTER_SUBGHZ
void ncp_hl_subghz_duty_cycle_msg(zb_uint8_t time_mins)
{
  ncp_hl_ind_header_t *ih;
  zb_uint8_t *p;
  zb_uint16_t txlen;

  TRACE_MSG(TRACE_TRANSPORT3, "ncp_hl_subghz_duty_cycle_msg time_mins %hu", (FMT__H, time_mins));
  TRACE_MSG(TRACE_INFO1, "ncp_hl_subghz_duty_cycle_msg time_mins %hu", (FMT__H, time_mins));
  txlen = ncp_hl_fill_ind_hdr(&ih, NCP_HL_AF_SUBGHZ_SUSPEND_IND, sizeof(zb_uint8_t));
  p = (zb_uint8_t*)(&ih[1]);
  *p = time_mins;
  ncp_send_packet(ih, txlen);
}
#endif
