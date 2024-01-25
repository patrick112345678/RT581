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
/* PURPOSE: ZC sends address conflict status.
*/
#define ZB_TRACE_FILE_ID 41682
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../nwk/nwk_internal.h"
#include "../zbs_feature_tests.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_zc_addr = TEST_ZC_ADDR;
static zb_ieee_addr_t g_ncp_addr = TEST_NCP_ADDR;
static zb_uint8_t gs_nwk_key[16] = TEST_NWK_KEY;
static zb_uint8_t g_ic1[16+2] = TEST_IC;

static zb_uint16_t zr_nwk_addr;

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

static void send_address_conflict(zb_uint8_t param);

static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t ncp_addr = TEST_NCP_ADDR;

  TRACE_MSG(TRACE_APS1, "device annce, cap 0x%x, addr 0x%x", (FMT__D_D, da->capability, da->nwk_addr));

  if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, ncp_addr))
  {
    TRACE_MSG(TRACE_ERROR, "Unknown device has joined!", (FMT__0));
  }
  else if (da->capability & 2)  /* FFD */
  {
    TRACE_MSG(TRACE_APS1, "ZR - send address conflict after 10s", (FMT__0));
    zr_nwk_addr = da->nwk_addr;
    ZB_SCHEDULE_ALARM(send_address_conflict, 0, 10*ZB_TIME_ONE_SECOND);
  }
  else
  {
    static zb_ieee_addr_t fake_addr = {0x11, 0x22, 0xef, 0xcd, 0xab, 0x50, 0x50, 0x50};
    static int once = 0;

    if (once == 0)
    {
      TRACE_MSG(TRACE_APS1, "ZED - create entry in addr with the same addr", (FMT__0));
      /* ZED does not detect address conflict. Instead ZC detects it and sends unsolicited Rejoin Resp */
      zb_nwk_is_conflict_addr(da->nwk_addr, fake_addr);
      once++;
    }
  }
}

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#ifdef ZB_NS_BUILD
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif
#endif

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_test_conflict_zc");


#ifdef ZB_SECURITY
//  ZB_SET_NIB_SECURITY_LEVEL(0);
#endif

  zb_set_long_address(g_zc_addr);

  zb_set_pan_id(TEST_PAN_ID);

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_production_config_disable(ZB_TRUE);
  ZDO_CTX().conf_attr.permit_join_duration = ZB_DEFAULT_PERMIT_JOINING_DURATION;

  zb_secur_setup_nwk_key(gs_nwk_key, 0);

#ifndef SUBGIG
  /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
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

  ZB_SET_TRAF_DUMP_ON();
  zb_zdo_register_device_annce_cb(test_device_annce_cb);

  //ZB_BDB().bdb_join_uses_install_code_key = 1;
  ZB_TCPOL().require_installcodes = ZB_TRUE;

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
        /* No CBKE in that test */
        zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);

        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

        zb_secur_ic_add(g_ncp_addr, ZB_IC_TYPE_128, g_ic1, NULL);
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
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
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }

  if (param)
  {
    zb_buf_free(param);
  }
}

static void send_address_conflict(zb_uint8_t param)
{
  zb_bufid_t buf = zb_buf_get_out();
  ZVUNUSED(param);
  TRACE_MSG(TRACE_ERROR, ">>Sending address conflict message", (FMT__0));

  if (buf)
  {
    zb_nlme_send_status_t *request = ZB_BUF_GET_PARAM(buf, zb_nlme_send_status_t);

    request->dest_addr           = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    request->status.status       = ZB_NWK_COMMAND_STATUS_ADDRESS_CONFLICT;
    request->status.network_addr = zr_nwk_addr;
    request->ndsu_handle         = ZB_NWK_INTERNAL_NSDU_HANDLE;

    ZB_SCHEDULE_ALARM(zb_nlme_send_status, buf, ZB_NWK_OCTETS_TO_BI(ZB_NWKC_MAX_BROADCAST_JITTER_OCTETS));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "TEST FAILED: Could not get out buf!", (FMT__0));
  }

  TRACE_MSG(TRACE_ERROR, "<<Sending address conflict message", (FMT__0));
}

/*! @} */
