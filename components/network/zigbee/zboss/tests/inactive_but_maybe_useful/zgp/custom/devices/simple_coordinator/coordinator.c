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
/* PURPOSE: Simple coordinator for GP device
*/

#define ZB_TRACE_FILE_ID 41561
#include "zb_common.h"

#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zboss_api_zgp.h"
#include "zb_zcl.h"
#include "match_info.h"

#include "zb_ha.h"

#include "sample_controller.h"

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif


zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#define ENDPOINT  10
#define  TEST_ZGPD_SRC_ID 0x12345678
#define RESTART_COMM_TIMEOUT 3 * ZB_TIME_ONE_SECOND

/******************* Declare attributes ************************/

/* Basic cluster attributes data */
zb_uint8_t g_attr_zcl_version  = ZB_ZCL_VERSION;
zb_uint8_t g_attr_power_source = ZB_ZCL_BASIC_POWER_SOURCE_UNKNOWN;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attr_list, &g_attr_zcl_version, &g_attr_power_source);

/* Identify cluster attributes data */
zb_uint16_t g_attr_identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list, &g_attr_identify_time);

/********************* Declare device **************************/

ZB_HA_DECLARE_SAMPLE_CLUSTER_LIST( sample_clusters,
          basic_attr_list, identify_attr_list);

ZB_HA_DECLARE_SAMPLE_EP(sample_ep, ENDPOINT, sample_clusters);

ZB_HA_DECLARE_SAMPLE_CTX(sample_ctx, sample_ep);

/******************* Declare server parameters *****************/

/******************* Declare test data & constants *************/

MAIN()
{
  ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

/* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zc");



  zb_set_default_ed_descriptor_values();

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_zc_addr);
  ZB_PIBCACHE_PAN_ID() = 0x1aaa;

  ZB_AIB().aps_designated_coordinator = 1;

  ZB_ZGP_SET_MATCH_INFO(&g_zgps_match_info);

  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);

  ZB_SET_NIB_SECURITY_LEVEL(0);

  if (zdo_dev_start() != RET_OK)
  {
    TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
  }
  else
  {
    zcl_main_loop();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}

void start_comm(zb_uint8_t param)
{
  ZVUNUSED(param);
  TRACE_MSG(TRACE_APP1, "start commissioning again", (FMT__0));
  zb_zgps_start_commissioning(0);
  ZB_SCHEDULE_ALARM(start_comm, 0, RESTART_COMM_TIMEOUT);
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %h", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
    start_comm(0);
  }
  else
  {
    TRACE_MSG(
        TRACE_ERROR,
        "Device started FAILED status %d",
        (FMT__D, (int)buf->u.hdr.status));
    zb_free_buf(buf);
  }
  TRACE_MSG(TRACE_APP1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK

#include <stdio.h>
MAIN()
{
  ARGV_UNUSED;

  printf("HA profile and ZGP sink role should be enabled in zb_config.h\n");

  MAIN_RETURN(1);
}

#endif // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_SINK
