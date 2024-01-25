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
/*  PURPOSE: Console monitor
*/

#define ZB_TRACE_FILE_ID 41707
#include "zboss_api.h"
#include "zb_common.h"
#include "zb_bufpool.h"
#include "zb_osif.h"
#include "zb_time.h"
#include <stdio.h>
#include <string.h>

#include "zb_cert_test_globals.h"
#include "zb_console_monitor.h"
#if defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT
#include "nrf_log_ctrl.h"
#include "ble_zigbee_cert_uart.h"
#endif /* defined ZB_NRF_52 && defined SOFTDEVICE_PRESENT */

#if defined ZB_NRF_52 && defined ZB_NRF_TRACE
#define CONSOLE_MONITOR_SLEEP_TIMEOUT 4000000
#else
#define CONSOLE_MONITOR_SLEEP_TIMEOUT 1000000
#endif /* defined ZB_NRF_52 && defined ZB_NRF_TRACE */

#define API_WAIT_TIMEOUT_MS -1
#define ZB_MAC_TRANSPORT_SIGNATURE_SIZE 2
#define ZB_CONSOLE_MONITOR_INVITATION '$'
#define ZB_CONSOLE_MONITOR_INVITATION_SIZE 1
#define ZB_CONSOLE_MONITOR_MAX_PAYLOAD_SIZE 256

static zb_console_monitor_ctx_t gs_console_ctx;
static void zb_console_monitor_rx_handler(zb_uint8_t symbol);
zb_step_status_t step_state;

char uart_command[100]; 
zb_uint8_t symbol_pos = 0;

void zb_console_monitor_sleep(zb_time_t timeout)
{
  /* CR: 12/16/2016: CR:MINOR Are you 146% sure timer is not started already?
     Maybe, check for it? */
  /* VP: 12/19/2016: add check to prevent timer double start */
  if (!ZB_CHECK_TIMER_IS_ON())
  {
    zb_timer_start(ZB_MILLISECONDS_TO_BEACON_INTERVAL(ZB_USECS_TO_MILLISECONDS(timeout)));
  }
  /* Block here */
  osif_sleep_using_transc_timer(timeout);
}

/* Non-blocking function */
void zb_console_monitor_init()
{
  /* Minimal ZBOSS init */
  zb_globals_init();
#if defined UNIX
  zb_mac_transport_init();
#endif
  ZB_PLATFORM_MULTI_TEST_INIT();
  /* HACK: Disable timer stop - have sync delays below! */
  zb_timer_disable_stop();
  TRACE_INIT("monitor");
  ZB_ENABLE_ALL_INTER();

  zb_console_monitor_sleep(2000000); /* sleep 2 seconds */

  zb_osif_set_uart_byte_received_cb(zb_console_monitor_rx_handler);

  ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));
}

void zb_console_monitor_deinit()
{
  /* HACK: Enable timer stop back! */
  zb_timer_enable_stop();
  ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));
}

static void zb_console_monitor_send_invintation()
{
  zb_console_monitor_pkt_t monitor_pkt;
  zb_time_t tm;
  /* Fill the packet header */
  monitor_pkt.hdr.sig[0] = 0xde;
  monitor_pkt.hdr.sig[1] = 0xad;
  monitor_pkt.hdr.h.len = sizeof(zb_console_monitor_pkt_t) - ZB_MAC_TRANSPORT_SIGNATURE_SIZE;
  monitor_pkt.hdr.h.type = ZB_MAC_TRANSPORT_TYPE_CONSOLE;
  tm = ZB_TIMER_GET();
  ZB_HTOLE16((zb_uint8_t*)&monitor_pkt.hdr.h.time, (zb_uint8_t*)&tm);

  /* Fill the payload */
  monitor_pkt.data.payload[0] = ZB_CONSOLE_MONITOR_INVINTATION;

  /* OK, send data now */
  zb_osif_serial_put_bytes((zb_uint8_t *)&monitor_pkt, sizeof(zb_console_monitor_pkt_t));

#if defined ZB_NRF_52
  ZB_OSIF_SERIAL_FLUSH();
#endif /* ZB_NRF_52 */
}

static zb_bool_t is_tmo_expired(zb_time_t t0, zb_time_t tmo)
{
  return (ZB_TIME_GE(osif_transceiver_time_get(), ZB_TIME_ADD(t0, tmo)));
}

/* Blocking function */
zb_uint8_t zb_console_monitor_get_cmd(zb_uint8_t *buffer, zb_uint8_t max_lenght)
{
  gs_console_ctx.buffer = buffer;
  gs_console_ctx.max_lenght = max_lenght;

  ZB_BZERO(gs_console_ctx.buffer, gs_console_ctx.max_lenght);
  gs_console_ctx.pos = 0;
  gs_console_ctx.cmd_recvd = ZB_FALSE;

  zb_console_monitor_sleep(ZB_MULTITEST_CONSOLE_SLEEP_TIMEOUT);

  /* Ready to receive  */
  gs_console_ctx.rx_ready = ZB_TRUE;

  while (!gs_console_ctx.cmd_recvd)
  {
    /* zb_console_monitor_sleep(CONSOLE_MONITOR_SLEEP_TIMEOUT); */

    //JJ disable send invintation
    //zb_console_monitor_send_invintation();

#if defined UNIX
    ns_api_wait(&ZB_IOCTX().api_ctx, API_WAIT_TIMEOUT_MS);
#else /* defined UNIX */
    {
      zb_time_t t0;

      t0 = osif_transceiver_time_get();
      /* Block until sleep tmo expired or received cmd */
      while (!(is_tmo_expired(t0, ZB_MULTITEST_CONSOLE_SLEEP_TIMEOUT) ||
               gs_console_ctx.cmd_recvd))
      {
        /* Do nothing */
      }
    }
#endif

  }

  return gs_console_ctx.status;
}

static void zb_console_monitor_rx_handler(zb_uint8_t symbol)
{
  if (gs_console_ctx.rx_ready)
  {
    zb_ret_t status = RET_EXIT;

    if (gs_console_ctx.pos < gs_console_ctx.max_lenght)
    {
      if ('\r' == symbol)
      {
        /* Ignore this symbol */
      }
      else if ('\n' == symbol)
      {
        /* NULL-terminator for C string */
        *(zb_uint8_t *)(gs_console_ctx.buffer + gs_console_ctx.pos) = 0;
        status = RET_OK;
      }
      else
      {
        *(zb_uint8_t *)(gs_console_ctx.buffer + gs_console_ctx.pos) = symbol;
        gs_console_ctx.pos++;

        /* Check for overflow */
        if (gs_console_ctx.pos == gs_console_ctx.max_lenght)
        {
          status = RET_OVERFLOW;
        }
      }
    }
    else
    {
      status = RET_OVERFLOW;
    }

    if (RET_EXIT != status)
    {
      gs_console_ctx.status = status;
      gs_console_ctx.rx_ready = ZB_FALSE;
      gs_console_ctx.cmd_recvd = ZB_TRUE;
    }
  }
}

void zb_console_monitor_rx_next_step(zb_uint8_t symbol)
{
  ZB_BZERO(&gs_console_ctx, sizeof(gs_console_ctx));

  gs_console_ctx.max_lenght = 1;
  if (1)//(gs_console_ctx.rx_ready)
  {
    zb_ret_t status = RET_EXIT;

    if (gs_console_ctx.pos < gs_console_ctx.max_lenght)
    {
      //printf("get symbol: %c.", symbol);
			if ('\n' != symbol)
			{
				uart_command[symbol_pos] = symbol;
			  symbol_pos++;
			}		
			if ('\r' == symbol)
      {
        /* start the next step */				
        status = RET_OK;
      }
      else if ('r' == symbol) 
      {
        __NVIC_SystemReset();
      }
      else
      {
				/* for MAC/PHY test */
				switch(symbol){
					case 'u':
            step_state = CHANNEL_INCREASING;
					  break;
					case 'd':
            step_state = CHANNEL_DECREASING;
					  break;
					case 't':
            step_state = TRANSMITTING;
					  break;
					case 'y':
            step_state = TRANSMITTING_1000;
					  break;
					case 'p':
            step_state = TESTING_AND_PRINTING;
					  break;
				}
        status = RET_OVERFLOW;
      }
    }
    else
    {
      status = RET_OVERFLOW;
    }

    if (RET_OK == status & !test_control_ctx.step_in_progress)
    {			
      test_control_ctx.step_in_progress = ZB_TRUE;
      test_control_handle_steps(0);
			
			symbol_pos = 0;
    }
  }
}

void zb_console_monitor_send_data(zb_uint8_t* buffer, zb_uint8_t size) {
  zb_console_monitor_pkt_t monitor_pkt;
  zb_time_t time;
  zb_uindex_t i;

  ZB_ASSERT (size <= ZB_CONSOLE_MONITOR_MAX_PAYLOAD_SIZE);

  /* Fill the packet header */
  monitor_pkt.hdr.sig[0] = 0xde;
  monitor_pkt.hdr.sig[1] = 0xad;
  monitor_pkt.hdr.h.len = sizeof(zb_console_monitor_pkt_t) -
    ZB_MAC_TRANSPORT_SIGNATURE_SIZE + size - 1;
  monitor_pkt.hdr.h.type = ZB_MAC_TRANSPORT_TYPE_CONSOLE;
  time = ZB_TIMER_GET();
  ZB_HTOLE16((zb_uint8_t*)&monitor_pkt.hdr.h.time, (zb_uint8_t*)&time);

  /* Fill the payload */
  for (i = 0; i < size; i++) {
    monitor_pkt.data.payload[i] = buffer[i];
  }

  /* OK, send data now */
  zb_osif_serial_put_bytes((zb_uint8_t *)&monitor_pkt, monitor_pkt.hdr.h.len + ZB_MAC_TRANSPORT_SIGNATURE_SIZE);
}
