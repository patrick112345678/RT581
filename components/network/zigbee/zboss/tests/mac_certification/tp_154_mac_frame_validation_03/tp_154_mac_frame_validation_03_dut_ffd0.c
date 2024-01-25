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
/* PURPOSE: TP/154/MAC/FRAME-VALIDATION-03 DUT FFD0
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_FRAME_VALIDATION_03_DUT_FFD0
#define ZB_TRACE_FILE_ID 57491
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_frame_validation_03_common.h"
#include "zb_osif.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MLME_COMM_STATUS_INDICATION
#define USE_ZB_MCPS_DATA_INDICATION
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_ffd0 = TEST_DUT_FFD0_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_get_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_get_short_addr(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
extern zb_step_status_t step_state;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_frame_validation_03_dut_ffd0");

  ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
  init_RT569_LED();
#if UART_CONTROL
  test_control_init();
  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_next_step);
#endif
  while(1)
  {
    zb_sched_loop_iteration();
  }


  TRACE_DEINIT();

  MAIN_RETURN(0);
}


static void test_started_cb(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  //TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));

  zb_buf_get_out_delayed(test_mlme_reset_request);
}

/***************** Test functions *****************/
static void test_mlme_reset_request(zb_uint8_t param)
{
  zb_mlme_reset_request_t *reset_req = ZB_BUF_GET_PARAM(param, zb_mlme_reset_request_t);
  reset_req->set_default_pib = 1;

  //TRACE_MSG(TRACE_APP1, "MLME-RESET.request()", (FMT__0));

  ZB_SCHEDULE_CALLBACK(zb_mlme_reset_request, param);
}


static void test_set_ieee_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
  req->pib_length = sizeof(zb_ieee_addr_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr_dut_ffd0, sizeof(zb_ieee_addr_t));

  req->confirm_cb_u.cb = test_get_ieee_addr;
  zb_mlme_set_request(param);
}


static void test_get_ieee_addr(zb_uint8_t param)
{
  zb_mlme_get_request_t *req;

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_request_t));
  req->pib_attr   = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
  req->pib_index  = 0;
  req->confirm_cb_u.cb = test_set_short_addr;

  zb_mlme_get_request(param);
}


static void test_set_short_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t short_addr = TEST_DUT_FFD0_SHORT_ADDRESS;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &short_addr, sizeof(zb_uint16_t));

  req->confirm_cb_u.cb = test_get_short_addr;
  zb_mlme_set_request(param);
}


static void test_get_short_addr(zb_uint8_t param)
{
  zb_mlme_get_request_t *req;

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_request_t));
  req->pib_attr   = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_index  = 0;
  req->confirm_cb_u.cb = test_set_association_permit;

  zb_mlme_get_request(param);
}


static void test_set_association_permit(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t association_permit = TEST_ASSOCIATION_PERMIT;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() Association permit", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_ASSOCIATION_PERMIT;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &association_permit, sizeof(zb_uint8_t));

  req->confirm_cb_u.cb = test_set_rx_on_when_idle;

  zb_mlme_set_request(param);
}

#if CHANNEL_CHANGE
static void test_get_command(zb_uint8_t param)
{
	switch (step_state)
	{
		case STARTING:
			printf("TEST_CHANNEL: %d.\n", TEST_CHANNEL);	
			printf("Send u/d to increase/decrease channel number. Send p to pass.\n");		
			test_step_register(test_get_command, param, 0);
      test_control_start(ZB_TEST_CONTROL_UART, 0);
		  break;
	  case CHANNEL_INCREASING:
			if(TEST_CHANNEL<26)
			{
		    printf("TEST_CHANNEL++.\n");	
		    TEST_CHANNEL++;
				printf("Channel has been changed to %d\n", TEST_CHANNEL);
			}
			else
			{
				printf("26 is the max channel number.\n");					
			}
			ZB_SCHEDULE_CALLBACK(test_get_command, param);		
      step_state = STEP_DONE;
	    break;
	  case CHANNEL_DECREASING:
		  if(TEST_CHANNEL>11)
			{
		    printf("TEST_CHANNEL--.\n");	
		    TEST_CHANNEL--;
				printf("Channel has been changed to %d\n", TEST_CHANNEL);
			}
			else
			{
				printf("11 is the min channel number.\n");							
			}
			ZB_SCHEDULE_CALLBACK(test_get_command, param);
      step_state = STEP_DONE;
	    break;
	  case TESTING_AND_PRINTING:
		  ZB_SCHEDULE_CALLBACK(test_mlme_start_request, param);	
		  step_state = STEP_DONE;
	    break;
		case STEP_DONE:
			test_step_register(test_get_command, param, 0);
      test_control_start(ZB_TEST_CONTROL_UART, 0);
		  break;
		default:
			printf("Unused command in this case.\n");
		  ZB_SCHEDULE_CALLBACK(test_get_command, param);
		  step_state = STEP_DONE;
		  break;	
	}
}
#endif

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t rx_on_when_idle = TEST_RX_ON_WHEN_IDLE;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &rx_on_when_idle, sizeof(zb_uint8_t));

#if CHANNEL_CHANGE
  req->confirm_cb_u.cb = test_get_command;
#else
	req->confirm_cb_u.cb = test_mlme_start_request;
#endif 
  zb_mlme_set_request(param);
}


static void test_mlme_start_request(zb_uint8_t param)
{
  zb_mlme_start_req_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_start_req_t);

  //TRACE_MSG(TRACE_APP1, "MLME-START.request()", (FMT__0));

  ZB_BZERO(req, sizeof(zb_mlme_start_req_t));

  req->pan_id = TEST_PAN_ID;
  req->logical_channel = TEST_CHANNEL;
  req->channel_page = TEST_PAGE;
  req->pan_coordinator = 1;      /* will be coordinator */
  req->coord_realignment = 0;
  req->beacon_order = ZB_TURN_OFF_ORDER;
  req->superframe_order = ZB_TURN_OFF_ORDER;

  ZB_SCHEDULE_CALLBACK(zb_mlme_start_request, param);
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
  //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

  /* Call the next test step */
  ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param)
{
  zb_ieee_addr_t device_address;
  zb_mlme_associate_indication_t *request = ZB_BUF_GET_PARAM((zb_bufid_t )param, zb_mlme_associate_indication_t);

  //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.indication()", (FMT__0));
  /*
    Very simple implementation: accept anybody, assign address 0x3344
   */
  ZB_IEEE_ADDR_COPY(device_address, request->device_address);

  //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.response()", (FMT__0));
  ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address,
                                   TEST_TH_FFD1_SHORT_ADDRESS, 0);
  ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);
}

ZB_MLME_COMM_STATUS_INDICATION(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-COMM-STATUS.indication()", (FMT__0));
  zb_ret_t ret;
  zb_address_ieee_ref_t addr_ref;
  zb_neighbor_tbl_ent_t *nent = NULL;
  zb_mlme_comm_status_indication_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_comm_status_indication_t);
	TRACE_MSG(TRACE_APP1, "PANid = %x", (FMT__H, TEST_PAN_ID));
	TRACE_MSG(TRACE_APP1, "SrcAddrMode = %x", (FMT__H, request->src_addr_mode));
	if (request->src_addr_mode == ZB_ADDR_64BIT_DEV)
  {
		//TRACE_MSG(TRACE_APP1, "SrcAddr = %x", (FMT__P, request->src_addr.addr_long));
		TRACE_MSG(TRACE_APP1, "SrcAddr " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(request->src_addr.addr_long)));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "SrcAddr = %x", (FMT__P, request->src_addr.addr_short));
  }
	TRACE_MSG(TRACE_APP1, "DstAddrMode = %x", (FMT__H, request->dst_addr_mode));
	if (request->dst_addr_mode == ZB_ADDR_64BIT_DEV)
  {
		//TRACE_MSG(TRACE_APP1, "DstAddr = %x", (FMT__P, request->dst_addr.addr_long));
		TRACE_MSG(TRACE_APP1, "DstAddr= " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(request->dst_addr.addr_long)));
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "DstAddr = %x", (FMT__P, request->dst_addr.addr_short));
  }
	TRACE_MSG(TRACE_APP1, "Status = %x", (FMT__H, request->status));

	if (request->status == 0xDE &&
		  request->src_addr_mode == 0x02 &&
	    request->dst_addr_mode == 0x02 &&
	    request->src_addr.addr_short == 0x3344 &&    
	    request->dst_addr.addr_short == 0x1122)
	{
		zb_osif_led_on(0);
		TRACE_MSG(TRACE_APP1, "0xDE = Unsupported_legacy", (FMT__0));
		TRACE_MSG(TRACE_APP1, "Frame_validation03 success", (FMT__0));
	}
  zb_buf_free(param);

} 

ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
	TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));
  zb_buf_free(param);
	//gpio_pin_clear(LED6);
}
/*! @} */
