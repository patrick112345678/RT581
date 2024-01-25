/**
 * @file cpc_config.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef CPC_CONFIG_H
#define CPC_CONFIG_H

#ifndef CPC_RX_PAYLOAD_MAX_LENGTH
#define CPC_RX_PAYLOAD_MAX_LENGTH      (484)
#endif // !CPC_RX_PAYLOAD_MAX_LENGTH

#ifndef CPC_TX_QUEUE_ITEM_MAX_COUNT
#define CPC_TX_QUEUE_ITEM_MAX_COUNT    (40)
#endif // !CPC_TX_QUEUE_ITEM_MAX_COUNT

#ifndef CPC_RX_BUFFER_MAX_COUNT
#define CPC_RX_BUFFER_MAX_COUNT        (20)
#endif // !CPC_RX_BUFFER_MAX_COUNT

#ifndef CPC_USER_ENDPOINT_MAX_COUNT
#define CPC_USER_ENDPOINT_MAX_COUNT    (2)
#endif // CPC_USER_ENDPOINT_MAX_COUNT

#endif /* CPC_CONFIG_H */
