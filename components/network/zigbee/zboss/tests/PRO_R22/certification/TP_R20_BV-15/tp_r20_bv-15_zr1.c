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
/* PURPOSE: TP/R20/BV-15: Messagess with Wildcard Profile
To confirm that a device will answer a Match Descriptor request and other APS data messages using the wildcard profile ID (0xFFFF).
Router side
*/

#define ZB_TEST_NAME TP_R20_BV_15_ZR1
#define ZB_TRACE_FILE_ID 40657

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../common/zb_cert_test_globals.h"
#include "test_common.h"


/* For NS build first ieee addr byte should be unique */
static zb_ieee_addr_t g_ieee_addr = {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

ZB_DECLARE_SIMPLE_DESC(2, 3);

static zb_af_simple_desc_2_3_t test_simple_desc;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_zr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_default_ffd_descriptor_values(ZB_ROUTER);

/* set power descriptor to appropriate values (sadly, these values differs from
 * test to test */
//zb_set_node_power_descriptor(ZB_POWER_MODE_SYNC_ON_WHEN_IDLE,
 //                            ZB_POWER_SRC_CONSTANT | ZB_POWER_SRC_RECHARGEABLE_BATTERY | ZB_POWER_SRC_DISPOSABLE_BATTERY,
 //                              ZB_POWER_SRC_CONSTANT, ZB_POWER_LEVEL_100);
/*
  simple descriptor for test
  SimpleDescriptor=
  ProfileID = 0x7f01 (Test Profile)
  NumInClusters = 0x02
  InClusterList = 0x0054, 0x00e0
  NumOutClusters = 0x03
  OutClusterList = 0x001c, 0x0038, 0x00a8
*/


  zb_set_simple_descriptor((zb_af_simple_desc_1_1_t*)&test_simple_desc,
                           1 /* endpoint */,                0x7f01 /* app_profile_id */,
                           0x0 /* app_device_id */,         0x0   /* app_device_version*/,
                           0x2 /* app_input_cluster_count */, 0x3 /* app_output_cluster_count */);


  zb_set_input_cluster_id((zb_af_simple_desc_1_1_t*)&test_simple_desc, 0,  0x54);
  zb_set_input_cluster_id((zb_af_simple_desc_1_1_t*)&test_simple_desc, 1,  0xe0);

  zb_set_output_cluster_id((zb_af_simple_desc_1_1_t*)&test_simple_desc, 0,  0x1c);
  zb_set_output_cluster_id((zb_af_simple_desc_1_1_t*)&test_simple_desc, 1,  0x38);
  zb_set_output_cluster_id((zb_af_simple_desc_1_1_t*)&test_simple_desc, 2,  0xa8);

  zb_add_simple_descriptor((zb_af_simple_desc_1_1_t*)&test_simple_desc);

  /* set ieee addr */
  zb_set_long_address(g_ieee_addr);

  /* turn off security */
  /* zb_cert_test_set_security_level(0); */

  /* become an ED */
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zr_role();

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

  zb_buf_free(param);
}
