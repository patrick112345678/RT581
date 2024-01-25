/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * http://www.dsr-zboss.com
 * http://www.dsr-corporation.com
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
/* PURPOSE: Platform specific
*/

#define ZB_TRACE_FILE_ID 8000

#include "zb_led_button.h"
#include "zb_osif.h"
#include <stdio.h>              /* for __write */

#ifndef PRINTF_USE_RT58X_LIB
#include "bsp.h"

#include "util_printf.h"
#include "util_log.h"
#endif

#include "rf_mcu.h"

#ifdef ZB_TRACE_OVER_USART
#include "zb_stm_serial.h"
#endif /* ZB_TRACE_OVER_USART */



#include "zb_time.h"

struct zb_globals_s;
typedef struct zb_globals_s zb_globals_t;

struct zb_intr_globals_s;
typedef ZB_VOLATILE struct zb_intr_globals_s zb_intr_globals_t;

extern zb_globals_t g_zb;
extern zb_intr_globals_t g_izb;

/**
   Macro to access globals
 */
/* Hope compiler can optimize &g_zb-> to g_zb. */
#define ZG (&g_zb)
#define ZIG (&g_izb)

/**
   Global data area for data to be accessed from interrupt handlers
 */
struct zb_intr_globals_s
{
#ifdef ZB_HAVE_IOCTX
    zb_io_ctx_t             ioctx;
#endif
#if defined( ENABLE_USB_SERIAL_IMITATOR )
    zb_usbc_ctx_t           usbctx; /*!< USB imitator IO context. */
#endif /* defined( ENABLE_USB_SERIAL_IMITATOR ) */
    zb_timer_t              time;
};

#define ZB_IOCTX() g_izb.ioctx
#define ZB_TIMER_CTX() g_izb.time
#define SER_CTX() ZB_IOCTX().serial_ctx

#define MAX_NUMBER_OF_TIMER       5         /*RT58x has 5 timer*/

#if !defined ZB_SOFT_SECURITY || defined ZB_HW_ZB_AES128
extern void zb_osif_hw_aes_init(void);
#endif
extern uint32_t console_drv_init(uart_baudrate_t baudrate);

#define SYSTICKCLOCK      (32l * 1000 * 1000)     /* SysTic clock freq, Hz - 32MHz */
#define SYSTICKCLOCKINIT(tick)  ((tick/1000000) * ZB_BEACON_INTERVAL_USEC)

#ifdef ZB_TRACE_OVER_USART
#define BAUD_RATE_115200    115200

void zb_app_uart_isr(void);

#endif /* ZB_TRACE_OVER_USART */

static void zb_osif_timer_init(void);

void led_hw_init(void)
{
#if (BOARD==A047) || (BOARD==A048)
    pin_set_mode(LED0, MODE_GPIO);
    pin_set_mode(LED1, MODE_GPIO);
    pin_set_mode(LED2, MODE_GPIO);
    pin_set_mode(LED3, MODE_GPIO);

    gpio_cfg_output(LED0);
    gpio_cfg_output(LED1);
    gpio_cfg_output(LED2);
    gpio_cfg_output(LED3);

#if (BOARD==A048)
    gpio_pin_set(LED0);
    gpio_pin_set(LED1);
    gpio_pin_set(LED2);
    gpio_pin_set(LED3);
#else
    gpio_pin_clear(LED0);
    gpio_pin_clear(LED1);
    gpio_pin_clear(LED2);
    gpio_pin_clear(LED3);
#endif

#else
    pin_set_mode(LED0, MODE_GPIO);
    pin_set_mode(LED1, MODE_GPIO);
    pin_set_mode(LED2, MODE_GPIO);
    pin_set_mode(LED3, MODE_GPIO);
    pin_set_mode(LED4, MODE_GPIO);
    pin_set_mode(LED5, MODE_GPIO);
    pin_set_mode(LED6, MODE_GPIO);
    pin_set_mode(LED7, MODE_GPIO);

    gpio_cfg_output(LED0);
    gpio_cfg_output(LED1);
    gpio_cfg_output(LED2);
    gpio_cfg_output(LED3);
    gpio_cfg_output(LED4);
    gpio_cfg_output(LED5);
    gpio_cfg_output(LED6);
    gpio_cfg_output(LED7);

    gpio_pin_set(LED0);
    gpio_pin_set(LED1);
    gpio_pin_set(LED2);
    gpio_pin_set(LED3);
    gpio_pin_set(LED4);
    gpio_pin_set(LED5);
    gpio_pin_set(LED6);
    gpio_pin_set(LED7);
#endif
    return;
}


void hw_key_isr(uint32_t pin, void *isr_param)
{
    zb_uint8_t ui8KeysPressed;
    zb_uint8_t pinHighLow = 0;

    //
    // Get bitmask of buttons pushed (clear directional keys' bitmask)
    //
    ui8KeysPressed = pin;

    //
    // Determine which LEDs to toggle
    //
    if (ui8KeysPressed == KEY0)
    {
#if (BOARD==A047)
        pinHighLow = gpio_pin_get(KEY0);
        if (pinHighLow == 0)
        {
            zb_button_on_cb(0);
        }
        else
        {
            zb_button_off_cb(0);
        }
#else
        zb_button_on_cb(0);
        zb_button_off_cb(0);
#endif
    }
    if (ui8KeysPressed == KEY1)
    {
#if (BOARD==A047)
        pinHighLow = gpio_pin_get(KEY1);
        if (pinHighLow == 0)
        {
            zb_button_on_cb(1);
        }
        else
        {
            zb_button_off_cb(1);
        }
#else
        zb_button_on_cb(1);
        zb_button_off_cb(1);
#endif

    }
    if (ui8KeysPressed == KEY2)
    {
        zb_button_on_cb(2);
        zb_button_off_cb(2);
    }
    if (ui8KeysPressed == KEY3)
    {
        zb_button_on_cb(3);
        zb_button_off_cb(3);
    }

#if (BOARD==A015)
    if (ui8KeysPressed == KEY4)
    {
        zb_button_on_cb(4);
        zb_button_off_cb(4);
    }
#endif

}

void key_hw_init(void)
{
#if (BOARD==A047) || (BOARD==A048)
    pin_set_mode(KEY0, MODE_GPIO);
    pin_set_mode(KEY1, MODE_GPIO);
    pin_set_mode(KEY2, MODE_GPIO);
    pin_set_mode(KEY3, MODE_GPIO);

#if (BOARD==A047)
    gpio_cfg_input(KEY0, GPIO_PIN_INT_BOTH_EDGE);
    gpio_cfg_input(KEY1, GPIO_PIN_INT_BOTH_EDGE);
#endif

#if (BOARD==A048)
    gpio_cfg_input(KEY0, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY1, GPIO_PIN_INT_EDGE_RISING);
#endif

    gpio_cfg_input(KEY2, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY3, GPIO_PIN_INT_EDGE_RISING);

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_512);

    gpio_debounce_enable(KEY0);
    gpio_debounce_enable(KEY1);
    gpio_debounce_enable(KEY2);
    gpio_debounce_enable(KEY3);

    gpio_register_isr(KEY0, hw_key_isr, NULL);
    gpio_register_isr(KEY1, hw_key_isr, NULL);
    gpio_register_isr(KEY2, hw_key_isr, NULL);
    gpio_register_isr(KEY3, hw_key_isr, NULL);

    gpio_int_enable(KEY0);
    gpio_int_enable(KEY1);
    gpio_int_enable(KEY2);
    gpio_int_enable(KEY3);
#else
    pin_set_mode(KEY0, MODE_GPIO);
    pin_set_mode(KEY1, MODE_GPIO);
    pin_set_mode(KEY2, MODE_GPIO);
    pin_set_mode(KEY3, MODE_GPIO);
    pin_set_mode(KEY4, MODE_GPIO);
    pin_set_mode(KEY5, MODE_GPIO);
    pin_set_mode(KEY6, MODE_GPIO);
    pin_set_mode(KEY7, MODE_GPIO);

    gpio_cfg_input(KEY0, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY1, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY2, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY3, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY4, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY5, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY6, GPIO_PIN_INT_EDGE_RISING);
    gpio_cfg_input(KEY7, GPIO_PIN_INT_EDGE_RISING);

    gpio_register_isr(KEY0, hw_key_isr, NULL);
    gpio_register_isr(KEY1, hw_key_isr, NULL);
    gpio_register_isr(KEY2, hw_key_isr, NULL);
    gpio_register_isr(KEY3, hw_key_isr, NULL);
    gpio_register_isr(KEY4, hw_key_isr, NULL);
    gpio_register_isr(KEY5, hw_key_isr, NULL);
    gpio_register_isr(KEY6, hw_key_isr, NULL);
    gpio_register_isr(KEY7, hw_key_isr, NULL);

    gpio_set_debounce_time(DEBOUNCE_SLOWCLOCKS_512);

    gpio_debounce_enable(KEY0);
    gpio_debounce_enable(KEY1);
    gpio_debounce_enable(KEY2);
    gpio_debounce_enable(KEY3);
    gpio_debounce_enable(KEY4);
    gpio_debounce_enable(KEY5);
    gpio_debounce_enable(KEY6);
    gpio_debounce_enable(KEY7);

    gpio_int_enable(KEY0);
    gpio_int_enable(KEY1);
    gpio_int_enable(KEY2);
    gpio_int_enable(KEY3);
    gpio_int_enable(KEY4);
    gpio_int_enable(KEY5);
    gpio_int_enable(KEY6);
    gpio_int_enable(KEY7);
#endif
    return;
}


/*this is pin mux setting*/
void init_default_pin_mux(void)
{
    int i;

    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/

    return;
}

extern void mac_rt570_hw_init(void);

void zb_rt570_init(void)
{
    /*we should set pinmux here or in SystemInit */
    init_default_pin_mux();

    dma_init();
    RfMcu_DmaInit();
    zb_osif_led_button_init();
    mac_rt570_hw_init();
    timer_set();
    zb_osif_timer_init();
}

void zb_reset(zb_uint8_t param)
{

}



/**
   Return random seed initializer for random number generator
 */
zb_uint32_t zb_random_seed(void)
{
    return get_random_number();
}

zb_uint32_t zb_get_utc_time()
{
    return 0;
}


#if defined ZB_TRACE_LEVEL && defined ZB_TRACE_OVER_JTAG
size_t __write(int handle, const unsigned char *buffer, size_t size);

/* trace over jtag */
void zb_osif_serial_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
    __write(1, buf, len);
}

void zb_osif_serial_init()
{
}
#endif


/* Timer */

/**
   timer interrupt handler
 */
static void timer_inter_handler(uint32_t timer_id)
{
    zb_time_t timer, stop_timer;

    ZB_TIMER_CTX().timer++;

    /* assign to prevent warnings in IAR */
    timer = ZB_TIMER_CTX().timer;
    stop_timer = ZB_TIMER_CTX().timer_stop;

    /* Stop timer if it expired or not running. */
    /*
    if (!ZB_TIMER_CTX().started ||
       ZB_TIME_GE(timer, stop_timer))
    {
    ZB_STOP_HW_TIMER();
    ZB_TIMER_CTX().timer_stop = ZB_TIMER_CTX().timer;
    ZB_TIMER_CTX().started = 0;
    }
    else
    */
    {
        ZB_START_HW_TIMER();
    }

}

void timer_dlyint_cfg(uint32_t timer_id, uint32_t Timer_RepDlyInt)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2, TIMER3, TIMER4};

    timer = base[timer_id];

    timer->REPEAT_DELAY.reg = Timer_RepDlyInt;
}

uint32_t timer_status(uint32_t timer_id)
{
    timern_t *timer;
    timern_t *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2, TIMER3, TIMER4};

    timer = base[timer_id];

    if (timer->CONTROL.bit.EN == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void timer_set(void)
{
    timer_config_mode_t cfg;
    uint8_t timerID = 0;

    if ( UsedTimer == Timer0) //use timer0 32Mhz clock
        /*the input clock is 32M/s, so it will become 1M ticks per second */
    {
        cfg.prescale = TIMER_PRESCALE_32;
    }
    else if (UsedTimer == Timer3) //use timer3 32Khz clock to suport power saving mode
        /*the input clock is 32K/s, so it will become 4K ticks per second */
    {
        cfg.prescale = TIMER_PRESCALE_8;
    }


    cfg.mode = TIMER_PERIODIC_MODE;
    cfg.int_en = TRUE;

    Timer_Open(UsedTimer, cfg, timer_inter_handler);

}

void timer_set_for_sleep(uint32_t tick)
{
    //the unit of tick(sleep time) is mS, RT58x used timer3 as the sleep timer
    //the currently timer3 is 4K ticks per second, 1ms as 4 timer tick
    Timer_Start(Timer3, tick * 4);
}



static void zb_osif_timer_init(void)
{

    ZB_START_HW_TIMER();
}


zb_time_t osif_sub_trans_timer(zb_time_t t2, zb_time_t t1)
{
    return 0;
}

void osif_sleep_using_transc_timer(zb_time_t timeout)
{

}

int SysTickIsEnabled(void)
{
    return 0;
}

#if defined ZB_TRACE_LEVEL && defined ZB_SERIAL_FOR_TRACE

#define GPIO0   0
#define GPIO1   1
#define GPIO2   2
#define GPIO3   3
#define GPIO11  11
#define GPIO12  12
#define GPIO13  13
#define GPIO14  14
#define GPIO28  28
#define GPIO29  29

#define rxbufsize   512
#define txbufsize   512


#ifdef PRINTF_USE_RT58X_LIB
static void uart0_callback(uint32_t event, void *p_context)
{
    /*Notice:
        UART_EVENT_TX_DONE  is for asynchronous mode send
        UART_EVENT_RX_DONE  is for synchronous  mode receive
     */
    if (event & UART_EVENT_TX_DONE)
    {
        /*if you use multi-tasking, signal the waiting task here.*/
        SER_CTX().tx_in_progress = 0;
    }

    if (event & UART_EVENT_RX_DONE)
    {
        /*if you use multi-tasking, signal the waiting task here.*/
    }

    if (event & (UART_EVENT_RX_OVERFLOW | UART_EVENT_RX_BREAK |
                 UART_EVENT_RX_FRAMING_ERROR | UART_EVENT_RX_PARITY_ERROR ))
    {

        //it's almost impossible for those error case.
        //do something ...

    }

}

void  init_debug_console(void)
{
    uint32_t status = STATUS_SUCCESS;
    uart_config_t  uart0_drv_config;
    uint32_t  handle;
    static uint8_t            tempbuffer[4];

    /*init uart0, 115200, 8bits 1 stopbit, none parity, no flow control.*/
    uart0_drv_config.baudrate = UART_BAUDRATE_115200;
    uart0_drv_config.databits = UART_DATA_BITS_8;
    uart0_drv_config.hwfc     = UART_HWFC_DISABLED;
    uart0_drv_config.parity   = UART_PARITY_NONE;

    /* Important: p_contex will be the second parameter in uart callback.
     * In this example, we do NOT use p_context, (So we just use handle for sample)
     * but you can use it for whaterever you want.
     */
    handle = 0;
    uart0_drv_config.p_context = (void *) handle;

    uart0_drv_config.stopbit  = UART_STOPBIT_ONE;
    uart0_drv_config.interrupt_priority = IRQ_PRIORITY_NORMAL;

    uart_init(0, &uart0_drv_config, uart0_callback);


    if (status != STATUS_SUCCESS)
    {
        while (1);
    }

    /*uart device is auto power on in uart_init function */
    uart_rx(0, tempbuffer, 1);

}
#else
static void uart0_callback(char ch)
{

}

#endif

/* trace over uart */
void zb_osif_serial_init()
{

    /*init debug uart port for printf*/
#ifdef PRINTF_USE_RT58X_LIB
    console_drv_init(UART_BAUDRATE_115200);
#else
    //rt58x_bsp_init();
#endif
    //init_debug_console();

    //printf("UART init done!\n", temp);
}

/* trace over uart */
extern void transmit_chars(uint32_t id);
int ZBOSS_Trace(char *ptr, int len);
void zb_osif_serial_put_bytes(const zb_uint8_t *buf, zb_short_t len)
{
    ZBOSS_Trace((char *)buf, len);
}

void rt_uart_transmit_chars(uint32_t id)
{
    /*Notice: this function is in ISR.*/
    //uart_handle_t *phandle;
    //UART_T     *uart;

    //phandle = &(m_uart_handle[id]);
    //uart = phandle->uart;

    /* Fill fifo with bytes from buffer */
    zb_uint8_t volatile *p = ZB_RING_BUFFER_PEEK(&SER_CTX().tx_buf);
    if ( p )
    {
        //uart->THR = *p;
        SER_CTX().tx_in_progress = 1;
        ZB_RING_BUFFER_FLUSH_GET(&SER_CTX().tx_buf);
    }
    else
    {
        SER_CTX().tx_in_progress = 0;
    }
}


/*Notice: this function is ISR, too.*/
#if 0
void rt_uart_receive_chars(uint32_t id)
{
    uart_handle_t *phandle;
    UART_T     *uart;


    phandle = &(m_uart_handle[id]);

    uart = phandle->uart;

    //
    // Put received bytes into callback
    //
    if (SER_CTX().byte_received_cb)
    {
        while (uart->LSR & UART_LSR_DR)
        {
            SER_CTX().byte_received_cb(uart->RBR);
        }
    }
    else
    {
        uint8_t temp = 0;
        temp += uart->RBR;
    }

}
#endif

/**
  * @brief  This function handles USARTx global interrupt request.
  * @param  None
  * @retval None
  */
void zb_app_uart_isr(void)
{

}

void zb_osif_set_uart_byte_received_cb(zb_callback_t cb)
{

}

#endif /* ZB_TRACE_OVER_USART */



/* Dummy implementattion for sleep. TODO: implement. */
extern void RT570_sleep_set(void);
extern void RT570_idle_set(void);

zb_uint32_t zb_osif_sleep(zb_uint32_t sleep_tmo)
{
    zb_uint32_t slept_time;

    RT570_sleep_set();

    //gpio_pin_clear(LED1);

    if (sleep_tmo > 0x3FFFFFFF)
    {
        sleep_tmo = 0x3FFFFFFF;
    }
    timer_set_for_sleep(sleep_tmo);

    timer_dlyint_cfg(Timer3, 0);

    //going to sleep
    outp32(0x40800000, 0x1);
    __WFI();

    //system wakeup
    slept_time = Timer_Current_Get(Timer3);
    RT570_idle_set();
    //gpio_pin_set(LED1);
    ZB_START_HW_TIMER();

    if (slept_time == 0)
    {
        return sleep_tmo;
    }
    else
    {
        return slept_time / 4;    //transfer timer ticks to ms
    }
}

void zb_osif_wake_up()
{
    aes_fw_init();
}
