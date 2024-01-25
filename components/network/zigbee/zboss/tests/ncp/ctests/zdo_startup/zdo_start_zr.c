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

#define ZB_TRACE_FILE_ID 40039
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_startup.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#define CHANNEL_MASK (1l << 11)

static void send_data(zb_uint8_t param);
zb_uint8_t data_indication(zb_uint8_t param);
zb_ieee_addr_t g_ieee_addr = TEST_ZR_ADDR;

MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRACE_MASK(TRACE_SUBSYSTEM_APP | TRACE_SUBSYSTEM_TRANSPORT | TRACE_SUBSYSTEM_ZDO);
  ZB_INIT("zdo_zr");

  zb_set_long_address(g_ieee_addr);
  zb_set_network_router_role(CHANNEL_MASK);

  zb_set_nvram_erase_at_start(ZB_TRUE);

  if (zboss_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zboss_start failed", (FMT__0));
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
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  switch(sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_DEVICE_FIRST_START", (FMT__0));

      TRACE_MSG(TRACE_APP1, "Device started with status %hd", (FMT__H, status));

      if (status == 0)
      {
        zb_af_set_data_indication(data_indication);
        ZB_SCHEDULE_APP_ALARM(send_data, 0, 5 * ZB_TIME_ONE_SECOND);
      }
      break;

    case ZB_BDB_SIGNAL_DEVICE_REBOOT:
      TRACE_MSG(TRACE_APP1, "ZB_BDB_SIGNAL_DEVICE_REBOOT", (FMT__0));

      TRACE_MSG(TRACE_APP1, "Device rebooted, status %hd", (FMT__H, status));
      break;

    default:
      break;
  }
}


static void send_data(zb_uint8_t param)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_short_t i;

  if (param == ZB_BUF_INVALID)
  {
    zb_buf_get_out_delayed(send_data);
    return;
  }

  ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);
  req = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
  req->dst_addr.addr_short = 0; /* send to ZC */
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;

  for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
  {
    ptr[i] = i % 32 + '0';
  }

  TRACE_MSG(TRACE_APP3, "Sending apsde_data.request", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}


zb_uint8_t data_indication(zb_uint8_t param)
{
  zb_ushort_t i;
  zb_uint8_t *ptr;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (ind->profileid == 0x0002)
  {
    ptr = (zb_uint8_t *)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP3, "data_indication: packet %hd len %d handle 0x%x",
            (FMT__H_D_D, param, (int)zb_buf_len(param), zb_buf_get_status(param)));

    for (i = 0 ; i < zb_buf_len(param) ; ++i)
    {
      TRACE_MSG(TRACE_APP3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
      if (ptr[i] != i % 32 + '0')
      {
        TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c",
          (FMT__H_C_D_C, ptr[i], ptr[i], (int)(i % 32 + '0'), (char)i % 32 + '0'));
      }
    }

    zb_buf_free(param);

    ZB_SCHEDULE_ALARM(send_data, 0, ZB_TIME_ONE_SECOND * 5);

    return ZB_TRUE;
  }

  return ZB_FALSE;
}


/*! @} */
