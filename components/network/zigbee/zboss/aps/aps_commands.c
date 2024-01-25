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
/* PURPOSE: Incoming APS commands handle
*/

#define ZB_TRACE_FILE_ID 2139
#include "zb_common.h"
#include "zb_aps.h"
#include "aps_internal.h"
#include "zb_secur.h"


/*! \addtogroup ZB_APS */
/*! @{ */


void zb_aps_in_command_handle(zb_uint8_t param, zb_uint16_t keypair_i,
                              zb_secur_key_id_t key_id)
{
  zb_uint8_t cmd_id;
  zb_bool_t secured;
  zb_uint16_t src_short = ((zb_apsde_data_indication_t *)ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t))->src_addr;

  ZB_TH_PUSH_PACKET(ZB_TH_APS_COMMAND, ZB_TH_PRIMITIVE_INDICATION, param);

  {
    zb_ushort_t hdr_size = sizeof(zb_aps_command_pkt_header_t);
    zb_aps_command_pkt_header_t *hdr = (zb_aps_command_pkt_header_t *)zb_buf_begin(param);

    secured = ZB_U2B(ZB_APS_FC_GET_SECURITY(hdr->fc));
    if (secured)
    {
      hdr_size += zb_aps_secur_aux_size(*(zb_uint8_t*)(hdr + 1));
      TRACE_MSG(TRACE_SECUR3, "got secured aps cmd frame, hdr+aux size %hd", (FMT__H, hdr_size));
    }
    else
    {
      /* R21: According to the TP_R20_BV-01, TC should reject an APS commands without APS
       * encryption when unique link key is in use. R21 spec is unclear in this part, so change this
       * behavior if needed when it will be fixed. */
      /* Try to find keypair_i even if pkt is unsecured - to detect if key exists but not used */
      zb_ieee_addr_t src_ieee_addr;
      if (zb_address_ieee_by_short(src_short, src_ieee_addr) == RET_OK)
      {
        keypair_i = zb_aps_keypair_get_index_by_addr(src_ieee_addr, ZB_SECUR_ANY_KEY_ATTR);
      }
    }

    {
      /* That code supposes zb_buf_cut_left() do not change buffer contents but just modifies buffer length and data begin offset.  */
      zb_uint8_t * cmd_id_p = zb_buf_cut_left(param, hdr_size + 1U);
      cmd_id = cmd_id_p[-1];
    }
  }
  TRACE_MSG(TRACE_SECUR3, "in aps cmd %hd param %hd", (FMT__H_H, cmd_id, param));

#ifndef ZB_COORDINATOR_ONLY
  if (
#if defined ZB_DISTRIBUTED_SECURITY_ON
    !(IS_DISTRIBUTED_SECURITY() || ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address)
      /* TC address set to -1 at init to allow distributed formation, but did
       * not authorized yet, so it is ok to join to the centralized net */
      || (zb_tc_is_distributed() && !IS_DISTRIBUTED_SECURITY()))
    &&
#endif
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00012,20} */
    !ZB_IS_TC()
    )
  {
    zb_uint16_t tc_addr = zb_address_short_by_ieee(ZB_AIB().trust_center_address);
    if (src_short != tc_addr)
    {
      /* R21, 4.4.1.3: if we are not a TC, need to do next things:
       * - if cmd is not APS encrypted:
       *   - if IEEE is in APS command frame, it shall be compared with the TC IEEE
       *   - if no IEEE is in APS command frame, verify that NWK source is 0x0000
       * - if cmd is APS encrypted:
       *   - verify that key corresponds to keypair that has a DeviceAddress == TC address
       * - if cmd is finally from TC (prev checks are ok), check it with policies
       */
      zb_bool_t handle_pkt;
      zb_ieee_addr_t aps_ieee_addr;
      zb_ieee_addr_t device_address;

      if (keypair_i != (zb_uint16_t)-1)
      {
        zb_aps_keypair_get_addr_by_idx(keypair_i, device_address);
      }

      handle_pkt = (zb_bool_t)((secured) ?
                               (((keypair_i != (zb_uint16_t)-1) &&
                                 (ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, device_address))) ||
                                /* Special case: we do not have a TC and it is Transport Key. */
                                ZB_IEEE_ADDR_IS_ZERO(ZB_AIB().trust_center_address))
                               :
                               (((zb_aps_get_ieee_source_from_cmd_frame(cmd_id, param, aps_ieee_addr) == RET_OK) &&
                                 ZB_IEEE_ADDR_CMP(ZB_AIB().trust_center_address, aps_ieee_addr))
                                || (src_short == 0U)));

      if (!handle_pkt)
      {
        TRACE_MSG(TRACE_APS1, "Command from 0x%d dropped: not from TC, secured %hd", (FMT__D_H, src_short, secured));
        zb_buf_free(param);
        param = 0;
      }
    }
  }
#endif /*ZB_COORDINATOR_ONLY */

  if (param != 0U)
  {
    if (!zb_secur_aps_accept_policy(cmd_id,
                                    secured,
                                    keypair_i))
    {
#ifdef ZB_CERTIFICATION_HACKS
      TRACE_MSG(TRACE_APS1, "Command 0x%hx dropped: must be aps encrypted, aps_security_off %hd",
                (FMT__H_H, cmd_id, ZB_CERT_HACKS().aps_security_off));
#else
      TRACE_MSG(TRACE_APS1, "Command 0x%hx dropped: must be aps encryptedd",
                (FMT__H, cmd_id));
#endif
      zb_buf_free(param);
      param = 0;
    }
  }

  if (param != 0U && secured)
  {
    if (!zb_secur_aps_cmd_is_encrypted_by_good_key(cmd_id, src_short, keypair_i))
    {
      /* BDB 10.2.3 Trust Center Link Keys Once the node has obtained an updated
         trust-center link key it SHALL ignore any APS commands from the Trust
         Center that are not encrypted with that key. */
      TRACE_MSG(TRACE_APS1, "Command 0x%hx dropped: encrypted by bad aps key i %d",
                (FMT__H_D, cmd_id, keypair_i));
      zb_buf_free(param);
      param = 0;
    }
  }

#ifdef ZB_ROUTER_ROLE
  if (ZB_IS_DEVICE_ZC_OR_ZR()
      && param != 0U)
  {
    switch (cmd_id)
    {
#ifndef ZB_COORDINATOR_ONLY
      case APS_CMD_TUNNEL:
        TRACE_MSG(TRACE_SECUR3, "process tunnel", (FMT__0));
        zb_aps_in_tunnel_cmd(param);
        param = 0;
        break;

#endif  /* #ifndef ZB_COORDINATOR_ONLY */
#ifdef ZB_COORDINATOR_ROLE
      case APS_CMD_UPDATE_DEVICE:
        zb_aps_in_update_device(param);
        param = 0;
        break;
#endif
#ifndef ZB_COORDINATOR_ONLY
      case APS_CMD_REMOVE_DEVICE:
        zb_aps_in_remove_device(param);
        param = 0;
        break;
#endif
#ifdef ZB_COORDINATOR_ROLE
      case APS_CMD_REQUEST_KEY:
        zb_aps_in_request_key(param, keypair_i
                              , key_id
          );
        param = 0;
        break;
      case APS_CMD_VERIFY_KEY:
        zb_aps_in_verify_key(param);
        param = 0;
        break;
#endif
      default:
        /* MISRA rule 16.4 - Mandatory default label */
        break;
    }
  }
#endif  /* router */
#ifndef ZB_COORDINATOR_ONLY
  if (param != 0U)
  {
    switch (cmd_id)
    {
      case APS_CMD_TRANSPORT_KEY:
        zb_aps_in_transport_key(param, keypair_i
                                , key_id
          );
        param = 0;
        break;
      case APS_CMD_SWITCH_KEY:
        /* Not need to process switch-key at ZED for 2007: it has only 1 slot for key so
         * key switched anyway. */
        /* In PRO, according to new PICS, we need to process a switch key by end device */
        /* because it also should keep up to 3 keys */
        zb_aps_in_switch_key(param);
        param = 0;
        break;
      case APS_CMD_CONFIRM_KEY:
        zb_aps_in_confirm_key(param);
        param = 0;
        break;
      default:
        /* MISRA rule 16.4 - Mandatory default label */
        break;
    }
  }
#endif  /* #ifndef ZB_COORDINATOR_ONLY */
  if (param != 0U)
  {
    TRACE_MSG(TRACE_ERROR, "Drop unknown aps cmd %hd", (FMT__H, cmd_id));
    zb_buf_free(param);
  }
}
/*! @} */
