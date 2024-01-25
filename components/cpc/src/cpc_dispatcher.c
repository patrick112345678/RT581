/**
 * @file cpc_dispatcher.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cpc_api.h"

static slist_node_t *process_queue;
static uint8_t event_counter = 0;


void cpc_dispatcher_init_handle(cpc_dispatcher_handle_t *handle)
{
    configASSERT(handle != NULL);

    handle->submitted = false;
    handle->fnct = NULL;
    handle->data = NULL;
}

status_t cpc_dispatcher_push(cpc_dispatcher_handle_t *handle,
                             cpc_dispatcher_fnct_t fnct,
                             void *data)
{

    enter_critical_section();

    if (handle->submitted)
    {
        leave_critical_section();
        return CPC_STATUS_BUSY; // Already dispatched
    }

    handle->fnct = fnct;
    handle->data = data;
    slist_push_back(&process_queue, &handle->node);

    configASSERT(event_counter < 255);
    ++event_counter;

    handle->submitted = true;

    leave_critical_section();

    cpc_signal_event(CPC_SIGNAL_SYSTEM);

    return CPC_STATUS_OK;
}

void cpc_dispatcher_cancel(cpc_dispatcher_handle_t *handle)
{

    enter_critical_section();
    if (handle->submitted)
    {
        slist_remove(&process_queue, &handle->node);

        configASSERT(event_counter > 0);
        --event_counter;

        handle->submitted = false;
    }
    leave_critical_section();
}

void cpc_dispatcher_process(void)
{
    slist_node_t *node;
    cpc_dispatcher_handle_t *handle;
    uint8_t event_count_processed;
    uint8_t event_count_to_process;

    MCU_ATOMIC_LOAD(event_count_processed, event_counter);
    event_count_to_process = event_count_processed;

    do
    {
        MCU_ATOMIC_SECTION(node = slist_pop(&process_queue););
        if (node != NULL)
        {
            handle = SLIST_ENTRY(node, cpc_dispatcher_handle_t, node);
            handle->submitted = false;
            handle->fnct(handle->data);
        }
    } while (node != NULL && --event_count_to_process);

    // Update the global event_counter
    enter_critical_section();
    if (event_counter >= event_count_processed)
    {
        event_counter -= event_count_processed;
    }
    else
    {
        // If we are here, an event was cancelled when processing the queue
        event_counter = 0;
    }
    leave_critical_section();
}
