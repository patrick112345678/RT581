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
/* PURPOSE: ZED joins to ZC, then get APS packets from TCP, send it to ZC.
Get data from ZC, send over TCP.
*/

#define ZB_TRACE_FILE_ID 41736
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "lwip/tcp.h"
#include "tcp_echoserver.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

static int s_accept_tcp = 1;

struct tcp_echoserver_struct *last_es = NULL;

void data_indication(zb_uint8_t param);

zb_ieee_addr_t g_ieee_addr    = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};


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
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ED;

  LwIP_Init();
  tcp_echoserver_init();

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
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, (int)zb_buf_get_status(param)));
  }
  zb_buf_free(param);
}


/**
   Read from TCP and send to ZC.
   This routine called when TCP got some data.
 */
void read_tcp_send_data(struct tcp_echoserver_struct *es)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_bufid_t buf;

  /* To prevent going out of buffers, read next data from TCP only after got
   * echo on previous portion. */
  if (s_accept_tcp)
  {
    s_accept_tcp = 0;
    buf = zb_get_out_buf();

#define MAX_STRING_SIZE (ZB_IO_BUF_SIZE - sizeof(zb_apsde_data_req_t))

    ZB_BUF_INITIAL_ALLOC(buf, MAX_STRING_SIZE, ptr);
    memcpy(ptr, es->p->payload, es->p->len);
    zb_buf_len(buf) = es->p->len;
    TRACE_MSG(TRACE_APS1, "read %hd from TCP", (FMT__H, zb_buf_len(buf)));

    if (zb_buf_len(buf) > 0)
    {
      req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
      req->dst_addr.addr_short = 0; /* send to ZC */
      req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
      req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
      req->radius = 1;
      req->profileid = 2;
      req->src_endpoint = 10;
      req->dst_endpoint = 10;

      buf->u.hdr.handle = 0x11;
      TRACE_MSG(TRACE_APS3, "Sending apsde_data.request data len %hd", (FMT__H, zb_buf_len(buf)));
      ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
    }
    else
    {
      TRACE_MSG(TRACE_APS1, "read %hd from TCP - hmm", (FMT__H, zb_buf_len(buf)));
      zb_buf_free(param);
    }
  }
}


void data_indication(zb_uint8_t param)
{
  zb_uint8_t *ptr;
  zb_bufid_t asdu = (zb_bufid_t )param;

  /* Remove APS header from the packet */
  ZB_APS_HDR_CUT_P(asdu, ptr);

  TRACE_MSG(TRACE_APS3, "data_indication: packet %p len %d status 0x%x", (FMT__P_D_D,
                         asdu, (int)zb_buf_len(asdu), asdu->u.hdr.status));

  /* send received data to TCP producing echo */
  memcpy(last_es->p->payload, ptr, zb_buf_len(asdu));
  tcp_echoserver_send(last_es->pcb, last_es);

  /* We got echo - now accept input over TCP */
  s_accept_tcp = 1;
}


/*! @} */
