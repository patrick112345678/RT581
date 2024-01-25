/*
 * @file
 * @brief This file contains definitions for a CPC based NCP interface to the OpenThread stack.
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef NCP_CPC_HPP_
#define NCP_CPC_HPP_

#include "openthread-core-config.h"

#include "cpc.h"
#include "cpc_api.h"
#include "ncp/ncp_base.hpp"

namespace ot {
namespace Ncp {

class NcpCPC : public NcpBase
{
    typedef NcpBase super_t;

public:
    /**
     * Constructor
     *
     * @param[in]  aInstance  The OpenThread instance structure.
     *
     */
    explicit NcpCPC(Instance *aInstance);

    /**
     * This method is called to transmit and receive data.
     *
     */
    void ProcessCpc(void);
    void HandleOpenEndpoint(void);

private:
    enum
    {
        kCpcTxBufferSize = OPENTHREAD_CONFIG_NCP_CPC_TX_CHUNK_SIZE
    };

    void HandleFrameAddedToNcpBuffer(void);

    static void HandleFrameAddedToNcpBuffer(void *                   aContext,
                                            Spinel::Buffer::FrameTag aTag,
                                            Spinel::Buffer::Priority aPriority,
                                            Spinel::Buffer *         aBuffer);

    void        SendToCPC(void);
    static void SendToCPC(Tasklet &aTasklet);
    static void HandleCPCSendDone(cpc_user_endpoint_id_t endpoint_id, void *buffer, void *arg, status_t status);
    void        HandleSendDone(void);
    static void HandleCPCReceive(cpc_user_endpoint_id_t endpoint_id, void *arg);
    static void HandleCPCEndpointError(uint8_t endpoint_id, void *arg);
    static void HandleEndpointError(Tasklet &aTasklet);
    void        HandleEndpointError(void);
    static void HandleOpenEndpoint(Tasklet &aTasklet);
    

    uint8_t                  mCpcTxBuffer[kCpcTxBufferSize];
    bool                     mIsReady;
    bool                     mIsWriting;
    cpc_endpoint_handle_t mUserEp;
    Tasklet                  mCpcSendTask;
    Tasklet                  mCpcEndpointErrorTask;
    Tasklet                  mCpcOpenEndpointTask;
};

} // namespace Ncp
} // namespace ot

#endif // NCP_CPC_HPP_
