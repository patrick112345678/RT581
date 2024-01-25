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
/* PURPOSE: test for ZC application written using ZDO.
*/
#define ZB_TRACE_FILE_ID 95
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"
#include "zb_trace.h"
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
/* define SUBGIG to test ZC against ZED at subgig. Comment it out to test ZR. */
//#define SUBGIG

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef NCP_SDK
static zb_ieee_addr_t g_zc_addr      = TEST_ZC_ADDR;
static zb_ieee_addr_t g_ze_addr      = TEST_ZE_ADDR;
static zb_ieee_addr_t g_ncp_addr     = TEST_NCP_ADDR;
static zb_uint8_t     gs_nwk_key[16] = TEST_NWK_KEY;
static zb_uint8_t     g_ic1[16+2]    = TEST_IC;
#else
static zb_ieee_addr_t g_zc_addr      = {0xde, 0xad, 0xf0, 0x0d, 0xde, 0xad, 0xf0, 0x0d};
#endif

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_neighbors_zc");

  zb_set_long_address(g_zc_addr);

#ifndef NCP_SDK
  zb_set_pan_id(0x1aaa);
#else
  zb_set_pan_id(TEST_PAN_ID);
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_production_config_disable(ZB_TRUE);

#ifdef NCP_SDK
  zb_secur_setup_nwk_key(gs_nwk_key, 0);
  zb_set_installcode_policy(ZB_TRUE);
#endif

#ifndef SUBGIG
  zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);
#else
  {
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);

    zb_channel_page_list_set_mask(channel_list, SUBGHZ_PAGE, 1<<SUBGHZ_CHANNEL);
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
  }
#endif

  zb_set_network_coordinator_role(1L << CHANNEL);

  zb_set_max_children(1);

  /* the value 2048min is for testing GBCS 3.2 requirements */
  zb_set_keepalive_mode(ED_TIMEOUT_REQUEST_KEEPALIVE);

  ZB_SET_TRAF_DUMP_ON();

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param)
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
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

#ifdef NCP_SDK
        zb_secur_ic_add(g_ncp_addr, ZB_IC_TYPE_128, g_ic1, NULL);
        zb_secur_ic_add(g_ze_addr, ZB_IC_TYPE_128, g_ic1, NULL);
#endif

        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;

      case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
      {
        static zb_uint8_t counter = 0;
        counter++;

        if (counter == 2)
        {
          zb_zdo_signal_device_annce_params_t *dev_annce_params
            = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

          TRACE_MSG(TRACE_APP1, "dev_annce short_addr 0x%x", (FMT__D, dev_annce_params->device_short_addr));
          TRACE_MSG(TRACE_APP1, "it's ZE, adding to the invisible list because it must rejoin to NCP ZR", (FMT__0));

          MAC_ADD_INVISIBLE_SHORT(dev_annce_params->device_short_addr);
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
    /*ZB_SYSTEST_EXIT_ERR("Device start FAILED status %d",
      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));*/
  }

  zb_buf_free(param);
}


/*! @} */
