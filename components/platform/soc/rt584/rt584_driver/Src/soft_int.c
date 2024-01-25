
#include "RT584_cm33.h"

/**
 * IMPORTANT: 
 *   This driver model is designed for secure world to notify non-secure 
 * world some events happen.
 *   So all software int module is running in non-secure.
 * 
 *   In this driver, we don't consider multi-thread to register a soft 
 * Interrupt. Customer should design software interrupt careful.
 *
 */

#define MAX_NUMBER_OF_SOFT_INT   2

static soft_int_cb_t soft_int_cb[MAX_NUMBER_OF_SOFT_INT] = {NULL, NULL};

uint32_t Register_Soft_Intr(uint32_t id, soft_int_cb_t soft_cb_fun)
{
    if ((id >= MAX_NUMBER_OF_SOFT_INT) || (soft_cb_fun==NULL))
        return STATUS_INVALID_PARAM;
    
    /*NOTICE: soft_cb_fun must be in non-secure.*/
    
    if (soft_int_cb[id] ==NULL)
    {
        soft_int_cb[id] = soft_cb_fun;        
        NVIC_EnableIRQ((IRQn_Type)(Soft0_IRQn + id));
        /*TODO: priority setting*/
    }
    else
    {
        return STATUS_INVALID_REQUEST;      /*device already opened*/
    }
    
    return STATUS_SUCCESS;     
}

uint32_t Enable_Soft_Intr(uint32_t id)
{
    if (id == 0)
    {
        SW_INT0->ENABLE_IRQ = ENABLE_SOFT_IRQ;
    }
    else
    if (id == 1)
    {
        SW_INT1->ENABLE_IRQ = ENABLE_SOFT_IRQ;
    }
    else
        return STATUS_INVALID_PARAM;
    
    return STATUS_SUCCESS;    
}

uint32_t Clear_Soft_Intr(uint32_t id)
{
    if (id == 0)
    {
        SW_INT0->CLEAR_IRQ = CLEAR_SOFT_IRQ;
    }
    else
    if (id == 1)
    {
        SW_INT1->CLEAR_IRQ = CLEAR_SOFT_IRQ;
    }
    else
        return STATUS_INVALID_PARAM; 
    
    return STATUS_SUCCESS;
}


uint32_t Get_Soft_Intr_Data(uint32_t id, uint32_t *data)
{
    if (id == 0)
    {
        *data = SW_INT0->DATA;
    }
    else
    if (id == 1)
    {
        *data = SW_INT1->DATA;
    }
    else
        return STATUS_INVALID_PARAM;
    
    return STATUS_SUCCESS;
}

uint32_t Set_Soft_Intr_Data(uint32_t id, uint32_t data)
{
    if (id == 0)
    {
        SW_INT0->DATA = data;
    }
    else 
    if (id == 1)
    {
        SW_INT1->DATA = data;
    }
    else
        return STATUS_INVALID_PARAM;
    
    return STATUS_SUCCESS;
}

void Soft0_Handler(void)
{
    soft_int_cb_t  cb_isr_fun;
    
    SW_INT0->CLEAR_IRQ = CLEAR_SOFT_IRQ;
    
    cb_isr_fun = soft_int_cb[0];
    
    if (cb_isr_fun !=NULL)
    {   /*
         * call register Software Interrupt callback
         * Please Notice: the CB is ISR context switch too!
         */
        cb_isr_fun(SW_INT0->DATA);
    }
}

void Soft1_Handler(void)
{
    soft_int_cb_t  cb_isr_fun;
    
    SW_INT1->CLEAR_IRQ = CLEAR_SOFT_IRQ;
    
     cb_isr_fun = soft_int_cb[1];
     
    if (cb_isr_fun !=NULL)
    {   /*
         * call register Software Interrupt callback
         * Please Notice: the CB is ISR context switch too!
         */
        cb_isr_fun(SW_INT1->DATA);
    }    
}

