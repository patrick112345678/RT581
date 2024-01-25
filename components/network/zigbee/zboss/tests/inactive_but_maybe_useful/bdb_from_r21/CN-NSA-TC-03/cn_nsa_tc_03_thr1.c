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
/* PURPOSE: TH ZR1 (sending malformed beacons, not respond on association requests)
*/

#define ZB_TEST_NAME CN_NSA_TC_03_THR1
#define ZB_TRACE_FILE_ID 41038
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "zb_mac.h"
#include "cn_nsa_tc_03_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error Define ZB_USE_NVRAM
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error Define ZB_CERTIFICATION_HACKS
#endif


enum th_test_step_e
{
  NORMAL_OPERATION,
  NO_ASSOC_RESP_1,
  NO_ASSOC_RESP_2,
  TEST_STEPS_COUNT
};

extern void zb_nwk_update_beacon_payload_conf(zb_uint8_t param);

static void save_test_state();
static void read_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t write_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t th_nvram_get_app_data_size_cb();
static void th_test_fsm(zb_uint8_t unused);


static int s_step_idx;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr1");

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr1);

  ZB_AIB().aps_use_nvram = 1;

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_THR1_PRIMARY_CHANNEL;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_THX_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;

  zb_nvram_register_app1_read_cb(read_th_app_data_cb);
  zb_nvram_register_app1_write_cb(write_th_app_data_cb, th_nvram_get_app_data_size_cb);

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



static void save_test_state()
{
  TRACE_MSG(TRACE_APS3, ">>save_test_state: saved value = %d", (FMT__D, s_step_idx + 1));
  zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
  TRACE_MSG(TRACE_APS3, "<<save_test_state", (FMT__0));
}


static void read_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  zb_ret_t ret;
  thx_nvram_app_dataset_t ds;

  if (payload_length == sizeof(ds))
  {
    ret = zb_osif_nvram_read(page, pos, (zb_uint8_t*)&ds, sizeof(ds));
    if (ret == RET_OK)
    {
      s_step_idx = ds.current_test_step;
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "read_th_app_data_cb: nvram read error %d", (FMT__D, ret));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "read_th_app_data_cb ds mismatch: got %d wants %d",
              (FMT__D_D, payload_length, sizeof(ds)));
  }
}


static zb_ret_t write_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos)
{
  zb_ret_t ret = RET_OK;
  thx_nvram_app_dataset_t ds;

  TRACE_MSG(TRACE_APS3, ">>write_th_app_data_cb", (FMT__0));

  ds.current_test_step = (zb_uint32_t) s_step_idx + 1;
  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APS3, "<<write_th_app_data_cb ret %d", (FMT__D, ret));
  return ret;
}


static zb_uint16_t th_nvram_get_app_data_size_cb()
{
  return sizeof(thx_nvram_app_dataset_t);
}


static void th_test_fsm(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APS1, ">>th_test_fsm: step = %d", (FMT__D, s_step_idx));
  switch (s_step_idx)
  {
    case NORMAL_OPERATION:
      TRACE_MSG(TRACE_APS2, "Do nothing (will be rebooted soon)", (FMT__0));
      break;

    case NO_ASSOC_RESP_1:
    case NO_ASSOC_RESP_2:
      TRACE_MSG(TRACE_APS2, "Disable association response", (FMT__0));
      ZB_CERT_HACKS().disable_association_response = 1;
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "TEST ERROR: unknown th fsm state", (FMT__0));
      break;
  }

  save_test_state();

  TRACE_MSG(TRACE_APS1, "<<th_test_fsm", (FMT__0));
}


ZB_ZDO_STARTUP_COMPLETE(zb_uint8_t param)
{
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        TRACE_MSG(TRACE_APS1, "Device STARTED OK", (FMT__0));
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        ZB_SCHEDULE_CALLBACK(th_test_fsm, 0);
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
