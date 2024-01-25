
#include "RT584_cm33.h"

/*different TRNG trigger reason */

#define  random_value_buffer_size    (16)

#define  random_bufsize_mask         (random_value_buffer_size-1)

#define  TRNG_UNITIAL                 0
#define  TRNG_RUNNING                 1
#define  TRNG_IDLE                    2

static uint32_t  random_value_buffer[random_value_buffer_size];

static volatile uint16_t  random_wr_idx;     /*Notice: this write index must be volatile*/
static volatile uint16_t  random_rd_idx;     /*Notice: this read index must be volatile*/

static volatile uint8_t   trng_state = TRNG_UNITIAL;



#define    TRNG_VON_NEUMANN           0
#define    TRNG_XOR_CORRECTOR         1

#define    TRNG_INT_ENABLE            (1<<1)

void random_number_reset(void)
{        
    /*check TRNG is busy or not --- in most case it should be idle*/
    while (TRNG_CTRL->TRNG2 & 0x1)
        ;
    
    random_wr_idx = 0;
    random_rd_idx = 0;
    
    NVIC_EnableIRQ(OTP_IRQn);
        
    trng_state = TRNG_RUNNING;
    
    TRNG_CTRL->TRNG1 = (TRNG_INT_ENABLE|TRNG_VON_NEUMANN);    /*select von Neumann */
    /*trigger TRNG Enable*/
    TRNG_CTRL->TRNG0 = 1;
    
}

/*
 *Notice: It should call random_number_init before call this function,
 *       
 */
uint32_t get_random_number(void)
{
    uint32_t  temp;
    
    if (trng_state == TRNG_UNITIAL)
    {
        random_number_reset();
    }
    else
    if ((random_wr_idx == random_rd_idx) && (trng_state == TRNG_IDLE))
    {   /*it should not happen this error case!!*/
        trng_state = TRNG_RUNNING;
        
        TRNG_CTRL->TRNG1 = (TRNG_INT_ENABLE|TRNG_VON_NEUMANN);    /*select von Neumann */
        /*trigger TRNG Enable*/
        TRNG_CTRL->TRNG0 = 1;
    }
        
    
    while (random_wr_idx==random_rd_idx)
        ;       /*wait TRGN generate new random number */
    
    /* 2023/01/06 consider one case... 
     * there is only one random_number entry in buffer
     * read here but still not update random_rd_idx
     * however, during this period TRNG interrupt happen
     * ISR check buffer full, so stop to generate next random number.
     * TRNG become idle. no new random number generated in background!!
     */  
    temp =  random_value_buffer[(random_rd_idx & random_bufsize_mask)];
    random_rd_idx++;
    
    /*ok now at least one free space for random_number*/
    
    if (trng_state == TRNG_IDLE)
    {
        trng_state = TRNG_RUNNING;
        
        TRNG_CTRL->TRNG1 = (TRNG_INT_ENABLE|TRNG_VON_NEUMANN);    /*select von Neumann */
        /*trigger TRNG Enable*/
        TRNG_CTRL->TRNG0 = 1;
    }
    
    return temp;
}

void Trng_Handler(void)
{
    uint16_t      data_in_fifo, freespace;

    random_value_buffer[(random_wr_idx & random_bufsize_mask)] = TRNG_CTRL->TRNG3;
    random_wr_idx++;
    
    TRNG_CTRL->TRNG0 = 0x2;       /*IRQ interrupt clear*/
    
    data_in_fifo = (random_wr_idx - random_rd_idx) & random_bufsize_mask;
    freespace = random_bufsize_mask - data_in_fifo;

    if (freespace==0)
    {   /*buffer full*/
        trng_state = TRNG_IDLE;
        return;
    }
    else
    {   /*try to get next random value in buffer*/

        /*wait interrupt status clear*/
        while (TRNG_CTRL->TRNG2 & 2)
            ;
                
        /*trig next TRNG random number*/
        TRNG_CTRL->TRNG0 = 1; 
    } 
}
