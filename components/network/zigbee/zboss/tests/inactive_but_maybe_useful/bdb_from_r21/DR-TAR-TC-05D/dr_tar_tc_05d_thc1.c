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


#define ZB_TEST_NAME DR_TAR_TC_05D_THC1
#define ZB_TRACE_FILE_ID 41130
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
#include "dr_tar_tc_05d_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

#define TEST_OPTIONS_LAST_OCTET_IS_ABSENT  0x01
#define TEST_OPTIONS_DST_IEEE_FIELD_ABSENT 0x02
#define TEST_OPTIONS_BROADCAST_CMD         0x04


extern void zdo_send_req_by_short(zb_uint16_t command_id, zb_uint8_t param,
                                  zb_callback_t cb, zb_uint16_t addr,
                                  zb_uint8_t resp_counter);

static void device_annce_cb(zb_zdo_device_annce_t *da);
static void buffer_manager(zb_uint8_t idx);
static void send_mgmt_leave(zb_uint8_t param, zb_uint16_t options);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thc1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thc1);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;
  ZB_AIB().aps_use_nvram = 1;

  zb_zdo_register_device_annce_cb(device_annce_cb);
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_COORDINATOR;
  ZB_AIB().aps_designated_coordinator = 1;
  zb_secur_setup_nwk_key(g_nwk_key, 0);

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
    ZB_SCHEDULE_ALARM(buffer_manager, 0, THC1_START_TEST_DELAY);
    ZB_SCHEDULE_ALARM(buffer_manager, 1, THC1_START_TEST_DELAY + THC1_SEND_LEAVE_DELAY);
    ZB_SCHEDULE_ALARM(buffer_manager, 2, THC1_START_TEST_DELAY + 2*THC1_SEND_LEAVE_DELAY);
    ZB_SCHEDULE_ALARM(buffer_manager, 3, THC1_START_TEST_DELAY + 3*THC1_SEND_LEAVE_DELAY);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}


static void buffer_manager(zb_uint8_t idx)
{
  TRACE_MSG(TRACE_ZDO1, ">>buffer_manager: idx = %d", (FMT__D, idx));

  switch (idx)
  {
    case 0:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_leave, TEST_OPTIONS_LAST_OCTET_IS_ABSENT);
      break;
    case 1:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_leave, TEST_OPTIONS_DST_IEEE_FIELD_ABSENT);
      break;
    case 2:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_leave, TEST_OPTIONS_BROADCAST_CMD);
      break;
    case 3:
      ZB_GET_OUT_BUF_DELAYED2(send_mgmt_leave, 0);
      break;
  }

  TRACE_MSG(TRACE_ZDO1, "<<buffer_manager", (FMT__0));
}


static void send_mgmt_leave(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint8_t* ptr;
  zb_uint8_t req_size = sizeof(zb_zdo_mgmt_leave_req_t);
  zb_uint16_t dest_addr;

  TRACE_MSG(TRACE_ZDO2, ">>send_mgmt_leave: buf_param = %d", (FMT__D, param));

  dest_addr = (options & TEST_OPTIONS_BROADCAST_CMD)? 0xffff
                                                    : zb_address_short_by_ieee(g_ieee_addr_dut);

  ZB_BUF_INITIAL_ALLOC(buf, req_size, ptr);
  ZB_IEEE_ADDR_COPY(ptr, g_ieee_addr_dut);
  ptr += sizeof(zb_ieee_addr_t);
  *ptr = 0;

  if (options & TEST_OPTIONS_LAST_OCTET_IS_ABSENT)
  {
    ZB_BUF_CUT_RIGHT(buf, sizeof(zb_uint8_t));
  }
  if (options & TEST_OPTIONS_DST_IEEE_FIELD_ABSENT)
  {
    ZB_BUF_CUT_LEFT(buf, sizeof(zb_ieee_addr_t), ptr);
  }

  zdo_send_req_by_short(ZDO_MGMT_LEAVE_REQ_CLID, param, NULL,
                        dest_addr, ZB_ZDO_CB_UNICAST_COUNTER);

  TRACE_MSG(TRACE_ZDO2, "<<send_mgmt_leave", (FMT__0));
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
