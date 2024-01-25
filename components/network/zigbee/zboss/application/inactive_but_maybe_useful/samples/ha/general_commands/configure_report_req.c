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

#define ZB_TRACE_FILE_ID 40119
/* TODO: Debug it! */

#define VERSION_DELTA_VALUE 5

/*! Sample function to generate and configure reporting
  command. Server sends this command to configure client side
  @param data_buf - pointer to allocated buffer zb_buf_t
  @param dst_addr - short destination address
  @param dst_ep - destination endpoint number
  @param src_ep - source endpoint number
  @param min_interval - minimum reporting interval
  @param max_interval - maximum reporting interval
*/
void send_configure_reporting_clnt_req_sample(zb_buf_t *data_buf,
                                              zb_uint16_t dst_addr, zb_uint8_t dst_ep,
                                              zb_uint8_t src_ep,
                                              zb_uint16_t min_interval, zb_uint16_t max_interval)
{
  /** [ZB_ZCL_PACKET] */
  zb_uint8_t *cmd_payload;

  cmd_payload = ZB_ZCL_START_PACKET(data_buf);
  // Form command payload

  /* Add Attribute reporting configuration record (correct one) */
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_CONFIGURE_REPORTING_SEND_REPORT);  //Direction value
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, ZB_ZCL_ATTR_OO_ON_OFF_ID);  // attr id
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_TYPE_BOOL);          // attr type
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, min_interval);               // minimum interval
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, max_interval);               // maximum interval
  /* Reportable change is omitted for discrete data type */

  /* Add Attribute reporting configuration record (with error - invalid attribute id) */
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, ZB_ZCL_ATTR_B_APPLICATION_VERSION_ID);  // attr id
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_TYPE_U8);                        // attr type
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, min_interval);               // minimum interval
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, max_interval);               // maximum interval
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, VERSION_DELTA_VALUE);            // Reportable change

  ZB_ZCL_FINISH_PACKET(data_buf, cmd_payload);


  ZB_ZCL_SEND_WRITE_ATTRS_CMD(data_buf, dst_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              dst_ep, src_ep, ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_ON_OFF);
  /** [ZB_ZCL_PACKET] */
}

/*! Sample function to generate and configure reporting command.
  Client sends this command to inform server side about reporting
  @param data_buf - pointer to allocated buffer zb_buf_t
  @param dst_addr - short destinition address
  @param dst_ep - destinition endpoint number
  @param src_ep - source endpoint number
  @param timeout - timeout period
*/
void send_configure_reporting_srv_req_sample(zb_buf_t *data_buf,
                                              zb_uint16_t dst_addr, zb_uint8_t dst_ep,
                                              zb_uint8_t src_ep,
                                              zb_uint16_t timeout)
{
  zb_uint8_t *cmd_payload;

  cmd_payload = ZB_ZCL_START_PACKET(data_buf);
  // Form command payload

  /* Add Attribute reporting configuration record (correct one) */
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_CONFIGURE_REPORTING_RECV_REPORT);  //Direction value
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, ZB_ZCL_ATTR_OO_ON_OFF_ID);  // attr id
  ZB_ZCL_PACKET_PUT_DATA8(cmd_payload, ZB_ZCL_ATTR_TYPE_BOOL);          // attr type
  ZB_ZCL_PACKET_PUT_DATA16_VAL(cmd_payload, timeout);                   // timeout

  ZB_ZCL_FINISH_PACKET(data_buf, cmd_payload);


  ZB_ZCL_SEND_WRITE_ATTRS_CMD(data_buf, dst_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                              dst_ep, src_ep, ZB_AF_HA_PROFILE_ID, ZB_ZCL_CLUSTER_ID_ON_OFF);
}
