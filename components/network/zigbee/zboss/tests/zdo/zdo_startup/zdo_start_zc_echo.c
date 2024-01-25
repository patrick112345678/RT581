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
/* PURPOSE: Test for ZC application written using ZDO.
*/

#define ZB_TRACE_FILE_ID 41738
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


/*
  The test is: ZC starts PAN, ZR joins to it by association and send APS data packet, when ZC
  received packet, it sends packet to ZR, when ZR received packet, it sends
  packet to ZC etc.
 */

static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr);

void data_indication(zb_uint8_t param);

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_zc");

//  ZB_SET_NIB_SECURITY_LEVEL(0);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;

  /* Set to 1 to force entire nvram erase. */
  ZB_AIB().aps_nvram_erase_at_start = 1;
  /* set to 1 to enable nvram usage. */
  ZB_AIB().aps_use_nvram = 1;
  ZB_AIB().aps_channel_mask = (1L << 21);
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
  TRACE_MSG(TRACE_APS3, ">>zb_zdo_startup_complete status %d", (FMT__D, (int)zb_buf_get_status(param)));
  if (zb_buf_get_status(param) == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
    zb_af_set_data_indication(data_indication);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, (int)zb_buf_get_status(param)));
  }
  zb_buf_free(param);
}


/*
   Trivial test: dump all APS data received
 */


void data_indication(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_bufid_t asdu = (zb_bufid_t )param;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(asdu, zb_apsde_data_indication_t);

  /* Remove APS header from the packet */
  ZB_APS_HDR_CUT_P(asdu, ptr);
  (void)ptr;

  TRACE_MSG(TRACE_APS3, "apsde_data_indication: packet %p len %d status 0x%x", (FMT__P_D_D,
                         asdu, (int)zb_buf_len(asdu), asdu->u.hdr.status));

  /* send packet back to ZR */
  zc_send_data(asdu, ind->src_addr);
}

static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr)
{
    zb_apsde_data_req_t *req;

    req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
    req->dst_addr.addr_short = addr; /* send to ZR */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;
    buf->u.hdr.handle = 0x11;
    TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}

/*! @} */
