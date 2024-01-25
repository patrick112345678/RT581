/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: P/154/MAC/DATA-02 DUT FFD0
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_02_DUT_FFD0
#define ZB_TRACE_FILE_ID 57201
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_02_common.h"
#include "zb_osif.h"

#if defined MAC_TEST_ADD_EXT_DELAY && defined ZB_NRF_52
#include "boards.h"
#endif /* MAC_TEST_ADD_EXT_DELAY && ZB_NRF_52 */

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_START_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_INDICATION
#define USE_ZB_MLME_COMM_STATUS_INDICATION
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MCPS_DATA_CONFIRM
#define USE_ZB_MLME_PURGE_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_dut_ffd0 = TEST_DUT_FFD0_MAC_ADDRESS;
static zb_ieee_addr_t g_ieee_addr_th_rfd1 = TEST_TH_RFD1_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_set_short_addr_ffd0(zb_uint8_t param);
static void test_set_short_addr_none(zb_uint8_t param);
static void test_set_short_addr_restore(zb_uint8_t param);
static void test_set_short_addr_req(zb_uint8_t param, zb_uint16_t short_addr, zb_callback_t cb);
static void test_set_pan_id(zb_uint8_t param);
static void test_set_rx_on_when_idle(zb_uint8_t param);
static void test_set_association_permit(zb_uint8_t param);
static void test_mlme_start_request(zb_uint8_t param);
static void test_purge_request(zb_uint8_t param);

static void test_set_short_addr_none_conf(zb_uint8_t param);
static void test_set_short_addr_restore_conf(zb_uint8_t param);

ZB_MLME_ASSOCIATE_INDICATION(zb_uint8_t param);

static zb_uint8_t dut_test_step = TEST_STEP_INITIAL;

static zb_uint8_t dut_msdu_payload[TEST_MSDU_LENGTH] = TEST_MSDU;

zb_mac_test2_t test_result2;
extern zb_step_status_t step_state;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_data_02_dut_ffd0");

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

  req->confirm_cb_u.cb = test_set_short_addr_ffd0;
  zb_mlme_set_request(param);
}

static void test_set_short_addr_ffd0(zb_uint8_t param)
{
  test_set_short_addr_req(param, TEST_DUT_FFD0_SHORT_ADDRESS, test_set_pan_id);
}

static void test_set_short_addr_none(zb_uint8_t param)
{
  test_set_short_addr_req(param, ZB_MAC_SHORT_ADDR_NOT_ALLOCATED, test_set_short_addr_none_conf);
}

static void test_set_short_addr_restore(zb_uint8_t param)
{
  test_set_short_addr_req(param, TEST_DUT_FFD0_SHORT_ADDRESS, test_set_short_addr_restore_conf);
}

static void test_set_short_addr_req(zb_uint8_t param, zb_uint16_t short_addr, zb_callback_t cb)
{
  zb_mlme_set_request_t *req;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac short addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_SHORT_ADDRESS;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &short_addr, sizeof(zb_uint16_t));

  req->confirm_cb_u.cb = cb;
  zb_mlme_set_request(param);
}

static void test_set_pan_id(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint16_t panid = TEST_PAN_ID;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() PANID", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint16_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_PANID;
  req->pib_length = sizeof(zb_uint16_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &panid, sizeof(zb_uint16_t));

  req->confirm_cb_u.cb = test_set_rx_on_when_idle;
  zb_mlme_set_request(param);
}

static void test_set_rx_on_when_idle(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_uint8_t rx_on_when_idle = TEST_RX_ON_WHEN_IDLE;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() RxOnWhenIdle", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_uint8_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE;
  req->pib_length = sizeof(zb_uint8_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), &rx_on_when_idle, sizeof(zb_uint8_t));

  req->confirm_cb_u.cb = test_set_association_permit;
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

static void test_purge_request(zb_uint8_t param)
  {
    zb_mlme_purge_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_purge_request_t);
    req->msdu_handle = 0xc;
    TRACE_MSG(TRACE_APP1, "Next: MLME-PURGE.request", (FMT__0));
		#if UART_CONTROL
    test_step_register(zb_mlme_purge_request, param, 0);
    test_control_start(ZB_TEST_CONTROL_UART, 0);
    #else
    ZB_SCHEDULE_CALLBACK(zb_mlme_purge_request, param);
    #endif
  }

static void test_mcps_data_request(zb_uint8_t param)
{

  zb_mcps_data_req_params_t *data_req;
  zb_uint8_t *msdu;

  dut_test_step++;
	TRACE_MSG(TRACE_APP1, "Next: MCPS-DATA.request test_step %hd", (FMT__H, dut_test_step));

  msdu = zb_buf_initial_alloc(param, TEST_MSDU_LENGTH);
  ZB_MEMCPY(msdu, dut_msdu_payload, TEST_MSDU_LENGTH * sizeof(zb_uint8_t));

  data_req = ZB_BUF_GET_PARAM(param, zb_mcps_data_req_params_t);
  ZB_BZERO(data_req, sizeof(zb_mcps_data_req_params_t));

  switch(dut_test_step)
  {
    case  DUT2TH_SHORT2SHORT_INDIRECT_ACK:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
      data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->dst_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  DUT2TH_SHORT2EXT_INDIRECT_ACK:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2EXT_INDIRECT_ACK", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
      data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
      ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_rfd1);
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  DUT2TH_EXT2SHORT_INDIRECT_ACK:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2SHORT_INDIRECT_ACK", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
      ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_ffd0);
      data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->dst_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  DUT2TH_EXT2EXT_INDIRECT_ACK:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_EXT2EXT_INDIRECT_ACK", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_64BIT_DEV;
      ZB_IEEE_ADDR_COPY(data_req->src_addr.addr_long, g_ieee_addr_dut_ffd0);
      data_req->dst_addr_mode = ZB_ADDR_64BIT_DEV;
      ZB_IEEE_ADDR_COPY(data_req->dst_addr.addr_long, g_ieee_addr_th_rfd1);
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
      data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->dst_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_PURGE:
      //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_PURGE", (FMT__0));
      data_req->src_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->src_addr.addr_short = TEST_DUT_FFD0_SHORT_ADDRESS;
      data_req->dst_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
      data_req->dst_addr.addr_short = TEST_TH_RFD1_SHORT_ADDRESS;
      data_req->dst_pan_id = TEST_PAN_ID;
      data_req->msdu_handle = 0x0c;
      data_req->tx_options = 0x05;
      break;

    case  TEST_STEP_FINISHED:
      break;
  }
	if(dut_test_step < TEST_STEP_FINISHED)
	{
		#if UART_CONTROL
    test_step_register(zb_mcps_data_request, param, 0);
    test_control_start(ZB_TEST_CONTROL_UART, 0);
    #else
    ZB_SCHEDULE_CALLBACK(zb_mcps_data_request, param);
    #endif
	}
  if(dut_test_step == DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_PURGE)
  {
    zb_buf_get_out_delayed(test_purge_request);
  }
}

static void test_schedule_mcps_data_request(zb_uint8_t param, zb_uint32_t tmo)
{
#ifdef MAC_TEST_ADD_EXT_DELAY
#ifdef ZB_NRF_52
  if (dut_test_step == TH2DUT_SHORT2BROADCAST)
  {
    /* Wait for Button 1 to be pressed */
    bsp_board_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS);
    bsp_board_led_on(BSP_BOARD_LED_1);
    while(!bsp_board_button_state_get(bsp_board_pin_to_button_idx(BSP_BUTTON_1)));
    bsp_board_led_off(BSP_BOARD_LED_1);
  }
#endif /* ZB_NRF_52 */
  ZB_SCHEDULE_ALARM(test_mcps_data_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(10000));
#else
  if (!tmo)
  {
    ZB_SCHEDULE_CALLBACK(test_mcps_data_request, param);
  }
  else
  {
    ZB_SCHEDULE_ALARM(test_mcps_data_request, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(tmo));
  }
#endif /* MAC_TEST_ADD_DELAY */
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
  zb_buf_free(param);
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
  ZB_MLME_BUILD_ASSOCIATE_RESPONSE(param, device_address, TEST_TH_RFD1_SHORT_ADDRESS, 0);
  ZB_SCHEDULE_CALLBACK(zb_mlme_associate_response, param);
}

ZB_MLME_COMM_STATUS_INDICATION(zb_uint8_t param)
{
  //TRACE_MSG(TRACE_APP1, "MLME-COMM-STATUS.indication()", (FMT__0));
  zb_buf_free(param);
}

ZB_MCPS_DATA_CONFIRM(zb_uint8_t param)
{
  zb_mcps_data_confirm_params_t data_conf_params = *ZB_BUF_GET_PARAM(param, zb_mcps_data_confirm_params_t);
	if(data_conf_params.status == 0) //h~k
	{
		if(data_conf_params.src_addr_mode == 0x02) 
		{
			if(data_conf_params.dst_addr_mode == 0x02) //short to short
		  {
			  test_result2.dut2th.h = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, dut_test_step));
		  }
		  else if (data_conf_params.dst_addr_mode == 0x03) //short to ex
		  {
			  test_result2.dut2th.i = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, dut_test_step));
		  }
		}
		else if (data_conf_params.src_addr_mode == 0x03) 
		{
			if(data_conf_params.dst_addr_mode == 0x02) //ex to short
		  {
			  test_result2.dut2th.j = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, dut_test_step));
		  }
		  else if (data_conf_params.dst_addr_mode == 0x03) //ex to ex
		  {
			  test_result2.dut2th.k = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, dut_test_step));
		  }
		}
	}
	else if (data_conf_params.status == 0xF0) //l (transaction expired)
	{
		if (dut_test_step == DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE)
		{
			test_result2.dut2th.l = 1;
			TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm success. test_step %hd", (FMT__H, dut_test_step));
		}
	}
	
	
	//TRACE_MSG(TRACE_APP1, "MCPS-DATA.confirm()", (FMT__0));

  if (dut_test_step < TEST_STEP_FINISHED)
  {
		if (dut_test_step == DUT2TH_SHORT2SHORT_INDIRECT_ACK_NO_POLL_EXPIRE)
    {
      test_schedule_mcps_data_request(param, 5000);
			//test_schedule_mcps_data_request(param, 0);
      param = 0;
    }
		else
		{
			test_schedule_mcps_data_request(param, 0);
      param = 0;
		}
  }

  if (param != 0)
  {
    zb_buf_free(param);
  }
}


ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
  zb_mac_mhr_t mac_hdr;
	zb_uint_t mhr_size = zb_parse_mhr(&mac_hdr, param);
	zb_uint8_t fc1 = mac_hdr.frame_control[1];
	
	dut_test_step++;
	
	if(fc1/16 < 0x0C) // short to ~
	{
		if(fc1%16 < 0x0C) // s2s
	  {
		  if(mac_hdr.frame_control[0] == 0x41) //no ack
			{
				if(mac_hdr.dst_addr.addr_short == 0xFFFF)
				{
					test_result2.th2dut.g = 1;
					TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
				}
				else
				{
					test_result2.th2dut.a = 1;
					TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
				}
			}
			else
			{
				test_result2.th2dut.b = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
			}	
	  }
		else                // s2ex
		{
			test_result2.th2dut.c = 1;
			TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
		}
	}
	else              // ex to ~
	{
		if(fc1%16 < 0x0C) // ex2s
	  {
		  if(mac_hdr.dst_addr.addr_short == 0xFFFF)
			{
			  test_result2.th2dut.f = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
			}
			else
			{
				test_result2.th2dut.d = 1;
				TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
			}
	  }
		else                // ex2ex
		{
			test_result2.th2dut.e = 1;
			TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication success. test_step %hd", (FMT__H, dut_test_step));
		}
	}

  if (dut_test_step < TEST_STEP_FINISHED)
  {
    if (dut_test_step >= TH2DUT_SHORT2BROADCAST)
    {
      test_schedule_mcps_data_request(param, 0);
      param = 0;
    }
  }

  if (param != 0)
  {
    zb_buf_free(param);
  }
}

static void test_set_short_addr_none_conf(zb_uint8_t param)
{
  if (dut_test_step == DUT2TH_SHORT2EXT_INDIRECT_ACK ||
      dut_test_step == DUT2TH_EXT2SHORT_INDIRECT_ACK)
  {
    test_schedule_mcps_data_request(param, 0);
  }
  else
  {
    //TRACE_MSG(TRACE_ERROR, "Unexpected state %hd param %hd", (FMT__H_H, dut_test_step, param));
    ZB_ASSERT(0);
    zb_buf_free(param);
  }
}

static void test_set_short_addr_restore_conf(zb_uint8_t param)
{
  if (dut_test_step == DUT2TH_EXT2SHORT_INDIRECT_ACK)
  {
    ZB_SCHEDULE_CALLBACK(test_set_short_addr_none, param);
  }
  else if (dut_test_step == DUT2TH_EXT2EXT_INDIRECT_ACK)
  {
    test_schedule_mcps_data_request(param, 0);
  }
  else
  {
    //TRACE_MSG(TRACE_ERROR, "Unexpected state %hd param %hd", (FMT__H_H, dut_test_step, param));
    ZB_ASSERT(0);
    zb_buf_free(param);
  }
}

void test_final_result2(void)
{
	if (test_result2.th2dut.a == ZB_TRUE &&
		  test_result2.th2dut.b == ZB_TRUE &&
	    test_result2.th2dut.c == ZB_TRUE &&
	    test_result2.th2dut.d == ZB_TRUE &&
	    test_result2.th2dut.e == ZB_TRUE &&
	    test_result2.th2dut.f == ZB_TRUE &&
	    test_result2.th2dut.g == ZB_TRUE &&
	    test_result2.dut2th.h == ZB_TRUE &&
	    test_result2.dut2th.i == ZB_TRUE &&
	    test_result2.dut2th.j == ZB_TRUE &&
	    test_result2.dut2th.k == ZB_TRUE &&
	    test_result2.dut2th.l == ZB_TRUE &&
	    test_result2.dut2th.m == ZB_TRUE)
	{
		zb_osif_led_on(0);
		TRACE_MSG(TRACE_APP1, "Data02 completely success.", (FMT__0));
  }
}

ZB_MLME_PURGE_CONFIRM(zb_uint8_t param)
{
	zb_mlme_purge_confirm_t *purge_conf = ZB_BUF_GET_PARAM(param, zb_mlme_purge_confirm_t);
	if(zb_buf_get_status(param) == 0x00)
	{
	  test_result2.dut2th.m = 1;
		TRACE_MSG(TRACE_APP1, "zb_mlme_purge_confirm success. test_step %hd", (FMT__H, dut_test_step));
		test_final_result2();
	}
  zb_mlme_purge_confirm_t *conf = ZB_BUF_GET_PARAM(param, zb_mlme_purge_confirm_t);	
  //TRACE_MSG(TRACE_APP1, "zb_mlme_purge_confirm param %hd handle 0x%hx status %hd",
            //(FMT__H_H_H, param, conf->msdu_handle, zb_buf_get_status(param)));
  zb_buf_free(param);
}

/*! @} */
