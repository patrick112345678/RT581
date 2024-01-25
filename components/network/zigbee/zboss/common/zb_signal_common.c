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
/* PURPOSE: application signal common functions
*/

#define ZB_TRACE_FILE_ID            1211

#include "zb_common.h"


zb_zdo_app_signal_type_t zb_get_app_signal(zb_uint8_t param,
        zb_zdo_app_signal_hdr_t **sg_p)
{
    zb_zdo_app_signal_type_t sig_type;

    if (zb_buf_len(param) == 0)
    {
        sig_type = ZB_ZDO_SIGNAL_DEFAULT_START;
        if (sg_p != NULL)
        {
            *sg_p = NULL;
        }
    }
    else if (zb_buf_len(param) < sizeof(zb_zdo_app_signal_hdr_t))
    {
        sig_type = ZB_ZDO_SIGNAL_ERROR;
        if (sg_p != NULL)
        {
            *sg_p = NULL;
        }
    }
    else
    {
        /* Suppress warning about alignment. Buffer begin is aligned here for sure. */
        void *p = zb_buf_begin(param);
        zb_zdo_app_signal_hdr_t *sg = (zb_zdo_app_signal_hdr_t *)p;
        if (sg_p != NULL)
        {
            *sg_p = sg;
        }
        sig_type = (zb_zdo_app_signal_type_t)sg->sig_type;
    }
    return sig_type;
}

void *zb_app_signal_pack(zb_uint8_t param, zb_uint32_t signal_code,
                         zb_int16_t status, zb_uint8_t data_size)
{
    zb_uint8_t *body;

    ZB_ASSERT(param);
    if (signal_code == ZB_ZDO_SIGNAL_DEFAULT_START && data_size == 0)
    {
        /* Default (compatible with previous API) event has no body */
        body = zb_buf_reuse(param);
        zb_buf_set_status(param, (status == RET_OK ? RET_OK : RET_ERROR));
        ZVUNUSED(body);
        /*
         * Don't use tracing in this function - it breaks sleepy ED if ZDO trace
         * subsystem is enabled.
         */
        /* TRACE_MSG(TRACE_ZDO1, "zb_app_signal_pack ZB_ZDO_SIGNAL_DEFAULT_START"
         *           " status %d", (FMT__D, zb_buf_get_status(param)));
         */
        return NULL;
    }
    else
    {
        /*
         * Initial alloc creates aligned pointer. Signal_code size is 4 bytes,
         * so signal data is aligned as well.
         */
        body = zb_buf_initial_alloc(param, sizeof(signal_code) + data_size);
        ZB_MEMCPY(body, &signal_code, sizeof(signal_code));
        zb_buf_set_status(param, (status == RET_OK ? RET_OK : RET_ERROR));
        /* TRACE_MSG(TRACE_ZDO1, "zb_app_signal_pack sig %ld size %d status %d",
         *           (FMT__L_D_D, signal_code, data_size,
         *           zb_buf_get_status(param)));
         */
        return (void *)(body + sizeof(signal_code));
    }
}

