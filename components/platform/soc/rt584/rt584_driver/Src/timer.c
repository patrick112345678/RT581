/**************************************************************************//**
 * @file     timer.c
 * @version
 * @brief    TIMER driver file
 *
 * @copyright
 ******************************************************************************/

#include "RT584_cm33.h"

#define MAX_NUMBER_OF_TIMER       3         /*3 timer*/

/* Do we need isr_context?
 * We don't protect timer, so user application should design carefully.
 *
 */

#define TIMER_STATE_OPEN           1
#define TIMER_STATE_CLOSED         0

/* the timer_config_t is  8 bytes data structure.
 * timer_config_mode_t  mode is only 4 bits.
 */
typedef struct {
    timer_isr_handler_t  timer_handler;           /**< user application ISR handler. */
    timer_config_mode_t  cfg;
    uint8_t              state : 2;               /**< device state. */
} timer_config_t;

static timer_config_t   timer_cfg[MAX_NUMBER_OF_TIMER];

void Timer_Int_Callback_Register(uint32_t timer_id,
                                 timer_isr_handler_t timer_handler)
{
    timer_cfg[timer_id].timer_handler = timer_handler;

    return;
}

/*timer_config_mode_t is only 1 bytes, so we use it as parameter, instead of pointer*/
uint32_t Timer_Open(uint32_t timer_id,
                    timer_config_mode_t cfg,
                    timer_isr_handler_t timer_handler)
{
    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    if(timer_cfg[timer_id].state!=TIMER_STATE_CLOSED)
        return STATUS_INVALID_REQUEST;      /*device already opened*/

    /*Notice: timer_handler could be NULL when polled/freerun mode.*/
    
    /*
     * Because of watchdog reset, it is possible that "last time" timer interrupt pending
     * in system, so we should set callback before enable interrupt, otherwise when
     * we call NVIC_EnableIRQ(...), it will trigger interrupt at the same time and jump
     * an invalid address such that hardware fault generated.
     */
    Timer_Int_Callback_Register(timer_id, timer_handler);
    timer_cfg[timer_id].cfg = cfg;

    timer_cfg[timer_id].state = TIMER_STATE_OPEN;
    

    if(timer_handler != NULL) 
    {
        NVIC_EnableIRQ((IRQn_Type)(Timer0_IRQn + timer_id));
    }

    return STATUS_SUCCESS;
}

uint32_t Timer_Load(uint32_t timer_id, uint32_t timeout_ticks)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];

    timer->LOAD = (timeout_ticks-1);

    return STATUS_SUCCESS;
}

uint32_t Timer_Start(uint32_t timer_id, uint32_t timeout_ticks)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    if(timer_cfg[timer_id].state != TIMER_STATE_OPEN)
        return STATUS_NO_INIT;      /*DEVIC SHOULD BE OPEN FIRST.*/

    timer = base[timer_id];

    Timer_Load(timer_id, timeout_ticks);

    timer->CONTROL.bit.UP_CONUNT = 0;
    timer->CONTROL.bit.ONE_SHOT_EN = 0;
    timer->CONTROL.bit.INT_ENABLE = timer_cfg[timer_id].cfg.int_en;

    if(timer_cfg[timer_id].cfg.mode == TIMER_FREERUN_MODE) 
    {
        timer->CONTROL.bit.MODE = 0;
    } 
    else 
    {
        timer->CONTROL.bit.MODE = 1;
    }

    timer->CONTROL.bit.PRESCALE = timer_cfg[timer_id].cfg.prescale;
    timer->CONTROL.bit.EN = 1;

    return STATUS_SUCCESS;
}

uint32_t Timer_Stop(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    if(timer_cfg[timer_id].state != TIMER_STATE_OPEN)
        return STATUS_NO_INIT;      /*DEVIC SHOULD BE OPEN FIRST.*/

    timer = base[timer_id];
    timer->CONTROL.reg = 0;         /*Disable timer*/

    return STATUS_SUCCESS;
}

uint32_t Timer_Close(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
    timer->CONTROL.reg = 0;         /*Disable timer*/

    /*disable interrupt.*/    
    NVIC_DisableIRQ((IRQn_Type)(Timer0_IRQn + timer_id));

    timer_cfg[timer_id].timer_handler = NULL;

    timer_cfg[timer_id].state = TIMER_STATE_CLOSED;

    return STATUS_SUCCESS;
}

uint32_t Timer_Status_Get(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
    return (timer->CONTROL.bit.INT_STATUS);
}

uint32_t Timer_Current_Get(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->VALUE);
}

uint32_t Timer_Ch0_Capture_Value_Get(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CH0_CAP_VALUE);
}

uint32_t Timer_Ch0_Capture_Int_Status(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CONTROL.bit.CH0_CAPTURE_INT_STATUS);
}

uint32_t Timer_Ch1_Capture_Value_Get(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CH1_CAP_VALUE);
}

uint32_t Timer_Ch1_Capture_Int_Status(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CONTROL.bit.CH1_CAPTURE_INT_STATUS);
}

uint32_t get_timer_enable_status(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CONTROL.bit.TIMER_ENABLE_STATUS);
}

uint32_t get_timer32k_enable_status(uint32_t timer_id)
{
    timer32kn_t *timer;
    timer32kn_t *base[MAX_NUMBER_OF_TIMER] = {TIMER32K0, TIMER32K1};

    if(timer_id > MAX_NUMBER_OF_TIMER)
        return STATUS_INVALID_PARAM;

    timer = base[timer_id];
 
    return (timer->CONTROL.bit.TIMER_ENABLE_STATUS);
}


/*TIMER0 interrupt*/
void Timer0_Handler(void)
{
#ifdef DEBUG
    if(timer_cfg[0].timer_handler == NULL) {
        /*if timer_handler is NULL, it should not generate interrupt!*/
        while(1);
    }
#endif

    //TIMER0->CLEAR = 1;       /*clear interrupt*/

    if(timer_cfg[0].timer_handler != NULL) 
    {
        timer_cfg[0].timer_handler(0);
    }
    return;
}

/*TIMER1 interrupt*/
void Timer1_Handler(void)
{
    //TIMER1->CLEAR = 1;       /*clear interrupt*/

    if(timer_cfg[1].timer_handler != NULL) 
    {
        timer_cfg[1].timer_handler(1);
    }
    return;
}

/*TIMER2 interrupt*/
void Timer2_Handler(void)
{
    //TIMER2->CLEAR = 1;       /*clear interrupt*/

    if(timer_cfg[2].timer_handler != NULL) 
    {
        timer_cfg[2].timer_handler(2);
    }
    return;
}


