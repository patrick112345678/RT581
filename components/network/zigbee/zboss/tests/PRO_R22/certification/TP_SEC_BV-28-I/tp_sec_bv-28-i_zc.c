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
/* PURPOSE: 11.28 TP/SEC/BV-28-I Security Remove Device (ZR)
Objective: DUT as ZR correctly handles APS Remove device and ZDO Mgmt_Leave_req.
*/

#define ZB_TEST_NAME TP_SEC_BV_28_I_ZC
#define ZB_TRACE_FILE_ID 40555
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

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif

enum sec_bv_28_step_e
{
  SEC_BV_28_LEAVE_NONEX,
  SEC_BV_28_LEAVE_ZED2,
  SEC_BV_28_LEAVE_ZR
};

static const zb_ieee_addr_t g_ieee_addr_c = IEEE_ADDR_C;
static const zb_ieee_addr_t g_ieee_addr_r = IEEE_ADDR_R;
static const zb_ieee_addr_t g_ieee_addr_ed1 = IEEE_ADDR_ED1;
static const zb_ieee_addr_t g_ieee_addr_ed2 = IEEE_ADDR_ED2;
static const zb_ieee_addr_t g_ieee_addr_nonex = IEEE_ADDR_NONEX;

static zb_uint8_t test_counter = 0;

static zb_uint16_t TEST_PAN_ID = 0x1AAA;

static void send_remove_device(zb_uint8_t param);
static void send_remove_device_cmd(zb_uint8_t unused);
static void send_mgmt_leave(zb_uint8_t param,
                            zb_ieee_addr_t device,
                            zb_bool_t flag_rejoin,
                            zb_bool_t flag_remove_children);
static void send_mgmt_leave_cmd(zb_uint8_t unused);
static void send_mgmt_lqi_req(zb_uint8_t unused);

MAIN()
{
  ARGV_UNUSED;


  ZB_INIT("zdo_1_zc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_c);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  MAC_ADD_VISIBLE_LONG((zb_uint8_t*) g_ieee_addr_r);

  zb_set_pan_id(TEST_PAN_ID);

  zb_set_max_children(3);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_key0, 0);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_zdo_set_aps_unsecure_join(INSECURE_JOIN_ZC);

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

  switch (sig)
  {
    case ZB_ZDO_SIGNAL_DEFAULT_START:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));

#ifdef TEST_ENABLED
        /* send Remove Device to ZR with target addr of ZED1 */
        ZB_SCHEDULE_ALARM(send_remove_device_cmd, 0, TIME_ZC_REMOVE_DEV_ZED1);

        /* send a ZDO Mgmt_Leave_req for a non-existent device */
	ZB_SCHEDULE_ALARM(send_mgmt_leave_cmd, 0, TIME_ZC_LEAVE_REQ_NONEX);

        /* send a ZDO Mgmt_Leave_req for device ZED2 */
	ZB_SCHEDULE_ALARM(send_mgmt_leave_cmd, 0, TIME_ZC_LEAVE_REQ_ZED2);

        /* send a mgnt_lqi_req */
	ZB_SCHEDULE_ALARM(send_mgmt_lqi_req, 0, TIME_ZC_LQI_REQ);

	/* send a ZDO Mgmt_Leave_req for device ZR */
	ZB_SCHEDULE_ALARM(send_mgmt_leave_cmd, 0, TIME_ZC_LEAVE_REQ_ZR);
#endif

      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break; /* ZB_ZDO_SIGNAL_DEFAULT_START */

    default:
      if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
      {
	TRACE_MSG(TRACE_APS1, "zboss_signal_handler: status OK, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "zboss_signal_handler: status FAILED, status %d",
		  (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
      }
      break;
  }

  zb_buf_free(param);
}


static void send_remove_device(zb_uint8_t param)
{
  zb_apsme_remove_device_req_t *req = ZB_BUF_GET_PARAM(param, zb_apsme_remove_device_req_t);

  TRACE_MSG(TRACE_SECUR1, "force zed remove", (FMT__0));

  ZB_IEEE_ADDR_COPY(req->parent_address, g_ieee_addr_r);
  ZB_IEEE_ADDR_COPY(req->child_address, g_ieee_addr_ed1);

  ZB_SCHEDULE_CALLBACK(zb_secur_apsme_remove_device, param);

}

static void send_remove_device_cmd(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  zb_buf_get_out_delayed(send_remove_device);
}

static void send_mgmt_leave(zb_bufid_t 	buf,
                            	 zb_ieee_addr_t device,
                            	 zb_bool_t 	flag_rejoin,
                            	 zb_bool_t 	flag_remove_children)
{
  zb_zdo_mgmt_leave_param_t *req = NULL;

  TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_leave", (FMT__0));


  zb_buf_initial_alloc(buf, 0);

  req = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_leave_param_t);

  req->remove_children = flag_remove_children;
  req->rejoin = flag_rejoin;

  ZB_MEMCPY(req->device_address, device, sizeof(zb_ieee_addr_t));

  req->dst_addr = zb_address_short_by_ieee((zb_uint8_t*)g_ieee_addr_r);

  zdo_mgmt_leave_req(buf, NULL);

  TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_leave", (FMT__0));
}

static void send_mgmt_leave_cmd(zb_uint8_t unused)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_ieee_addr_t device_addr;

  TRACE_MSG(TRACE_ZDO1, ">>send_mgmt_leave_cmd", (FMT__0));

  ZVUNUSED(unused);

  if (buf)
  {
    switch(test_counter)
    {
      case SEC_BV_28_LEAVE_NONEX:
	ZB_IEEE_ADDR_COPY(device_addr, &g_ieee_addr_nonex);
	break;
      case SEC_BV_28_LEAVE_ZED2:
	ZB_IEEE_ADDR_COPY(device_addr, &g_ieee_addr_ed2);
	break;
      case SEC_BV_28_LEAVE_ZR:
	ZB_IEEE_ADDR_COPY(device_addr, &g_ieee_addr_r);
	break;
      default:
	break;
    }

    send_mgmt_leave(buf,
                    device_addr,
                    ZB_FALSE /* rejoin */,
                    ZB_FALSE /* remove children */);

    test_counter++;
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ZDO1, "<<send_mgmt_leave_cmd", (FMT__0));
}

static void send_mgmt_lqi_req(zb_uint8_t unused)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_ZDO1, ">>mgmt_lqi_req", (FMT__0));

  if (buf)
  {
    zb_zdo_mgmt_lqi_param_t *req_param = ZB_BUF_GET_PARAM(buf, zb_zdo_mgmt_lqi_param_t);

    ZB_BZERO(req_param, sizeof(zb_zdo_mgmt_lqi_param_t));
    req_param->dst_addr = zb_address_short_by_ieee((zb_uint8_t*) g_ieee_addr_r);

    zb_zdo_mgmt_lqi_req(buf, NULL);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ZDO1, "<<mgmt_lqi_req", (FMT__0));
}
