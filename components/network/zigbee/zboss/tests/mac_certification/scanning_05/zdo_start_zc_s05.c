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

#define ZB_TRACE_FILE_ID 41701
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#define ZB_TEST_ADDR  0xfffe
#define TEST_PAN_ID   0x1AAA
#define ZB_TEST_BEACON_PAYLOAD_LENGTH 0
#define ZB_TEST_BEACON_PAYLOAD "\0"
#define ZB_TEST_ASSOCIATION_PERMIT 0
#define ZB_TEST_CHANNEL 0x14


zb_ieee_addr_t g_ieee_addr = {0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC}; /* test extended addres */


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

void tst_set_channel()
{
  zb_mlme_set_request_t *req;
  zb_bufid_t buf = zb_get_out_buf();

  TRACE_MSG(TRACE_NWK2, "tst_set_channel", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  req->pib_attr = ZB_PHY_PIB_CURRENT_CHANNEL;
  req->pib_length = sizeof(zb_uint8_t);
  *((zb_uint16_t *)(req + 1)) = ZB_TEST_CHANNEL;
  zb_mlme_set_request(param);
}

void tst_set_short_addr()
{
  zb_mlme_set_request_t *req;
  zb_bufid_t buf = zb_get_out_buf();

  TRACE_MSG(TRACE_NWK2, "set_short_addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  *((zb_uint16_t *)(req + 1)) = ZB_TEST_ADDR;
  zb_mlme_set_request(param);
}

void tst_set_assosiation_permit()
{
  zb_mlme_set_request_t *req;
  zb_bufid_t buf = zb_get_out_buf();

  TRACE_MSG(TRACE_NWK2, "tst_set_assosiation_permit", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  req->pib_attr = ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT;
  req->pib_length = sizeof(zb_uint16_t);
  *((zb_uint8_t *)(req + 1)) = ZB_TEST_ASSOCIATION_PERMIT;
  zb_mlme_set_request(param);
}

void tst_beacon_payload()
{
  zb_mlme_set_request_t *req;
  zb_bufid_t buf = zb_get_out_buf();
  zb_uint8_t size;

  TRACE_MSG(TRACE_NWK2, "tst_beacon_payload", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_beacon_payload_t));
  req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD;
  req->pib_length = sizeof(zb_mac_beacon_payload_t);
  *((zb_uint8_t *)(req + 1)) = ZB_TEST_BEACON_PAYLOAD_LENGTH;

  size = sizeof(zb_mac_beacon_payload_t);
  if (size > ZB_TEST_BEACON_PAYLOAD_LENGTH)
  {
    size = ZB_TEST_BEACON_PAYLOAD_LENGTH;
  }
  ZB_MEMSET((zb_uint8_t *)(req + 1), 0, sizeof(zb_mac_beacon_payload_t));
  ZB_MEMCPY((zb_uint8_t *)(req + 1), ZB_TEST_BEACON_PAYLOAD, size);
  zb_mlme_set_request(param);
}

void tst_beacon_payload_len()
{
  zb_mlme_set_request_t *req;
  zb_bufid_t buf = zb_get_out_buf();

  TRACE_MSG(TRACE_NWK2, "tst_beacon_payload_len", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD_LENGTH;
  req->pib_length = sizeof(zb_uint8_t);
  *((zb_uint8_t *)(req + 1)) = ZB_TEST_BEACON_PAYLOAD_LENGTH;
  zb_mlme_set_request(param);
}

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

#if !(defined KEIL || defined SDCC)
  if ( argc < 3 )
  {
    printf("%s <read pipe path> <write pipe path>\n", argv[0]);
    return 0;
  }
#endif

  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("tst_zc");

  ZB_SET_NIB_SECURITY_LEVEL(0);
#ifdef ZB8051
  TRACE_MSG(TRACE_ERROR, "ZB_8051_TIMER_VALUE %d", (FMT__D,ZB_8051_TIMER_VALUE));
#endif


  ZB_IEEE_ADDR_COPY(ZB_PIB_EXTENDED_ADDRESS(), &g_ieee_addr);
  MAC_PIB().mac_pan_id = TEST_PAN_ID;

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  /* channel # 0x14 */
  ZB_AIB().aps_channel_mask = (1l << ZB_TEST_CHANNEL);

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
    tst_set_assosiation_permit();
    tst_beacon_payload();
    tst_beacon_payload_len();
    tst_set_short_addr();

    TRACE_MSG(TRACE_APS1, "set beacon payload %x %x ",
              (FMT__H_H, ((zb_uint8_t*)&MAC_PIB().mac_beacon_payload)[0], ((zb_uint8_t*)&MAC_PIB().mac_beacon_payload)[1]));

    TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
    zb_af_set_data_indication(data_indication);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, (int)zb_buf_get_status(param)));
  }
  zb_buf_free(param);
}

void data_indication(zb_uint8_t param)
{
  ZVUNUSED(param);
}

static void zc_send_data(zb_bufid_t buf, zb_uint16_t addr)
{
  ZVUNUSED(buf);
  ZVUNUSED(addr);
}


/*! @} */
