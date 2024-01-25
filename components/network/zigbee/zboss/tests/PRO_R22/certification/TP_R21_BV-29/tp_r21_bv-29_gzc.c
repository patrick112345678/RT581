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
/* PURPOSE: TP_R21_BV-29 ZigBee Coordinator (Gzc)
*/

#define ZB_TEST_NAME TP_R21_BV_29_GZC
#define ZB_TRACE_FILE_ID 40869

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

#ifndef ZB_USEALIAS
#error ZB_USEALIAS not defined!
#endif

static zb_bool_t is_initialized;
static zb_bool_t is_annce_sent;

static void device_annce_cb(zb_zdo_device_annce_t *da);

static const zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;
static const zb_ieee_addr_t g_ieee_addr_gzc = IEEE_ADDR_gZC;

zb_ieee_addr_t ieee_addr_ff = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

zb_uint16_t g_dut_addr = 0;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("gzc");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_gzc);
  zb_cert_test_set_common_channel_settings();
  zb_cert_test_set_zc_role();

  zb_set_use_extended_pan_id(g_ext_pan_id);
  zb_set_max_children(2);
  zb_set_pan_id(0x1aaa);
  is_initialized = ZB_FALSE;
  is_annce_sent = ZB_FALSE;
  zb_zdo_register_device_annce_cb(device_annce_cb);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key0, 0);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key1, 1);
  zb_zdo_set_aps_unsecure_join(ZB_TRUE);

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

void send_dev_annc(zb_uint8_t param, zb_uint16_t dut_addr)
{
#ifndef NCP_MODE_HOST
    zb_zdo_device_annce_t *da;
    zb_uint16_t taddr = dut_addr;
    zb_apsde_data_req_t *dreq = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));

    TRACE_MSG(TRACE_ERROR, "Send dev_annce %d 0x%x", (FMT__D_D, param, dut_addr));

    da = zb_buf_initial_alloc(param, sizeof(*da));
    zb_cert_test_inc_zdo_tsn();
    da->tsn = zb_cert_test_get_zdo_tsn();;
    ZB_HTOLE16(&da->nwk_addr, &taddr);
    /* setup ext addr to ffx8 */
    ZB_IEEE_ADDR_COPY(da->ieee_addr, ieee_addr_ff);
    da->capability = 0;
    ZB_MAC_CAP_SET_ALLOCATE_ADDRESS(da->capability, ZG->nwk.handle.rejoin_capability_alloc_address);

    if (ZB_U2B(ZB_PIBCACHE_RX_ON_WHEN_IDLE()))
    {
	ZB_MAC_CAP_SET_RX_ON_WHEN_IDLE(da->capability, 1);
    }

    ZB_BZERO(dreq, sizeof(*dreq));
    /* Broadcast to all devices for which macRxOnWhenIdle = TRUE.
       MAC layer in ZE sends unicast to its parent.
    */
    dreq->dst_addr.addr_short = ZB_NWK_BROADCAST_RX_ON_WHEN_IDLE;
    dreq->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    /* use default radius, max_depth * 2 */
    dreq->clusterid = ZDO_DEVICE_ANNCE_CLID;

    /* ise ZGP aliases to set gZC short source address as dut short address */
    dreq->use_alias = ZB_TRUE;
    dreq->alias_src_addr = dut_addr;
    dreq->alias_seq_num = 0x99;

    ZG->zdo.handle.dev_annce_param = param;

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
#else
    ZVUNUSED(param);
    ZVUNUSED(dut_addr);
    ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

void zb_test_step1(zb_uint8_t param)
{
  ZVUNUSED(param);
  zb_buf_get_out_delayed_ext(send_dev_annc,g_dut_addr, 0);
}

static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_APS2, ">> device_annce_cb, da %p", (FMT__P, da));

  TRACE_MSG(TRACE_APS2, ">> ieee_addr  " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(da->ieee_addr)));
  TRACE_MSG(TRACE_APS2, ">> nwk_addr 0x%hx tsn %hd caps %hd", (FMT__H_H_H, da->nwk_addr, da->tsn, da->capability));
  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dut))
  {
      TRACE_MSG(TRACE_APS2, "dut annce", (FMT__0));
      if(is_annce_sent == ZB_FALSE)
      {
        TRACE_MSG(TRACE_APS2, "Send Device_annce with alias", (FMT__0));
        g_dut_addr = da->nwk_addr;
        ZB_SCHEDULE_ALARM(zb_test_step1, 0, 10 * ZB_TIME_ONE_SECOND);
	is_annce_sent = ZB_TRUE;
      }
      else
      {
	TRACE_MSG(TRACE_APS2, "Not first annce, skip", (FMT__0));
      }
  }

  TRACE_MSG(TRACE_APS2, "<< device_annce_cb", (FMT__0));
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
      if (is_initialized == ZB_FALSE)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
	is_initialized = ZB_TRUE;
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

/*! @} */

