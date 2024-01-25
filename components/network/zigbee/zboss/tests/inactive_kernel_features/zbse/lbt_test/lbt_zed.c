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
/* PURPOSE: test for ZED application written using ZDO.
*/

#define ZB_TRACE_FILE_ID 63696
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"

#include "lbt_test.h"

#ifndef ZB_ED_ROLE
#error define ZB_ED_ROLE to compile ze tests
#endif

#ifndef ZB_SUB_GHZ_LBT
#error LBT is not compiled!
#endif  /* ZB_SUB_GHZ_LBT */

/*! \addtogroup ZB_TESTS */
/*! @{ */

zb_ieee_addr_t g_ze_addr = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};

MAIN()
{
    ARGV_UNUSED;

#if !(defined KEIL || defined SDCC || defined ZB_IAR )
#ifdef ZB_NS_BUILD
    if ( argc < 3 )
    {
        printf("%s <read pipe path> <write pipe path>\n", argv[0]);
        return 0;
    }
#endif
#endif
    ZB_SET_TRAF_DUMP_ON();
    /* Init device, load IB values from nvram or set it to default */

    ZB_INIT("lbt_zed");


    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), g_ze_addr);

    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

#ifdef CHANNEL
    {
        zb_channel_list_t channel_list;

        zb_channel_list_init(channel_list);

        zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE0_2_4_GHZ, (1L << 11) | (1L << 15) | (1L << 20) | (1L << 26));

        zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE28_SUB_GHZ, 1L << 1);
        zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE28_SUB_GHZ, 1L << CHANNEL);

        zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE29_SUB_GHZ, 1L << 1);

        zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE30_SUB_GHZ, 1L << 1);

        /* zb_channel_list_add(channel_list, ZB_CHANNEL_PAGE31_SUB_GHZ, 1L<<1); */

        zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);

        /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
        /* zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL); */
    }
#endif

    /* Set to 1 to force entire nvram erase. */
    ZB_AIB().aps_nvram_erase_at_start = 1;
    /* set to 1 to enable nvram usage. */
    ZB_AIB().aps_use_nvram = 1;
    ZB_SET_TRAF_DUMP_ON();

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


static void send_data(zb_buf_t *buf)
{
    zb_apsde_data_req_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_apsde_data_req_t));
    zb_uint8_t *ptr = NULL;
    zb_short_t i;

    req->dst_addr.addr_short = 0; /* send to ZC */
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;
    req->clusterid = 0xf00d;

    buf->u.hdr.handle = 0x11;
    ZB_BUF_INITIAL_ALLOC(buf, ZB_TEST_DATA_SIZE, ptr);

    for (i = 0 ; i < ZB_TEST_DATA_SIZE ; ++i)
    {
        ptr[i] = i % 32 + '0';
    }
    TRACE_MSG(TRACE_APS2, "Sending apsde_data.request", (FMT__0));

    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, ZB_REF_FROM_BUF(buf));
    /* Poll for incoming data */
    TRACE_MSG(TRACE_ZDO3, "Call zb_zdo_pim_start_turbo_poll_packets 1", (FMT__0));
    zb_zdo_pim_start_turbo_poll_packets(1);

    /* DEBUG!!! */
    /* Poison TX stat per each data packet being sent */
    nwk_txstat_tx_inc();
    nwk_txstat_fail_inc();
}

zb_bool_t gs_can_send = 1;

void duty_cycle_status_listener(zb_uint8_t status)
{
    if (status == ZB_MAC_DUTY_CYCLE_STATUS_CRITICAL ||
            status == ZB_MAC_DUTY_CYCLE_STATUS_SUSPENDED)
    {
        gs_can_send = 0;
    }
    else if (status == ZB_MAC_DUTY_CYCLE_STATUS_NORMAL)
    {
        gs_can_send = 1;
    }
}

void send_data_loop(zb_uint8_t param)
{
    if (gs_can_send)
    {
        zb_buf_t *buf;

        buf = ZB_GET_OUT_BUF();
        if (buf == NULL)
        {
            ZB_SCHEDULE_ALARM(send_data_loop, 0, 1 * ZB_TIME_ONE_SECOND);
            return;
        }
        TRACE_MSG(TRACE_ERROR, "Sending apsde_data.request ED", (FMT__0));

        send_data(buf);
    }

    ZB_SCHEDULE_ALARM(send_data_loop, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(200)); //
}


void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

    TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
              (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        //! [signal_skip_startup]
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        {
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

            zb_zdo_register_duty_cycle_mode_indication_cb(duty_cycle_status_listener);

            ZB_SCHEDULE_CALLBACK(send_data_loop, 0);
        }
        break;
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_APP1, "Device START failed", (FMT__0));
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    zb_free_buf(ZB_BUF_FROM_REF(param));
}


/*! @} */
