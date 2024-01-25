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
/* PURPOSE: ZGPD routines
*/

#define ZB_TRACE_FILE_ID 2109

#include "zb_common.h"

/* WARNING: Currently ZGP works only with our mac. */
#if defined ZB_ZGPD_ROLE && defined ZB_ENABLE_ZGP
#include "zb_mac.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zboss_api_zgp.h"
#include "zgp/zgp_internal.h"
#include "zgpd/zb_zgpd.h"

#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_SET_CONFIRM
#include "zb_mac_only_stubs.h"

void zgpd_send_app_description(zb_uint8_t param);

/** @addtogroup zgp_zgpd_int */
/** @{ */

#define ZGPD_NOTIFY_COMM_COMPLETE(buf, res) \
{ \
  zb_buf_set_status(param, res);                \
  if (ZGPD->comm_cb != NULL) \
  { \
    ZB_SCHEDULE_CALLBACK(ZGPD->comm_cb, param); \
    ZGPD->comm_cb = NULL; \
  } \
  else \
  { \
    zb_buf_free(param); \
  } \
}

#define STARTUP_COMPLETE(buf, res) \
{ \
  zb_buf_set_status(param, res);                \
  if (ZGPD->startup_cb != NULL) \
  { \
    ZB_SCHEDULE_CALLBACK(ZGPD->startup_cb, param); \
    ZGPD->startup_cb = NULL; \
  } \
}


/**
 * Global ZGPD context
 */
zb_zgpd_ctx_t g_zgpd_ctx;

void zgpd_set_channel_and_call(zb_uint8_t param, zb_callback_t func);
void zgpd_set_dsn_and_call(zb_uint8_t param, zb_callback_t func);
void zgpd_set_channel_send_channel_req(zb_uint8_t param);
void zgpd_send_channel_req(zb_uint8_t param);
void zgpd_switch_rx_on(zb_uint8_t param);
void zgpd_switch_rx_off(zb_uint8_t param);
void zgpd_channel_req_retry(zb_uint8_t param);
void zgpd_channel_config_ind(zb_uint8_t param);
void zgpd_set_channel_send_commiss_req(zb_uint8_t param);
void zgpd_send_commissioning_req(zb_uint8_t param);
void zgpd_commiss_reply_ind(zb_uint8_t param);

static void rx_on_when_idle_confirm(zb_uint8_t param);


zb_uint8_t zb_zgpd_mac_num_gen(void)
{
  if(ZGPD->use_random_seq_num)
  {
    zb_uint8_t o_v = ZB_RANDOM_U8();
#ifndef ZB_ALIEN_MAC
    if (o_v==MAC_PIB().mac_dsn)o_v++;
#endif
    TRACE_MSG(TRACE_ZGP3, "zb_mac_num_gen: %hd", (FMT__H, o_v));
#ifndef ZB_ALIEN_MAC
    MAC_PIB().mac_dsn = o_v;
#endif
    return o_v;
  }
#ifndef ZB_ALIEN_MAC
  else
    return ++MAC_PIB().mac_dsn;
#endif
  return 0;
}

/**
 * @brief Construct ZGP Stub NWK header of GPDF based on the ZGPD context
 *
 * @param  buf        [in]  Buffer with GPDF
 * @param  frame_type [in]  Frame type that should be used in NWK Frame control
 *
 * @return Number of bytes written in NWK header
 */
static zb_uint8_t fill_gpdf_nwk_hdr(zb_bufid_t buf, zb_gpdf_info_t *gpdf_info)
{
  zb_uint8_t *ptr;
  zb_uint8_t  frame_type = ZB_GPDF_NFC_GET_FRAME_TYPE(gpdf_info->nwk_frame_ctl);
  zb_uint8_t  src_id_fld_size = ZGPD_SRC_ID_SIZE(ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl), frame_type);
  zb_uint8_t  endpoint_present = (((ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_APP_ID_0010) && (frame_type != ZGP_FRAME_TYPE_MAINTENANCE)) ? 1 : 0);
  zb_uint8_t  sec_level = ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl);
  zb_uint8_t  ext_nwk_frame_ctl_size = (ZB_GPDF_NFC_GET_NFC_EXT(gpdf_info->nwk_frame_ctl) == 1);
  zb_uint8_t  zgp_nwk_hdr_len;
  zb_uint8_t  zgp_gpdf_fcs = GPDF_SECURITY_FRAME_COUNTER_SIZE(sec_level);

#ifdef ZB_CERTIFICATION_HACKS
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_UNPROTECT_PACKET))
  {
    zgp_gpdf_fcs = 0;
  }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INSERT_EXTNWK_FC_DATA: %hd ", (FMT__H, ZGPD->ch_insert_extnwk_data));
    if(ZGPD->ch_insert_extnwk_data==0) ext_nwk_frame_ctl_size = 0; else ext_nwk_frame_ctl_size = 1; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_FC))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INSERT_FC: %hd", (FMT__H, ZGPD->ch_insert_frame_counter));
    if(ZGPD->ch_insert_frame_counter>=1) { zgp_gpdf_fcs=4; } else zgp_gpdf_fcs=0; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_FRAME_TYPE: %hd -> %hd", (FMT__H_H, frame_type, ZGPD->ch_replace_frame_type));
    src_id_fld_size = ZGPD_SRC_ID_SIZE(ZB_GPDF_EXT_NFC_GET_APP_ID(gpdf_info->nwk_ext_frame_ctl), frame_type);
    if(ZGPD->ch_replace_frame_type>ZGP_FRAME_TYPE_DATA) src_id_fld_size = 4; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_APPID))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_APPID: %hd -> %hd", (FMT__H_H, ZGPD->id.app_id, ZGPD->ch_replace_app_id));
    TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_APPID:src_id_fld_size: %hd -> %hd", (FMT__H_H, src_id_fld_size,
      ZGPD_SRC_ID_SIZE(ZB_GPDF_EXT_NFC_GET_APP_ID(ZGPD->id.app_id), frame_type)) );
    src_id_fld_size = ZGPD_SRC_ID_SIZE(ZB_GPDF_EXT_NFC_GET_APP_ID(ZGPD->id.app_id), frame_type);
  }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_EP))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INSERT_EP: %hd", (FMT__H, ZGPD->ch_insert_endpoint));
    if(ZGPD->ch_insert_endpoint>=1) { endpoint_present=1; } else endpoint_present=0; }
#endif
  zgp_nwk_hdr_len = 1 //NWK frame control
            + ext_nwk_frame_ctl_size
            + src_id_fld_size
            + endpoint_present
            + zgp_gpdf_fcs;

  TRACE_MSG(TRACE_ZGP1, ">> fill_gpdf_nwk_hdr buf %p, gpdf_info %p",
      (FMT__P_P, buf, gpdf_info));

  TRACE_MSG(TRACE_ZGP2, "zgp_nwk_hdr_len %hd, application id %hd",
      (FMT__H_H, zgp_nwk_hdr_len, gpdf_info->zgpd_id.app_id));

  ptr = zb_buf_alloc_left(buf, zgp_nwk_hdr_len);

#ifdef ZB_CERTIFICATION_HACKS
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG))
  {
    *ptr++ = ZGPD->ch_tmp_nwk_hdr;
  }
  else
#endif
  *ptr++ = gpdf_info->nwk_frame_ctl;

  if (ext_nwk_frame_ctl_size)
  {
#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL))
    {
      TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_SEC_LEVEL: %hd -> %hd", (FMT__H_H, sec_level, ZGPD->ch_replace_sec_level));
      *ptr++ = ZGPD->ch_tmp_ext_nwk_hdr;
    }
    else
#endif
      *ptr++ = gpdf_info->nwk_ext_frame_ctl;
  }

  if (src_id_fld_size > 0)
  {
#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SRCID))
    {
      TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_SRCID: %hd -> %hd", (FMT__H_H, ZGPD->id.addr.src_id, ZGPD->ch_replace_src_id));
      ZB_HTOLE32(ptr, &ZGPD->ch_replace_src_id);
    }
    else
#endif
      ZB_HTOLE32(ptr, &ZGPD->id.addr.src_id);
    ptr += 4;
  }

  if (endpoint_present)
  {
    *ptr++ = ZGPD->id.endpoint;
  }
#ifdef ZB_CERTIFICATION_HACKS
  if (zgp_gpdf_fcs > 0)
#else
  if (sec_level > ZB_ZGP_SEC_LEVEL_REDUCED)
#endif
  {
    ZB_HTOLE32(ptr, &gpdf_info->sec_frame_counter);
  }

  TRACE_MSG(TRACE_ZGP1, "<< fill_gpdf_nwk_hdr, ret %hd", (FMT__H, zgp_nwk_hdr_len));

  return zgp_nwk_hdr_len;
}

#ifndef ZB_ALIEN_MAC
static void set_consistent_mac_dsn_cb(zb_uint8_t param)
{
  zb_buf_free(param);
  ZB_ASSERT(ZGPD->tx_buf_ref);
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, ZGPD->tx_buf_ref);
  ZGPD->tx_buf_ref = 0;
}

static void set_consistent_mac_dsn(zb_uint8_t param)
{
  ZGPD->mac_dsn = ((ZGPD->security_frame_counter-1) & 0x000000FF);
  zgpd_set_dsn_and_call(param, set_consistent_mac_dsn_cb);
}
#endif

static zb_uint8_t set_rx_after_tx_by_cmd()
{
  zb_uint8_t ret = 0;

  switch (ZGPD->tx_cmd)
  {
    case ZB_GPDF_CMD_REQUEST_ATTRIBUTES:
    case ZB_GPDF_CMD_CHANNEL_REQUEST:
      ret = 1;
      break;
    case ZB_GPDF_CMD_COMMISSIONING:
      /* According to A.1.7.3 GPD commissioning: The GPD SHOULD only set the RxAfterTx sub-field in the Commissioning
GPDF, if it expects a response, i.e. if at least one of the sub-fields PANId request sub-field or GPD
Security key request is set to 0b1 */
      ret = (ZGPD->pan_id_request == 1) ||
            (ZGPD->gpd_security_key_request == 1) ||
            ((ZGPD->commissioning_method == ZB_ZGPD_COMMISSIONING_BIDIR) && (ZGPD->app_info_options.app_descr_flw == 0));
      break;
    case ZB_GPDF_CMD_APPLICATION_DESCR:
      ret = ((ZGPD->commissioning_method == ZB_ZGPD_COMMISSIONING_BIDIR) && (ZGPD->app_descr.next_report == 0));
      break;
  }

  return ret;
}

/**
 * @brief Construct and send GPDF command frame
 *
 * @param buf_ref    [in]  Reference to buffer with ZGP application payload
 * @param frame_type [in]  Frame type that should be used in NWK Frame control
 */
void zb_zgpd_send_req(zb_uint8_t buf_ref, zb_uint8_t frame_type, zb_bool_t use_secur)
{
  zb_mcps_data_req_params_t *mac_data_req = ZB_BUF_GET_PARAM(buf_ref, zb_mcps_data_req_params_t);
  zb_uint8_t sec_level = (use_secur) ? ( ZGPD->security_level ) : ZB_ZGP_SEC_LEVEL_NO_SECURITY;
  zb_gpdf_info_t gpdf_info;
  zb_uint8_t ext_nwk_frame_ctl_size = ZGPD_EXTENDED_NWK_FRAME_CTL_SIZE(frame_type, sec_level);
  /* cmd id is not encrypted in commissioning frame, so can extract it */
  zb_uint8_t is_comm_frame = *(zb_uint8_t *)(zb_buf_begin(buf_ref)) == ZB_GPDF_CMD_COMMISSIONING;

  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_send_req buf_ref %hd, frame_type %hd, use_secur %hd, sec_lvl %hd",
      (FMT__H_H_H_H, buf_ref, frame_type, use_secur, sec_level));
  TRACE_MSG(TRACE_ZGP2, "application id %hd", (FMT__H, ZGPD->id.app_id));

  if (sec_level == ZB_ZGP_SEC_LEVEL_NO_SECURITY)
  {
    use_secur = ZB_FALSE;
  }

  /*  A.1.4.2.1 Maintenance FrameType
      If the FrameType 0b01 (Maintenance frame) is used,
      then the GPD SrcID field and the Endpoint field SHALL NOT be present.
      The GPD IEEE address in the MAC header SHOULD NOT be present.
      The security fields (Security frame counter and MIC) SHALL NOT be present and
      the frame SHALL be sent unprotected.
      If the GPDF is sent from the GPD, the Extended NWK Frame Control field SHALL be omitted.
      If the GPDF is sent to the GPD, the Extended NWK Frame Control field SHALL be omitted.
      In both cases, the NWK Frame Control Extension sub-field SHALL be set to 0b0. */
  if(frame_type==ZGP_FRAME_TYPE_MAINTENANCE) ext_nwk_frame_ctl_size = 0;

  {
    zb_uint8_t ft = frame_type;
    zb_uint8_t proto = ZB_ZGP_PROTOCOL_VERSION;
    zb_uint8_t autocomm = (ZGPD->auto_commissioning_pending ? 1 : 0);
#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_FRAME_TYPE))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_FRAME_TYPE: %hd -> %hd", (FMT__H_H, frame_type, ZGPD->ch_replace_frame_type));
      ft = ZGPD->ch_replace_frame_type; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_PROTO_VERSION))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_PROTO_VERSION: %hd -> %hd", (FMT__H_H, proto, ZGPD->ch_replace_proto_version));
      proto = ZGPD->ch_replace_proto_version; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_AUTO_COMM))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_AUTO_COMM: %hd -> %hd", (FMT__H_H, autocomm, ZGPD->ch_replace_autocomm));
      autocomm = ZGPD->ch_replace_autocomm; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_EXTNWK_FC_FLAG: %hd -> %hd", (FMT__H_H, ext_nwk_frame_ctl_size, ZGPD->ch_replace_extnwk_flag));
      ZGPD->ch_tmp_nwk_hdr = (ft) | (proto << 2) | ((autocomm) << 6) | ((ZGPD->ch_replace_extnwk_flag) << 7); }
#endif

    gpdf_info.nwk_frame_ctl = ( (ft) | (proto << 2) | ((autocomm) << 6) | ((ext_nwk_frame_ctl_size) << 7));
  }
  {
    zb_uint8_t security_key_type = ZGPD->security_key_type;
    zb_uint8_t sl = sec_level;
    zb_uint8_t app_id = ZGPD->id.app_id;
    zb_uint8_t direction = ZGP_FRAME_DIR_FROM_ZGPD;
    zb_uint8_t rxaftertx;

    rxaftertx = set_rx_after_tx_by_cmd();

#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_KEY_TYPE))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_KEY_TYPE: %hd -> %hd", (FMT__H_H, security_key_type, ZGPD->ch_replace_key_type));
      security_key_type = ZGPD->ch_replace_key_type; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_RXAFTERTX))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_RXAFTERTX: %hd -> %hd", (FMT__H_H, rxaftertx, ZGPD->ch_replace_rxaftertx));
      rxaftertx = ZGPD->ch_replace_rxaftertx; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_APPID))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_APPID: %hd -> %hd", (FMT__H_H, app_id, ZGPD->ch_replace_app_id));
      app_id = ZGPD->ch_replace_app_id; }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_DIRECTION))
    { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_DIRECTION: %hd -> %hd", (FMT__H_H, direction, ZGPD->ch_replace_direction));
      direction = ZGPD->ch_replace_direction; }
#endif
/*  For ApplicationID 0b000 and 0b010 (GP) and ApplicationID 0b001 (LPED), the bits 3-7
    are defined in Figure 8. For ApplicationID 0b000 (GP), the Extended NWK Frame Control
    field SHALL be present if the GPDF is protected, if RxAfterTx is set, or if the GPDF
    is sent to the GPD.*/
/*  A.1.4.2.1 Maintenance FrameType
    If the FrameType 0b01 (Maintenance frame) is used,
    then the GPD SrcID field and the Endpoint field SHALL NOT be present.
    The GPD IEEE address in the MAC header SHOULD NOT be present.
    The security fields (Security frame counter and MIC) SHALL NOT be present and
    the frame SHALL be sent unprotected.
    If the GPDF is sent from the GPD, the Extended NWK Frame Control field SHALL be omitted.
    If the GPDF is sent to the GPD, the Extended NWK Frame Control field SHALL be omitted.
    In both cases, the NWK Frame Control Extension sub-field SHALL be set to 0b0. */
    if(((app_id == ZB_ZGP_APP_ID_0000)||(app_id == ZB_ZGP_APP_ID_0010))&&(frame_type!=ZGP_FRAME_TYPE_MAINTENANCE))
      gpdf_info.nwk_frame_ctl |= (rxaftertx << 7);

    ZB_GPDF_NWK_FRAME_CTL_EXT(gpdf_info.nwk_ext_frame_ctl,
                            app_id,
                            sl,
                            ZGP_KEY_TYPE_IS_INDIVIDUAL(((use_secur) ? security_key_type : 0)),
                            rxaftertx,
                            direction);
#ifdef ZB_CERTIFICATION_HACKS
    ZB_GPDF_NWK_FRAME_CTL_EXT(ZGPD->ch_tmp_ext_nwk_hdr,
                            app_id,
                            ZGPD->ch_replace_sec_level,
                            ZGP_KEY_TYPE_IS_INDIVIDUAL(security_key_type),
                            rxaftertx,
                            direction);
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_USE_CH_SEC_LEVEL))
      gpdf_info.nwk_ext_frame_ctl = ZGPD->ch_tmp_ext_nwk_hdr;
#endif  /* ZB_CERTIFICATION_HACKS */
  }

  if (sec_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
  {
#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_FRAME_COUNTER))
    {
      TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_FRAME_COUNTER: %hd -> %hd", (FMT__L_L, ZGPD->security_frame_counter, ZGPD->ch_replace_frame_counter));
      gpdf_info.sec_frame_counter = ZGPD->ch_replace_frame_counter;
    }
    else
#endif
      gpdf_info.sec_frame_counter = ZGPD->security_frame_counter;
#ifdef ZB_CERTIFICATION_HACKS
      if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INCR_SFC))
      { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INCR_SFC: %hd", (FMT__H, ++gpdf_info.sec_frame_counter)); }
#endif
  }

  /* Security frame counter and mac dsn should be synchronized (see ZGP spec, A.1.6.4.4) */
  /* For gpdSecurityLevel 0b10 and 0b11, the MAC sequence number field SHOULD carry the 1LSB of the
   * gpdSecurityFrameCounter. (see ZGP spec, A.1.6.4.4) */
  ZGPD->security_frame_counter++;

  gpdf_info.zgpd_id     = ZGPD->id;
  gpdf_info.zgpd_cmd_id = *(zb_uint8_t*)zb_buf_begin(buf_ref);
  gpdf_info.mac_addr_flds.s.dst_addr   = ZB_NWK_BROADCAST_ALL_DEVICES;
  gpdf_info.mac_addr_flds.s.dst_pan_id = ZB_BROADCAST_PAN_ID;
  gpdf_info.mac_addr_flds_len = sizeof(gpdf_info.mac_addr_flds.s);

  gpdf_info.nwk_hdr_len = fill_gpdf_nwk_hdr(buf_ref, &gpdf_info);

  ZB_BZERO(mac_data_req, sizeof(zb_mcps_data_req_params_t));

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_MISSING_IEEE))
  {
    mac_data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
    mac_data_req->src_addr.addr_short = ZB_NWK_BROADCAST_ALL_DEVICES;
  }
  else
#endif
  if (((ZGPD->id.app_id != ZB_ZGP_APP_ID_0000) && (frame_type != ZGP_FRAME_TYPE_MAINTENANCE))
      || (ZGPD->send_ieee_addr_in_comm_frame && is_comm_frame))
  {
    TRACE_MSG(TRACE_ZGP2, "Send IEEE Src addr", (FMT__0));
    mac_data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
    ZB_64BIT_ADDR_COPY(mac_data_req->src_addr.addr_long, ZB_PIBCACHE_EXTENDED_ADDRESS());
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_CORRUPT_IEEE_ADDR))
  {
    mac_data_req->src_addr.addr_long[0] ^= 0x1;
  }
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_BZERO_IEEE_ADDR))
  {
    ZB_64BIT_ADDR_ZERO(mac_data_req->src_addr.addr_long);
  }
#endif
  }

  /* TODO: Implement random MAC sequence number */
  {
    zb_uint8_t sl = ZGPD->security_level;
#ifdef ZB_CERTIFICATION_HACKS
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SEC_LEVEL))
    {
      TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_SEC_LEVEL: %hd -> %hd", (FMT__L_L, sl, ZGPD->ch_replace_sec_level));
      sl = ZGPD->ch_replace_sec_level;
    }
    if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_FC))
    {
      TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INSERT_FC: %hd", (FMT__H, ZGPD->ch_insert_frame_counter));
      if(ZGPD->ch_insert_frame_counter==8) sl = ZGPD->security_level;
    }
#endif


    if (sl > ZB_ZGP_SEC_LEVEL_NO_SECURITY
#ifdef ZB_CERTIFICATION_HACKS
        && !ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_UNPROTECT_PACKET)
#endif
       )
    {
#ifdef ZB_CERTIFICATION_HACKS
      if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_KEY))
      {
        TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_KEY: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ZGPD->ch_replace_key)));
        zb_zgp_protect_frame(&gpdf_info, ZGPD->ch_replace_key, buf_ref);
      }
      else
#endif
      zb_zgp_protect_frame(&gpdf_info, ZGPD->security_key, buf_ref);
    }
  }

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_MAX_PAYLOAD))
  {
    zb_uint8_t  bytes_writen = zb_buf_len(buf_ref);
    zb_uint8_t  remaining = 0;
    zb_uint8_t *ptr;
    zb_uint8_t  i;

    remaining = (((ZGPD->id.app_id == ZB_ZGP_APP_ID_0000) ? 55 : 50) - bytes_writen) + 6;

    TRACE_MSG(TRACE_ZGP1, "bytes_writen: %hd, remaining: %hd", (FMT__H_H, bytes_writen, remaining));

    ptr = zb_buf_alloc_right(buf_ref, remaining);
    for (i = 0; i < remaining; i++)
    {
      ZB_GPDF_PUT_UINT8(ptr, i);
    }

    TRACE_MSG(TRACE_ZGP1, "bytes_writen: %hd", (FMT__H, zb_buf_len(buf_ref)));
  }
#endif

  /* Always use broadcast dest */
  mac_data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  mac_data_req->dst_addr.addr_short = ZB_NWK_BROADCAST_ALL_DEVICES;
  if (ZGPD->pan_id != 0)
  {
    mac_data_req->dst_pan_id = ZGPD->pan_id;
  }
  else
  {
    mac_data_req->dst_pan_id = ZB_BROADCAST_PAN_ID;
  }
  mac_data_req->tx_options = MAC_TX_OPTION_NO_CSMA_CA;

#ifndef ZB_ALIEN_MAC
  TRACE_MSG(TRACE_ZGP3, "use_secur: %hd, MAC_DSN: %hd, SEC_FRAME_COUNTER: %d",
            (FMT__H_H_D, use_secur, MAC_PIB().mac_dsn, (ZGPD->security_frame_counter-1)));
  if (use_secur && (MAC_PIB().mac_dsn != ((ZGPD->security_frame_counter-1) & 0x000000FF)))
  {
    TRACE_MSG(TRACE_ZGP1, "!set_consistent_mac_dsn", (FMT__0));
    ZGPD->tx_buf_ref = buf_ref;
    zb_buf_get_out_delayed(set_consistent_mac_dsn);
  }
  else
#endif
  {
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, buf_ref);
  }
  ZGPD->tx_cmd = 0;

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_send_req", (FMT__0));
}

void zb_zgpd_send_data_req(zb_uint8_t buf_ref, zb_bool_t use_secur)
{
  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_send_data_req buf_ref %hd", (FMT__H, buf_ref));

  zb_zgpd_send_req(buf_ref, ZGP_FRAME_TYPE_DATA, use_secur);

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_send_data_req", (FMT__0));
}

void zb_zgpd_send_maint_req(zb_uint8_t buf_ref)
{
  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_send_maint_req buf_ref %hd", (FMT__H, buf_ref));

  zb_zgpd_send_req(buf_ref, ZGP_FRAME_TYPE_MAINTENANCE, ZB_FALSE);

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_send_maint_req", (FMT__0));
}

void zb_zgpd_start_commissioning(zb_callback_t cb)
{
  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_start_commissioning", (FMT__0));

  ZGPD->comm_state = ZB_ZGPD_STATE_COMM_IN_PROGRESS;
  ZGPD->comm_cb = cb;

  switch (ZGPD->commissioning_method)
  {
    case ZB_ZGPD_COMMISSIONING_AUTO:
      TRACE_MSG(TRACE_ZGP1, "autocommissioning", (FMT__0));
      ZGPD->auto_commissioning_pending = 1;
      break;

    case ZB_ZGPD_COMMISSIONING_UNIDIR:
      /* send commissioning GPFS */
      TRACE_MSG(TRACE_ZGP1, "unidirectional commissioning", (FMT__0));
      ZGPD->toggle_channel = ZB_FALSE;
      ZGPD->comm_state = ZB_ZGPD_STATE_COMMISSIONED;
      zb_buf_get_out_delayed(zgpd_send_commissioning_req);
      break;

    case ZB_ZGPD_COMMISSIONING_BIDIR:
      /* See:
         Figure 69  Exemplary MSC for proxy-based commissioning for
         bidirectional commissioning capable GPD

         A.3.9 GP commissioning
       */

      /* 1. Select channel */

      /* Set ToggleChannel true */
      /* Set current channel to ZB_ZGPD_FIRST_CH */
      /* Repeat until channel select done or stop { */
      /* Set "RX channel at next attempt" (== last tx channel at next attempt) to
       * current channel + ZB_ZGPD_CH_SERIES * 2. Put it into GPFS. (Really,
       * can calculate it using current channel) */
      /* Send ZB_ZGPD_CH_SERIES Channel Request GPDFS incrementing current channel.  */
      /* Remember time just before sending last GPDFS in series. */
      /* Set ZB_GPD_RX_OFFSET timeout. Drop all incoming packets (imitate RX off) */
      /* After ZB_GPD_RX_OFFSET timeout enable RX on ZB_GPD_MIN_RX_WINDOW  */
      /* If got Channel Configuration GPDF, set channel and break loop */
      /* Sleep for 1 second */
      /* } */

      /* 2. Commissioning GPDF / Commissioning reply */

      /* do { */
      /* Send Commissioning GPDFS with or without extended fields. If security
       * is on, include key there. */
      /* Set ZB_GPD_RX_OFFSET timeout. Drop all incoming packets (imitate RX off) */
      /* After ZB_GPD_RX_OFFSET timeout enable RX on ZB_GPD_MIN_RX_WINDOW */
      /* } while nothing received */

      /* 3. Success */
      /* On receiving Commissioning Reply send encrypted Success GPDF. */
#ifdef ZB_CERTIFICATION_HACKS
      if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CR_START_CH))
      {
        ZGPD->channel = ZGPD->ch_replace_cr_start_ch;
      }
      else
#endif
      ZGPD->channel = ZB_ZGPD_FIRST_CH;
      ZGPD->toggle_channel = ZB_TRUE;
      /* Warning: turning rx off feature is not implemented */
#ifdef ZB_ZGPD_TURN_RX_OFF_ENABLED
      ZGPD->rx_on = ZB_FALSE;
#endif
      TRACE_MSG(TRACE_ZGP1, "bidirectional commissioning. channel %hd toggle_channel T rx_on F", (FMT__H, ZGPD->channel));
      /* send first channel request */
      zb_buf_get_out_delayed(zgpd_set_channel_send_channel_req);
      break;

    default:
      TRACE_MSG(TRACE_ZGP1, "Unknown commissioning method %hd", (FMT__H, ZGPD->commissioning_method));
  }

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_start_commissioning", (FMT__0));
}

#ifndef ZB_ALIEN_SCHEDULER
void zgpd_main_loop()
{
  TRACE_MSG(TRACE_ZGP1, ">> zgpd_main_loop", (FMT__0));

  while (!ZB_SCHEDULER_IS_STOP())
  {
    zb_sched_loop_iteration();
  }

  /* TRACE_MSG(TRACE_ZGP1, "<< zgpd_main_loop", (FMT__0)); - unreachable */
}
#else
void zgpd_main_loop()
{
  MAIN_FUNCTION_NAME();
}
#endif

void zgpd_set_channel_and_call(zb_uint8_t param, zb_callback_t func)
{
  zb_mlme_set_request_t *req;

  req = zb_buf_initial_alloc(param, sizeof(*req));
  /* switch channel and re-send channel req  */
  req->pib_attr = ZB_PHY_PIB_CURRENT_CHANNEL;
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CR_TRANSMIT_CH)&&(ZGPD->toggle_channel == ZB_TRUE))
    {
      *((zb_uint8_t*)(req+1)) = ZGPD->ch_replace_cr_transmit_ch;
    }
  else
#endif
  *((zb_uint8_t*)(req+1)) = ZGPD->channel;
  req->confirm_cb_u.cb = func;
  TRACE_MSG(TRACE_ZGP1, "set channel %hd then run func %p param %hd", (FMT__H_P_H, *((zb_uint8_t*)(req+1)), func, param));
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

  /* For gpdSecurityLevel 0b10 and 0b11, the MAC sequence number field SHOULD carry the 1LSB of the
   * gpdSecurityFrameCounter. (see ZGP spec, A.1.6.4.4) */
static void zgpd_set_dsn_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZGP1, "set security_frame_counter 1LSB after update DSN %hd then run func %p param %hd",
            (FMT__H_P_H, ZGPD->mac_dsn, ZGPD->mac_dsn_cb, param));
  ZB_SCHEDULE_CALLBACK(ZGPD->mac_dsn_cb, param);
}

void zgpd_set_dsn_and_call(zb_uint8_t param, zb_callback_t func)
{
  zb_mlme_set_request_t *req;

  ZGPD->mac_dsn_cb = func;

  req = zb_buf_initial_alloc(param, sizeof(*req));
  req->pib_attr = ZB_PIB_ATTRIBUTE_DSN;
  *((zb_uint8_t*)(req+1)) = ZGPD->mac_dsn;
  req->confirm_cb_u.cb = zgpd_set_dsn_cb;
  TRACE_MSG(TRACE_ZGP1, "set DSN %hd then run func %p param %hd", (FMT__H_P_H, ZGPD->mac_dsn, req->confirm_cb_u.cb, param));
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

void zgpd_set_ieee_and_call(zb_uint8_t param, zb_callback_t func)
{
  zb_mlme_set_request_t *req;

  req = zb_buf_initial_alloc(param, sizeof(*req));
  req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
  req->pib_index = 0;
  req->pib_length = sizeof(zb_64bit_addr_t);
  ZB_MEMCPY((req+1), &ZGPD->id.addr.ieee_addr, req->pib_length);
  req->confirm_cb_u.cb = func;
  TRACE_MSG(TRACE_ZGP1, "set ieee then run func %p param %hd", (FMT__P_H, func, param));
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);
}

void zgpd_set_channel_send_channel_req(zb_uint8_t param)
{
  zgpd_set_channel_and_call(param, zgpd_send_channel_req);
}

void zgpd_send_channel_req(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_uint8_t rx_next_attempt;
  zb_uint8_t rx_next_next_attempt;
  zb_uint8_t auto_comm = 1;

  /* create packet */
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CMD)&&(ZGPD->ch_replace_cmd==ZB_GPDF_CMD_SUCCESS))
  {
    ZGPD->comm_state = ZB_ZGPD_STATE_COMMISSIONED;
    ptr = zb_buf_initial_alloc(param, 1);
  }
  else
#endif
    ptr = zb_buf_initial_alloc(param, 2);
  /* command id */
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CMD))
  {
    ZB_GPDF_PUT_UINT8(ptr, ZGPD->ch_replace_cmd);
    TRACE_MSG(TRACE_ZGP1, "Replace cmd to: 0x%02x", (FMT__H, ZGPD->ch_replace_cmd));
  }
  else
#endif
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_CHANNEL_REQUEST);
  ZGPD->tx_cmd = ZB_GPDF_CMD_CHANNEL_REQUEST;
  /* Bits 0-3:
     Rx channel in the next attempt
     Bits 4-7:
     Rx channel in the second next attempt

     Second next attempt is not used - set zero?
  */
  rx_next_attempt = ((ZGPD->channel - ZB_ZGPD_FIRST_CH) / ZB_ZGPD_CH_SERIES + 2) * ZB_ZGPD_CH_SERIES - 1;
  rx_next_next_attempt = (rx_next_attempt + ZB_ZGPD_CH_SERIES) % (ZB_ZGPD_LAST_CH - ZB_ZGPD_FIRST_CH);

  TRACE_MSG(TRACE_ZGP1, "send channel req: ch %hd next att %hd next next %hd",
            (FMT__H_H_H, ZGPD->channel, rx_next_attempt, rx_next_next_attempt));

#ifdef ZB_CERTIFICATION_HACKS
  if (!(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CMD)&&(ZGPD->ch_replace_cmd==0xe2)))
  {
    if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_CR_NEXT_CH)){
      ZB_GPDF_PUT_UINT8(ptr, ZGPD->ch_replace_cr_next_ch);
    } else
#endif
      ZB_GPDF_PUT_UINT8(ptr, rx_next_attempt | (rx_next_next_attempt << 4));
#ifdef ZB_CERTIFICATION_HACKS
  }
#endif
  if ((ZGPD->channel - ZB_ZGPD_FIRST_CH) % ZB_ZGPD_CH_SERIES == ZB_ZGPD_CH_SERIES - 1)
  {
    auto_comm = 0;
    /* Last channel in the series. Setup alarm to start RX. */
    TRACE_MSG(TRACE_ZGP1, "last gpdf in the series - set alarm %d before rx enable", (FMT__D, ZB_GPD_RX_OFFSET_MS));
#ifdef ZB_CERTIFICATION_HACKS
     if (!ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_EXIT_AFTER_REQ_SERIES))
#endif
    ZB_SCHEDULE_ALARM(zgpd_switch_rx_on, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GPD_RX_OFFSET_MS));
  }

/* was: if (ZGPD->maint_frame_for_channel_req)*/
/* 08/11/2016 [AEV] Change behavior if not comissioned yet */
/* The Channel Request GPDF can use the following values of the Frame Type
   sub-field of the NWK Frame Control field: 0b01 and 0b00.
   - When sent as part of the commissioning procedure, the GPD Channel Request command
       SHALL be sent with Frame Type sub-field of the NWK Frame Control field
       set to 0b01 (Maintenance frame; see sec. A.1.4.1.2).
   - When sent in operational mode, the GPD Channel Request command SHALL be sent with
       Frame Type sub-field of the NWK Frame Control field set to 0b00
       (Data frame; see sec. A.1.4.1.2); it SHALL then be secured with the
       security settings as established during the commissioning.*/
  if (ZGPD->comm_state != ZB_ZGPD_STATE_COMMISSIONED)
  {
  /*
   * When sent as part of the commissioning procedure, the GPD Channel Request command
   * SHALL be sent with Frame Type sub-field of the NWK Frame Control field set to 0b01
   * (Maintenance frame; see sec. A.1.4.1.2).
   * In a Maintenance FrameType, the Auto-Commissioning sub-field, if set to 0b0,
   * indicates that the GPD will enter the receive mode gpdRxOffset ms after
   * completion of this GPDF transmission, for at least gpdMinRxWindow.
   * If the value of this sub-field is 0b1,  then the GPD will not enter
   * the receive mode after sending this particular GPDF.
   */
    ZGPD->auto_commissioning_pending = auto_comm;
    ZB_SEND_MAINTENANCE_GPDF(param);
  }
  else
  {
    zb_zgpd_send_data_req(param, ZB_FALSE);
  }
}

void zgpd_switch_rx_on(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZGP1, "switch rx on at channel %hd (%hd), schedule rx off after %d ms",
            (FMT__H_H_D,
             ZGPD->channel,
             ZGPD->channel, ZB_GPD_MIN_RX_WINDOW_MS));
  ZGPD->rx_on = ZB_TRUE;
  /* TODO: switch RX on in HW */
  ZB_SCHEDULE_ALARM(zgpd_switch_rx_off, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GPD_MIN_RX_WINDOW_MS));
}

void zgpd_switch_rx_off(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ZGP1, "switch rx off. toggle_channel %hd, channel was %dh, comm_state: %d",
            (FMT__H_H_H, ZGPD->toggle_channel, ZGPD->channel, ZGPD->comm_state));
#ifdef ZB_ZGPD_TURN_RX_OFF_ENABLED
  ZGPD->rx_on = ZB_FALSE;
  /* TODO: switch RX off in HW */
#endif

  /* "toggle_channel" means we are in progress of sending Channel requests */
  if (ZGPD->toggle_channel && ZGPD_COMMISSIONING_IN_PROGRESS())
  {
    ZGPD->channel++;
    if (ZGPD->channel > ZB_ZGPD_LAST_CH)
    {
      ZGPD->channel = ZB_ZGPD_FIRST_CH;
    }
    TRACE_MSG(TRACE_ZGP1, "retry channel req after %d", (FMT__D, ZB_GPD_COMMISSIONING_RETRY_INTERVAL));
#ifdef ZB_CERTIFICATION_HACKS
    if (!ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ))
#endif
    ZB_SCHEDULE_ALARM(zgpd_channel_req_retry, 0, ZB_GPD_COMMISSIONING_RETRY_INTERVAL);
  }
}

void zgpd_channel_req_retry(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zgpd_set_channel_send_channel_req);
}

void zb_mcps_data_confirm(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZGP1, ">> zb_mcps_data_confirm param %hd", (FMT__H, param));

  TRACE_MSG(TRACE_ZGP1, "channel %hd toggle_channel %hd commissioned %hd comm_state %hd",
            (FMT__H_H_H_H, ZGPD->channel, ZGPD->toggle_channel, ZGPD_IS_COMMISSIONED(), ZGPD->comm_state));

  if (ZGPD->toggle_channel && ZGPD_COMMISSIONING_IN_PROGRESS())
  {
    if ((ZGPD->channel - ZB_ZGPD_FIRST_CH) % ZB_ZGPD_CH_SERIES == ZB_ZGPD_CH_SERIES - 1)
    {
      /* Last channel in the series. RX alarm is already set - do nothing here. */
      TRACE_MSG(TRACE_ZGP1, "last ch in the series", (FMT__0));
      zb_buf_free(param);
    }
    else
    {
      /* Immediately send next Channel request GPFS on another channel */
      ZGPD->channel = (ZGPD->channel == ZB_ZGPD_LAST_CH) ? ZB_ZGPD_FIRST_CH : ZGPD->channel + 1U;
      TRACE_MSG(TRACE_ZGP1, "new ch %hd, will send set ch req", (FMT__H, ZGPD->channel));
#ifdef ZB_CERTIFICATION_HACKS
      if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ))
      {
        zb_buf_free(param);
      }
      else
#endif
      {
        ZB_SCHEDULE_CALLBACK(zgpd_set_channel_send_channel_req, param);
      }
    }
  }
  else if (ZGPD_IS_COMMISSIONED())
  {
    zb_uint8_t comm_res = (zb_buf_get_status(param) == MAC_SUCCESS) ?
                 ZB_ZGPD_COMM_SUCCESS : ZB_ZGPD_COMM_FAILED;

    if (comm_res != ZB_ZGPD_COMM_SUCCESS)
    {
      ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;
    }

    ZGPD_NOTIFY_COMM_COMPLETE(buf, comm_res);
  }
  else
  {
    zb_buf_free(param);
  }


  TRACE_MSG(TRACE_ZGP1, "<< zb_mcps_data_confirm param %d", (FMT__H, param));
}

void zb_gp_mcps_data_indication(zb_uint8_t param)
{
  zb_mcps_data_indication(param);
}

void zb_mcps_data_indication(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZGP1, ">> zb_mcps_data_indication param %hd rx_on %hd", (FMT__H_H, param, ZGPD->rx_on));

  if (ZGPD->rx_on)
  {
    zb_mac_mhr_t mac_hdr;
    zb_ushort_t mhr_size;
    zb_uint8_t *nwk = NULL;

    DUMP_TRAF("mac ", zb_buf_begin(param), zb_buf_len(param), 0);

    /* parse and remove MAC header */
    mhr_size = zb_parse_mhr(&mac_hdr, param);
    ZB_MAC_CUT_HDR(param, mhr_size, nwk);

    if (ZB_NWK_FRAMECTL_GET_PROTOCOL_VERSION(nwk) == ZB_ZGP_PROTOCOL_VERSION)
    {
      zb_gpdf_info_t *gpdf_info;
      zb_int8_t nwk_hdr_len;
      zb_bool_t sec_success = ZB_TRUE;

      gpdf_info = ZB_BUF_GET_PARAM(param, zb_gpdf_info_t);

      /* EES: we need update nwk pointer because ZB_BUF_GET_PARAM may be perform memmove */
      nwk = zb_buf_begin(param);

      nwk_hdr_len = zgp_parse_gpdf_nwk_hdr(nwk, zb_buf_len(param), gpdf_info);

      if (nwk_hdr_len == 0)
      {
        TRACE_MSG(TRACE_ZGP1, "Error: Can't parse incoming data - drop it", (FMT__0));
      }
      else
      {
        gpdf_info->nwk_hdr_len = nwk_hdr_len;

        if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
        {
          if (ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl) == ZB_ZGP_SEC_LEVEL_REDUCED)
          {
            gpdf_info->mac_addr_flds_len = mhr_size - sizeof(mac_hdr.frame_control) - sizeof(mac_hdr.seq_number);

            ZB_MEMCPY(&gpdf_info->mac_addr_flds.in,
                      (zb_uint8_t*)zb_buf_begin(param) - (gpdf_info->mac_addr_flds_len),
                  gpdf_info->mac_addr_flds_len);
          }

          ZB_MEMCPY(gpdf_info->key, ZGPD->security_key, sizeof(ZGPD->security_key));
          sec_success = (RET_OK==zb_zgp_decrypt_and_auth(param))? ZB_TRUE:ZB_FALSE;
        }

        if (!sec_success)
        {
          TRACE_MSG(TRACE_ZGP1, "Security processing failed - drop data", (FMT__0));
        }
        else
        {
          zb_buf_cut_right(param, ZB_GPDF_MIC_SIZE(ZB_GPDF_EXT_NFC_GET_SEC_LEVEL(gpdf_info->nwk_ext_frame_ctl)));
          zb_buf_cut_left(param, nwk_hdr_len);
          TRACE_MSG(TRACE_ZGP1, "Security processing OK, toggle ch=%d cmd=0x%02x", (FMT__H_H, ZGPD->toggle_channel, nwk[nwk_hdr_len]));

          if (ZGPD->toggle_channel
              && nwk[nwk_hdr_len] == ZB_GPDF_CMD_CHANNEL_CONFIGURATION)
          {
            TRACE_MSG(TRACE_ZGP1, "got channel configuration", (FMT__0));
/* 07/29/14 NK: CR {  */
            ZB_SCHEDULE_CALLBACK(ZGPD_FN(zgpd_channel_config_ind), param);
            param = 0;
          }
          else if (!ZGPD->toggle_channel
                   && nwk[nwk_hdr_len] == ZB_GPDF_CMD_COMMISSIONING_REPLY)
          {
            TRACE_MSG(TRACE_ZGP1, "got commissioning reply", (FMT__0));
            ZB_SCHEDULE_CALLBACK(ZGPD_FN(zgpd_commiss_reply_ind), param);
            param = 0;
/* 07/29/14 NK: CR }  */
          }
        }
      }
    } /* if GP */
    else
    {
      TRACE_MSG(TRACE_ZGP2, "unhandled GP packet - drop data", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ZGP2, "RX is off - drop data", (FMT__0));
  }
  if (param)
  {
    zb_buf_free(param);
  }
  TRACE_MSG(TRACE_ZGP1, "<< zb_mcps_data_indication", (FMT__0));
}

void zgpd_channel_config_ind(zb_uint8_t param)
{
  zb_uint8_t payload_ch = *((zb_uint8_t*)zb_buf_begin(param)+1) & 0x0f;
  ZGPD->toggle_channel = ZB_FALSE;
  ZGPD->channel = payload_ch + ZB_ZGPD_FIRST_CH;

  TRACE_MSG(TRACE_ZGP1, "zgpd_channel_config_ind set channel %hd, toggle_channel F", (FMT__H, ZGPD->channel));
#ifdef ZB_CERTIFICATION_HACKS
  if ((ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_EXIT_AFTER_CH_REQ)))
    zb_buf_free(param);
  else
#endif
  ZB_SCHEDULE_CALLBACK(zgpd_set_channel_send_commiss_req, param);
}

void zgpd_set_channel_send_commiss_req(zb_uint8_t param)
{
  zgpd_set_channel_and_call(param, zgpd_send_commissioning_req);
}

void zgpd_commiss_req_retry(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zgpd_set_channel_send_commiss_req);
}

static void zgpd_fill_app_info(zb_uint8_t param)
{
  zb_uint8_t  size = 1;
  zb_uint8_t *ptr;

  if (ZGPD->app_info_options.manuf_id_present)
  {
    size += 2;
  }

  if (ZGPD->app_info_options.manuf_model_id_present)
  {
    size += 2;
  }

  if (ZGPD->app_info_options.gpd_cmds_present)
  {
    ZB_ASSERT(ZGPD_MAX_MS_CMDS != 0);
    size += (1 + ZGPD->gpd_cmds[0]);
  }

  if (ZGPD->app_info_options.cluster_list_present)
  {
    ZB_ASSERT(ZGPD_MAX_MS_CLUSTERS != 0);
    size += (1 + ((ZGPD_MS_CLUSTERS_GET_SRV_COUNT() + ZGPD_MS_CLUSTERS_GET_CLI_COUNT()) * sizeof(ZGPD->clsts_list[0])));
  }

  if (ZGPD->app_info_options.switch_info_present)
  {
    size += sizeof(zb_gpdf_comm_switch_info_t);
  }

  TRACE_MSG(TRACE_ZGP1, "MS size: %hd", (FMT__H, size));

  ptr = zb_buf_alloc_right(param, size);
  {
    zb_uint8_t opt = *((zb_uint8_t*) &ZGPD->app_info_options);
    ZB_GPDF_PUT_UINT8(ptr, opt);
  }

  if (ZGPD->app_info_options.manuf_id_present)
  {
    ZB_GPDF_PUT_UINT16(ptr, &ZGPD->manuf_id);
  }

  if (ZGPD->app_info_options.manuf_model_id_present)
  {
    ZB_GPDF_PUT_UINT16(ptr, &ZGPD->manuf_model_id);
  }

  if (ZGPD->app_info_options.gpd_cmds_present)
  {
    zb_uindex_t i;

    ZB_GPDF_PUT_UINT8(ptr, ZGPD->gpd_cmds[0]);
    for (i = 1; i <= ZGPD->gpd_cmds[0]; i++)
    {
      ZB_GPDF_PUT_UINT8(ptr, ZGPD->gpd_cmds[i]);
    }
  }

  if (ZGPD->app_info_options.cluster_list_present)
  {
    zb_uindex_t i;
    zb_uint8_t clst_cnt = ZGPD_MS_CLUSTERS_GET_SRV_COUNT() + ZGPD_MS_CLUSTERS_GET_CLI_COUNT();

    TRACE_MSG(TRACE_ZGP1, "clst_cnt: %hd", (FMT__H, clst_cnt));

    ZB_GPDF_PUT_UINT8(ptr, ZGPD->clsts_list_size);
    for (i = 0; i <= clst_cnt; i++)
    {
      ZB_GPDF_PUT_UINT16(ptr, &ZGPD->clsts_list[i]);
    }
  }

  if (ZGPD->app_info_options.switch_info_present)
  {
    zb_uint8_t sw_config = *((zb_uint8_t*) &ZGPD->switch_config);
    ZB_GPDF_PUT_UINT8(ptr, 2); /* len */
    ZB_GPDF_PUT_UINT8(ptr, sw_config);
    ZB_GPDF_PUT_UINT8(ptr, 0); /* current contact status */
  }

  TRACE_MSG(TRACE_ZGP1, "zgpd_send_commissioning_req manuf_id 0x%hx manuf_model_id 0x%hx",
            (FMT__H_H, ZGPD->manuf_id, ZGPD->manuf_model_id));
}

static void zgpd_put_app_descr_report(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_uint8_t options;
  zb_uint8_t len;
  zgp_report_desc_t *report = &ZGPD->app_descr.reports[ZGPD->app_descr.next_report];

  len = 3 + report->point_descs_data_len;

  if (report->options.timeout_present)
  {
    len += 2;
  }

  ptr = zb_buf_alloc_right(param, len);

  ZB_GPDF_PUT_UINT8(ptr, ZGPD->app_descr.next_report); /* Report Identifier*/

  options = *((zb_uint8_t *) &report->options);
  ZB_GPDF_PUT_UINT8(ptr, options);

  if (report->options.timeout_present)
  {
    ZB_GPDF_PUT_UINT16(ptr, &report->timeout);
  }

  ZB_GPDF_PUT_UINT8(ptr, report->point_descs_data_len);

  /* todo: */
  ZB_MEMCPY(ptr, report->point_descs_data, report->point_descs_data_len);
}

static void zgpd_send_app_description_cont(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  ptr = zb_buf_initial_alloc(param, 3);

  ZGPD->tx_cmd = ZB_GPDF_CMD_APPLICATION_DESCR;
  ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_APPLICATION_DESCR);
  ZB_GPDF_PUT_UINT8(ptr, ZGPD->app_descr.total_reports);
  ZB_GPDF_PUT_UINT8(ptr, 1);

  zgpd_put_app_descr_report(param);

  ZGPD->app_descr.next_report++;
  if (ZGPD->app_descr.next_report == ZGPD->app_descr.total_reports)
  {
    ZGPD->app_descr.next_report = 0;
  }

  zb_zgpd_send_data_req(param, ZB_FALSE);

  if (ZGPD->app_descr.next_report == 0)
  {
    ZGPD->app_info_options.app_descr_flw = 0;
    if (ZGPD->commissioning_method == ZB_ZGPD_COMMISSIONING_BIDIR)
    {
      ZB_SCHEDULE_ALARM(zgpd_switch_rx_on, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GPD_RX_OFFSET_MS));
    }
  }
  else
  {
    zb_buf_get_out_delayed(zgpd_send_app_description);
  }
}

void zgpd_send_app_description(zb_uint8_t param)
{
  ZB_SCHEDULE_ALARM(zgpd_send_app_description_cont, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

void zgpd_send_commissioning_req(zb_uint8_t param)
{
  zb_uint8_t* ptr;
  zb_uint8_t ext_opt_fld_size = (ZGPD->security_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY) ? 1 : 0;
  zb_uint8_t s_k_p,s_k_e;
  zb_uint8_t secur_lvl = ZGPD->security_level;
#ifdef ZB_CERTIFICATION_HACKS
  zb_bool_t insert_key_data = ZB_TRUE;
#endif

  TRACE_MSG(TRACE_ZGP1, ">> zgpd_send_commissioning_req param %hd", (FMT__H, param));

  if (ZGPD->comm_state == ZB_ZGPD_STATE_COMMISSIONED)
  {
    zb_buf_free(param);
    return;
  }

  if (ZGPD->commissioning_method == ZB_ZGPD_COMMISSIONING_BIDIR)
  {
#ifdef ZB_CERTIFICATION_HACKS
    if (!ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ))
#endif
      ZB_SCHEDULE_ALARM(zgpd_commiss_req_retry, 0, ZB_GPD_COMMISSIONING_RETRY_INTERVAL);
  }

  /* TODO: Security fields filling for other than individual key cases */

  ptr = zb_buf_initial_alloc(param, 3 + ext_opt_fld_size);
  ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_COMMISSIONING);
  ZGPD->tx_cmd = ZB_GPDF_CMD_COMMISSIONING;
  ZB_GPDF_PUT_UINT8(ptr, ZGPD->device_id);

/* A.1.7.3 The GPD SHOULD only request the key by setting GPD Security
   key request to 0b1, if it supports security,
   i.e. if the Security level capabilities sub-field of the Extended
   Options field of the GPD Commissioning command is set to 0b10 or 0b11.*/
/* A.3.9.1 If GPD outgoing counter field is present in the payload of the GPD Commissioning
   command (and it SHALL if SecurityLevel capabilities sub-field of the Extended Options
   field is set to 0b10 or 0b11) */
  if(secur_lvl > ZB_ZGP_SEC_LEVEL_REDUCED)
  {
    ZGPD->gpd_outgoing_counter_present = 1;

    if((!ZGPD->gpd_security_key_request)&&(ZGPD->security_key_type==ZB_ZGP_SEC_KEY_TYPE_NO_KEY))
    {
      TRACE_MSG(TRACE_ERROR, "ERROR! SEC_LEVEL=%d (not off), but you don't request key and GPD key_type set to NO_KEY!", (FMT__H, secur_lvl));
      ZB_ASSERT(0);
    }
  }

  {
    zb_uint8_t opt = ZB_GPDF_COMM_OPT_FLD(
                        !ZGPD->use_random_seq_num,
                        ZGPD->rx_on_capability,         /* rx on capability */
                        ZGPD->application_info_present,
                        ZGPD->pan_id_request,           /* PAN ID request */
                        ZGPD->gpd_security_key_request,  /* GP security key request */
                        ZGPD->fixed_location,           /* Fixed location */
                        ext_opt_fld_size);              /* Extended options field present */

#ifdef ZB_CERTIFICATION_HACKS
    if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_COMM_OPT_RESERVED_SET))
    {
      opt |= (1 << 3);
    }
#endif
    ZB_GPDF_PUT_UINT8(ptr, opt);
  }

/*A GPD setting GPD Security key request to 0b1 SHALL also set the GPD Key present sub-field of
  the Extended Options field of the the Commissioning GPDF and include correctly protected GPD
  Key field. This is done to allow the Combo Basic devices according to the current specification,
  which may not be capable of delivering a shared key, to use the OOB key instead. */
  if( (ZGPD->gpd_security_key_request) || (ZGP_KEY_TYPE_IS_INDIVIDUAL(ZGPD->security_key_type)) )
  {
    if(ZGPD->oob_key_present==ZB_FALSE)
    {
      TRACE_MSG(TRACE_ERROR, "ERROR! Must supply OOB key!", (FMT__0));
      ZB_ASSERT(0);
    }
  }

  if(ZGPD->security_key_present)
  {
    /*  [AEV] This is working mode, Do not force encryption if you want testing */
      ZGPD->security_key_encryption = 1;
  }

  s_k_p = ZGPD->security_key_present;
  s_k_e = ZGPD->security_key_encryption;
#ifdef ZB_CERTIFICATION_HACKS
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SKP))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_SKP: %hd -> %hd", (FMT__H_H, s_k_p, ZGPD->ch_replace_security_key_present));
    s_k_p = ZGPD->ch_replace_security_key_present; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_SKE))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_SKE: %hd -> %hd", (FMT__H_H, s_k_e, ZGPD->ch_replace_security_key_encrypted));
    s_k_e = ZGPD->ch_replace_security_key_encrypted; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_SK))
  { TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_INSERT_SK: 1 -> %hd", (FMT__H, ZGPD->ch_insert_security_key));
      if(ZGPD->ch_insert_security_key==0)insert_key_data = ZB_FALSE; }
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_REPLACE_COMM_SEC_LEVEL))
  {
    TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_REPLACE_COMM_SEC_LEVEL: %hd -> %hd",
              (FMT__H_H, ZGPD->security_level, ZGPD->ch_replace_comm_sec_level));
    secur_lvl = ZGPD->ch_replace_comm_sec_level;
  }
#endif

  if (ext_opt_fld_size)
  {
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_COMM_EXT_OPT_FLD(
        secur_lvl,
        ZGPD->security_key_type,
        s_k_p,
        s_k_e,
        ZGPD->gpd_outgoing_counter_present)
    );
  }

  TRACE_MSG(TRACE_ZGP1, "zgpd_send_commissioning_req deviceId 0x%hx", (FMT__H, ZGPD->device_id));

  /* Always use Out-Of-Box key and encrypt it with TCLK key */
  if (secur_lvl > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
  {
    if(ZGPD->security_key_present)
    {
      if(s_k_e)
      {
        zb_uint8_t encrypted_key[ZB_CCM_KEY_SIZE];
        zb_uint8_t mic[ZB_CCM_M];
        zb_uint8_t alloc_size = ZB_CCM_M + sizeof(ZGPD->security_frame_counter);
#ifdef ZB_CERTIFICATION_HACKS
        if(insert_key_data)
#endif
          alloc_size+=ZB_CCM_KEY_SIZE;

        ptr = zb_buf_alloc_right(
          param,
          alloc_size);

        TRACE_MSG(TRACE_ERROR, "encrypting zgpd oob with gp_link_key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(ZGPD->gp_link_key)));
        zb_zgp_protect_gpd_key(ZB_TRUE, &ZGPD->id, ZGPD->oob_key, ZGPD->gp_link_key, encrypted_key, ZGPD->security_frame_counter, mic);
#ifdef ZB_CERTIFICATION_HACKS
        if(insert_key_data)
#endif
        {
          ZB_MEMCPY(ptr, encrypted_key, ZB_CCM_KEY_SIZE);
          ptr += ZB_CCM_KEY_SIZE;
        }
#ifdef ZB_CERTIFICATION_HACKS
        if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_CORRUPT_COMM_MIC))
        {
          zb_uint_t i;

          TRACE_MSG(TRACE_ZGP1, "CH: ZB_ZGPD_CH_CORRUPT_COMM_MIC", (FMT__0));
          for (i = 0; i < ZB_CCM_M; ++i)
          {
            mic[i] ^= 0xFF;
          }
        }
#endif
        ZB_MEMCPY(ptr, mic, ZB_CCM_M);
        ptr += ZB_CCM_M;
      }
      else
      {
        zb_uint8_t alloc_size = sizeof(ZGPD->security_frame_counter);
        #ifdef ZB_CERTIFICATION_HACKS
        if(insert_key_data)
        #endif
          alloc_size+=ZB_CCM_KEY_SIZE;

        ptr = zb_buf_alloc_right(
          param,
          alloc_size);
#ifdef ZB_CERTIFICATION_HACKS
        if(insert_key_data)
#endif
        {
          ZB_MEMCPY(ptr, ZGPD->oob_key, ZB_CCM_KEY_SIZE);
          ptr += ZB_CCM_KEY_SIZE;
        }
      }
    }
    else
    {
      ptr = zb_buf_alloc_right(
        param,
        sizeof(ZGPD->security_frame_counter));
    }

    /* the MAC sequence number SHALL be incremental; it MAY but is not required to be aligned with
     * the GPD outgoing counter field in the payload of the GPD Commissioning command. */
    if(ZGPD->gpd_outgoing_counter_present)
    {
      ZB_GPDF_PUT_UINT32(ptr, &ZGPD->security_frame_counter);
    }
  }

  if (ZGPD->application_info_present)
  {
    zgpd_fill_app_info(param);
  }

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_INSERT_COMM_EXTRA_PLD))
  {
    zb_uint8_t *ptr;
    zb_uint8_t  i;

    ptr = zb_buf_alloc_right(param, ZGPD->ch_comm_extra_payload);
    for (i = 0; i < ZGPD->ch_comm_extra_payload; i++)
    {
      ZB_GPDF_PUT_UINT8(ptr, i + ZGPD->ch_comm_extra_payload_start_byte);
    }

    TRACE_MSG(TRACE_ZGP1, "bytes_writen: %hd", (FMT__H, zb_buf_len(param)));
  }
#endif

#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_MAX_COMM_PAYLOAD))
  {
    zb_uint8_t  bytes_writen = zb_buf_len(param);
    zb_uint8_t  remaining = 0;
    zb_uint8_t *ptr;
    zb_uint8_t  i;

    TRACE_MSG(TRACE_ZGP1, "ch_max_comm_payload: %hd", (FMT__H, ZGPD->ch_max_comm_payload));

    remaining = ((ZGPD->ch_max_comm_payload > bytes_writen) ? ((ZGPD->ch_max_comm_payload - bytes_writen) + 1) : 0);

    TRACE_MSG(TRACE_ZGP1, "bytes_writen: %hd, remaining: %hd", (FMT__H_H, bytes_writen, remaining));

    ptr = zb_buf_alloc_right(param, remaining);
    for (i = 1; i <= remaining; i++)
    {
      ZB_GPDF_PUT_UINT8(ptr, i);
    }

    TRACE_MSG(TRACE_ZGP1, "bytes_writen: %hd", (FMT__H, zb_buf_len(param)));
  }
#endif

  zb_zgpd_send_data_req(param, ZB_FALSE);

  if (ZGPD->app_info_options.app_descr_flw)
  {
    zb_buf_get_out_delayed(zgpd_send_app_description);
  }
  else
  {
    if (ZGPD->commissioning_method == ZB_ZGPD_COMMISSIONING_BIDIR)
    {
      /* it will not work with real timeout of 5ms. May need to implement
       * some other way or just switch RX always on. */
  #ifdef ZB_CERTIFICATION_HACKS
      if (!ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_STOP_AFTER_1_COMMREQ))
  #endif
      ZB_SCHEDULE_ALARM(zgpd_switch_rx_on, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_GPD_RX_OFFSET_MS));
    }
  }

  TRACE_MSG(TRACE_ZGP1, "<< zgpd_send_commissioning_req", (FMT__0));
}

static void zgpd_send_success_cmd(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_uint8_t protect = ZB_TRUE;

  ptr = zb_buf_initial_alloc(param, 1);
  ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_SUCCESS);
  ZGPD->tx_cmd = ZB_GPDF_CMD_SUCCESS;

  TRACE_MSG(TRACE_ZGP1, "send Success GPDF", (FMT__0));
#ifdef ZB_CERTIFICATION_HACKS
  if (ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_UNPROTECT_SUCCESS)) protect = ZB_FALSE;
  if (!ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_DO_NOT_SEND_SUCCESS))
#endif
    zb_zgpd_send_data_req(param, protect);
}

void zgpd_send_success_cmd_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zgpd_send_success_cmd);
}

/**
 * @brief Parse commissioning reply payload received from ZGPS
 *
 * @param buf_ptr [in]   Pointer to commissioning command payload
 * @param params  [out]  Pointer to parsed commissioning parameters
 */
static void zb_zgpd_parse_commissioning_reply(zb_uint8_t *buf_ptr, zb_gpdf_comm_reply_t *params)
{
  TRACE_MSG(TRACE_ZGP2, ">> zb_zgpd_parse_commissioning_reply, buf_ptr %p, params %p",
                        (FMT__H_P, buf_ptr, params));

  params->options = *buf_ptr++;

  if (ZB_GPDF_COMM_REPLY_PAN_ID_PRESENT(params->options))
  {
    ZB_LETOH16(&params->pan_id, buf_ptr);
    TRACE_MSG(TRACE_ZGP3, "pan_id : 0x%x", (FMT__D, params->pan_id));
    buf_ptr += 2;
  }

  if (ZB_GPDF_COMM_REPLY_SEC_KEY_PRESENT(params->options))
  {
    ZB_MEMCPY(params->security_key, buf_ptr, sizeof(params->security_key));
    TRACE_MSG(TRACE_ZGP3, "key    : " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(params->security_key)));
    buf_ptr += sizeof(params->security_key);
  }

  if (ZB_GPDF_COMM_REPLY_SEC_KEY_ENCRYPTED(params->options))
  {
    ZB_MEMCPY(params->key_mic, buf_ptr, 4);
    TRACE_MSG(TRACE_ZGP3, "key_mic: 0x%02x%02x%02x%02x"  , (FMT__H_H_H_H, params->key_mic[0],params->key_mic[1],params->key_mic[2],params->key_mic[3]));
    buf_ptr += 4;
  }
/*A.4.2.1.2   Commissioning Reply command
  The Frame Counter field is only present when the sub-fields of the Options field are set as follows:
  SecurityLevel sub-field to 0b10 or 0b11, GPDsecurityKeyPresent sub-field to 0b1 and the GPDkeyEn-
  cryption sub-field to 0b1; otherwise it is absent. It carries the security frame counter value that was
  used to encrypt the shared security key transmitted (see A.3.7.1.2.3).*/
  if((ZB_GPDF_COMM_REPLY_SEC_LEVEL(params->options)>=ZB_ZGP_SEC_LEVEL_FULL_NO_ENC) &&
    ZB_GPDF_COMM_REPLY_SEC_KEY_PRESENT(params->options) &&
    ZB_GPDF_COMM_REPLY_SEC_KEY_ENCRYPTED(params->options))
  {
    ZB_LETOH32(&params->frame_counter, buf_ptr);
    TRACE_MSG(TRACE_ZGP3, "frame_counter: 0x%08x", (FMT__L, params->frame_counter));
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgpd_parse_commissioning_reply", (FMT__0));
}

/* Line: 5711 Note:659
  If gpdSecurityLevel = 0b11, the Success GPDF SHALL be secured SecurityLevel = 0b11. */
void zgpd_commiss_reply_ind(zb_uint8_t param)
{
  zb_uint8_t *ptr = zb_buf_begin(param);
  zb_bool_t key_auth_success = ZB_FALSE;
  zb_gpdf_comm_reply_t reply;

  TRACE_MSG(TRACE_ZGP1, "zgpd_commissioning_reply_ind param %d", (FMT__H, param));

#ifdef ZB_CERTIFICATION_HACKS
  if(ZB_ZGPD_CHACK_GET(ZB_ZGPD_CH_SKIP_FIRST_COMM_REPLY))
  {
    TRACE_MSG(TRACE_ZGP1, "ZGPD->ch_skip_first_n_comm_reply = %d", (FMT__H, ZGPD->ch_skip_first_n_comm_reply));
    if((ZGPD->ch_skip_first_n_comm_reply)>0)
    {
      TRACE_MSG(TRACE_ZGP1, "<< zgpd_commissioning_reply_ind return by hack", (FMT__0));
      ZGPD->ch_skip_first_n_comm_reply--;
      zb_buf_free(param);
      return;
    }
  }
#endif

  ZB_SCHEDULE_ALARM_CANCEL(zgpd_commiss_req_retry, 0);

  zb_zgpd_parse_commissioning_reply(ptr+1, &reply);

  /* Import key if needed */
  if(ZGPD->gpd_security_key_request == 1)
  {
    if (ZB_GPDF_COMM_REPLY_SEC_KEY_PRESENT(reply.options))
    {
      ZGPD->security_key_type = ZB_GPDF_COMM_REPLY_SEC_KEY_TYPE(reply.options);
      TRACE_MSG(TRACE_ZGP1, "key type: %d", (FMT__H, ZGPD->security_key_type));
      switch(ZGPD->security_key_type)
      {
        case ZB_ZGP_SEC_KEY_TYPE_NO_KEY:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_NO_KEY", (FMT__0));
        break;
        case ZB_ZGP_SEC_KEY_TYPE_NWK:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_NWK", (FMT__0));
        break;
        case ZB_ZGP_SEC_KEY_TYPE_GROUP:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_GROUP", (FMT__0));
        break;
        case ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_GROUP_NWK_DERIVED", (FMT__0));
        break;
        case ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_ZGPD_INDIVIDUAL", (FMT__0));
        break;
        case ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL:
          TRACE_MSG(TRACE_ZGP3, "ZB_ZGP_SEC_KEY_TYPE_DERIVED_INDIVIDUAL", (FMT__0));
        break;
        default:
          TRACE_MSG(TRACE_ZGP3, "not supported key type", (FMT__0));
          ZB_ASSERT(0);
        break;
      }

      if(ZB_GPDF_COMM_REPLY_SEC_KEY_ENCRYPTED(reply.options))
      {
        zb_uint8_t plain_key[ZB_CCM_KEY_SIZE];
        zb_uint32_t sfc_mem = ZGPD->security_frame_counter;
        ZGPD->security_frame_counter = reply.frame_counter;

        key_auth_success = (zb_bool_t)(zb_zgp_decrypt_n_auth_gpd_key(
            ZB_FALSE,  /* Is frame from GPD */
            &ZGPD->id,
            ZGPD->gp_link_key,
            reply.security_key,
            reply.frame_counter,
            reply.key_mic,
            plain_key) == RET_OK);
        if (key_auth_success)
        {
          TRACE_MSG(TRACE_ZGP1, "decrypted GPD key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(plain_key)));
          ZB_MEMCPY(ZGPD->security_key, plain_key, ZB_CCM_KEY_SIZE);
        }
        else
        {
          TRACE_MSG(TRACE_ZGP1, "Error decrypting GPD key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(plain_key)));
        }
        ZGPD->security_frame_counter = sfc_mem;
      }
      else
      {
        ZB_MEMCPY(ZGPD->security_key, reply.security_key, ZB_CCM_KEY_SIZE);
        key_auth_success = ZB_TRUE;
      }
    }
    else
    {
      TRACE_MSG(TRACE_ZGP1, "Security key requested but not received, will use our OOB key", (FMT__0));
      key_auth_success = ZB_TRUE;
    }
  }

  if( ZGPD->security_level == ZB_ZGP_SEC_LEVEL_NO_SECURITY )
  {
    key_auth_success = ZB_TRUE;
  }
  if(( ZGPD->security_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY )&&( ZGPD->gpd_security_key_request ==0 ))
  {
    key_auth_success = ZB_TRUE;
  }

  if(ZGPD->pan_id_request == 1)
  {
    if (ZB_GPDF_COMM_REPLY_PAN_ID_PRESENT(reply.options))
    {
      ZGPD->pan_id = reply.pan_id;
    }
    else
    {
      TRACE_MSG(TRACE_ZGP1, "PAN_ID requested but not received!", (FMT__0));
      ZB_ASSERT(0);
    }
  }

  /* send Success GPDF */
  if (key_auth_success)
  {
    zgpd_send_success_cmd(param);
#ifdef ZB_CERTIFICATION_HACKS
    if(ZGPD->ch_resend_success_gpdf>0)
    {
      TRACE_MSG(TRACE_ZGP1, "Resend Success GPDF after : %d * 0.1sec", (FMT__H, ZGPD->ch_resend_success_gpdf));
      ZB_SCHEDULE_ALARM(zgpd_send_success_cmd_delayed, 0, ZB_TIME_ONE_SECOND*0.1*ZGPD->ch_resend_success_gpdf);
      ZGPD->ch_resend_success_gpdf = 0;
    }
#endif

    /*
    ZB_BUF_INITIAL_ALLOC(param, 1, ptr);
    ZB_GPDF_PUT_UINT8(ptr, ZB_GPDF_CMD_SUCCESS);
    ZGPD->tx_cmd = ZB_GPDF_CMD_SUCCESS;

    TRACE_MSG(TRACE_ZGP1, "send Success GPDF", (FMT__0));
    ZGPD->comm_state = ZB_ZGPD_STATE_COMM_SENT_SUCCESS;
    zb_zgpd_send_data_req(param, ZB_TRUE);
    */
    ZGPD->toggle_channel = ZB_TRUE;
    ZGPD->comm_state = ZB_ZGPD_STATE_COMMISSIONED;
    TRACE_MSG(TRACE_ZGP1, "toggle_channel T commissioned T", (FMT__0));
  }
}

void zb_zgpd_decommission_cont(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_decommission_cont", (FMT__0));

  ZB_SEND_PAYLOADLESS_GPDF(param, ZB_GPDF_CMD_DECOMMISSIONING, ZB_TRUE);
  ZGPD->comm_state = ZB_ZGPD_STATE_NOT_COMMISSIONED;

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_decommission_cont", (FMT__0));
}

void zb_zgpd_decommission()
{
  TRACE_MSG(TRACE_ZGP1, ">> zb_zgpd_decommission", (FMT__0));

  zb_buf_get_out_delayed(zb_zgpd_decommission_cont);

  TRACE_MSG(TRACE_ZGP1, "<< zb_zgpd_decommission", (FMT__0));
}

static void get_pib_attribute_dsn_cb(zb_uint8_t param)
{
  zb_mlme_get_confirm_t *conf = (zb_mlme_get_confirm_t *)zb_buf_begin(param);
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZGP2, ">> get_pib_attribute_dsn_cb, param %hd", (FMT__H, param));

  ret = conf->status;

  if (ret == RET_OK)
  {
    ZB_ASSERT(conf->pib_length = sizeof(zb_uint8_t));

    ZGPD->security_frame_counter = *(zb_uint8_t *)(conf+1)
    /* NK : Some non-DSR MAC increments DSN before sending packet, when our mac - after,
       so need to additionally inc security_frame_counter
    */
#ifdef ZB_MAC_INCREMENTS_GP_CNT_AFTER
      + 1
#endif
      ;
  }

  STARTUP_COMPLETE(buf, ret);

  TRACE_MSG(TRACE_ZGP2, "<< get_pib_attribute_dsn_cb", (FMT__0));
}

void zb_mlme_set_confirm(zb_uint8_t param)
{

  TRACE_MSG(TRACE_ZGP2, ">> zb_mlme_set_confirm, param %hd", (FMT__H, param));

  /** From ZGP spec, A.1.6.4.4:
   * For gpdSecurityLevel 0b01, the MAC sequence number field shall carry the 1LSB of the
   * gpdSecurityFrameCounter.
   * For gpdSecurityLevel 0b10 and 0b11, the MAC sequence number field should carry the 1LSB of the
   * gpdSecurityFrameCounter.
   */
  if (ZGPD->security_level > ZB_ZGP_SEC_LEVEL_NO_SECURITY)
  {
    zb_mlme_get_request_t *req;

    req = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_request_t));
    req->pib_attr   = ZB_PIB_ATTRIBUTE_DSN;
    req->pib_index  = 0;
    req->confirm_cb_u.cb = get_pib_attribute_dsn_cb;

    zb_mlme_get_request(param);
  }
  else
  {
    STARTUP_COMPLETE(buf, RET_OK);
  }

  TRACE_MSG(TRACE_ZGP2, "<< zb_mlme_set_confirm", (FMT__0));
}


void zb_mlme_reset_confirm(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;

  TRACE_MSG(TRACE_ZGP2, ">> zb_mlme_reset_confirm, param %hd", (FMT__H, param));

  /* set rx_on_when_idle, specified by app. Defaut is true */
  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + 1);
  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_index = 0;
  req->pib_length = 1;
  ZB_MEMCPY((req+1), &(ZGPD->rx_on_when_idle), 1);
  req->confirm_cb_u.cb = rx_on_when_idle_confirm;
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_ZGP2, "<< zb_mlme_reset_confirm", (FMT__0));
}


static void rx_on_when_idle_confirm(zb_uint8_t param)
{
  zb_uint8_t page;
  zb_mlme_set_request_t *req;

  TRACE_MSG(TRACE_ZGP3, ">> rx_on_when_idle_confirm param %hd", (FMT__H, param));

  /* set channel page. Defaut is 0 (2.4GHz) */
  page = 0;
  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + 1);
  req->pib_attr = ZB_PHY_PIB_CURRENT_PAGE;
  req->pib_index = 0;
  req->pib_length = 1;
  ZB_MEMCPY((req+1), &page, 1);
  req->confirm_cb_u.cb = zb_mlme_set_confirm;
  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_ZGP3, "<< rx_on_when_idle_confirm", (FMT__0));
}

static void zb_zgpd_dev_start_cont(zb_uint8_t param)
{
  zb_mlme_reset_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgpd_dev_start_cont, param %hd", (FMT__H, param));

  req->set_default_pib = ZB_TRUE;

  ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgpd_dev_start_cont", (FMT__0));
}

zb_ret_t zb_zgpd_dev_start(zb_callback_t cb)
{
  zb_ret_t ret = RET_OK;

  TRACE_MSG(TRACE_ZGP2, ">> zb_zgpd_dev_start, cb %p", (FMT__P, cb));

  ZGPD->rx_on                        = ZB_TRUE;
  ZGPD->startup_cb                   = cb;

  zb_buf_get_out_delayed(zb_zgpd_dev_start_cont);

  TRACE_MSG(TRACE_ZGP2, "<< zb_zgpd_dev_start, ret %hd", (FMT__H, ret));

  return ret;
}

void zb_zgpd_device_reset_security(void)
{
	#if 0
  zb_uint8_t key[] = ZB_STANDARD_TC_KEY;
	#else
  zb_uint8_t key[];
  ZB_MEMCPY(&(key), ZB_STANDARD_TC_KEY, ZB_CCM_KEY_SIZE);
	#endif

  ZB_MEMCPY(g_zgpd_ctx.gp_link_key,key,ZB_CCM_KEY_SIZE);
  TRACE_MSG(TRACE_ZGP1, "set default gp_link_key: " TRACE_FORMAT_128, (FMT__A_A, TRACE_ARG_128(g_zgpd_ctx.gp_link_key)));
}

/* When we use ZGP build we have no implementation of
 * function zb_nwk_unlock_in from NWK
 * so i put here simple function stub
 */
void zb_nwk_unlock_in(zb_uint8_t param)
{
  ZVUNUSED(param);
}

#endif  /* ZB_ZGPD_ROLE */

/** @} */
