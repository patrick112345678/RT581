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
/* PURPOSE: Zigbee Light Link, Startup Attribute Sets
*/

#define ZB_TRACE_FILE_ID 2120
#include "zb_common.h"
#include "zb_zcl.h"
#include "zll/zb_zll_common.h"
#include "zll/zb_zll_sas.h"

#if defined ZB_ENABLE_ZLL

#if defined ZB_ENABLE_ZLL_SAS


/*
 *  Startup attributes. See spec 8.1.6.1
 *
 */
void zb_zll_sas_startup_attr()
{
    TRACE_MSG(TRACE_ZCL1, "> zb_zll_sas_startup_attr", (FMT__0));

    /* Our stack had already set protocol version as appropriate (see
       ZB_PROTOCOL_VERSION) */
    /* Our stack had already set stack profile version, see ZB_STACK_PROFILE */
    /* Startup control parameter is an indicator one, not implemented */

    ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_ZLL_SAS_SHORT_ADDRESS;
    ZB_EXTPANID_ZERO(ZB_AIB().aps_use_extended_pan_id);
    ZB_PIBCACHE_PAN_ID() = ZB_ZLL_SAS_PAN_ID;
#ifndef DEBUG
    zb_aib_channel_page_list_set_2_4GHz_mask(ZB_ZLL_SAS_CHANNEL_MASK); /* MMDEVSTUBS */
#endif
    // Protocol version 0x02 Zigbee specification 2007.
    // Stack profile 0x02 Zigbee-Pro.
    // Startup control 0x03
#ifndef ZB_BDB_TOUCHLINK
    ZDO_CTX().conf_attr.permit_join_duration = ZB_ZLL_SAS_DISABLE_PERMIT_JOIN;
#endif
    TRACE_MSG(TRACE_ZCL1, "< zb_zll_sas_startup_attr", (FMT__0));
}

/*
 *  Security attributes. See spec 8.1.6.2
 *
 */
void zb_zll_sas_security_attr()
{
#ifndef ZB_BDB_TOUCHLINK
    zb_uint8_t standard_key[ZB_CCM_KEY_SIZE] =  ZB_STANDARD_TC_KEY;

    TRACE_MSG(TRACE_ZCL1, "> zb_zll_sas_security_attr", (FMT__0));

    /* Master, network, and default trust center link keys are not set because
         they are same as stack defaults. Also this point should be controlled for
         possible changes in the future. */

    ZB_IEEE_ADDR_ZERO(ZB_AIB().trust_center_address);
#ifndef ZB_DEBUG
    /* Why was 1??? */
    ZB_SET_NIB_SECURITY_LEVEL(0x05);
#endif
    ZB_MEMSET(&ZB_NIB().secur_material_set[0].key, 0, ZB_CCM_KEY_SIZE);
    ZB_NIB().active_key_seq_number = 0x00;
    ZB_MEMCPY(&(ZB_AIB().tc_standard_key), standard_key, ZB_CCM_KEY_SIZE);
    /* ZB_BZERO(&(ZB_AIB().tc_alternative_key), ZB_CCM_KEY_SIZE); */
    ZB_AIB().aps_insecure_join = ZB_ZLL_SAS_INSECURE_JOIN;

#else  /* ZB_BDB_TOUCHLINK */

    /* Set reasonable defaults */
    {
        zb_uint8_t mc[] = ZB_TOUCHLINK_PRECONFIGURED_KEY;
        ZB_MEMCPY(ZLL_DEVICE_INFO().master_key, mc, ZB_CCM_KEY_SIZE);
        ZB_MEMCPY(ZLL_DEVICE_INFO().certification_key, mc, ZB_CCM_KEY_SIZE);
        ZLL_DEVICE_INFO().key_index = ZB_ZLL_CERTIFICATION_KEY_INDEX;
        ZLL_DEVICE_INFO().key_info = ZB_ZLL_MASTER_KEY | ZB_ZLL_CERTIFICATION_KEY;
    }

#endif  /* ZB_BDB_TOUCHLINK */
    TRACE_MSG(TRACE_ZCL1, "< zb_zll_sas_security_attr", (FMT__0));
}


void zb_zll_process_sas()
{
    TRACE_MSG(TRACE_ZCL1, "> zb_zll_process_sas", (FMT__0));

    // Startup attributes. See spec 8.1.6.1
    zb_zll_sas_startup_attr();

    // Security attributes. See spec 8.1.6.2
    zb_zll_sas_security_attr();

    TRACE_MSG(TRACE_ZCL1, "< zb_zll_process_sas", (FMT__0));
}

#endif /* defined ZB_ENABLE_ZLL_SAS */

#endif /* defined ZB_ENABLE_ZLL */
