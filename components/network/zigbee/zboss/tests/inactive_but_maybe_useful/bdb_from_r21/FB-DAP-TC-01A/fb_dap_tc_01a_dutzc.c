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
/* PURPOSE:
*/


#define ZB_TEST_NAME FB_DAP_TC_01A_DUTZC
#define ZB_TRACE_FILE_ID 41195
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

#define DUT_ENDPOINT 8
#define HA_DOOR_LOCK_ENDPOINT DUT_ENDPOINT

/********************* Declare attributes  **************************/

/* Door Lock cluster attributes data */
/* Set attributes values as Initial Conditions on the test */
static zb_uint8_t lock_state       = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_STATE_LOCKED;
static zb_uint8_t lock_type        = ZB_ZCL_ATTR_DOOR_LOCK_LOCK_TYPE_DEADBOLT;
static zb_uint8_t actuator_enabled = ZB_ZCL_ATTR_DOOR_LOCK_ACTUATOR_ENABLED_ENABLED;


ZB_ZCL_DECLARE_DOOR_LOCK_ATTRIB_LIST(door_lock_attr_list,
    &lock_state,
    &lock_type,
    &actuator_enabled);

/* Basic cluster attributes data */
static zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
static zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
static zb_uint16_t g_attr_identify_time = 0;
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/* Groups cluster attributes data */
static zb_uint8_t g_attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list, &g_attr_name_support);


/********************* Declare device **************************/
ZB_HA_DECLARE_DOOR_LOCK_CLUSTER_LIST(door_lock_cluster,
                                     door_lock_attr_list,
                                     basic_attr_list,
                                     identify_attr_list,
                                     groups_attr_list);

ZB_HA_DECLARE_DOOR_LOCK_EP(door_lock_ep,
                           HA_DOOR_LOCK_ENDPOINT,
                           door_lock_cluster);

ZB_HA_DECLARE_DOOR_LOCK_CTX(door_lock_cluster_ctx, door_lock_ep);


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_1_zc");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr);
  ZB_BDB().bdb_primary_channel_set = (1 << 14);
  ZB_BDB().bdb_secondary_channel_set = 0;
  /* Start as ZC */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_BDB().bdb_mode = 1;
  ZB_AF_REGISTER_DEVICE_CTX(&door_lock_cluster_ctx);

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


// ![zb_bdb_finding_binding_target_example]
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

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Successfull steering", (FMT__0));
        zb_bdb_finding_binding_target(DUT_ENDPOINT);
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
// ![zb_bdb_finding_binding_target_example]



/*! @} */
