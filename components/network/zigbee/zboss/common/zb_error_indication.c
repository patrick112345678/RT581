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
/* PURPOSE: Error indication implementation
*/


#define ZB_TRACE_FILE_ID 116
#include "zboss_api_core.h"
#include "zb_error_indication.h"

#ifdef ZB_USE_ERROR_INDICATION

#ifdef ZB_TOOL
zb_error_indication_ctx_t s_err_ind;
#define ERR_IND s_err_ind
#else

#include "zb_common.h"

#define ERR_IND ZG->err_ind
#endif

void zb_error_register_app_handler(zb_error_handler_t cb)
{
  TRACE_MSG(TRACE_COMMON1, ">> zb_error_register_app_handler, cb %p",
            (FMT__P, cb));

  ZB_ASSERT(cb != NULL);

  if (cb != NULL)
  {
    ERR_IND.app_err_handler = cb;
  }

  TRACE_MSG(TRACE_COMMON1, "<< zb_error_register_app_handler", (FMT__0));
}


static void error_unhandled(zb_uint8_t severity, zb_ret_t err_code, void *additional_info)
{
  TRACE_MSG(TRACE_ERROR, "Error is not handled: severity %d, err_code %d",
            (FMT__D_D, severity, err_code));

  ZVUNUSED(additional_info);

  if (severity == (zb_uint8_t)ZB_ERROR_SEVERITY_MINOR)
  {
    TRACE_MSG(TRACE_ERROR, "not fatal, can proceed", (FMT__0));
  }
  else
  {
    /* TODO: we write this dataset to store diagnostics counters. But we need to check that the error
     is not an NVRAM one before trying to write a dataset. */
#if 0
#if defined ZB_USE_NVRAM && defined ZDO_DIAGNOSTICS
    (void)zb_nvram_write_dataset(ZB_NVRAM_HA_DATA);
    (void)zb_nvram_write_dataset(ZB_NVRAM_ZDO_DIAGNOSTICS_DATA);
#endif /* ZB_USE_NVRAM && ZDO_DIAGNOSTICS */
#endif

    /* TODO: maybe, we need to call ZB_ABORT or zb_reset, not ZB_ASSERT? */
#ifdef USE_ASSERT
    ZB_ASSERT(0);
#else
    zb_reset(0);
#endif
  }
}

void zb_error_raise(zb_uint8_t severity, zb_ret_t err_code, void *additional_info)
{
  zb_bool_t handled = ZB_FALSE;

  TRACE_MSG(TRACE_ERROR, ">> zb_error_raise severity %d, err_code %d",
            (FMT__D_D, severity, err_code));

  /* We can try different handlers (e.g. module level, stack/top-level level) first,
     now only the NCP Host defines a stack handler */
#ifdef HAVE_TOP_LEVEL_ERROR_HANDLER
  if (!handled)
  {
    TRACE_MSG(TRACE_ERROR, "call a stack handler", (FMT__0));
    handled = zb_error_top_level_handler(severity, err_code, additional_info);
  }
#endif /* HAVE_TOP_LEVEL_ERROR_HANDLER */

  if (!handled && ERR_IND.app_err_handler != NULL)
  {
    TRACE_MSG(TRACE_ERROR, "call an app error handler", (FMT__0));

    handled = ERR_IND.app_err_handler(severity, err_code, additional_info);
  }

  if (!handled)
  {
    error_unhandled(severity, err_code, additional_info);
  }

  TRACE_MSG(TRACE_ERROR, "<< zb_error_raise", (FMT__0));
}

#endif /* ZB_USE_ERROR_INDICATION */
