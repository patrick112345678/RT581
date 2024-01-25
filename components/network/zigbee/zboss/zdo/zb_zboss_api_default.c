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
/*  PURPOSE: ZBOSS core API
*/

#define ZB_TRACE_FILE_ID 59
#include "zb_common.h"
#include "zb_bdb_internal.h"
#include "zb_scheduler.h"
#include "zb_nwk.h"
#include "zboss_api.h"
#include "zb_zdo.h"
#include "zb_aps.h"

void zb_set_rx_on_when_idle(zb_bool_t rx_on)
{
    TRACE_MSG(TRACE_ZDO2, "zb_set_rx_on_when_idle, rx_on %d", (FMT__D, rx_on));
#ifndef ZB_ED_RX_OFF_WHEN_IDLE
    ZB_PIBCACHE_RX_ON_WHEN_IDLE() = rx_on;
#else
    ZVUNUSED(rx_on);
#endif
}

zb_bool_t zb_get_rx_on_when_idle()
{
    return (zb_bool_t)ZB_PIBCACHE_RX_ON_WHEN_IDLE();
}

void zb_set_channel_mask(zb_uint32_t channel_mask)
{
    zb_aib_channel_page_list_set_2_4GHz_mask(channel_mask);
}

zb_uint32_t zb_get_channel_mask(void)
{
    return zb_aib_channel_page_list_get_2_4GHz_mask();
}

#ifdef ZB_OSIF_CONFIGURABLE_TX_POWER
zb_ret_t zb_set_tx_power(zb_uint8_t tx_power)
{
    zb_uint8_t channel;
    channel = zb_get_channel();
    return zb_osif_set_transmit_power(channel, tx_power);
}
#endif


/*! \addtogroup zboss_general_api */
/*! @{ */

void zb_set_pan_id(zb_uint16_t pan_id)
{
    ZB_PIBCACHE_PAN_ID() = pan_id;
}

zb_uint16_t zb_get_pan_id(void)
{
    return ZB_PIBCACHE_PAN_ID();
}

void zb_set_node_descriptor_manufacturer_code_req(zb_uint16_t manuf_code, zb_set_manufacturer_code_cb_t cb)
{
    zdo_set_node_descriptor_manufacturer_code(manuf_code);

    if (cb != NULL)
    {
        cb(RET_OK);
    }
}

/**
   Set 64-bit long address

   @param addr - long address
*/
void zb_set_long_address(const zb_ieee_addr_t addr)
{
    ZB_IEEE_ADDR_COPY(ZB_PIBCACHE_EXTENDED_ADDRESS(), addr);
    printf("%X\r\n", addr[0]);
}

/**
   Get 64-bit long address

   @return 64-bit long address
*/
void zb_get_long_address(zb_ieee_addr_t addr)
{
    ZB_IEEE_ADDR_COPY(addr, ZB_PIBCACHE_EXTENDED_ADDRESS());
}

/**
   Set Extended Pan ID (apsUseExtendedPANID attribute)
   @param ext_pan_id - Long (64-bit) Extented Pan ID
*/
void zb_set_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
    ZB_EXTPANID_COPY(ZB_AIB().aps_use_extended_pan_id, ext_pan_id);
}

/**
   Get Extended Pan ID (nwkExtendedPANId attribute)
   @param ext_pan_id - pointer to memory where result will be stored

   @return Long (64-bit) Extented Pan ID
*/
void zb_get_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
    ZB_EXTPANID_COPY(ext_pan_id, ZB_NIB_EXT_PAN_ID());
}

zb_uint8_t zb_get_current_page(void)
{
    return ZB_PIBCACHE_CURRENT_PAGE();
}

zb_uint8_t zb_get_current_channel(void)
{
    return ZB_PIBCACHE_CURRENT_CHANNEL();
}

/*! @} */

#ifdef ZB_USE_NVRAM
void zb_set_nvram_erase_at_start(zb_bool_t erase)
{
    ZB_AIB().aps_nvram_erase_at_start = ZB_B2U(erase);
}
#endif

void zb_set_nwk_role_mode_common_ext(zb_nwk_device_type_t device_type,
                                     zb_channel_list_t channel_list,
                                     zb_commissioning_type_t mode)
{
    zb_uint8_t used_page;

    ZB_AIB().aps_designated_coordinator = ZB_B2U(device_type == ZB_NWK_DEVICE_TYPE_COORDINATOR);
    ZB_NIB().device_type = device_type;

    used_page = zb_channel_page_list_get_first_filled_page(channel_list);
    if (used_page != ZB_CHANNEL_PAGES_NUM)
    {
        /* Channel list is not empty. */
        zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
    }

    COMM_CTX().commissioning_type = mode;
}

void zb_set_nwk_role_mode_common(zb_nwk_device_type_t device_type,
                                 zb_uint32_t channel_mask,
                                 zb_commissioning_type_t mode)
{
    zb_channel_list_t channel_list;
    zb_channel_list_init(channel_list);
    zb_channel_page_list_set_2_4GHz_mask(channel_list, channel_mask);
    zb_set_nwk_role_mode_common_ext(device_type, channel_list, mode);
}

/* this API is unused, looks old and non-functioning */
#if 0
#ifdef ZB_SE_COMMISSIONING

static void set_se_mm_mode_common(zb_channel_list_t channel_list)
{
    COMM_CTX().commissioning_type = ZB_COMMISSIONING_SE;
    ZB_TCPOL().update_trust_center_link_keys_required = ZB_FALSE;
    zb_channel_page_list_copy(ZB_AIB().aps_channel_mask_list, channel_list);
}

#ifdef ZB_COORDINATOR_ROLE
void zb_se_set_network_coordinator_role_select_device(zb_channel_list_t channel_list)
{
    ZB_AIB().aps_designated_coordinator = ZB_TRUE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
    ZB_SET_SUBGHZ_SWITCH_MODE(0);
    set_se_mm_mode_common(channel_list);
}


void zb_se_set_network_coordinator_role_switch_device(zb_channel_list_t channel_list)
{
    ZB_AIB().aps_designated_coordinator = ZB_TRUE;
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_COORDINATOR;
    ZB_SET_SUBGHZ_SWITCH_MODE(1);
    set_se_mm_mode_common(channel_list);
}
#endif

#ifdef ZB_ED_FUNC
void zb_se_set_network_ed_role_select_device(zb_channel_list_t channel_list)
{
    ZB_NIB().device_type = ZB_NWK_DEVICE_TYPE_ED;
    ZB_SET_SUBGHZ_SWITCH_MODE(0);
    set_se_mm_mode_common(channel_list);
}
#endif

#endif  /*  ZB_SE_COMMISSIONING */

#endif /* excluded SE funcs */

#ifdef ZB_ROUTER_ROLE
void zb_set_max_children(zb_uint8_t max_children)
{
    ZB_NIB().max_children = max_children;
}


zb_uint8_t zb_get_max_children(void)
{
    return ZB_NIB().max_children;
}
#endif

zb_ret_t zboss_start(void)
{
    return zdo_dev_start();
}

zb_ret_t zboss_start_no_autostart(void)
{
    return zb_zdo_start_no_autostart();
}

void zboss_start_continue(void)
{
    ZB_SCHEDULE_CALLBACK(zb_zdo_dev_start_cont, 0);
}

zb_commissioning_type_t zb_zdo_get_commissioning_type(void)
{
    return COMM_CTX().commissioning_type;
}

void zb_secur_set_tc_rejoin_enabled(zb_bool_t enable)
{
    ZB_TCPOL().allow_tc_rejoins = enable;
}

void zb_secur_set_ignore_tc_rejoin(zb_bool_t enable)
{
    ZB_TCPOL().ignore_unsecure_tc_rejoins = enable;
}

void zb_secur_set_unsecure_tc_rejoin_enabled(zb_bool_t enable)
{
    ZB_TCPOL().allow_unsecure_tc_rejoins = enable;
}

void zb_zdo_disable_network_mgmt_channel_update(zb_bool_t disable)
{
    ZDO_CTX().handle.channel_update_disabled = disable;
}

#ifdef ZB_SECURITY_INSTALLCODES
void zb_tc_set_use_installcode(zb_uint8_t use_ic)
{
    ZB_TCPOL().require_installcodes = ZB_U2B(use_ic);
}
#endif /* ZB_SECURITY_INSTALLCODES */

#ifdef ZB_CONTROL4_NETWORK_SUPPORT
#ifdef ZB_ED_FUNC
void zb_permit_control4_network(void)
{
    ZB_TCPOL().permit_control4_network = 1;
}

zb_bool_t zb_control4_network_permitted(void)
{
    return ZB_TCPOL().permit_control4_network;
}
#endif /* ZB_ED_FUNC */

void zb_disable_control4_emulator(void)
{
    ZB_TCPOL().control4_network_emulator = 0;
}

void zb_enable_control4_emulator(void)
{
    ZB_TCPOL().control4_network_emulator = 1;
}
#endif /* ZB_CONTROL4_NETWORK_SUPPORT */

#ifdef ZB_ROUTER_ROLE
void zb_disable_transport_key_aps_encryption(void)
{
    ZB_TCPOL().aps_unencrypted_transport_key_join = ZB_TRUE;
}

void zb_enable_transport_key_aps_encryption(void)
{
    ZB_TCPOL().aps_unencrypted_transport_key_join = ZB_FALSE;
}

zb_bool_t zb_is_transport_key_aps_encryption_enabled(void)
{
    return (ZB_TCPOL().aps_unencrypted_transport_key_join) ? ZB_FALSE : ZB_TRUE;
}
#endif

#ifdef ZB_COORDINATOR_ROLE
void zb_start_concentrator_mode(zb_uint8_t radius, zb_uint32_t disc_time)
{
    ZB_NIB_SET_IS_CONCENTRATOR(ZB_TRUE);
    ZB_NIB_SET_CONCENTRATOR_RADIUS(radius);
    ZB_NIB_SET_CONCENTRATOR_DISC_TIME(disc_time);
    /* 10/22/2019 EE CR:MINOR Add check for device role: we can be compiled with ZC sup[port but start as ZR! */
#ifndef ZB_LITE_NO_SOURCE_ROUTING
    /* Start route discovery */
    zb_nwk_concentrator_start();
#endif /* ZB_LITE_NO_SOURCE_ROUTING */
}

void zb_stop_concentrator_mode(void)
{
    ZB_NIB_SET_IS_CONCENTRATOR(ZB_FALSE);
    ZB_NIB_SET_CONCENTRATOR_RADIUS(0);
    ZB_NIB_SET_CONCENTRATOR_DISC_TIME(0);
    /* 10/22/2019 EE CR:MINOR Add check for device role: we can be compiled with ZC sup[port but start as ZR! */
#ifndef ZB_LITE_NO_SOURCE_ROUTING
    /* Stop route discovery */
    zb_nwk_concentrator_stop();
#endif /* ZB_LITE_NO_SOURCE_ROUTING */
}

#endif /* ZB_COORDINATOR_ROLE */

#ifdef ZB_SECURITY_INSTALLCODES
void zb_set_installcode_policy(zb_bool_t allow_ic_only)
{
    ZB_TCPOL().require_installcodes = allow_ic_only;
}
#endif /* ZB_SECURITY_INSTALLCODES */


void zb_set_use_extended_pan_id(const zb_ext_pan_id_t ext_pan_id)
{
    TRACE_MSG(TRACE_ZDO2, ">> zb_set_use_extended_pan_id, ext_pan_id " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ext_pan_id)));

    ZB_EXTPANID_COPY(ZB_AIB().aps_use_extended_pan_id, ext_pan_id);

    TRACE_MSG(TRACE_ZDO2, "<< zb_set_use_extended_pan_id", (FMT__0));
}

void zb_get_use_extended_pan_id(zb_ext_pan_id_t ext_pan_id)
{
    ZB_EXTPANID_COPY(ext_pan_id, ZB_AIB().aps_use_extended_pan_id);

    TRACE_MSG(TRACE_ZDO2, "zb_get_use_extended_pan_id: " TRACE_FORMAT_64,
              (FMT__A, TRACE_ARG_64(ext_pan_id)));
}

zb_uint16_t zb_get_short_address(void)
{
    return ZB_PIBCACHE_NETWORK_ADDRESS();
}
