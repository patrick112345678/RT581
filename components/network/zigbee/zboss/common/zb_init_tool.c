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
/* PURPOSE: ZigBee tool initialization
*/



#define ZB_TRACE_FILE_ID 43
#include "zb_common.h"

#include "zb_bufpool.h"
#include "zb_ringbuffer.h"
#include "zb_scheduler.h"
#include "zb_mac_transport.h"

/*! \addtogroup ZB_BASE */
/*! @{ */

/**
   Globals data structure implementation - let it be here.
 */
zb_globals_t g_zb;

void zb_init_tool(void)
{
  ZB_MEMSET(ZG, 0, sizeof(zb_globals_t));

  ZB_PLATFORM_INIT();

  zb_sched_init();
  zb_init_buffers();

#ifdef ZB_MACSPLIT_HOST
  zb_mac_init();
  zb_mac_transport_init();
#endif
}

void zb_nwk_unlock_in(zb_uint8_t param)
{
  ZVUNUSED(param);
}

void zb_check_oom_status(zb_uint8_t param)
{
  ZVUNUSED(param);
}

zb_ret_t zb_nwk_secure_frame(zb_bufid_t src, zb_uint_t mac_hdr_size, zb_bufid_t dst)
 {
  ZVUNUSED(src);
  ZVUNUSED(dst);
  ZVUNUSED(mac_hdr_size);
  return 0;
}

zb_ret_t zb_aps_secure_frame(zb_bufid_t src, zb_uint_t mac_hdr_size, zb_bufid_t dst, zb_bool_t is_tunnel)
{
  ZVUNUSED(src);
  ZVUNUSED(dst);
  ZVUNUSED(is_tunnel);
  ZVUNUSED(mac_hdr_size);
  return 0;
}

#ifndef USE_ZB_MLME_RESET_CONFIRM
void zb_mlme_reset_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif


#ifndef USE_ZB_MLME_SET_CONFIRM
void zb_mlme_set_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif


#ifndef USE_ZB_MLME_START_CONFIRM
void zb_mlme_start_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif


#ifndef USE_ZB_MCPS_DATA_CONFIRM
void zb_mcps_data_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_BEACON_NOTIFY_INDICATION
void zb_mlme_beacon_notify_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_POLL_CONFIRM
void zb_mlme_poll_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_ORPHAN_INDICATION
void zb_mlme_orphan_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_ASSOCIATE_CONFIRM
void zb_mlme_associate_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_ASSOCIATE_INDICATION
void zb_mlme_associate_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_SCAN_CONFIRM
void zb_mlme_scan_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MCPS_DATA_INDICATION
void zb_mcps_data_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MCPS_POLL_INDICATION
void zb_mcps_poll_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif


#ifndef USE_ZB_MLME_COMM_STATUS_INDICATION
void zb_mlme_comm_status_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif


#ifndef USE_ZB_MLME_PURGE_CONFIRM
void zb_mlme_purge_confirm(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MLME_DUTY_CYCLE_MODE_INDICATION
void zb_mlme_duty_cycle_mode_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

#ifndef USE_ZB_MCPS_PURGE_INDIRECT_QUEUE_CONFIRM
void zb_mcps_purge_indirect_queue_confirm(zb_uint8_t param)
{
  ZVUNUSED(param);
}
#endif

#ifndef USE_ZB_GP_MCPS_DATA_INDICATION
void zb_gp_mcps_data_indication(zb_uint8_t param)
{
  zb_buf_free(param);
}
#endif

/*! @} */
