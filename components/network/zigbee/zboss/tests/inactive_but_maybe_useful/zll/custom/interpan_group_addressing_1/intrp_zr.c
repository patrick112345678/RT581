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
/* PURPOSE: Dimmable light for ZLL profile
*/

#define ZB_TRACE_FILE_ID 41636
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#ifndef ZB_ALIEN_MAC
#include "zb_mac_globals.h"
#endif
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_aps_interpan.h"

#include "intrp_test.h"

#if defined ZB_ENABLE_INTER_PAN_EXCHANGE

#if ! defined ZB_ROUTER_ROLE
#error define ZB_ROUTER_ROLE to compile zr test device
#endif

void send_intrp_data(zb_uint8_t param);

#if defined SECOND_DEVICE
zb_ieee_addr_t g_ieee_addr_my = INTRP_TEST_IEEE_ADDR_2;
zb_uint16_t g_group_id = INTRP_TEST_GROUP_ID_1;
#elif defined THIRD_DEVICE
zb_ieee_addr_t g_ieee_addr_my = INTRP_TEST_IEEE_ADDR_3;
zb_uint16_t g_group_id = INTRP_TEST_GROUP_ID_2;
#else /* defined SECOND_DEVICE */
#error One of SECOND_DEVICE or THIRD_DEVICE must be defined for the test device.
#endif /* defined SECOND_DEVICE */

MAIN()
{
    ARGV_UNUSED;

#ifndef KEIL
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif

    /* Init device, load IB values from nvram or set it to default */

#if defined SECOND_DEVICE
    ZB_INIT("intrp_zr2nd");
#else /* defined SECOND_DEVICE */
    ZB_INIT("intrp_zr3rd");
#endif /* defined SECOND_DEVICE */


    zb_set_default_ed_descriptor_values();

    ZB_AIB().aps_channel_mask = 1l << MY_CHANNEL;
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_my);
    //ZB_IEEE_ADDR_COPY(ZB_NIB_EXT_PAN_ID(), &g_ieee_addr_my);
    ZB_PIBCACHE_PAN_ID() = INTRP_TEST_PAN_ID_2;
#ifndef ZB_ALIEN_MAC
    MAC_PIB().mac_pan_id = INTRP_TEST_PAN_ID_2;
#endif
    ZG->nwk.handle.joined = 1;

    ZB_SET_NIB_SECURITY_LEVEL(0);

    if (zb_zll_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "ERROR zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_apsme_add_group_req_t *aps_req;
    zb_apsme_add_group_conf_t *conf;

    if ((buf->u.hdr.status == 0) || (buf->u.hdr.status == ZB_NWK_STATUS_ALREADY_PRESENT))
    {
        /* Adding device to group - copied from zcl/zcl_groups.c */
        zb_buf_reuse(buf);
        aps_req = ZB_GET_BUF_PARAM(buf, zb_apsme_add_group_req_t);
        ZB_BZERO(aps_req, sizeof(*aps_req));
        ZB_MEMCPY(&(aps_req->group_address), &g_group_id, sizeof(g_group_id));
        aps_req->endpoint = 0;
        zb_apsme_add_group_request(ZB_REF_FROM_BUF(buf));
        conf = ZB_GET_BUF_PARAM(buf, zb_apsme_add_group_conf_t);
        if (conf->status != ZB_APS_STATUS_SUCCESS)
        {
            TRACE_MSG(TRACE_ERROR, "Error adding to group: status 0x%hx", (FMT__H, conf->status));
        }
        else
        {
            TRACE_MSG(TRACE_ZCL1, "Device STARTED OK", (FMT__0));
        }
    }
    else
    {
        TRACE_MSG(
            TRACE_ERROR,
            "ERROR Device started FAILED status %d",
            (FMT__D, (int)buf->u.hdr.status));
    }
    zb_free_buf(buf);
}

void zb_intrp_data_confirm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_APS1, "> zb_intrp_data_confirm param %hd", (FMT__H, param));

    TRACE_MSG(TRACE_APS1, "< zb_intrp_data_confirm", (FMT__0));
}/* void zb_intrp_data_confirm(zb_uint8_t param) */

void zb_intrp_data_indication(zb_uint8_t param)
{
    zb_buf_t *buffer = ZB_BUF_FROM_REF(param);
    zb_intrp_data_ind_t *indication = ZB_GET_BUF_PARAM(buffer, zb_intrp_data_ind_t);
    zb_intrp_data_req_t request;
    zb_uint8_t *data;
    zb_uint16_t tmp;
    zb_uint8_t test_step;

    TRACE_MSG(TRACE_APS1, "> zb_intrp_data_indication param %hd", (FMT__H, param));
    TRACE_MSG(TRACE_ZCL1, "CHANNEL = %d, mask = 0x%x", (FMT__D_D, ZB_PIBCACHE_CURRENT_CHANNEL(), ZB_AIB().aps_channel_mask));

    TRACE_MSG(TRACE_APS3, "Current buffer length is %hd", (FMT__H, ZB_BUF_LEN(buffer)));

    ZB_BUF_CUT_LEFT(
        buffer,
        ZB_INTRP_HEADER_SIZE(indication->dst_addr_mode == ZB_INTRP_ADDR_GROUP),
        data);
    TRACE_MSG(
        TRACE_APS3,
        "Buffer length after cutting Stub-APS header is %hd", (FMT__H, ZB_BUF_LEN(buffer)));

    ZB_MEMCPY(&tmp, &(indication->dst_addr.addr_short), sizeof(tmp));
    /* TODO remove magic number */
    if (indication->dst_addr_mode != ZB_INTRP_ADDR_GROUP || (tmp != g_group_id && tmp != 0xffff))
    {
        TRACE_MSG(
            TRACE_ERROR,
            "Not a group addressing, or not to our group: dst_addr_mode %hd dst_addr 0x%04x",
            (FMT__D_D, indication->dst_addr_mode, tmp));
        zb_free_buf(buffer);
    }
    else if (ZB_BUF_LEN(buffer) != sizeof(zb_uint8_t))
    {
        TRACE_MSG(TRACE_ERROR, "Erroneous payload size %hd (!= 1)", (FMT__H, ZB_BUF_LEN(buffer)));
        zb_free_buf(buffer);
    }
    else
    {
        test_step = *data;
        TRACE_MSG(TRACE_APS3, "Got test step %hd", (FMT__H, test_step));
        ZB_BUF_REUSE(buffer);
        ZB_BZERO(&request, sizeof(request));
        request.dst_addr_mode = ZB_INTRP_ADDR_IEEE;
        ZB_IEEE_ADDR_COPY(request.dst_addr.addr_long, indication->src_addr);
        /* CR:MEDIUM no need to copy 16 bit values */
        ZB_MEMCPY(&(request.dst_pan_id), &(indication->src_pan_id), sizeof(indication->src_pan_id));
        ZB_MEMCPY(&(request.profile_id), &(indication->profile_id), sizeof(indication->profile_id));
        ZB_MEMCPY(&(request.cluster_id), &(indication->cluster_id), sizeof(indication->cluster_id));
        ZB_BUF_INITIAL_ALLOC(buffer, sizeof(zb_uint8_t), data);
        *data = test_step;
        request.asdu_handle = 0;
        ZB_MEMCPY(ZB_GET_BUF_PARAM(buffer, zb_intrp_data_req_t), &request, sizeof(zb_intrp_data_req_t));
        ZB_SCHEDULE_CALLBACK(zb_intrp_data_request, param);
    }

    TRACE_MSG(TRACE_APS1, "< zb_intrp_data_indication", (FMT__0));
}/* void zb_intrp_data_indication(zb_uint8_t param) */

#else /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */

#include <stdio.h>

int main()
{
    printf(" Inter-PAN exchange is not supported\n");
    return 0;
}

#endif /* defined ZB_ENABLE_INTER_PAN_EXCHANGE */
