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
/* PURPOSE: TP/R21/BV-03 - Coordinator on channel #25 (gZC3)
*/

#define ZB_TEST_NAME TP_R21_BV_03_GZC3
#define ZB_TRACE_FILE_ID 40775

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "test_common.h"
#include "mac_internal.h"
#include "zb_mac_globals.h"
#include "../common/zb_cert_test_globals.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static const zb_ieee_addr_t g_ieee_addr_gzc3 = IEEE_ADDR_gZC3;

/* Continuously send data frames */
static void send_data_iteration(zb_uint8_t param);
static void stop_sending_data_packets(zb_uint8_t param);
static void send_mac_data(zb_uint8_t param);
static void send_mac_data_tx_cb(zb_uint8_t param);

#ifndef NCP_MODE_HOST
static void confirm_mac_data_cb(zb_uint8_t param);
#endif


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */
  ZB_INIT("zdo_3_gzc3");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
	
  zb_set_long_address(g_ieee_addr_gzc3);

  /* let's always be coordinator */
  zb_set_network_coordinator_role_legacy(1l << ZC3_CHANNEL);
  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key_zc, 0);

#ifdef SECURITY_LEVEL
  zb_cert_test_set_security_level(SECURITY_LEVEL);
#endif

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
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
//	ZB_SCHEDULE_ALARM(send_data_iteration, 0, SEND_DATA_ITERATION_DELAY);
//	ZB_SCHEDULE_ALARM(stop_sending_data_packets, 0, STOP_SENDING_DATA_PACKETS_DELAY);
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


static void send_data_iteration(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS3, ">>send_data_iteration", (FMT__0));

  ZVUNUSED(param);
  zb_buf_get_out_delayed(send_mac_data);

  TRACE_MSG(TRACE_APS3, "<<send_data_iteration", (FMT__0));
}


static void stop_sending_data_packets(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APS3, ">>stop_sending_data_packets", (FMT__0));

  ZVUNUSED(param);
  ZB_SCHEDULE_ALARM_CANCEL(send_data_iteration, ZB_ALARM_ALL_CB);

  TRACE_MSG(TRACE_APS3, "<<stop_sending_data_packets", (FMT__0));
}


static void send_mac_data(zb_uint8_t param)
{
  zb_uint16_t fc;
  zb_uint8_t* buf_ptr;
  int i;

  TRACE_MSG(TRACE_APS3, ">>send_mac_data: param = %d", (FMT__D, param));

  /* MAC DATA: FC(2)| SN(1)| SRC_PAN(2)| MAC_ADDR(8)| DATA(100)| FCS(2) = 115 bytes  */

  buf_ptr = (zb_uint8_t*) zb_buf_initial_alloc(param, 115);

  fc = 0;
  ZB_FCF_SET_FRAME_TYPE(&fc, MAC_FRAME_DATA);
  ZB_FCF_SET_SECURITY_BIT(&fc, 0);
  ZB_FCF_SET_FRAME_PENDING_BIT(&fc, 0);
  ZB_FCF_SET_ACK_REQUEST_BIT(&fc, 0);
  ZB_FCF_SET_PANID_COMPRESSION_BIT(&fc, 0);
  ZB_FCF_SET_FRAME_VERSION(&fc, MAC_FRAME_IEEE_802_15_4);
  ZB_FCF_SET_SRC_ADDRESSING_MODE((zb_uint8_t*) &fc, ZB_ADDR_64BIT_DEV);
  ZB_FCF_SET_DST_ADDRESSING_MODE((zb_uint8_t*) &fc, ZB_ADDR_NO_ADDR);

  buf_ptr = zb_put_next_htole16(buf_ptr, fc);
  *buf_ptr++ = zb_cert_test_get_mac_tsn(); /* Sequence number */
  zb_cert_test_inc_mac_tsn();
  buf_ptr = zb_put_next_htole16(buf_ptr, g_dutzr_pan_id);
  ZB_IEEE_ADDR_COPY(buf_ptr, g_ieee_addr_gzc3);
  buf_ptr += sizeof(zb_ieee_addr_t);

  /* 100 bytes of data */
  for (i = 0; i < 100; ++i)
  {
    *buf_ptr++ = (zb_uint8_t) (0x30 + i);
  }

#if 0
  PRINT_BUF_PAYLOAD_BY_REF(param);
#endif
  ZB_SCHEDULE_TX_CB(send_mac_data_tx_cb, param);

  TRACE_MSG(TRACE_APS3, "<<send_mac_data", (FMT__0));
}


static void send_mac_data_tx_cb(zb_uint8_t param)
{
#ifndef NCP_MODE_HOST
  MAC_CTX().tx_wait_cb = confirm_mac_data_cb;
  MAC_CTX().tx_wait_cb_arg = param;
  zb_mac_send_frame(param, 13);
#else
  ZVUNUSED(param);
  ZB_ASSERT(ZB_FALSE && "TODO: use NCP API here");
#endif
}

#ifndef NCP_MODE_HOST
static void confirm_mac_data_cb(zb_uint8_t param)
{
  zb_buf_free(param);
  ZB_SCHEDULE_ALARM(send_data_iteration, 0, SEND_DATA_ITERATION_DELAY);
}
#endif

/*! @} */

