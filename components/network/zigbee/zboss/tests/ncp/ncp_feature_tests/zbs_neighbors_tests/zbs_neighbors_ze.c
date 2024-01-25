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
/* PURPOSE: test for ZED application written using ZDO.
*/

#define ZB_TRACE_FILE_ID 4
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_min.h"
#endif

//#define SUBGIG

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif
/*! \addtogroup ZB_TESTS */
/*! @{ */

zb_ieee_addr_t g_ze_addr = TEST_ZE_ADDR;

#ifdef NCP_SDK
static zb_uint8_t g_ic1[16+2] = TEST_IC;
#endif

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

#define FINISH_TEST_TIMEOUT_SEC 5


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_neighbors_ze");

#ifdef ZB_SECURITY
  ZB_SET_NIB_SECURITY_LEVEL(5);
#endif

  zb_set_long_address(g_ze_addr);
  zb_set_rx_on_when_idle(ZB_FALSE);

#ifndef SUBGIG
  /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
  zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);
#else

  {
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_page_list_set_mask(channel_list, SUBGHZ_PAGE, 1<<SUBGHZ_CHANNEL);
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
    //zb_se_set_network_ed_role_select_device(channel_list);
  }
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_set_network_ed_role(1L << CHANNEL);

  zb_set_ed_timeout(ED_AGING_TIMEOUT_10SEC);

  ZB_SET_TRAF_DUMP_ON();

#ifdef NCP_SDK
    ZB_TCPOL().require_installcodes = 1;
    zb_secur_ic_set(ZB_IC_TYPE_128, g_ic1);
#endif
  if (zdo_dev_start() != RET_OK)
  {
    //ZB_SYSTEST_EXIT_ERR("zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

static void change_channel_cb(zb_uint8_t param)
{
  zb_mlme_set_confirm_t *conf = (zb_mlme_set_confirm_t*)zb_buf_begin(param);

  TRACE_MSG(TRACE_APP3, ">>change_channel_cb(), param %hd status %hd",
            (FMT__H_H, param, conf->status));

  if (conf->status == MAC_SUCCESS)
  {
    TRACE_MSG(TRACE_APP3, "channel was changed!", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "couldn't change channel!", (FMT__0));
    ZB_ASSERT(0);
  }
}

static void change_channel(zb_uint8_t param)
{
  zb_uint8_t channel = NEW_CHANNEL;

  TRACE_MSG(TRACE_APP3, ">>change_channel(), param %hd, channel %hd",
            (FMT__H_H, param, channel));

  zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL, &channel, 1, change_channel_cb);
}

static void change_channel_delayed(zb_uint8_t param)
{
  (void)param;
  zb_buf_get_out_delayed(change_channel);
}

void zboss_signal_handler(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      //! [signal_skip_startup]
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        break;

      //! [signal_sleep]
      case ZB_COMMON_SIGNAL_CAN_SLEEP:
      {
        zb_zdo_signal_can_sleep_params_t *can_sleep_params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_can_sleep_params_t);

        /* TODO: check if app is ready for sleep and sleep_tmo is ok, disable peripherals if needed */
        TRACE_MSG(TRACE_ERROR, "Can sleep for %ld ms", (FMT__L, can_sleep_params->sleep_tmo));
#ifdef ZB_USE_SLEEP
        zb_sleep_now();
#endif
        /* TODO: enable peripherals if needed */
      }
      break;

      case ZB_ZDO_SIGNAL_LEAVE:
      {
        zb_uint16_t zc_addr = 0x0000;
        zb_zdo_signal_leave_params_t *leave_params =
          ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_leave_params_t);

        TRACE_MSG(TRACE_APP1, "leave_params->leave_type {}", (FMT__H, leave_params->leave_type));

        MAC_ADD_INVISIBLE_SHORT(zc_addr);

        ZB_SCHEDULE_ALARM(change_channel_delayed, 0 /* unused */, FINISH_TEST_TIMEOUT_SEC * ZB_TIME_ONE_SECOND);
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
    TRACE_MSG(TRACE_APP1, "Device START failed", (FMT__0));
    //ZB_SYSTEST_EXIT_ERR("Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  zb_buf_free(param);
}


/*! @} */
