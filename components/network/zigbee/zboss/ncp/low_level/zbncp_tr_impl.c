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
/*  PURPOSE: NCP low level protocol transport wrappers implementation.
*/

#define ZB_TRACE_FILE_ID 28
#include "zbncp_tr_impl.h"

void zbncp_transport_construct(zbncp_transport_t *tr, const zbncp_transport_ops_t *ops)
{
    tr->ops = *ops;
}

void zbncp_transport_init(zbncp_transport_t *tr, const zbncp_transport_cb_t *cb)
{
    tr->ops.init(tr->ops.impl, cb);
}

void zbncp_transport_send(zbncp_transport_t *tr, zbncp_cmemref_t mem)
{
    tr->ops.send(tr->ops.impl, mem);
}

void zbncp_transport_recv(zbncp_transport_t *tr, zbncp_memref_t mem)
{
    tr->ops.recv(tr->ops.impl, mem);
}
