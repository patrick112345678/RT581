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
/* PURPOSE: TH ZR2 (target)
*/


#define ZB_TEST_NAME DR_TAR_TC_05A_THR2
#define ZB_TRACE_FILE_ID 41224
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
#include "dr_tar_tc_05a_common.h"
#include "on_off_client.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */
/******************* Declare attributes ************************/

/* Basic cluster attributes data */
static zb_uint8_t attr_zcl_version  = ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE;
static zb_uint8_t attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &attr_zcl_version, &attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t attr_identify_time = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &attr_identify_time);


/********************* Declare device **************************/
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(on_off_client_clusters,
                                   basic_attr_list,
                                   identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(on_off_client_ep, TH_ZCRX_ENDPOINT, on_off_client_clusters);

DECLARE_ON_OFF_CLIENT_CTX(on_off_client_ctx, on_off_client_ep);


/**********************General definitions for test***********************/
static void device_annce_cb(zb_zdo_device_annce_t *da);
static void trigger_rejoin_delayed(zb_uint8_t unused);
static void trigger_rejoin(zb_uint8_t param);
static void mute_thr1c2_packets(zb_uint8_t unused);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr2");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr2);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;

  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_AF_REGISTER_DEVICE_CTX(&on_off_client_ctx);
  zb_zdo_register_device_annce_cb(device_annce_cb);
  ZB_NIB().max_children = 0;

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


/******************************Implementation********************************/
static void device_annce_cb(zb_zdo_device_annce_t *da)
{
  TRACE_MSG(TRACE_APP2, ">> device_annce_cb, da %p", (FMT__P, da));

  if (ZB_IEEE_ADDR_CMP(da->ieee_addr, g_ieee_addr_dut))
  {
    TRACE_MSG(TRACE_APP2, "device_annce_cb: DUT joins", (FMT__0));
    ZB_SCHEDULE_ALARM(trigger_rejoin_delayed, 0, THR2_REJOIN_TO_NETWORK_DELAY);
  }

  TRACE_MSG(TRACE_APP2, "<< device_annce_cb", (FMT__0));
}


static void trigger_rejoin_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_GET_OUT_BUF_DELAYED(trigger_rejoin);
}


static void trigger_rejoin(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_nlme_leave_request_t *req = ZB_GET_BUF_PARAM(buf, zb_nlme_leave_request_t);

  TRACE_MSG(TRACE_APS1, ">>trigger_rejoin: buf_param = %d", (FMT__D, param));

  req->remove_children = 0;
  req->rejoin = 1;
  ZB_IEEE_ADDR_ZERO(req->device_address);
  zb_nlme_leave_request(param);

  TRACE_MSG(TRACE_APS1, "<<trigger_rejoin", (FMT__0));
}


static void mute_thr1c2_packets(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  MAC_ADD_INVISIBLE_SHORT(zb_address_short_by_ieee(g_ieee_addr_thr1c2));
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
        ZB_SCHEDULE_ALARM(mute_thr1c2_packets, 0, THR2_MUTE_THR1C2_PACKETS);
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
