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
/* PURPOSE: TH ZC1
*/

#define ZB_TEST_NAME TP_BDB_DR_TAR_TC_03D_THR1
#define ZB_TRACE_FILE_ID 40569

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_zcl.h"
#include "zb_bdb_internal.h"
#include "zb_zcl.h"
#include "../nwk/nwk_internal.h"

#include "tp_bdb_dr_tar_tc_03d_common.h"
#include "../common/zb_cert_test_globals.h"

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

#define TEST_OVERRIDE_REQUEST 0x01
#define TEST_HAS_DST_IEEE_ADDR 0x02

static zb_ieee_addr_t g_ieee_addr_thr1 = IEEE_ADDR_THR1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void buffer_manager(zb_uint8_t idx);
static void construct_and_send_leave(zb_uint8_t param, zb_uint16_t options);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_long_address(g_ieee_addr_thr1);

  zb_set_network_router_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  ZB_CERT_HACKS().enable_leave_to_router_hack = 1;

  zb_set_max_children(1);
  zb_zdo_register_device_annce_cb(device_annce_cb);

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


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_APP2, ">> device_annce_cb, da %p", (FMT__P, da));

  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dut))
  {
    ZB_SCHEDULE_ALARM(buffer_manager, 0, THR1_SEND_LEAVE_DELAY_START);
    ZB_SCHEDULE_ALARM(buffer_manager, 1, THR1_SEND_LEAVE_DELAY_START + THR1_SEND_LEAVE_DELAY_NEXT);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}


static void buffer_manager(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_ZDO1, ">>buffer_manager: idx = %d", (FMT__D, idx));

  switch (idx)
  {
    case 0:
      zb_buf_get_out_delayed_ext(construct_and_send_leave, TEST_HAS_DST_IEEE_ADDR | TEST_OVERRIDE_REQUEST, 0);
      /* BREAK */
      break;

    case 1:
      zb_buf_get_out_delayed_ext(construct_and_send_leave, 0, 0);
      /* BREAK */
      break;

  }

  TRACE_MSG(TRACE_ZDO1, "<<buffer_manager", (FMT__D, idx));
}


static void construct_and_send_leave(zb_uint8_t param, zb_uint16_t options)
{
  zb_nwk_hdr_t *nwhdr;
  zb_uint8_t *lp;
  zb_uint8_t hdr_size;
  zb_uint8_t has_dst_ieee = options & TEST_HAS_DST_IEEE_ADDR;
  zb_uint8_t *internal_leave_confirm;

  hdr_size = 3*sizeof(zb_uint16_t) + 2*sizeof(zb_uint8_t) + sizeof(zb_ieee_addr_t)
             + sizeof(zb_nwk_aux_frame_hdr_t);

  if (has_dst_ieee)
  {
    hdr_size += sizeof(zb_ieee_addr_t);
  }

  nwhdr = zb_buf_initial_alloc(param, hdr_size);

  ZB_BZERO2(nwhdr->frame_control);
  ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control,
  ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
  ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, 1);
  ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, 1, !!has_dst_ieee);
  zb_buf_flags_or(param, ZB_BUF_SECUR_NWK_ENCR);

  nwhdr->src_addr = zb_cert_test_get_network_addr();
  nwhdr->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  nwhdr->radius = 1;
  nwhdr->seq_num = zb_cert_test_get_nib_seq_number();
  zb_cert_test_inc_nib_seq_number();

  if (has_dst_ieee)
  {
    zb_get_long_address(nwhdr->src_ieee_addr);
    ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut);
  }
  else
  {
    zb_get_long_address(nwhdr->dst_ieee_addr);
  }

  lp =  (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, sizeof(zb_uint8_t));
  *lp = 0;
  ZB_LEAVE_PL_SET_REJOIN(*lp, ZB_FALSE);
  ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_FALSE);
  if ( !(options & TEST_OVERRIDE_REQUEST) )
  {
    ZB_LEAVE_PL_SET_REQUEST(*lp);
  }

  internal_leave_confirm = zb_buf_alloc_tail(param, sizeof(zb_uint8_t));
  *internal_leave_confirm = ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE;
  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, param);
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_uint8_t status = ZB_GET_APP_SIGNAL_STATUS(param);
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  TRACE_MSG(TRACE_APP1, ">>zb_zdo_startup_complete status %d", (FMT__D, status));

  switch (sig)
  {
    case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, status));
      }
      break; /* ZB_BDB_SIGNAL_DEVICE_FIRST_START */

    default:
      if (status == 0)
      {
	TRACE_MSG(TRACE_APS1, "Unknown signal, status OK", (FMT__0));
      }
      else
      {
	TRACE_MSG(TRACE_ERROR, "Unknown signal, status %d", (FMT__D, status));
      }
      break;
  }

  zb_buf_free(param);
}

/*! @} */
