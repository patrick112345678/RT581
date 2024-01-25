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
/* PURPOSE: ZR joins to ZC, then sends APS packet.
*/

#define ZB_TRACE_FILE_ID 40039
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_startup.h"

/*! \addtogroup ZB_TESTS */
/*! @{ */

#define TIMEOUT_MUL 1

static void send_data(zb_uint8_t param);
zb_uint8_t data_indication(zb_uint8_t param);
zb_ieee_addr_t g_ieee_addr = TEST_ZR_ADDR;

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

    ZB_INIT("zdo_zr");

    zb_set_long_address(g_ieee_addr);

    /* Subgig is impossible with ZR! */
    /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
    zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);

    zb_set_nvram_erase_at_start(ZB_TRUE);
    zb_set_network_router_role(1L << CHANNEL);

    ZB_SET_TRAF_DUMP_ON();

    //ZB_BDB().bdb_join_uses_install_code_key = 1;

    /* Uncomment to test linkage with distributed security feature on */
    //zb_enable_distributed();

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
#ifdef STRESS_TEST
            {
                /* This is quite heavy test causing many collisions. */
                zb_bufid_t b = ZB_GET_OUT_BUF();
                if (b)
                {
                    send_data(ZB_REF_FROM_BUF(b));
                }
            }
#endif
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
        zb_buf_free(param);
    }
}


static void send_data(zb_uint8_t param)
{
    zb_apsde_data_req_t *req;
    zb_uint8_t *ptr = NULL;
    zb_short_t i;

    ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);
    req = ZB_BUF_GET_PARAM(param, zb_apsde_data_req_t);
    req->dst_addr.addr_short = 0; /* send to ZC */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;


    for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
    {
        ptr[i] = i % 32 + '0';
    }
    TRACE_MSG(TRACE_APS3, "Sending apsde_data.request", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
}


zb_uint8_t data_indication(zb_uint8_t param)
{
    zb_ushort_t i;
    zb_uint8_t *ptr;
    zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

    if (ind->profileid == 0x0002)
    {
        ptr = (zb_uint8_t *)zb_buf_begin(param);

        TRACE_MSG(TRACE_APS3, "data_indication: packet %hd len %d handle 0x%x",
                  (FMT__H_D_D,
                   param, (int)zb_buf_len(param), zb_buf_get_status(param)));

        for (i = 0 ; i < zb_buf_len(param) ; ++i)
        {
            TRACE_MSG(TRACE_APS3, "%x %c", (FMT__D_C, (int)ptr[i], ptr[i]));
            if (ptr[i] != i % 32 + '0')
            {
                TRACE_MSG(TRACE_ERROR, "Bad data %hx %c wants %x %c", (FMT__H_C_D_C, ptr[i], ptr[i],
                          (int)(i % 32 + '0'), (char)i % 32 + '0'));
            }
        }

        /* send_data(asdu); */
        ZB_SCHEDULE_ALARM(send_data, param, ZB_RANDOM_VALUE(ZB_TIME_ONE_SECOND * 5 * TIMEOUT_MUL));

        return ZB_TRUE;               /* processed */
    }

    return ZB_FALSE;
}


/*! @} */
