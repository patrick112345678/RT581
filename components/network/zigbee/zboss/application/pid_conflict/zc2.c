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

/* PURPOSE: ZC used to trigger PAN ID conflict. Also, it is a ZC
 * that does NOT handle the PAN ID conflict.
 */

#define ZB_TRACE_FILE_ID 40107

#include "zb_common.h"
#include "pid_conflict_common.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/* Max number of children */
#define MAX_CHILD_NR 1

static const zb_ieee_addr_t g_zc2_addr = IEEE_ADDR_ZC2;

/* Key for ZC2 */
static zb_uint8_t g_nwk_key_zc2[16] = {0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
                                       0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd};

MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable */
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("zdo_zc2");

  /* Set up defaults for the commissioning */
  zb_set_long_address(g_zc2_addr);
  zb_set_pan_id(0x1bbb);
  zb_set_network_coordinator_role(1l<<21);
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
  zb_set_max_children(MAX_CHILD_NR);

  MAC_ADD_INVISIBLE_SHORT(0x0000);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key_zc2, 0);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* Initiate the stack start with starting the commissioning */
  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    /* Call the main loop */
    zboss_main_loop();
  }

  /* Deinitialize trace */
  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static void shutdown_plz(zb_uint8_t param)
{
  ZVUNUSED(param);
  return;
}


static void change_panid(zb_uint8_t param)
{
  zb_mlme_start_req_t * req;

  param = zb_buf_get_out();
  //! [zb_mlme_start_request]
  req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  TRACE_MSG(TRACE_NWK1, "Change PANID", (FMT__0));

  zb_set_pan_id(0x1aaa);
  ZB_NIB_SEQUENCE_NUMBER() = zb_random()%256;
  /* zb_cert_test_set_nib_seq_number(zb_random()%256); */
  ZB_MEMSET(req, 0, sizeof(*req));
  req->pan_id = 0x1aaa;
  req->logical_channel = zb_get_current_channel();
  req->channel_page = 0;
  req->pan_coordinator = (ZB_IS_DEVICE_ZC());
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;
  ZG->nwk.handle.state = ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION;
  /* zb_cert_test_set_nwk_state(ZB_NLME_STATE_PANID_CONFLICT_RESOLUTION); */
  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
  //! [zb_mlme_start_request]
  ZB_SCHEDULE_ALARM(shutdown_plz, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(80000));
}


/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        /* It is disabled by default. */
        zb_enable_panid_conflict_resolution(ZB_FALSE);

        change_panid(0);
        break;
      default:
        TRACE_MSG(TRACE_APP1, "Unknown signal 0x%hx", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  /* Free the buffer if it is not used */
  if (param)
  {
    zb_buf_free(param);
  }
}
