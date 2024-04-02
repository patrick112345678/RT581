#include "stdint.h"
#include "cm3_mcu.h"
#include "project_config.h"

#include "FreeRTOS.h"
#include "task.h"

#include "log.h"

#include "mem_mgmt.h"


#define MEM_MGMT_DEBUG_INFO_TASK_SIZE         20
#define MEM_MGMT_DEBUG_INFO_SIZE              100

typedef struct MEM_MGMT_DEBUG_LOG_ENTRY
{
    char *func;
    uint32_t file_line  : 16;   // max 0xFFFF = 65535
    uint32_t malloc_len : 16;   // max 0xFFFF = 65535

    uint32_t malloc_offset  : 19;   // max 256k = 0x40000(19 bits)
    uint32_t valid          : 1;
    uint32_t                : 4;
    uint32_t task_index     : 8;
} malloc_log_entry_t;

typedef struct MEM_MGMT_DEBUG_LOG
{
    char *task_name_ptr[MEM_MGMT_DEBUG_INFO_TASK_SIZE];
    uint32_t malloc_base_addr;
    malloc_log_entry_t entry[MEM_MGMT_DEBUG_INFO_SIZE];
} malloc_log_t;

static malloc_log_t g_malloc_log;

//==================================
//     Memory functions
//==================================
/** Task name pointer transfor to table index
 * @param task_name task name pointer
 * @return table index
 */
static uint8_t search_current_task_name_index(void)
{
    char *current_task_name = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
    uint32_t u32_idx;

    for (u32_idx = 0; u32_idx < MEM_MGMT_DEBUG_INFO_TASK_SIZE; u32_idx++)
    {
        if (g_malloc_log.task_name_ptr[u32_idx] == NULL)
        {
            g_malloc_log.task_name_ptr[u32_idx] = current_task_name;
            break;
        }
        else if (g_malloc_log.task_name_ptr[u32_idx] == current_task_name)
        {
            break;
        }
    }

    if (u32_idx < MEM_MGMT_DEBUG_INFO_TASK_SIZE)
    {
        return u32_idx;
    }
    else
    {
        return 0xFF;
    }
}

static void malloc_info_insert(void *p_mptr, uint32_t u32_msize, char *pc_func_ptr, uint32_t u32_line)
{
    uint32_t u32_idx;

    if (g_malloc_log.malloc_base_addr == 0)
    {
        g_malloc_log.malloc_base_addr = (uint32_t)p_mptr & 0xFFF00000;
    }

    for (u32_idx = 0; u32_idx < MEM_MGMT_DEBUG_INFO_SIZE; u32_idx++)
    {
        if (g_malloc_log.entry[u32_idx].valid == 0)
        {
            g_malloc_log.entry[u32_idx].func = pc_func_ptr;
            g_malloc_log.entry[u32_idx].file_line = u32_line & 0xFFFF;
            g_malloc_log.entry[u32_idx].malloc_len = u32_msize & 0xFFFF;
            g_malloc_log.entry[u32_idx].malloc_offset = (uint32_t)p_mptr & 0x7FFFF; // max 512k
            g_malloc_log.entry[u32_idx].task_index = search_current_task_name_index();
            g_malloc_log.entry[u32_idx].valid = 1;
            break;
        }
    }
}

static void malloc_info_delete(void *p_mptr)
{
    uint32_t u32_idx, malloc_offset;

    malloc_offset = (uint32_t)p_mptr & 0xFFFFF;

    for (u32_idx = 0; u32_idx < MEM_MGMT_DEBUG_INFO_SIZE; u32_idx++)
    {
        if (g_malloc_log.entry[u32_idx].valid)
        {
            if (g_malloc_log.entry[u32_idx].malloc_offset == malloc_offset)
            {
                memset(&g_malloc_log.entry[u32_idx], 0x0, sizeof(g_malloc_log.entry[u32_idx]));
                break;
            }
        }
    }
}

void mem_mgmt_show_info(void)
{
    uint32_t u32_idx;
    log_info("+------------+-------+---------------------------\n");
    log_info("|  Pointer   | Size  | Func-Line(Task)\n");
    log_info("+------------+-------+---------------------------\n");
    for (u32_idx = 0; u32_idx < MEM_MGMT_DEBUG_INFO_SIZE; u32_idx++)
    {
        if (g_malloc_log.entry[u32_idx].valid)
        {
            log_info("| 0x%08X | %05u | %s-%d(%s)\n",
                     g_malloc_log.malloc_base_addr | g_malloc_log.entry[u32_idx].malloc_offset,
                     g_malloc_log.entry[u32_idx].malloc_len,
                     g_malloc_log.entry[u32_idx].func,
                     g_malloc_log.entry[u32_idx].file_line,
                     g_malloc_log.task_name_ptr[g_malloc_log.entry[u32_idx].task_index]);
        }
    }
    log_info("+------------+-------+---------------------------\n");
}
/** Memory allocation
 * @param u32_size sleep time in milliseconds
 */
void *mem_malloc_fn(uint32_t u32_size, const char *pc_func_ptr, uint32_t u32_line)
{
    vTaskSuspendAll();
    void *ptr;
    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/
    if (u32_size == 0)
    {
        xTaskResumeAll();
        return NULL;
    }

    if (u32_size & portBYTE_ALIGNMENT_MASK)
    {
        u32_size += portBYTE_ALIGNMENT - (u32_size & portBYTE_ALIGNMENT_MASK);
    }

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    ptr = pvPortMalloc(u32_size);

    if (ptr)
    {
        malloc_info_insert(ptr, u32_size, (char *)pc_func_ptr, u32_line);
    }
    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    xTaskResumeAll();
    return ptr;
}

/** Memory free
 * @param p_pointer
 */
void mem_free_fn(void *p_pointer, const char *pc_func_ptr, uint32_t u32_line)
{
    vTaskSuspendAll();
    char *current_task_name = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());
    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/
    if (p_pointer == NULL)
    {
        log_info("%s-%d(%s) free null pointer\n",
                 pc_func_ptr, u32_line, current_task_name);
    }
    configASSERT(p_pointer != NULL);

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    vPortFree(p_pointer);

    malloc_info_delete(p_pointer);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    xTaskResumeAll();
}



/** Memory copy
 * @param p_pointer_dest, p_pointer_src, lens
 */
void mem_copy_fn(void *p_pointer_dest, const void *p_pointer_src, uint32_t lens, const char *pc_func_ptr, uint32_t u32_line)
{
    vTaskSuspendAll();
    char *d = p_pointer_dest;
    const char *s = p_pointer_src;
    while (lens--)
    {
        *d++ = *s++;
    }
    xTaskResumeAll();
}
