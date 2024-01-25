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
/* PURPOSE: TH ZC1 (sending malformed beacons, open network that not allows join)
*/

#define ZB_TEST_NAME CN_NSA_TC_03_THC1
#define ZB_TRACE_FILE_ID 41040
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
  NO_CHILD_CAP_IN_BEACON,
  NO_ASSOC_RESP_1,
  NO_ASSOC_RESP_2,
  DO_NOT_SEND_NWK_KEY,
  DO_NOT_RESPOND_ON_REQUEST_KEY,
  TEST_STEPS_COUNT
};

extern void zb_nwk_update_beacon_payload_conf(zb_uint8_t param);

static void save_test_state();
static void read_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t write_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t th_nvram_get_app_data_size_cb();
static void set_default_beacon_pl(zb_uint8_t *set_req, zb_uint8_t *beacon_pl);
static void th_test_fsm(zb_uint8_t unused);
static void th_set_no_child_cap_in_beacon_pl(zb_uint8_t param);
static void dev_annce_cb(zb_zdo_device_annce_t *da);


static int s_step_idx;


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thc1");

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thc1);

  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  zb_secur_setup_nwk_key(g_nwk_key, 0);
  ZB_AIB().aps_use_nvram = 1;

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_THC1_PRIMARY_CHANNEL;
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


/* taken from nwk_cr_formation.c  */
static void set_default_beacon_pl(zb_uint8_t *set_req, zb_uint8_t *beacon_pl)
{
  zb_mlme_set_request_t *req = (zb_mlme_set_request_t*) set_req;
  zb_mac_beacon_payload_t *pl = (zb_mac_beacon_payload_t*) beacon_pl;

  TRACE_MSG(TRACE_APS3, ">>set_default_beacon_pl", (FMT__0));

  req->pib_attr = ZB_PIB_ATTRIBUTE_BEACON_PAYLOAD;
  req->pib_index = 0;
  req->pib_length = sizeof(zb_mac_beacon_payload_t);
  req->confirm_cb = zb_nwk_update_beacon_payload_conf;

  ZB_BZERO(pl, sizeof(zb_mac_beacon_payload_t));

  pl->stack_profile = ZB_STACK_PROFILE;
  pl->protocol_version = ZB_PROTOCOL_VERSION;
  if (ZB_NIB_DEPTH() > 0xf)
  {
    pl->device_depth = 0xf;
  }
  else
  {
    pl->device_depth = ZB_NIB_DEPTH();
  }
  ZB_EXTPANID_COPY(pl->extended_panid, ZB_NIB_EXT_PAN_ID());
  ZB_MEMSET(&pl->txoffset, -1, sizeof(pl->txoffset));

  if ((ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num))
  {
    pl->router_capacity = 1;
    pl->end_device_capacity = 1;
    TRACE_MSG(TRACE_APS3, "set capacity to 1", (FMT__0));
  }
  else
  {
    /* ZB_PIBCACHE_ASSOCIATION_PERMIT() =
     * (ZB_NIB().max_children > ZB_NIB().router_child_num + ZB_NIB().ed_child_num);
     */
    pl->router_capacity = ZB_PIBCACHE_ASSOCIATION_PERMIT();
    pl->end_device_capacity = ZB_PIBCACHE_ASSOCIATION_PERMIT();
    TRACE_MSG(TRACE_NWK3, "set capacity to %hd", (FMT__H, ZB_PIBCACHE_ASSOCIATION_PERMIT()));
  }
  pl->nwk_update_id = ZB_NIB_UPDATE_ID();

  TRACE_MSG(TRACE_APS3, "<<set_default_beacon_pl", (FMT__0));
}


static void th_test_fsm(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  TRACE_MSG(TRACE_APS1, ">>th_test_fsm: step = %d", (FMT__D, s_step_idx));
  switch (s_step_idx)
  {
    case NO_CHILD_CAP_IN_BEACON:
      TRACE_MSG(TRACE_APS2, "Set child capacity to zero", (FMT__0));
      ZB_GET_OUT_BUF_DELAYED(th_set_no_child_cap_in_beacon_pl);
      break;

    case NO_ASSOC_RESP_1:
    case NO_ASSOC_RESP_2:
      TRACE_MSG(TRACE_APS2, "Disable association response", (FMT__0));
      ZB_CERT_HACKS().disable_association_response = 1;
      break;

    case DO_NOT_SEND_NWK_KEY:
      TRACE_MSG(TRACE_APS2, "Disable nwk key sending to joiner", (FMT__0));
      ZB_SET_NIB_SECURITY_LEVEL(0);
      break;

    case DO_NOT_RESPOND_ON_REQUEST_KEY:
      TRACE_MSG(TRACE_APS2,
                "Disable tclk key sending to joiner (during link key exchange)",
                (FMT__0));
      ZB_AIB().tcpolicy.allow_tc_link_key_requests = 0;
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "TEST ERROR: unknown th fsm state", (FMT__0));
      break;
  }

  save_test_state();

  TRACE_MSG(TRACE_APS1, "<<th_test_fsm", (FMT__0));
}


static void th_set_no_child_cap_in_beacon_pl(zb_uint8_t param)
{
  zb_mlme_set_request_t *req;
  zb_mac_beacon_payload_t *pl;
  size_t used_buf_space;

  TRACE_MSG(TRACE_APS2, ">>th_set_beacon_payload", (FMT__0));

  used_buf_space = sizeof(zb_mlme_set_request_t) + sizeof(zb_mac_beacon_payload_t);
  ZB_BUF_INITIAL_ALLOC(ZB_BUF_FROM_REF(param), used_buf_space, req);
  ZB_BZERO(req, used_buf_space);

  pl = (zb_mac_beacon_payload_t *)(req+1);
  set_default_beacon_pl((zb_uint8_t*) req, (zb_uint8_t*) pl);

  pl->router_capacity = 0;
  pl->end_device_capacity = 0;

  ZB_SCHEDULE_CALLBACK(zb_mlme_set_request, param);

  TRACE_MSG(TRACE_APS2, "<<th_set_beacon_payload", (FMT__0));
}


static void dev_annce_cb(zb_zdo_device_annce_t *da)
{
  if (ZB_IEEE_ADDR_CMP(g_ieee_addr_thr1, da->ieee_addr) == ZB_TRUE)
  {
    TRACE_MSG(TRACE_ZDO2, "TH ZR1 joined to TH ZC1.", (FMT__0));
    ZB_SCHEDULE_ALARM(th_test_fsm, 0, THC1_START_TEST_TIMEOUT);
  }
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
        ZB_TCPOL().trust_center_node_join_timeout = 30 * ZB_TIME_ONE_SECOND;
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
        break;

      case ZB_BDB_SIGNAL_STEERING:
        if (s_step_idx < NO_ASSOC_RESP_1)
        {
          zb_zdo_register_device_annce_cb(dev_annce_cb);
        }
        else
        {
          ZB_SCHEDULE_CALLBACK(th_test_fsm, 0);
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
