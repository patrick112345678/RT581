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
/* PURPOSE: ZC echo application to deliberately distort transmitted packets
 *   Distortion stages:
 *     - Change Frame Counter field in NWK security header
 *     - Change Frame Counter field in APS security header
 *     - Change MAC Frame Type and NWK Key. Alternately for different packets
 *     - Attempt to flood a peer
 *     - "Rest" stage for ZBOSS stacks after flooding
 *     - Change the radio channel for simulate the disappearance of ZC
 *
 * ZB_CERTIFICATION_HACKS must be defined
 */
#define ZB_TRACE_FILE_ID 97

#define ZB_CERTIFICATION_HACKS

#include "zb_config.h"

#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "../zbs_feature_tests.h"
#include "zb_trace.h"
#ifdef ZB_CONFIGURABLE_MEM
#include "zb_mem_config_max.h"
#endif
#ifdef ZB_CERTIFICATION_HACKS
#include "mac_internal.h"
#endif
/* define SUBGIG to test ZC against ZED at subgig. Comment it out to test ZR. */
//#define SUBGIG

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

/*! \addtogroup ZB_TESTS */
/*! @{ */

#ifdef NCP_SDK
static zb_ieee_addr_t g_zc_addr = TEST_ZC_ADDR;
static zb_ieee_addr_t g_ze_addr = TEST_ZE_ADDR;
static zb_ieee_addr_t g_ncp_addr = TEST_NCP_ADDR;
static zb_uint8_t gs_nwk_key[16] = TEST_NWK_KEY;
static zb_uint8_t g_ic1[16+2] = TEST_IC;

#else
zb_ieee_addr_t     g_zc_addr = {0xde, 0xad, 0xf0, 0x0d, 0xde, 0xad, 0xf0, 0x0d};
#endif

static zb_uint_t         g_packets_rcvd;
static zb_uint_t         g_packets_sent;
static zb_uint16_t       g_remote_addr;

#define SUBGHZ_PAGE 1
#define SUBGHZ_CHANNEL 25

#ifdef FLOOD_MODE
#undef ZB_TEST_DATA_SIZE
#define ZB_TEST_DATA_SIZE 50
#endif

static void zc_send_data(zb_uint8_t param);
static void zc_get_buf_send_data(zb_uint8_t param);
void zc_send_data_loop(zb_uint8_t param);

#ifdef ZB_CERTIFICATION_HACKS

// #define NWK_SECUR_COUNTER_BREAKS_BY_ALARM

#define NWK_SECUR_KEY_BREAKS_NUM_MAX     3
#define APS_SECUR_KEY_BREAKS_NUM_MAX     2
#define MAC_FRAME_TYPE_BREAKS_NUM_MAX    4
#define NWK_SECUR_COUNTER_BREAKS_NUM_MAX 5
#define APS_SECUR_COUNTER_BREAKS_NUM_MAX 3 /* should turn 9 in the counter aps_fc_failure in py test 
                                            * (APS_SECUR_COUNTER_BREAKS_NUM_MAX * ZB_N_APS_MAX_FRAME_RETRIES) */
#define FLOOD_REST_PKTS_MAX 4
#define FLOOD_DURATION_SEC 5

typedef enum
{
  DIAGSTAGE_NWK_SEC_COUNTERS,              /* Change Frame Counter field in NWK security header */
  DIAGSTAGE_APS_SEC_COUNTERS_AND_KEY,      /* Change Frame Counter field in APS security header and distort APS encr key for different packets */
  DIAGSTAGE_NWK_SEC_KEY_AND_FRAME_TYPE,    /* Change MAC Frame Type and NWK Key. Alternately for different packets */
  DIAGSTAGE_FLOOD,                         /* Attempt to flood a peer */
  DIAGSTAGE_FLOOD_REST,                    /* "Rest" stage for ZBOSS stacks after flooding */
  DIAGSTAGE_CHANNEL_CHG,                   /* Change the radio channel for simulate the disappearance of ZC */
} diag_stage_t;

static diag_stage_t diag_stage = DIAGSTAGE_NWK_SEC_COUNTERS;
static zb_uint_t  g_resend_data_interval = 14;                 /* The time in seconds to recover the echo transmission
                                                                * if it stopped due to packets corruptions */
static zb_uint8_t nwk_secur_counter_breaks_num = 0;
static zb_uint8_t nwk_secur_key_breaks_num = 0;
static zb_uint8_t aps_secur_counter_breaks_num = 0;
static zb_uint8_t aps_secur_key_breaks_num = 0;
static zb_uint8_t mac_frame_type_breaks_num = 0;
static zb_uint8_t g_flood_rest_pkt_cntr = 0;
static zb_uint8_t g_flood_stop = 0;
void set_nwk_secur_counter_breaking_flag(zb_uint8_t param);
static void break_aps_secur_counter_cb(zb_uint32_t *out_sec_counter);

#define CHANGE_CHANNEL_DELAY 45
static zb_bool_t channel_changing_flag = ZB_TRUE;
static void change_channel(zb_uint8_t param);
static void schedule_channel_changing(zb_uint8_t param);
static void schedule_channel_changing_delayed(zb_uint8_t param);
static void zc_flood_pre_start(zb_uint8_t param);
static void zc_flood_start(zb_uint8_t param);
static void zc_flood_loop(zb_uint8_t param);
static void zc_flood_stop(zb_uint8_t param);

#endif /* ZB_CERTIFICATION_HACKS */

static void test_exit(zb_uint8_t param)
{
  ZB_ASSERT(param == 0);
  //zb_systest_finished();
}


static zb_bool_t exchange_finished()
{
#ifndef NCP_SDK
  return (g_packets_sent >= PACKETS_FROM_ZC_NR &&
          g_packets_rcvd >= PACKETS_FROM_ED_NR) ? ZB_TRUE : ZB_FALSE;
#else
  return ZB_FALSE;
#endif
}


static void test_device_annce_cb(zb_zdo_device_annce_t *da)
{
  zb_ieee_addr_t ze_addr = TEST_ZE_ADDR;
  zb_ieee_addr_t zr_addr = TEST_ZR_ADDR;

  TRACE_MSG(TRACE_APP3, ">>test_device_annce_cb()", (FMT__0));

  if (!ZB_IEEE_ADDR_CMP(da->ieee_addr, ze_addr) &&
      !ZB_IEEE_ADDR_CMP(da->ieee_addr, zr_addr))
  {
    //ZB_SYSTEST_EXIT_ERR("Unknown device has joined!", (FMT__0));
  }
  /* Use first joined device as destination for outgoing APS packets */
  if (g_remote_addr == 0)
  {
    g_remote_addr = da->nwk_addr;
    /* Assign the interval for resend data if there no echo_reply.
     * Depending on rx_on_when_idle of remote device */
    /*
    g_resend_data_interval = ZB_MAC_CAP_GET_RX_ON_WHEN_IDLE(da->capability) ? 5 : 14;
    */
#ifndef NCP_SDK
    if (PACKETS_FROM_ZC_NR != 0)
    {
      TRACE_MSG(TRACE_APP3, "schedule zc_send_data_loop(), g_remote_addr 0x%x",
                (FMT__D, g_remote_addr));
      ZB_SCHEDULE_CALLBACK(zc_send_data_loop, 0);
    }
#else
    {
      zb_bufid_t buf = zb_buf_get_out();
      ZB_ASSERT(buf);
#ifdef FLOOD_MODE
      ZB_SCHEDULE_CALLBACK(zc_send_data, buf);
#else
      ZB_SCHEDULE_ALARM(zc_send_data, buf, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
#endif
    }
#endif

#ifdef ZB_CERTIFICATION_HACKS
#ifdef START_TEST_FROM_FLOOD_STAGE
    /* Start the flood and schedule it's off */
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 4 - flood", (FMT__0));
    ZB_SCHEDULE_ALARM(zc_flood_pre_start, 0, 4 * ZB_TIME_ONE_SECOND);
    ZB_SCHEDULE_ALARM(zc_flood_stop, 0, (FLOOD_DURATION_SEC + 5) * ZB_TIME_ONE_SECOND);
    diag_stage = DIAGSTAGE_FLOOD; /* Go to stage 4 */
#else
    /* Run the first hack stage */
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 1 (schedule set_nwk_secur_counter_breaking_flag())", (FMT__0));
#ifdef NWK_SECUR_COUNTER_BREAKS_BY_ALARM
    ZB_SCHEDULE_ALARM(set_nwk_secur_counter_breaking_flag, 0, 4 * ZB_TIME_ONE_SECOND);
#endif
#endif
#endif /* ZB_CERTIFICATION_HACKS */
  }
}


MAIN()
{
  ARGV_UNUSED;

  ZB_SET_TRAF_DUMP_ON();
  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zbs_diagnostics_zc");

  zb_set_long_address(g_zc_addr);

#ifndef NCP_SDK
  zb_set_pan_id(0x1aaa);
#else
  zb_set_pan_id(TEST_PAN_ID);
#endif

  zb_set_nvram_erase_at_start(ZB_TRUE);
  zb_production_config_disable(ZB_TRUE);

#ifdef NCP_SDK
  zb_secur_setup_nwk_key(gs_nwk_key, 0);
  zb_set_installcode_policy(ZB_TRUE);
#endif

#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().break_nwk_fcf_counter = ZB_FALSE;
  ZB_CERT_HACKS().break_aps_fcf_counter = ZB_FALSE;
  ZB_CERT_HACKS().break_nwk_key = ZB_FALSE;
  ZB_CERT_HACKS().make_frame_not_valid = ZB_FALSE;
  /* Will be assigned later
  ZB_CERT_HACKS().secur_aps_counter_hack_cb = break_aps_secur_counter_cb;
  */
#endif /* ZB_CERTIFICATION_HACKS */

#ifndef SUBGIG
  /* ZB_AIB().aps_channel_mask = (1L << CHANNEL); */
  zb_aib_channel_page_list_set_2_4GHz_mask(1L << CHANNEL);
#else
  {
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);

    zb_channel_page_list_set_mask(channel_list, SUBGHZ_PAGE, 1<<SUBGHZ_CHANNEL);
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
  }
#endif

  zb_set_network_coordinator_role(1L << CHANNEL);

  zb_set_max_children(1);

  ZB_SET_TRAF_DUMP_ON();
  zb_zdo_register_device_annce_cb(test_device_annce_cb);

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

zb_uint8_t data_indication(zb_uint8_t param)
{
  zb_ushort_t i;
  zb_uint8_t *ptr;
  zb_apsde_data_indication_t *ind = ZB_BUF_GET_PARAM(param, zb_apsde_data_indication_t);

  if (ind->profileid == 0x0002)
  {
    ptr = (zb_uint8_t*)zb_buf_begin(param);

    TRACE_MSG(TRACE_APP3, "apsde_data_indication: packet %p %d len %d status 0x%x",
              (FMT__P_H_D_D,
               ptr, param, (int)zb_buf_len(param), zb_buf_get_status(param)));


    for (i = 0 ; i < zb_buf_len(param) ; ++i)
    {
      TRACE_MSG(TRACE_APP3, "%x (%c)", (FMT__D_C, (int)ptr[i], ptr[i]));
      if (ptr[i] != i % 32 + '0')
      {
        /*ZB_SYSTEST_EXIT_ERR("Bad data %hx (%c) wants %hx (%c)",
          (FMT__H_C_H_C, ptr[i], ptr[i],
          (zb_ushort_t)(i % 32 + '0'), (char)(i % 32 + '0')));*/
      }
    }
    g_packets_rcvd++;
    if (exchange_finished())
    {
      ZB_SCHEDULE_ALARM(test_exit, 0, 1*ZB_TIME_ONE_SECOND);
    }

    ZB_SCHEDULE_ALARM_CANCEL(zc_send_data, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_ALARM_CANCEL(zc_get_buf_send_data, ZB_ALARM_ALL_CB);
#ifndef NCP_SDK
    /* zc_send_data(asdu, g_remote_addr); */
    ZB_SCHEDULE_ALARM(zc_send_data, param, ZB_TIME_ONE_SECOND * 5);
#else
#ifndef FLOOD_MODE
    /* param is the input buffer - be free! */
    zb_buf_free(param);
#ifdef ZB_CERTIFICATION_HACKS
    if (diag_stage != DIAGSTAGE_FLOOD)
#endif
    {
      ZB_SCHEDULE_ALARM(zc_get_buf_send_data, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
    }
#else /* FLOOD_MODE */
    ZB_SCHEDULE_CALLBACK(zc_send_data, param);
    param = zb_buf_get_out();
    if (param)
    {
      ZB_SCHEDULE_CALLBACK(zc_send_data, param);
    }
    param = zb_buf_get_out();
    if (param)
    {
      ZB_SCHEDULE_CALLBACK(zc_send_data, param);
    }
#endif /* FLOOD_MODE */
#endif /* NCP_SDK */
    return ZB_TRUE;               /* processed */
  }

  return ZB_FALSE;
}


void zb_zdo_startup_complete(zb_uint8_t param)
{
  zb_zdo_app_signal_hdr_t *sg_p = NULL;
  zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sg_p);

  TRACE_MSG(TRACE_APP1, "zboss_signal_handler: status %hd signal %hd",
            (FMT__H_H, ZB_GET_APP_SIGNAL_STATUS(param), sig));

  if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
  {
    switch(sig)
    {
      case ZB_ZDO_SIGNAL_DEFAULT_START:
      case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
      case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        zb_aib_tcpol_set_update_trust_center_link_keys_required(ZB_FALSE);
        bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);

#ifdef NCP_SDK
        zb_secur_ic_add(g_ncp_addr, ZB_IC_TYPE_128, g_ic1, NULL);
        zb_secur_ic_add(g_ze_addr, ZB_IC_TYPE_128, g_ic1, NULL);
#endif
        TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
        //zb_systest_started();
        zb_af_set_data_indication(data_indication);
        break;
      default:
        break;
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    /*ZB_SYSTEST_EXIT_ERR("Device start FAILED status %d",
      (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));*/
  }

  zb_buf_free(param);
}

#ifdef ZB_CERTIFICATION_HACKS
static void zc_diagstage_nwk_sec_counters(void)
{
#ifndef NWK_SECUR_COUNTER_BREAKS_BY_ALARM
  if (nwk_secur_counter_breaks_num < NWK_SECUR_COUNTER_BREAKS_NUM_MAX)
  {
    if (g_packets_sent > 2 && g_packets_sent % 2 == 0)
    {
      nwk_secur_counter_breaks_num++;
      ZB_CERT_HACKS().break_nwk_fcf_counter = ZB_TRUE;
      TRACE_MSG(TRACE_APP1, "break_nwk_fcf_counter = ZB_TRUE", (FMT__0));
    }
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 2 (call set_aps_secur_counter_breaking_flag())", (FMT__0));
    diag_stage = DIAGSTAGE_APS_SEC_COUNTERS_AND_KEY; /* Go to stage 2 */
    ZB_CERT_HACKS().secur_aps_counter_hack_cb = break_aps_secur_counter_cb;
  }
#endif
}

static void zc_diagstage_aps_sec_counters_and_keys(void)
{
  if (aps_secur_counter_breaks_num >= APS_SECUR_COUNTER_BREAKS_NUM_MAX &&
      aps_secur_key_breaks_num >= APS_SECUR_KEY_BREAKS_NUM_MAX)
  {
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 3 (let diag_stage = DIAGSTAGE_NWK_SEC_KEY_AND_FRAME_TYPE)", (FMT__0));
    ZB_CERT_HACKS().secur_aps_counter_hack_cb = NULL;
    diag_stage = DIAGSTAGE_NWK_SEC_KEY_AND_FRAME_TYPE; /* Go to stage 3 */
  }
  if (aps_secur_counter_breaks_num < APS_SECUR_COUNTER_BREAKS_NUM_MAX)
  {
    /* Break only every fourth package */
    if (g_packets_sent % 4 == 0)
    {
      aps_secur_counter_breaks_num++;
      ZB_CERT_HACKS().break_aps_fcf_counter = ZB_TRUE;
    }
  }
  if (aps_secur_key_breaks_num < APS_SECUR_KEY_BREAKS_NUM_MAX)
  {
    /* Break only every fourth package, shifted two */
    if ((g_packets_sent + 2) % 4 == 0)
    {
      aps_secur_key_breaks_num++;
      /* Flag is set to ZB_FALSE by ZBOSS kernel */
      ZB_CERT_HACKS().break_aps_key = ZB_TRUE;
      TRACE_MSG(TRACE_APP3, "activate break_aps_key. packets_sent %u", (FMT__D, g_packets_sent));
    }
  }
}

static void zc_diagstage_nwk_sec_key_and_frame_type(void)
{
  /* Check the end of the stage. */
  if (nwk_secur_key_breaks_num >= NWK_SECUR_KEY_BREAKS_NUM_MAX &&
      mac_frame_type_breaks_num >= MAC_FRAME_TYPE_BREAKS_NUM_MAX)
  {
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 4 - flood", (FMT__0));
    ZB_SCHEDULE_ALARM_CANCEL(zc_send_data, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_ALARM_CANCEL(zc_get_buf_send_data, ZB_ALARM_ALL_CB);
    /* Start the flood and schedule it's off */
    ZB_SCHEDULE_ALARM(zc_flood_pre_start, 0, 4 * ZB_TIME_ONE_SECOND);
    ZB_SCHEDULE_ALARM(zc_flood_stop, 0, (FLOOD_DURATION_SEC + 5) * ZB_TIME_ONE_SECOND);
    diag_stage = DIAGSTAGE_FLOOD; /* Go to stage 4 */
  }

  if (nwk_secur_key_breaks_num < NWK_SECUR_KEY_BREAKS_NUM_MAX)
  {
    /* Break only every fourth package */
    if (g_packets_sent % 4 == 0)
    {
      nwk_secur_key_breaks_num++;
      /* Flag is set to ZB_FALSE by ZBOSS kernel */
      ZB_CERT_HACKS().break_nwk_key = ZB_TRUE;
      TRACE_MSG(TRACE_APP3, "activate break_nwk_key. packets_sent %u", (FMT__D, g_packets_sent));
    }
  }

  if (mac_frame_type_breaks_num < MAC_FRAME_TYPE_BREAKS_NUM_MAX)
  { 
    /* Change the frame type only every fourth package, shifted two */
    if ((g_packets_sent + 2) % 4 == 0)
    {
      mac_frame_type_breaks_num++;
      /* Flag is set to ZB_FALSE by ZBOSS kernel */
      ZB_CERT_HACKS().make_frame_not_valid = ZB_TRUE;
      TRACE_MSG(TRACE_APP3, "change frame type to MAC_FRAME_RESERVED1. packets_sent %u", (FMT__D, g_packets_sent));
    }
  }
}

static void zc_diagstage_flood_rest(void)
{
  if (g_flood_rest_pkt_cntr++ >= FLOOD_REST_PKTS_MAX)
  {
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 6 (call schedule_channel_changing())", (FMT__0));
    diag_stage = DIAGSTAGE_CHANNEL_CHG; /* Go to stage 6 */
    /* Allow to send data and change channel before receiving an echo-reply */
    ZB_SCHEDULE_ALARM(schedule_channel_changing_delayed, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(500));
  }
}

static void zc_flood_stop(zb_uint8_t param)
{
  (void)param;
  /* Stop loop zc_flood() */
  g_flood_stop = 1;
  ZB_CERT_HACKS().zc_flood_mode = ZB_FALSE;
  /* Restore echo transmission */
  ZB_SCHEDULE_ALARM(zc_get_buf_send_data, 0, 2 * ZB_TIME_ONE_SECOND);
  TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 5 - flood rest", (FMT__0));
  diag_stage = DIAGSTAGE_FLOOD_REST;
}

/* Fill and send the special packet and then schedule the flood */
static void zc_flood_pre_start(zb_uint8_t param)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr;
  /* A special packet, informing the python test about
   * the beginning and duration of the flood from ZC
   * Format (9 bytes):
   *   0xaaaa - packet head  (2 bytes)
   *   NN - flood duration   (1 byte)
   *   0xbbbb - packet tail  (2 bytes)
   *   KKKK - g_packets_sent (2 bytes)
   */
  param = zb_buf_get_out();
  if (param)
  {
    ptr = zb_buf_initial_alloc(param, 7);
    req = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));
    req->tx_options = ZB_APSDE_TX_OPT_ACK_TX | ZB_APSDE_TX_OPT_SECURITY_ENABLED;
    req->dst_addr.addr_short = g_remote_addr;
    req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    req->radius = 1;
    req->profileid = 2;
    req->src_endpoint = 10;
    req->dst_endpoint = 10;
    req->clusterid = 0;
    g_packets_sent++;
    ptr[0] = ptr[1] = 0xaa;
    ptr[2] = FLOOD_DURATION_SEC;
    ptr[3] = ptr[4] = 0xbb;
    /* place packet counter in last two bytes */
    ptr[5] = (g_packets_sent >> 8) & 0xff;
    ptr[6] = g_packets_sent & 0xff;
    
    TRACE_MSG(TRACE_ERROR, "Sending the flood start packet", (FMT__0));
    ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
    ZB_SCHEDULE_ALARM(zc_flood_start, 0, 1 * ZB_TIME_ONE_SECOND);
  }
}

static void zc_flood_start(zb_uint8_t param)
{
  (void)param;
  ZB_CERT_HACKS().zc_flood_mode = ZB_TRUE;
  ZB_SCHEDULE_CALLBACK(zc_flood_loop, 0);
}

static void zc_flood_loop(zb_uint8_t param)
{
  (void)param;

  if (g_flood_stop)
  {
    return;
  }
  /* Do not call zb_handle_beacon_req() directly! */
  /* ZB_SCHEDULE_TX_CB(zb_handle_beacon_req, 0); */
  ZB_SCHEDULE_CALLBACK(zc_get_buf_send_data, 0);
  ZB_SCHEDULE_CALLBACK(zc_flood_loop, 0);
}
#endif

static void zc_get_buf_send_data(zb_uint8_t param)
{
  (void)param;
  ZB_SCHEDULE_ALARM_CANCEL(zc_send_data, ZB_ALARM_ALL_CB);
  ZB_SCHEDULE_ALARM_CANCEL(zc_get_buf_send_data, ZB_ALARM_ALL_CB);
  zb_buf_get_out_delayed(zc_send_data);
}

static void zc_send_data(zb_uint8_t param)
{
  zb_apsde_data_req_t *req;
  zb_uint8_t *ptr = NULL;
  zb_ushort_t i;

#ifdef ZB_CERTIFICATION_HACKS
  /* In a normal echo test, the zc_send_data() is rescheduled by data_indication().
   * But, since we break transmission, we reschedule ourselves for a "long" time.
   */
  if (diag_stage != DIAGSTAGE_FLOOD)
  {
    ZB_SCHEDULE_ALARM(zc_get_buf_send_data, 0, ZB_TIME_ONE_SECOND * g_resend_data_interval);
  }
#endif

  ptr = zb_buf_initial_alloc(param, ZB_TEST_DATA_SIZE);
  req = zb_buf_get_tail(param, sizeof(zb_apsde_data_req_t));
  req->tx_options = ZB_APSDE_TX_OPT_ACK_TX | ZB_APSDE_TX_OPT_SECURITY_ENABLED;
  req->dst_addr.addr_short = g_remote_addr;
  req->addr_mode = ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
  req->radius = 1;
  req->profileid = 2;
  req->src_endpoint = 10;
  req->dst_endpoint = 10;
  req->clusterid = 0;
  /* zb_buf_set_handle(param, 0x11); */
  g_packets_sent++;
  for (i = 0 ; i < ZB_TEST_DATA_SIZE-2 ; ++i)
  {
    ptr[i] = i % 32 + '0';
  }
  /* place packet counter in last two bytes */
  ptr[i++] = (g_packets_sent >> 8) & 0xff;
  ptr[i]   = g_packets_sent & 0xff;

  TRACE_MSG(TRACE_APP3, "Sending apsde_data.request %hd pkts_sent %u", (FMT__H_D, param, g_packets_sent));
  ZB_SCHEDULE_CALLBACK(zb_apsde_data_request, param);
#ifdef ZB_CERTIFICATION_HACKS
  if (diag_stage == DIAGSTAGE_NWK_SEC_COUNTERS)
  {
    zc_diagstage_nwk_sec_counters();
  }
  else if (diag_stage == DIAGSTAGE_APS_SEC_COUNTERS_AND_KEY)
  {
    zc_diagstage_aps_sec_counters_and_keys();
  }
  else if (diag_stage == DIAGSTAGE_NWK_SEC_KEY_AND_FRAME_TYPE)
  {
    zc_diagstage_nwk_sec_key_and_frame_type();
  }
  else if (diag_stage == DIAGSTAGE_FLOOD_REST)
  {
    zc_diagstage_flood_rest();
  }
#endif
}

#ifndef NCP_SDK
void zc_send_data_loop(zb_uint8_t param)
{
  zb_bufid_t buf;

  ZB_ASSERT(param == 0);
  ZVUNUSED(param);
  buf = zb_buf_get_out();
  if (buf == 0)
  {
    TRACE_MSG(TRACE_ERROR, "Can't allocate buffer!", (FMT__0));
    ZB_SCHEDULE_ALARM(zc_send_data_loop, 0, 1*ZB_TIME_ONE_SECOND);
    return;
  }
  zc_send_data(buf);
  if (g_packets_sent < PACKETS_FROM_ZC_NR)
  {
    ZB_SCHEDULE_ALARM(zc_send_data_loop, 0, 1*ZB_TIME_ONE_SECOND);
  }
  else
  {
    if (exchange_finished())
    {
      ZB_SCHEDULE_ALARM(test_exit, 0, 1*ZB_TIME_ONE_SECOND);
    }
  }
}
#endif

#ifdef ZB_CERTIFICATION_HACKS
static void schedule_channel_changing_delayed(zb_uint8_t param)
{
  (void)param;
  zb_buf_get_out_delayed(schedule_channel_changing);
}

static void schedule_channel_changing(zb_uint8_t param)
{
  static zb_uint8_t runs_counter = 0;

  TRACE_MSG(TRACE_APP3, ">>schedule_channel_changing(), param %hd, delay %hd sec, runs_counter %hd",
            (FMT__H_H_H, param, CHANGE_CHANNEL_DELAY, runs_counter));

  if (runs_counter == 0)
  {
    ZB_SCHEDULE_ALARM_CANCEL(zc_send_data, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_ALARM_CANCEL(zc_get_buf_send_data, ZB_ALARM_ALL_CB);
    ZB_SCHEDULE_CALLBACK(change_channel, param);
  }
  else if (runs_counter == 1)
  {
    ZB_SCHEDULE_ALARM(change_channel, param, CHANGE_CHANNEL_DELAY * ZB_TIME_ONE_SECOND);
  }
  else
  {
    ZB_SCHEDULE_ALARM(change_channel, param, CHANGE_CHANNEL_DELAY * ZB_TIME_ONE_SECOND);
  }

  runs_counter++;
}

static void change_channel_cb(zb_uint8_t param)
{
  zb_mlme_set_confirm_t *conf = (zb_mlme_set_confirm_t*)zb_buf_begin(param);

  TRACE_MSG(TRACE_APP3, ">>change_channel_cb(), param %hd status %hd",
            (FMT__H_H, param, conf->status));

  if (conf->status == MAC_SUCCESS)
  {
    if (!channel_changing_flag)
    {
      /* restore echo transmission after 1000 ms */
      ZB_SCHEDULE_ALARM_CANCEL(zc_send_data, ZB_ALARM_ALL_CB);
      ZB_SCHEDULE_ALARM_CANCEL(zc_get_buf_send_data, ZB_ALARM_ALL_CB);
      ZB_SCHEDULE_ALARM(zc_get_buf_send_data, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
    }
    TRACE_MSG(TRACE_APP3, "channel was changed!", (FMT__0));
    channel_changing_flag = !channel_changing_flag;
    ZB_SCHEDULE_CALLBACK(schedule_channel_changing, param);
  }
  else
  {
    TRACE_MSG(TRACE_APP3, "couldn't change channel! try again", (FMT__0));
    ZB_SCHEDULE_CALLBACK(change_channel, param);
  }
}

static void change_channel(zb_uint8_t param)
{
  zb_uint8_t channel = channel_changing_flag ? NEW_CHANNEL : CHANNEL;

  TRACE_MSG(TRACE_APP3, ">>change_channel(), param %hd, channel %hd",
            (FMT__H_H, param, channel));

  zb_nwk_pib_set(param, ZB_PHY_PIB_CURRENT_CHANNEL, &channel, 1, change_channel_cb);
}


void set_nwk_secur_counter_breaking_flag(zb_uint8_t param)
{
  ZVUNUSED(param);

  TRACE_MSG(TRACE_APP3, "set_nwk_secur_counter_breaking_flag(), nwk_secur_counter_breaks_num %hd",
            (FMT__H, nwk_secur_counter_breaks_num));

  if (!ZB_CERT_HACKS().break_nwk_fcf_counter)
  {
    nwk_secur_counter_breaks_num++;

    if (nwk_secur_counter_breaks_num <= NWK_SECUR_COUNTER_BREAKS_NUM_MAX)
    {
      ZB_CERT_HACKS().break_nwk_fcf_counter = ZB_TRUE;
      TRACE_MSG(TRACE_APP1, "break_nwk_fcf_counter = ZB_TRUE", (FMT__0));
    }
  }

  if (nwk_secur_counter_breaks_num <= NWK_SECUR_COUNTER_BREAKS_NUM_MAX)
  {
    ZB_SCHEDULE_ALARM(set_nwk_secur_counter_breaking_flag, 0, 1.5 * ZB_TIME_ONE_SECOND);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "DIAGS: goto stage 2 (call set_aps_secur_counter_breaking_flag())", (FMT__0));
    diag_stage = DIAGSTAGE_APS_SEC_COUNTERS_AND_KEY; /* Go to stage 2 */
    ZB_CERT_HACKS().secur_aps_counter_hack_cb = break_aps_secur_counter_cb;
  }
}

static void break_aps_secur_counter_cb(zb_uint32_t *out_sec_counter)
{
  TRACE_MSG(TRACE_APP3, "break_aps_secur_counter_cb(): out_sec_counter %ld aps_secur_counter_breaks_num %hd break_aps_fcf_counter %hd",
            (FMT__L_H_H, *out_sec_counter, aps_secur_counter_breaks_num, ZB_CERT_HACKS().break_aps_fcf_counter));

  if (ZB_CERT_HACKS().break_aps_fcf_counter)
  {
    TRACE_MSG(TRACE_ERROR, "*aps_out_sec_counter = 0;", (FMT__0));

    *out_sec_counter = 0;
    ZB_CERT_HACKS().break_aps_fcf_counter = ZB_FALSE;
  }
}
#endif /* ZB_CERTIFICATION_HACKS */


/*! @} */
