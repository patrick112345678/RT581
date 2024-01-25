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

#define ZB_TEST_NAME TP_R21_BV_07_ZR
#define ZB_TRACE_FILE_ID 40532

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"

#include "test_common.h"
#include "../common/zb_cert_test_globals.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static const zb_ieee_addr_t g_ieee_addr_zr = IEEE_ADDR_ZR;

static void zb_mgmt_lqi_request_delayed(zb_uint8_t param);
static void zb_mgmt_lqi_request(zb_uint8_t param);
static void call_rejoin_initiation(zb_uint8_t param);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_2_zr");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_set_long_address(g_ieee_addr_zr);

  ZB_CERT_HACKS().disable_cyclic_tc_rejoin = 1;

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        /* Cause router send commands to ZC. After ZC changes it's nwk key
         * ZR will fail to proceed secured frame and will rejoin */
        ZB_SCHEDULE_ALARM(zb_mgmt_lqi_request_delayed, 0,
                          ZB_TIME_ONE_SECOND * 10);
        ZB_SCHEDULE_ALARM(zb_mgmt_lqi_request_delayed, 0,
                          ZB_TIME_ONE_SECOND * 20);
        ZB_SCHEDULE_ALARM(zb_mgmt_lqi_request_delayed, 0,
                          ZB_TIME_ONE_SECOND * 30);
        break;
      case ZB_NLME_STATUS_INDICATION:
      {
        zb_zdo_signal_nlme_status_indication_params_t *params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_nlme_status_indication_params_t);

        TRACE_MSG(TRACE_ZDO1, "NLME status indication: status 0x%hx address 0x%x",
                  (FMT__H_D, params->nlme_status.status, params->nlme_status.network_addr));

        /* Copy-pasted from zdo_app.c. Not the best way to initiate TC rejoin, but let's keep it for
         * now to not break the test. */
        if (ZB_NWK_COMMAND_STATUS_IS_SECURE(params->nlme_status.status) &&
            zb_nwk_get_nbr_rel_by_short(params->nlme_status.network_addr) == ZB_NWK_RELATIONSHIP_PARENT)
        {
#ifndef NCP_MODE_HOST
          ZDO_CTX().parent_link_failure = 0;
          TRACE_MSG(TRACE_ZDO1, "Security error - cleat authenticated flag and rejoin", (FMT__0));
          ZG->aps.authenticated = ZB_FALSE;
          zb_cert_test_set_nwk_state(ZB_NLME_STATE_IDLE);
          zb_buf_get_out_delayed(call_rejoin_initiation);
#else
          ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
        }
      }
      break;

      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
                        (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  zb_buf_free(param);
}

static void call_rejoin_initiation(zb_uint8_t param)
{
  zb_uint8_t *rejoin_reason = ZB_BUF_GET_PARAM(param, zb_uint8_t);
  *rejoin_reason = ZB_REJOIN_REASON_SELF_INITIATED;

  ZB_SCHEDULE_CALLBACK(zdo_commissioning_initiate_rejoin, param);
}

static void zb_mgmt_lqi_request_delayed(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed(zb_mgmt_lqi_request);
}


static void zb_mgmt_lqi_request(zb_uint8_t param)
{
  zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(param, zb_zdo_mgmt_lqi_param_t);

  req_param->dst_addr = 0x0000;
  req_param->start_index = 0;

  zb_zdo_mgmt_lqi_req(param, NULL);
}


/*! @} */

