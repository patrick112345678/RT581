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
/* PURPOSE: test for mac - end device side
*/

#define ZB_TRACE_FILE_ID 41710
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_ringbuffer.h"
#include "zb_bufpool.h"
#include "zb_mac_transport.h"
#include "zb_nwk.h"
#include "zb_secur.h"

#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MCPS_DATA_CONFIRM
#include "zb_mac_only_stubs.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


#define MY_ADDR   0x0002
#define LOG_FILE "tst_ze"
//#define TEST_CHANEL_MASK (1l << 14)
#define TEST_CHANEL_MASK ZB_TRANSCEIVER_ALL_CHANNELS_MASK /* (0xffff << 11) */

void scanning_03(zb_uint8_t param1)
{
  zb_bufid_t buf = zb_get_out_buf();
  zb_nlme_network_discovery_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_network_discovery_request_t);
  zb_ret_t ret;

  ZVUNUSED(param1);

  MAC_PIB().mac_auto_request = 1;
  ZB_AIB().aps_channel_mask = TEST_CHANEL_MASK;
  req->scan_channels = ZB_AIB().aps_channel_mask;
  req->scan_duration = ZB_DEFAULT_SCAN_DURATION; /* TODO: configure it somehow? */
  TRACE_MSG(TRACE_APS1, "disc, then join by assoc", (FMT__0));

  COMM_CTX().discovery_ctx.disc_count = COMM_CTX().discovery_ctx.nwk_scan_attempts;
  ret = zb_schedule_callback(zb_nlme_network_discovery_request, param);
}


/*
  Link without APS.
  Define here APS routines.
*/

void zb_nlde_data_confirm(zb_uint8_t param)
{
  zb_bufid_t nsdu = (zb_bufid_t )param;

  TRACE_MSG(TRACE_NWK3, "nlde_data_confirm: packet %p status %d", (FMT__P_D,
            nsdu, nsdu->u.hdr.status));

  /* FIXME: Seems this buffer removed twice */
  zb_free_buf(nsdu);
}


void zb_nlde_data_indication(zb_uint8_t param)
{
  zb_bufid_t nsdu = (zb_bufid_t )param;

  TRACE_MSG(TRACE_NWK3, "nlde_data_indication: packet %p handle 0x%x", (FMT__P_D,
            nsdu, nsdu->u.hdr.status));
  zb_free_buf(nsdu);
}


void usage(char **argv)
{

  printf("%s <read pipe path> <write pipe path>\n", argv[0]);
#else
  ZVUNUSED(argv);
#endif
}


MAIN()
{
  ARGV_UNUSED;

  ZB_INIT(LOG_FILE);

  /* In the real life network address will be assigned at network join; now
   * hardcode it. */
  ZB_PIBCACHE_NETWORK_ADDRESS() = MY_ADDR;
  ZB_SET_JOINED_STATUS(ZB_TRUE);

  /* call nwk API from the scheduler loop */
  ZB_SCHEDULE_CALLBACK(scanning_03, 0);

  while(1)
  {
    zb_sched_loop_iteration();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
}


void zb_mlme_start_confirm(zb_uint8_t param)
{
  TRACE_MSG(TRACE_NWK2, "zb_mlme_start_confirm", (FMT__0));

  {
    zb_uint8_t *pl;

    req = zb_buf_initial_alloc(param, 5, pl);
    pl[0] = 0x00;
    pl[1] = 0x01;
    pl[2] = 0x02;
    pl[3] = 0x03;
    pl[4] = 0x04;
  }
  {
    zb_mcps_data_req_params_t *data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);

    data_req->dst_addr = 0x3344;
    data_req->src_addr = 0x1122;
    data_req->msdu_handle = 0;
    data_req->tx_options = 0;
  }

  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
}

void zb_mcps_data_confirm(zb_uint8_t param)
{
  zb_mcps_data_confirm_params_t *confirm_params = ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
  TRACE_MSG(TRACE_NWK2, "zb_mcps_data_confirm param %hd handle 0x%hx status 0x%hx",
            (FMT__H_H_H, (zb_uint8_t)param, (zb_uint8_t)confirm_params->msdu_handle, zb_buf_get_status(param)));
  zb_buf_free(param);
}


void zb_nlme_network_discovery_request(zb_uint8_t param)
{
  zb_nlme_network_discovery_request_t *request =
      ZB_BUF_GET_PARAM((zb_bufid_t )param, zb_nlme_network_discovery_request_t);
  zb_nlme_network_discovery_request_t rq;

  TRACE_MSG(TRACE_NWK1, ">>zb_nlme_network_discovery_request %p", (FMT__P, request));

  ZB_MEMCPY(&rq, request, sizeof(rq));
  TRACE_MSG(TRACE_NWK1, "scan channels 0x%x scan_duration %hd", (FMT__D_H, rq.scan_channels, rq.scan_duration));
  ZB_MLME_BUILD_SCAN_REQUEST((zb_bufid_t )param, rq.scan_channels, ACTIVE_SCAN, rq.scan_duration);
  ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);

  TRACE_MSG(TRACE_NWK1, "<<zb_nlme_network_discovery_request", (FMT__0));
}

void zb_mlme_beacon_notify_indication(zb_uint8_t param)
{
  zb_uint8_t *mac_hdr = ZB_MAC_GET_FCF_PTR(zb_buf_begin((zb_bufid_t )param));
  zb_ushort_t payload_off = zb_mac_get_beacon_payload_offset(mac_hdr);
  zb_mac_beacon_payload_t *beacon_payload;

  TRACE_MSG(TRACE_NWK1, ">>zb_mlme_beacon_notify_indication %hd", (FMT__H, param));

  beacon_payload = (zb_mac_beacon_payload_t *)(mac_hdr + payload_off);

#ifdef ZB_MAC_TESTING_MODE
  {
    zb_uint8_t payload_len;

    payload_len = zb_buf_len(param) - payload_off - ZB_BEACON_PAYLOAD_TAIL;

    TRACE_MSG(TRACE_NWK1, "beacon payload dump", (FMT__0));
    dump_traf((zb_uint8_t*)beacon_payload, payload_len);
  }
#endif  /* ZB_MAC_TESTING_MODE */

  TRACE_MSG(TRACE_NWK1, "<<zb_mlme_beacon_notify_indication", (FMT__0));
}

void zb_mlme_scan_confirm(zb_uint8_t param)
{
  zb_mac_scan_confirm_t *scan_confirm;
  zb_uint8_t i;

  TRACE_MSG(TRACE_NWK1, ">>zb_mlme_scan_confirm %hd", (FMT__H, param));

#ifdef ZB_MAC_TESTING_MODE

  scan_confirm = ZB_BUF_GET_PARAM(param, zb_mac_scan_confirm_t);
  TRACE_MSG(TRACE_NWK3, "scan type %hd, status %hd, auto_req %hd",
            (FMT__H_H_H, scan_confirm->scan_type, scan_confirm->status, MAC_PIB().mac_auto_request));
  if (scan_confirm->scan_type == ACTIVE_SCAN && scan_confirm->status == MAC_SUCCESS && MAC_PIB().mac_auto_request)
  {
    zb_pan_descriptor_t *pan_desc;
    pan_desc = (zb_pan_descriptor_t*)zb_buf_begin(param);
    TRACE_MSG(TRACE_NWK3, "ative scan res count %hd", (FMT__H, scan_confirm->result_list_size));
    for(i = 0; i < scan_confirm->result_list_size; i++)
    {
      TRACE_MSG(TRACE_NWK3,
                "pan desc: coord addr mode %hd, coord addr %x, pan id %x, channel %hd, superframe %x, lqi %hx",
                (FMT__H_D_D_H_D_H, pan_desc->coord_addr_mode, pan_desc->coord_address.addr_short, pan_desc->coord_pan_id,
                 pan_desc->logical_channel, pan_desc->super_frame_spec, pan_desc->link_quality));

      if (pan_desc->coord_addr_mode == ZB_ADDR_64BIT_DEV)
      {
        TRACE_MSG(TRACE_MAC3, "Extended coord addr " TRACE_FORMAT_64,
                  (FMT__A, TRACE_ARG_64(pan_desc->coord_address.addr_long)));
      }

      pan_desc++;
    }
  }
#endif

  TRACE_MSG(TRACE_NWK1, "<<zb_mlme_scan_confirm", (FMT__0));
}


/*! @} */
