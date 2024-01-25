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
/* PURPOSE: 11.23 TP/APS/ BV-23-I Binding with Groups (HC V1 RX DUT)
*/

#define ZB_TEST_NAME TP_APS_BV_23_I_ZR1
#define ZB_TRACE_FILE_ID 40686

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "tp_aps_bv-23-i_common.h"
#include "../common/zb_cert_test_globals.h"


static const zb_ieee_addr_t g_ieee_addr_r1 = IEEE_ADDR_R1;
static zb_af_simple_desc_1_1_t s_dut_simple_desc_list[ZB_MAX_EP_NUMBER];


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("zdo_zr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif

  zb_set_long_address(g_ieee_addr_r1);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();
  zb_set_max_children(0);
  zb_set_nvram_erase_at_start(ZB_TRUE);

  /* zb_cert_test_set_security_level(0); */

  ZB_NIB_SET_USE_MULTICAST(USE_NWK_MULTICAST);

  zb_set_simple_descriptor(&s_dut_simple_desc_list[0],  1 /* endpoint */, 0x0104 /* HA */,
                           0xABCD, (zb_bitfield_t) 0x7,
                           1 /* input clusters */,
                           1 /* output clusters */);
  zb_set_input_cluster_id(&s_dut_simple_desc_list[0], 0, 0x001c);
  zb_set_output_cluster_id(&s_dut_simple_desc_list[0], 0, 0x001c);
  zb_add_simple_descriptor(&s_dut_simple_desc_list[0]);

#if ZB_MAX_EP_NUMBER > 1
  zb_set_simple_descriptor(&s_dut_simple_desc_list[1], 1 /* endpoint */, 0x0104 /* HA */,
                           0xABCD, (zb_bitfield_t) 0x7,
                           1 /* input clusters */,
                           1 /* output clusters */);
  zb_set_input_cluster_id(&s_dut_simple_desc_list[1], 0, 0x001c);
  zb_set_output_cluster_id(&s_dut_simple_desc_list[1], 0, 0x001c);
  zb_add_simple_descriptor(&s_dut_simple_desc_list[1]);
#endif

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

static void test_buf_req(zb_uint8_t unused)
{
  zb_bufid_t buf = zb_buf_get_out();
  zb_buffer_test_req_param_t *req_param = NULL;

  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APP1, ">>test_buf_req", (FMT__0));
  if (!buf)
  {
    TRACE_MSG(TRACE_ERROR, "test_buf_req: error - unable to get data buffer", (FMT__0));
    ZB_EXIT(1);
  }

  /* sending by group binding, so dst addr does not required */
  req_param = ZB_BUF_GET_PARAM(buf, zb_buffer_test_req_param_t);
  BUFFER_TEST_REQ_SET_DEFAULT(req_param);
  req_param->len        = TEST_BUFFER_LEN;
  req_param->addr_mode  = ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  req_param->src_ep     = ZR1_EP_SRC;
  req_param->dst_ep     = ZR1_EP_DST;

  zb_tp_buffer_test_request(buf, NULL);

  PRINT_BIND_TABLE();

  TRACE_MSG(TRACE_APP1, "<<test_buf_req", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_ERROR, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  if (0 == status)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));

        test_step_register(test_buf_req, GROUP_3, TP_APS_BV_23_I_STEP_5_TIME_DUTZR);
        test_step_register(test_buf_req, GROUP_4, TP_APS_BV_23_I_STEP_8_TIME_DUTZR);
        test_step_register(test_buf_req, GROUP_3, TP_APS_BV_23_I_STEP_10_TIME_DUTZR);

        /* The line below is not needed anymore, only three sending
         * attempts are expected in this test case
         */
        /* test_step_register(test_buf_req, GROUP_4, TP_APS_BV_23_I_STEP_11_TIME_DUTZR); */
        test_control_start(TEST_MODE, TP_APS_BV_23_I_STEP_5_DELAY_DUTZR);
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "Unknown signal %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

