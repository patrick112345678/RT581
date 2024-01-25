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
/* PURPOSE: MAC optional commands: orphan scan and coordinator realignment
*/

#define ZB_TRACE_FILE_ID 288
#include "zb_common.h"

#if !defined ZB_ALIEN_MAC && !defined ZB_MACSPLIT_HOST

/*! \addtogroup ZB_MAC */
/*! @{ */


#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zb_mac.h"
#include "mac_internal.h"
#include "zb_mac_transport.h"
#include "zb_mac_globals.h"


#if defined ZB_ROUTER_ROLE

#ifndef ZB_LITE_NO_ORPHAN_SCAN
/*
  Sends coordinator realignment command
  @param is_broadcast - if TRUE, cmd is broadcast over PAN, if FALSE
  cmd is directed to orphaned device
  @param orphaned_dev_ext_addr - orphaned device extended addres
  @param orphaned_dev_short_addr - orphaned device short address,
  maybe set to 0xFFFE

  @return RET_OK if ok, error code on error
*/
zb_ret_t zb_tx_coord_realignment_cmd(zb_bool_t is_broadcast,
                                     zb_ieee_addr_t orphaned_dev_ext_addr,
                                     zb_uint16_t orphaned_dev_short_addr,
                                     zb_uint16_t pan_id,
                                     zb_uint8_t logical_channel,
                                     zb_uint8_t channel_page)
{
    zb_ret_t ret = RET_OK;
    zb_uint_t packet_length;
    zb_uint_t mhr_len;
    void *ptr;
    zb_mac_mhr_t mhr = {0};
    zb_coord_realignment_cmd_t coord_realign_cmd;

    /*
      mac spec, 7.3.8 Coordinator realignment command
      1) MHR, Frame Control:
      - if cmd is directed to orphaned device, set dst addr mode = ZB_ADDR_64BIT_DEV;
      if cmd is broadcast to the PAN set dst addr mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST
      - src addr mode = ZB_ADDR_64BIT_DEV
      - Frame Pending = 0
      - if cmd is directed to orphaned device, set Acknowledgment Request = 1,
      set Acknowledgment Request = 0 for PAN broadcast
      - if channel page is present, set Frame Version = 0x01, if channel
      page is absent, set 0x00 if no security and 0x01 if security is on
      - set dst PAN id = 0xFFFF
      - if cmd is directed to orphaned device, dst addr = ext addr of orphaned device
      if cmd is broadcast, set dst addr = 0xFFFF
      - src PAN id = pib.mac_pan_id
      - src addr = aExtendedAddress
      2) set Command Frame Identifier = MAC_CMD_COORDINATOR_REALIGNMENT
      3) set PAN id = req.pan_id
      4) set Coordinator Short Address = pib.mac_short_address
      5) set Logical Channel = req.logical_channel
      6) if cmd is broadcast, set Short Addr = 0xFFFF
      if cmd is directed to orphaned device, set short addr = orphaned device
      short addr, or 0xFFFE if device has no short addr
      7) if req.channel_page != existing channel page, set Channel Page = req.channel_page

    */

    TRACE_MSG(TRACE_MAC1, "+zb_tx_coord_realignment_cmd", (FMT__0));

    mhr_len = zb_mac_calculate_mhr_length((zb_uint8_t)ZB_ADDR_64BIT_DEV,
                                          (zb_uint8_t)(is_broadcast == ZB_TRUE ? ZB_ADDR_16BIT_DEV_OR_BROADCAST : ZB_ADDR_64BIT_DEV),
                                          ZB_FALSE);
    packet_length = mhr_len;
    packet_length += sizeof(zb_coord_realignment_cmd_t);
    /* TODO: reduce packet_length if Channel Page is omitted */

    ptr = zb_buf_initial_alloc(MAC_CTX().operation_buf, packet_length);
    ZB_ASSERT(ptr);

    ZB_BZERO(ptr, packet_length);

    /* Fill Frame Controll then call zb_mac_fill_mhr() */
    /*
      mac spec  7.2.1.1 Frame Control field
      | Frame Type | Security En | Frame Pending | Ack.Request | PAN ID Compres | Reserv | Dest.Addr.Mode | Frame Ver | Src.Addr.gMode |
    */
    ZB_BZERO2(mhr.frame_control);
    ZB_FCF_SET_FRAME_TYPE(mhr.frame_control, MAC_FRAME_COMMAND);
    /* frame pending is 0 */
    if (!is_broadcast)
    {
        /* cmd is directed to orphaned device */
        ZB_FCF_SET_ACK_REQUEST_BIT(mhr.frame_control, 1U);
        ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);
    }
    else
    {
        /* cmd is broadcast to PAN */
        /* ack request bit is 0 */
        ZB_FCF_SET_DST_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_16BIT_DEV_OR_BROADCAST);
    }
    /* PAN id compression is 0 */

    /* if Channel Page is omitted, than frame_version field should be default */
    /* MAC_FRAME_VERSION defined in zb_config.h */
    ZB_FCF_SET_FRAME_VERSION(mhr.frame_control, MAC_FRAME_VERSION);



    ZB_FCF_SET_SRC_ADDRESSING_MODE(mhr.frame_control, ZB_ADDR_64BIT_DEV);

    /* mac spec 7.5.6.1 Transmission */
    mhr.seq_number = ZB_MAC_DSN();
    ZB_INC_MAC_DSN();
    mhr.dst_pan_id = ZB_BROADCAST_PAN_ID; /* broadcast value for both cases orphaned/broadcast */
    if (!is_broadcast && !ZB_IS_64BIT_ADDR_ZERO(orphaned_dev_ext_addr))
    {
        /* cmd is directed to orphaned device */
        ZB_IEEE_ADDR_COPY(mhr.dst_addr.addr_long, orphaned_dev_ext_addr);
    }
    else
    {
        /* cmd is broadcast to PAN */
        mhr.dst_addr.addr_short = ZB_MAC_SHORT_ADDR_NO_VALUE;
    }
    mhr.src_pan_id = MAC_PIB().mac_pan_id;
    /* aExtendedAddress The 64-bit (IEEE) address assigned to the device */
    ZB_MEMCPY(&mhr.src_addr, MAC_PIB().mac_extended_address, sizeof(zb_ieee_addr_t));

    /* TODO: fill Aux Security Header */

    zb_mac_fill_mhr(ptr, &mhr);

    /*
      mac spec 7.3.8 Coordinator realignment command
      | MHR    | Command  Frame Id | PAN Id | Coord Short Addr | Logical Channel | Short Address | Channel page |
      |var size| 1 byte            | 2 b    | 2 b              | 1 b             | 2 b           | 0/1 b        |
    */
    coord_realign_cmd.cmd_frame_id = MAC_CMD_COORDINATOR_REALIGNMENT;
    ZB_HTOLE16((zb_uint8_t *)&coord_realign_cmd.pan_id, &pan_id);
    ZB_HTOLE16((zb_uint8_t *)&coord_realign_cmd.coord_short_addr, &MAC_PIB().mac_short_address);
    coord_realign_cmd.logical_channel = logical_channel;
    if (!is_broadcast)
    {
        /* cmd is directed to orphaned device */
        ZB_HTOLE16((zb_uint8_t *)&coord_realign_cmd.short_addr, &orphaned_dev_short_addr);
    }
    else
    {
        /* cmd is broadcast to PAN */
        /* do not use HTOLE16 macro because 0xffff value is symmetric */
        coord_realign_cmd.short_addr = ZB_MAC_SHORT_ADDR_NO_VALUE;
    }
#if (MAC_FRAME_VERSION > MAC_FRAME_IEEE_802_15_4_2003)
    coord_realign_cmd.channel_page = channel_page; /* TODO: it maybe omitted! */
#else
    ZVUNUSED(channel_page);
#endif

    /* TODO: copy size maybe reduced if channel_page is not used */
    ZB_MEMCPY((zb_uint8_t *)ptr + mhr_len, &coord_realign_cmd, sizeof(zb_coord_realignment_cmd_t));

    /* TODO: when implement realignment, call via tx q */
    ZB_ASSERT(mhr_len <= ZB_UINT8_MAX);
    zb_mac_send_frame(MAC_CTX().operation_buf, (zb_uint8_t)mhr_len);
    TRACE_MSG(TRACE_MAC1, "<< zb_tx_coord_realignment_cmd, ret %i", (FMT__D, ret));
    return ret;
}


#ifdef ZB_OPTIONAL_MAC_FEATURES

/*
  Performs PAN realigning. mac spec 7.5.2.3.2 Realigning a PAN
  @return RET_OK, RET_BLOCKED, error code on error
*/
zb_ret_t zb_realign_pan(zb_uint8_t param)
{
    zb_ret_t ret = RET_OK;

    ZVUNUSED(param);
    /*
      MAC 7.5.2.3.2 Realigning a PAN
      1) if beacon mode is on, set Frame Control.Frame Pending = 1, do NOT change
      other parameters, send scheduled beacon

      2) send realignment command
      3) if beacon mode is off, send realignment command immediately
      4) if realignment command send fails, set status CHANNEL_ACCESS_FAILURE
      5) update PIB parameters according to 7.5.2.3.4 zb_mac_update_superframe_and_pib()
    */
    TRACE_MSG(TRACE_MAC1, "+zb_realign_pan", (FMT__0));
    /* send realignment command*/
    ret = zb_tx_coord_realignment_cmd(ZB_TRUE, NULL, 0,
                                      ZB_PIB_SHORT_PAN_ID(), MAC_PIB().phy_current_channel, 0);

    TRACE_MSG(TRACE_MAC1, "<< zb_realign_pan, ret %i", (FMT__D, ret));
    return ret;
}
#endif  /* ZB_COORDINATOR_ROLE || ZB_ROUTER_ROLE */

void zb_handle_coord_realignment_cmd(zb_uint8_t param)
{
    zb_uint8_t *cmd_ptr;
    zb_mac_mhr_t mhr;

    cmd_ptr = zb_buf_begin(param);
    cmd_ptr += zb_parse_mhr(&mhr, param);

    TRACE_MSG(TRACE_MAC3, "upd network", (FMT__0));
    /* TODO: check realign command length */

    TRACE_MSG(TRACE_MAC3, "cmd_id %hd", (FMT__H, (zb_uint8_t)*cmd_ptr));
    /* skip command id */
    cmd_ptr++;

    /* update panid */
    zb_get_next_letoh16(&MAC_PIB().mac_pan_id, &cmd_ptr);
    TRACE_MSG(TRACE_MAC1, "set pan id %d", (FMT__D, MAC_PIB().mac_pan_id));
    ZB_TRANSCEIVER_UPDATE_PAN_ID();

    /* update coordinator short address */
    zb_get_next_letoh16(&MAC_PIB().mac_coord_short_address, &cmd_ptr);
    TRACE_MSG(TRACE_MAC3, "coord addr set: 0x%x", (FMT__D, MAC_PIB().mac_coord_short_address));
    ZB_TRANSCEIVER_SET_COORD_SHORT_ADDR(MAC_PIB().mac_coord_short_address);

    /* logical channel */
    // Try comment it out: NWK must set it.  ZB_PIBCACHE_CURRENT_CHANNEL() = *cmd_ptr;
    MAC_PIB().phy_current_channel = *cmd_ptr;
    TRACE_MSG(TRACE_MAC3, "set channel: %hd", (FMT__H, MAC_PIB().phy_current_channel));
    ZB_TRANSCEIVER_SET_CHANNEL(MAC_PIB().phy_current_page, MAC_PIB().phy_current_channel);
#if defined ZB_MAC_DUTY_CYCLE_MONITORING
    zb_mac_duty_cycle_update_regulated(MAC_PIB().phy_current_page);
#endif /* ZB_MAC_DUTY_CYCLE_MONITORING */

    cmd_ptr++;

    /* short address */
    zb_get_next_letoh16(&MAC_PIB().mac_short_address, &cmd_ptr);
    TRACE_MSG(TRACE_MAC3, "saddr set: 0x%x", (FMT__D, MAC_PIB().mac_short_address));
    ZB_TRANSCEIVER_UPDATE_SHORT_ADDR();

    zb_buf_free(param);
}

#endif  /* ZB_OPTIONAL_MAC_FEATURES */
#endif  /* #ifndef ZB_LITE_NO_ORPHAN_SCAN */

/*! @} */

#endif /* !ZB_ALIEN_MAC && !ZB_MACSPLIT_HOST */

