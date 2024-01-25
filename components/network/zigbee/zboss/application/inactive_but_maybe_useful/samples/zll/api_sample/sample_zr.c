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
/* PURPOSE: ZLL API sample (router role)
*/

#define ZB_TRACE_FILE_ID 40182
#include "zb_common.h"
#include "zll/zb_zll_commissioning.h"

int main(int argc, char** argv)
{
  ZVUNUSED(argc);
  ZVUNUSED(argv);
}/* int main(int argc, char** argv) */

void zb_zdo_startup_complete(zb_uint8_t param)
{
  ZVUNUSED(param); /* Just for compilability. */
}/* void zb_zdo_startup_complete(zb_uint8_t param) */

#if defined ZB_ENABLE_ZLL

void send_scan_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);

  TRACE_MSG(TRACE_APS1, "> send_scan_request_sample param %hd", (FMT__H, param));

  /* 04/02/2018 EE CR:MINOR Why do we use buf here and inside? Looks like param will be more natural. */
  ZB_ZLL_COMMISSIONING_SEND_SCAN_REQ(buf, NULL);

  TRACE_MSG(TRACE_APS1, "< send_scan_request_sample", (FMT__0));
}/* void scan_request_sample(zb_uint8_t param) */

void get_scan_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_scan_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> get_scan_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_SCAN_REQ(&request, buf, status);
  if (status == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_APS1, "Sending response", (FMT__0));
    /* Here we use zeros instead of some values - just to check it compiles. */
    ZB_ZLL_COMMISSIONING_SEND_SCAN_RES(
      buf,
      zcl_header.seq_number,
      NULL,
      status);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_scan_request_sample", (FMT__0));
}/* void scan_request_sample(zb_uint8_t param) */

void get_scan_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_scan_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_scan_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_SCAN_RES(&request, buf, stat);
  if (stat != ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_scan_response_sample", (FMT__0));
}/* void scan_request_sample(zb_uint8_t param) */

void send_device_information_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_device_information_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_DEVICE_INFO_REQ(buf, 0, addr, NULL, status);

  TRACE_MSG(TRACE_APS1, "< send_device_information_request_sample", (FMT__0));
}

void get_device_information_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_device_information_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t* dst_ptr;
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_device_information_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_DEVICE_INFO_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_APS1, "Sending response", (FMT__0));
    /* Here we use zeros instead of some values - just to check it compiles. */
    ZB_ZLL_COMMISSIONING_INIT_DEVICE_INFO_RES(
        buf,
        dst_ptr,
        zcl_header.seq_number,
        request.start_idx,
        1);
    ZB_ZLL_COMMISSIONING_ADD_EP_INFO_DEVICE_INFO_RES(
        dst_ptr,
        0,
        ZB_AF_ZLL_PROFILE_ID,
        0,
        0,
        0,
        0);
    ZB_ZLL_COMMISSIONING_SEND_DEVICE_INFO_RES(buf, dst_ptr, addr, NULL, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_device_information_request_sample", (FMT__0));
}/* void scan_request_sample(zb_uint8_t param) */

void get_device_information_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_device_information_common_res_t request;
  zb_zll_commissioning_device_information_ep_info_res_t dev_info;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_device_information_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_DEVICE_INFO_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_GET_NEXT_EP_INFO_DEVICE_INFO_RES(&dev_info, buf, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_device_information_response_sample", (FMT__0));
}/* void scan_request_sample(zb_uint8_t param) */

void send_identify_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_identify_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_IDENTIFY_REQ(buf, 0, addr, NULL, status);

  TRACE_MSG(TRACE_APS1, "< send_identify_request_sample", (FMT__0));
}/* void send_identify_request_sample(zb_uint8_t param) */

void get_identify_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_identify_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_device_information_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_IDENTIFY_REQ(&request, buf, stat);
  if (stat != ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_device_information_request_sample", (FMT__0));
}/* void get_identify_request_sample(zb_uint8_t param) */

void send_reset_to_factory_new_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_reset_to_factory_new_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_RESET_TO_FN_REQ(buf, addr, NULL, status);

  TRACE_MSG(TRACE_APS1, "< send_reset_to_factory_new_request_sample", (FMT__0));
}/* void send_identify_request_sample(zb_uint8_t param) */

void get_reset_to_factory_new_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_reset_to_fn_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_reset_to_factory_new_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_RESET_TO_FN_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_reset_to_factory_new_request_sample", (FMT__0));
}/* void get_reset_to_factory_new_request_sample(zb_uint8_t param) */

void send_network_start_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t enc_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_network_start_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_NETWORK_START_REQ(
      buf,
      addr,
      0,
      enc_key,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      addr,
      NULL,
      status);

  TRACE_MSG(TRACE_APS1, "< send_network_start_request_sample", (FMT__0));
}/* void send_identify_request_sample(zb_uint8_t param) */

void get_network_start_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_start_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_start_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_START_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_SEND_NETWORK_START_RES(buf, zcl_header.seq_number, 0, addr, NULL, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_start_request_sample", (FMT__0));
}/* void get_reset_to_factory_new_request_sample(zb_uint8_t param) */

void get_network_start_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_start_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_start_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_START_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_start_response_sample", (FMT__0));
}/* void get_reset_to_factory_new_request_sample(zb_uint8_t param) */

void send_network_join_router_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t enc_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_network_join_router_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_REQ(
      buf,
      0,
      enc_key,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      addr,
      NULL,
      status);

  TRACE_MSG(TRACE_APS1, "< send_network_join_router_request_sample", (FMT__0));
}/* void send_network_join_router_request_sample(zb_uint8_t param) */

void get_network_joint_router_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_join_router_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_joint_router_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ROUTER_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ROUTER_RES(
        buf, 0, zcl_header.seq_number, addr, NULL, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_joint_router_request_sample", (FMT__0));
}/* void get_network_joint_router_request_sample(zb_uint8_t param) */

void get_network_join_router_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_join_router_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_join_router_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ROUTER_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_join_router_response_sample", (FMT__0));
}/* void get_reset_to_factory_new_request_sample(zb_uint8_t param) */

void send_network_join_ed_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t enc_key[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_network_join_ed_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ED_REQ(
      buf,
      0,
      enc_key,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      addr,
      NULL,
      status);

  TRACE_MSG(TRACE_APS1, "< send_network_join_ed_request_sample", (FMT__0));
}/* void send_network_join_ed_request_sample(zb_uint8_t param) */

void get_network_joint_ed_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_join_end_device_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_joint_ed_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ED_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_SEND_NETWORK_JOIN_ED_RES(
        buf, 0, zcl_header.seq_number, addr, NULL, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_joint_ed_request_sample", (FMT__0));
}/* void get_network_joint_ed_request_sample(zb_uint8_t param) */

void get_network_join_ed_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_join_end_device_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_join_ed_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_JOIN_ED_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_join_ed_response_sample", (FMT__0));
}/* void get_network_join_ed_response_sample(zb_uint8_t param) */

void send_network_update_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_ieee_addr_t addr = {0, 1, 2, 3, 4, 5, 6, 7};
  zb_uint8_t status;

  TRACE_MSG(TRACE_APS1, "> send_network_update_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_NETWORK_UPDATE_REQ(buf, 0, addr, NULL, status);

  TRACE_MSG(TRACE_APS1, "< send_network_update_sample", (FMT__0));
}/* void send_network_update_sample(zb_uint8_t param) */

void get_network_update_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_network_update_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_network_update_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_NETWORK_UPDATE_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_network_update_sample", (FMT__0));
}/* void get_network_update_sample(zb_uint8_t param) */

void send_group_id_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t addr = 0x0101;

  TRACE_MSG(TRACE_APS1, "> send_group_id_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_GET_GROUP_ID_REQ(
      buf,
      0,
      addr,
      0,
      0,
      NULL);

  TRACE_MSG(TRACE_APS1, "< send_group_id_request_sample", (FMT__0));
}/* void send_group_id_request_sample(zb_uint8_t param) */

void get_group_id_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_get_group_ids_req_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t* ptr;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_group_id_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_GET_GROUP_ID_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_INIT_GET_GROUP_ID_RES(buf, ptr, 1, 0, 1, zcl_header.seq_number);
    ZB_ZLL_COMMISSIONING_ADD_GROUP_INFO_GET_GROUP_ID_RES(
        ptr,
        0x0102,
        ZB_ZLL_COMMISSIONING_GROUP_INFO_GROUP_TYPE_VALUE);
    ZB_ZLL_COMMISSIONING_SEND_GET_GROUP_ID_RES(
        buf,
        ptr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&zcl_header).source.u.short_addr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&zcl_header).src_endpoint,
        0,
        NULL);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_group_id_request_sample", (FMT__0));
}/* void get_group_id_request_sample(zb_uint8_t param) */

void get_group_id_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_get_group_ids_res_permanent_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_zll_commissioning_group_info_record_t grp_info;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_group_id_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_GET_GROUP_ID_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_GET_NEXT_GROUP_INFO_GET_GROUP_ID_RES(&grp_info, buf, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_group_id_response_sample", (FMT__0));
}/* void get_group_id_response_sample(zb_uint8_t param) */

void send_ep_list_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t addr = 0x0101;

  TRACE_MSG(TRACE_APS1, "> send_group_id_request_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_GET_EP_LIST_REQ(
      buf,
      0,
      addr,
      0,
      0,
      NULL);

  TRACE_MSG(TRACE_APS1, "< send_group_id_request_sample", (FMT__0));
}/* void send_group_id_request_sample(zb_uint8_t param) */

void get_ep_list_request_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_get_endpoint_list_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t* ptr;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_ep_list_request_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_GET_EP_LIST_REQ(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_INIT_GET_EP_LIST_RES(buf, 1, 0, 1, zcl_header.seq_number, ptr);
    ZB_ZLL_COMMISSIONING_ADD_EP_INFO_GET_EP_LIST_RES(ptr, 0x0102, 0, ZB_AF_ZLL_PROFILE_ID, 0, 0);
    ZB_ZLL_COMMISSIONING_SEND_GET_EP_LIST_RES(
        buf,
        ptr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&zcl_header).source.u.short_addr,
        ZB_ZCL_PARSED_HDR_SHORT_DATA(&zcl_header).src_endpoint,
        0,
        NULL);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_ep_list_request_sample", (FMT__0));
}/* void get_ep_list_request_sample(zb_uint8_t param) */

void get_ep_list_response_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_get_endpoint_list_res_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_zll_commissioning_endpoint_info_record_t ep_info;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_ep_list_response_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_GET_EP_LIST_RES(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_SUCCESS)
  {
    ZB_ZLL_COMMISSIONING_GET_NEXT_EP_INFO_GET_EP_LIST_RES(&ep_info, buf, stat);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_ep_list_response_sample", (FMT__0));
}/* void get_ep_list_response_sample(zb_uint8_t param) */

void send_ep_info_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t addr = 0x0101;

  TRACE_MSG(TRACE_APS1, "> send_ep_info_sample param %hd", (FMT__H, param));

  ZB_ZLL_COMMISSIONING_SEND_EP_INFO(
      buf,
      0,
      ZB_AF_ZLL_PROFILE_ID,
      0,
      0,
      addr,
      0,
      0,
      NULL);

  TRACE_MSG(TRACE_APS1, "< send_ep_info_sample", (FMT__0));
}/* void send_ep_info_sample(zb_uint8_t param) */

void get_ep_info_sample(zb_uint8_t param)
{
  zb_buf_t* buf = ZB_BUF_FROM_REF(param);
  zb_zll_commissioning_endpoint_information_t request;
  zb_zcl_parsed_hdr_t zcl_header;
  zb_uint8_t stat;

  TRACE_MSG(TRACE_APS1, "> get_ep_info_sample param %hd", (FMT__H, param));

  ZB_ZCL_COPY_PARSED_HEADER(buf, &zcl_header);

  ZB_ZLL_COMMISSIONING_GET_EP_INFO(&request, buf, stat);
  if (stat == ZB_ZCL_PARSE_STATUS_FAILURE)
  {
    TRACE_MSG(TRACE_ERROR, "Wrong packet size.", (FMT__0));
  }

  TRACE_MSG(TRACE_APS1, "< get_ep_info_sample", (FMT__0));
}/* void get_ep_info_sample(zb_uint8_t param) */

#endif /* defined ZB_ENABLE_ZLL */
