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
/* PURPOSE: ZR joins to ZC, then sends APS packet.
*/

#define ZB_TRACE_FILE_ID 41735
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "../gpmac/zb_gpmac_internal.h"
#include "zb_time.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

void send_data(zb_uint8_t param);
void data_indication(zb_uint8_t param);

zb_ieee_addr_t g_ieee_addr    = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MAIN()
{
  ARGV_UNUSED;

#ifndef KEIL
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_zr");

//  ZB_SET_NIB_SECURITY_LEVEL(0);

  /* set ieee addr */
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);

  /* set device type */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zdo_main_loop();
  }

  TRACE_MSG(TRACE_ERROR, "aps ext pan id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
  if (zb_buf_get_status(param) == 0)
  {
    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
    zb_af_set_data_indication(data_indication);

    ZB_SCHEDULE_ALARM(send_data, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)zb_buf_get_status(param)));
    zb_buf_free(param);
  }
}


void send_data(zb_uint8_t param)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_short_t i;
  zb_bufid_t buf;

  if (param)
  {
  }
  else
  {
    buf = zb_get_out_buf();
    param = param;
  }

  ZB_BUF_INITIAL_ALLOC(buf, ZB_TEST_DATA_SIZE, ptr);
  req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
  req->dst_addr.addr_short = 0; /* send to ZC */
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = 0; //ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;

  buf->u.hdr.handle = 0x11;

#if 0
  for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
  {
    ptr[i] = i % 32 + '0';
  }
#else
  {
  zb_uint32_t gp_time;
  zb_time_t t;

  HAL_TIMER_GET_CURRENT_TIME(gp_time);
  t = ZB_TIMER_CTX().timer;

  ZB_MEMSET(ptr, 0, ZB_TEST_DATA_SIZE);
  ZB_MEMCPY(ptr, &gp_time, sizeof(gp_time));
  ZB_MEMCPY(ptr + sizeof(gp_time), &t, sizeof(t));
  }
#endif


  TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
  ZB_SCHEDULE_ALARM(send_data, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
}


void data_indication(zb_uint8_t param)
{
  zb_ushort_t i;
  zb_uint8_t *ptr;
  zb_bufid_t asdu = (zb_bufid_t )param;

  /* Remove APS header from the packet */
  ZB_APS_HDR_CUT_P(asdu, ptr);

  TRACE_MSG(TRACE_APS3, "data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
                         asdu, (int)zb_buf_len(asdu), asdu->u.hdr.status));

  for (i = 0 ; i < zb_buf_len(asdu) ; ++i)
  {
    TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
    if (ptr[i] != i % 32 + '0')
    {
      TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                              (int)(i % 32 + '0'), (char)i % 32 + '0'));
    }
  }

  zb_free_buf(asdu);
//  send_data(asdu);
}


/*! @} */
