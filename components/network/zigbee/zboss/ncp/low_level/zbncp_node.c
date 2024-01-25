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
/*  PURPOSE: NCP low level protocol instantiation.
*/

#define ZB_TRACE_FILE_ID 25
#include "zbncp.h"
#include "zbncp_tr_impl.h"
#include "zbncp_ll_impl.h"

typedef struct zbncp_node_s
{
    zbncp_transport_t tr;
    zbncp_ll_proto_t ll;
} zbncp_node_t;

static zbncp_node_t node;

zbncp_ll_proto_t *zbncp_ll_create(const zbncp_transport_ops_t *tr_ops)
{
    zbncp_transport_construct(&node.tr, tr_ops);
    zbncp_ll_construct(&node.ll, &node.tr);
    return &node.ll;
}
