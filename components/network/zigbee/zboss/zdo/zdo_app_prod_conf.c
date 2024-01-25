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
/* PURPOSE: Production configuration load moved there from zdo_app.c
*/

#define ZB_TRACE_FILE_ID 11
#include "zb_common.h"
#include "zb_scheduler.h"
#include "zb_bufpool.h"
#include "zb_nwk.h"
#include "zb_nwk_nib.h"
#include "zb_aps.h"
#include "zb_zdo.h"
#include "zdo_common.h"
#include "zb_secur.h"
#include "zb_secur_api.h"
#include "zb_nvram.h"
#include "zb_bdb_internal.h"
#include "zb_watchdog.h"
#include "zb_ncp.h"
#include "zdo_wwah_parent_classification.h"

#if defined ZB_ENABLE_ZLL
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_nwk_features.h"
#endif /* defined ZB_ENABLE_ZLL */


#ifdef ZB_PRODUCTION_CONFIG

static zb_bool_t zb_production_config_disabled = ZB_FALSE;

/* Currently production config size is read in buffer to avoid static allocation for it
 * Added safe check in case ZB_PRODUCTION_CONFIG_APP_MAX_SIZE is changed blindly.
 */
ZB_ASSERT_COMPILE_DECL(ZB_PRODUCTION_CONFIG_APP_MAX_SIZE < ZB_IO_BUF_SIZE);

#if !defined(NCP_MODE_HOST) && !defined(ZB_NCP_PRODUCTION_CONFIG_ON_HOST)
zb_bool_t zb_production_configuration_check_presence(void)
{
  return zb_osif_prod_cfg_check_presence();
}


zb_ret_t zb_production_cfg_read_header(zb_uint8_t *prod_cfg_hdr, zb_uint16_t hdr_len)
{
  return zb_osif_prod_cfg_read_header(prod_cfg_hdr, hdr_len);
}


zb_ret_t zb_production_cfg_read(zb_uint8_t *buffer, zb_uint16_t len, zb_uint16_t offset)
{
   return zb_osif_prod_cfg_read(buffer, len, offset);
}

#endif /* !NCP_MODE_HOST && !ZB_NCP_PRODUCTION_CONFIG_ON_HOST */

static zb_uint32_t zb_production_config_calculate_crc(zb_production_config_hdr_t *prod_cfg_hdr)
{
#define READ_PORTION_SIZE 32
  zb_uint32_t crc = 0;
  zb_int16_t len = (zb_int16_t)prod_cfg_hdr->len - (zb_int16_t)sizeof(zb_uint32_t);
  zb_uint16_t offset = (zb_uint16_t)sizeof(zb_uint32_t);
  zb_uint8_t buf[READ_PORTION_SIZE];
  zb_uint8_t read_portion;
  zb_ret_t ret = RET_OK;

  while (len > 0 && ret == RET_OK)
  {
    read_portion = (len > READ_PORTION_SIZE) ? (zb_uint8_t)READ_PORTION_SIZE : (zb_uint8_t)len;
    ret = zb_production_cfg_read(buf, read_portion, offset);

    len -= (zb_int16_t)read_portion;
    offset += read_portion;

    if (ZB_PRODUCTION_CONFIG_VERSION_1_0 == prod_cfg_hdr->version)
    {
      crc = zb_crc32_next_v2(crc, buf, read_portion);
    }
    else if (ZB_PRODUCTION_CONFIG_VERSION_2_0 == prod_cfg_hdr->version)
    {
      crc = zb_crc32_next(crc, buf, (int)read_portion);
    }
    else
    {
      ZB_ASSERT(ZB_FALSE);
      TRACE_MSG(TRACE_ERROR, "found unknown production config version", (FMT__0));
    }
  }

  return crc;
}


void zb_production_config_disable(zb_bool_t val)
{
  zb_production_config_disabled = val;
}


zb_bool_t zb_is_production_config_disabled(void)
{
  return zb_production_config_disabled;
}


zb_ret_t zb_production_config_check(zb_production_config_hdr_t *prod_cfg_hdr)
{
  zb_ret_t ret = RET_OK;
  zb_uint32_t calc_crc;

  if (zb_is_production_config_disabled())
  {
    TRACE_MSG(TRACE_ERROR, "production config is disabled", (FMT__0));
    ret = RET_ERROR;
  }

  /* Check presence of production configuration in memory */
  if (!zb_production_configuration_check_presence())
  {
    TRACE_MSG(TRACE_INFO1, "no production config block found", (FMT__0));
    ret = RET_ERROR;
  }

  /* Check valid CRC */
  if (ret == RET_OK)
  {
    ret = zb_production_cfg_read_header((zb_uint8_t *)prod_cfg_hdr, (zb_uint16_t)sizeof(zb_production_config_hdr_t));

    /*cstat !MISRAC2012-Rule-14.3_b */
    /** @mdr{00009,0} */
    if (ret != RET_OK)
    {
      TRACE_MSG(TRACE_ERROR, "unable to read prod_cfg header", (FMT__0));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    calc_crc = zb_production_config_calculate_crc(prod_cfg_hdr);

    TRACE_MSG(TRACE_COMMON3, "zb_get_production_config crc %lx expected %lx",
              (FMT__L_L, calc_crc, prod_cfg_hdr->crc));

    if (calc_crc != prod_cfg_hdr->crc)
    {
      TRACE_MSG(TRACE_ERROR, "production configuration crc mismatch", (FMT__0));
      ret = RET_ERROR;
    }
  }

  return ret;
}


/* TODO: move*/
#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
static zb_ret_t zb_production_config_fetch_tx_power(zb_uint8_t page, zb_uint8_t channel, zb_int8_t *out_dbm)
{
  zb_ret_t ret = RET_INVALID_PARAMETER;
  zb_uint8_t array_idx = (zb_uint8_t)-1;
  zb_uint8_t array_ofs = (zb_uint8_t)-1;

  /* get TX power offsets */
  zb_channel_get_tx_power_offset(page, channel, &array_idx, &array_ofs);

  TRACE_MSG(TRACE_ZDO3,
            "zb_production_config_fetch_tx_power page %hd channel %hd arr_idx %hd arr_ofs %hd",
            (FMT__H_H_H_H, page, channel, array_idx, array_ofs));

  if (array_idx != (zb_uint8_t)-1
      && array_ofs != (zb_uint8_t)-1)
  {
    ret = zb_production_cfg_read(
                (zb_uint8_t *)out_dbm,
                (zb_uint16_t)sizeof(zb_uint8_t),
                (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t, mac_tx_power[array_idx]) + (zb_uint16_t)array_ofs);
  }

  return ret;
}

#endif  /* ZB_MAC_CONFIGURABLE_TX_POWER */

static zb_ret_t zb_production_config_load_ver_1_0(void)
{
  /* len of read_buf is according to the largest field of zb_production_config_ver_1_t */
  zb_uint8_t read_buf[ZB_CCM_KEY_SIZE+ZB_CCM_KEY_CRC_SIZE] = { 0 };
  zb_uint32_t aps_channel_mask = 0;
  zb_ret_t ret;

  /* APS Channel Mask */
  ret = zb_production_cfg_read(
    read_buf,
    (zb_uint16_t)sizeof(zb_uint32_t),
    (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_1_t, aps_channel_mask));

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,1} */
  if (RET_OK == ret)
  {
    ZB_MEMCPY(&aps_channel_mask, read_buf, sizeof(aps_channel_mask));
    zb_set_channel_mask(aps_channel_mask);

    TRACE_MSG(TRACE_ZDO1, "aps_mask 2.4 0x%lx", (FMT__L, aps_channel_mask));

    ret = zb_production_cfg_read(
      read_buf,
      (zb_uint16_t)sizeof(zb_ieee_addr_t),
      (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_1_t, extended_address));
  }

  /* IEEE Address */
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,2} */
  if (RET_OK == ret)
  {
    zb_ieee_addr_t extended_address;

    ZB_MEMCPY(extended_address, read_buf, sizeof(zb_ieee_addr_t));

    /* Not all platforms should configure MAC address. Set empty address in prod config for that case. */
    if (!ZB_IEEE_ADDR_IS_ZERO(extended_address))
    {
      TRACE_MSG(TRACE_ERROR, "IEEE address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(extended_address)));
      zb_set_long_address(extended_address);
    }
  }

  /* MAC TX Power */
#ifdef ZB_MAC_CONFIGURABLE_TX_POWER
  zb_mac_set_tx_power_provider_function(zb_production_config_fetch_tx_power);
#endif

  /* Install Code */
#if defined ZB_SECURITY_INSTALLCODES
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00012,30} */
  if (!ZB_IS_TC())
  {
    zb_uindex_t i;
    zb_bool_t install_code_present = ZB_FALSE;

    ret = zb_production_cfg_read(
      read_buf, ZB_CCM_KEY_SIZE + 2U /* two bytes CRC */,
      (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_1_t, install_code));

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00009,3} */
    if (RET_OK == ret)
    {
      for (i = 0; i < (ZB_CCM_KEY_SIZE + 2U); ++i)
      {
        if (read_buf[i] != 0U)
        {
          install_code_present = ZB_TRUE;
          break;
        }
      }
    }

    if (install_code_present)
    {
      TRACE_MSG(TRACE_COMMON3, "Installcode present", (FMT__0));
      ret = zb_secur_ic_set(ZB_IC_TYPE_128, read_buf);
    }
    else
    {
      TRACE_MSG(TRACE_COMMON3, "Installcode is not set", (FMT__0));
    }
  }
#endif /* ZB_SECURITY_INSTALLCODES */

  return ret;
}


static zb_ret_t zb_production_config_load_ver_2_0(zb_production_config_hdr_t *prod_cfg_hdr,
                                                  zb_uint16_t *stack_cfg_len)
{
  zb_uint8_t read_buf[32];
  zb_ret_t ret;
  zb_uint32_t aps_channel_mask;
  zb_uint8_t options = 0;

  ZVUNUSED(prod_cfg_hdr);

  *stack_cfg_len = (zb_uint16_t)sizeof(zb_production_config_ver_2_t);

  /* APS Channel Mask */
  ret = zb_production_cfg_read(
    read_buf,
    ZB_PROD_CFG_APS_CHANNEL_LIST_SIZE * (zb_uint16_t)sizeof(zb_uint32_t),
    (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t, aps_channel_mask_list));

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,4} */
  if (ret == RET_OK)
  {
#if !defined ZB_SUBGHZ_ONLY_MODE
    ZB_MEMCPY(&aps_channel_mask, read_buf, sizeof(aps_channel_mask));
    TRACE_MSG(TRACE_ZDO1, "aps_mask 2.4 %lx", (FMT__L, aps_channel_mask));
    zb_set_channel_mask(aps_channel_mask);
#endif /* !ZB_SUBGHZ_ONLY_MODE */
#if defined ZB_SUBGHZ_BAND_ENABLED
    {
      zb_uint8_t i;
      for (i = 1; i < ZB_PROD_CFG_APS_CHANNEL_LIST_SIZE && ret == RET_OK; i++)
      {
        zb_uint32_t channel_mask;
        /* It's necessary to support the prod.config v2 with 5 pages: 0 and 28-31.
         * Now we have 23-27 pages between 0 and 28 pages in sub-ghz - 10 pages in total.
         * So our stack will think the 23th page is set
         * when we use the prod.config v2 with 0 and 28-31 pages
         * that has a channel mask for 28th page.
         */
        zb_uint8_t offset = 5U;
        ZB_MEMCPY(&channel_mask, read_buf + sizeof(channel_mask) * i, sizeof(zb_uint32_t));
        TRACE_MSG(TRACE_ZDO1, "aps_mask [%d] %lx", (FMT__D_L, i, channel_mask));
        zb_aib_channel_page_list_set_mask(i + offset, channel_mask);
      }
    }
#endif /* ZB_SUBGHZ_BAND_ENABLED */
  }

  /* IEEE address */
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,5} */
  if (ret == RET_OK)
  {
    ret = zb_production_cfg_read(
      read_buf,
      (zb_uint16_t)sizeof(zb_64bit_addr_t),
      (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t, extended_address));
  }

  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,6} */
  if (ret == RET_OK)
  {
    zb_ieee_addr_t extended_address;

    ZB_MEMCPY(extended_address, read_buf, sizeof(zb_ieee_addr_t));

    /* Not all platforms should configure MAC address. Set empty address in prod config for that case. */
    if (!ZB_IEEE_ADDR_IS_ZERO(extended_address))
    {
      TRACE_MSG(TRACE_ZDO1, "IEEE address " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(extended_address)));
      zb_set_long_address(extended_address);
    }
  }

  /* TX power */
#ifdef ZB_MAC_CONFIGURABLE_TX_POWER

  /* register as tx power value provider */
  zb_mac_set_tx_power_provider_function(zb_production_config_fetch_tx_power);

  /* Skipping this section now since tx power values will be explicitly requested by mac layer when needed */
#endif

  /* Options */
  /*cstat !MISRAC2012-Rule-14.3_a */
  /** @mdr{00009,7} */
  if (ret == RET_OK)
  {
    ret = zb_production_cfg_read(
      &options, (zb_uint16_t)sizeof(zb_uint8_t), (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t, options));
  }

#if defined ZB_SECURITY_INSTALLCODES
  if (
    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00009,10} */
    ret == RET_OK &&
    !ZB_IS_TC()
  )
  {
    zb_uint16_t i;
    zb_uint16_t sum = 0;
    zb_ic_types_t ic_type;
    zb_uint16_t ic_len;

    ic_type = (zb_ic_types_t)(options & ZB_PROD_CFG_OPTIONS_IC_TYPE_MASK);
    ic_len = ZB_IC_SIZE_BY_TYPE(ic_type);

    ret = zb_production_cfg_read(
      read_buf, ic_len + 2U /* two bytes CRC */,
      (zb_uint16_t)ZB_OFFSETOF(zb_production_config_ver_2_t, install_code));

    for (i = 0; i < (ic_len + 2U); i++)
    {
      sum += read_buf[i];
    }

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00009,8} */
    if (ret == RET_OK)
    {
      if (sum != 0U)
      {
        TRACE_MSG(TRACE_ZDO1, "Installcode present", (FMT__0));
        ret = zb_secur_ic_set(ic_type, read_buf);
      }
      else
      {
        TRACE_MSG(TRACE_ZDO1, "Installcode is not set", (FMT__0));
      }
    }
  }
#endif /* ZB_SECURITY_INSTALLCODES */

#if 1//def ZB_ENABLE_SE
  /* Can have here certificates for SE only, but count the size
     and skip it even if not in SE mode. */
  if (ret == RET_OK &&
      ZB_U2B(options & ZB_PROD_CFG_OPTIONS_CERT_PRESENT_MASK))
  {
    zb_uint16_t crt_mask;
    zb_uindex_t i;
    const zb_uint8_t c_certsize[2]={21+22+48, 36+37+74};

    *stack_cfg_len += (zb_uint16_t)sizeof(zb_uint16_t);

    ret = zb_production_cfg_read(
      (zb_uint8_t *)&crt_mask, (zb_uint16_t)sizeof(zb_uint16_t),
      (zb_uint16_t)sizeof(zb_production_config_ver_2_t));

    /*cstat !MISRAC2012-Rule-14.3_a */
    /** @mdr{00009,9} */
    if (ret == RET_OK)
    {
      TRACE_MSG(TRACE_ZDO1, "cs_mask [0x%x]", (FMT__D, crt_mask));
      for (i = 0; i < sizeof(zb_uint16_t); i++)
      {
        if ((crt_mask & (1UL<<i)) != 0U)
        {
          *stack_cfg_len += c_certsize[i];
        }
      }
    }
  }
#endif  /* #ifdef ZB_ENABLE_SE */

  return ret;
}


void zdo_load_production_config(void)
{
  zb_ret_t ret;
  zb_bufid_t buf = zb_buf_get_any();
  zb_production_config_hdr_t prod_cfg_hdr;
  zb_uint16_t stack_cfg_len = 0;
  zb_uint8_t *application_production_config;
  zb_uint16_t application_section_len = 0;

  TRACE_MSG(TRACE_ZDO1, ">> load production config", (FMT__0));

  ret = zb_production_config_check(&prod_cfg_hdr);

  if (ret == RET_OK)
  {
    if (prod_cfg_hdr.version == ZB_PRODUCTION_CONFIG_VERSION_1_0)
    {
      ret = zb_production_config_load_ver_1_0();
    }
    else if (prod_cfg_hdr.version == ZB_PRODUCTION_CONFIG_CURRENT_VERSION)
    {
      ret = zb_production_config_load_ver_2_0(&prod_cfg_hdr, &stack_cfg_len);
    }
    else
    {
      TRACE_MSG(TRACE_ERROR, "production config version mismatch expected %d actual %d",
                (FMT__D_D, ZB_PRODUCTION_CONFIG_CURRENT_VERSION, prod_cfg_hdr.version));
      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    /* Place application related part of stack in the buffer and pass it to application */
    application_section_len = prod_cfg_hdr.len - stack_cfg_len;

    TRACE_MSG(TRACE_COMMON2, "passing application prod_cfg offset %d len %d bytes",
              (FMT__D_D, stack_cfg_len, application_section_len));

    if (application_section_len > ZB_PRODUCTION_CONFIG_APP_MAX_SIZE)
    {
      TRACE_MSG(TRACE_ERROR, "production config app section len %hd is greater than max size %hd",
                (FMT__D_D, application_section_len, ZB_PRODUCTION_CONFIG_APP_MAX_SIZE));

      ret = RET_ERROR;
    }
  }

  if (ret == RET_OK)
  {
    application_production_config = zb_app_signal_pack(buf,
                                                       ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY,
                                                       0,
                                                       (zb_uint8_t)application_section_len);
    if (application_section_len > 0U)
    {
      ret = zb_production_cfg_read(application_production_config,
                                             application_section_len,
                                             stack_cfg_len);
    }
  }

  if (ret == RET_OK)
  {
    ZB_SCHEDULE_CALLBACK(zboss_signal_handler, buf);
  }
  /* If production configuration is not present or invalid, notify application
   * so that it would know to apply some defaults if needed
   */
  else
  {
    /* This may or may not backfire with older application which treat non-zero status as error
     * Although in the most cases the only effect will be "Device status Failed" line in trace.
     */
    /* Lot of existing nsng tasts fails to execute in manual mode now. Guys, were you safe? */
    TRACE_MSG(TRACE_INFO1, "no valid production configuration found, signaling to application", (FMT__0));
    (void)zb_app_signal_pack(buf, ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, (zb_int16_t)RET_ERROR, 0U);
    ZB_SCHEDULE_CALLBACK(zboss_signal_handler, buf);
  }

  TRACE_MSG(TRACE_ZDO1, "<< load production config", (FMT__0));
}

#endif  /* ZB_PRODUCTION_CONFIG */
