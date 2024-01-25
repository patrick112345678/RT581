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
/* PURPOSE: 5.3. P/154/MAC/SCANNING-03 FFD1
MAC-only build
*/

#define ZB_TRACE_FILE_ID 41695
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"

#define USE_ZB_MLME_SCAN_CONFIRM
#define USE_ZB_MLME_BEACON_NOTIFY_INDICATION
#include "zb_mac_only_stubs.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#include "zb_bank_common.h"

#define LOG_FILE "tst_ze"
#define TEST_CHANEL_MASK ZB_TRANSCEIVER_ALL_CHANNELS_MASK /* (0xffff << 11) */
#define TEST_SCAN_DURATION 5

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
  MAC_PIB().mac_auto_request = 1;

  {
    zb_bufid_t buf = zb_get_out_buf();

    ZB_MLME_BUILD_SCAN_REQUEST(param, TEST_CHANEL_MASK, ACTIVE_SCAN, TEST_SCAN_DURATION);
    ZB_SCHEDULE_CALLBACK(zb_mlme_scan_request, param);
  }

  while(1)
  {
    zb_sched_loop_iteration();
  }

  TRACE_DEINIT();

  MAIN_RETURN(0);
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
#ifdef ZB_MAC_TESTING_MODE

  zb_mac_scan_confirm_t *scan_confirm;
  zb_uint8_t i;

#endif  /* ZB_MAC_TESTING_MODE */

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
                "pan desc: coord addr mode %hd, coord addr %x, pan id %x, channel %hd, superfame %x, lqi %hx",
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
