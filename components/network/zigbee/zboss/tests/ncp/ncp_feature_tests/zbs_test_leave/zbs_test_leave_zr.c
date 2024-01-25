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
/* PURPOSE: ZR joins to ZC, then leaves network.
*/

#define ZB_TRACE_FILE_ID 41685
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

/* Note: must undef SUBGIG in ZC! */
static zb_uint8_t g_ic1[16 + 2] = TEST_IC;
static zb_ieee_addr_t g_ieee_addr = TEST_ZR_ADDR;

static void send_data(zb_uint8_t param);
zb_uint8_t data_indication(zb_uint8_t asdu);

void zb_leave_req(zb_uint8_t param);
void zb_leave_callback(zb_uint8_t param);

static void remove_self(zb_uint8_t param);

MAIN()
{
    ARGV_UNUSED;

#ifndef KEIL
#endif
    ZB_SET_TRAF_DUMP_ON();
    ZB_SET_TRACE_ON();
    //ZB_SET_TRACE_MASK(-1);
    ZB_SET_TRACE_MASK(0xDFFF);
    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("zbs_test_leave_zr");

    zb_set_long_address(g_ieee_addr);

    zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);

    zb_set_nvram_erase_at_start(ZB_TRUE);
    ZB_SET_TRAF_DUMP_ON();

    ZB_TCPOL().require_installcodes = ZB_TRUE;
    zb_secur_ic_set(ZB_IC_TYPE_128, g_ic1);

    zb_set_network_coordinator_role(1L << CHANNEL);

    if (zdo_dev_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zdo_main_loop();
    }

    //! [trace_64]
    TRACE_MSG(TRACE_ERROR, "aps ext pan id " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ZB_AIB().aps_use_extended_pan_id)));
    //! [trace_64]

    TRACE_DEINIT();

    MAIN_RETURN(0);
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
            zb_af_set_data_indication(data_indication);
            send_data(param);
            ZB_SCHEDULE_ALARM(zb_leave_req, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(4000));
#ifdef STRESS_TEST
            {
                /* This is quite heavy test causing many collisions. */
                zb_buf_t *b = ZB_GET_OUT_BUF();
                if (b)
                {
                    send_data(b);
                }
            }
#endif
            param = 0;
            break;
        default:
            break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device start FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}


static void send_data(zb_bufid_t buf)
{
    zb_apsde_data_req_t *req;
    zb_uint8_t *ptr = NULL;
    zb_short_t i;

    ptr = zb_buf_initial_alloc(buf, ZB_TEST_DATA_SIZE);
    req = zb_buf_get_tail(buf, sizeof(zb_apsde_data_req_t));
    req->dst_addr.addr_short = 0; /* send to ZC */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;

    zb_buf_set_handle(buf, 0x11);

    for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
    {
        ptr[i] = i % 32 + '0';
    }
    TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, buf);
}


zb_uint8_t data_indication(zb_uint8_t asdu)
{
    zb_ushort_t i;
    zb_uint8_t *ptr;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(asdu, zb_apsde_data_indication_t);

    if (ind->profileid == 0x0002)
    {
        /* Remove APS header from the packet */
        ZB_APS_HDR_CUT_P(asdu, ptr);

        TRACE_MSG(TRACE_APS3, "data_indication: packet %p len %d handle 0x%x", (FMT__P_D_D,
                  asdu, (int)zb_buf_len(asdu), zb_buf_get_status(asdu)));

        for (i = 0 ; i < zb_buf_len(asdu) ; ++i)
        {
            TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
            if (ptr[i] != i % 32 + '0')
            {
                TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                          (int)(i % 32 + '0'), (char)i % 32 + '0'));
            }
        }

#ifndef NCP_SDK
        send_data(asdu);
#else
        ZB_SCHEDULE_ALARM(send_data, asdu, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
#endif

        return ZB_TRUE;
    }

    return ZB_FALSE;
}

void zb_leave_callback(zb_bufid_t buf)
{
    zb_uint8_t *ret = (zb_uint8_t *)zb_buf_begin(buf);

    TRACE_MSG(TRACE_ERROR, "LEAVE MGMT CALLBACK %hd", (FMT__H, *ret));

    zb_buf_free(buf);
}

void zb_leave_req(zb_uint8_t param)
{
    ZVUNUSED(param);
    zb_buf_get_out_delayed(remove_self);
}

static void remove_self(zb_bufid_t buf)
{
    zb_nlme_leave_request_t *req = zb_buf_get_tail(buf, sizeof(zb_nlme_leave_request_t));

    TRACE_MSG(TRACE_ZDO1, ">>remove_child: buf = %d", (FMT__D, buf));

    ZB_IEEE_ADDR_COPY(req->device_address, g_ieee_addr);
    req->remove_children = 0;
    req->rejoin = 0;
    zb_nlme_leave_request(buf);

    TRACE_MSG(TRACE_ZDO1, "<<remove_child", (FMT__0));
}


/*! @} */
