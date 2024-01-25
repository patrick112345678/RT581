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
/* PURPOSE: TH tool
*/

#define ZB_TEST_NAME GPP_DEVICE_ID_R_TH_TOOL
#define ZB_TRACE_FILE_ID 41306
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_zcl.h"
#include "zgp/zgp_internal.h"

#include "zb_ha.h"

#include "test_config.h"

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param);


#if defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER

//zb_uint16_t g_zc_addr = 0;
static zb_ieee_addr_t g_th_tool_addr = TH_TOOL_IEEE_ADDR;

MAIN()
{
    ARGV_UNUSED;
    /* Init device, load IB values from nvram or set it to default */
    ZB_INIT("th_tool");

    ZB_AIB().aps_channel_mask = (1 << TEST_CHANNEL);
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_tool_addr);
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

    zb_set_default_ed_descriptor_values();

    zgp_cluster_set_app_zcl_cmd_handler(zcl_specific_cluster_cmd_handler);

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

static zb_uint8_t zcl_specific_cluster_cmd_handler(zb_uint8_t param)
{
    /** [VARIABLE] */
    zb_buf_t *zcl_cmd_buf = (zb_buf_t *)ZB_BUF_FROM_REF(param);
    zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(zcl_cmd_buf, zb_zcl_parsed_hdr_t);
    zb_bool_t cmd_processed = ZB_FALSE;


    TRACE_MSG(TRACE_ZCL1, "> zcl_specific_cluster_cmd_handler %hd", (FMT__H, param));

    ZB_ZCL_DEBUG_DUMP_HEADER(cmd_info);
    TRACE_MSG(TRACE_ZCL1, "payload size: %hd", (FMT__H, ZB_BUF_LEN(zcl_cmd_buf)));

    TRACE_MSG(TRACE_ZCL1,
              "< zcl_specific_cluster_cmd_handler cmd_processed %hd", (FMT__H, cmd_processed));
    return cmd_processed;
}

static void simple_desc_req(zb_uint8_t buf_ref, zb_uint16_t short_addr, zb_uint8_t ep)
{
    zb_buf_t                 *buf = ZB_BUF_FROM_REF(buf_ref);
    zb_zdo_simple_desc_req_t *req;

    TRACE_MSG(TRACE_APP3, "> simple_desc_req, buf_ref %hd, addr 0x%x, ep %hd",
              (FMT__H_D_H, buf_ref, short_addr, ep));

    ZB_ASSERT(short_addr != ZB_UNKNOWN_SHORT_ADDR);
    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_simple_desc_req_t), req);
    ZB_BZERO(req, sizeof(zb_zdo_simple_desc_req_t));
    req->nwk_addr = short_addr;
    req->endpoint = ep;

    zb_zdo_simple_desc_req(buf_ref, NULL);

    TRACE_MSG(TRACE_APP3, "< simple_desc_req", (FMT__0));
}

ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZCL1, "> zb_zdo_startup_complete %hd", (FMT__H, param));

    if (buf->u.hdr.status == 0)
    {
        TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));

        simple_desc_req(param, DUT_GPPB_ADDR, ZGP_ENDPOINT);
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Error, Device start FAILED status %d",
                  (FMT__D, (int)buf->u.hdr.status));
        zb_free_buf(buf);
    }

    TRACE_MSG(TRACE_ZCL1, "< zb_zdo_startup_complete", (FMT__0));
}

#else // defined ZB_ENABLE_HA && defined ZB_ENABLE_ZGP_CLUSTER

#include <stdio.h>
int main()
{
    printf(" HA and ZGP cluster is not supported\n");
    return 0;
}

#endif // defined ZB_ENABLE_HA
