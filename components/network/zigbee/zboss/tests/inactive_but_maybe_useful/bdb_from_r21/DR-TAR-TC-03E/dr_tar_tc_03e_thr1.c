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
/* PURPOSE: TH ZR1
*/


#define ZB_TEST_NAME DR_TAR_TC_03E_THR1
#define ZB_TRACE_FILE_ID 40962
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
#include "dr_tar_tc_03e_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */


#define TEST_OVERRIDE_REQUEST 0x01
#define TEST_HAS_DST_IEEE_ADDR 0x02

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void buffer_manager(zb_uint8_t idx);
static void construct_and_send_leave(zb_uint8_t param, zb_uint16_t options);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_CERT_HACKS().enable_leave_to_router_hack = 1;
  ZB_AIB().aps_use_nvram = 1;

  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_NIB().max_children = 1;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);
  zb_zdo_register_device_annce_cb(device_annce_cb);

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


static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_APP2, ">> device_annce_cb, da %p", (FMT__P, da));

  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dut))
  {
    ZB_SCHEDULE_ALARM(buffer_manager, 0, THR1_START_TEST_DELAY);
    ZB_SCHEDULE_ALARM(buffer_manager, 1, THR1_START_TEST_DELAY + 2*ZB_TIME_ONE_SECOND);
  }


  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}


static void buffer_manager(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_ZDO1, ">>buffer_manager: idx = %d", (FMT__D, idx));

  switch (idx)
  {
    case 0:
      ZB_GET_OUT_BUF_DELAYED2(construct_and_send_leave,
                              TEST_OVERRIDE_REQUEST | TEST_HAS_DST_IEEE_ADDR);
      /* BREAK */
      break;

    case 1:
      ZB_GET_OUT_BUF_DELAYED2(construct_and_send_leave, 0);
      /* BREAK */
      break;
  }

  TRACE_MSG(TRACE_ZDO1, "<<buffer_manager", (FMT__D, idx));
}


static void construct_and_send_leave(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_nwk_hdr_t *nwhdr;
  zb_uint8_t *lp;
  zb_uint8_t hdr_size;
  zb_uint8_t has_dst_ieee = options & TEST_HAS_DST_IEEE_ADDR;

  hdr_size = 3*sizeof(zb_uint16_t) + 2*sizeof(zb_uint8_t) + sizeof(zb_ieee_addr_t)
             + sizeof(zb_nwk_aux_frame_hdr_t);

  if (has_dst_ieee)
  {
    hdr_size += sizeof(zb_ieee_addr_t);
  }

  ZB_BUF_INITIAL_ALLOC(buf, hdr_size, nwhdr);

  ZB_BZERO2(nwhdr->frame_control);
  ZB_NWK_FRAMECTL_SET_FRAME_TYPE_N_PROTO_VER(nwhdr->frame_control,
  ZB_NWK_FRAME_TYPE_COMMAND, ZB_PROTOCOL_VERSION);
  ZB_NWK_FRAMECTL_SET_SECURITY(nwhdr->frame_control, 1);
  ZB_NWK_FRAMECTL_SET_SRC_DEST_IEEE(nwhdr->frame_control, 1, !!has_dst_ieee);
  buf->u.hdr.encrypt_type |= ZB_SECUR_NWK_ENCR;

  nwhdr->src_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
  nwhdr->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  nwhdr->radius = 1;
  nwhdr->seq_num = ZB_NIB_SEQUENCE_NUMBER();
  ZB_NIB_SEQUENCE_NUMBER_INC();
  ZB_IEEE_ADDR_COPY(nwhdr->src_ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());

  if (has_dst_ieee)
  {
    ZB_IEEE_ADDR_COPY(nwhdr->src_ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
    ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, g_ieee_addr_dut);
  }
  else
  {
    ZB_IEEE_ADDR_COPY(nwhdr->dst_ieee_addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
  }

  lp = (zb_uint8_t *)nwk_alloc_and_fill_cmd(buf, ZB_NWK_CMD_LEAVE, sizeof(zb_uint8_t));
  *lp = 0;
  ZB_LEAVE_PL_SET_REJOIN(*lp, ZB_FALSE);
  ZB_LEAVE_PL_SET_REMOVE_CHILDREN(*lp, ZB_FALSE);
  if ( !(options & TEST_OVERRIDE_REQUEST) )
  {
    ZB_LEAVE_PL_SET_REQUEST(*lp);
  }

  ZB_SET_BUF_PARAM(buf, ZB_NWK_INTERNAL_LEAVE_CONFIRM_AT_DATA_CONFIRM_HANDLE, zb_uint8_t);
  ZB_SCHEDULE_CALLBACK(zb_nwk_forward, ZB_REF_FROM_BUF(buf));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal", (FMT__0));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
  }
  zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
