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

#define ZB_TRACE_FILE_ID 41570
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
#include "test_config.h"

#define COMMISSIONING_TIMEOUT  0
#define DECOMMISSIONING_TIMEOUT 0

/* Number of times report attribute callback should be called.
 * Every attribute report from ZGPD can contain one or two records.
 * It depends on ZGPD type: whether it uses manufacturer-specific
 * commissioning or not. If attribute report contains two records,
 * then report attribute callback will be called twice for every
 * ZGPD attribute report */
static int  gs_expected_report_attr_call_count;

#if ! defined ZB_COORDINATOR_ROLE
#error define ZB_COORDINATOR_ROLE to compile zc tests
#endif


zb_ieee_addr_t g_zc_addr = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
#define ENDPOINT  10

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

zb_uint8_t report_count = 0;
zb_uint8_t g_error_cnt = 0;


void commissioning_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  TRACE_MSG(TRACE_APP1, ">> commissioning_cb result %d", (FMT__D, result));

  if ((zgpd_id->app_id != ZB_ZGP_APP_ID_0000)
      || (zgpd_id->addr.src_id != TEST_ZGPD_SRC_ID))
  {
    g_error_cnt++;
    TRACE_MSG(TRACE_ERROR, "Unexpected src_id (%d) in commissioning cb",
        (FMT__D, zgpd_id->addr.src_id));
  }

  if (result == ZB_ZGP_ZGPD_DECOMMISSIONED)
  {
    if (report_count==gs_expected_report_attr_call_count && g_error_cnt==0)
    {
       TRACE_MSG(TRACE_ERROR, "Test finished. Status: OK", (FMT__0));
    }
  }
  else if (result != ZB_ZGP_COMMISSIONING_COMPLETED)
  {
    TRACE_MSG(TRACE_ERROR, "Commissioning FAILED status %d",
        (FMT__D, result));
    g_error_cnt++;
  }

  TRACE_MSG(TRACE_APP1, "<< commissioning_cb ", (FMT__0));
}


void comm_req_cb(
    zb_zgpd_id_t  *zgpd_id,
    zb_uint8_t    device_id,
    zb_uint16_t   manuf_id,
    zb_uint16_t   manuf_model_id)
{
  TRACE_MSG(TRACE_APP1, ">> comm_req_cb zgpd_id %p, dev_id 0x%hx, manuf_id %d, manuf_model_id 0x%x",
      (FMT__P_H_D_H, zgpd_id, device_id, manuf_id, manuf_model_id));

  ZVUNUSED(manuf_id);
  ZVUNUSED(manuf_model_id);

  if ((zgpd_id->app_id != ZB_ZGP_APP_ID_0000)
      || (zgpd_id->addr.src_id != TEST_ZGPD_SRC_ID))
  {
    g_error_cnt++;
    TRACE_MSG(TRACE_ERROR, "Unexpected src_id (%d) in commissioning cb",
        (FMT__D, zgpd_id->addr.src_id));
  }

  if (device_id == ZB_ZGP_MANUF_SPECIFIC_DEV_ID)
  {
    gs_expected_report_attr_call_count = REPORT_COUNT;
  }
  else
  {
    gs_expected_report_attr_call_count = REPORT_COUNT*2;
  }

  zb_zgps_accept_commissioning(ZB_TRUE);

  TRACE_MSG(TRACE_APP1, "<< comm_req_cb ", (FMT__0));
}


void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
    zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value);

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

  ZB_ZGP_REGISTER_COMM_REQ_CB(comm_req_cb);
  ZB_ZGP_REGISTER_COMM_COMPLETED_CB(commissioning_cb);
  /****************** Register Device ********************************/
  ZB_AF_REGISTER_DEVICE_CTX(&sample_ctx);

  ZB_ZCL_SET_REPORT_ATTR_CB(&report_attribute_cb);

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

void report_attribute_cb(zb_zcl_addr_t *addr, zb_uint8_t ep, zb_uint16_t cluster_id,
    zb_uint16_t attr_id, zb_uint8_t attr_type, zb_uint8_t *value)
{
  ZVUNUSED(value);
  ZVUNUSED(addr);
  ZVUNUSED(attr_type);
  TRACE_MSG(TRACE_APP1, ">> report_attribute_cb ep %hd, cluster %d, attr %d",
            (FMT__H_D_D, ep, cluster_id, attr_id));

  report_count++;

  if (cluster_id != ZB_ZCL_CLUSTER_ID_IAS_ZONE ||
      (attr_id != ZB_ZCL_ATTR_IAS_ZONE_ZONETYPE_ID &&
       attr_id != ZB_ZCL_ATTR_IAS_ZONE_ZONESTATUS_ID)
     )
  {
    g_error_cnt++;
    TRACE_MSG(TRACE_ERROR, "Error, incorrect report: cluster_id 0x%x, attr_id 0x%x",
        (FMT__D_D, cluster_id, attr_id));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "val = %d", (FMT__D, *(zb_uint16_t*)value));
  }

  if (report_count==gs_expected_report_attr_call_count && g_error_cnt==0)
  {
    zb_zgps_start_commissioning(DECOMMISSIONING_TIMEOUT);
  }

  TRACE_MSG(TRACE_APP1, "<< report_attribute_cb", (FMT__0));
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APP1, "> zb_zdo_startup_complete %h", (FMT__H, param));

  if (buf->u.hdr.status == 0)
  {
    TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
    zb_zgps_start_commissioning(COMMISSIONING_TIMEOUT);
  }
  else
  {
    g_error_cnt++;
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
