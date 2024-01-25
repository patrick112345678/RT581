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


#define ZB_TEST_NAME FB_PRE_TC_06_THR1
#define ZB_TRACE_FILE_ID 41231
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
#include "test_device_initiator.h"
#include "fb_pre_tc_06_common.h"


#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error ZB_USE_NVRAM is not compiled!
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

static zb_bool_t attr_on_off = 1;
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &attr_on_off);

static zb_uint8_t attr_name_support = 0;
ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(group_attr_list, &attr_name_support);


/********************* Declare device **************************/
DECLARE_INITIATOR_CLUSTER_LIST(initiator_device_clusters,
                               basic_attr_list,
                               identify_attr_list);

DECLARE_INITIATOR_EP(initiator_device_ep,
                     THR1_ENDPOINT,
                     initiator_device_clusters);

DECLARE_INITIATOR_CTX(initiator_device_ctx, initiator_device_ep);


/************************General Definitions*****************************/
#define MAX_U16_ARR_SZ 8
#define MAX_U8_ARR_SZ  6
typedef struct StateParams_s
{
  int         state;
  zb_uint8_t  repetition_counter;
  zb_uint8_t  counter_threshold;
  zb_uint8_t  dest_ep;
  zb_uint16_t dest_addr;
  zb_uint8_t  num_of_params_u16;
  zb_uint16_t array_u16[MAX_U16_ARR_SZ];
  zb_uint8_t  num_of_params_u8;
  zb_uint8_t  array_u8[MAX_U8_ARR_SZ];
} StateParams_t;


enum TestStates_e
{
  S00_START_FB,
  S01_REMOVE_ALL_GROUPS,
  S02_GET_GROUP_MEMBERSHIP_ALL,
  S03_ADD_GROUP_M,
  S04_BRCAST_ADD_GROUP_IF_IDENT_P,
  S05_BRCAST_ADD_GROUP_IF_IDENT_N,
  S06_GET_GROUP_MEMBERSHIP_N_P,
  S07_VIEW_GROUP_N,
  S08_ADD_GROUP_OI6,
  S09_GET_GROUP_MEMBERSHIP_ALL,
  S10_REMOVE_GROUP_O1,
  S11_GET_GROUP_MEMBERSHIP_ALL,
  S12_REMOVE_ALL_GROUPS,
  S13_GET_GROUP_MEMBERSHIP_ALL,
  S14_BRCAST_ADD_GROUP_M,
  S15_GET_GROUP_MEMBERSHIP_ALL,
  S16_ADD_GROUP_N,
  S17_ADD_GROUP_N,
  S18_GET_GROUP_MEMBERSHIP_ALL,
  S19_READ_ATTR_REQ,
  S20_ADD_GROUP_O_WITH_NAME,
  S21_VIEW_GROUP_O,
  S22_BRCAST_GET_GROUP_MEMBERSHIP,
  S23_BRCAST_EP_GET_GROUP_MEMBERSHIP_ALL,
  S24_GET_GROUP_MEMBERSHIP_1_N,
  S25_GET_GROUP_MEMBERSHIP_1_P,
  S26_VIEW_GROUP_P,
  S27_REMOVE_GROUP_P,
  S28_FINISH_TEST
};


static StateParams_t s_test_desc;


static void th_buffer_manager(zb_uint8_t unused);
static zb_uint8_t th_zcl_handler(zb_uint8_t param);
static void move_to_next_state(zb_uint8_t unused);

static void send_remove_all_groups(zb_uint8_t param);
static void send_remove_group(zb_uint8_t param);
static void send_get_group_membership(zb_uint8_t param);
static void send_add_group(zb_uint8_t param);
static void send_add_group_if_identifying(zb_uint8_t param);
static void send_view_group(zb_uint8_t param);
static void send_read_attr_req(zb_uint8_t param);



/*********************Additional functions*******************************/
static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster);
static void trigger_fb_initiator(zb_uint8_t unused);



MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);
  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;

  /* Assignment required to force Distributed formation */
  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_IEEE_ADDR_COPY(ZB_AIB().trust_center_address, g_unknown_ieee_addr);
  zb_secur_setup_nwk_key(g_nwk_key, 0);

  ZB_AF_REGISTER_DEVICE_CTX(&initiator_device_ctx);
  ZB_AIB().aps_use_nvram = 1;

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


/***********************************Implementation**********************************/
static void th_buffer_manager(zb_uint8_t unused)
{
  zb_callback_t next_func = NULL;

  s_test_desc.dest_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  s_test_desc.dest_ep = DUT_ENDPOINT;

  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZCL1, ">>th_buffer_manager: state = %d", (FMT__D, s_test_desc.state));

  switch (s_test_desc.state)
  {
    case S00_START_FB:
      {
        ZB_SCHEDULE_ALARM(trigger_fb_initiator, 0, THR1_FB_INITIATOR_DELAY);
      }
      break;

    case S01_REMOVE_ALL_GROUPS:
    case S12_REMOVE_ALL_GROUPS:
      {
        next_func = send_remove_all_groups;
      }
      break;

    case S03_ADD_GROUP_M:
      {
        next_func = send_add_group;
        s_test_desc.array_u16[0] = GROUP_M;
      }
      break;
    case S04_BRCAST_ADD_GROUP_IF_IDENT_P:
      {
        next_func = send_add_group_if_identifying;
        s_test_desc.dest_addr = 0xfffd;
        s_test_desc.array_u16[0] = GROUP_P;
        ZB_SCHEDULE_ALARM(move_to_next_state, 0, THR1_SHORT_DELAY);
      }
      break;

    case S05_BRCAST_ADD_GROUP_IF_IDENT_N:
      {
        next_func = send_add_group_if_identifying;
        s_test_desc.dest_addr = 0xfffd;
        s_test_desc.array_u16[0] = GROUP_N;
        ZB_SCHEDULE_ALARM(move_to_next_state, 0, THR1_SHORT_DELAY);
      }
      break;
    case S06_GET_GROUP_MEMBERSHIP_N_P:
      {
        next_func = send_get_group_membership;
        s_test_desc.num_of_params_u16 = 2;
        s_test_desc.array_u16[0] = GROUP_N;
        s_test_desc.array_u16[1] = GROUP_P;
      }
      break;
    case S07_VIEW_GROUP_N:
      {
        next_func = send_view_group;
        s_test_desc.array_u16[0] = GROUP_N;
      }
      break;

    case S08_ADD_GROUP_OI6:
      {
        next_func = send_add_group;
        s_test_desc.array_u16[0] = GROUP_OI_START + s_test_desc.repetition_counter;
      }
      break;

    case S02_GET_GROUP_MEMBERSHIP_ALL:
    case S09_GET_GROUP_MEMBERSHIP_ALL:
    case S11_GET_GROUP_MEMBERSHIP_ALL:
    case S13_GET_GROUP_MEMBERSHIP_ALL:
    case S15_GET_GROUP_MEMBERSHIP_ALL:
    case S18_GET_GROUP_MEMBERSHIP_ALL:
      {
        next_func = send_get_group_membership;
        s_test_desc.num_of_params_u16 = 0;
      }
      break;
    case S10_REMOVE_GROUP_O1:
      {
        next_func = send_remove_group;
        s_test_desc.array_u16[0] = GROUP_OI_START + 0;
      }
      break;
  
    case S14_BRCAST_ADD_GROUP_M:
      {
        s_test_desc.dest_addr = 0xfffd;
        next_func = send_add_group;
        s_test_desc.array_u16[0] = GROUP_M;
        ZB_SCHEDULE_ALARM(move_to_next_state, 0, THR1_SHORT_DELAY);
      }
      break;
    case S16_ADD_GROUP_N:
    case S17_ADD_GROUP_N:
      {
        next_func = send_add_group;
        s_test_desc.array_u16[0] = GROUP_N;
      }
      break;

    case S19_READ_ATTR_REQ:
      {
        next_func = send_read_attr_req;
      }
      break;

    case S20_ADD_GROUP_O_WITH_NAME:
      {
        int i;

        next_func = send_add_group;
        s_test_desc.array_u16[0] = GROUP_O;
        s_test_desc.num_of_params_u16 = 6;
        s_test_desc.array_u8[0] = 0x05;

        for (i = 1; i < 6; ++i)
        {
          s_test_desc.array_u8[i] = (zb_uint8_t) i-1;
        }
      }
      break;

    case S21_VIEW_GROUP_O:
      {
        next_func = send_view_group;
        s_test_desc.array_u16[0] = GROUP_O;
      }
      break;
    case S22_BRCAST_GET_GROUP_MEMBERSHIP:
      {
        next_func = send_get_group_membership;
        s_test_desc.dest_addr = 0xfffd;
        s_test_desc.num_of_params_u16 = 0;
      }
      break;

    case S23_BRCAST_EP_GET_GROUP_MEMBERSHIP_ALL:
      {
        next_func = send_get_group_membership;
        s_test_desc.num_of_params_u16 = 0;
        s_test_desc.dest_ep = 0xff;
      }
      break;

    case S24_GET_GROUP_MEMBERSHIP_1_N:
      {
        next_func = send_get_group_membership;
        s_test_desc.num_of_params_u16 = 1;
        s_test_desc.array_u16[0] = GROUP_N;
      }
      break;
    case S25_GET_GROUP_MEMBERSHIP_1_P:
      {
        next_func = send_get_group_membership;
        s_test_desc.num_of_params_u16 = 1;
        s_test_desc.array_u16[0] = GROUP_P;
      }
      break;

    case S26_VIEW_GROUP_P:
      {
        next_func = send_view_group;
        s_test_desc.array_u16[0] = GROUP_P;
      }
      break;
    case S27_REMOVE_GROUP_P:
      {
        next_func = send_remove_group;
        s_test_desc.array_u16[0] = GROUP_P;
      }
      break;

    case S28_FINISH_TEST:
      {
        TRACE_MSG(TRACE_ZCL1, "th_buffer_manager: test fhas been finished", (FMT__0));
      }
      break;
    default:
      {
        TRACE_MSG(TRACE_ZCL1, "th_buffer_manager: unknown state - nothing to do", (FMT__0));
      }
  }

  if (next_func)
  {
    ZB_GET_OUT_BUF_DELAYED(next_func);
  }

  TRACE_MSG(TRACE_ZCL1, "<<th_buffer_manager", (FMT__0));
}


static zb_uint8_t th_zcl_handler(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_zcl_parsed_hdr_t *cmd_info = ZB_GET_BUF_PARAM(buf, zb_zcl_parsed_hdr_t);
  int check_cmd_is_common = cmd_info->is_common_command &&
                            (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI) &&
                            (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_GROUPS);
  int check_cmd_is_groups = !cmd_info->is_common_command &&
                            (cmd_info->cmd_direction == ZB_ZCL_FRAME_DIRECTION_TO_CLI) &&
                            (cmd_info->cluster_id == ZB_ZCL_CLUSTER_ID_GROUPS);
  int move_fsm = 0;

  TRACE_MSG(TRACE_ZCL1, ">>th_zcl_handler: buf_param = %d, state = %d",
            (FMT__D_D, param, s_test_desc.state));

  if (check_cmd_is_common)
  {
    switch (cmd_info->cmd_id)
    {
      case ZB_ZCL_CMD_READ_ATTRIB_RESP:
      case ZB_ZCL_CMD_DEFAULT_RESP:
        ZB_SCHEDULE_ALARM_CANCEL(move_to_next_state, ZB_ALARM_ANY_PARAM);
        move_fsm = 1;
        break;
    }
  }
  else if (check_cmd_is_groups)
  {
    switch (cmd_info->cmd_id)
    {
      case ZB_ZCL_CMD_GROUPS_ADD_GROUP_RES:
      case ZB_ZCL_CMD_GROUPS_VIEW_GROUP_RES:
      case ZB_ZCL_CMD_GROUPS_GET_GROUP_MEMBERSHIP_RES:
      case ZB_ZCL_CMD_GROUPS_REMOVE_GROUP_RES:
        ZB_SCHEDULE_ALARM_CANCEL(move_to_next_state, ZB_ALARM_ANY_PARAM);
        move_fsm = 1;
        break;
    }
  }

  if (move_fsm)
  {
    move_to_next_state(0);
  }

  TRACE_MSG(TRACE_ZCL1, "<<th_zcl_handler", (FMT__0));

  return ZB_FALSE;
}


static void move_to_next_state(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZCL1, ">>move_to_next_state: current state = %d", (FMT__D, s_test_desc.state));

  switch (s_test_desc.state)
  {
    case S00_START_FB:
      {
        ++s_test_desc.state;
        ZB_AF_SET_ENDPOINT_HANDLER(THR1_ENDPOINT, th_zcl_handler);
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;
    case S01_REMOVE_ALL_GROUPS:
    case S02_GET_GROUP_MEMBERSHIP_ALL:
    case S03_ADD_GROUP_M:
      {
        ++s_test_desc.state;
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;
    case S04_BRCAST_ADD_GROUP_IF_IDENT_P:
      {
        ++s_test_desc.state;
        ZB_SCHEDULE_ALARM(th_buffer_manager, 100, DUT_START_IDENTIFYING_DELAY);
      }
      break;
      
    case S05_BRCAST_ADD_GROUP_IF_IDENT_N:
      {
        ++s_test_desc.state;
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;
    case S06_GET_GROUP_MEMBERSHIP_N_P:
      {
        ++s_test_desc.state;
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;
    case S07_VIEW_GROUP_N:
      {
        ++s_test_desc.state;
        s_test_desc.repetition_counter = 0;
        s_test_desc.counter_threshold = 6;
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;

      
    case S08_ADD_GROUP_OI6:
      {
        if (++s_test_desc.repetition_counter >= s_test_desc.counter_threshold)
        {
          ++s_test_desc.state;
        }
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;

    case S09_GET_GROUP_MEMBERSHIP_ALL:
    case S10_REMOVE_GROUP_O1:
    case S11_GET_GROUP_MEMBERSHIP_ALL:
    case S12_REMOVE_ALL_GROUPS:
    case S13_GET_GROUP_MEMBERSHIP_ALL:
    case S14_BRCAST_ADD_GROUP_M:
    case S15_GET_GROUP_MEMBERSHIP_ALL:
    case S16_ADD_GROUP_N:
    case S17_ADD_GROUP_N:
    case S18_GET_GROUP_MEMBERSHIP_ALL:
    case S19_READ_ATTR_REQ:
    case S20_ADD_GROUP_O_WITH_NAME:
    case S21_VIEW_GROUP_O:
    case S22_BRCAST_GET_GROUP_MEMBERSHIP:
    case S23_BRCAST_EP_GET_GROUP_MEMBERSHIP_ALL:
    case S24_GET_GROUP_MEMBERSHIP_1_N:
    case S25_GET_GROUP_MEMBERSHIP_1_P:
    case S26_VIEW_GROUP_P:
    case S27_REMOVE_GROUP_P:
      {
        ++s_test_desc.state;
        ZB_SCHEDULE_CALLBACK(th_buffer_manager, 255);
      }
      break;

    case S28_FINISH_TEST:
      {
        TRACE_MSG(TRACE_ZCL1, "move_to_next_state: FINISH", (FMT__0));
      }
      break;
    default:
      {
        TRACE_MSG(TRACE_ZCL1, "move_to_next_state: unknown state transition", (FMT__0));
      }
  }

  TRACE_MSG(TRACE_ZCL1, "<<move_to_next_state", (FMT__0));
}



static void send_remove_all_groups(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;

  TRACE_MSG(TRACE_ZCL2, ">>send_remove_all_groups: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_SEND_REMOVE_ALL_GROUPS_REQ(buf, dest_addr,
                                          ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                          dest_ep, THR1_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                          ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                          NULL);

  TRACE_MSG(TRACE_ZCL2, "<<send_remove_all_groups", (FMT__0));
}


static void send_remove_group(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;

  TRACE_MSG(TRACE_ZCL2, ">>send_remove_group: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_SEND_REMOVE_GROUP_REQ(buf, dest_addr,
                                      ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                      dest_ep, THR1_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                      NULL, s_test_desc.array_u16[0]);

  TRACE_MSG(TRACE_ZCL2, "<<send_remove_group", (FMT__0));
}


static void send_get_group_membership(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;
  zb_uint8_t *ptr;
  zb_uint8_t i;

  TRACE_MSG(TRACE_ZCL2, ">>send_get_group_membership: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_INIT_GET_GROUP_MEMBERSHIP_REQ(buf, ptr,
    ZB_ZCL_ENABLE_DEFAULT_RESPONSE, s_test_desc.num_of_params_u16);

  for (i = 0; i < s_test_desc.num_of_params_u16; ++i)
  {
    ZB_ZCL_GROUPS_ADD_ID_GET_GROUP_MEMBERSHIP_REQ(ptr, s_test_desc.array_u16[i]);
  }

  ZB_ZCL_GROUPS_SEND_GET_GROUP_MEMBERSHIP_REQ(buf, ptr, dest_addr,
                                              ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                              dest_ep, THR1_ENDPOINT,
                                              ZB_AF_HA_PROFILE_ID, NULL);

  TRACE_MSG(TRACE_ZCL2, "<<send_get_group_membership", (FMT__0));
}


static void send_add_group(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;

  TRACE_MSG(TRACE_ZCL2, ">>send_add_group: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_SEND_ADD_GROUP_REQ(buf, dest_addr,
                                   ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                   dest_ep, THR1_ENDPOINT,
                                   ZB_AF_HA_PROFILE_ID, ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                   NULL, s_test_desc.array_u16[0]);
  
  TRACE_MSG(TRACE_ZCL2, "<<send_add_group", (FMT__0));
}


static void send_add_group_if_identifying(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;

  TRACE_MSG(TRACE_ZCL2, ">>send_add_group_if_identifying: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_SEND_ADD_GROUP_IF_IDENT_REQ(buf, dest_addr,
                                            ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                            dest_ep, THR1_ENDPOINT,
                                            ZB_AF_HA_PROFILE_ID,
                                            ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                            NULL, s_test_desc.array_u16[0]);

  TRACE_MSG(TRACE_ZCL2, "<<send_add_group_if_identifying", (FMT__0));
}


static void send_view_group(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;

  TRACE_MSG(TRACE_ZCL2, ">>send_view_group: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GROUPS_SEND_VIEW_GROUP_REQ(buf, dest_addr,
                                    ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                    dest_ep, THR1_ENDPOINT,
                                    ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                                    NULL, s_test_desc.array_u16[0]);

  TRACE_MSG(TRACE_ZCL2, "<<send_view_group", (FMT__0));
}


static void send_read_attr_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_uint16_t dest_addr = s_test_desc.dest_addr;
  zb_uint8_t dest_ep = s_test_desc.dest_ep;
  zb_uint8_t *ptr;

  TRACE_MSG(TRACE_ZCL2, ">>send_read_attr_req: buf_param = %d", (FMT__D, param));

  ZB_ZCL_GENERAL_INIT_READ_ATTR_REQ_A(buf, ptr, ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                                      ZB_ZCL_ENABLE_DEFAULT_RESPONSE);
  ZB_ZCL_GENERAL_ADD_ID_READ_ATTR_REQ(ptr, ZB_ZCL_ATTR_GROUPS_NAME_SUPPORT_ID);
  ZB_ZCL_GENERAL_SEND_READ_ATTR_REQ(buf, ptr, dest_addr, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                                    dest_ep, THR1_ENDPOINT, ZB_AF_HA_PROFILE_ID,
                                    ZB_ZCL_CLUSTER_ID_GROUPS, NULL);

  TRACE_MSG(TRACE_ZCL2, "<<send_read_attr_req", (FMT__0));
}




static zb_bool_t finding_binding_cb(zb_int16_t status,
                                    zb_ieee_addr_t addr,
                                    zb_uint8_t ep,
                                    zb_uint16_t cluster)
{
  TRACE_MSG(TRACE_ZCL1, "finding_binding_cb status %d addr " TRACE_FORMAT_64 " ep %hd cluster %d",
            (FMT__D_A_H_D, status, TRACE_ARG_64(addr), ep, cluster));
  return ZB_TRUE;
}


static void trigger_fb_initiator(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  ZB_BDB().bdb_commissioning_time = THR1_FB_DURATION;
  ZB_BDB().bdb_commissioning_group_id = GROUP_M;
  zb_bdb_finding_binding_initiator(THR1_ENDPOINT, finding_binding_cb);
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, ZB_NIB().extended_pan_id))
        {
          bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        }
        else
        {
          th_buffer_manager(0);
        }
        break;

      case ZB_BDB_SIGNAL_STEERING:
        TRACE_MSG(TRACE_APS1, "Network steering", (FMT__0));
        th_buffer_manager(0);
        break;

      case ZB_BDB_SIGNAL_FINDING_AND_BINDING_INITIATOR_FINISHED:
        TRACE_MSG(TRACE_APS1, "Finding&binding done", (FMT__0));
        if (BDB_COMM_CTX().state == ZB_BDB_COMM_IDLE)
        {
          move_to_next_state(0);
        }
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
