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
/* PURPOSE: TP/154/MAC/DATA-04 DUT RFD1 (case 1)
MAC-only build
*/

#define ZB_TEST_NAME TP_154_MAC_DATA_04_RFD_DUT_RFD1
#define ZB_TRACE_FILE_ID 57567
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "../common/zb_cert_test_globals.h"
#include "tp_154_mac_data_04_common.h"
#include "zb_mac_globals.h"
#include "zb_osif.h"

#ifndef ZB_MULTI_TEST
#define USE_ZB_MLME_RESET_CONFIRM
#define USE_ZB_MLME_ASSOCIATE_CONFIRM
#define USE_ZB_MCPS_DATA_INDICATION
#define USE_ZB_MLME_POLL_CONFIRM
#include "zb_mac_only_stubs.h"
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

static zb_ieee_addr_t g_ieee_addr_rfd1 = TEST_RFD1_MAC_ADDRESS;

static void test_started_cb(zb_uint8_t param);
static void test_mlme_reset_request(zb_uint8_t param);
static void test_set_ieee_addr(zb_uint8_t param);
static void test_associate_request(zb_uint8_t param);
static void test_poll_alarm(zb_uint8_t param);
extern zb_step_status_t step_state;

MAIN()
{
  ARGV_UNUSED;

  ZB_INIT("tp_154_mac_data_04_rfd_dut_rfd1");

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
		  ZB_SCHEDULE_CALLBACK(test_associate_request, param);	
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

static void test_set_ieee_addr(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;

  //TRACE_MSG(TRACE_APP1, "MLME-SET.request() mac IEEE addr", (FMT__0));

  req = zb_buf_initial_alloc(param, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));
  ZB_BZERO(req, sizeof(zb_mlme_set_request_t) + sizeof(zb_ieee_addr_t));

  req->pib_attr = ZB_PIB_ATTRIBUTE_EXTEND_ADDRESS;
  req->pib_length = sizeof(zb_ieee_addr_t);
  ZB_MEMCPY((zb_uint8_t *)(req+1), g_ieee_addr_rfd1, sizeof(zb_ieee_addr_t));

#if CHANNEL_CHANGE
  req->confirm_cb_u.cb = test_get_command;
#else
	req->confirm_cb_u.cb = test_associate_request;
#endif
  zb_mlme_set_request(param);
}

static void test_associate_request(zb_uint8_t param)
{
  zb_uint16_t addr = TEST_FFD0_SHORT_ADDRESS;
  ZB_MLME_BUILD_ASSOCIATE_REQUEST(param,
                                  TEST_PAGE,
                                  TEST_CHANNEL,
                                  TEST_PAN_ID,
                                  ZB_ADDR_16BIT_DEV_OR_BROADCAST,
                                  &addr,
                                  TEST_ASSOCIATION_CAP_INFO);
  ZB_SCHEDULE_CALLBACK(zb_mlme_associate_request, param);
}



static void test_poll_alarm(zb_uint8_t param)
{
  zb_mlme_poll_request_t *req = ZB_BUF_GET_PARAM(param, zb_mlme_poll_request_t);
  TRACE_MSG(TRACE_APP1, "Next: poll_alarm", (FMT__0));

  //TRACE_MSG(TRACE_APP1, "DUT2TH_SHORT2SHORT_INDIRECT_ACK", (FMT__0));
  req->coord_addr_mode = ZB_ADDR_16BIT_DEV_OR_BROADCAST;
  req->coord_addr.addr_short = TEST_FFD0_SHORT_ADDRESS;
  req->coord_pan_id = TEST_PAN_ID;
	
	#if UART_CONTROL
  test_step_register(zb_mlme_poll_request, param, 0);
  test_control_start(ZB_TEST_CONTROL_UART, 0);
  #else
  ZB_SCHEDULE_CALLBACK(zb_mlme_poll_request, param);
  #endif
}

static void test_schedule_poll_alarm(zb_uint8_t param, zb_uint32_t tmo)
{
#ifdef MAC_TEST_ADD_EXT_DELAY
  ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000));
#else
  if (!tmo)
  {
    ZB_SCHEDULE_CALLBACK(test_poll_alarm, param);
  }
  else
  {
    ZB_SCHEDULE_ALARM(test_poll_alarm, param, ZB_MILLISECONDS_TO_BEACON_INTERVAL(tmo));
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

ZB_MLME_ASSOCIATE_CONFIRM(zb_uint8_t param)
{
  //TRACE_MSG(TRACE_APP1, "MLME-ASSOCIATE.confirm()", (FMT__0));
  test_schedule_poll_alarm(param, 100);
}


ZB_MCPS_DATA_INDICATION(zb_uint8_t param)
{
  zb_osif_led_on(0); // blue  
	TRACE_MSG(TRACE_APP1, "MCPS-DATA.indication()", (FMT__0));
	TRACE_MSG(TRACE_APP1, "Data04_DUT_RFD1 success", (FMT__0));
  ZB_SCHEDULE_ALARM_CANCEL(test_poll_alarm, ZB_ALARM_ANY_PARAM);
  zb_buf_free(param);
}

ZB_MLME_POLL_CONFIRM(zb_uint8_t param)
{
  test_schedule_poll_alarm(param, 1000);
}


/*! @} */
