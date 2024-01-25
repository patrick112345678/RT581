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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 41675

#include "zb_trace_signal_params.h"

static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params);
static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params);

void zb_trace_signal_params(zb_zdo_app_signal_type_t sig_type, zb_zdo_app_signal_hdr_t *sg_p)
{
  TRACE_MSG(TRACE_APP3, ">> zb_trace_signal_params, sig_type 0x%hx", (FMT__H, sig_type));

  if (sg_p)
  {
    switch (sig_type)
    {
      case ZB_ZDO_SIGNAL_DEVICE_UPDATE:
      {
        zb_zdo_signal_device_update_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_update_params_t);
        zb_trace_device_update_signal(params);
      }
      break;

      case ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED:
      {
        zb_zdo_signal_device_authorized_params_t *params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_authorized_params_t);

        zb_trace_device_authorized_signal(params);
      }
      break;

      default:
        TRACE_MSG(TRACE_APP3, "signal type is invalid or tracing isn't implemented!", (FMT__0));
        break;
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "invalid pointer to the zb_zdo_app_signal_hdr_t!", (FMT__0));
  }

  TRACE_MSG(TRACE_APP3, "<< zb_trace_signal_params", (FMT__0));
}

static void zb_trace_device_update_signal(zb_zdo_signal_device_update_params_t *params)
{
  TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_UPDATE", (FMT__0));
  TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));
  TRACE_MSG(TRACE_APP3, "short_addr: 0x%x", (FMT__D, params->short_addr));

  switch (params->status)
  {
    case ZB_STD_SEQ_SECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_STD_SEQ_UNSECURED_JOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
      break;
    case ZB_DEVICE_LEFT:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - DEVICE_LEFT", (FMT__H, params->status));
      break;
    case ZB_STD_SEQ_UNSECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_SECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - SECURED_REJOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_UNSECURED_JOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_JOIN", (FMT__H, params->status));
      break;
    case ZB_HIGH_SEQ_UNSECURED_REJOIN:
      TRACE_MSG(TRACE_APP3, "status: 0x%hx - UNSECURED_REJOIN", (FMT__H, params->status));
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "status: 0x%hx - INVALID STATUS", (FMT__H, params->status));
      break;
  }
}

static void zb_trace_device_authorized_signal(zb_zdo_signal_device_authorized_params_t *params)
{
  TRACE_MSG(TRACE_APP3, "ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED", (FMT__0));
  TRACE_MSG(TRACE_APP3, "long_addr: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(params->long_addr)));
  TRACE_MSG(TRACE_APP3, "short_addr: 0x%x", (FMT__D, params->short_addr));

  switch (params->authorization_type)
  {
    case ZB_ZDO_AUTHORIZATION_TYPE_LEGACY:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - LEGACY DEVICE", (FMT__H, params->authorization_type));
      switch (params->authorization_status)
      {
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_SUCCESS:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_LEGACY_DEVICE_AUTHORIZATION_FAILED:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
          break;
        default:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
          break;
      }
      break;

    case ZB_ZDO_AUTHORIZATION_TYPE_R21_TCLK:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - R21 TCLK", (FMT__H, params->authorization_type));
      switch (params->authorization_status)
      {
        case ZB_ZDO_TCLK_AUTHORIZATION_SUCCESS:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - SUCCESS", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_TCLK_AUTHORIZATION_TIMEOUT:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - TIMEOUT", (FMT__H, params->authorization_status));
          break;
        case ZB_ZDO_TCLK_AUTHORIZATION_FAILED:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - FAILED", (FMT__H, params->authorization_status));
          break;
        default:
          TRACE_MSG(TRACE_APP3, "auth_status: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_status));
          break;
      }
      break;

    default:
      TRACE_MSG(TRACE_APP3, "auth_type: 0x%hx - INVALID VALUE", (FMT__H, params->authorization_type));
      break;
  }
}
