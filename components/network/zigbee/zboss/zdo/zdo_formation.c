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
/* PURPOSE: Formation code moved there from zdo_app.c
   For NCP this file is compiled in SoC and Host-side mode with different defines.
*/

#define ZB_TRACE_FILE_ID 53
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

#if defined ZB_HA_SUPPORT_EZ_MODE
#include "ha/zb_ha_ez_mode_comissioning.h"
#endif

/*! \addtogroup ZB_ZDO */
/*! @{ */

#ifdef ZB_FORMATION

static void zdo_comm_set_permit_join_after_formation(zb_uint8_t param);

void zdo_formation_force_link(void)
{
    TRACE_MSG(TRACE_ZDO1, "zdo_formation_force_link", (FMT__0));

#ifndef NCP_MODE_HOST
    /* Link NWK-level formation if this is not NCP host */
    zb_nwk_formation_force_link();

    APS_SELECTOR().authhenticate_child_directly = secur_authenticate_child_directly;
#endif /* !defined NCP_MODE_HOST */
}


void zb_nlme_network_formation_confirm(zb_uint8_t param)
{
    TRACE_MSG(TRACE_INFO1, "zb_nlme_network_formation_confirm %hd status %hd",
              (FMT__H_H, param, zb_buf_get_status(param)));

    if (zb_buf_get_status(param) != RET_OK)
    {
        ZB_SCHEDULE_CALLBACK(zdo_commissioning_formation_failed, param);
    }
    else
    {
        zb_address_ieee_ref_t addr_ref;

        /* Be sure my address exists in the translation table and
           lock it as not locked translation table entry a) can be reused, b) will not be saved in nvram. */
        zb_address_by_short(ZB_PIBCACHE_NETWORK_ADDRESS(), ZB_TRUE, ZB_TRUE, &addr_ref);

        secur_tc_init();

#ifdef ZB_USE_NVRAM
        /* If we fail, trace is given and assertion is triggered */
        (void)zb_nvram_write_dataset(ZB_NVRAM_COMMON_DATA);
#endif

        TRACE_MSG(TRACE_ZDO3, "Schedule zdo_send_parent_annce from zb_nlme_network_formation_confirm", (FMT__0));
        ZB_SCHEDULE_CALLBACK(zdo_send_parent_annce_at_formation, 0);
        ZB_SCHEDULE_CALLBACK(zdo_comm_set_permit_join_after_formation, param);
    }
}


static void zdo_comm_set_permit_join_after_formation(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO2, ">>set_permit_join_after_formation %hd max_children %hd",
              (FMT__H_H, param, zb_get_max_children()));

    zdo_comm_set_permit_join(param, zdo_commissioning_formation_done);

    TRACE_MSG(TRACE_ZDO2, "<<set_permit_join_after_formation", (FMT__0));
}


void zdo_commissioning_formation_done(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_formation_done param %hd", (FMT__H, param));

    ZB_ASSERT(COMM_SELECTOR().signal != NULL);
    COMM_SELECTOR().signal(ZB_COMM_SIGNAL_FORMATION_DONE, param);
}


void zdo_commissioning_formation_failed(zb_uint8_t param)
{
    TRACE_MSG(TRACE_ZDO1, "zdo_commissioning_formation_failed, param %d", (FMT__D, param));

    ZB_ASSERT(COMM_SELECTOR().signal != NULL);
    COMM_SELECTOR().signal(ZB_COMM_SIGNAL_FORMATION_FAILED, param);
}


/* High-level commissioning is at Host side of NCP */
#if !defined NCP_MODE || defined NCP_MODE_HOST || defined ZB_CONFIGURABLE_COMMISSIONING_TYPE

void zdo_start_formation(zb_uint8_t param)
{
    /*! [zb_nlme_network_formation_request] */
    /* will start as coordinator: Formation */
    zb_nlme_network_formation_request_t *req = ZB_BUF_GET_PARAM(param, zb_nlme_network_formation_request_t);
    zb_ext_pan_id_t ext_pan_id;
    zb_ext_pan_id_t use_ext_pan_id;

    TRACE_MSG(TRACE_ZDO2, "channel mask list:", (FMT__0));
#if defined TRACE_LEVEL && !defined NCP_MODE_HOST
    if (TRACE_ENABLED(TRACE_ZDO2))
    {
        zb_uint_t i;

        for (i = 0; i < ZB_CHANNEL_PAGES_NUM; i++)
        {
            TRACE_MSG(TRACE_ZDO2,
                      "page %hd, mask 0x%lx",
                      (FMT__H_L,
                       zb_aib_channel_page_list_get_page(i),
                       zb_aib_channel_page_list_get_mask(i)));
        }
    }
#endif

    zb_get_extended_pan_id(ext_pan_id);
    if (!ZB_EXTPANID_IS_ZERO(ext_pan_id)
            /* in sub-GHz current channel can be 0! Note: not tested. */
            && (zb_get_current_channel() != 0 || zb_get_current_page() != 0))
    {
        TRACE_MSG(TRACE_ERROR, "Pan ID = " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(ext_pan_id)));
        TRACE_MSG(TRACE_ERROR, "Start already created network on %hd channel",
                  (FMT__H, zb_get_current_channel()));
        /* Call nwk directly or call NCP SoC */
#if defined ZB_DISTRIBUTED_SECURITY_ON
        zb_sync_distributed();
#endif  /* ZB_DISTRIBUTED_SECURITY_ON */
        zb_nwk_cont_without_formation(param);
    }
    else
    {
        /* we must set nwkExtendedPanID to aspUseExtendedPanID if any. Else use our
        * MAC address. */
        /* aps_use_extended_pan_id is assigned by zb_set_extended_pan_id() even at NCP Host */
        zb_get_use_extended_pan_id(use_ext_pan_id);
        if (!ZB_EXTPANID_IS_ZERO(use_ext_pan_id))
        {
            /* For NCP Host: added ext panid into req and remove that
             * assignment from here - better do it in
             * zb_nlme_network_formation_request */
            ZB_IEEE_ADDR_COPY(req->extpanid, use_ext_pan_id);
        }
        else
        {
            ZB_IEEE_ADDR_ZERO(req->extpanid);
        }

        /* 1st start case - do network formation */
        TRACE_MSG(TRACE_INFO1, "First start, perform network formation", (FMT__0));

        /* FIXME: BDB assigns v_scan_channels but we do not use it! */
        /* TODO: in BDB mode use first primary, then secondary channels mask */

        ZB_ASSERT(FORMATION_SELECTOR().get_formation_channels_mask != NULL);
        FORMATION_SELECTOR().get_formation_channels_mask(req->scan_channels_list);
        req->scan_duration = ZB_DEFAULT_SCAN_DURATION; /* TODO: configure it somehow? */
        /* timeout for every channel is
           ((1l << duration) + 1) * 15360 / 1000000

           For duration 8 ~ 4s
           For duration 5 ~0.5s
           For duration 2 ~0.08s
           For duration 1 ~0.05s
        */
#if defined ZB_DISTRIBUTED_SECURITY_ON
        zb_sync_distributed();
        if (IS_DISTRIBUTED_SECURITY())
        {
            TRACE_MSG(TRACE_ERROR, "ZR - do distributed netwok formation", (FMT__0));
            req->distributed_network = 1;
            req->distributed_network_address = zb_get_short_address();
        }
#endif  /* ZB_DISTRIBUTED_SECURITY_ON */
        /* Possible NCP call.. */
        ZB_SCHEDULE_CALLBACK(zb_nlme_network_formation_request, param);
    }
}

#endif  /* !defined NCP_MODE || defined NCP_MODE_HOST */

#endif  /* ZB_FORMATION */

/*! @} */
