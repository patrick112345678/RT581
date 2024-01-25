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
/* PURPOSE: TH gpt
*/

#define ZB_TRACE_FILE_ID 41291

#include "../include/zgp_test_templates.h"

#include "test_config.h"

#define ENDPOINT 10

static zb_ieee_addr_t g_th_gpt_addr = TH_GPT_IEEE_ADDR;
static zb_ieee_addr_t g_dut_zr_addr = DUT_ZR_IEEE_ADDR;
static zb_ieee_addr_t g_th_gpp_addr = TH_GPP_IEEE_ADDR;
static zb_uint8_t g_key_nwk[] = NWK_KEY;

/*! Program states according to test scenario */
enum test_states_e
{
  TEST_STATE_INITIAL,
  TEST_STATE_READY_TO_COMMISSIONING,
  TEST_STATE_READ_GPP_PROXY_TABLE,
  TEST_STATE_NODE_DESCRIPTOR_REQ1,
  TEST_STATE_NODE_DESCRIPTOR_REQ2,
  TEST_STATE_FINISHED
};

ZB_ZGPC_DECLARE_ZCL_ON_OFF_TOGGLE_TEST_TEMPLATE(TEST_DEVICE_CTX, ENDPOINT, 5000)

static zb_bool_t get_short_addr_by_ieee(zb_ieee_addr_t ieee, zb_uint16_t *short_addr)
{
  zb_address_ieee_ref_t   addr_ref;

  if (zb_address_by_ieee(ieee, ZB_FALSE, ZB_FALSE, &addr_ref) == RET_OK)
  {
    zb_address_short_by_ref(short_addr, addr_ref);
  }
  else
  {
    TRACE_MSG(TRACE_APP1, "Error find short address for IEEE: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ieee)));
    return ZB_FALSE;
  }
  return ZB_TRUE;
}

static void node_desc_req(zb_uint8_t buf_ref, zb_uint8_t counter)
{
  zb_buf_t               *buf = ZB_BUF_FROM_REF(buf_ref);
  zb_zdo_node_desc_req_t *req;
  zb_uint16_t             short_address;

  TRACE_MSG(TRACE_APP3, "> node_desc_req param %hd", (FMT__H, buf_ref));

  if (!get_short_addr_by_ieee(g_dut_zr_addr, &short_address))
  {
    zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
    TRACE_MSG(TRACE_APP1, "Error find short address for DUT_ZR by IEEE", (FMT__0));
    return;
  }

  ZB_BUF_INITIAL_ALLOC(buf, sizeof(zb_zdo_node_desc_req_t), req);
  ZB_BZERO(req, sizeof(zb_zdo_node_desc_req_t));

  req->nwk_addr = short_address;
#ifdef ZB_CERTIFICATION_HACKS
  if (TEST_DEVICE_CTX.test_state == TEST_STATE_NODE_DESCRIPTOR_REQ2)
  {
    counter = 28;
  }
  ZB_CERT_HACKS().aps_counter_custom_setup = 1;
  ZB_CERT_HACKS().nwk_counter_custom_setup = 1;
  ZB_CERT_HACKS().aps_counter_custom_value = counter;
  ZB_CERT_HACKS().nwk_counter_custom_value = counter;
#endif
  zb_zdo_node_desc_req(buf_ref, NULL);
  TRACE_MSG(TRACE_APP3, "< node_desc_req", (FMT__0));
}

static void read_gpp_table(zb_uint8_t buf_ref, zb_callback_t cb)
{
  zb_uint16_t           short_address;

  if (!get_short_addr_by_ieee(g_th_gpp_addr, &short_address))
  {
    zb_free_buf(ZB_BUF_FROM_REF(buf_ref));
    TRACE_MSG(TRACE_APP1, "Error find short address for TH_GPP by IEEE", (FMT__0));
    return;
  }

  zgp_cluster_read_attr(buf_ref, short_address, ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                        ZB_ZCL_ATTR_GPP_PROXY_TABLE_ID,
                        ZB_ZCL_ENABLE_DEFAULT_RESPONSE, cb);
}

static void send_zcl(zb_uint8_t buf_ref, zb_callback_t cb)
{
  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READ_GPP_PROXY_TABLE:
      read_gpp_table(buf_ref, cb);
      ZB_ZGPC_SET_PAUSE(3);
      break;
    case TEST_STATE_NODE_DESCRIPTOR_REQ1:
      node_desc_req(buf_ref, 30);
      break;
    case TEST_STATE_NODE_DESCRIPTOR_REQ2:
      node_desc_req(buf_ref, 28);
      break;
  }
}

static zb_bool_t custom_comm_cb(zb_zgpd_id_t *zgpd_id, zb_zgp_comm_status_t result)
{
  ZVUNUSED(zgpd_id);

  if (result == ZB_ZGP_COMMISSIONING_COMPLETED)
  {
#ifdef ZB_USE_BUTTONS
    zb_led_blink_off(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
    return ZB_TRUE;
  }
  return ZB_FALSE;
}

static void perform_next_state(zb_uint8_t param)
{
  if (TEST_DEVICE_CTX.pause)
  {
    ZB_SCHEDULE_ALARM(perform_next_state, 0,
                      ZB_TIME_ONE_SECOND*TEST_DEVICE_CTX.pause);
    TEST_DEVICE_CTX.pause = 0;
    return;
  }

  TEST_DEVICE_CTX.test_state++;

  switch (TEST_DEVICE_CTX.test_state)
  {
    case TEST_STATE_READY_TO_COMMISSIONING:
      zb_zgps_start_commissioning(ZGP_GPS_GET_COMMISSIONING_WINDOW() *
                                  ZB_TIME_ONE_SECOND);
#ifdef ZB_USE_BUTTONS
  zb_led_blink_on(ZB_LED_ARG_CREATE(3, ZB_LED_BLINK_HALF_SEC));
#endif
      break;
    case TEST_STATE_FINISHED:
      TRACE_MSG(TRACE_APP1, "Test finished. Status: OK", (FMT__0));
      break;
    default:
    {
      if (param)
      {
        zb_free_buf(ZB_BUF_FROM_REF(param));
      }
      ZB_SCHEDULE_ALARM(test_send_command, 0, ZB_TIME_ONE_SECOND);
    }
  }
}

static zb_uint16_t addr_ass_cb(zb_ieee_addr_t ieee_addr)
{
  ZVUNUSED(ieee_addr);
  /* fix address for next ZR */
	return 0x5678;
}

static void zgpc_custom_startup()
{
  #if !(defined KEIL || defined SDCC || defined ZB_IAR)
#endif

  /* Init device, load IB values from nvram or set it to default */

  ZB_INIT("th_gpt");


  /* let's always be coordinator */
  ZB_AIB().aps_designated_coordinator = 1;
  ZB_AIB().aps_channel_mask = (1<<TEST_CHANNEL);
  zb_set_default_ffd_descriptor_values(ZB_COORDINATOR);

  ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), &g_th_gpt_addr);
  ZB_PIBCACHE_PAN_ID() = TEST_PAN_ID;
  ZB_PIBCACHE_RX_ON_WHEN_IDLE() = ZB_B2U(ZB_TRUE);

  zb_secur_setup_nwk_key(g_key_nwk, 0);

  ZB_NIB_SET_USE_MULTICAST(ZB_FALSE);

  /* Must use NVRAM for ZGP */
  ZB_AIB().aps_use_nvram = 1;

  /* Need to block GPDF recv directly */
#ifdef ZB_ZGP_SKIP_GPDF_ON_NWK_LAYER
  ZG->nwk.skip_gpdf = 0;
#endif
#ifdef ZB_ZGP_RUNTIME_WORK_MODE_WITH_PROXIES
  ZGP_CTX().enable_work_with_proxies = 1;
#endif
#ifdef ZB_CERTIFICATION_HACKS
  ZB_CERT_HACKS().ccm_check_cb = NULL;
#endif

  ZGP_GPS_COMMUNICATION_MODE = ZGP_COMMUNICATION_MODE_GROUPCAST_DERIVED;

  ZGP_GPS_COMMISSIONING_EXIT_MODE = ZGP_COMMISSIONING_EXIT_MODE_ON_PAIRING_SUCCESS;

  ZGP_GPS_SECURITY_LEVEL = ZB_ZGP_FILL_GPS_SECURITY_LEVEL(
                            ZB_ZGP_SEC_LEVEL_FULL_NO_ENC,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_PROTECTION_WITH_GP_LINK_KEY,
                            ZB_ZGP_DEFAULT_SEC_LEVEL_INVOLVE_TC);

  ZGP_GP_SET_SHARED_SECURITY_KEY_TYPE(ZB_ZGP_SEC_KEY_TYPE_NO_KEY);

  TEST_DEVICE_CTX.custom_comm_cb = custom_comm_cb;

  zb_nwk_set_address_assignment_cb(addr_ass_cb);
}

ZB_ZGPC_DECLARE_STARTUP_PROCESS()
