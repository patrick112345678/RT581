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
/*  PURPOSE: Stubs to be used in SGW components that doesn't have scheduler.
 For now this allows to use net tracing not only in gw application.
*/

#define ZB_TRACE_FILE_ID 205
#include "zboss_api_core.h"


void zb_schedule_callback_from_alien(zb_callback_t func, zb_uint8_t param)
{
    /* call directly */
    (*func)(param);
}


zb_ret_t zb_schedule_callback(zb_callback_t func, zb_uint8_t param)
{
    /* call directly */
    (*func)(param);
    return RET_OK;
}

zb_ret_t zb_schedule_callback2(zb_callback2_t func, zb_uint8_t param, zb_uint16_t user_param)
{
    /* call directly */
    (*func)(param, user_param);
    return RET_OK;
}

zb_ret_t zb_schedule_callback_prior(zb_callback_t func, zb_uint8_t param)
{
    /* call directly */
    (*func)(param);
    return RET_OK;
}