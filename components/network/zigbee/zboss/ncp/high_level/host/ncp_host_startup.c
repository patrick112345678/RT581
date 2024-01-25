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
/*  PURPOSE: ZBOSS NCP Host startup sequence.
*/

#define ZB_TRACE_FILE_ID 94
#include "zb_common.h"
#include "zb_error_indication.h"
#include "ncp_host_hl_proto.h"
#include "ncp_host_hl_transport_internal_api.h"
#include "ncp_host_hl_adapter.h"
#include "ncp_host_soc_state.h"
#include "ncp_hl_proto.h"

#define DELAY_BEFORE_RESET (ZB_TIME_ONE_SECOND)
#define TRANSPORT_REINIT_TIMEOUT ZB_MILLISECONDS_TO_BEACON_INTERVAL(100)

#define INIT_PARAM_FIRST 0
#define INIT_PARAM_REINIT 1

typedef enum ncp_startup_stage_e
{
  STARTUP_STAGE_SETTING_PARAMS,
  STARTUP_STAGE_READING_SOC_STATE,
  STARTUP_STAGE_DONE
} ncp_startup_stage_t;

typedef enum ncp_setting_param_step_e
{
  PARAM_STEP_RESET,
  /* TODO: check NCP version? See ZOI-613. */
  PARAM_STEP_REQUEST_MODULE_VERSION,
#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
  PARAM_STEP_REQUEST_PRODUCTION_CONFIG,
#else
  /* Serial number is requested first to check whether
   * production config is presented. In that case host requests
   * channel mask, long address, local installcode and vendor data and
   * previous host settings are ignored.
   * If production config is not presented, host uses previous settings.
   */
  PARAM_STEP_REQUEST_SERIAL_NUMBER,
  PARAM_STEP_REQUEST_CHANNEL_MASK,
  PARAM_STEP_REQUEST_LONG_ADDR,
  PARAM_STEP_REQUEST_LOCAL_IC,
  PARAM_STEP_REQUEST_VENDOR_DATA,
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
#endif /* ZB_PRODUCTION_CONFIG */
  PARAM_STEP_LONG_ADDR,
  PARAM_STEP_SECUR_MATERIAL,
  PARAM_STEP_USE_EXTENDED_PAN_ID,
  PARAM_STEP_PAN_ID,
  PARAM_STEP_RX_ON_WHEN_IDLE,
  PARAM_STEP_DEVICE_TYPE,
  PARAM_STEP_MAX_CHILDREN,
  PARAM_STEP_TC_POLICY_TC_LINK_KEYS_REQUIRED,
#ifdef ZB_SECURITY_INSTALLCODES
  PARAM_STEP_IC_POLICY,
  PARAM_STEP_LOCAL_IC_SET,
#endif /* ZB_SECURITY_INSTALLCODES */
  PARAM_STEP_CHANNEL_LIST,
  PARAM_STEP_PERMIT_CONTROL4_NETWORK,
  PARAM_STEP_KEEPALIVE_MODE,
  PARAM_STEP_ED_TIMEOUT,
#ifdef ZB_LIMIT_VISIBILITY
  PARAM_STEP_MAC_INVISIBLE_SHORTS,
  PARAM_STEP_MAC_VISIBLE_LONGS,
#endif /* ZB_LIMIT_VISIBILITY */
  PARAM_STEP_SETTING_SIMPLE_DESCRIPTORS,
  /* TODO: add setting a power descriptor when it is in a public API. See ZOI-614 */
  PARAM_STEP_DONE
} ncp_setting_param_step_t;

typedef enum ncp_reading_soc_state_step_e
{
  SOC_STATE_STEP_AUTHENTICATED,
  SOC_STATE_STEP_PARENT_ADDRESS,
  SOC_STATE_STEP_EXTENDED_PAN_ID,
  SOC_STATE_STEP_PAN_ID,
  SOC_STATE_STEP_COORDINATOR_VERSION,
  SOC_STATE_STEP_NETWORK_ADDRESS,
  SOC_STATE_STEP_CURRENT_PAGE_AND_CHANNEL,
  SOC_STATE_STEP_TRUST_CENTER_ADDRESS,
  SOC_STATE_STEP_TCLK,
  SOC_STATE_STEP_ADDRESS_TABLE,
  SOC_STATE_STEP_APS_KEYS_TABLE,
  SOC_STATE_STEP_GROUP_TABLE,
  SOC_STATE_STEP_READ_NVRAM,
  SOC_STATE_STEP_DONE
} ncp_reading_soc_state_step_t;

typedef struct ncp_host_startup_state_s
{
  ncp_startup_stage_t current_stage;
  zb_uint_t current_step;
  zb_uindex_t simple_desc_index;
  zb_bool_t autostart;
  zb_uindex_t start_secur_material_index;
  zb_bool_t soc_initial_reset_resp_processed;
  zb_uindex_t current_dataset;
#ifdef ZB_LIMIT_VISIBILITY
  zb_uindex_t mac_invisible_short_index;
  zb_uindex_t mac_visible_long_index;
#endif /* ZB_LIMIT_VISIBILITY */

#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
  zb_uint8_t *production_config_data;
  zb_uint16_t production_config_data_len;
#else
  zb_bool_t production_config_presented;
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
#endif /* ZB_PRODUCTION_CONFIG */
  zb_bool_t transport_error_detected;
} ncp_host_startup_state_t;

static ncp_host_startup_state_t startup_ctx;

static void startup_dispatch(void);
static void startup_next_step(zb_uint8_t param);

static void startup_next_step(zb_uint8_t param)
{
  TRACE_MSG(TRACE_ZDO2, ">> startup_next_step", (FMT__0));
  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZDO2, "current stage: %d", (FMT__D, startup_ctx.current_stage));
  TRACE_MSG(TRACE_ZDO2, "current step: %d", (FMT__D, startup_ctx.current_step));

  ZB_ASSERT(startup_ctx.current_stage < STARTUP_STAGE_DONE);

  startup_ctx.current_step++;
  switch(startup_ctx.current_stage)
  {
    case STARTUP_STAGE_SETTING_PARAMS:
      if (startup_ctx.current_step >= PARAM_STEP_DONE)
      {
        startup_ctx.current_stage++;
        startup_ctx.current_step = 0;
      }
      break;
    case STARTUP_STAGE_READING_SOC_STATE:
      if (startup_ctx.current_step >= SOC_STATE_STEP_DONE)
      {
        startup_ctx.current_stage++;
        startup_ctx.current_step = 0;
      }
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "incorrect stage", (FMT__0));
      ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_ZDO2, "new stage: %d", (FMT__D, startup_ctx.current_stage));
  TRACE_MSG(TRACE_ZDO2, "new step: %d", (FMT__D, startup_ctx.current_step));

  startup_dispatch();

  TRACE_MSG(TRACE_ZDO2, "<< startup_next_step", (FMT__0));
}


static void soc_reset(zb_uint8_t nvram_erase)
{
  TRACE_MSG(TRACE_ZDO2, ">> soc_reset, nvram_erase %d", (FMT__D, nvram_erase));

  if (nvram_erase)
  {
    ncp_host_reset_nvram_erase();
  }
  else if (!startup_ctx.soc_initial_reset_resp_processed)
  {
    ncp_host_reset();
  }
  else
  {
    TRACE_MSG(TRACE_ZDO2, "initial Reset Response is received, reset is not needed", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_reset", (FMT__0));
}

void ncp_host_handle_reset_indication(zb_uint8_t reset_src)
{
  ZVUNUSED(reset_src);

  ncp_host_handle_reset_response(RET_OK, ZB_FALSE);
}

void ncp_host_handle_reset_response(zb_ret_t status_code, zb_bool_t is_solicited)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_reset_response, status: 0x%x, is_solicited %d",
    (FMT__D_H, status_code, is_solicited));

  ZB_ASSERT(status_code == RET_OK);

  /* We can get unsolicited reset response when SoC starts */
  ZB_ASSERT(is_solicited
            || (startup_ctx.current_stage <= STARTUP_STAGE_SETTING_PARAMS
                && startup_ctx.current_step <= PARAM_STEP_RESET));

  if (is_solicited)
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(!startup_ctx.soc_initial_reset_resp_processed);
    startup_ctx.soc_initial_reset_resp_processed = ZB_TRUE;
  }
}

static void soc_request_module_version(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_module_version", (FMT__0));

  ret = ncp_host_get_module_version();

  ZB_ASSERT(ret == RET_OK);
  TRACE_MSG(TRACE_ZDO2, "<< soc_request_module_version", (FMT__0));
}

#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
static void soc_request_production_config(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_production_config", (FMT__0));

  ret = ncp_host_request_production_config();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_production_config", (FMT__0));
}
#else
static void soc_request_channel_mask(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_channel_mask", (FMT__0));

  if (startup_ctx.production_config_presented)
  {
    ret = ncp_host_get_zigbee_channel_mask();
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_channel_mask", (FMT__0));
}


static void soc_request_long_addr(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_long_addr", (FMT__0));

  if (startup_ctx.production_config_presented)
  {
    ret = ncp_host_get_local_ieee_addr(0);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_long_addr", (FMT__0));
}


static void soc_request_local_ic(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_local_ic", (FMT__0));

  if (startup_ctx.production_config_presented)
  {
    ret = ncp_host_secur_get_local_ic();
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_local_ic", (FMT__0));
}


static void soc_request_serial_number(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_serial_number", (FMT__0));

  ret = ncp_host_get_serial_number();
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_serial_number", (FMT__0));
}


static void soc_request_vendor_data(void)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_request_vendor_data", (FMT__0));

  if (startup_ctx.production_config_presented)
  {
    ret = ncp_host_get_vendor_data();
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_request_vendor_data", (FMT__0));
}

#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
#endif /* ZB_PRODUCTION_CONFIG */


static void soc_set_long_addr(const zb_ieee_addr_t long_addr)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_set_long_addr, addr: " TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(long_addr)));

  ZB_ASSERT(ZB_IEEE_ADDR_IS_VALID(long_addr));

  if (
#if defined(ZB_PRODUCTION_CONFIG) && !defined(ZB_NCP_PRODUCTION_CONFIG_ON_HOST)
    !startup_ctx.production_config_presented
#else
    ZB_TRUE
#endif
    )
  {
    ret = ncp_host_set_local_ieee_addr(0, long_addr);
    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_long_addr", (FMT__0));
}

#if defined ZB_PRODUCTION_CONFIG && defined ZB_NCP_PRODUCTION_CONFIG_ON_HOST
zb_bool_t zb_production_configuration_check_presence(void)
{
  return startup_ctx.production_config_data_len != 0;
}


zb_ret_t zb_production_cfg_read_header(zb_uint8_t *prod_cfg_hdr, zb_uint16_t hdr_len)
{
  if (startup_ctx.production_config_data_len < hdr_len)
  {
    return RET_ERROR;
  }
  else
  {
    ZB_MEMCPY(prod_cfg_hdr, startup_ctx.production_config_data, hdr_len);
    return RET_OK;
  }
}


zb_ret_t zb_production_cfg_read(zb_uint8_t *buffer, zb_uint16_t len, zb_uint16_t offset)
{
  if (startup_ctx.production_config_data_len < offset + len)
  {
    return RET_ERROR;
  }
  else
  {
    ZB_MEMCPY(buffer, ((zb_uint8_t*)startup_ctx.production_config_data + offset), len);
    return RET_OK;
  }
}


static void startup_production_config_data_reset(void)
{
  startup_ctx.production_config_data = NULL;
  startup_ctx.production_config_data_len = 0;
}


void ncp_host_handle_request_production_config_response(zb_ret_t status_code,
                                                        zb_uint16_t production_config_data_len,
                                                        zb_uint8_t *production_config_data)
{
  TRACE_MSG(TRACE_ZDO2, ">> ncp_host_handle_request_production_config_response, status: 0x%x, data len %d",
    (FMT__D_D, status_code, production_config_data_len));

  if (status_code == RET_OK)
  {
    startup_ctx.production_config_data = production_config_data;
    startup_ctx.production_config_data_len = production_config_data_len;
  }
  else
  {
    startup_production_config_data_reset();
  }

  zdo_load_production_config();

  startup_production_config_data_reset();

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}
#endif /* ZB_PRODUCTION_CONFIG && ZB_NCP_PRODUCTION_CONFIG_ON_HOST */

void ncp_host_handle_get_module_version_response(zb_ret_t status_code,
                                                 zb_uint32_t fw_version,
                                                 zb_uint32_t stack_version,
                                                 zb_uint32_t ncp_version)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_get_module_version_response, status: 0x%x",
          (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ncp_host_state_set_module_version(fw_version, stack_version, ncp_version);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}

void ncp_host_handle_set_local_ieee_addr_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_local_ieee_addr_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


void ncp_host_handle_set_end_device_timeout_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_set_end_device_timeout_response, status_code %d",
    (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  if (!ncp_host_state_get_zboss_started())
  {
    ZB_ASSERT(startup_ctx.current_step == PARAM_STEP_ED_TIMEOUT);
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_set_end_device_timeout_response", (FMT__0));
}


void ncp_host_handle_nwk_set_keepalive_mode_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_nwk_set_keepalive_mode_response, status_code %d",
    (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  if (!ncp_host_state_get_zboss_started())
  {
    ZB_ASSERT(startup_ctx.current_step == PARAM_STEP_KEEPALIVE_MODE);
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_nwk_set_keepalive_mode_response", (FMT__0));
}


static void soc_set_next_secur_material(zb_uint8_t param)
{
  zb_ret_t ret = RET_ERROR;
  zb_uint8_t key_idx = 0;
  zb_uint8_t nwk_key[ZB_CCM_KEY_SIZE];

  ZVUNUSED(param);

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_next_secur_material", (FMT__0));

  for (key_idx = startup_ctx.start_secur_material_index; key_idx < ZB_SECUR_N_SECUR_MATERIAL; key_idx++)
  {
    if (ncp_host_state_get_nwk_key(nwk_key, key_idx))
    {
      ret = ncp_host_set_nwk_key(nwk_key, key_idx);
      ZB_ASSERT(ret == RET_OK);

      break;
    }
  }

  /* If there is not nwk key to set on SoC, just continue startup procedure */
  if (key_idx == ZB_SECUR_N_SECUR_MATERIAL)
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_next_secur_material", (FMT__0));
}


void ncp_host_handle_set_nwk_key_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_nwk_key_response, status: 0x%x", (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  startup_ctx.start_secur_material_index++;

  if (startup_ctx.start_secur_material_index == ZB_SECUR_N_SECUR_MATERIAL)
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(soc_set_next_secur_material, 0);
  }
}


static void soc_set_use_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_use_extended_pan_id, ext_pan_id:" TRACE_FORMAT_64,
            (FMT__A, TRACE_ARG_64(ext_pan_id)));

  if (ZB_EXTPANID_IS_ZERO(ext_pan_id))
  {
    TRACE_MSG(TRACE_ZDO2, "use_extended_pan_id is not set, proceed to the next step", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ret = ncp_host_set_zigbee_extended_pan_id(ext_pan_id);
    ZB_ASSERT(ret == RET_OK);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_use_extended_pan_id", (FMT__0));
}


static void soc_set_pan_id(zb_uint16_t pan_id)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, "<< soc_set_pan_id, pan_id: 0x%x", (FMT__D, pan_id));

  if (!pan_id)
  {
    TRACE_MSG(TRACE_ZDO2, "pan_id is not set, proceed to the next step", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ret = ncp_host_set_zigbee_pan_id(pan_id);
    ZB_ASSERT(ret == RET_OK);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_pan_id", (FMT__0));
}


void ncp_host_handle_set_zigbee_pan_id_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_zigbee_pan_id_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


void ncp_host_handle_set_zigbee_extended_pan_id_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_zigbee_extended_pan_id_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_set_device_type(zb_nwk_device_type_t device_type)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_set_device_type, device_type: %d", (FMT__D, device_type));

  ret = ncp_host_set_zigbee_role(device_type);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_device_type", (FMT__0));
}


static void soc_set_max_children(zb_uint8_t max_children)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_set_max_children, max_children: %h", (FMT__H, max_children));

  ret = ncp_host_set_max_children(max_children);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_max_children", (FMT__0));
}


static void soc_set_local_ic(zb_uint8_t ic_type, zb_uint8_t *ic)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_SECUR1, ">> soc_set_local_ic, type: %hd", (FMT__H, ic_type));

  if (
#if defined(ZB_PRODUCTION_CONFIG) && !defined(ZB_NCP_PRODUCTION_CONFIG_ON_HOST)
    ic_type != ZB_IC_TYPE_MAX && !startup_ctx.production_config_presented
#else
    ZB_TRUE
#endif
    )
  {
    TRACE_MSG(TRACE_SECUR1, "  ic: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));

    ret = ncp_host_secur_set_local_ic(ic_type, ic);

    ZB_ASSERT(ret == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_local_ic", (FMT__0));
}


static void soc_set_tc_policy_tc_link_keys_required(zb_bool_t link_keys_required)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_SECUR1, ">> soc_set_tc_policy_tc_link_keys_required, required: %hd", (FMT__H, link_keys_required));

  ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_TC_LINK_KEYS_REQUIRED, link_keys_required);;
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_tc_policy_tc_link_keys_required", (FMT__0));
}



static void soc_set_ic_policy(zb_bool_t allow_ic_only)
{
  zb_ret_t ret = RET_ERROR;
  TRACE_MSG(TRACE_SECUR1, ">> soc_set_ic_policy, allow_ic_only: %hd", (FMT__H, allow_ic_only));

  ret = ncp_host_set_tc_policy(NCP_HL_TC_POLICY_IC_REQUIRED, allow_ic_only);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_ic_policy", (FMT__0));
}


void ncp_host_handle_set_zigbee_role_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_device_type_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


void ncp_host_handle_set_max_children_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_max_children_response, status: 0x%x", (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


void ncp_host_handle_set_tc_policy_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT2, "ncp_host_handle_set_tc_policy_response, status_code %d",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  if (!ncp_host_state_get_zboss_started())
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
}


void ncp_host_handle_set_local_ic_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_local_ic_response, status: 0x%x", (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_set_channel_list(zb_channel_list_t channel_list)
{
  zb_ret_t status = RET_ERROR;
  zb_uint32_t channel_mask_page_0;
  /* TODO: support pages other than page 0. See ZOI-612. */

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_channel_list", (FMT__0));

  if (
#if defined(ZB_PRODUCTION_CONFIG) && !defined(ZB_NCP_PRODUCTION_CONFIG_ON_HOST)
    !startup_ctx.production_config_presented
#else
    ZB_TRUE
#endif
    )
  {
    channel_mask_page_0 = zb_channel_page_list_get_mask(channel_list, ZB_CHANNEL_LIST_PAGE0_IDX);
    TRACE_MSG(TRACE_ZDO2, "page 0 channel mask: 0x%x", (FMT__D, channel_mask_page_0));

    status = ncp_host_set_zigbee_channel_mask(0, channel_mask_page_0);
    ZB_ASSERT(status == RET_OK);
  }
  else
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_channel_list", (FMT__0));
}


void ncp_host_handle_set_zigbee_channel_mask_response(zb_ret_t status_code)
{
  /* TODO: add channel page. See ZOI-612. */
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_zigbee_channel_mask_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_set_rx_on_when_idle(zb_bool_t rx_on)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_rx_on_when_idle, rx_on %d", (FMT__D, rx_on));

  status = ncp_host_set_rx_on_when_idle(rx_on);
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_rx_on_when_idle", (FMT__0));
}


void ncp_host_handle_set_rx_on_when_idle_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_set_rx_on_when_idle_response, status_code 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}

static void soc_set_permit_control4_network(zb_bool_t permit)
{
  TRACE_MSG(TRACE_ZDO2, ">> soc_set_permit_control4_network, permit_on %d", (FMT__D, permit));

  /* TODO: use NCP call when it is implemented */

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_permit_control4_network", (FMT__0));
}


/* Set End Device Timeout */
void soc_set_ed_timeout(zb_uint_t timeout)
{
  zb_ret_t ret = RET_BUSY;

  TRACE_MSG(TRACE_TRANSPORT2, ">> zb_set_ed_timeout", (FMT__0));

  ret = ncp_host_set_end_device_timeout(timeout);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_TRANSPORT2, "<< zb_set_ed_timeout, ret %d", (FMT__D, ret));
}

#ifdef ZB_LIMIT_VISIBILITY
void soc_set_mac_invisible_shorts(void)
{
  zb_ret_t ret;
  zb_uint8_t addresses_count;
  zb_uint16_t *addresses;

  TRACE_MSG(TRACE_TRANSPORT2, ">> soc_set_mac_invisible_shorts, idx %d",
    (FMT__D, startup_ctx.mac_invisible_short_index));

  addresses = ncp_host_state_get_mac_invisible_shorts(&addresses_count);

  if (startup_ctx.mac_invisible_short_index >= addresses_count)
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ret = ncp_host_mac_add_invisible_short(addresses[startup_ctx.mac_invisible_short_index]);
    ZB_ASSERT(ret == RET_OK);

    startup_ctx.mac_invisible_short_index++;
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< soc_set_mac_invisible_shorts, ret %d", (FMT__D, ret));
}


void soc_set_mac_visible_longs(void)
{
  zb_ret_t ret;
  zb_uint8_t addresses_count;
  zb_ieee_addr_t *addresses;

  TRACE_MSG(TRACE_TRANSPORT2, ">> soc_set_mac_visible_longs, idx %d",
    (FMT__D, startup_ctx.mac_visible_long_index));

  addresses = ncp_host_state_get_mac_visible_longs(&addresses_count);

  if (startup_ctx.mac_visible_long_index >= addresses_count)
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ret = ncp_host_mac_add_visible_long(addresses[startup_ctx.mac_visible_long_index]);
    ZB_ASSERT(ret == RET_OK);

    startup_ctx.mac_visible_long_index++;
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< soc_set_mac_visible_longs, ret %d", (FMT__D, ret));
}
#endif /* ZB_LIMIT_VISIBILITY */

/* Set Keepalive Mode */
void soc_set_keepalive_mode(nwk_keepalive_supported_method_t mode)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_TRANSPORT2, ">> soc_set_keepalive_mode", (FMT__0));

  ret = ncp_host_nwk_set_keepalive_mode(mode);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_TRANSPORT2, "<< soc_set_keepalive_mode, ret %d", (FMT__D, ret));
}


static void soc_set_simple_descriptor(zb_af_simple_desc_1_1_t *dsc)
{
  zb_ret_t ret;

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_simple_descriptor", (FMT__0));

  TRACE_MSG(TRACE_ZDO3,
            "dsc %p ep %hd prof_id 0x%x dev_id 0x%x dev_ver 0x%hx in_clu_c %hd out_clu_c %hd",
            (FMT__P_H_D_D_D_H_H, dsc, dsc->endpoint, dsc->app_profile_id, dsc->app_device_id,
             dsc->app_device_version, dsc->app_input_cluster_count, dsc->app_output_cluster_count));

  /* TODO: support cluster filtering, see ZB_FILTER_OUT_CLUSTERS.
           It could be needed for SE and SubGHz support. See ZOI-615 */

  ret = ncp_host_af_set_simple_descriptor(dsc->endpoint,
                                          dsc->app_profile_id,
                                          dsc->app_device_id,
                                          dsc->app_device_version,
                                          dsc->app_input_cluster_count,
                                          dsc->app_output_cluster_count,
                                          dsc->app_cluster_list,
                                          &dsc->app_cluster_list[dsc->app_input_cluster_count]);
  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_simple_descriptor", (FMT__0));
}

static void soc_set_next_simple_descriptor(void);

void ncp_host_handle_af_set_simple_descriptor_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_af_set_simple_descriptor_response, status: 0x%x",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  startup_ctx.simple_desc_index++;
  soc_set_next_simple_descriptor();
}


static void soc_set_next_simple_descriptor(void)
{
  zb_af_endpoint_desc_t *ep = NULL;

  TRACE_MSG(TRACE_ZDO2, ">> soc_set_next_simple_descriptor", (FMT__0));
  TRACE_MSG(TRACE_ZDO2, "current desc index: %d", (FMT__D, startup_ctx.simple_desc_index));

  if (ZCL_CTX().device_ctx == NULL)
  {
    TRACE_MSG(TRACE_ZDO2, "device context is not registered, move to next step", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else if (startup_ctx.simple_desc_index == ZCL_CTX().device_ctx->ep_count)
  {
    TRACE_MSG(TRACE_ZDO2, "all descriptors are set", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ep = ZCL_CTX().device_ctx->ep_desc_list[startup_ctx.simple_desc_index];

    zb_zcl_init_endpoint(ep);
    soc_set_simple_descriptor(ep->simple_desc);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_set_next_simple_descriptor", (FMT__0));
}


static void setting_param_dispatch(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> setting_param_dispatch, step %d",
            (FMT__D, startup_ctx.current_step));

  switch(startup_ctx.current_step)
  {
    case PARAM_STEP_RESET:
      /* wait in case SoC is going to send an unsolicited reset response */
      ZB_SCHEDULE_ALARM(soc_reset, ncp_host_state_get_nvram_erase(), DELAY_BEFORE_RESET);
      break;
    case PARAM_STEP_REQUEST_MODULE_VERSION:
    {
      soc_request_module_version();
      break;
    }
#ifdef ZB_PRODUCTION_CONFIG
#ifdef ZB_NCP_PRODUCTION_CONFIG_ON_HOST
    case PARAM_STEP_REQUEST_PRODUCTION_CONFIG:
    {
      soc_request_production_config();
      break;
    }
#else
    case PARAM_STEP_REQUEST_SERIAL_NUMBER:
    {
      soc_request_serial_number();
      break;
    }
    case PARAM_STEP_REQUEST_CHANNEL_MASK:
    {
      soc_request_channel_mask();
      break;
    }
    case PARAM_STEP_REQUEST_LONG_ADDR:
    {
      soc_request_long_addr();
      break;
    }
    case PARAM_STEP_REQUEST_LOCAL_IC:
    {
      soc_request_local_ic();
      break;
    }
    case PARAM_STEP_REQUEST_VENDOR_DATA:
    {
      soc_request_vendor_data();
      break;
    }
#endif /* ZB_NCP_PRODUCTION_CONFIG_ON_HOST */
#endif /* ZB_PRODUCTION_CONFIG */

    case PARAM_STEP_LONG_ADDR:
    {
      zb_ieee_addr_t long_addr;
      ncp_host_state_get_long_addr(long_addr);
      soc_set_long_addr(long_addr);
      break;
    }
    case PARAM_STEP_SECUR_MATERIAL:
    {
      soc_set_next_secur_material(0);
      break;
    }
    case PARAM_STEP_USE_EXTENDED_PAN_ID:
    {
      zb_ext_pan_id_t use_ext_pan_id;
      ncp_host_state_get_use_extended_pan_id(use_ext_pan_id);
      soc_set_use_extended_pan_id(use_ext_pan_id);
      break;
    }
    case PARAM_STEP_PAN_ID:
      soc_set_pan_id(ncp_host_state_get_pan_id());
      break;
    case PARAM_STEP_DEVICE_TYPE:
      soc_set_device_type(ncp_host_state_get_device_type());
      break;
    case PARAM_STEP_MAX_CHILDREN:
      soc_set_max_children(ncp_host_state_get_max_children());
      break;
    case PARAM_STEP_TC_POLICY_TC_LINK_KEYS_REQUIRED:
      soc_set_tc_policy_tc_link_keys_required(ncp_host_state_get_tc_policy_tc_link_keys_required());
      break;
#ifdef ZB_SECURITY_INSTALLCODES
    case PARAM_STEP_IC_POLICY:
      soc_set_ic_policy(ncp_host_state_get_allow_ic_only());
      break;
    case PARAM_STEP_LOCAL_IC_SET:
      soc_set_local_ic(ncp_host_state_get_ic_type(), ncp_host_state_get_ic());
      break;
#endif /* ZB_SECURITY_INSTALLCODES */
    case PARAM_STEP_CHANNEL_LIST:
    {
      zb_channel_list_t channel_list;
      ncp_host_state_get_channel_list(channel_list);
      soc_set_channel_list(channel_list);
      break;
    }
    case PARAM_STEP_RX_ON_WHEN_IDLE:
      soc_set_rx_on_when_idle(ncp_host_state_get_rx_on_when_idle());
      break;
    case PARAM_STEP_PERMIT_CONTROL4_NETWORK:
      soc_set_permit_control4_network(ncp_host_state_get_permit_control4_network());
      break;
    case PARAM_STEP_KEEPALIVE_MODE:
      if (ncp_host_state_keepalive_mode_is_set())
      {
        soc_set_keepalive_mode(ncp_host_state_get_keepalive_mode());
      }
      else
      {
        ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
      }
      break;
    case PARAM_STEP_ED_TIMEOUT:
      if (ncp_host_state_ed_timeout_is_set())
      {
        soc_set_ed_timeout(ncp_host_state_get_ed_timeout());
      }
      else
      {
        ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
      }
      break;
#ifdef ZB_LIMIT_VISIBILITY
    case PARAM_STEP_MAC_INVISIBLE_SHORTS:
    {
      soc_set_mac_invisible_shorts();
      break;
    }
    case PARAM_STEP_MAC_VISIBLE_LONGS:
    {
      soc_set_mac_visible_longs();
      break;
    }
#endif /* ZB_LIMIT_VISIBILITY */
    case PARAM_STEP_SETTING_SIMPLE_DESCRIPTORS:
      soc_set_next_simple_descriptor();
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "incorrect setting param step", (FMT__0));
      ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< setting_param_dispatch", (FMT__0));
}


static void soc_get_authenticated_status(void)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_authenticated_status", (FMT__0));

  status = ncp_host_get_authentication_status();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_authenticated_status", (FMT__0));
}


void ncp_host_handle_get_authentication_status_response(zb_ret_t status_code, zb_bool_t authenticated)
{
  TRACE_MSG(TRACE_ZDO2,
            "ncp_host_handle_get_authentication_status_response, status_code 0x%x, auth %d",
            (FMT__D_D, status_code, authenticated));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_authenticated(authenticated);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_parent_address(void)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_parent_address", (FMT__0));

  status = ncp_host_get_parent_address();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_parent_address", (FMT__0));
}


void ncp_host_handle_get_parent_address_response(zb_ret_t status_code, zb_uint16_t parent_address)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_get_parent_address_response, status 0x%x, parent 0x%x",
            (FMT__D_D, status_code, parent_address));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_parent_short_address(parent_address);
  }

  if (!ncp_host_state_get_zboss_started())
  {
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
}


static void soc_get_extended_pan_id(void)
{
  zb_ret_t status = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_get_extended_pan_id", (FMT__0));

  status = ncp_host_get_extended_pan_id();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_extended_pan_id", (FMT__0));
}


void ncp_host_handle_get_extended_pan_id_response(zb_ret_t status_code,
                                                  const zb_ieee_addr_t ext_pan_id)
{
  TRACE_MSG(TRACE_ZDO2,
            "ncp_host_handle_get_extended_pan_id_response, status 0x%x, ext_pan_id " TRACE_FORMAT_64,
            (FMT__D_A, status_code, TRACE_ARG_64(ext_pan_id)));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_extended_pan_id(ext_pan_id);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_pan_id(void)
{
  zb_ret_t status = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_get_pan_id", (FMT__0));

  status = ncp_host_get_zigbee_pan_id();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_pan_id", (FMT__0));
}


void ncp_host_handle_get_zigbee_pan_id_response(zb_ret_t status_code, zb_uint16_t pan_id)
{
  TRACE_MSG(TRACE_ZDO2,
            "ncp_host_handle_get_zigbee_pan_id_response, status 0x%x, pan_id 0x%x",
            (FMT__D_D, status_code, pan_id));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_pan_id(pan_id);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_coordinator_version(void)
{
  zb_ret_t status = RET_ERROR;
  TRACE_MSG(TRACE_ZDO2, ">> soc_get_coordinator_version", (FMT__0));

  status = ncp_host_get_coordinator_version();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_coordinator_version", (FMT__0));
}


void ncp_host_handle_get_coordinator_version_response(zb_ret_t status_code, zb_uint8_t version)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_get_coordinator_version_response, status 0x%x, version %d",
            (FMT__D_D, status_code, version));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_coordinator_version(version);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_network_address(void)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_network_address", (FMT__0));

  status = ncp_host_get_short_address();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_network_address", (FMT__0));
}


void ncp_host_handle_get_short_address_response(zb_ret_t status_code, zb_uint16_t address)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_get_short_address_response, status 0x%x, address 0x%x",
            (FMT__D_D, status_code, address));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_short_address(address);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_current_page_and_channel(void)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_current_page_and_channel", (FMT__0));

  status = ncp_host_get_zigbee_channel();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_current_page_and_channel", (FMT__0));
}


void ncp_host_handle_get_zigbee_channel_response(zb_ret_t status_code,
                                                 zb_uint8_t page,
                                                 zb_uint8_t channel)
{
  TRACE_MSG(TRACE_ZDO2,
            "ncp_host_handle_get_zigbee_channel_response, status 0x%x, page %d, channel %d",
            (FMT__D_D_D, status_code, page, channel));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    ncp_host_state_set_current_page(page);
    ncp_host_state_set_current_channel(channel);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_trust_center_address(void)
{
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_trust_center_address", (FMT__0));

  status = ncp_host_get_trust_center_address();
  ZB_ASSERT(status == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_trust_center_address", (FMT__0));
}


void ncp_host_handle_get_trust_center_address_response(zb_ret_t status_code,
                                                       const zb_ieee_addr_t address)
{
  TRACE_MSG(TRACE_ZDO2,
            "ncp_host_handle_get_trust_center_address_response, status 0x%x, address " TRACE_FORMAT_64,
            (FMT__D_A, status_code, TRACE_ARG_64(address)));

  ZB_ASSERT(status_code == RET_OK);

  if (status_code == RET_OK)
  {
    zb_aib_set_trust_center_address(address);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_tclk(void)
{
  zb_ieee_addr_t trust_center_address;
  zb_ret_t status = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_tclk", (FMT__0));

  if (!zb_aib_trust_center_address_unknown()
      && !zb_aib_trust_center_address_zero())
  {
    zb_aib_get_trust_center_address(trust_center_address);
    status = ncp_host_get_aps_key_by_ieee(trust_center_address);
    ZB_ASSERT(status == RET_OK);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO2, "trust center address is unknown, skip this step", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_tclk", (FMT__0));
}


void ncp_host_handle_get_aps_key_by_ieee_response(zb_ret_t status_code, const zb_uint8_t *key)
{
  TRACE_MSG(TRACE_ZDO2, "ncp_host_handle_get_aps_key_by_ieee_response, status 0x%x",
            (FMT__D, status_code));

  /* TODO: check key source and type, key check depends on commissioning type */
  if (status_code == RET_OK)
  {
    TRACE_MSG(TRACE_ZDO2, "key: " TRACE_FORMAT_128, (FMT__AA, TRACE_ARG_128(key)));
    ncp_host_state_set_tclk_valid(ZB_TRUE);
  }
  else
  {
    ncp_host_state_set_tclk_valid(ZB_FALSE);
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
}


static void soc_get_address_table(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> soc_get_address_table", (FMT__0));

  /* TODO: request address table from SoC, when it is needed.
           As of 02/26/21 all SDK and HA samples work fine without it.
           Most probably we don't need it at all. */

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_address_table", (FMT__0));
}


static void soc_get_aps_keys_table(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> soc_get_aps_keys_table", (FMT__0));

  /* TODO: request aps keys table from SoC when it is needed. See ZOI-201. */

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_aps_keys_table", (FMT__0));
}


static void soc_get_group_table(void)
{
  zb_ret_t ret = RET_ERROR;

  TRACE_MSG(TRACE_ZDO2, ">> soc_get_group_table", (FMT__0));

  ret = ncp_host_aps_get_group_table_request();

  ZB_ASSERT(ret == RET_OK);

  TRACE_MSG(TRACE_ZDO2, "<< soc_get_group_table", (FMT__0));
}


static void soc_read_nvram(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> soc_read_nvram", (FMT__0));

  startup_ctx.current_dataset++;
  while(!ncp_host_nvram_dataset_is_supported(startup_ctx.current_dataset)
        && startup_ctx.current_dataset < ZB_NVRAM_DATASET_NUMBER)
  {
    startup_ctx.current_dataset++;
  }

  if (startup_ctx.current_dataset == ZB_NVRAM_DATASET_NUMBER)
  {
    TRACE_MSG(TRACE_ZDO2, "All datasets are read", (FMT__0));
    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ncp_host_nvram_read_request((zb_uint16_t)startup_ctx.current_dataset);
  }

  TRACE_MSG(TRACE_ZDO2, "<< soc_read_nvram", (FMT__0));
}


void ncp_host_handle_nvram_read_response(zb_ret_t status_code,
                                         zb_uint8_t *buf, zb_uint16_t len,
                                         zb_nvram_dataset_types_t ds_type,
                                         zb_uint16_t ds_version,
                                         zb_nvram_ver_t nvram_version)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_nvram_read_response, status %d",
            (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK
            || status_code == RET_NOT_FOUND);

  if (status_code == RET_OK)
  {
    ncp_host_nvram_read_dataset(ds_type, buf, len, ds_version, nvram_version);
  }

  soc_read_nvram();

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_nvram_read_response", (FMT__0));
}


void ncp_host_handle_aps_get_group_table_response(zb_ret_t status_code,
                                                  zb_aps_group_table_ent_t* table,
                                                  zb_size_t entry_cnt)
{
  zb_aps_group_table_t *group_table = NULL;

  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_aps_get_group_table_request, status 0x%x",
            (FMT__D, status_code));

  group_table = ncp_host_state_get_group_table();

  if (status_code == RET_OK)
  {
    ZB_MEMCPY(group_table, table, sizeof(*table) * entry_cnt);
    group_table->n_groups = entry_cnt;
  }

  ZB_SCHEDULE_CALLBACK(startup_next_step, 0);

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_aps_get_group_table_request", (FMT__0));
}

#ifdef ZB_LIMIT_VISIBILITY
void ncp_host_handle_mac_add_invisible_short_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_mac_add_invisible_short_response, status_code %d",
    (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  if (!ncp_host_state_get_zboss_started())
  {
    /* Try to set new invisible short address */
    soc_set_mac_invisible_shorts();
  }

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_mac_add_invisible_short_response", (FMT__0));
}


void ncp_host_handle_mac_add_visible_long_response(zb_ret_t status_code)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_mac_add_visible_long_response, status_code %d",
    (FMT__D, status_code));

  ZB_ASSERT(status_code == RET_OK);

  if (!ncp_host_state_get_zboss_started())
  {
    /* Try to set new invisible short address */
    soc_set_mac_visible_longs();
  }

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_mac_add_visible_long_response", (FMT__0));
}
#endif /* ZB_LIMIT_VISIBILITY */


void ncp_host_handle_get_local_ieee_addr_response(zb_ret_t status_code,
                                                  zb_ieee_addr_t addr,
                                                  zb_uint8_t interface_num)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_get_local_ieee_addr_response, status 0x%x",
            (FMT__D, status_code));

  TRACE_MSG(TRACE_TRANSPORT2, "  interface_num %hd, addr " TRACE_FORMAT_64,
            (FMT__H_A, interface_num, TRACE_ARG_64(addr)));

  ZB_ASSERT(interface_num == 0 && "It is assumed that MAC interface is always equals to 0");

  if (!ncp_host_state_get_zboss_started())
  {
    if (status_code == RET_OK)
    {
      ncp_host_state_set_long_addr(addr);
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT2, "Impossible to request local IEEE address", (FMT__0));

      ZB_ASSERT(ZB_FALSE);
    }

    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(status_code == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_get_local_ieee_addr_response", (FMT__0));
}


void ncp_host_handle_secur_get_local_ic_response(zb_ret_t status_code, zb_uint8_t ic_type, const zb_uint8_t *ic)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_secur_get_local_ic_response, status 0x%x",
            (FMT__D, status_code));

  TRACE_MSG(TRACE_TRANSPORT3, "  ic_type: %hd", (FMT__H, ic_type));
  TRACE_MSG(TRACE_TRANSPORT3, "  ic: " TRACE_FORMAT_128, (FMT__B, TRACE_ARG_128(ic)));

  if (!ncp_host_state_get_zboss_started())
  {
    if (status_code == RET_OK)
    {
      ncp_host_state_set_ic(ic_type, ic);
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT2, "Impossible to request local installcode", (FMT__0));

      ZB_ASSERT(ZB_FALSE);
    }

    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(status_code == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_secur_get_local_ic_response", (FMT__0));
}


void ncp_host_handle_get_zigbee_channel_mask_response(zb_ret_t status_code,
                                                      zb_uint8_t channel_list_len,
                                                      zb_uint8_t channel_page,
                                                      zb_uint32_t channel_mask)
{
  TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_get_zigbee_channel_mask_response, status 0x%x",
            (FMT__D, status_code));

  TRACE_MSG(TRACE_TRANSPORT3, "  channel_list_len: %hd, channel_page %hd, channel_mask %lx",
    (FMT__H_H_L, channel_list_len, channel_page, channel_mask));

  if (!ncp_host_state_get_zboss_started())
  {
    if (status_code == RET_OK)
    {
      zb_channel_list_t channel_list;
      zb_channel_list_init(channel_list);
      zb_channel_page_list_set_2_4GHz_mask(channel_list, channel_mask);

      zb_set_bdb_primary_channel_set(channel_mask);

      /* TODO: add support of other pages except PAGE0_2_4_GHZ. See ZOI-612. */
      ZB_ASSERT(channel_list_len == 1);
      ZB_ASSERT(channel_page == ZB_CHANNEL_PAGE0_2_4_GHZ);

      ncp_host_state_set_channel_list(channel_list);
    }
    else
    {
      TRACE_MSG(TRACE_TRANSPORT2, "Impossible to request local channel mask", (FMT__0));

      ZB_ASSERT(ZB_FALSE);
    }

    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(status_code == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_get_zigbee_channel_mask_response", (FMT__0));
}


#if defined ZB_PRODUCTION_CONFIG && !defined(NCP_PRODUCTION_CONFIG_ON_HOST)
void ncp_host_handle_get_serial_number_response(zb_ret_t status_code, const zb_uint8_t *serial_number)
{
  zb_uint8_t empty_serial[ZB_NCP_SERIAL_NUMBER_LENGTH];

  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_get_serial_number_response, status 0x%x",
    (FMT__D, status_code));

  ZB_BZERO(empty_serial, sizeof(empty_serial));

#if TRACE_ENABLED(TRACE_TRANSPORT3)
  {
    zb_uindex_t serial_byte_index;
    for (serial_byte_index = 0; serial_byte_index < ZB_NCP_SERIAL_NUMBER_LENGTH; serial_byte_index++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, " %hd", (FMT__H, serial_number[serial_byte_index]));
    }
  }
#endif /* TRACE_ENABLED(TRACE_TRANSPORT3) */

  if (!ncp_host_state_get_zboss_started())
  {
    startup_ctx.production_config_presented = ZB_MEMCMP(serial_number, empty_serial, sizeof(empty_serial)) != 0;

    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(status_code == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT3, " production_config_presented %hd", (FMT__H, startup_ctx.production_config_presented));

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_get_serial_number_response", (FMT__0));
}


void ncp_host_handle_get_vendor_data_response(zb_ret_t status_code, zb_uint8_t vendor_data_size, const zb_uint8_t *vendor_data)
{
  TRACE_MSG(TRACE_TRANSPORT3, ">> ncp_host_handle_get_vendor_data_response, status 0x%x",
    (FMT__D, status_code));

  TRACE_MSG(TRACE_TRANSPORT3, "  vendor_data_size %hd", (FMT__H, vendor_data_size));

#if TRACE_ENABLED(TRACE_TRANSPORT3)
  {
    zb_uindex_t vendor_byte_index;
    for (vendor_byte_index = 0; vendor_byte_index < vendor_data_size; vendor_byte_index++)
    {
      TRACE_MSG(TRACE_TRANSPORT3, " %hd", (FMT__H, vendor_data[vendor_byte_index]));
    }
  }
#endif /* TRACE_ENABLED(TRACE_TRANSPORT3) */

  if (!ncp_host_state_get_zboss_started())
  {
    zb_uint8_t param;
    zb_uint8_t *application_production_config;

    param = zb_buf_get(ZB_TRUE, 0);
    ZB_ASSERT(param != ZB_BUF_INVALID);

    if (status_code == RET_OK)
    {
      application_production_config = zb_app_signal_pack(param,
                                                        ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY,
                                                        0, vendor_data_size);
      ZB_MEMCPY(application_production_config, vendor_data, vendor_data_size);
    }
    else
    {
      zb_app_signal_pack(param, ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, RET_ERROR, 0);
    }

    ZB_SCHEDULE_CALLBACK(zboss_signal_handler, param);

    ZB_SCHEDULE_CALLBACK(startup_next_step, 0);
  }
  else
  {
    ZB_ASSERT(status_code == RET_OK);
  }

  TRACE_MSG(TRACE_TRANSPORT3, "<< ncp_host_handle_get_vendor_data_response", (FMT__0));
}

#endif /* ZB_PRODUCTION_CONFIG && NCP_PRODUCTION_CONFIG_ON_HOST */

static void reading_soc_state_dispatch(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> reading_soc_state_dispatch, step %d",
            (FMT__D, startup_ctx.current_step));

  switch(startup_ctx.current_step)
  {
    case SOC_STATE_STEP_AUTHENTICATED:
      soc_get_authenticated_status();
      break;
    case SOC_STATE_STEP_PARENT_ADDRESS:
      soc_get_parent_address();
      break;
    case SOC_STATE_STEP_EXTENDED_PAN_ID:
      soc_get_extended_pan_id();
      break;
    case SOC_STATE_STEP_PAN_ID:
      soc_get_pan_id();
      break;
    case SOC_STATE_STEP_COORDINATOR_VERSION:
      soc_get_coordinator_version();
      break;
    case SOC_STATE_STEP_NETWORK_ADDRESS:
      soc_get_network_address();
      break;
    case SOC_STATE_STEP_CURRENT_PAGE_AND_CHANNEL:
      soc_get_current_page_and_channel();
      break;
    case SOC_STATE_STEP_TRUST_CENTER_ADDRESS:
      soc_get_trust_center_address();
      break;
    case SOC_STATE_STEP_TCLK:
      soc_get_tclk();
      break;
    case SOC_STATE_STEP_ADDRESS_TABLE:
      soc_get_address_table();
      break;
    case SOC_STATE_STEP_APS_KEYS_TABLE:
      soc_get_aps_keys_table();
      break;
    case SOC_STATE_STEP_GROUP_TABLE:
      soc_get_group_table();
      break;
    case SOC_STATE_STEP_READ_NVRAM:
      soc_read_nvram();
      break;

    default:
      TRACE_MSG(TRACE_ERROR, "incorrect reading soc state step", (FMT__0));
      ZB_ASSERT(0);
  }

  TRACE_MSG(TRACE_ZDO2, "<< reading_soc_state_dispatch", (FMT__0));
}


static void startup_done(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> startup_done", (FMT__0));

  ncp_host_state_set_zboss_started(ZB_TRUE);

  if (startup_ctx.autostart)
  {
    zboss_start_continue();
  }
  else
  {
    zb_buf_get_out_delayed(zb_send_no_autostart_signal);
  }

  TRACE_MSG(TRACE_ZDO2, "<< startup_done", (FMT__0));
}

static void startup_dispatch(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> startup_dispatch, stage %d", (FMT__D, startup_ctx.current_stage));
  switch(startup_ctx.current_stage)
  {
    case STARTUP_STAGE_SETTING_PARAMS:
      setting_param_dispatch();
      break;
    case STARTUP_STAGE_READING_SOC_STATE:
      reading_soc_state_dispatch();
      break;
    case STARTUP_STAGE_DONE:
      startup_done();
      break;
    default:
      TRACE_MSG(TRACE_ERROR, "incorrect startup stage", (FMT__0));
      ZB_ASSERT(0);
  }
  TRACE_MSG(TRACE_ZDO2, "<< startup_dispatch", (FMT__0));
}

static void begin_startup(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> begin_startup", (FMT__0));

  /* All params are zeroed on start, so just call dispatcher here */
  startup_dispatch();

  TRACE_MSG(TRACE_ZDO2, "<< begin_startup", (FMT__0));
}


zb_ret_t zboss_start(void)
{
  zb_ret_t ret = RET_ERROR;
  ZB_ASSERT(!ncp_host_state_get_zboss_started());

  startup_ctx.autostart = ZB_TRUE;
  ret = zboss_start_no_autostart();

  /* Default FFD descriptors will be set at device.
     1. call zboss_start_no_autostart
     2. start commissioning_cb
  */
  return ret;
}


void zboss_start_continue(void)
{
  TRACE_MSG(TRACE_ZDO2, ">> zboss_start_continue", (FMT__0));

  zb_buf_get_out_delayed(zdo_commissioning_start);

  TRACE_MSG(TRACE_ZDO2, "<< zboss_start_continue", (FMT__0));
}


static void init_transport(zb_uint8_t param)
{
  startup_ctx.transport_error_detected = ZB_FALSE;

  TRACE_MSG(TRACE_ZDO2, "try to (re)init transport", (FMT__0));

  ncp_host_hl_proto_init();

  if (startup_ctx.transport_error_detected)
  {
    ZB_SCHEDULE_ALARM(init_transport, param, TRANSPORT_REINIT_TIMEOUT);
  }
  else
  {
    TRACE_MSG(TRACE_ZDO2, "transport inited", (FMT__0));
    if (param == INIT_PARAM_FIRST)
    {
      begin_startup();
    }
  }
}


/*
   load production config - config is on the dev side - not needed?
   set node descriptor - Set Node Descriptor
   After setting Node Descriptor and other stuff send manually ZB_ZDO_SIGNAL_SKIP_STARTUP?
 */
zb_ret_t zboss_start_no_autostart(void)
{
  zb_ret_t ret = RET_ERROR;
  ZB_ASSERT(!ncp_host_state_get_zboss_started());

  zb_timer_enable_stop();

  ret = ncp_host_state_validate_start_state();

  ZB_ASSERT(ret == RET_OK);
  if (ret == RET_OK)
  {
    init_transport(INIT_PARAM_FIRST);
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "invalid start params", (FMT__0));
  }

  return ret;
}

#ifdef HAVE_TOP_LEVEL_ERROR_HANDLER
zb_bool_t zb_error_top_level_handler(zb_uint8_t severity, zb_ret_t err_code, void *additional_info)
{
  zb_bool_t ret = ZB_FALSE;

  ZVUNUSED(severity);
  ZVUNUSED(additional_info);

#ifdef ZB_HAVE_SERIAL
  if (err_code == ERROR_CODE(ERROR_CATEGORY_SERIAL, ZB_ERROR_SERIAL_INIT_FAILED))
  {
    if (startup_ctx.current_stage == 0
        && startup_ctx.current_step == 0)
    {
      startup_ctx.transport_error_detected = ZB_TRUE;
      ret = ZB_TRUE;
    }
  }
  else if (err_code == ERROR_CODE(ERROR_CATEGORY_SERIAL, ZB_ERROR_SERIAL_READ_FAILED))
  {
    /* probably, we sent a reset request to the SoC and we need to reinitialize the transport */
    if (startup_ctx.current_stage == STARTUP_STAGE_SETTING_PARAMS
        && startup_ctx.current_step == PARAM_STEP_RESET)
    {
      TRACE_MSG(TRACE_ERROR, "read error, SoC reset expected", (FMT__0));
      startup_ctx.transport_error_detected = ZB_TRUE;
      ret = ZB_TRUE;
      init_transport(INIT_PARAM_REINIT);
    }
  }
#endif /* ZB_HAVE_SERIAL */

  return ret;
}
#endif /* HAVE_TOP_LEVEL_ERROR_HANDLER */


void ncp_host_init(void)
{
  ncp_host_adapter_init_ctx();
  ncp_host_proto_configuration_init_ctx();

  ncp_host_state_init();
}
