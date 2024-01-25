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
/* PURPOSE: TH ZR1 (initiator)
*/


#define ZB_TEST_NAME FB_PRE_TC_01B_THR1
#define ZB_TRACE_FILE_ID 41137
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
#include "on_off_client.h"
#include "fb_pre_tc_01b_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif


#ifndef ZB_CERTIFICATION_HACKS
#error ZB_CERTIFICATION_HACKS is not defined!
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
DECLARE_ON_OFF_CLIENT_CLUSTER_LIST(on_off_controller_clusters,
                                   basic_attr_list,
                                   identify_attr_list);

DECLARE_ON_OFF_CLIENT_EP(on_off_controller_ep,
                         THR1_ENDPOINT,
                         on_off_controller_clusters);

DECLARE_ON_OFF_CLIENT_CTX(on_off_controller_ctx, on_off_controller_ep);


extern void zdo_send_resp_by_short(zb_uint16_t command_id,
                                   zb_uint8_t param,
                                   zb_uint16_t addr);


static void disable_frame_retransmission(zb_uint8_t param);
static void enable_frame_retransmission(zb_uint8_t unused);
static zb_uint8_t identify_handler(zb_uint8_t param);
static void zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id);
static void send_ieee_addr_resp(zb_uint8_t param);


static int s_ieee_resp_format;


MAIN()
{
    ARGV_UNUSED;

    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zdo_thr1");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
    ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
    ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
    ZB_BDB().bdb_mode = 1;

    ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;

#ifdef ZB_USE_NVRAM
    ZB_AIB().aps_use_nvram = 1;
#endif

    ZB_NIB().max_children = 1;

    ZB_AF_REGISTER_DEVICE_CTX(&on_off_controller_ctx);

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


static void disable_frame_retransmission(zb_uint8_t param)
{
    s_ieee_resp_format = param;
    TRACE_MSG(TRACE_ZDO1, "Toggle frame retransmission", (FMT__0));

    ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, identify_handler);
}


static void enable_frame_retransmission(zb_uint8_t unused)
{
    ZVUNUSED(unused);
    TRACE_MSG(TRACE_ZDO1, "Enable frame retransmission", (FMT__0));

    ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, NULL);
    ZB_CERT_HACKS().disable_frame_retransmission = 0;
    ZB_CERT_HACKS().force_frame_indication = 0;
    ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 0;
    ZB_CERT_HACKS().zdo_af_handler_cb = NULL;
}


static zb_uint8_t identify_handler(zb_uint8_t param)
{
    ZVUNUSED(param);
    ZB_CERT_HACKS().pass_incoming_zdo_cmd_to_app = 1;
    ZB_CERT_HACKS().zdo_af_handler_cb = zdo_rx_handler;
    ZB_CERT_HACKS().disable_frame_retransmission = 0;
    ZB_CERT_HACKS().force_frame_indication = 1;
    return ZB_FALSE;
}


static void zdo_rx_handler(zb_uint8_t param, zb_uint16_t cluster_id)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);

    TRACE_MSG(TRACE_ZDO1, ">>zdo_rx_handler: buf_param = param", (FMT__D, param));

    ZB_CERT_HACKS().disable_frame_retransmission = 1;

    switch (cluster_id)
    {
    case ZDO_IEEE_ADDR_REQ_CLID:
        send_ieee_addr_resp(param);
        break;
    default:
        enable_frame_retransmission(0);
        zb_free_buf(buf);
    }

    TRACE_MSG(TRACE_ZDO1, "<<zdo_rx_handler", (FMT__0));
}


static void send_ieee_addr_resp(zb_uint8_t param)
{
    zb_buf_t *buf = ZB_BUF_FROM_REF(param);
    zb_zdo_ieee_addr_resp_t *resp;
    zb_apsde_data_indication_t ind;
    zb_uint8_t *packet;
    zb_uint8_t status, tsn;
    zb_uint16_t nwk_addr;

    TRACE_MSG(TRACE_ZDO1, ">>send_ieee_addr_resp: buf_param = %d, resp_type = %d",
              (FMT__D_D, param, s_ieee_resp_format));

    packet = ZB_BUF_BEGIN(buf);
    tsn = *packet;
    ZB_MEMCPY(&ind, ZB_GET_BUF_PARAM(buf, zb_apsde_data_indication_t), sizeof(ind));
    ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_ieee_addr_resp_t), packet);
    resp = (zb_zdo_ieee_addr_resp_t *) packet;

    if (!s_ieee_resp_format)
    {
        status = ZB_ZDP_STATUS_SUCCESS;
        nwk_addr = ZB_PIBCACHE_NETWORK_ADDRESS();
    }
    else
    {
        status = ZB_ZDP_STATUS_DEVICE_NOT_FOUND;
        nwk_addr = zb_address_short_by_ieee(g_ieee_addr_the1);
    }

    resp->status = status;
    ZB_IEEE_ADDR_COPY(resp->ieee_addr_remote_dev, g_ieee_addr_the1);
    ZB_HTOLE16((zb_uint8_t *) &resp->nwk_addr_remote_dev, &nwk_addr);

    zdo_send_resp_by_short(ZDO_IEEE_ADDR_RESP_CLID, param, ind.src_addr);

    TRACE_MSG(TRACE_ZDO1, "<<send_ieee_addr_resp", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            ZB_SCHEDULE_ALARM(disable_frame_retransmission, 0, THE1_FB_DELAY1);
            ZB_SCHEDULE_ALARM(disable_frame_retransmission, 1, THE1_FB_DELAY2);
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
