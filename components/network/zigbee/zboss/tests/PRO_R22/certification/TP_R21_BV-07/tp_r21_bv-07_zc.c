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

#define ZB_TEST_NAME TP_R21_BV_07_ZC
#define ZB_TRACE_FILE_ID 40531

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


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_zc = IEEE_ADDR_ZC;

static const zb_uint8_t g_key2[16] = { 0x45, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66};

static void dev_annce_cb(zb_zdo_device_annce_t *da);
static void test_close_permit_join(zb_uint8_t param);
static void change_nwk_key(zb_uint8_t param);


static zb_bool_t s_device_started = ZB_FALSE;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();
  zb_set_long_address(g_ieee_addr_zc);
  zb_set_pan_id(0x1aaa);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key_nwk1, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_key2, 1);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

  /* disable rejoin for child */
  zb_cert_test_set_aib_allow_tc_rejoins(ZB_FALSE);

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
	if (s_device_started == ZB_FALSE)
	{
	  TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
	  s_device_started = ZB_TRUE;
	  zb_zdo_register_device_annce_cb(dev_annce_cb);
	  ZB_SCHEDULE_ALARM(change_nwk_key, 0, ZB_TIME_ONE_SECOND * 21);
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


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  ZVUNUSED(da);
  TRACE_MSG(TRACE_APS1, "disabling association on ZC",(FMT__0));
  zb_buf_get_out_delayed(test_close_permit_join);
}


static void test_close_permit_join(zb_uint8_t param)
{
  zb_nlme_permit_joining_request_t *req;

  TRACE_MSG(TRACE_APS1, ">>test_close_permit_join", (FMT__0));

  TRACE_MSG(TRACE_APS1, "Closing permit join on ZC, buf = %d",
            (FMT__D, param));

  req = zb_buf_get_tail(param, sizeof(zb_nlme_permit_joining_request_t));
  req->permit_duration = 0;
  zb_nlme_permit_joining_request(param);

  TRACE_MSG(TRACE_APS1, "<<test_close_permit_join", (FMT__0));
}


static void change_nwk_key(zb_uint8_t param)
{
  ZVUNUSED(param);

#ifndef NCP_MODE_HOST
  TRACE_MSG(TRACE_APS1, "zb_change_nwk_key - Key changed", (FMT__0));
  /* cause ZR to rejoin by switching nwk key*/
  ZG->zdo.handle.key_sw = 1;
  secur_nwk_key_switch(1);
#else
  ZB_ASSERT(ZB_FALSE && "TOOD: use NCP API here");
#endif
}

/*! @} */

