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
/*  PURPOSE: TP/154/MAC/ACK-FRAME-DELIVERY-01 DUT FFD0 implementation
*/

#define ZB_TEST_NAME TP_154_MAC_ACK_FRAME_DELIVERY_01_DUT_FFD0
#define ZB_TRACE_FILE_ID 57457
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_ack_frame_delivery_01_common.h"


#ifndef ZB_MULTI_TEST

#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_COMM_STATUS_INDICATION

#include "zb_mac_only_stubs.h"

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif // #ifndef ZB_COORDINATOR_ROLE

#endif // #ifndef ZB_MULTI_TEST

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr = TEST_DUT_FFD0_IEEE_ADDR;

static void test_started_cb(zb_uint8_t unused);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_get_ieee_addr(zb_uint8_t param);
static void test_set_short_addr(zb_uint8_t param);
static void test_get_short_addr(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_mcps_data_request(zb_uint8_t param);

static zb_uint8_t g_dut_test_step = TEST_STEP_DUT_INITIAL;
extern zb_step_status_t step_state;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_ack_frame_delivery_01_dut_ffd0");

  ZB_SCHEDULE_CALLBACK(test_started_cb, 0);
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
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr, sizeof(zb_ieee_addr_t));

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

static void test_get_short_addr(zb_uint8_t param)
{
  zb_mlme_get_request_t *req;

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_get_request_t));
  req->pib_attr   = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_index  = 0;
	
#if CHANNEL_CHANGE
  req->confirm_cb_u.cb = test_get_command;
#else
	req->confirm_cb_u.cb = test_mlme_start_request;
#endif
  zb_mlme_get_request(param);
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

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t rx_on_when_idle = TEST_DUT_FFD0_RX_ON_WHEN_IDLE;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_length = sizeof(zb_uint8_t);
  *(zb_uint8_t *)(req+1) = rx_on_when_idle;

  zb_mlme_set_request(param);
}

static void test_mcps_data_request(zb_uint8_t param)
{

  zb_mcps_data_req_params_t *data_req;
  zb_uint8_t *msdu;
  zb_uint8_t i;
  zb_uint8_t msdu_length;
  TRACE_MSG(TRACE_APP1, "Next: MCPS-DATA.request", (FMT__0));
  g_dut_test_step++;

  msdu_length = TEST_MSDU_LENGTH;
  msdu = zb_buf_initial_alloc(param, msdu_length);
  for (i = 0; i < msdu_length; i++)
  {
    msdu[i] = i;
  }

  data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

  data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
  data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  data_req->dst_addr.addr_short = TEST_TH_FFD1_SHORT_ADDRESS;
  data_req->dst_pan_id = TEST_PAN_ID;
  data_req->msdu_handle = 0;
  data_req->tx_options = 0x01;
  
	#if UART_CONTROL
  test_step_register(zb_mcps_data_request, param, 0);
  test_control_start(ZB_TEST_CONTROL_UART, 0);
  #else
  ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
  #endif
}

/***************** MAC Callbacks *****************/
ZB_MLME_RESET_CONFIRM(zb_uint8_t param)
{
  //TRACE_MSG(TRACE_APP1, "MLME-RESET.confirm()", (FMT__0));

  /* Call the next test step */
  ZB_SCHEDULE_CALLBACK(test_set_ieee_addr, param);
}

ZB_MLME_START_CONFIRM(zb_uint8_t param)
{
  //TRACE_MSG(TRACE_APP1, "MLME-START.confirm()", (FMT__0));

  ZB_SCHEDULE_CALLBACK(test_set_rx_on_when_idle, param);
}

ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param)
{
  zb_ieee_addr_t device_address;
  zb_mlme_associate_indication_t *request = ZB_BUF_GET_PARAM(param, zb_mlme_associate_indication_t);

  //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.indication()", (FMT__0));
  /*
    Very simple implementation: accept anybody, assign address 0x3344
   */
  ZB_IEEE_ADDR_COPY(device_address, request->device_address);

  //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.response()", (FMT__0));
  ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address, TEST_TH_FFD1_SHORT_ADDRESS, 0);
  ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);

}

ZB_MLME_COMM_STATUS_INDICATION(zb_uint8_t param)
{
  TRACE_MSG(TRACE_APP1, "MLME-COMM-STATUS.indication()", (FMT__0));
  ZB_SCHEDULE_ALARM(test_mcps_data_request, param, TEST_DATA_FRAME_PERIOD);
	TRACE_MSG(TRACE_APP1, "Check for 3 rounds of TH RX_ON_When_Idle: Off -> On -> Off", (FMT__0));
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
  zb_mcps_data_confirm_params_t data_conf_params = *ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
	TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm(): status = %hx", (FMT__0, data_conf_params.status));
  if (g_dut_test_step < TEST_STEP_DUT_SEND_DATA_FRAME3)
  {
    ZB_SCHEDULE_ALARM(test_mcps_data_request, param, TEST_DATA_FRAME_PERIOD);
  }
  else
  {
    zb_buf_free(param);
  }
}
/*! @} */
