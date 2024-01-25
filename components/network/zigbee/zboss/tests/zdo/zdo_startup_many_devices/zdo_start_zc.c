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
#define ZB_TRACE_FILE_ID 40036
#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_startup.h"
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

zb_ieee_addr_t     g_zc_addr = {0xde, 0xad, 0xf0, 0x0d, 0xde, 0xad, 0xf0, 0x0d};

static zb_uint_t         g_packets_rcvd;
static zb_uint_t         g_packets_sent;
static zb_uint16_t       g_remote_addr;

static void zc_send_data(zb_uint8_t param, zb_uint16_t address);
void zc_send_data_loop(zb_uint8_t param);

static void test_exit(zb_uint8_t param)
{
  ZB_ASSERT(param == 0);
}


static zb_bool_t exchange_finished()
{
  return ZB_FALSE;
}


static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t ze_addr = TEST_ZE_ADDR;
  zb_ieee_addr_t zr_addr = TEST_ZR_ADDR;

  if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, ze_addr) &&
      !ZB_IEEE_ADDR_CMP(da->ieee_addr, zr_addr))
  {
    TRACE_MSG(TRACE_ERROR, "Unknown device has joined!", (FMT__0));
  }
  /* Use first joined device as destination for outgoing APS packets */
  if (g_remote_addr == 0)
  {
    g_remote_addr = da->nwk_addr;
    {
      /* zb_bufid_t buf = zb_buf_get_out(); */
      /* ZB_ASSERT(buf); */
      /* ZB_SCHEDULE_CALLBACK(zc_send_data, buf); */
    }
  }
}


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */
  ZB_SET_TRACE_MASK(0x0);

  ZB_INIT("zdo_zc");

  zb_set_long_address(g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  zb_set_network_coordinator_role_legacy(1l << CHANNEL);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_SET_TRAF_DUMP_ON();
  zb_zdo_register_device_annce_cb(test_device_annce_cb);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zboss_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}



zb_uint8_t data_indication(zb_uint8_t param){
  zb_ushort_t i;
  zb_uint8_t *ptr;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (ind->profileid == 0x0002)
  {
    /* Remove APS header from the packet */
    ZB_APS_HDR_CUT_P(param, ptr);

    TRACE_MSG(TRACE_APS3, "apsde_data_indication: packet %p %d len %d status 0x%x",
              (FMT__P_H_D_D,
              ptr, param, (int)zb_buf_len(param), zb_buf_get_status(param)));


    for (i = 0 ; i < zb_buf_len(param) ; ++i)
    {
      TRACE_MSG(TRACE_APS3, "%x (%c)", (FMT__D_C, (int)ptr[i], ptr[i]));
      if (ptr[i] != i % 32 + '0')
      {
        TRACE_MSG(TRACE_ERROR, "Bad data %hx (%c) wants %hx (%c)",
                  (FMT__H_C_H_C, ptr[i], ptr[i],
                  (zb_ushort_t)(i % 32 + '0'), (char)(i % 32 + '0')));
      }
    }
    g_packets_rcvd++;
    if (exchange_finished())
    {
      ZB_SCHEDULE_ALARM(test_exit, 0, 1*ZB_TIME_ONE_SECOND);
    }

    zc_send_data(param, ind->src_addr);
    /* ZB_SCHEDULE_ALARM(zc_send_data, param, ZB_TIME_ONE_SECOND * 5); */

    return ZB_TRUE;               /* processed */
  }

  return ZB_FALSE;
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
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_ERROR, "Device STARTED OK", (FMT__0));
        zb_af_set_data_indication(data_indication);
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


static void zc_send_data(zb_uint8_t param, zb_uint16_t address)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_short_t i;

  ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);
  req = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = address;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;
  req->clusterid = 0;
  zb_buf_set_handle(param, 0x11);
  for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
  {
    ptr[i] = i % 32 + '0';
  }
  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request %hd", (FMT__H, param));
  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}


void zc_send_data_loop(zb_uint8_t param)
{
  zb_bufid_t buf;

  ZB_ASSERT(param == 0);
  ZVUNUSED(param);
  buf = zb_buf_get_out();
  if (buf == 0)
  {
    TRACE_MSG(TRACE_ERROR, "Can't allocate buffer!", (FMT__0));
    ZB_SCHEDULE_ALARM(zc_send_data_loop, 0, 1*ZB_TIME_ONE_SECOND);
    return;
  }
  /* zc_send_data(buf); */
  g_packets_sent++;
  if (g_packets_sent < PACKETS_FROM_ZC_NR)
  {
    ZB_SCHEDULE_ALARM(zc_send_data_loop, 0, 1*ZB_TIME_ONE_SECOND);
  }
  else
  {
    if (exchange_finished())
    {
      ZB_SCHEDULE_ALARM(test_exit, 0, 1*ZB_TIME_ONE_SECOND);
    }
  }
}

/*! @} */
