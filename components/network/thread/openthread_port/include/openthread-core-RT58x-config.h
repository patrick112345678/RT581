/*
 *  Copyright (c) 2021, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "project_config.h"

/**
 * @file
 *   This file includes all compile-time configuration constants used by
 *   efr32 applications for OpenThread.
 */
#ifndef OPENTHREAD_CORE_RT58X_CONFIG_H_
#define OPENTHREAD_CORE_RT58X_CONFIG_H_

/*
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */

#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "RT581"
#endif

/**
 * @def OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
 *
 * Define as 1 to enable `AnnounceSender` which will periodically send MLE Announce message on all channels.
 *
 * The list of channels is determined from the Operational Dataset's ChannelMask.
 *
 */
#ifndef OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE
#define OPENTHREAD_CONFIG_ANNOUNCE_SENDER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
 *
 * The maximum size of the CLI line in bytes including the null terminator.
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH
#define OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH 640
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 32
#endif

#if OPENTHREAD_CONFIG_MLE_MAX_CHILDREN > 256
#define OPENTHREAD_CONFIG_MESSAGE_BUFFER_SIZE                             \
    (sizeof(void *) * ((OPENTHREAD_CONFIG_MLE_MAX_CHILDREN % 8) > 0       \
                           ? (OPENTHREAD_CONFIG_MLE_MAX_CHILDREN / 8) + 1 \
                           : (OPENTHREAD_CONFIG_MLE_MAX_CHILDREN / 8)))
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 100
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT
 *
 * The default child timeout value (in seconds).
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT
#define OPENTHREAD_CONFIG_MLE_CHILD_TIMEOUT_DEFAULT \
    240 // 2147483000(Deep sleep use)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Define to 1 if you want to enable software transmission security logic.
 *
 */
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 0
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ENERGY_SCAN_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
 *
 * Define as 1 to enable support for locally initializing an Active Operational
 * Dataset.
 *
 * @note This functionality is deprecated and not recommended.
 *
 */
#ifndef OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT
#define OPENTHREAD_CONFIG_OPERATIONAL_DATASET_AUTO_INIT 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
 *
 * The assert is managed by platform defined logic when this flag is set.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT
#define OPENTHREAD_CONFIG_PLATFORM_ASSERT_MANAGEMENT 1
#endif


#define OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 256
#endif
#define OPENTHREAD_CONFIG_COAP_API_ENABLE 1
#define OPENTHREAD_CONFIG_JOINER_ENABLE 1
#define OPENTHREAD_CONFIG_COMMISSIONER_ENABLE 0
#define OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE 0
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 0
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#define OPENTHREAD_CONFIG_DTLS_ENABLE 1
#define OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT 1
#define OPENTHREAD_CONFIG_PING_SENDER_ENABLE 1
#define OPENTHREAD_CONFIG_TCP_ENABLE 0
#define OPENTHREAD_CONFIG_TMF_NETDIAG_CLIENT_ENABLE 1
#define OPENTHREAD_SPINEL_CONFIG_OPENTHREAD_MESSAGE_ENABLE 1
#define OPENTHREAD_CONFIG_SNTP_CLIENT_ENABLE 1

#define OPENTHREAD_CONFIG_ECDSA_ENABLE 1

#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 160
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 3
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 3
#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 3

#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_DIRECT 5
#define OPENTHREAD_CONFIG_MAC_MAX_CSMA_BACKOFFS_INDIRECT 5

#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_DEBG
#define OPENTHREAD_CONFIG_LOG_LEVEL_INIT OT_LOG_LEVEL_NONE
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 1
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1

#define OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE 1
#define OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_THREAD_VERSION OT_THREAD_VERSION_1_3

#define OPENTHREAD_PLATFORM_POSIX 0
/**
 * @def OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS
 *
 * Specifies the desired number of router links that a REED / FED attempts to
 * maintain.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS
#define OPENTHREAD_CONFIG_MLE_CHILD_ROUTER_LINKS 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
 *
 * Define as 1 to enable Link Metrics initiator feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE
#define OPENTHREAD_CONFIG_MLE_LINK_METRICS_INITIATOR_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
 *
 * Define as 1 to enable Link Metrics subject feature.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
#define OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE 0
#endif

#define OPENTHREAD_CONFIG_MLR_ENABLE 1

#define OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE 0 //(if use freertos set to 0)

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE == 1

/**
 * @def OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US
 *
 * Define how many microseconds ahead should MAC deliver CSL frame to SubMac.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US
#define OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
 *
 * Reception scheduling and ramp up time needed for the CSL receiver to be
 * ready, in units of microseconds.
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD
#define OPENTHREAD_CONFIG_CSL_RECEIVE_TIME_AHEAD 320
#endif

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
 *
 * The minimum time (in microseconds) before the MHR start that the radio should
 * be in receive state and ready to properly receive in order to properly
 * receive any IEEE 802.15.4 frame. Defaults to the duration of SHR + PHR.
 *
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AHEAD (6 * 32)
#endif

/**
 * @def OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
 *
 * The minimum time (in microseconds) after the MHR start that the radio should
 * be in receive state in order to properly receive any IEEE 802.15.4 frame.
 * Defaults to the duration of a maximum size frame, plus AIFS, plus the
 * duration of maximum enh-ack frame. Platforms are encouraged to improve this
 * value for energy efficiency purposes.
 *
 */
#ifndef OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER
#define OPENTHREAD_CONFIG_MIN_RECEIVE_ON_AFTER ((127 + 6 + 39) * 32)
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE \
    OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
#endif

#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE

#define OPENTHREAD_CONFIG_MAC_FILTER_ENABLE 1
#define OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE 1

#define OPENTHREAD_CONFIG_DUA_ENABLE 1
#define OPENTHREAD_CONFIG_MLR_ENABLE 1
#define OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE 1

/**
 * @OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL
 *
 * Specifies the time-in-queue threshold interval in milliseconds to mark ECN on
 * a message if it is ECN-capable or drop the message if not ECN-capable.
 *
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_MARK_ECN_INTERVAL 2000
#endif

/**
 * @OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL
 *
 * Specifies the time-in-queue threshold interval in milliseconds to drop a
 * message.
 *
 */
#ifndef OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL
#define OPENTHREAD_CONFIG_DELAY_AWARE_QUEUE_MANAGEMENT_DROP_MSG_INTERVAL 2000
#endif

#endif // OPENTHREAD_CORE_RT58X_CONFIG_H_
