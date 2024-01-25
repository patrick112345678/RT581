/**
 * @file hci_interface.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-31
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _HCI_INTERFACE_H_
#define _HCI_INTERFACE_H_

#include "ble_hci.h"

typedef enum
{
    HIC_INTERFACE_CALLBACK_TYPE_EVENT,
    HIC_INTERFACE_CALLBACK_TYPE_DATA,
} hci_interface_callback_type_t;


typedef int (*hci_interface_callback_t)(uint8_t *p_data, uint16_t data_len);

void hci_interface_init(void);
void hci_interface_callback_set(hci_interface_callback_type_t type, hci_interface_callback_t pfn_callback);
int hci_interface_message_write(ble_hci_message_t *pmesg);

#endif // 1_HCI_INTERFACE_H_

