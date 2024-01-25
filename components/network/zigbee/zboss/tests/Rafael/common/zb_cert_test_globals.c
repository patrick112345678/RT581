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
/* PURPOSE: Definitions common to all R22 certification tests
*/

#define ZB_TRACE_FILE_ID 40100
#include "zb_common.h"
#include "zb_aps.h"
#include "zb_cert_test_globals.h"
#include "zb_mac_globals.h"

#ifndef ZB_MULTI_TEST
static zb_test_ctx_t s_test_ctx;
#endif

void zb_cert_test_set_init_globals(void)
{
#ifdef ZB_MULTI_TEST
  zb_test_ctx_t *test_ctx = zb_multitest_get_test_ctx();
#else
  zb_test_ctx_t *test_ctx = &s_test_ctx;
  ZB_BZERO(test_ctx, sizeof(test_ctx));
#endif

  test_ctx->page = ZB_CERT_TEST_DEAULT_PAGE;
  test_ctx->channel = ZB_CERT_TEST_DEAULT_CHANNEL;
  test_ctx->mode = ZB_CERT_TEST_DEAULT_MODE;
}


void zb_cert_test_set_common_channel_settings(void)
{
#ifndef NCP_MODE_HOST
  zb_channel_list_t channel_list;
  zb_channel_list_init(channel_list);
  zb_channel_list_add(channel_list, TEST_PAGE, (1L << TEST_CHANNEL));

  zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
#else
  /* TODO: use NCP API for this settings  */
#endif
}


void zb_cert_test_set_zc_role(void)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().aps_designated_coordinator = ZB_TRUE;
  ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
#endif
}


void zb_cert_test_set_zr_role(void)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().aps_designated_coordinator = ZB_FALSE;
  ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ROUTER;
#endif
}


void zb_cert_test_set_zed_role(void)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().aps_designated_coordinator = ZB_FALSE;
  ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;
#endif
}


void zb_cert_test_set_security_level(zb_uint8_t level)
{
#ifndef NCP_MODE_HOST
  ZB_SET_NIB_SECURITY_LEVEL(level);
#else
  ZVUNUSED(level);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint16_t zb_cert_test_get_network_addr()
{
#ifndef NCP_MODE_HOST
  return ZB_PIBCACHE_NETWORK_ADDRESS();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_set_network_addr(zb_uint16_t addr)
{
#ifndef NCP_MODE_HOST
  ZB_PIBCACHE_NETWORK_ADDRESS() = addr;
#else
  ZVUNUSED(addr);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint8_t zb_cert_test_get_nib_seq_number(void)
{
#ifndef NCP_MODE_HOST
  return ZB_NIB_SEQUENCE_NUMBER();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif

}


void zb_cert_test_inc_nib_seq_number(void)
{
#ifndef NCP_MODE_HOST
  ZB_NIB_SEQUENCE_NUMBER_INC();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


void zb_cert_test_set_nib_seq_number(zb_uint8_t seq_number)
{
#ifndef NCP_MODE_HOST
  ZB_NIB_SEQUENCE_NUMBER() = seq_number;
#else
  ZVUNUSED(seq_number);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint8_t zb_cert_test_get_zdo_tsn(void)
{
#ifndef NCP_MODE_HOST
  return ZDO_CTX().tsn;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


zb_uint8_t zb_cert_test_inc_zdo_tsn(void)
{
#ifndef NCP_MODE_HOST
  ZDO_CTX().tsn++;
  return ZDO_CTX().tsn;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


zb_uint8_t zb_cert_test_get_aps_counter(void)
{
#ifndef NCP_MODE_HOST
  return ZB_AIB_APS_COUNTER();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_inc_aps_counter(void)
{
#ifndef NCP_MODE_HOST
  ZB_AIB_APS_COUNTER_INC();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint16_t zb_cert_test_get_parent_short_addr(void)
{
#ifndef NCP_MODE_HOST
  zb_uint16_t addr;
  zb_address_short_by_ref(&addr, ZG->nwk.handle.parent);

  return addr;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_get_parent_ieee_addr(zb_ieee_addr_t addr)
{
#ifndef NCP_MODE_HOST
  zb_address_ieee_by_ref(addr, ZG->nwk.handle.parent);
#else
  ZVUNUSED(addr);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


void zb_cert_test_set_device_type(zb_nwk_device_type_t type)
{
#ifndef NCP_MODE_HOST
  ZB_NIB().device_type = type;
#else
  ZVUNUSED(type);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint8_t zb_cert_test_nib_get_max_depth(void)
{
#ifndef NCP_MODE_HOST
  return ZB_NIB().max_depth;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_set_keepalive_mode(nwk_keepalive_supported_method_t mode)
{
#ifndef NCP_MODE_HOST
  ZB_SET_KEEPALIVE_MODE(mode);
#else
  ZVUNUSED(mode);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

void zb_cert_test_set_permit_join_duration(zb_uint8_t permit_join_duration)
{
#ifndef NCP_MODE_HOST
  ZDO_CTX().conf_attr.permit_join_duration = permit_join_duration;
#else
  ZVUNUSED(permit_join_duration);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

//JJ
void zb_cert_test_set_nwk_state(zb_uint8_t state)
{
#ifndef NCP_MODE_HOST
  ZG->nwk.handle.state = state;
#else
  ZVUNUSED(state);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint8_t zb_cert_test_nib_get_update_id(void)
{
#ifndef NCP_MODE_HOST
  return ZB_NIB_UPDATE_ID();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_nib_set_disable_rejoin(zb_uint8_t disable_rejoin)
{
#ifndef NCP_MODE_HOST
  ZB_NIB().disable_rejoin = disable_rejoin;
#else
  ZVUNUSED(disable_rejoin);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


void zb_cert_test_nib_set_outgoing_frame_counter(zb_uint32_t frame_counter)
{
#ifndef NCP_MODE_HOST
  ZB_NIB().outgoing_frame_counter = frame_counter;
#else
  ZVUNUSED(frame_counter);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


void zb_cert_test_set_aib_always_rejoin(zb_uint8_t always_rejoin)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().always_rejoin = always_rejoin;
#else
  ZVUNUSED(always_rejoin);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


void zb_cert_test_set_aib_allow_tc_rejoins(zb_uint8_t allow_tc_rejoins)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().tcpolicy.allow_tc_rejoins = ZB_U2B(allow_tc_rejoins);
#else
  ZVUNUSED(allow_tc_rejoins);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}


zb_uint8_t zb_cert_test_get_mac_tsn(void)
{
#ifndef NCP_MODE_HOST
  return ZB_MAC_DSN();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
  return 0;
#endif
}


void zb_cert_test_inc_mac_tsn(void)
{
#ifndef NCP_MODE_HOST
  ZB_INC_MAC_DSN();
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

void zb_cert_test_set_aps_use_nvram(void)
{
#ifndef NCP_MODE_HOST
  ZB_AIB().aps_use_nvram = 1;
#else
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

#ifdef NCP_MODE_HOST
zb_uint8_t zb_zdo_mgmt_bind_req(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void fill_nldereq(zb_uint8_t param, zb_uint16_t addr, zb_uint8_t secure)
{
  ZVUNUSED(param);
  ZVUNUSED(addr);
  ZVUNUSED(secure);

  ZB_ASSERT(ZB_FALSE);
}

void zb_nlde_data_request(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_installcode_policy(zb_bool_t allow_ic_only)
{
  ZVUNUSED(allow_ic_only);

  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t zb_zdo_mgmt_lqi_req(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_nwk_change_me_addr(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void mac_add_invisible_short(zb_uint16_t addr)
{
  ZVUNUSED(addr);
  ZB_ASSERT(ZB_FALSE);
}

void mac_remove_invisible_short(zb_uint16_t addr)
{
  ZVUNUSED(addr);
  ZB_ASSERT(ZB_FALSE);
}

void mac_add_visible_device(zb_ieee_addr_t long_addr)
{
  ZVUNUSED(long_addr);
  ZB_ASSERT(ZB_FALSE);
}


void zb_nwk_set_address_assignment_cb(zb_addr_assignment_cb_t cb)
{
  ZVUNUSED(cb);
  ZB_ASSERT(ZB_FALSE);
}


void zb_secur_apsme_request_key(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}


void zb_tp_buffer_test_request(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);
  ZB_ASSERT(ZB_FALSE);
}


void zb_zdo_register_device_annce_cb(zb_device_annce_cb_t cb)
{
  ZVUNUSED(cb);
  ZB_ASSERT(ZB_FALSE);
}

void aps_data_hdr_fill_datareq(zb_uint8_t fc, zb_apsde_data_req_t *req, zb_bufid_t param)
{
  ZVUNUSED(fc);
  ZVUNUSED(req);
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_node_descriptor_manufacturer_code_req(zb_uint16_t manuf_code, zb_set_manufacturer_code_cb_t cb)
{
  ZVUNUSED(manuf_code);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);
}

void zb_zdo_pim_set_long_poll_interval(zb_time_t ms)
{
  ZVUNUSED(ms);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_use_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
  ZVUNUSED(ext_pan_id);
  ZB_ASSERT(ZB_FALSE);
}

/*void zb_get_use_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
  ZVUNUSED(ext_pan_id);
  ZB_ASSERT(ZB_FALSE);
}*/

void zb_set_input_cluster_id(zb_af_simple_desc_1_1_t *simple_desc, zb_uint8_t cluster_number, zb_uint16_t cluster_id)
{
  ZVUNUSED(simple_desc);
  ZVUNUSED(cluster_number);
  ZVUNUSED(cluster_id);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_output_cluster_id(zb_af_simple_desc_1_1_t *simple_desc, zb_uint8_t cluster_number, zb_uint16_t cluster_id)
{
  ZVUNUSED(simple_desc);
  ZVUNUSED(cluster_number);
  ZVUNUSED(cluster_id);

  ZB_ASSERT(ZB_FALSE);
}

zb_ret_t zb_add_simple_descriptor(zb_af_simple_desc_1_1_t *simple_desc)
{
  ZVUNUSED(simple_desc);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_set_simple_descriptor(zb_af_simple_desc_1_1_t *simple_desc,
                              zb_uint8_t  endpoint,
                              zb_uint16_t app_profile_id,
                              zb_uint16_t app_device_id,
                              zb_bitfield_t app_device_version,
                              zb_uint8_t app_input_cluster_count,
                              zb_uint8_t app_output_cluster_count)
{
  ZVUNUSED(simple_desc);
  ZVUNUSED(endpoint);
  ZVUNUSED(app_profile_id);
  ZVUNUSED(app_device_id);
  ZVUNUSED(app_device_version);
  ZVUNUSED(app_input_cluster_count);
  ZVUNUSED(app_output_cluster_count);

  ZB_ASSERT(ZB_FALSE);
}

zb_ret_t zb_nwk_neighbor_get_by_ieee(zb_ieee_addr_t long_addr, zb_neighbor_tbl_ent_t **nbt)
{
  ZVUNUSED(long_addr);
  ZVUNUSED(nbt);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_zdo_set_aps_unsecure_join(zb_bool_t insecure_join)
{
  ZVUNUSED(insecure_join);
  ZB_ASSERT(ZB_FALSE);
}

void zb_nwk_forward(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t zb_zdo_nwk_addr_req(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_zdo_pim_permit_turbo_poll(zb_bool_t permit)
{
  ZVUNUSED(permit);
  ZB_ASSERT(ZB_FALSE);
}

void *nwk_alloc_and_fill_cmd(zb_bufid_t buf, zb_uint8_t cmd, zb_uint8_t cmd_size)
{
  ZVUNUSED(buf);
  ZVUNUSED(cmd);
  ZVUNUSED(cmd_size);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

zb_ret_t zb_nwk_neighbor_get_by_short(zb_uint16_t short_addr, zb_neighbor_tbl_ent_t **nbt)
{
  ZVUNUSED(short_addr);
  ZVUNUSED(nbt);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

zb_aps_device_key_pair_set_t * zb_secur_update_key_pair(zb_ieee_addr_t address,
                                                        zb_uint8_t* key,
                                                        zb_uint8_t key_type,
                                                        zb_uint8_t key_attr,
                                                        zb_uint8_t key_source)
{
  ZVUNUSED(address);
  ZVUNUSED(key);
  ZVUNUSED(key_type);
  ZVUNUSED(key_attr);
  ZVUNUSED(key_source);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void tp_send_req_by_short(zb_uint16_t command_id,
  zb_uint8_t param,
  zb_uint16_t profile_id,
  zb_uint16_t addr,
  zb_uint8_t addr_mode,
  zb_uint8_t src_ep,
  zb_uint8_t dst_ep,
  zb_uint8_t tx_options,
  zb_uint8_t radius)
{
  ZVUNUSED(command_id);
  ZVUNUSED(param);
  ZVUNUSED(profile_id);
  ZVUNUSED(addr);
  ZVUNUSED(addr_mode);
  ZVUNUSED(src_ep);
  ZVUNUSED(dst_ep);
  ZVUNUSED(tx_options);
  ZVUNUSED(radius);

  ZB_ASSERT(ZB_FALSE);

}

zb_nwk_hdr_t *nwk_alloc_and_fill_hdr(zb_bufid_t buf,
                                     zb_uint16_t src_addr, zb_uint16_t dst_addr,
                                     zb_bool_t is_multicast, zb_bool_t is_secured, zb_bool_t is_cmd_frame, zb_bool_t force_long)
{
  ZVUNUSED(buf);
  ZVUNUSED(src_addr);
  ZVUNUSED(dst_addr);
  ZVUNUSED(is_multicast);
  ZVUNUSED(is_secured);
  ZVUNUSED(is_cmd_frame);
  ZVUNUSED(force_long);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

zb_uint8_t zb_nwk_get_nbr_dvc_type_by_ieee(zb_ieee_addr_t addr)
{
  ZVUNUSED(addr);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_nlme_permit_joining_request(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t *secur_nwk_key_by_seq(zb_ushort_t key_seq_number)
{
  ZVUNUSED(key_seq_number);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_secur_switch_nwk_key_br(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void zb_apsme_transport_key_request(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void zb_start_concentrator_mode(zb_uint8_t radius, zb_uint32_t disc_time)
{
  ZVUNUSED(radius);
  ZVUNUSED(disc_time);

  ZB_ASSERT(ZB_FALSE);
}

void zb_nwk_pib_set(zb_uint8_t param, zb_uint8_t attr, void *value,
                    zb_ushort_t value_size, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(attr);
  ZVUNUSED(value);
  ZVUNUSED(value_size);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);
}

void zb_nlme_send_status(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void zb_mac_send_frame(zb_bufid_t buf, zb_uint8_t mhr_len)
{
  ZVUNUSED(buf);
  ZVUNUSED(mhr_len);

  ZB_ASSERT(ZB_FALSE);
}

void zb_aps_send_command(zb_uint8_t param, zb_uint16_t dest_addr, zb_uint8_t command, zb_bool_t secure,
                         zb_uint8_t secure_aps_i)
{
  ZVUNUSED(param);
  ZVUNUSED(dest_addr);
  ZVUNUSED(command);
  ZVUNUSED(secure);
  ZVUNUSED(secure_aps_i);

  ZB_ASSERT(ZB_FALSE);
}

void zb_secur_apsme_remove_device(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_default_ffd_descriptor_values(zb_logical_type_t device_type)
{
  ZVUNUSED(device_type);
  ZB_ASSERT(ZB_FALSE);
}

void zb_mcps_data_request(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void zb_mcps_build_data_request(zb_bufid_t buf, zb_uint16_t src_addr_param, zb_uint16_t dst_addr_param,
  zb_uint8_t tx_options_param, zb_uint8_t msdu_hande_param)
{
  ZVUNUSED(buf);
  ZVUNUSED(src_addr_param);
  ZVUNUSED(dst_addr_param);
  ZVUNUSED(tx_options_param);
  ZVUNUSED(msdu_hande_param);

  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_callback_t cb, zb_uint16_t addr,
                           zb_uint8_t resp_counter)
{
  ZVUNUSED(command_id);
  ZVUNUSED(param);
  ZVUNUSED(cb);
  ZVUNUSED(addr);
  ZVUNUSED(resp_counter);
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_fcf_set_src_addressing_mode(zb_uint8_t *p_fcf, zb_uint8_t addr_mode)
{
  ZVUNUSED(p_fcf);
  ZVUNUSED(addr_mode);
  ZB_ASSERT(ZB_FALSE);
}

void zb_tp_transmit_counted_packets_req_ext(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);
}

void zb_tp_transmit_counted_packets_req(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);
}

void zdo_send_resp_by_short(zb_uint16_t command_id, zb_uint8_t param, zb_uint16_t addr)
{
  ZVUNUSED(command_id);
  ZVUNUSED(param);
  ZVUNUSED(addr);

  ZB_ASSERT(ZB_FALSE);
}

void zb_nlme_route_discovery_request(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

zb_bool_t zb_tp_send_link_status_command(zb_uint8_t param, zb_uint16_t short_addr)
{
  ZVUNUSED(param);
  ZVUNUSED(short_addr);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_mlme_start_request(zb_uint8_t param)
{
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE);
}

void zb_enable_auto_pan_id_conflict_resolution(zb_bool_t status)
{
  ZVUNUSED(status);
  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t zb_zdo_mgmt_nwk_update_req(zb_uint8_t param, zb_callback_t cb)
{
  ZVUNUSED(param);
  ZVUNUSED(cb);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_init_ed_aging(zb_neighbor_tbl_ent_t *nbt, zb_uint8_t timeout, zb_bool_t run_aging)
{
  ZVUNUSED(nbt);
  ZVUNUSED(timeout);
  ZVUNUSED(run_aging);

  ZB_ASSERT(ZB_FALSE);
}

zb_ret_t zb_nwk_send_rrec(zb_bufid_t cbuf, zb_uint16_t src_addr, zb_uint16_t dst_addr)
{
  ZVUNUSED(cbuf);
  ZVUNUSED(src_addr);
  ZVUNUSED(dst_addr);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_tp_device_announce(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_zdo_do_set_channel(zb_uint8_t channel)
{
  ZVUNUSED(channel);

  ZB_ASSERT(ZB_FALSE);
}

void zb_secur_send_nwk_key_update_br(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_fcf_set_dst_addressing_mode(zb_uint8_t *p_fcf, zb_uint8_t addr_mode)
{
  ZVUNUSED(p_fcf);
  ZVUNUSED(addr_mode);

  ZB_ASSERT(ZB_FALSE);
}

void zdo_send_device_annce(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

zb_uint8_t zb_nwk_get_nbr_rel_by_short(zb_uint16_t addr)
{
  ZVUNUSED(addr);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

zb_ret_t zb_nwk_delete_neighbor_by_short(zb_uint16_t addr)
{
  ZVUNUSED(addr);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}


zb_nwk_routing_t *new_routing_table_ent()
{
  ZB_ASSERT(ZB_FALSE);

  return 0;
}

#ifdef ZB_PRO_STACK

zb_nwk_routing_t *zb_nwk_mesh_find_route(zb_uint16_t dest_addr, zb_uint8_t is_multicast)
{
  ZVUNUSED(dest_addr);
  ZVUNUSED(is_multicast);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

#else

zb_nwk_routing_t *zb_nwk_mesh_find_route(zb_uint16_t dest_addr)
{
  ZVUNUSED(dest_addr);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

#endif



void zb_set_node_power_descriptor(zb_current_power_mode_t current_power_mode, zb_uint8_t available_power_sources,
                                  zb_uint8_t current_power_source, zb_power_source_level_t current_power_source_level)
{
  ZVUNUSED(current_power_mode);
  ZVUNUSED(available_power_sources);
  ZVUNUSED(current_power_source);
  ZVUNUSED(current_power_source_level);

  ZB_ASSERT(ZB_FALSE);
}

void zdo_set_aging_timeout(zb_uint8_t timeout)
{
  ZVUNUSED(timeout);

  ZB_ASSERT(ZB_FALSE);
}

void zb_zdo_pim_start_poll(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_zdo_pim_stop_poll(zb_uint8_t param)
{
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

zb_ret_t zb_zdo_set_lpd_cmd_mode(zb_uint8_t mode)
{
  ZVUNUSED(mode);

  ZB_ASSERT(ZB_FALSE);

  return 0;
}

void zb_zdo_set_lpd_cmd_timeout(zb_uint8_t timeout)
{
  ZVUNUSED(timeout);

  ZB_ASSERT(ZB_FALSE);
}

void zb_start_get_peer_short_addr(zb_address_ieee_ref_t dst_addr_ref, zb_callback_t cb, zb_uint8_t param)
{
  ZVUNUSED(dst_addr_ref);
  ZVUNUSED(cb);
  ZVUNUSED(param);

  ZB_ASSERT(ZB_FALSE);
}

void zb_set_default_ed_descriptor_values()
{
  ZB_ASSERT(ZB_FALSE);
}


#endif /* NCP_MODE_HOST */