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
/* PURPOSE:
*/

#define ZB_TRACE_FILE_ID 40117
/* TODO: Debug it! */

/*! Sample function to generate and send write attribute command
  @param data_buf - pointer to allocated buffer zb_buf_t
  @param dst_addr - short destinition address
  @param dst_ep - destinition endpoint number
  @param src_ep - source endpoint number
  @note This sample generates command for On/off switch configuration cluster
*/
void send_write_attr_req_sample(zb_buf_t *data_buf, zb_uint16_t dst_addr, zb_uint8_t dst_ep, zb_uint8_t src_ep)
{
  zb_uint8_t *cmd_payload;

  cmd_payload = ZB_ZCL_START_PACKET(data_buf);
  // Form command payload
  // Add write attribute record 1
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, ZB_ZCL_ATTR_OOSC_SWITCH_TYPE_ID);  // attr id
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_TYPE_8BIT_ENUM);            // attr type
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_OOSC_SWITCH_TYPE_MOMENTARY);// data
  // Add write attribute record 2
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, ZB_ZCL_ATTR_OOSC_SWITCH_ACTIONS_ID);// attr id
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_TYPE_8BIT_ENUM);             // attr type
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_OOSC_SWITCH_ACTIONS_TOGGLE); // data

  ZB_ZCL_FINISH_PACKET(data_buf, cmd_payload);

  ZB_ZCL_SEND_WRITE_ATTRS_CMD(data_buf, dst_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              dst_ep, src_ep, ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_ON_OFF_SWITCH_CONFIG);
}
