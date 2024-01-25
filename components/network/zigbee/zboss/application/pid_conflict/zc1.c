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

/* PURPOSE: Sample application showing PAN ID conflict resolution by ZC.
 */

#define ZB_TRACE_FILE_ID 40106

#include "zb_common.h"
#include "pid_conflict_common.h"


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#define ZB_CERTIFICATION_HACKS
#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS macro
#endif

/* Max number of children */
#define MAX_CHILD_NR 2

/* IEEE address of the device */
static const zb_ieee_addr_t g_zc1_addr = IEEE_ADDR_ZC1;

/* Key for ZC1 */
static zb_uint8_t g_nwk_key_zc1[16] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
  ARGV_UNUSED;

  /* Trace enable */
  ZB_SET_TRACE_ON();
  /* Traffic dump enable */
  ZB_SET_TRAF_DUMP_ON();

  /* Global ZBOSS initialization */
  ZB_INIT("zdo_zc1");

  /* Set up defaults for the commissioning */
  zb_set_network_coordinator_role(1l<<21);
  zb_set_max_children(MAX_CHILD_NR);
  zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

  /* assign our address */
  zb_set_long_address(g_zc1_addr);

  zb_set_pan_id(0x1aaa);

  /* Comment following line in order to trigger NWK Update
   * without need to receive NWK Report from ZR1 or ZR2.
   * Ignore ZC2:
   */
   MAC_ADD_INVISIBLE_SHORT(0x0000);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key_zc1, 0);

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


static void buffer_test_cb(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS1, "buffer_test_cb %hd", (FMT__H, param));
  if (param == ZB_TP_BUFFER_TEST_OK)
  {
    TRACE_MSG(TRACE_APS1, "status OK", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APS1, "status ERROR", (FMT__0));
  }
}


static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr)
{
  zb_buffer_test_req_param_t *req_param;
  TRACE_MSG(TRACE_ERROR, "send_test_request to %d", (FMT__D, addr));
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len = 0x10;
  req_param->dst_addr = addr;
  req_param->src_ep = 1;

  zb_tp_buffer_test_request(buf, buffer_test_cb);
}


static void send_buffertest1(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();

  ZVUNUSED(param);

  /* ZC1 sends a broadcast Buffer Test Request (secured, KEY0) to all
   * members of the PAN */

  if (buf)
  {
    TRACE_MSG(TRACE_ERROR, "buffer test 1", (FMT__0));
    zc_send_data(buf, ZB_NWK_BROADCAST_ALL_DEVICES);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  /* Schedule next test, where ZR1 won't bellog to PAN
   * since it does not support PAN ID conflict resolution */
  ZB_SCHEDULE_APP_ALARM(send_buffertest1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(40000));

}


/* Callback to handle the stack events */
void zboss_signal_handler(zb_uint8_t param)
{
  zb_pan_id_conflict_info_t *info = NULL;
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
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        zb_enable_panid_conflict_resolution(ZB_TRUE);
        ZB_SCHEDULE_APP_ALARM(send_buffertest1, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(40000));
        break;
      case ZB_NWK_SIGNAL_PANID_CONFLICT_DETECTED:
        TRACE_MSG(TRACE_APP1, "PID conflict detected", (FMT__0));
        info = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_pan_id_conflict_info_t);
        ZB_ZDO_SIGNAL_CUT_HEADER(param);
        zb_start_pan_id_conflict_resolution(param);
        param = 0;
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
