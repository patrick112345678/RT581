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
/*  PURPOSE: Weak implementation of NCP interface.
 Required to link non-ncp tests in NCP-enabled build
*/

#define ZB_TRACE_FILE_ID 12028
#include "zb_common.h"
#include "zb_ncp.h"

#ifdef NCP_MODE

ZB_WEAK_PRE void ncp_signal(ncp_signal_t signal, zb_uint8_t param) ZB_WEAK;
#if defined ZB_ENABLE_SE_MIN_CONFIG
ZB_WEAK_PRE void ncp_se_signal(zse_commissioning_signal_t signal, zb_uint8_t param) ZB_WEAK;
#endif /* ZB_ENABLE_SE_MIN_CONFIG */
ZB_WEAK_PRE void ncp_address_update_ind(zb_uint16_t short_address) ZB_WEAK;
#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
ZB_WEAK_PRE void ncp_route_reply_ind(zb_nwk_cmd_rrep_t *rrep) ZB_WEAK;
ZB_WEAK_PRE void ncp_nwk_route_req_send_ind(zb_nwk_cmd_rreq_t *rreq) ZB_WEAK;
ZB_WEAK_PRE void ncp_nwk_route_send_rrec_ind(zb_nwk_cmd_rrec_t *rrec) ZB_WEAK;
#endif
ZB_WEAK_PRE zb_bool_t ncp_catch_zcl_packet(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info, zb_uint8_t zcl_parse_status) ZB_WEAK;
ZB_WEAK_PRE zb_bool_t ncp_catch_aps_data_conf(zb_uint8_t param) ZB_WEAK;
ZB_WEAK_PRE zb_bool_t ncp_catch_nwk_disc_conf(zb_uint8_t param) ZB_WEAK;
ZB_WEAK_PRE zb_bool_t ncp_partner_lk_failed(zb_uint8_t param) ZB_WEAK;
ZB_WEAK_PRE void sncp_auto_turbo_poll_ed_timeout(zb_bool_t is_on) ZB_WEAK;
ZB_WEAK_PRE void sncp_auto_turbo_poll_aps_rx(zb_bool_t is_on) ZB_WEAK;
ZB_WEAK_PRE void sncp_auto_turbo_poll_aps_tx(zb_bool_t is_on) ZB_WEAK;
ZB_WEAK_PRE void sncp_auto_turbo_poll_stop(void) ZB_WEAK;

void ncp_signal(ncp_signal_t signal, zb_uint8_t param)
{
    ZVUNUSED(signal);
    zb_buf_free(param);
}

#if defined ZB_ENABLE_SE_MIN_CONFIG
void ncp_se_signal(zse_commissioning_signal_t signal, zb_uint8_t param)
{
    ZVUNUSED(signal);
    zb_buf_free(param);
}
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

zb_bool_t ncp_catch_zcl_packet(zb_uint8_t param, zb_zcl_parsed_hdr_t *cmd_info, zb_uint8_t zcl_parse_status)
{
    ZVUNUSED(param);
    ZVUNUSED(cmd_info);
    ZVUNUSED(zcl_parse_status);
    return ZB_FALSE;
}

zb_bool_t ncp_catch_aps_data_conf(zb_uint8_t param)
{
    ZVUNUSED(param);
    return ZB_FALSE;
}

zb_bool_t ncp_catch_nwk_disc_conf(zb_uint8_t param)
{
    ZVUNUSED(param);
    return ZB_FALSE;
}

#if defined ZB_ENABLE_SE_MIN_CONFIG
zb_bool_t ncp_partner_lk_failed(zb_uint8_t param)
{
    ZVUNUSED(param);
    return ZB_FALSE;
}
#endif /* ZB_ENABLE_SE_MIN_CONFIG */

void ncp_address_update_ind(zb_uint16_t short_address)
{
    ZVUNUSED(short_address);
}

void ncp_apsme_remote_bind_unbind_ind(zb_uint8_t param, zb_bool_t bind)
{
    ZVUNUSED(param);
    ZVUNUSED(bind);
}

#ifdef ZB_APSDE_REQ_ROUTING_FEATURES
void ncp_route_reply_ind(zb_nwk_cmd_rrep_t *rrep)
{
    ZVUNUSED(rrep);
}

void ncp_nwk_route_req_send_ind(zb_nwk_cmd_rreq_t *rreq)
{
    ZVUNUSED(rreq);
}

void ncp_nwk_route_send_rrec_ind(zb_nwk_cmd_rrec_t *rrec)
{
    ZVUNUSED(rrec);
}
#endif

void sncp_auto_turbo_poll_ed_timeout(zb_bool_t is_on)
{
    ZVUNUSED(is_on);
}

void sncp_auto_turbo_poll_aps_rx(zb_bool_t is_on)
{
    ZVUNUSED(is_on);
}

void sncp_auto_turbo_poll_aps_tx(zb_bool_t is_on)
{
    ZVUNUSED(is_on);
}

void sncp_auto_turbo_poll_stop(void)
{

}

#endif  /* NCP_MODE */
