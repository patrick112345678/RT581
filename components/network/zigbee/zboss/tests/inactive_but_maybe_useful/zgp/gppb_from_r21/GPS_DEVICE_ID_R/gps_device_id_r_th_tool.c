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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPS_DEVICE_ID_R_TH_TOOL
#define ZB_TRACE_FILE_ID 41320

#include "test_config.h"

#include "../include/zgp_test_templates.h"

#ifdef ZB_CERTIFICATION_HACKS

static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;

enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READ_SIMPLE_DESCRIPTOR,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_SIMPLE_TEST_TEMPLATE(TEST_DEVICE_CTX, 1000)

static void simple_desc_req(zb_uint8_t buf_ref, zb_uint16_t short_addr, zb_uint8_t ep)
{
  zb_buf_t                 *buf = ZB_BUF_FROM_REF(buf_ref);
  zb_zdo_simple_desc_req_t *req;

  TRACE_MSG(TRACE_APP3, "> simple_desc_req, buf_ref %hd, addr 0x%x, ep %hd",
            (FMT__H_D_H, buf_ref, short_addr, ep));

  ZB_ASSERT(short_addr != ZB_UNKNOWN_SHORT_ADDR);
  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_simple_desc_req_t), req);
  ZB_BZERO(req, sizeof(zb_zdo_simple_desc_req_t));
  req->nwk_addr = short_addr;
  req->endpoint = ep;

  zb_zdo_simple_desc_req(buf_ref, NULL);

  TRACE_MSG(TRACE_APP3, "< simple_desc_req", (FMT__0));
}

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  ZVUNUSED(buf_ref);
  ZVUNUSED(cb);
}

static void perform_next_state(zb_uint8_t param)
{
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_SIMPLE_DESCRIPTOR:
      if (!param)
      {
        TEST_DEVICE_CTX.test_state--;
        ZB_GET_OUT_BUF_DELAYED(perform_next_state);
        return;
      }
      simple_desc_req(param, DUT_GPS_ADDR, ZGP_ENDPOINT);
	  ZB_SCHEDULE_ALARM(perform_next_state, 0, ZB_TIME_ONE_SECOND);
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static void zgpc_custom_startup()
{
/* Init device, load IB values from nvram or set it to default */
  ZB_INIT("th_tool");

  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_set_default_ed_descriptor_values();
}

#endif /* ZB_CERTIFICATION_HACKS */

ZB_ZGPC_TH_DECLARE_STARTUP_PROCESS()
