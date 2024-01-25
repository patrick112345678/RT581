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
/* PURPOSE: Zigbee Smart Energy, Startup Attribute Sets
*/

#define ZB_TRACE_FILE_ID 1937
#include "zb_common.h"

#ifdef ZB_ENABLE_SE

#if defined ZB_ENABLE_SE_SAS

#include "se/zb_se_sas.h"

void zb_se_init_sas()
{
    TRACE_MSG(TRACE_ZCL1, "> zb_se_init_sas", (FMT__0));

    /* Our stack had already set protocol version as appropriate (see
       ZB_PROTOCOL_VERSION) */
    /* Our stack had already set stack profile version, see ZB_STACK_PROFILE */
    /* Startup control parameter is an indicator one, not implemented. */
    /* Config_Rejoin_Interval and Config_Max_Rejoin_Interval attributes are
       optional, so not supported */

    /* Startup Parameters */
    if (!ZB_JOINED())
    {
        ZB_PIBCACHE_NETWORK_ADDRESS() = ZB_SE_SAS_SHORT_ADDRESS;
        /* FIXME: not sure. It supposes never rejoin. */
        //EE commented out ZB_EXTPANID_ZERO(ZB_AIB().aps_use_extended_pan_id);
        ZB_PIBCACHE_PAN_ID() = ZB_SE_SAS_PAN_ID;
        /* MMDEVQ: right? */
        zb_aib_channel_page_list_set_2_4GHz_mask(ZB_SE_SAS_CHANNEL_MASK); /* MMDEVSTUBS */

        ZB_IEEE_ADDR_ZERO(ZB_AIB().trust_center_address);

        /* FIXME: Have ZB_SE_SAS_LINK_LEY and ZB_SE_SAS_NETWORK_KEY, but not sure how to use it!
           Looks like we may ignore these values.
         */

        ZB_AIB().aps_insecure_join = ZB_SE_SAS_INSECURE_JOIN;
    }

    /* Join Parameters */
    COMM_CTX().discovery_ctx.nwk_scan_attempts = ZB_SE_SAS_SCAN_ATTEMPTS;
    COMM_CTX().discovery_ctx.nwk_time_btwn_scans = ZB_SE_SAS_TIME_BTWN_SCANS;

    /* Concentrator Parameters */
    /* TODO: Radius - have attr in NIB, not implemented. */

    /* Binding Parameters */
    ZDO_CTX().conf_attr.enddev_bind_timeout = ZB_SE_SAS_END_DEVICE_BIND_TIMEOUT;

    ZDO_CTX().conf_attr.permit_join_duration = ZB_DEFAULT_PERMIT_JOINING_DURATION;

    TRACE_MSG(TRACE_ZCL1, "< zb_se_init_sas", (FMT__0));
}

#endif /* defined ZB_ENABLE_SE_SAS */

#endif /* ZB_ENABLE_SE */
