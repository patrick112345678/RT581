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
/* PURPOSE: ZigBee stack initialization: NWK and lower
*/



#define ZB_TRACE_FILE_ID 2147
#include "zb_common.h"

#include "zb_bufpool.h"
#include "zb_ringbuffer.h"
#include "zb_scheduler.h"
#include "zb_mac_transport.h"
#include "zb_aps.h"

/*! \addtogroup ZB_BASE */
/*! @{ */





/**
   Globals data structure implementation - let it be here.

   FIXME: maybe, put it into separate .c file?
 */
zb_globals_t g_zb;
zb_64bit_addr_t g_zero_addr={0,0,0,0,0,0,0,0};

void zb_init_wo_aps(zb_char_t *trace_comment, zb_char_t *rx_pipe, zb_char_t *tx_pipe)
{
  ZVUNUSED(trace_comment);
  ZVUNUSED(rx_pipe);
  ZVUNUSED(tx_pipe);
  /* first of all, prevent our seset by watchdog during init */    
/*  Address: 0x93 
	Bits Name Description Reset Value R/W 
	7 COBKEN Enable common bank scheme 0 R/W 
	6-3 Reserved - - - 
	2-0 PBS Program bank selection bit 0x0 R/W 
 
  COBKEN : 
	1: Enable common-bank scheme that uses a common memory which located from 0x0000 to 0x7fff. 
	Furthermore, data located between 0x8000 and 0xffff can be placed in the different bank.  
	0: Keep Mentor M8051EW external memory (MEXT) setting.  
*/

  /* fill all our RAM by 0 - do not init it on per-component basis */
  ZB_MEMSET(ZG, 0, sizeof(zb_globals_t));

  ZB_START_DEVICE();

  TRACE_INIT(trace_comment);

  /* special trick for ns build run on 8051 simulator: get node number from the
   * rx pipe name  */
  /* set defaults, then update it from nvram */
  zb_ib_set_defaults(rx_pipe);
  zb_ib_load();

  zb_sched_init();
  zb_init_buffers();

#ifndef ZB8051
  zb_mac_transport_init(rx_pipe, tx_pipe);
#endif

  zb_mac_init();

  zb_nwk_init();


#if defined ZB_NVRAM_WRITE_CFG && defined ZB_USE_NVRAM && defined C8051F120
/* Write config to nvram. Think there's no any reason to invoke this second time*/
/*
zb_uint8_t aps_designated_coord
zb_uint8_t aps_use_insecure_join
zb_uint8_t aps_use_extended_pan_id
zb_ieee_addr_t mac_extended_address
*/
{
zb_uint8_t addr[8]={0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x0B};
zb_write_nvram_config(0, 1, 1, addr);
}
#endif





#ifdef ZB_USE_NVRAM
/*  zb_config_from_nvram();
  zb_read_up_counter();
  zb_read_security_key();
  zb_read_formdesc_data();*/
#endif


}


/*! @} */
