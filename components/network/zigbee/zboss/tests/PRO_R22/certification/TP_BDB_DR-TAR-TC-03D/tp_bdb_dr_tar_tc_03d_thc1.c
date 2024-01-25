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
/* PURPOSE: DUT ZC
*/

#define ZB_TEST_NAME TP_BDB_DR_TAR_TC_03D_THC1
#define ZB_TRACE_FILE_ID 40567

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

/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

static zb_ieee_addr_t g_ieee_addr_thc1 = IEEE_ADDR_THC1;
static zb_ieee_addr_t g_ieee_addr_dut = IEEE_ADDR_DUT;

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void buffer_manager(zb_uint8_t idx);
static void construct_and_send_leave(zb_uint8_t param);

static zb_uint16_t TEST_PAN_ID = 0x1AAA;

MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thc1");
#if UART_CONTROL	
	test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif


  zb_set_pan_id(TEST_PAN_ID);
  zb_set_long_address(g_ieee_addr_thc1);

  zb_set_network_coordinator_role((1l << TEST_CHANNEL));
  zb_set_nvram_erase_at_start(ZB_TRUE);

  zb_secur_setup_nwk_key((zb_uint8_t*) g_nwk_key, 0);

  zb_set_max_children(1);

  /* allow joining without perm_join (for connection through zr) */
  ZB_TCPOL().authenticate_always = 1;

  zb_zdo_register_device_annce_cb(device_annce_cb);

//  ZB_BDB().allow_remote_policy_change = 0;

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
    ZB_SCHEDULE_ALARM(buffer_manager, 0, THC1_SEND_LEAVE_DELAY_DUT_ZED);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}

static void buffer_manager(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_ZDO1, ">>buffer_manager: idx = %d", (FMT__D, idx));

  zb_buf_get_out_delayed(construct_and_send_leave);

  TRACE_MSG(TRACE_ZDO1, "<<buffer_manager", (FMT__D, idx));
}

static void construct_and_send_leave(zb_uint8_t param)
{
  zb_nwk_hdr_t *nwhdr;
  zb_uint8_t *lp;
  zb_uint8_t hdr_size;

  TRACE_MSG(TRACE_ZDO1, "construct_and_send_leave", (FMT__0));

  hdr_size = 3*sizeof(zb_uint16_t) + 2*sizeof(zb_uint8_t) + 2*sizeof(zb_ieee_addr_t)
             + sizeof(zb_nwk_aux_frame_hdr_t) ;

  nwhdr = zb_buf_initial_alloc(param, hdr_size);

  ZB_BZERO2(nwhdr->frame_control);
  ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control,
  ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
  ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, 1);
  ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, 1, 1);
  zb_buf_flags_or(param, ZB_BUF_SECUR_NWK_ENCR);

  nwhdr->src_addr = zb_cert_test_get_network_addr();
  nwhdr->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  nwhdr->radius = 1;
  nwhdr->seq_num = zb_cert_test_get_nib_seq_number();
  zb_cert_test_inc_nib_seq_number();

  zb_get_long_address(nwhdr->src_ieee_addr);
  ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut);

  lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(param, ZB_NWK_CMD_LEAVE, sizeof(zb_uint8_t));
  *lp = 0;
  ZB_LEAVE_PL_SET_REJOIN(*lp, ZB_FALSE);
  ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_FALSE);
  ZB_LEAVE_PL_SET_REQUEST(*lp);

#if 1
  zb_mcps_build_data_request(param, ZB_PIBCACHE_NETWORK_ADDRESS(),
                             zb_address_short_by_ieee(g_ieee_addr_dut),
                             MAC_TX_OPTION_ACKNOWLEDGED_BIT,
                             ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE);
#endif
  zb_nwk_unlock_in(param);
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);

  /* Convert addresses to LE order */
  ZB_NWK_ADDR_TO_LE16(nwhdr->dst_addr);
  ZB_NWK_ADDR_TO_LE16(nwhdr->src_addr);
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

        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
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
