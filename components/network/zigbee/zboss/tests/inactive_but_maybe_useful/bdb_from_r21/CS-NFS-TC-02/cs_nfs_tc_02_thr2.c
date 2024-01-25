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
/* PURPOSE: TH ZR2 - joining to network, checking error cases
*/


#define ZB_TEST_NAME CS_NFS_TC_02_THR2
#define ZB_TRACE_FILE_ID 40961
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_bdb_internal.h"
#include "cs_nfs_tc_02_common.h"


/*! \addtogroup ZB_TESTS */
/*! @{ */


#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifndef ZB_ROUTER_ROLE
#error Router role is not compiled!
#endif

#ifndef ZB_USE_NVRAM
#error define ZB_USE_NVRAM
#endif

#ifndef ZB_CERTIFICATION_HACKS
#error define ZB_CERTIFICATION_HACKS
#endif


enum test_state_e
{
  REQUEST_KEY_NOT_SEND,
  REQUEST_KEY_UNPROTECTED,
  REQUEST_KEY_WITH_MIC_ERROR,
  REQUEST_KEY_PROTECTED_WITH_KEY_LOAD_KEY,
  REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY,
  REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL,
  REQUEST_KEY_FOR_NWK_KEY,
  REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC,
  REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1,
  VERIFY_KEY_NOT_SEND,
  VERIFICATION_FAILS,
  CORRECT_TCLK_RETENTION,
  TEST_STEPS_COUNT
};


static zb_bool_t upon_sending_request_key(zb_uint8_t param);
static zb_bool_t upon_sending_verify_key(zb_uint8_t param);

static void revert_key(zb_uint8_t unused);
static void revert_aps_security(zb_uint8_t unused);
static void forward_test(zb_uint8_t unused);

/* will be called upon Request Key sending */
static zb_bool_t test_fsm1();
/* will be called upon Verify Key sending */
static zb_bool_t test_fsm2();
static void verify_tclk_update(zb_uint8_t call_n);
static void verify_step(zb_bool_t verdict, zb_bool_t enable_trace);

static void send_cmd_delayed(zb_uint8_t unused);

static void prepare_request_key(zb_buf_t *buf,
                                zb_uint8_t key_type,
                                zb_ieee_addr_t partner_ieee);
static void send_aps_cmd_via_nwk(zb_buf_t *buf,
                                 zb_uint8_t cmd_id,
                                 zb_secur_key_id_t key_id,
                                 zb_uint8_t options);

static void send_request_key(zb_uint8_t param, zb_uint16_t options);
static void send_buffer_test_req(zb_uint8_t param);
static void buffer_test_resp_cb(zb_uint8_t status);


/* Copied from stack */
static void auth_trans(
  zb_ushort_t ccm_m,
  zb_uint8_t *key,
  zb_uint8_t *nonce,
  zb_uint8_t *string_a,
  zb_uint16_t string_a_len,
  zb_uint8_t *string_m,
  zb_uint16_t string_m_len,
  zb_uint8_t *t);
static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result);


/* NVRAM support */
static void save_test_state();
#ifdef USE_NVRAM_IN_TEST
static void read_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length);
static zb_ret_t write_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos);
static zb_uint16_t th_nvram_get_app_data_size_cb();
#endif /* USE_NVRAM_IN_TEST */


static int s_step_idx;
static int s_error_count;
static zb_uint8_t s_temp_key[ZB_CCM_KEY_SIZE];


MAIN()
{
  ARGV_UNUSED;

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("zdo_thr2");


  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_ieee_addr_thr2);

  ZB_BDB().bdb_primary_channel_set = TEST_BDB_PRIMARY_CHANNEL_SET;
  ZB_BDB().bdb_secondary_channel_set = TEST_BDB_SECONDARY_CHANNEL_SET;
  ZB_BDB().bdb_mode = 1;

  ZB_NIB_DEVICE_TYPE() = ZB_NWK_DEVICE_TYPE_ROUTER;
  ZB_NIB().max_children = 0;
  ZB_AIB().aps_use_nvram = 1;

#ifdef USE_NVRAM_IN_TEST
  zb_nvram_register_app1_read_cb(read_th_app_data_cb);
  zb_nvram_register_app1_write_cb(write_th_app_data_cb, th_nvram_get_app_data_size_cb);
#endif


  ZB_CERT_HACKS().stay_on_network_after_auth_failure = 1;
  ZB_CERT_HACKS().req_key_call_cb = upon_sending_request_key;
  ZB_TCPOL().tclk_exchange_attempts_max = 0;

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


static zb_bool_t upon_sending_request_key(zb_uint8_t param)
{
  zb_bool_t ret;

  TRACE_MSG(TRACE_ZDO3, ">>upon_sending_request_key: buf = %d", (FMT__D, param));

  ZVUNUSED(param);
  ret = test_fsm1();

  TRACE_MSG(TRACE_ZDO3, "<<upon_sending_request_key", (FMT__0));

  return ret;
}


static zb_bool_t upon_sending_verify_key(zb_uint8_t param)
{
  zb_bool_t ret;

  TRACE_MSG(TRACE_ZDO3, ">>upon_sending_verify_key: buf = %d", (FMT__D, param));

  ZVUNUSED(param);
  ret = test_fsm2();

  TRACE_MSG(TRACE_ZDO3, "<<upon_sending_verify_key", (FMT__0));

  return ret;
}


static void revert_key(zb_uint8_t unused)
{
  zb_aps_device_key_pair_set_t *aps_key;

  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO3, "revert pas link key", (FMT__0));

  aps_key = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_PROVISIONAL_KEY);
  if (aps_key)
  {
    ZB_MEMCPY(aps_key->link_key, s_temp_key, ZB_CCM_KEY_SIZE);
  }
}


static void revert_aps_security(zb_uint8_t unused)
{
  ZVUNUSED(unused);
  TRACE_MSG(TRACE_ZDO3, "Switch aps_security_off to 0", (FMT__0));
  ZB_CERT_HACKS().aps_security_off = 0;
}


static void forward_test(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  if (s_step_idx < REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL)
  {
    ++s_step_idx;
  }
  test_fsm1();
}


static zb_bool_t test_fsm1()
{
  zb_bool_t drop_cmd = ZB_TRUE;
  zb_bool_t verify = ZB_TRUE;
  zb_uint8_t alarm_param = 0;

  TRACE_MSG(TRACE_ZDO3, ">>test_fsm1: state = %d", (FMT__D, s_step_idx));

  switch (s_step_idx)
  {
    case REQUEST_KEY_NOT_SEND:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 not send Request Key", (FMT__0));
      }
      break;

    case REQUEST_KEY_UNPROTECTED:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key unprotected", (FMT__0));
        ZB_CERT_HACKS().aps_security_off = 1;
        ZB_SCHEDULE_ALARM(revert_aps_security, 0, TH_REVERT_KEY_DELAY);
        drop_cmd = ZB_FALSE;
      }
      break;

    case REQUEST_KEY_WITH_MIC_ERROR:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key with MIC error", (FMT__0));
        verify = ZB_FALSE;
        {
          zb_aps_device_key_pair_set_t *aps_key;

          aps_key = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_PROVISIONAL_KEY);
          if (aps_key)
          {
            ZB_MEMCPY(s_temp_key, aps_key->link_key, ZB_CCM_KEY_SIZE);
            ZB_MEMCPY(aps_key->link_key, g_link_key, ZB_CCM_KEY_SIZE);
            ZB_SCHEDULE_ALARM(revert_key, 0, TH_REVERT_KEY_DELAY);
          }
          ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
          ZB_SCHEDULE_ALARM(forward_test, 0, TH_RESEND_REQUEST_KEY_DELAY);
        }
      }
      break;

    case REQUEST_KEY_PROTECTED_WITH_KEY_LOAD_KEY:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key protected with Key-Load Key", (FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
        verify = ZB_FALSE;
        ZB_SCHEDULE_ALARM(forward_test, 0, TH_RESEND_REQUEST_KEY_DELAY);
      }
      break;

    case REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key protected with Key-Transport Key", (FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
        verify = ZB_FALSE;
        ZB_SCHEDULE_ALARM(forward_test, 0, TH_RESEND_REQUEST_KEY_DELAY);
      }
      break;

    case REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL:
      {
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: THr2 send Request Key protected with invalid security level Key (4B MIC only, no encryption)",
                  (FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
        alarm_param = 1; /* go to next state immediately after verify_tclk_exchange */
      }
      break;

    case REQUEST_KEY_FOR_NWK_KEY:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for NWK Key", (FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
      }
      break;

    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for Application Link Key with TC",(FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
      }
      break;

    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 send Request Key for Application Link Key with THr1",(FMT__0));
        ZB_GET_OUT_BUF_DELAYED2(send_request_key, s_step_idx);
      }
      break;

    case VERIFY_KEY_NOT_SEND:
    case VERIFICATION_FAILS:
    case CORRECT_TCLK_RETENTION:
      ZB_CERT_HACKS().verify_key_call_cb = upon_sending_verify_key;
      drop_cmd = ZB_FALSE;
      verify = ZB_FALSE;
      break;

    default:
      verify = ZB_FALSE;
      TRACE_MSG(TRACE_ZDO1, "TEST: unknown state transition", (FMT__0));
  }

  if (verify == ZB_TRUE)
  {
    ZB_SCHEDULE_ALARM_CANCEL(verify_tclk_update, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_ALARM(verify_tclk_update, alarm_param, TH_WAIT_FOR_TCLK_UPDATE_DELAY);
  }

  TRACE_MSG(TRACE_ZDO3, "<<test_fsm1", (FMT__0));

  return drop_cmd;
}


static zb_bool_t test_fsm2()
{
  zb_bool_t drop_cmd = ZB_TRUE;
  int skip_alarm = 0;

  TRACE_MSG(TRACE_ZDO3, ">>test_fsm2: state = %d", (FMT__D, s_step_idx));

  switch (s_step_idx)
  {
    case VERIFY_KEY_NOT_SEND:
      {
        TRACE_MSG(TRACE_ZDO1, "TEST: THr2 not send Verify Key", (FMT__0));
        ZB_CERT_HACKS().verify_key_call_cb = NULL;
      }
      break;

    case VERIFICATION_FAILS:
      {
        TRACE_MSG(TRACE_ZDO1,
                  "TEST: THr2 fails verification and send out cmd protected with dTCLK",
                  (FMT__0));
        ZB_GET_OUT_BUF_DELAYED(send_buffer_test_req);
        ZB_CERT_HACKS().verify_key_call_cb = NULL;
      }
      break;

    case CORRECT_TCLK_RETENTION:
      TRACE_MSG(TRACE_ZDO1,
                "TEST: correct TCLK retention (for multiple devices) after updated TCLK confirmation",
                (FMT__0));
      drop_cmd = ZB_FALSE;
      ZB_CERT_HACKS().verify_key_call_cb = NULL;
      break;

    default:
      skip_alarm = 1;
      drop_cmd = ZB_FALSE;
  }

  if (!skip_alarm)
  {
    ZB_SCHEDULE_ALARM_CANCEL(verify_tclk_update, ZB_ALARM_ANY_PARAM);
    ZB_SCHEDULE_ALARM(verify_tclk_update, 0, TH_WAIT_FOR_TCLK_UPDATE_DELAY);
  }

  TRACE_MSG(TRACE_ZDO3, "<<test_fsm2", (FMT__0));

  return drop_cmd;
}


static void verify_tclk_update(zb_uint8_t call_n)
{
  zb_aps_device_key_pair_set_t *aps_key_unverified;
  zb_aps_device_key_pair_set_t *aps_key_verified;
  zb_bool_t verdict = ZB_FALSE;

  aps_key_unverified = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_UNVERIFIED_KEY);
  aps_key_verified = zb_secur_get_link_key_by_address(g_ieee_addr_dut, ZB_SECUR_VERIFIED_KEY);

  if (aps_key_unverified)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: THR2 receive Transport Key with new TCLK", (FMT__0));
  }
  else if (aps_key_verified)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: THR2 receive Confirm Key for new TCLK", (FMT__0));
  }

  switch (s_step_idx)
  {
    case REQUEST_KEY_WITH_MIC_ERROR:
    case REQUEST_KEY_PROTECTED_WITH_KEY_LOAD_KEY:
    case REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY:
    case REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL:
    case REQUEST_KEY_NOT_SEND:
    case REQUEST_KEY_UNPROTECTED:
    case REQUEST_KEY_FOR_NWK_KEY:
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC:
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1:
      TRACE_MSG(TRACE_ZDO1,
                "TEST: aps_unverified = %p, aps_verified = %p",
                (FMT__P_P, aps_key_unverified, aps_key_verified));
      verdict = (aps_key_unverified || aps_key_verified)? ZB_FALSE: ZB_TRUE;
      break;

    case VERIFY_KEY_NOT_SEND:
    case VERIFICATION_FAILS:
      TRACE_MSG(TRACE_ZDO1,
                "TEST: aps_unverified = %p, aps_verified = %p",
                (FMT__P_P, aps_key_unverified, aps_key_verified));
      verdict = (aps_key_unverified)? ZB_FALSE: ZB_TRUE;
      break;
  }

  if (!call_n)
  {
    ZB_SCHEDULE_ALARM(send_cmd_delayed, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
    //ZB_SCHEDULE_ALARM(send_cmd_delayed, 0, 7 * ZB_TIME_ONE_SECOND);
  }
  else
  {
    verify_step(verdict, ZB_TRUE);
  }
}


static void verify_step(zb_bool_t verdict, zb_bool_t enable_trace)
{
  if (enable_trace == ZB_TRUE)
  {
    switch (verdict)
    {
      case ZB_FALSE:
        TRACE_MSG(TRACE_ZDO1, "TEST: VERDICT - STEP FAILED", (FMT__0));
        ++s_error_count;
        break;
      case ZB_TRUE:
        TRACE_MSG(TRACE_ZDO1, "TEST: VERDICT - STEP PASSED", (FMT__0));
        break;
    }
  }
  ++s_step_idx;
  save_test_state();

  if (s_step_idx == TEST_STEPS_COUNT)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: completed, errors - %d", (FMT__D, s_error_count));
  }
}



static void send_cmd_delayed(zb_uint8_t unused)
{
  ZVUNUSED(unused);

  switch (s_step_idx)
  {
    case REQUEST_KEY_NOT_SEND:
    case REQUEST_KEY_UNPROTECTED:
    case REQUEST_KEY_FOR_NWK_KEY:
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC:
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1:
    case VERIFY_KEY_NOT_SEND:
    case VERIFICATION_FAILS:
      ZB_GET_OUT_BUF_DELAYED2(send_request_key, 0xffff);
      ZB_SCHEDULE_ALARM(verify_tclk_update, 1, TH_WAIT_FOR_TCLK_UPDATE_DELAY);
      break;

    case CORRECT_TCLK_RETENTION:
      ZB_GET_OUT_BUF_DELAYED(send_buffer_test_req);
      break;
  }
}


static void prepare_request_key(zb_buf_t *buf,
                                zb_uint8_t key_type,
                                zb_ieee_addr_t partner_ieee)
{
  zb_apsme_request_key_pkt_t *pkt;
  zb_uint8_t size = sizeof(zb_uint8_t);

  if (key_type == ZB_REQUEST_APP_LINK_KEY)
  {
    size += sizeof(zb_ieee_addr_t);
  }

  ZB_BUF_INITIAL_ALLOC(buf, size, pkt);
  pkt->key_type = key_type;

  if (key_type == ZB_REQUEST_APP_LINK_KEY)
  {
    ZB_IEEE_ADDR_COPY(pkt->partner_address, partner_ieee);
  }
}


static void send_aps_cmd_via_nwk(zb_buf_t *buf,
                                 zb_uint8_t cmd_id,
                                 zb_secur_key_id_t key_id,
                                 zb_uint8_t options)
{
  zb_uint8_t param = ZB_REF_FROM_BUF(buf);
  zb_aps_command_pkt_header_t *hdr;
  zb_aps_data_aux_nonce_frame_hdr_t *aux;

  zb_aps_command_add_secur(buf, cmd_id, key_id, 0x0000);

  hdr = (zb_aps_command_pkt_header_t *)ZB_BUF_BEGIN(buf);
  ZB_APS_FC_SET_FRAME_TYPE(hdr->fc, ZB_APS_FRAME_COMMAND);
  hdr->aps_counter = ZB_AIB_APS_COUNTER();
  ZB_AIB_APS_COUNTER_INC();

  if ( (options == REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL) ||
       (options == REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY) )
  {
    zb_uint8_t t[16];
    zb_uint8_t *key = (zb_uint8_t*)&ZB_AIB().tc_standard_key;
    zb_uint8_t key_buf[ZB_CCM_KEY_SIZE];
    zb_secur_ccm_nonce_t nonce;
    zb_uint8_t *payload;
    zb_uint8_t hdrs_size;
    zb_uint8_t *ptr;
    zb_ushort_t str_m_len;
    int n;

    aux = (zb_aps_data_aux_nonce_frame_hdr_t*) (hdr + 1);
    /* Set security level to 1 */
    if (options == REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL)
    {
      aux->secur_control &= 0x07;
      aux->secur_control |= 0x01;
    }

    nonce.frame_counter = aux->frame_counter;
    nonce.secur_control = aux->secur_control;
    ZB_IEEE_ADDR_COPY(nonce.source_address, ZB_PIBCACHE_EXTENDED_ADDRESS());

    payload = (zb_uint8_t *)aux + zb_aps_secur_aux_size(aux->secur_control);
    hdrs_size = payload - ZB_BUF_BEGIN(buf);
    str_m_len = (ZB_BUF_LEN(buf) - hdrs_size);
    n = (options == REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY)? 0: 2;

    /* Remove APS encryption added by zb_aps_command_add_secur */
    buf->u.hdr.encrypt_type &= ~ZB_SECUR_APS_ENCR;

    /* hashed key pair key for transport key command */
    zb_cmm_key_hash(key, n, key_buf);

    auth_trans(ZB_CCM_M, key_buf, (zb_uint8_t*)&nonce,
               (zb_uint8_t*)hdr, payload - (zb_uint8_t*)hdr,
               (zb_uint8_t*)payload, str_m_len, t);

    ZB_BUF_ALLOC_RIGHT(buf, sizeof(zb_uint32_t), ptr);
    if (options == REQUEST_KEY_WITH_INVALID_SECURITY_LEVEL)
    {
      /* now MIC contains in t, copy first 4 bytes into buffer */
      ZB_MEMCPY(ptr, t, sizeof(zb_uint32_t));
    }

    if (options == REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY)
    {
      zb_buf_t *encr_buf = zb_get_out_buf();
      zb_uint8_t *encr_ptr;

      if (!encr_buf)
      {
        TRACE_MSG(TRACE_ERROR, "TEST: no buffer for encryption", (FMT__0));
        zb_free_buf(buf);
        return;
      }

      ZB_BUF_INITIAL_ALLOC(encr_buf,
                           (payload - (zb_uint8_t*)hdr) + str_m_len + ZB_CCM_M,
                           encr_ptr);
      encrypt_trans(ZB_CCM_M, key_buf, (zb_uint8_t*)&nonce, payload,
                    str_m_len, t, encr_ptr + (payload - (zb_uint8_t*)hdr));
      ZB_MEMCPY(payload, encr_ptr + (payload - (zb_uint8_t*)hdr), str_m_len + sizeof(zb_uint32_t));
      zb_free_buf(encr_buf);
      ZB_SECUR_SET_ZEROED_LEVEL(aux->secur_control);
    }
  }


  fill_nldereq(param, 0x0000, ZB_TRUE);
  ZB_SCHEDULE_CALLBACK(zb_nlde_data_request, param);
}


static void send_request_key(zb_uint8_t param, zb_uint16_t options)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_secur_key_id_t key_id;

  /* Fill command payload */
  switch (options)
  {
    case REQUEST_KEY_FOR_NWK_KEY:
      /* TODO: APSME-REQUEST-KEY allow ZB_REQUEST_APP_LINK_KEY and ZB_REQUEST_TC_LINK_KEY only  */
      prepare_request_key(buf, ZB_STANDARD_NETWORK_KEY, (zb_uint8_t*) g_unknown_ieee_addr);
      break;
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_TC:
      prepare_request_key(buf, ZB_REQUEST_APP_LINK_KEY, g_ieee_addr_dut);
      break;
    case REQUEST_KEY_FOR_APP_LINK_KEY_WITH_THR1:
      prepare_request_key(buf, ZB_REQUEST_APP_LINK_KEY, g_ieee_addr_thr1);
      break;
    default:
      prepare_request_key(buf, ZB_REQUEST_TC_LINK_KEY, (zb_uint8_t*) g_unknown_ieee_addr);
  }

  /* Send command - switch key protection type */
  switch (options)
  {
    case REQUEST_KEY_PROTECTED_WITH_KEY_LOAD_KEY:
      key_id = ZB_SECUR_KEY_LOAD_KEY;
      break;
    case REQUEST_KEY_PROTECTED_WITH_KEY_TRANSPORT_KEY:
      key_id = ZB_SECUR_KEY_TRANSPORT_KEY;
      break;
    default:
      key_id = ZB_SECUR_DATA_KEY;
  }

  send_aps_cmd_via_nwk(buf, APS_CMD_REQUEST_KEY, key_id, options);
}


static void send_buffer_test_req(zb_uint8_t param)
{
  zb_buf_t *buf = ZB_BUF_FROM_REF(param);
  zb_buffer_test_req_param_t *req = ZB_GET_BUF_TAIL(buf, sizeof(zb_buffer_test_req_param_t));

  TRACE_MSG(TRACE_ZDO1, "TEST: Sending Buffer Test Request", (FMT__0));

  BUFFER_TEST_REQ_SET_DEFAULT(req);
  req->dst_addr = zb_address_short_by_ieee(g_ieee_addr_dut);
  zb_tp_buffer_test_request(param, buffer_test_resp_cb);
}


static void buffer_test_resp_cb(zb_uint8_t status)
{
  if (s_step_idx != CORRECT_TCLK_RETENTION)
  {
    return;
  }

  if (!status)
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT answer on command", (FMT__0));
    verify_step(ZB_TRUE, ZB_TRUE);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO1, "TEST: DUT does not answer on command", (FMT__0));
    verify_step(ZB_FALSE, ZB_TRUE);
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
        if (s_step_idx == CORRECT_TCLK_RETENTION)
        {
          ZB_SCHEDULE_ALARM(send_cmd_delayed, 0, TH_WAIT_FOR_PERMIT_JOIN_EXPIRES_DELAY);
        }
        break;

      default:
        TRACE_MSG(TRACE_APS1, "Unknown signal - %hd", (FMT__H, sig));
    }
  }
  else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
  {
    TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    if (s_step_idx == CORRECT_TCLK_RETENTION)
    {
      verify_step(ZB_FALSE, ZB_TRUE);
    }
  }

  zb_free_buf(ZB_BUF_FROM_REF(param));
}



static void auth_trans(
  zb_ushort_t ccm_m,
  zb_uint8_t *key,
  zb_uint8_t *nonce,
  zb_uint8_t *string_a,
  zb_uint16_t string_a_len,
  zb_uint8_t *string_m,
  zb_uint16_t string_m_len,
  zb_uint8_t *t)
{
  zb_uint8_t *p;
  zb_ushort_t len;
  zb_uint8_t tmp[16];
  zb_uint8_t b[16];
  /* A.2.2Authentication Transformation */

  /*
    1 Form the 1-octet Flags field
     Flags = Reserved || Adata || M || L
    2 Form the 16-octet B0 field
     B0 = Flags || Nonce N || l(m)
  */

  b[0] = (1<<6) | (ZB_CCM_L - 1) | (((ccm_m - 2)/2) << 3);
  ZB_MEMCPY(&b[1], nonce, ZB_CCM_NONCE_LEN);
  ZB_HTOBE16(&b[ZB_CCM_NONCE_LEN+1], &string_m_len);
  ZB_CHK_ARR(b, 16);

  /* Execute CBC-MAC (CBC with zero initialization vector) */

  /* iteration 0: E(key, b ^ zero). result (X1) is in t. */
  zb_aes128(key, b, t);

  /* A.2.1Input Transformation */
  /*
    3 Form the padded message AddAuthData by right-concatenating the resulting
    string with the smallest non-negative number of all-zero octets such that the
    octet string AddAuthData has length divisible by 16

    Effectively done by memset(0)
   */
  /*
    1 Form the octet string representation L(a) of the length l(a) of the octet string a:
  */
  ZB_HTOBE16(tmp, &string_a_len);
  /* 2 Right-concatenate the octet string L(a) and the octet string a itself: */
  if (string_a_len <= 16 - 2)
  {
    ZB_MEMSET(&tmp[2], 0, 14);
    ZB_MEMCPY(&tmp[2], string_a, string_a_len);
    len = 0;
  }
  else
  {
    ZB_MEMCPY(&tmp[2], string_a, 14);
    len = string_a_len - 14;
  }

  /* iteration 1: L(a) || a in tmp  */
  xor16(t, tmp, b);
  ZB_CHK_ARR(b, 16);
  zb_aes128(key, b, t);

  /* next iterations with a */
  p = string_a + 16 - 2;
  /* while no padding necessary */
  while (len >= 16)
  {
    xor16(t, p, b);
    ZB_CHK_ARR(b, 16);
    zb_aes128(key, b, t);
    p += 16;
    len -= 16;
  }
  if (len)
  {
    /* rest with padding */
    ZB_MEMSET(tmp, 0, 16);
    ZB_MEMCPY(tmp, p, len);
    xor16(t, tmp, b);
    ZB_CHK_ARR(b, 16);
    zb_aes128(key, b, t);
  }

  /* done with a, now process m */
  len = string_m_len;
  p = string_m;
  ZB_CHK_ARR(string_m, string_m_len);
  while (len >= 16)
  {
    ZB_CHK_ARR(t, 16);
    ZB_CHK_ARR(p, 16);
    xor16(t, p, b);

    ZB_CHK_ARR(key, 16);
    ZB_CHK_ARR(b, 16);
    ZB_CHK_ARR(t, 16);

    zb_aes128(key, b, t);
    p += 16;
    len -= 16;
  }
  if (len)
  {
    /* rest with padding */
    ZB_MEMSET(tmp, 0, 16);
    ZB_MEMCPY(tmp, p, len);
    xor16(t, tmp, b);
    zb_aes128(key, b, t);
  }

  /* return now. In first ccm_m bytes of t is T */
}


static void xor16(zb_uint8_t *v1, zb_uint8_t *v2, zb_uint8_t *result)
{
  zb_ushort_t i;
  for (i = 0 ; i < 16 ; ++i)
  {
    result[i] = v1[i] ^ v2[i];
  }
}



static void save_test_state()
{
  TRACE_MSG(TRACE_APS3, ">>save_test_state: saved value = %d", (FMT__D, s_step_idx));

#ifdef USE_NVRAM_IN_TEST
  /* Need factory new TH ZR that able to save it's test state between reboots */
  zb_nvram_clear();
  zb_nvram_write_dataset(ZB_NVRAM_APP_DATA1);
#else
  TRACE_MSG(TRACE_APS3, "NVRAM is not supported", (FMT__0));
#endif

  TRACE_MSG(TRACE_APS3, "<<save_test_state", (FMT__0));
}


#ifdef USE_NVRAM_IN_TEST
static void read_th_app_data_cb(zb_uint8_t page, zb_uint32_t pos, zb_uint16_t payload_length)
{
  zb_ret_t ret;
  th_nvram_app_dataset_t ds;

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
  th_nvram_app_dataset_t ds;

  TRACE_MSG(TRACE_APS3, ">>write_th_app_data_cb", (FMT__0));

  ds.current_test_step = (zb_uint32_t) s_step_idx;
  ret = zb_osif_nvram_write(page, pos, (zb_uint8_t*)&ds, sizeof(ds));

  TRACE_MSG(TRACE_APS3, "<<write_th_app_data_cb ret %d", (FMT__D, ret));
  return ret;
}


static zb_uint16_t th_nvram_get_app_data_size_cb()
{
  return sizeof(th_nvram_app_dataset_t);
}
#endif /* USE_NVRAM_IN_TEST */


/*! @} */
